#ifndef _MHWIMM_DATABASE_H_
#define _MHWIMM_DATABASE_H_

#include <cstddef>
#include <cstdint>

#include <string>

struct sqlite3;

namespace mhwimm_db_ns {

  /**
   * SQL_OP - allowed database operations
   * ADD:     INSERT
   * DEL:     DELETE
   * ASK:     SELECT
   */
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
    IDX_FILE_PATH,
    IDX_INSTALL_DATE
  };

  /**
   * db_table_record - [mod name] [file path] [install date]
   * # when processing SQL operations,these three fields are used
   *   as filters.
   * # when processing SQL_ADD,three fields can not be NULL.
   */
  struct db_table_record {
    uint8_t is_mod_name_set:1;
    std::string mod_name;
    uint8_t is_file_path_set:1;
    std::string file_path;
    uint8_t is_install_date_set:1;
    std::string install_date;
  };

  enum INTEREST_FIELD : uint8_t {
    NO_INTEREST = 0,
    INTEREST_NAME = 1,
    INTEREST_PATH,
    INTEREST_DATE
  };

  using interest_db_field_t = uint8_t;

  class mhwimm_db finally {
  public:
    mhwimm_db(const char *db_name, const char *db_path) =default
      : db_name_(db_name), db_path_(db_path)
    {
      db_handler_ = nullptr;
      sql_stmt_ = nullptr;
      current_op_ = SQL_OP::SQL_NOP;
      current_status_ = DB_STATUS::DB_IDLE;
      record_buf_ = db_table_record{0};
      local_err_msg_ = "nil";
      more_row_indicator_ = flase;
    }

    mhwimm_db(const mhwimm_db &) =delete;
    mhwimm_db &operator=(const mhwimm_db &) =delete;
    mhwimm_db(mhwimm_db &&) =delete;
    mhwimm_db &operator=(mhwimm_db &&) =delete;

    int openDB(void);
    int closeDB(void);
    int tryCreateTable(void);

    bool is_db_opened(void) const noexcept { return db_handler_ != nullptr; }

    std::string returnDBpath(void) const noexcept
    {
      return db_path_ + "/" + db_name_;
    }

    auto currentSelectedModName(void) const noexcept
    {
      return record_buf_.mod_name;
    }

    int getFieldValue(db_tr_idx i, std::string &buf)
    {
      if (!more_row_indicator_) {
        current_status_ = DB_STATUS::DB_ERROR;
        local_err_msg_ = "db: error: no result returned.";
        return -1;
      }
      switch (i) {
      case db_tr_idx::IDX_MOD_NAME:
        buf = record_buf_.mod_name;
        break;
      case db_tr_idx::IDX_FILE_PATH:
        buf = record_buf_.file_path;
        break;
      case db_tr_idx::IDX_INSTALL_DATE:
        buf = record_buf_.install_date;
      default:
        current_status_ = DB_STATUS::DB_ERROR;
        local_err_msg_ = "db: error: undefined filed index."
        return -1;
      }
      return 0;
    }

    void registerDBOperation(SQL_OP op, const db_table_record &operand)
    {
      current_status_ = DB_STATUS::DB_WORKING;
      current_op_ = op;
      record_buf_ = operand;
      current_status_ = DB_STATUS::DB_IDLE;
    }

    void chgDBStatus(DB_STATUS s) noexcept
    {
      current_status_ = s;
    }
    int executeDBOperation(void);
    auto getCurrentOP(void) { return current_op_; }

    auto getDBStatus(void) { return current_status_; }
    void resetDB(void)
    {
      current_status_ = DB_STATUS::DB_IDLE;
      current_op_ = SQL_OP::NOP;
      record_buf_ = {0};
    }

    void getDBErrMsg(std::string &outside_buf) const noexcept
    {
      current_status_ = DB_STATUS::DB_WORKING;
      if (current_status_ == DB_STATUS::DB_ERROR)
        outside_buf = local_err_msg_;
      current_status_ = DB_STATUS::DB_IDEL;
    }

  private:
    std::string db_name_;
    std::string db_path_;
    sqlite3 *db_handler_;
    sqlite3_stmt *sql_stmt_;
    SQL_OP current_op_;
    db_table_record record_buf_;
    DB_STATUS current_status_;
    std::string local_err_msg_;
    bool more_row_indicator_;

    constexpr const char *table_name_ = "mhwimm_db_table";

    /* we must create table at the first time */
    constexpr const char *sqlCreateTable_ =
      "CREATE TABLE mhwimm_db_table ("
      "mod_name CHAR NOT NULL,"
      "mod_path CHAR NOT NULL PRIMARY KEY,"
      "install_date CHAR NOT NULL);";
  };

}



#endif
