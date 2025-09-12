/**
 * Header of Executor Thread Worker
 */
#ifndef _MHWIMM_EXECUTOR_THREAD_H_
#define _MHWIMM_EXECUTOR_THREAD_H_

#include "mhwimm_executor.h"
#include "mhwimm_sync_mechanism.h"

/**
 * mhwimm_executor_thread_worker - C++ multithread worker for Executor
 * @exe:        lvalue reference to Executor handler
 * @ctrlmsg:    lvalue reference to msg exchg entity
 * @mfiles_list:    lvalue reference to a structure which contains the
 *                  necessary containers
 */
void mhwimm_executor_thread_worker(mhwimm_executor_ns::mhwimm_executor &exe,
                                   mhwimm_sync_mechanism_ns::uiexemsgexchg &ctrlmsg,
                                   mhwimm_sync_mechanism_ns::mod_files_list &mfiles_list);

#endif
