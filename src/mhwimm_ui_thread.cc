/**
 * UI Thread Worker
 */
#include "mhwimm_ui_thread.h"
#include "mhwimm_sync_mechanism.h"

#include <cassert>

using namespace mhwimm_sync_mechanism_ns;

/**
 * mhwimm_ui_thread_worker - UI thread worker
 * @mmui:                    UI object handler
 * @ctrlmsg:                 message exchange structure between UI and
 *                           Executor
 */
void mhwimm_ui_thread_worker(mhwimm_ui_ns::mhwimm_ui &mmui, uiexemsgexchg &ctrlmsg)
{
  makeup_uniquelock_and_associate_condv(uiexe_lock, ctrlmsg.condv_sync);
  uiexe_lock.unlock();

  mmui.printStartupMsg();
  for (; !program_exit;) {
    // at the beginning,the condv must be even.
    // because we will unlock it but without
    // increase after we finished retrieve output
    /// message.

    /* command event cycle */
    mmui.newLine();
    mmui.printPrompt();

    ssize_t ret(mmui.readFromUser());
    if (ret < 0) {
      mmui.printMessage(std::string{"ui thread error: Failed to read user input!"});
      continue;
    }

    // It is my round now!
    ctrlmsg.condv_sync.wait_cond_even(uiexe_lock);

    ctrlmsg.status = UIEXE_STATUS::UI_CMD;
    mmui.sendCMDTo(ctrlmsg.io_buf);

    ctrlmsg.condv_sync.update_and_notify(uiexe_lock);
    // Round finished.

    do {
      ctrlmsg.condv_sync.wait_cond_even(uiexe_lock);
      assert(ctrlmsg.status != UIEXE_STATUS::UI_CMD);
      
      if (ctrlmsg.status == UIEXE_STATUS::EXE_NOMSG) 
        break;

      mmui.newLine();
      mmui.printIndentSpaces();
      mmui.printMessage(ctrlmsg.io_buf);

      if (ctrlmsg.status == UIEXE_STATUS::EXE_ONEMSG)
        break;
      
      ctrlmsg.condv_sync.update_and_notify(uiexe_lock);
    } while (1);
    // do not update sequence counter,because it is not
    // the time to Executor start a new trasaction.
    ctrlmsg.condv_sync.unlock(uiexe_lock);
  }

  // we exited the for-cycle,but the condv is still
  // even now,we must release it to let Executor
  // parse a new transaction with an invalid command,
  // then it will execute NOP.
  ctrlmsg.condv_sync.wait_cond_even(uiexe_lock);
  ctrlmsg.status = UIEXE_STATUS::UI_CMD;
  ctrlmsg.io_buf = "nop";
  ctrlmsg.condv_sync.update_and_notify(uiexe_lock);

  mmui.printMessage(std::string{"Program exiting."});
  mmui.newLine();
}
