#ifndef _MHWIMM_EXECUTOR_THREAD_H_
#define _MHWIMM_EXECUTOR_THREAD_H_

class mhwimm_executor_ns::mhwimm_executor;
struct mhwimm_sync_mechanism_ns::uiexemsgexchg;
struct mhwimm_sync_mechanism_ns::mod_files_list;

void mhwimm_executor_thread_worker(mhwimm_executor_ns::mhwimm_executor &exe,
                                   mhwimm_sync_mechanism_ns::uiexemsgexchg &ctrlmsg);


#endif
