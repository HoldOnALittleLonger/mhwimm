#ifndef _MHWIMMC_UI_THREAD_H_
#define _MHWIMMC_UI_THREAD_H_

#include <mutex>
#include <thread>
#include <string>


extern std::mutex ui_cmd_mutex;
extern std::string ui_cmd_msg_buffer;

class mhwimmc_ui;

/**
 * mhwimmc_ui_thread_worker - C++ multithread worker for take charge of 
 *                            UI
 * @mmcui:                    an object is type of mhwimmc_ui
 */
void mhwimmc_ui_thread_worker(mhwimmc_ui &mmcui);

#endif
