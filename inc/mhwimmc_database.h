#ifndef _MHWIMMC_DATABASE_H_
#define _MHWIMMC_DATABASE_H_

#include <cstddef>
#include <cstdint>

#include <string>

struct sqlite3;

namespace mhwimmc_db_ns {

  enum class SQL_OP : uint8_t {
    SQL_ADD,
    SQL_DEL,
    SQL_ASK,
    SQL_NOP
  };

  enum class DB_STATUS : uint8_t {
    DB_IDLE,
    DB_WORKING,
    DB_ERROR
  };

  enum class db_tr_idx : uint8_t {
    IDX_MOD_NAME = 1,
    IDX_MOD_PATH,
    IDX_INSTALL_DATE
  };

  struct db_table_record {
    uint8_t mod_name_is_set:1;
    std::string mod_name;
    uint8_t mod_path_is_set:1;
    std::string mod_path;
    uint8_t install_date_is_set:1;
    std::string install_date;
  };

  class mhwimmc_db finally {
  public:
    mhwimmc_db(const char *db_name, const char *db_path) =default
      : db_name_(db_name), db_path_(db_path)
    {
      db_handler_ = nullptr;
      current_op_ = SQL_OP::SQL_NOP;
      current_status_ = DB_STATUS::DB_IDLE;
      record_buf_ = db_table_record{0};
      local_err_msg_ = "nil";
    }

    int openDB(void);
    int closeDB(void);

    void getFieldValue(db_tr_idx i, std::string &buf)
    {
      current_status_ = DB_STATUS::DB_WORKING;
      switch (i) {
      case db_tr_idx::IDX_MOD_NAME:
        buf = record_buf_.mod_name;
        break;
      case db_tr_idx::IDX_MOD_PATH:
        buf = record_buf_.mod_path;
        break;
      case db_tr_idx::IDX_INSTALL_DATE:
        buf = record_buf_.install_date;
      default:;
      }
      current_status_ = DB_STATUS::DB_IDLE;
    }

    void registerDBOperation(SQL_OP op, const db_table_record &operand)
    {
      current_status_ = DB_STATUS::DB_WORKING;
      current_op_ = op;
      record_buf_ = operand;
      current_status_ = DB_STATUS::DB_IDLE;
    }

    int executeDBOperation(void);
    auto getCurrentOP(void) { return current_op_; }

    auto getDBStatus(void) { return current_status_; }
    void resetDBStatus(void)
    {
      current_status_ = DB_STATUS::DB_IDLE;
      current_op_ = SQL_OP::NOP;
    }

    void getDBErrMsg(std::string &outside_buf)
    {
      if (current_status_ == DB_STATUS::DB_ERROR)
        outside_buf = local_err_msg_;
    }

  private:
    std::string db_name_;
    std::string db_path_;
    sqlite3 *db_handler_;
    SQL_OP current_op_;
    db_table_record record_buf_;
    DB_STATUS current_status_;
    std::string local_err_msg_;
  };

}



#endif
