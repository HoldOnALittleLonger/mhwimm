#include "mhwimm_sync_mechanism.h"
#include "mhwimm_database.h"

#include <assert.h>

static mhwimm_db_ns::mhwimm_db *db_impl(nullptr);
extern mhwimm_sync_mechanism_ns::mod_files_list *mfl_for_db;
extern mhwimm_db_ns::interest_db_field_t interest_field;

void init_regDB_routines(mhwimm_db_ns::mhwimm_db *db)
{
  db_impl = db;
}

/* request DB returns all mods' names in @mfl */
void regDBop_getAllInstalled_Modsname(mhwimm_sync_mechanism_ns::mod_files_list *mfl)
{
  assert(db_impl != nullptr);
  mfl_for_db = mfl;
  interest_field = mhwimm_db_ns::INTEREST_FIELD::INTEREST_NAME;
  mhwimm_db_ns::db_table_record dtr = {0};
  db_impl->registerDBOperation(mhwimm_db_ns::SQL_OP::SQL_ASK, dtr);
}

/* request DB returns the detail info about a specified mod in @mfl */
void regDBop_getInstalled_Modinfo(const std::string &modname,
                                  mhwimm_sync_mechanism_ns::mod_files_list *mfl)
{
  assert(db_impl != nullptr);
  mfl_for_db = mfl;
  interest_field = mhwimm_db_ns::INTEREST_FIELD::INTEREST_PATH;
  mhwimm_db_ns::db_table_record dtr = {
    .is_mod_name_set = 1,
    .mod_name = modname, // as filter
  };
  db_impl->registerDBOperation(mhwimm_db_ns::SQL_OP::SQL_ASK, dtr);
}

/* request DB add new mod infos into database */
/* the @interest_field is set to 6,and DB op set to SQL_ADD */
/* db thread worker must checks these,and read infos from @mfl */
void regDBop_add_mod_info(const std::string &modname,
                          mhwimm_sync_mechanism_ns::mod_files_list *mfl)
{
  assert(db_impl != nullptr);
  mfl_for_db = mfl;
  mhwimm_db_ns::db_table_record dtr = {
    .is_mod_name_set = 1,
    .mod_name = modname,
  };
  db_impl->registerDBOperation(mhwimm_db_ns::SQL_OP::SQL_ADD, dtr);
}

/* request DB remove records about a specified mod */
/* @interest_field is set to _zero_,means we do not needs any */
/* result set */
void regDBop_remove_mod_info(const std::string &modname)
{
  mhwimm_db_ns::db_table_record dtr = {
    .is_mod_name_set = 1,
    .mod_name = modname, // as filter
  };
  db_impl->registerDBOperation(mhwimm_db_ns::SQL_OP::SQL_DEL, dtr);
}


