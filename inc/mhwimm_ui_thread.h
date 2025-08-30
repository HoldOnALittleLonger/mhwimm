#ifndef _MHWIMM_UI_THREAD_H_
#define _MHWIMM_UI_THREAD_H_

class mhwimm_ui_ns::mhwimm_ui;
struct mhwimm_sync_mechanism_ns::uiexemsgexchg;

/**
 * mhwimm_ui_thread_worker - C++ multithread worker for take charge of 
 *                           UI
 * @mmui:                    an object is type of mhwimm_ui
 * @ctrlmsg:                 ui executor msg exchange entity
 */
void mhwimm_ui_thread_worker(mhwimm_ui_ns::mhwimm_ui &mmui,
                             mhwimm_sync_mechanism_ns::uiexemsgexchg &ctrlmsg);

#endif
