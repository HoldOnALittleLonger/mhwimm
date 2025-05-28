#include "mhwimmc_cmd_thread.h"
#include "mhwimmc_sync_types.h"

#include <cstddef>


void mhwimmc_cmd_thread_worker(mhwimmc_cmd_ns::mhwimmc_cmd &cmd_handler,
                               exportToDBCallbackFunc_t exportFunc,
                               importFromCallbackFunc_t importFunc,
                               mhwimmc_sync_types_ns::ucmsgexchg *ucme)
{
  std::unique_lock::unique_lock cdb_locker(&cdbmutex);

  // because these two wrapper functions will be called by INSTALL/UNINSTALL/INSTALLED only,
  // and the commands will be executed with database mutex acquired,thus
  // we can simply unlock the mutex to tell DB module deal with request,and lock
  // the mutex later for cmd thread.

  auto ExportSyncToDB = [&, exportFunc, cdb_locker]
    (const std::string &name, const std::list<std::string> &info_list) -> void {
    exportFunc(name, info_list);
    cdb_locker.unlock();
    cdb_locker.lock();
  };

  auto ImportSyncToDB = [&, importFunc, cdb_locker]
    (const std::string &name, std::list<std::string> &info_list) -> void {
    importFunc(name, info_list);
    cdb_locker.unlock();
    cdb_locker.lock();
  };

  cmd_handler.registerExportCallback(ExportSyncToDB);
  cmd_handler.registerImportCallback(ImportSyncToDB);

  for (; ;) {
    if (program_exit)
      return 0;

    std::unique_lock::unique_lock uc_locker(&ucmutex);

    // new command event cycle
    // reset status
    // clear output infos
    cmd_handler.resetStatus();
    cmd_handler.clearGetOutputHistory();

    int ret = cmd_handler.parseCMD(ucme->io_buf);
    if (ret < 0) {
      ucme->status = 1;
      cmd_handler.getCMDOutput(ucme->io_buf);
      continue;
    }

    ret = cmd_handler.executeCurrentCMD();
    if (ret < 0) {
      ucme->status = 1;
      cmd_handler.getCMDOutput(ucme->io_buf);
      continue;
    }

    if (cmd_handler.currentCMD() == mhwimmc_cmd_ns::INSTALLED) {
      // output more than once
      do {
        ret = cmd_handler.getCMDOutput(ucme->io_buf);
        ucme->stats = ret < 0 ? 0 : 2;
        if (ret < 0)
          break;
        uc_locker.unlock();
      } while(uc_locker.lock(), 1);
    } else {
      ret = cmd_handler.getCMDOutput(ucme->io_buf);
      if (ret < 0) {
        // CMD no output
        ucme->status = 0;
      } else {
        ucme->satus = 1;
      }
    }
        
  }
}
