#include "mhwimmc_database.h"

namespace mhwimmc_db_ns {


  int mhwimmc_db::openDB(void)
  {
    current_status_ = DB_STATUS::DB_WORKING;
    int ret = sqlite3_open_v2((db_path_ + "/" + db_name_).c_str(), &db_handler_,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    current_status_ = DB_STATUS::DB_IDLE;
    ret = ret != SQLITE_OK ? -1 : 0;
    if (ret < 0) {
      current_status_ = DB_STATUS::DB_IDLE;
      local_err_msg_ = "db: error: failed to open database.";
    }
    return ret;
  }

  int mhwimmc_db::closeDB(void)
  {
    current_status_ = DB_STATUS::DB_WORKING;
    int ret = sqlite3_close_v2(db_handler_);
    current_status_ = DB_STATUS::DB_IDLE;
    ret = ret != SQLITE_OK ? -1 : 0;
    if (ret < 0) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = "db: error: failed to close database.";
    }
    return ret;
  }

  int mhwimmc_db::tryCreateTable(void)
  {
    current_status_ = DB_STATUS::DB_WORKING;
    if (!db_handler_) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = "db: error: attempt to operate a invalid database.";
      return -1;
    }

    int ret = sqlite3_prepare_v2(db_handler_, sqlCreateTable_, -1,
                                 &sql_stmt_, NULL);
    if (ret != SQLITE_OK) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = "db: error: failed to prepare SQL statment for create table.";
      return -1;
    }

    ret = sqlite3_step(sql_stmt_);
    current_status_ = DB_STATUS::DB_IDLE;

    ret = ret != SQLITE_DONE ? -1 : 0;
    if (ret < 0) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = "db: error: failed to create table.";
    }

    sqlite3_finalize(sql_stmt_);
    sql_stmt_ = nullptr;
    return ret;
  }

  int mhwimmc_db::executeDBOperation(void)
  {
    current_status_ = DB_STATUS::DB_WORKING;

    if (current_op_ == SQL_OP::SQL_NOP) {
      current_status_ = DB_STATUS::DB_IDLE;
      return 0;
    }
    if (more_row_indicator_)
      goto more_row;

    // SQL statements
    std::string SELECT(std::string{"SELECT * FROM "} + table_name_);
    std::string INSERT(std::string{"INSERT INTO "} + table_name_);
    std::string DELETE(std::string{"DELETE FROM "} + table_name_);

    auto construct_stmt_with_where_cond = [&, record_buf_](std::string &sqlstmt, const char *LINK)
      ->void {
      if (record_buf_.mod_name_is_set || record_buf_.mod_path_is_set ||
          record_buf_.install_date_is_set)
        sqlstmt += " WHERE mod_name = $key1 " + LINK + " mod_path = $key2 " +
          LINK + " install_date = $key3;";
      else
        sqlstmt += ";";
    };
    auto construct_select = [&](const char *LINK) -> void {
      construct_stmt_with_where_cond(SELECT, LINK);
    };
    auto construct_delete = [&](const char *LINK) -> void {
      construct_stmt_with_where_cond(DELETE, LINK);
    };

    auto construct_insert = [&](void) -> void {
      INSERT += " (mod_name, mod_path, install_date) VALUES ($key1, $key2, $key3);";
    };

    int ret = 0;
    switch (current_op_) {
    case SQL_OP::SQL_ADD:
      if (!record_buf_.mod_name_is_set || !record_buf_.mod_path_is_set ||
          !record_buf_.install_date_is_set) {
        current_status_ = DB_STATUS::DB_ERROR;
        local_err_msg_ = "db: error: lack values to be inserted.";
        return -1;
      }
      construct_insert();
      ret = sqlite3_prepare_v2(db_handler_, INSERT.c_str(), -1, &sql_stmt_,
                               NULL);
      break;
    case SQL_OP::SQL_DEL:
      construct_delete("AND");
      ret = sqlite3_prepare_v2(db_handler_, DELETE.c_str(), -1, &sql_stmt_,
                               NULL);
      break;
    case SQL_OP::SQL_ASK:
      construct_select("AND");
      ret = sqlite3_prepare_v2(db_handler_, SELECT.c_str(), -1, &sql_stmt_,
                               NULL);
      break;
    default:
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = "db: error: un-supported database operation.";
      return -1;
    }

    if (ret != SQLITE_OK) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = "db: error: failed to make preparing for db operation.";
      return -1;
    }

    // from there the sqlite_stmt object have been prepared.
    // now we can bind parameters.
    if (current_op_ == SQL_OP::SQL_ADD) {
      ret = 0;

      ret |= sqlite3_bind_text(sql_stmt_, 1, record_buf_.mod_name.c_str(), -1,
                              SQLITE_STATIC);
      ret |= sqlite3_bind_text(sql_stmt_, 2, record_buf_.mod_path.c_str(), -1,
                              SQLITE_STATIC);
      ret |= sqlite3_bind_text(sql_stmt_, 3, record_buf_.install_date.c_str(), -1,
                              SQLITE_STATIC);
    } else {
      ret = 0;
      // unspecified use '*' to instead
      const char *key_1 = record_buf_.mod_name_is_set ? record_buf_.mod_name.c_str() : "*";
      const char *key_2 = record_buf_.mod_path_is_set ? record_buf_.mod_path.c_str() : "*";
      const char *key_3 = record_buf_.install_date_is_set ? record_buf_.install_date.c_str() :
        "*";

      ret |= sqlite3_bind_text(sql_stmt_, 1, key_1, -1, SQLITE_STATIC);
      ret |= sqlite3_bind_text(sql_stmt_, 2, key_2, -1, SQLITE_STATIC);
      ret |= sqlite3_bind_text(sql_stmt_, 3, key_3, -1, SQLITE_STATIC);
    }

    if (ret != SQLITE_OK) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = "db: error: failed to setup field values.";
      ret = -1;
      goto finalize_out;
    }

    // now we can evaluate sql statement.
    switch (current_op_) {
    case SQL_OP::SQL_ADD:
    case SQL_OP::SQL_DEL:
      ret = sqlite3_step(sql_stmt_);
      if (ret != SQLITE_DONE) {
        current_status_ = DB_STATUS::DB_ERROR;
        local_err_msg_ = "db: error: failed to process insert/delete.";
        ret = -1;
        goto finalize_out;
      }
      break;
    case SQL_OP::SQL_ASK:
      goto more_row;
    default:;
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
      record_buf_.mod_name = sqlite3_column_text(sql_stmt_, 0);
      record_buf_.mod_path = sqlite3_column_text(sql_stmt_, 1);
      record_buf_.install_date = sqlite3_column_text(sql_stmt_, 2);
      return 0;
    }
    else if (ret == SQLITE_DOEN) {
      goto complete_executing;
    } else {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = "db: error: failed to retrieve data records.";
      ret = -1;
      goto finalize_out;
    }
  }








}
