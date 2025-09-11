#ifndef _MHWIMM_SYNC_MECHANISM_H_
#define _MHWIMM_SYNC_MECHANISM_H_

#include <cstddef>
#include <cstdint>

#include <string>
#include <mutex>
#include <list>

#include "mhwimm_database.h"

#define NOP_DELAY() for (int i(10240); i > 0; --i)

namespace mhwimm_sync_mechanism_ns {

  /**
   * uiexemsgexchg - structure used to represents the msg format between
   *                 UI module and Executor module
   * @status:        bit field
   *                   3 => UI module request
   *                   2 => Executor module has more msg to send
   *                   1 => Executor module has just one msg to send
   *                   0 => Executor module has no msg to send
   * @io_buf:        message buffer
   * @lock:          lock used to protext the object
   */
  enum class UIEXE_STATUS : uint8_t { EXE_NOMSG, EXE_ONEMSG, EXE_MOREMSG, UI_CMD };
  struct uiexemsgexchg {
    UIEXE_STATUS status;
    unsigned int new_msg:1;
    std::string io_buf;
    std::mutex lock;
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

}

/* C99 standard */
typedef int atomic_t;

/* program_exit - value used to indicates whether the program should stop */
extern atomic_t program_exit;

/* exedb_sync_mutex - mutex used to make synchronization between Executor and DB */
extern std::mutex exedb_sync_mutex;

/* is_db_op_succeed - indicate whether the last db operation is succeed */
extern bool is_db_op_succeed;

extern void regDBop_getAllInstalled_Modsname(typename mhwimm_sync_mechanism_ns::mod_files_list *mfl);
extern void regDBop_getInstalled_Modinfo(const std::string &modname,
                                         typename mhwimm_sync_mechanism_ns::mod_files_list *mfl);
extern void regDBop_add_mod_info(const std::string &modname,
                                 typename mhwimm_sync_mechanism_ns::mod_files_list *mfl);
extern void regDBop_remove_mod_info(const std::string &modname);
extern void init_regDB_routines(typename mhwimm_db_ns::mhwimm_db *db_impl);

#endif
