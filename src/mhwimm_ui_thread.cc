/**
 * UI Thread Worker
 */
#include "mhwimm_ui_thread.h"
#include "mhwimm_sync_mechanism.h"

#include <assert.h>

using namespace mhwimm_sync_mechanism_ns;

/**
 * mhwimm_ui_thread_worker - UI thread worker
 * @mmui:                    UI object handler
 * @ctrlmsg:                 message exchange structure between UI and
 *                           Executor
 */
void mhwimm_ui_thread_worker(mhwimm_ui_ns::mhwimm_ui &mmui, uiexemsgexchg &ctrlmsg)
{
  mmui.printStartupMsg();
  for (; ;) {

    /* command event cycle */
    mmui.newLine();

    /**
     * we do not need concurrent reading protection for this
     * indicator.
     * only several cases the state of indicator will change :
     *   exit command
     *     when Command module is working,UI module would be blocked
     *     until the mutex released
     *   SIGINT
     *   SIGTERM
     *     when signal action hander is working,the program is prepare
     *     switch back to User Mode,but before signal action handler
     *     finished,no thread can resume executing
     */
    if (program_exit)
      break;

    mmui.printPrompt();

    ssize_t ret(mmui.readFromUser());
    if (ret < 0) {
      mmui.printMessage(std::string{"ui thread error: Failed to read user input!"});
      continue;
    }

    ctrlmsg.condv_sync.wait_cond_even();

    ctrlmsg.status = UIEXE_STATUS::UI_CMD;
    mmui.sendCMDTo(ctrlmsg.io_buf);

    ctrlmsg.condv_sync.update_and_notify();

    /**
     * Executor handles user command
     * UI module have to wait for it accomplished and get output
     */
    for (bool is_break(false); is_break; is_break = !is_break) {
      is_break = false;
      ctrlmsg.condv_sync.wait_cond_even();
      assert(ctrlmsg.status != UIEXE_STATUS::UI_CMD);
      
      if (ctrlmsg.status == UIEXE_STATUS::EXE_NOMSG) {
        is_break = true;
        goto cond_notify;
      }

      mmui.newLine();
      mmui.printIndentSpaces();
      mmui.printMessage(ctrlmsg.io_buf);

      if (ctrlmsg.status == UIEXE_STATUS::EXE_ONEMSG)
        is_break = true;

    cond_notify:
      ctrlmsg.condv_sync.update_and_notify();
    }

  }

  mmui.printMessage(std::string{"Program exiting."});
  mmui.newLine();
}
