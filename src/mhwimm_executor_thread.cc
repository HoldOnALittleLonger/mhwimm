/**
 * Executor Thread Worker
 */
#include "mhwimm_executor_thread.h"
#include "mhwimm_sync_mechanism.h"

#ifdef DEBUG
// for debug
#include <iostream>
#endif

#include <cstddef>

#include <assert.h>

using namespace mhwimm_executor_ns;

/**
 * mhwimm_executor_thread_worker - thread worker for Executor
 * @exe:                           Executor handler
 * @ctrlmsg:                       communication between Executor and UI
 * @mfiles_list:                   structure holds some containers used
 *                                 to interactive with database
 * # the works that this routine will processes :
 *     1> wait user input in @ctrlmsg
 *     2> parse command input
 *     3> if cmd is INSTALL,then send DB request attempt to retrieve the
 *        mod info for check whether this mod been installed;otherwise,
 *        install the mode and send DB request to add new records after
 *        INSTALL accomplished
 *     4> if cmd is UNINSTALL / INSTALLED,then send DB request for retrieve
 *        mod records before process the real operation
 *     5> send command output msg to UI via @ctrlmsg
 */
void mhwimm_executor_thread_worker(mhwimm_executor_ns::mhwimm_executor &exe,
                                   mhwimm_sync_mechanism_ns::uiexemsgexchg &ctrlmsg,
                                   mhwimm_sync_mechanism_ns::mod_files_list &mfiles_list)
{
  using mhwimm_sync_mechanism_ns::UIEXE_STATUS;

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

    // wait condition satisied.
    ctrlmsg.condv_sync.wait_cond_odd();
    assert(ctrlmsg.status == UIEXE_STATUS::UI_CMD);

    int ret = exe.parseCMD(ctrlmsg.io_buf);
    /* we failed to parse command input */
    if (ret || exe.currentStatus() == mhwimm_executor_status::ERROR) {
      (void)exe.getCMDOutput(ctrlmsg.io_buf);
      ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
      ctrlmsg.condv_sync.update_and_notify();
      continue;
    }

    // we must retrieve mod info before execute these two cmds.
    switch (exe.currentCMD()) {
    case mhwimm_executor_cmd::INSTALL:
      exedb_condv_sync.wait_cond_even();
      regDBop_getInstalled_Modinfo(exe.modNameForINSTALL(), &mfiles_list);
      exedb_condv_sync.update_and_notify();
      exedb_condv_sync.wait_cond_even(); // condition satisfied,returned with
      exedb_condv_sync.unlock();         // lock acquired,but we just unlock it
                                         // at there without notification,that is
                                         // because it is not the time to start
                                         // next database transaction.
      if (!is_db_op_succeed) {
        ctrlmsg.io_buf = std::string{"executor thread error: Failed to interactive"
                                     " with DB for checking whether this mod been"
                                     " installed."};
        ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
        ctrlmsg.condv_sync.update_and_notify();
        continue;
      } else {
        mfiles_list.lock.lock();
        if (mfiles_list.directory_list.size() != 0 ||
            mfiles_list.regular_file_list.size() != 0) {
          ctrlmsg.io_buf = std::string{"executor thread error: This mod been installed."};
          ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
          ctrlmsg.condv_sync.update_and_notify();
          continue;
        }
        mfiles_list.lock.unlock();
      }
      break;
    case mhwimm_executor_cmd::INSTALLED:
      exedb_condv_sync.wait_cond_even();
      regDBop_getAllInstalled_Modsname(&mfiles_list);
      exedb_condv_sync.update_and_notify();
      exedb_condv_sync.wait_cond_even();
      exedb_condv_sync.unlock();
      if (!is_db_op_succeed) {
        ctrlmsg.io_buf = std::string{"executor thread error: Failed to retrieve installed mods."};
        ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
        ctrlmsg.condv_sync.update_and_notify();
        continue;
      }
      break;
    case mhwimm_executor_cmd::UNINSTALL:
      exedb_condv_sync.wait_cond_even();
      regDBop_getInstalled_Modinfo(exe.modNameForUNINSTALL(), &mfiles_list);
      exedb_condv_sync.update_and_notify();
      exedb_condv_sync.wait_cond_even();
      exedb_condv_sync.unlock();
      if (!is_db_op_succeed) {
        ctrlmsg.io_buf = std::string{"executor thread error: Failed to retrieve mod info from DB."};
        ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
        ctrlmsg.condv_sync.update_and_notify();
        continue;
      }
    }

    exe.executeCurrentCMD();
    if (exe.currentStatus() == mhwimm_executor_status::ERROR) {
      // encountered error,now have to send error msg to UI.
      exe.getCMDOutput(ctrlmsg.io_buf);
      ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
      ctrlmsg.condv_sync.update_and_notify();
      continue;
    }

    // for INSTALL,we have to add new mod's info to DB.
    // for UNINSTALL,we have to remove the mod's info from DB.
    switch (exe.currentCMD()) {
    case mhwimm_executor_cmd::INSTALL:
      exedb_condv_sync.wait_cond_even();
      regDBop_add_mod_info(exe.modNameForINSTALL(), &mfiles_list);
      exedb_condv_sync.update_and_notify();
      exedb_condv_sync.wait_cond_even();
      exedb_condv_sync.unlock();
      if (!is_db_op_succeed) {
        /* if we failed to add new records to BD,we must undo INSTALL. */
        exe.setCMD(mhwimm_executor_ns::mhwimm_executor_cmd::UNINSTALL);
        exe.bypassSyntaxChecking(1);
        exe.executeCurrentCMD();
        ctrlmsg.io_buf = std::string{"executor error: Failed to add records to DB."};
        ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
        ctrlmsg.condv_sync.update_and_notify();
        continue;
      }
      break;
    case mhwimm_executor_cmd::UNINSTALL:
      exedb_condv_sync.wait_cond_even();
      regDBop_remove_mod_info(exe.modNameForUNINSTALL());
      exedb_condv_sync.update_and_notify();
      exedb_condv_sync.wait_cond_even();
      exedb_condv_sync.unlock();
      if (!is_db_op_succeed) {
        ctrlmsg.io_buf = std::string{"executor error: Failed to remove records from DB."};
        ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
        ctrlmsg.condv_sync.update_and_notify();
        continue;
      }
    }

#ifdef DEBUG
    std::size_t sget(0);
#endif

    // the last command accomplished without any error,now we can send
    // cmd output to UI.
    // upon there,we still holding @ctrlmsg.condv_sync
    for (; ;) {
      ret = exe.getCMDOutput(ctrlmsg.io_buf);
      if (ret) {
        ctrlmsg.status = UIEXE_STATUS::EXE_NOMSG;
        ctrlmsg.condv_sync.update_and_notify();
        break; // break with release condv
      }

#ifdef DEBUG
      sget++;
#endif
      ctrlmsg.status = UIEXE_STATUS::EXE_MOREMSG;
      ctrlmsg.condv_sync.update_and_notify();
      ctrlmsg.condv_sync.wait_cond_odd();
    }
#ifdef DEBUG
    std::cerr << "\n Count succeed get command output msg : " << sget << std::endl;
#endif
  }

}
