#include "mhwimmc_database_thread.h"
#include "mhwimmc_database.h"
#include "mhwimmc_sync_types.h"

#include <chrono>
#include <iostream>
#include <cstdbool>

/* pDBCommit_modname - used to register mod name */
static const std::string *pDBCommit_modname(nullptr);

/* conspDBCommit_ADD_records - used to register mod filepaths list */
static const std::list<std::string> *conspDBCommit_ADD_DEL_records(nullptr);

/* pDBCommit_ASK_RetBuf - used to register the buffer for stores data got from DB */
static std::list<std::string> *pDBCommit_ASK_RetBuf(nullptr);

/* ASK_req / ADD_req / DEL_req - what kind of OP been committed */
static uint8_t ASK_req(0);
static uint8_t ADD_req(0);
static uint8_t DEL_req(0);

/* ins_date -  used  by ADD operation for field "install_date" */
static std::time_t ins_date(0);

/* is_db_op_succeed - used to tell CMD module whether DB operation is succeed */
bool is_db_op_succeed(false);

using mhwimmc_sync_type_ns::program_exit, mhwimmc_sync_type_ns::cdbmutex;

int commitASKRequest(const std::string &mod_name, std::list<std::string> &path_buf)
{
  pDBCommit_modname = &mod_name;
  pDBCommit_ASK_RetBuf = &path_buf;
  ASK_req = 1;
  ADD_req = 0;
  DEL_req = 0;
  return 0;
}

int commitADDRequest(const std::string &mod_name, const std::list<std::string> &path_list)
{
  pDBCommit_modname = &mod_name;
  conspDBCommit_ADD_DEL_records = &path_list;
  ADD_req = 1;
  ASK_req = 0;
  DEL_req = 0;
  return 0;
}

int commitDELRequest(const std::string &mod_name, const std::list<std::string> &path_list)
{
  pDBCommit_modname = &mod_name;
  conspDBCommit_ADD_DEL_records = &path_list;
  DEL_req = 1;
  ADD_req = 0;
  ASK_req = 0;
  return 0;
}



/**
 * do_db_add_request - do the real DB operation ADD / DEL
 * @db:                db handler
 * @add_or_del:        add or delete
 */
static void do_db_add_del_request(mhwimmc_db_ns::mhwimmc_db &db, int add_or_del)
{
  mhwimmc_db_ns::db_table_record record;

  record.mod_name = *pDBCommit_modname;
  record.mod_name_is_set = 1;
  record.mod_path_is_set = 1;
  record.install_date_is_set = 0;

  if (add_or_del) {
    record.install_date = std::ctime(ins_date);
    record.install_date_is_set = 1;
  }
  mhwimmc_db_ns::SQL_OP op(add_or_del ? mhwimmc_db_ns::SQL_OP::SQL_ADD :
                           mhwimmc_db_ns::SQL_OP::SQL_DEL);

  is_db_op_succeed = true;
  for (auto r : *conspDBCommit_ADD_DEL_records) {
    record.mod_path = r;
    db.registerDBOperation(op, record);
    if (db.executeDBOperation() < 0) {
      std::string db_err_msg;
      db.getDBErrMsg(db_err_msg);
      std::cerr << "db_module: error: OP = " << add_or_del ? "ADD " : "DEL " << db_err_msg << std::endl;
      is_db_op_succeed = false;
      break;
    }
  }
}

static inline void do_db_add_request(mhwimmc_db_ns::mhwimmc_db &db)
{
  do_db_add_del_request(db, 1);
}

static inline void do_db_del_request(mhwimmc_db_ns::mhwimmc_db &db)
{
  do_db_add_del_request(db, 0);
}

/**
 * do_db_ask_request - do the real DB operation ASK and fill up return buffer
 * @db:                db handler
 */
static void do_db_ask_request(mhwimmc_db_ns::mhwimmc_db &db)
{
  mhwimmc_db_ns::db_table_record record;

  record.mod_name = *pDBCommit_modname;
  record.mod_name_is_set = 1;

  record.mod_path_is_set = 0;
  record.install_date_is_set = 0;

  db.registerDBOperation(mhwimmc_db_ns::SQL_OP::SQL_ASK, record);

  is_db_op_succeed = true;
  do {
    if (db.executeDBOperation() < 0) {
      std::string db_err_msg;
      db.getDBErrMsg(db_err_msg);
      std::cerr << "db_module: error: OP = ASK, " << db_err_msg << std::endl;
      is_db_op_succeed = false;
      break;
    }
    std::string tmpbuf;
    int ret = db.getFieldValue(mhwimmc_db_ns::db_tr_idx::IDX_MOD_PATH, tmpbuf);
    if (ret < 0)
      break;
    pDBCommit_ASK_RetBuf->insert(tmpbuf);
  } while (1);
}

// caller must ensures @db been opened
// we do not check it at there
/**
 * mhwimmc_db_thread_worker - DB module control thread
 * @db:                       db handler passed by caller
 */
void mhwimmc_db_thread_worker(mhwimmc_db_ns::mhwimmc_db &db)
{
  for (; ;) {
    if (program_exit)
      break;
    std::unique_lock::unique_lock cdb_sync(&cdbmutex);

    // we reset DB status,that means we discard the DB operation last registered
    // since DB operations might failed,if the SQL statement is correct,the error
    // must be caused by parameters get from CMD module
    db.resetDBStatus();

    if (ASK_req) {
      do_db_ask_request(db);
    } else if (ADD_req) {
      ins_date = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      do_db_add_request(db);
    } else if (DEL_req) {
      do_db_del_request(db);
    }

    // no database request has been committed in the case that
    // @ASK_req == @ADD_req == 0
  }
}
