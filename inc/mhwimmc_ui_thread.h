#ifndef _MHWIMMC_UI_THREAD_H_
#define _MHWIMMC_UI_THREAD_H_

#include <string>

class UI::mhwimmc_ui;
struct ucmsgexchg;

/**
 * mhwimmc_ui_thread_worker - C++ multithread worker for take charge of 
 *                            UI
 * @mmcui:                    an object is type of mhwimmc_ui
 */
void mhwimmc_ui_thread_worker(mhwimmc_ui &mmcui, struct ucmsgexchg *ucme);

#endif
