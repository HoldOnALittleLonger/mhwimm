#ifndef _MHWIMM_EXECUTOR_THREAD_H_
#define _MHWIMM_EXECUTOR_THREAD_H_

#include "mhwimm_executor.h"
#include "mhwimm_sync_mechanism.h"

void mhwimm_executor_thread_worker(mhwimm_executor_ns::mhwimm_executor &exe,
                                   mhwimm_sync_mechanism_ns::uiexemsgexchg &ctrlmsg,
                                   mhwimm_sync_mechanism_ns::mod_files_list &mfiles_list);


#endif
