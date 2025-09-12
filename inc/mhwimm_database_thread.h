/**
 * Header of Database Thread Worker
 */
#ifndef _MHWIMM_DATABASE_THREAD_H_
#define _MHWIMM_DATABASE_THREAD_H_

#include "mhwimm_database.h"

/**
 * mhwimm_db_thread_worker - C++ multithread worker for Database
 * @db:    lvalue reference to database handler
 */
void mhwimm_db_thread_worker(mhwimm_db_ns::mhwimm_db &db);

#endif
