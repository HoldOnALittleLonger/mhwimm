/**
 * Executor Thread Worker
 * The synchronization between UI and Executor :
 *   UI wait until the value becomes even
 *   Executor wait until the value becomes odd
 *   - UI process at first,Executor at second
 * The synchronization between Executor and DB :
 *   Executor wait until the value becomes even
 *   DB wait until the value becomes odd
 *   - Executor process at first,Executor at second
 *     in one period.
 */
#include "mhwimm_executor_thread.h"
#include "mhwimm_sync_mechanism.h"

#ifdef DEBUG
// for debug
#include <iostream>
#endif

#include <cstddef>
#include <cassert>

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

  makeup_uniquelock_and_associate_condv(exeui_lock, ctrlmsg.condv_sync);
  exeui_lock.unlock();

  makeup_uniquelock_and_associate_condv(exedb_lock, exedb_condv_sync);
  exedb_lock.unlock();

  exe.setMFLImpl(&mfiles_list);
  for (; !program_exit;) {
    // clear containers and status.
    mfiles_list.lock.lock();
    mfiles_list.regular_file_list.clear();
    mfiles_list.directory_list.clear();
    mfiles_list.mod_name_list.clear();
    mfiles_list.lock.unlock();
    exe.resetStatus();

    // It is my round now !
    ctrlmsg.condv_sync.wait_cond_odd(exeui_lock);
    assert(ctrlmsg.status == UIEXE_STATUS::UI_CMD);
    int ret = exe.parseCMD(ctrlmsg.io_buf);
    if (exe.currentStatus() == mhwimm_executor_status::ERROR) {
      // Failed to parse user input
      (void)exe.getCMDOutput(ctrlmsg.io_buf);
      ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
      ctrlmsg.condv_sync.update_and_notify(exeui_lock);
      continue;
    }

    /**
     * Commands INSTALL, INSTALLED, UNINSTALL all of them
     * must interactive with DB.
     * INSTALL interactive to DB twice,one before
     * do INSTALL,another one after done INSTALL.
     * - (1)> is the mod been installed?
     *   (2)> add the mod's info to database table
     * INSTALLED interactive to DB once,before do INSTALLED.
     * - (1)> retrieve the mod info from database
     * UNINSTALL interactive to DB twice,one before
     * do UNINSTALL,another one after done UNINSTALL.
     * - (1)> retrieve the mod info from database
     *   (2)> request database remove these records
     *
     * Synchronization :
     *   we get the condition variable at first,then
     *   request DB operation,and put the condition
     *   variable let DB start its work.
     *   after the operation accomplished,we get
     *   the condition variable again,but do not
     *   put it,just unlock as well.because we
     *   do not want DB to start a new transaction,
     *   we have not installed a new one.
     *
     * Before :
     *   For INSTALL and UNINSTALL,we can combine them
     *   together.
     *   For INSTALLED,we need all installed mods' name.
     * After :
     *   For INSTALL,request DB add
     *   For UNINSTALL,request DB remove
     */

    // Before
    if (exe.currentCMD() == mhwimm_executor_cmd::INSTALL ||
        exe.currentCMD() == mhwimm_executor_cmd::UNINSTALL ||
        exe.currentCMD() == mhwimm_executor_cmd::INSTALLED) {
      // It is my round now!
      exedb_condv_sync.wait_cond_even(exedb_lock);

      // register DB operation.
      switch (exe.currentCMD()) {
      case mhwimm_executor_cmd::INSTALL:
      case mhwimm_executor_cmd::UNINSTALL:
        regDBop_getInstalled_Modinfo(exe.getCurrentModName(), &mfiles_list);
        break;
      case mhwimm_executor_cmd::INSTALLED:
        regDBop_getAllInstalled_Modsname(&mfiles_list);
      }

      // Round finished.
      exedb_condv_sync.update_and_notify(exedb_lock);

      exedb_condv_sync.wait_cond_even(exedb_lock);
      exedb_condv_sync.unlock(exedb_lock); // DB stopped.

      if (is_db_op_succeed) {
        if (exe.currentCMD() == mhwimm_executor_cmd::INSTALL &&
            (mfiles_list.regular_file_list.size() != 0 ||
             mfiles_list.directory_list.size() != 0)) {
          // this mod been installed.
          ctrlmsg.io_buf = std::string{"executor thread error: This mod been installed."};
          ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
          ctrlmsg.condv_sync.update_and_notify(exeui_lock);
          continue;
        }
        // else UNINSTALL or no files or (UNINSTALL and no files)
      } else
        goto failed_db_interact_checking;
    }

    // Executor process the parsed command.
    exe.executeCurrentCMD();
    if (exe.currentStatus() == mhwimm_executor_status::ERROR) {
      // encountered error,now have to send error msg to UI.
      exe.getCMDOutput(ctrlmsg.io_buf);
      ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
      ctrlmsg.condv_sync.update_and_notify(exeui_lock);
      continue;
    }

    // After
    if (exe.currentCMD() == mhwimm_executor_cmd::INSTALL ||
        exe.currentCMD() == mhwimm_executor_cmd::UNINSTALL) {
      // It is my round now!
      exedb_condv_sync.wait_cond_even(exedb_lock);

      switch (exe.currentCMD()) {
      case mhwimm_executor_cmd::INSTALL:
        regDBop_add_mod_info(exe.getCurrentModName(), &mfiles_list);
        break;
      case mhwimm_executor_cmd::UNINSTALL:
        regDBop_remove_mod_info(exe.getCurrentModName());
      }
      // Round finished.
      exedb_condv_sync.update_and_notify(exedb_lock);

      exedb_condv_sync.wait_cond_even(exedb_lock);
      exedb_condv_sync.unlock(exedb_lock);

    failed_db_interact_checking:
      if (!is_db_op_succeed) {
        if (exe.currentCMD() == mhwimm_executor_cmd::INSTALL) {
          // we failed on add record for INSTALL,thus have to undo INSTALL.
          exe.setCMD(mhwimm_executor_cmd::UNINSTALL);
          exe.bypassSyntaxChecking(1);
          if (exe.executeCurrentCMD() < 0) {
            // undo INSTALL failed,
            // stop application.
            exe.setCMD(mhwimm_executor_cmd::EXIT);
            exe.bypassSyntaxChecking(0);
            exe.executeCurrentCMD();
            ctrlmsg.io_buf = std::string{"executor thread error: "
                                         "Failed to interactive with DB for INSTALL,"
                                         "and failure undo INSTALL,application stopping!"};
            ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
            ctrlmsg.condv_sync.update_and_notify(exeui_lock);
            continue;
          }
        }
        ctrlmsg.io_buf = std::string{"executor thread error: Failed to interactive"
                                     " with DB."};
        ctrlmsg.status = UIEXE_STATUS::EXE_ONEMSG;
        ctrlmsg.condv_sync.update_and_notify(exeui_lock);
        continue;
      }
    }

#ifdef DEBUG
    std::size_t sget(0);
#endif

    // From there,we send cmd output to UI.
    // We still holding the condition variable,now.
    for (; ;) {
      ret = exe.getCMDOutput(ctrlmsg.io_buf);
      if (ret) {
        ctrlmsg.status = UIEXE_STATUS::EXE_NOMSG;
        ctrlmsg.condv_sync.update_and_notify(exeui_lock);
        break; // break with release condv
      }

#ifdef DEBUG
      sget++;
#endif
      ctrlmsg.status = UIEXE_STATUS::EXE_MOREMSG;
      ctrlmsg.condv_sync.update_and_notify(exeui_lock);
      ctrlmsg.condv_sync.wait_cond_odd(exeui_lock);
    }
#ifdef DEBUG
    std::cerr << "\n Count succeed get command output msg : " << sget << std::endl;
#endif
  }

  exedb_condv_sync.wait_cond_even(exedb_lock);
  exedb_condv_sync.update_and_notify(exedb_lock); // release to let DB executing.
}
