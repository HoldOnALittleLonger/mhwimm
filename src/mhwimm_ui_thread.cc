#include "mhwimm_ui_thread.h"
#include "mhwimm_sync_mechanism.h"

#include <assert.h>

using namespace mhwimm_sync_mechanism_ns;

/**
 * mhwimm_ui_thread_worker - UI module control thread
 * @mmui:                    UI module core object handler
 * @ctrlmsg:                 message exchange structure between UI module and
 *                           CMD module
 */
void mhwimm_ui_thread_worker(mhwimm_ui_ns::mhwimm_ui &mmui, uiexemsgexchg &ctrlmsg)
{
  std::unique_lock uiexe_mutex_lock(ctrlmsg.lock);

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
    if (ret <= 0) {
      /**
       * = 0 -> EOF or ENTER
       * = -1 -> error encountered,or interrupted by signal
       */
      if (ret < 0) // print msg when detected error
        mmui.printMessage(std::string{"Failed to read user input!"});
      continue;
    }

    ctrlmsg.status = UIEXE_STATUS::UI_CMD;
    mmui.sendCMDTo(ctrlmsg.io_buf);

    uiexe_mutex_lock.unlock();

    NOP_DELAY();


    /**
     * CMD module handling user command
     * UI module have to wait for it accomplished and get output
     */
    for (; ;) {
      uiexe_mutex_lock.lock();

      // we disallow current status of @ctrlmsg is UI_CMD,
      // if in the case,that means Executor encountered fatal error.
      assert(ctrlmsg.status != UIEXE_STATUS::UI_CMD);

      if (ctrlmsg.status == UIEXE_STATUS::EXE_NOMSG) // no output info
        break;

      mmui.newLine();
      mmui.printIndentSpaces();
      mmui.printMessage(ctrlmsg.io_buf);

      if (ctrlmsg.status == UIEXE_STATUS::EXE_ONEMSG) // one line output
        break;

      uiexe_mutex_lock.unlock(); // if more than one line info have to be printed,
                                 // then we can release the lock and let Executor
                                 // updates the msg buffer.
      NOP_DELAY();
    }

  }

  mmui.printMessage(std::string{"Program exiting."});
  mmui.newLine();
}
