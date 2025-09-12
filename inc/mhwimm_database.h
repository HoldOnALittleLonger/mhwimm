/**
 * Monster Hunter World Iceborne Mod Manager Database
 * This file contains the definition of database,the
 * database is used to records what mods been installed
 * by mhwimm.
 */
#ifndef _MHWIMM_DATABASE_H_
#define _MHWIMM_DATABASE_H_

#include <cstddef>
#include <cstdint>

#include <string>

/* sqlite3 structures */
struct sqlite3;
struct sqlite3_stmt;

namespace mhwimm_db_ns {

#define DB_ERROR_NORESULT "db: error: no result returned."
#define DB_ERROR_NOFIELD "db: error: undefined filed index."

  /**
   * SQL_OP - allowed database operations
   * ADD:     INSERT
   * DEL:     DELETE
   * ASK:     SELECT
   * NOP:     nothing to do
   */
  enum class SQL_OP : uint8_t {
    SQL_ADD,
    SQL_DEL,
    SQL_ASK,
    SQL_NOP
  };

  /**
   * DB_STATUS - enumerate database status
   * DB_IDLE:    database is idle now
   * DB_WORKING: working
   * DB_ERROR:   database detected an error
   */
  enum class DB_STATUS : uint8_t {
    DB_IDLE,
    DB_WORKING,
    DB_ERROR
  };

  /**
   * db_tr_idx - database table record index for fields
   * IDX_MOD_NAME:    index for field "mod_name"
   * IDX_FILE_PATH:   index for field "file_path"
   * IDX_INSTALL_DATE:    index for field "install_date"
   */
  enum class db_tr_idx : uint8_t {
    IDX_MOD_NAME = 1,
    IDX_FILE_PATH,
    IDX_INSTALL_DATE
  };

  /**
   * db_table_record - database table record for one line
   * @mod_name:        mod_name field
   * @is_mod_name_set: indicator for @mod_name to tell database
   *                   whether @mod_name has a valid value
   * @file_path:       file_path field
   * @is_file_path_set:    indicator for @file_path to tell database
   *                       whether @file_path has a valid value
   * @install_date:        install_date field
   * @is_install_date_set: indicator for @install_date to tell
   *                       database whether @install_date has a
   *                       valid value
   * # this structure can be used to stores the resul from database,
   *   or the informations is going to be inserted into the table.
   *   it can as a filter when process database operations SQL_ASK
   *   and SQL_DEL
   */
  struct db_table_record {
    std::string mod_name;
    uint8_t is_mod_name_set:1;
    std::string file_path;
    uint8_t is_file_path_set:1;
    std::string install_date;
    uint8_t is_install_date_set:1;
  };
#define NULL_dtr {          \
  .mod_name = "BUG",        \
  .is_mod_name_set = 0,     \
  .file_path = "BUG",       \
  .is_file_path_set = 0,    \
  .install_date = "BUG",    \
  .is_install_date_set = 0, \
}

  /**
   * mhwimm_db - mhwimm database class definition,it wrapped some sqlite3
   *             routines,and interactive with sqlite3 database
   */
  class mhwimm_db final {
  public:
    // explicit constructor
    explicit mhwimm_db(const char *db_name, const char *db_path)
      : db_name_(db_name), db_path_(db_path)
    {
      db_handler_ = nullptr;
      sql_stmt_ = nullptr;
      current_op_ = SQL_OP::SQL_NOP;
      current_status_ = DB_STATUS::DB_IDLE;
      record_buf_ = db_table_record NULL_dtr;
      local_err_msg_ = "nil";
      more_rows_indicator_ = false;
    }

    // disabled copying,moving
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

    /**
     * getFieldValue - get the field's value from result
     * @i:             field index
     * @buf:           where to store the value
     * return:         0 OR -1
     */
    int getFieldValue(db_tr_idx i, std::string &buf)
    {
      if (!more_rows_indicator_) {
        current_status_ = DB_STATUS::DB_ERROR;
        local_err_msg_ = DB_ERROR_NORESULT;
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
        local_err_msg_ = DB_ERROR_NOFIELD;
        return -1;
      }
      return 0;
    }

    /**
     * registerDBOperation - register a database operation but do not
     *                       process it
     * @op:                  database operation
     * @operand:             table record,in some case,can be used as
     *                       filter
     */
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
    auto getCurrentOP(void) const { return current_op_; }
    auto getCurrentStatus(void) const { return current_status_; }
    auto getDBStatus(void) const { return current_status_; }

    void resetDB(void)
    {
      current_status_ = DB_STATUS::DB_IDLE;
      current_op_ = SQL_OP::SQL_NOP;
      record_buf_ = db_table_record NULL_dtr;
    }

    void getDBErrMsg(std::string &outside_buf) const noexcept
    {
      outside_buf = local_err_msg_;
    }

  private:
    /* db_name_ - databae file name */
    std::string db_name_;
    /* db_path_ - database file path withou file name */
    std::string db_path_;
    /* db_handler_ - sqlite3 database handler pointer */
    sqlite3 *db_handler_;
    /* sql_stmt_ - sqlite3 compiled SQL statement pointer */
    sqlite3_stmt *sql_stmt_;
    /* current_op_ - current registered operation */
    SQL_OP current_op_;
    /* record_buf_ - table record for one line */
    db_table_record record_buf_;
    /* current_status_ - current database status */
    DB_STATUS current_status_;
    /* local_err_msg_ - stores the error message produced by database */
    std::string local_err_msg_;
    /**
     * more_row_indicator_ - indicator for SQL_ASK operation,if it is
     *                       false,that means current SQL_ASK completed
     */
    bool more_rows_indicator_;

    /* table_name_ - table name */
    const char *table_name_ = "mhwimm_db_table";

    /* sqlCreateTable_ - SQL statement used to create table at first time */
    const char *sqlCreateTable_ = 
      "CREATE TABLE mhwimm_db_table ("
      "mod_name CHAR NOT NULL,"
      "file_path CHAR NOT NULL PRIMARY KEY,"
      "install_date CHAR NOT NULL);";
  };

}

#endif
