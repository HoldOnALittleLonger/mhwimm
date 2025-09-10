#include "mhwimm_executor_thread.h"
#include "mhwimm_sync_mechanism.h"

#include <cstddef>
#include <assert.h>

using namespace mhwimm_executor_ns;

/**
 * mhwimm_executor_thread_worker - thread worker for Executor
 * @exe:                           Executor handler
 * @ctrlmsg:                       communication between Executor and UI
 * # the works that this routine will processes :
 *     1> wait user input on @ctrlmsg
 *     2> parse command input
 *     3> if cmd is INSTALL,then send DB request after INSTALL accomplished
 *     4> if cmd is UNINSTALL,then send DB request for retrive mod records
 *     5> send command output msg to UI via @ctrlmsg
 */
void mhwimm_executor_thread_worker(mhwimm_executor_ns::mhwimm_executor &exe,
                                   mhwimm_sync_mechanism_ns::uiexemsgexchg &ctrlmsg,
                                   mhwimm_sync_mechanism_ns::mod_files_list &mfiles_list)
{
  using mhwimm_sync_mechanism_ns::UIEXE_STATUS;

  // lock EXE DB to stop DB event cycle.
  std::unique_lock<decltype(exedb_sync_mutex)> exedb_lock(exedb_sync_mutex);
  exe.setMFLImpl(&mfiles_list);

  for (; ;) {
    exe.resetStatus();

    // clear containers.
    mfiles_list.lock.lock();
    mfiles_list.regular_file_list.clear();
    mfiles_list.directory_list.clear();
    mfiles_list.mod_name_list.clear();
    mfiles_list.lock.unlock();

    if (program_exit) // shall we stop and exit?
      break;

    NOP_DELAY();
    // try to lock ctrl msg object for get user input.
    std::unique_lock<decltype(ctrlmsg.lock)> exeui_lock(ctrlmsg.lock);
    
    // if the status is not UI_CMD when entered Executor thread,then
    // there must be a fatal error was encountered.
    //    assert(ctrlmsg.status == UIEXE_STATUS::UI_CMD);

    int ret = exe.parseCMD(ctrlmsg.io_buf);
    /* we failed to parse command input */
    if (ret || exe.currentStatus() == mhwimm_executor_status::ERROR) {
      (void)exe.getCMDOutput(ctrlmsg.io_buf);
      ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
      continue;
    } else if (exe.currentCMD() == mhwimm_executor_cmd::NOP)
      continue;

    // we must retrive mod info before execute these two cmds.
    switch (exe.currentCMD()) {
    case mhwimm_executor_cmd::INSTALLED:
      regDBop_getAllInstalled_Modsname(&mfiles_list);
      exedb_lock.unlock();
      NOP_DELAY();
      exedb_lock.lock();
      if (!is_db_op_succeed) {
        ctrlmsg.io_buf = std::string{"error: Failed to retrive installed mods."};
        ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
        continue;
      }
      break;
    case mhwimm_executor_cmd::UNINSTALL:
      {
        const auto &mod_name(exe.modNameForUNINSTALL());
        regDBop_getInstalled_Modinfo(mod_name, &mfiles_list);
        exedb_lock.unlock();
        NOP_DELAY();
        exedb_lock.lock();
        if (!is_db_op_succeed) {
          ctrlmsg.io_buf = std::string{"error: Failed to retrive mod info from DB."};
          ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
          continue;
        }
      }
      break;
    }

    exe.executeCurrentCMD();
    if (exe.currentStatus() == mhwimm_executor_status::ERROR) {
      // encountered error,now have to send error msg to UI.
      exe.getCMDOutput(ctrlmsg.io_buf);
      ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
      continue;
    }

    // for INSTALL,we have to add new mod's info to DB.
    // for UNINSTALL,we have to remove the mod's info from DB.
    switch (exe.currentCMD()) {
    case mhwimm_executor_cmd::INSTALL:
      regDBop_add_mod_info(exe.modNameForINSTALL(), &mfiles_list);
      exedb_lock.unlock();
      exedb_lock.lock();
      if (!is_db_op_succeed) {
        /* if we failed to add new records to BD,we must undo INSTALL. */
        exe.setCMD(mhwimm_executor_ns::mhwimm_executor_cmd::UNINSTALL);
        exe.executeCurrentCMD();
        ctrlmsg.io_buf = std::string{"error: Failed to add records to DB."};
        ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
        continue;
      }
    case mhwimm_executor_cmd::UNINSTALL:
      regDBop_remove_mod_info(exe.modNameForUNINSTALL());
      exedb_lock.unlock();
      exedb_lock.lock();
      if (!is_db_op_succeed) {
        ctrlmsg.io_buf = std::string{"error: Failed to remove records from DB."};
        ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
        continue;
      }
    }

    // the last command accomplished without any error,now we can send
    // cmd output to UI.
    for (; ;) {
      ret = exe.getCMDOutput(ctrlmsg.io_buf);
      if (ret) {
        ctrlmsg.status = UIEXE_STATUS::EXE_NOMSG;
        break;
      }
      ctrlmsg.status = UIEXE_STATUS::EXE_MOREMSG;
      exeui_lock.unlock();
      NOP_DELAY();
      exeui_lock.lock();
    }


  }

}

