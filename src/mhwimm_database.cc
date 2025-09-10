#include "mhwimm_database.h"

#include <cstdint>
#include <sqlite3/sqlite3.h>

namespace mhwimm_db_ns {


  int mhwimm_db::openDB(void)
  {
    current_status_ = DB_STATUS::DB_WORKING;
    int ret = sqlite3_open_v2((db_path_ + "/" + db_name_).c_str(), &db_handler_,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    current_status_ = DB_STATUS::DB_IDLE;
    ret = ret != SQLITE_OK ? -1 : 0;
    if (ret < 0) {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = "db: error: failed to open database.";
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
      local_err_msg_ = "db: error: failed to close database.";
    }
    return ret;
  }

  int mhwimm_db::tryCreateTable(void)
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
    /**
     * construct_stmt_with_where_cond - construct SQL statement with additional logical LINK symbol
     * @sqlstmt:                        SQL statement in C++-style string
     * @LINK:                           SQL logical LINK symbol,it could be
     *                                    AND, OR
     */
    auto construct_stmt_with_where_cond = [&](std::string &sqlstmt, const char *LINK)
      ->void {
      sqlstmt += " WHERE ";

      if (record_buf_.is_mod_name_set)
        sqlstmt += " mod_name = $key1 ";

      if (record_buf_.is_mod_name_set && record_buf_.is_file_path_set)
        (sqlstmt += LINK) += " file_path = $key2 ";
      else
        sqlstmt += " file_path = $key2 ";

      if (record_buf_.is_file_path_set && record_buf_.is_install_date_set)
        (sqlstmt += LINK) += " install_date = $key3 ";
      else
        sqlstmt += " install_date = $key3 ";

      sqlstmt += ";";
    };

    auto construct_select = [&](const char *LINK) -> void {
      construct_stmt_with_where_cond(SELECT, LINK);
    };
    auto construct_delete = [&](const char *LINK) -> void {
      construct_stmt_with_where_cond(DELETE, LINK);
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

    // the sqlite_stmt object have been prepared.
    // now we can bind parameters.
    if (current_op_ == SQL_OP::SQL_ADD) {
      ret = 0;

      ret |= sqlite3_bind_text(sql_stmt_, 1, record_buf_.mod_name.c_str(), -1,
                               SQLITE_STATIC);
      ret |= sqlite3_bind_text(sql_stmt_, 2, record_buf_.file_path.c_str(), -1,
                               SQLITE_STATIC);
      ret |= sqlite3_bind_text(sql_stmt_, 3, record_buf_.install_date.c_str(), -1,
                               SQLITE_STATIC);
    } else {
      ret = 0;

      const char *key_mod_name = record_buf_.mod_name.c_str();
      const char *key_file_path = record_buf_.file_path.c_str();
      const char *key_ins_date = record_buf_.install_date.c_str();

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
      return 0;
    } else if (ret == SQLITE_DONE) {
      goto complete_executing;
    } else {
      current_status_ = DB_STATUS::DB_ERROR;
      local_err_msg_ = "db: error: failed to retrieve data records.";
      ret = -1;
      goto finalize_out;
    }
  }

}
