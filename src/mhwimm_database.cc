#include "mhwimm_database.h"

#include <cstdint>
#include <sqlite3/sqlite3.h>

#ifdef DEBUG
#include <iostream>
#endif

#define DB_ERROR_OPEN "db: error: failed to open database."
#define DB_ERROR_CLOSE "db: error: failed to close database."
#define DB_ERROR_BADHANDLER "db: error: attempt to operate a invalid database."
#define DB_ERROR_PRETABLE "db: error: failed to prepare SQL statment for create table."
#define DB_ERROR_CRTABLE "db: error: failed to create table."
#define DB_ERROR_LACKVS "db: error: lack values to be inserted."
#define DB_ERROR_BADOP "db: error: un-supported database operation."
#define DB_ERROR_PRESQL "db: error: failed to make preparing for db operation."
#define DB_ERROR_BINDV "db: error: failed to setup field values."
#define DB_ERROR_FAILEDAD "db: error: failed to process insert/delete."
#define DB_ERROR_FAILEDASK "db: error: failed to retrieve data records."

namespace mhwimm_db_ns {


  int mhwimm_db::openDB(void)
  {
    current_status_ = DB_STATUS::DB_WORKING;
    std::string dbpath(db_path_ + "/" + db_name_);
#ifdef DEBUG
    std::cerr << "Open DB - " << dbpath << std::endl;
#endif
    int ret = sqlite3_open_v2(dbpath.c_str(), &db_handler_,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    current_status_ = DB_STATUS::DB_IDLE;
    ret = ret != SQLITE_OK ? -1 : 0;
    if (ret < 0) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = DB_ERROR_OPEN;
    }
    return ret;
  }

  int mhwimm_db::closeDB(void)
  {
    current_status_ = DB_STATUS::DB_WORKING;
    int ret = sqlite3_close_v2(db_handler_);
    current_status_ = DB_STATUS::DB_IDLE;
    ret = ret != SQLITE_OK ? -1 : 0;
    if (ret < 0) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = DB_ERROR_CLOSE;
    }
    return ret;
  }

  int mhwimm_db::tryCreateTable(void)
  {
    current_status_ = DB_STATUS::DB_WORKING;
    if (!db_handler_) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = DB_ERROR_BADHANDLER;
      return -1;
    }

    int ret = sqlite3_prepare_v2(db_handler_, sqlCreateTable_, -1,
                                 &sql_stmt_, NULL);
    if (ret != SQLITE_OK) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = DB_ERROR_PRETABLE;
      return -1;
    }

    ret = sqlite3_step(sql_stmt_);
    current_status_ = DB_STATUS::DB_IDLE;

    ret = ret != SQLITE_DONE ? -1 : 0;
    if (ret < 0) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = DB_ERROR_CRTABLE;
#ifdef DEBUG
      std::cerr << "SQL statement - Create Table \n"
                << sqlCreateTable_ << std::endl;
#endif
    }

    sqlite3_finalize(sql_stmt_);
    sql_stmt_ = nullptr;
    return ret;
  }

  int mhwimm_db::executeDBOperation(void)
  {
    current_status_ = DB_STATUS::DB_WORKING;

    // no SQL operation specified.
    if (current_op_ == SQL_OP::SQL_NOP) {
      current_status_ = DB_STATUS::DB_IDLE;
      return 0;
    }

    int ret = 0;
    // SQL statements
    std::string SELECT(std::string{"SELECT * FROM "} + table_name_);
    std::string INSERT(std::string{"INSERT INTO "} + table_name_);
    std::string DELETE(std::string{"DELETE FROM "} + table_name_);

#ifdef DEBUG
    std::cerr << "Execute DB - SQL Statements \n"
              << SELECT << "\n"
              << INSERT << "\n"
              << DELETE << std::endl;
#endif

    /**
     * construct_stmt_with_where_cond - construct SQL statement with additional logical LINK symbol
     * @sqlstmt:                        SQL statement in C++-style string
     * @LINK:                           SQL logical LINK symbol,it could be
     *                                    AND, OR
     */
    auto construct_stmt_with_where_cond = [&](std::string &sqlstmt, const char *LINKER)
      ->void {

      if (!record_buf_.is_mod_name_set &&
          !record_buf_.is_file_path_set &&
          !record_buf_.is_install_date_set)
        return;

      sqlstmt += " WHERE ";

      bool need_linker(false);

      if (record_buf_.is_mod_name_set) {
        sqlstmt += " mod_name = $key1 ";
        need_linker = true;
      }

      if (record_buf_.is_file_path_set) {
        if (need_linker)
          sqlstmt += LINKER;
        sqlstmt += " file_path = $key2 ";
        need_linker = true;
      }

      if (record_buf_.is_install_date_set) {
        if (need_linker)
          sqlstmt += LINKER;
        sqlstmt += " install_date = $key3 ";
      }

      sqlstmt += ";";
    };

    auto construct_select = [&](const char *LINKER) -> void {
      construct_stmt_with_where_cond(SELECT, LINKER);
    };
    auto construct_delete = [&](const char *LINKER) -> void {
      construct_stmt_with_where_cond(DELETE, LINKER);
    };

    /* INSERT command no WHERE substatement */
    auto construct_insert = [&](void) -> void {
      INSERT += " (mod_name, file_path, install_date) VALUES ($key1, $key2, $key3);";
    };

    if (more_row_indicator_)
      goto more_row;

    switch (current_op_) {
    case SQL_OP::SQL_ADD:
      if (!record_buf_.is_mod_name_set || !record_buf_.is_file_path_set ||
          !record_buf_.is_install_date_set) {
        current_status_ = DB_STATUS::DB_ERROR;
        local_err_msg_ = DB_ERROR_LACKVS;
        return -1;
      }
      construct_insert();
#ifdef DEBUG
      std::cerr << "INSERT after construct - \n"
                << INSERT
                << std::endl;
#endif
      ret = sqlite3_prepare_v2(db_handler_, INSERT.c_str(), -1, &sql_stmt_,
                               NULL);
      break;
    case SQL_OP::SQL_DEL:
      construct_delete("AND");
#ifdef DEBUG
      std::cerr << "DELETE after construct - \n"
                << DELETE
                << std::endl;
#endif
      ret = sqlite3_prepare_v2(db_handler_, DELETE.c_str(), -1, &sql_stmt_,
                               NULL);
      break;
    case SQL_OP::SQL_ASK:
      construct_select("AND");
#ifdef DEBUG
      std::cerr << "SELECT after construct - \n"
                << SELECT
                << std::endl;
#endif
      ret = sqlite3_prepare_v2(db_handler_, SELECT.c_str(), -1, &sql_stmt_,
                               NULL);
      break;
    default:
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = DB_ERROR_BADOP;
      return -1;
    }

    if (ret != SQLITE_OK) {
#ifdef DEBUG
      std::cerr << sqlite3_errmsg(db_handler_) << std::endl;
#endif
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = DB_ERROR_PRESQL;
      return -1;
    }

    ret = SQLITE_OK;
    // the sqlite_stmt object have been prepared.
    // now we can bind parameters.
    if (current_op_ == SQL_OP::SQL_ADD) {
#ifdef DEBUG
      std::cerr << "INSERT binds - \n"
                << record_buf_.mod_name
                << "\n"
                << record_buf_.file_path
                << "\n"
                << record_buf_.install_date
                << std::endl;
#endif
      ret |= sqlite3_bind_text(sql_stmt_, 1, record_buf_.mod_name.c_str(), -1,
                               SQLITE_STATIC);
      ret |= sqlite3_bind_text(sql_stmt_, 2, record_buf_.file_path.c_str(), -1,
                               SQLITE_STATIC);
      ret |= sqlite3_bind_text(sql_stmt_, 3, record_buf_.install_date.c_str(), -1,
                               SQLITE_STATIC);
    } else if (record_buf_.is_mod_name_set) {
      const char *key_mod_name = record_buf_.mod_name.c_str();
      const char *key_file_path = record_buf_.file_path.c_str();
      const char *key_ins_date = record_buf_.install_date.c_str();
#ifdef DEBUG
      std::cerr << "SELECT/DELETE binds - \n"
                << key_mod_name
                << "\n"
                << key_file_path
                << "\n"
                << key_ins_date
                << std::endl;
#endif
      uint8_t key_idx(0);

      key_idx = sqlite3_bind_parameter_index(sql_stmt_, "$key1");
      if (key_idx)
        ret |= sqlite3_bind_text(sql_stmt_, key_idx, key_mod_name, -1, SQLITE_STATIC);

      key_idx = sqlite3_bind_parameter_index(sql_stmt_, "$key2");
      if (key_idx)
        ret |= sqlite3_bind_text(sql_stmt_, key_idx, key_file_path, -1, SQLITE_STATIC);

      key_idx = sqlite3_bind_parameter_index(sql_stmt_, "$key3");
      if (key_idx)
        ret |= sqlite3_bind_text(sql_stmt_, key_idx, key_ins_date, -1, SQLITE_STATIC);
    }

    if (ret != SQLITE_OK) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = DB_ERROR_BINDV;
      ret = -1;
#ifdef DEBUG
        std::cerr << sqlite3_errmsg(db_handler_) << std::endl;
#endif
      goto finalize_out;
    }

    // now we can evaluate sql statement.
    switch (current_op_) {
    case SQL_OP::SQL_ADD:
    case SQL_OP::SQL_DEL:
      ret = sqlite3_step(sql_stmt_);
      if (ret != SQLITE_DONE) {
        current_status_ = DB_STATUS::DB_ERROR;
        local_err_msg_ = DB_ERROR_FAILEDAD;
        ret = -1;
#ifdef DEBUG
        std::cerr << sqlite3_errmsg(db_handler_) << std::endl;
#endif
        goto finalize_out;
      }
      break;
    case SQL_OP::SQL_ASK:
      goto more_row;
    }

  complete_executing:
    current_status_ = DB_STATUS::DB_IDLE;
    ret = 0;

  finalize_out:
    sqlite3_finalize(sql_stmt_);
    sql_stmt_ = nullptr;
    return ret;

  more_row:
    more_row_indicator_ = false;
    ret = sqlite3_step(sql_stmt_);
    if (ret == SQLITE_ROW) {
      more_row_indicator_ = true;
      record_buf_.mod_name = reinterpret_cast<const char *>(sqlite3_column_text(sql_stmt_, 0));
      record_buf_.file_path = reinterpret_cast<const char *>(sqlite3_column_text(sql_stmt_, 1));
      record_buf_.install_date = reinterpret_cast<const char *>(sqlite3_column_text(sql_stmt_, 2));
#ifdef DEBUG
      std::cerr << "SELECT returned - "
                << record_buf_.mod_name
                << "\n"
                << record_buf_.file_path
                << "\n"
                << record_buf_.install_date
                << std::endl;
#endif
      return 0;
    } else if (ret == SQLITE_DONE) {
      goto complete_executing;
    } else {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = DB_ERROR_FAILEDASK;
      ret = -1;
#ifdef DEBUG
        std::cerr << sqlite3_errmsg(db_handler_) << std::endl;
#endif
      goto finalize_out;
    }
  }

}
