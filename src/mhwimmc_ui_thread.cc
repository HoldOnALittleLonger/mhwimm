#include "mhwimmc_ui_thread.h"
#include "mhwimmc_sync_types.h"

void mhwimmc_ui_thread_worker(mhwimmc_ui &mmcui, struct ucmsgexchg *ucme)
{
  std::unique_lock::unique_lock uitw_unique_lock(&ucmutex);

  mmcui.printStartupMsg();

  for (; ;) {
    /* command event cycle */
    mmcui.printMessage(std::string{"\n"});

    progexit_spinlock.lock();
    if (program_exit) {
      progexit_spinlock.unlock();
      break;
    }
    progexit_spinlock.unlock();

    mmcui.printPrompt();

    ssize_t ret(mmcui.readFromUser());
    if (ret < 0) {
      mmcui.printMessage(std::string{"Failed to read cmd input!\n"});
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

      if (ucme->status) {
        mmcui.printMessage(ucme->io_buf);
        break;
      }
      else
        mmcui.printMesage(ucme->io_buf);
        
      uitw_unique_lock.unlock();
    }
  }

  mmcui.printMessage(std::string{"Program exiting."});
}
