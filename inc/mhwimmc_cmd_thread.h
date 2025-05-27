#ifndef _MHWIMMC_CMD_THREAD_H_
#define _MHWIMMC_CMD_THREAD_H_

#include "mhwimmc_cmd.h"

void mhwimmc_cmd_thread_worker(mhwimmc_cmd_ns::Mmc_cmd &mmccmd,
                               exportToDBCallbackFunc_t exportFunc,
                               importFromDBCallbackFunc_t importFunc);

#endif
