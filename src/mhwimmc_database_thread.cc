#include "mhwimmc_database_thread.h"
#include "mhwimmc_database.h"

#include <chrono>

static const std::string *pDBCommit_modname(nullptr);
static const std::list<std::string> *conspDBCommit_ADD_records(nullptr);
static std::list<std::string> *pDBCommit_ASK_RetBuf(nullptr);

static uint8_t ASK_req(0);
static uint8_t ADD_req(0);

static std::time_t ins_date(0);

void commitASKRequest(const std::string &mod_name, std::list<std::string> &path_buf)
{
  pDBCommit_modname = &mod_name;
  pDBCommit_ASK_RetBuf = &path_buf;
  ASK_req = 1;
  ADD_req = 0;
}

void commitADDRequest(const std::string &mod_name, const std::list<std::string> &path_list)
{
  pDBCommit_modname = &mod_name;
  conspDBCommit_ADD_records = path_list;
  ADD_req = 1;
  ASK_req = 0;
}

static void do_db_add_request(mhwimmc_db_ns::mhwimmc_db &db)
{
  mhwimmc_db_ns::db_table_record record;

  record.mod_name = *pDBCommit_modname;
  record.mod_name_is_set = 1;
  record.install_date = std::ctime(ins_date);
  record.install_date_is_set = 1;

  for (auto r : *conspDBCommit_ADD_records) {
    record.mod_path = r;
    record.mod_path_is_set = 1;
    
    db.registerDBOperation(mhwimmc_db_ns::SQL_OP::SQL_ADD, record);
    db.executeDBOperation();
  }
}

static void do_db_ask_request(mhwimmc_db_ns::mhwimmc_db &db)
{
  mhwimmc_db_ns::db_table_record record;

  record.mod_name = *pDBCommit_modname;
  record.mod_name_is_set = 1;

  record.mod_path_is_set = 0;
  record.install_date_is_set = 0;

  db.registerDBOperation(mhwimmc_db_ns::SQL_OP::SQL_ASK, record);

  do {
    db.executeDBOperation();
    std::string tmpbuf;
    int ret = db.getFieldValue(mhwimmc_db_ns::db_tr_idx::IDX_MOD_PATH, tmpbuf);
    if (ret < 0)
      break;
    pDBCommit_ASK_RetBuf->insert(tmpbuf);
  } while (1);
}
