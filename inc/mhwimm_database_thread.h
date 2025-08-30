#ifndef _MHWIMM_DATABASE_THREAD_H_
#define _MHWIMM_DATABASE_THREAD_H_

class mhwimm_db_ns::mhwimm_db;
struct mhwimm_sync_mechanism_ns::mod_files_list;

void mhwimm_db_thread_worker(mhwimm_db_ns::mhwimm_db &db,
                             mhwimm_sync_mechanism_ns::mode_files_list &flist);

#endif
