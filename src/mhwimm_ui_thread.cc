/**
 * UI Thread Worker
 */
#include "mhwimm_ui_thread.h"
#include "mhwimm_sync_mechanism.h"

using namespace mhwimm_sync_mechanism_ns;

/**
 * mhwimm_ui_thread_worker - UI thread worker
 * @mmui:                    UI object handler
 * @ctrlmsg:                 message exchange structure between UI and
 *                           Executor
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
    if (ret < 0) {
      mmui.printMessage(std::string{"ui thread error: Failed to read user input!"});
      continue;
    }

    ctrlmsg.status = UIEXE_STATUS::UI_CMD;
    ctrlmsg.new_msg = 0;
    mmui.sendCMDTo(ctrlmsg.io_buf);

    uiexe_mutex_lock.unlock();

    /**
     * Executor handles user command
     * UI module have to wait for it accomplished and get output
     */
    for (std::size_t exenores(0); ;) {
      NOP_DELAY();
      uiexe_mutex_lock.lock();

      // wait Executor response
      // sometimes,lock the mutex again too fast will cause Executor
      // stay blocked and the command will have not parsed.
      if (ctrlmsg.status == UIEXE_STATUS::UI_CMD) {
        ++exenores;
        if (exenores > 16) {
          mmui.newLine();
          mmui.printMessage("ui thread error: Executor no response.");
        }
        uiexe_mutex_lock.unlock();
        continue;
      }

      if (ctrlmsg.status == UIEXE_STATUS::EXE_NOMSG)
        break;

      if (ctrlmsg.new_msg) {
        mmui.newLine();
        mmui.printIndentSpaces();
        mmui.printMessage(ctrlmsg.io_buf);
        ctrlmsg.new_msg = 0;
      }

      if (ctrlmsg.status == UIEXE_STATUS::EXE_ONEMSG)
        break;

      uiexe_mutex_lock.unlock(); // if more than one line info have to be printed,
                                 // then we can release the lock and let Executor
                                 // updates the msg buffer.
    }

  }

  mmui.printMessage(std::string{"Program exiting."});
  mmui.newLine();
}
