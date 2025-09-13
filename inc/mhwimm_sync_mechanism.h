/**
 * Synchronous Mechanisms Between UI,Executor,and Database
 */
#ifndef _MHWIMM_SYNC_MECHANISM_H_
#define _MHWIMM_SYNC_MECHANISM_H_

#include <cstddef>
#include <cstdint>

#include <string>
#include <mutex>
#include <condition_variable>
#include <list>
#include <cassert>

#include "mhwimm_database.h"

/* makeup_uniquelock_and_associate_condv
 *   - macro function used to build a local variable named @name
 *     is type of std::unique_lock<std::mutex>,and initializer
 *     it with @condv.lock_data.
 *     the unique lock is locked by default.
 */
#define makeup_uniquelock_and_associate_condv(name, condv) \
  std::unique_lock<std::mutex> name((condv).lock_data)

/* C99 standard */
typedef int atomic_t;

namespace mhwimm_sync_mechanism_ns {

  /**
   * conditionv - condition variable synchronization
   * data members:
   *   @lock_data:  std::mutex object
   *   @condv:      condition variable
   *   @value:      sequence counter
   * methods:
   *   @conditionv:       constructor
   *   @wait_cond_odd:    wait @value becomes odd
   *   @wait_cond_even:   wait @value becomes even
   *   @unlock:           unlock @lock
   *   @update:           increase @value
   *   @notify_one:       notify one instance which is waiting
   *                      on @condv
   *   @update_and_notify:    shortcut for unlock-update-notify
   */
  struct conditionv {
    std::mutex lock_data;
    std::condition_variable condv;
    atomic_t value;

    conditionv()
      : lock_data()
    {
      value = 0;
    }

    void wait_cond_v(std::unique_lock<std::mutex> &unlocked_ul, int v)
    {
      assert(unlocked_ul.mutex() == &this->lock_data);
      unlocked_ul.lock();
      this->condv.wait(unlocked_ul,
                       [&, this](void) -> bool {
                         return this->value == v;
                       });
    }

    void wait_cond_odd(std::unique_lock<std::mutex> &unlocked_ul)
    {
      assert(unlocked_ul.mutex() == &this->lock_data);
      unlocked_ul.lock();
      this->condv.wait(unlocked_ul,
                       [&, this](void) -> bool {
                         return this->value % 2;
                       });
    }

    void wait_cond_even(std::unique_lock<std::mutex> &unlocked_ul)
    {
      assert(unlocked_ul.mutex() == &this->lock_data);
      unlocked_ul.lock();
      this->condv.wait(unlocked_ul,
                       [&, this](void) -> bool {
                         return !(this->value % 2);
                       });
    }

    void unlock(std::unique_lock<std::mutex> &locked_ul)
    { 
      assert(locked_ul.mutex() == &this->lock_data);
      locked_ul.unlock();
    }

    void update(void) { ++value; }
    void notify_one(void) { condv.notify_one(); }

    void update_and_notify(std::unique_lock<std::mutex> &locked_ul)
    {
      update();
      unlock(locked_ul);
      notify_one();
    }
  };

  /* UIEXE_STATUS - enumerators for uiexemsgexchg.status */
  enum class UIEXE_STATUS : uint8_t { EXE_NOMSG, EXE_ONEMSG, EXE_MOREMSG, UI_CMD };

  /**
   * uiexemsgexchg - structure used to represents the message exechanging
   *                 between UI and Executor
   * @status:        current status of UI or Executor,used to tell each
   *                 other what is the current status and what is the type
   *                 of this message
   * @new_msg:       indicator for whether new message been produced or the
   *                 recently produced message been retrieved
   * @io_buf:        message buffer
   * @lock:          lock used to protect the object,also used to make a
   *                 sync between UI and Executor
   */
  struct uiexemsgexchg {
    UIEXE_STATUS status;
    std::string io_buf;
    struct conditionv condv_sync;
  };

  /**
   * mod_files_list - structure used to store mod file info
   * @regular_file_list:    regular file list
   * @directory_list:       directory list
   * @mod_name_list:        mod name list from DB
   * @lock:                 concurrent access protection
   */
  struct mod_files_list {
    std::list<std::string> regular_file_list;
    std::list<std::string> directory_list;
    std::list<std::string> &mod_name_list = directory_list;
    std::mutex lock;
  };

  /**
   * INTEREST_FIELD - enumerate fields that user interest to
   *                  get,these enumerators are used to establish
   *                  communication between database thread worker
   *                  and user
   * @NO_INTEREST:    nothing want to know
   * @INTEREST_NAME:  want mod_name field
   * @INTEREST_PATH:  want file_path field
   * @INTEREST_DATE:  want install_date field
   */
  enum INTEREST_FIELD : uint8_t {
    NO_INTEREST = 0,
    INTEREST_NAME = 1,
    INTEREST_PATH,
    INTEREST_DATE
  };
  using interest_db_field_t = uint8_t;
}

/* program_exit - value used to indicates whether the program should stop */
extern atomic_t program_exit;

/* exedb_sync - synchronization between Executor and DB */
extern mhwimm_sync_mechanism_ns::conditionv exedb_condv_sync;

/* is_db_op_succeed - indicate whether the last db operation is succeed */
extern bool is_db_op_succeed;

/* Database Register Helpers related */
extern mhwimm_sync_mechanism_ns::interest_db_field_t interest_field;
extern mhwimm_sync_mechanism_ns::mod_files_list *mfl_for_db;

extern void regDBop_getAllInstalled_Modsname(typename mhwimm_sync_mechanism_ns::mod_files_list *mfl);
extern void regDBop_getInstalled_Modinfo(const std::string &modname,
                                         typename mhwimm_sync_mechanism_ns::mod_files_list *mfl);
extern void regDBop_add_mod_info(const std::string &modname,
                                 typename mhwimm_sync_mechanism_ns::mod_files_list *mfl);
extern void regDBop_remove_mod_info(const std::string &modname);
extern void init_regDB_routines(typename mhwimm_db_ns::mhwimm_db *db_impl);

#endif
