#include "mhwimmc_cmd_thread.h"
#include "mhwimmc_sync_types.h"

#include <cstddef>

extern int commitDELRequest(const std::string &, const std::list<std::string> &);

using namespace mhwimmc_sync_type_ns;

/**
 * mhwimmc_cmd_thread_worker - CMD module control thread
 * @cmd_handler:               CMD module core object handler
 * @exportFunc:                callback function used to export data to DB
 * @importFunc:                callback function used to import data from DB
 * @ucme:                      messsage exchange structure between UI module
 *                             and CMD module
 */
void mhwimmc_cmd_thread_worker(mhwimmc_cmd_ns::mhwimmc_cmd &cmd_handler,
                               exportToDBCallbackFunc_t exportFunc,
                               importFromCallbackFunc_t importFunc,
                               ucmsgexchg *ucme)
{
  std::unique_lock::unique_lock cdb_locker(&cdbmutex);

  // because these two wrapper functions will be called by INSTALL/UNINSTALL/INSTALLED only,
  // and the commands will be executed with database mutex acquired,thus
  // we can simply unlock the mutex to tell DB module deal with request,and lock
  // the mutex later for get result.
  auto ExportSyncToDB = [&, exportFunc, cdb_locker]
    (const std::string &name, const std::list<std::string> &info_list) -> void {
    exportFunc(name, info_list);
    cdb_locker.unlock();
    cdb_locker.lock();
    int ret = 0;
    if (!is_db_op_succeed)
      ret = -1;
    return ret;
  };

  // we do not introduce new callback to commit delete operation on DB
  // since the import from DB routine only be called when process UNINSTALL / INSTALLED,
  // so we can catch the informations,and then process delete request later in this
  // control thread if the command is UNINSTALL,not from CMD module handler.
  std::string caught_mod_name;
  std::list<std::string> caught_import_data;

  auto ImportSyncToDB = [&, importFunc, cdb_locker]
    (const std::string &name, std::list<std::string> &info_list) -> void {
    importFunc(name, info_list);
    cdb_locker.unlock();
    cdb_locker.lock();
    int ret = 0;
    if (!is_db_op_succeed)
      ret = -1;
    caught_mod_name = name;
    caught_import_data = info_list;
    return ret;
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
        ucme->status = ret < 0 ? 0 : 2;
        if (ret < 0)
          break;
        uc_locker.unlock();
      } while(uc_locker.lock(), 1);
    } else if (cmd_handler.currentCMD() == mhwimmc_cmd_ns::UNINSTALL) {
      // let DB module delete the recoeds
      commitDELRequest(caught_mod_name, caught_import_data);
      cdb_locker.unlock();
      cdb_locker.lock();
      if (!is_db_op_succeed) {
        ucme->status = 1;
        ucme->io_buf = std::string{"error: Failed to delete DB records when process UNINSTALL."};
      }
    } else {
      ret = cmd_handler.getCMDOutput(ucme->io_buf);
      if (ret < 0) {
        // CMD no output
        ucme->status = 0;
      } else {
        ucme->status = 1;
      }
    }
        
  }
}
