#ifndef _MHWIMMC_CMD_THREAD_H_
#define _MHWIMMC_CMD_THREAD_H_

#include "mhwimmc_sync_types.h"

class mhwimmc_cmd_ns::mhwimmc_cmd;

void mhwimmc_cmd_thread_worker(mhwimmc_cmd_ns::mhwimmc_cmd &cmd_handler,
                               exportToDBCallbackFunc_t exportFunc,
                               importFromDBCallbackFunc_t importFunc,
                               ucmsgexchg *ucme);

#endif
