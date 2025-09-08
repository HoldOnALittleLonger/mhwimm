#include "mhwimmc_database_thread.h"
#include "mhwimmc_database.h"
#include "mhwimmc_sync_mechanism.h"

#include <chrono>
#include <iostream>
#include <cstdbool>

/* ins_date -  used  by ADD operation for field "install_date" */
static std::time_t ins_date(0);

/* is_db_op_succeed - used to tell CMD module whether DB operation is succeed */
bool is_db_op_succeed(false);

/* used to indicates what field the Executor needs. */
mhwimm_db_ns::interest_db_field_t interest_field(mhwimm_db_ns::INTEREST_FIELD::NO_INTEREST);

/* reg helper will setup this pointer variable */
mhwimm_sync_mechanism_ns::mod_files_list *mfl_for_db(nullptr);

// caller must ensures @db been opened
// we do not check it at there
/**
 * mhwimm_db_thread_worker - DB module control thread
 * @db:                      db handler passed by caller
 */
void mhwimmc_db_thread_worker(mhwimm_db_ns::mhwimm_db &db)
{
  {
    auto db_file_path(db.returnDBpath());
    struct stat the_stat = {0};
    bool need_create_table(false);
    if (stat(db_file_path.c_str(), &the_stat) < 0)
      need_create_table = true;

    std::string err_msg;
    if (db.openDB() < 0) {
      db.getErrMsg(err_msg);
      std::cerr << err_msg << std::endl;
      std::abort(); /* fatal error */
    }

    if (need_create_table) {
      /**
       * we'll create table at the first time to launch this application.
       */
      db.tryCreateTable();
      if (db.getDBStatus() == mhwimm_db_ns::DB_STATUS::DB_ERROR) {
        /* we failed to create table */
        db.getErrMsg(err_msg);
        std::cerr << err_msg << std::endl;
        std::abort(); /* fatal error */
      }
    }
  }

  auto do_DB_ask = [&, interest_field, mfl_for_db](void) -> void {
    std::unique_lock<decltype(mfl_for_db->lock)> mfl_lock(&mfl_for_db->lock);
    int ret(0);

    mhwimm_db_ns::db_tr_idx name_idx(mhwimm_db_ns::db_tr_idx::IDX_MOD_NAME);
    mhwimm_db_ns::db_tr_idx path_idx(mhwimm_db_ns::db_tr_idx::IDX_FILE_PATH);

    /* prepare to get result */
  repeat_get:
    ret = db.executeDBOperation();

    if (!ret && db.getCurrentStatus() == mhwimm_db_ns::DB_STATUS::DB_WORKING) {

      if (interest_field == mhwimm_db_ns::INTEREST_FIELD::INTEREST_NAME) {
        std::string mod_name;
        ret = db.getFieldValue(name_idx, mod_name);
        if (ret < 0)
          goto err_getField;
        mfl_for_db->mod_name_list.insert(mfl_for_db->mod_name_list.begin(), mod_name);
        goto repeat_get;
      } else if (interest_field == mhwimm_db_ns::INTEREST_FIELD::INTEREST_PATH) {
        std::string file_path;
        struct stat the_stat = {0};
        ret = db.getFieldValue(path_idx, file_path);
        if (ret)
          goto err_getField;

        ret = stat(file_path.c_str(), &the_stat);
        if (ret < 0) {
          std::string err_msg = std::string{"error: file - "} + file_path + " does not exist.";
          std::cerr << err_name << std::endl;
          db.chgDBStatus(mhwimm_db_ns::DB_STATUS::DB_ERROR);
          return;
        } else if (S_ISDIR(the_stat.st_mode)) {
          mfl_for_db->directory_list.insert(mfl_for_db->directory_list.begin(), file_path);
        } else {
          mfl_for_db->regular_file_list.insert(mfl_for_db->regular_file_list.begin(), file_path);
        }
        goto repeat_get;
      }
      else {
        std::string err_smg("error: this regDBop has not be implemented.");
        std::cerr << err_msg << std::endl;
        db.chgDBStatus(mhwimm_db_ns::DB_STATUS::DB_ERROR);
        return;
      }

    } else if (!ret) {
      /**
       * no more result can be got,method executeDBOperation() returned _zero_,
       * and in this case,db status must be DB_IDLE.
       */
      if (interest_field == mhwimm_db_ns::INTEREST_FIELD::INTEREST_NAME)
        mfl_for_db->mod_name_list.reverse();
      else {
        mfl_for_db->directory_list.reverse();
        mfl_for_db->regular_file_list.reverse();
      }
    }

    /* error detected when executing DB operation */
  err_executeDB:

  err_getField:
    std::string err_msg;
    db.getErrMsg(err_msg);
    std::cerr << err_msg << std::endl;
  };

  /* ADD - no result return */
  auto do_DB_add = [&, mfl_for_db](void) -> void {
    ins_date = std::system_clock::to_time_t(std::system_clock::now());
    std::string date(ctime(&ins_date));
    std::string date = date.substr(0, date.length() - 1);
    
    /* because regDBop been specified,we have retrieve it */
    mhwimm_db_ns::db_table_record dtr = {
      .is_mod_name_set = 1,
      .mod_name = db.currentSelectedModName(),
      .is_install_date_set = 1,
      .install_date = date,
    };

    std::unique_lock<decltype(mfl_for_db->lock)> mfl_lock(&mfl_for_db->lock);

    /* next,process two cycle for traverse file list */

    dtr.is_file_path_set = 1;
    /* step1 : import directory list */
    for (auto e : mfl_for_db->directory_list) {
      dtr.file_path = e;

      db.registerDBOperation(mhwimm_db_ns::SQL_OP::SQL_ADD, dtr);
      db.executeDBOperation();
      
      if (db.getCurrentStatus() == mhwimm_db_ns::DB_STATUS::DB_ERROR)
        goto err_exit_with_undo;
    }

    /* step2 : import regular file list */
    for (auto e : mfl_for_db->regular_file_list) {
      dtr.file_path = e;

      db.registerDBOperation(mhwimm_db_ns::SQL_OP::SQL_ADD, dtr);
      db.executeDBOperation();

      if (db.getCurrentStatus() == mhwimm_db_ns::DB_STATUS::DB_ERROR)
        goto err_exit_with_undo;
    }
    return;

  err_exit_with_undo:
    /* we encountered error when import record into db */
    std::string err_msg;
    db.getErrMsg(err_msg);
    std::cerr << err_msg << std::endl;

    /* delete all records of current mod from db */
    dtr.is_file_path_set = 0;
    dtr.is_install_date_set = 0;
    db.registerDBOperation(mhwimm_db_ns::SQL_OP::SQL_DEL, dtr);
    db.executeDBOperation();

    if (db.getCurrentStatus() == mhwimm_db_ns::DB_STATUS::DB_ERROR) {
      db.getErrMsg(err_msg);
      std::cerr << err_msg << std::endl;
    }

    /* tell Thread Worker we encountered error */
    db.chgDBStatus(mhwimm_db_ns::DB_STATUS::DB_ERROR);
  };

  /* DEL - no result return */
  auto do_DB_del = [&](void) -> void {
    /* actually,we just invoke method executeDBOperation() as well */
    /* because the regDBop helper been registered OP and DTR filter */

    db.executeDBOperation();
    if (db.getCurrentStatus() == mhwimm_db_ns::DB_STATUS::DB_ERROR) {
      std::string err_msg;
      db.getErrMsg(err_msg);
      std::cerr << err_msg << std::endl;
    }
  };


  for (; ;) {
    if (program_exit)
      break;

    db.resetDB();

    std::unique_lock<decltype(exedb_sync_mutex)> exedb_lock(&exedb_sync_mutex);

    // DB operation will be registered by Executor via call to register helpers.
    
    switch (db.getCurrentOP()) {
    case mhwimm_db_ns::SQL_OP::SQL_ASK:
      do_DB_ask();
      break;
    case mhwimm_db_ns::SQL_OP::SQL_ADD:
      do_DB_add();
      break;
    case mhwimm_db_ns::SQL_OP::SQL_DEL:
      do_DB_del();
      break;
    default:
      continue; /* un-supported command or SQL_NOP */
    }

    if (db.getCurrentStatus() == mhwimm_db_ns::DB_STATUS::DB_ERROR)
      is_db_op_succeed = false;
    is_db_op_succeed = true;
  }
}
