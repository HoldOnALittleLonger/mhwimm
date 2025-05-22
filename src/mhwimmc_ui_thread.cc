#include "mhwimmc_ui_thread.h"
#include "mhwimmc_sync_types.h"

void mhwimmc_ui_thread_worker(mhwimmc_ui &mmcui, struct ucmsgexchg *ucme)
{
  std::unique_lock::unique_lock uitw_unique_lock(&ucmutex);

  mmcui.printStartupMsg();

  for (; ;) {
    /* command event cycle */
    mmcui.newLine();

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

    mmcui.printPrompt();

    ssize_t ret(mmcui.readFromUser());
    if (ret < 0) {
      mmcui.printMessage(std::string{"Failed to read cmd input!"});
      continue;
    }

    mmcui.sendCMDTo(ucme->io_buf);
    uitw_unique_lock.unlock();

    /**
     * CMD module handling user command
     * UI module have to wait for it accomplished and get output
     */
    while (1) {
      uitw_unique_lock.lock();

      mmcui.newLine();
      mmcui.printIndentSpaces();
      mmcui.printMessage(ucme->io_buf);

      if (ucme->status)
        break;

      uitw_unique_lock.unlock();
    }
  }

  mmcui.printMessage(std::string{"Program exiting."});
  mmcui.newLine();
}
