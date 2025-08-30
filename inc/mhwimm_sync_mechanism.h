#ifndef _MHWIMM_SYNC_MECHANISM_H_
#define _MHWIMM_SYNC_MECHANISM_H_

#include <cstddef>
#include <cstdint>

#include <string>
#include <mutex>
#include <list>

/* C99 standard */
typedef int atomic_t;

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
   * @mutex_lock:    lock used to protext the object
   */
  enum class UIEXE_STATUS : uint8_t { EXE_NOMSG, EXE_ONEMSG, EXE_MOREMSG, UI_CMD };
  struct uiexemsgexchg {
    uint8_t status:2;
    std::string io_buf;
    std::mutex lock;
  };

  struct mod_files_list {
    std::list<std::string> regular_file_list;
    std::list<std::string> directory_list;
    std::mutex lock;
  };

  /* exedb_sync_mutex - mutex used to make synchronization between Executor and DB */
  extern std::mutex exedb_sync_mutex;

  /* program_exit - value used to indicates whether the program should stop */
  extern atomic_t program_exit;

  /* is_db_op_succeed - indicate whether the last db operation is succeed */
  extern bool is_db_op_succeed;
}

#endif
