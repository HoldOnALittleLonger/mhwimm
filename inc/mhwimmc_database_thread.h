#ifndef _MHWIMMC_DATABASE_THREAD_H_
#define _MHWIMMC_DATABASE_THREAD_H_

#include "mhwimmc_sync_types.h"

class mhwimmc_db_ns::mhwimmc_db;

// these two routines are used to handle CMD module requests to
// DB module.
// when CMD module wants to export data to DB,commitADDRequest() should be called
// when CMD module wants to import data from DB,commitASKRequest() should be called
void commitASKRequest(const std::string &mod_name, std::list<std::string> &path_buf);
void commitADDRequest(const std::string &mod_name, const std::list<std::string> &path_list);

void mhwimmc_db_thread_worker(mhwimmc_db_ns::mhwimmc_db &db);

#endif
