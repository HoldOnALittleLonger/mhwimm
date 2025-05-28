#ifndef _MHWIMMC_UI_THREAD_H_
#define _MHWIMMC_UI_THREAD_H_

#include <cstddef>

class mhwimmc_ui_ns::mhwimmc_ui;
struct mhwimmc_sync_type_ns::ucmsgexchg;

/**
 * mhwimmc_ui_thread_worker - C++ multithread worker for take charge of 
 *                            UI
 * @mmcui:                    an object is type of mhwimmc_ui
 */
void mhwimmc_ui_thread_worker(mhwimmc_ui_ns::mhwimmc_ui &mmcui, mhwimmc_sync_type_ns::ucmsgexchg *ucme);

#endif
