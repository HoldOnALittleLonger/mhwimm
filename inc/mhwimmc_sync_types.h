#ifndef _MHWIMMC_SYNC_TYPES_H_
#define _MHWIMMC_SYNC_TYPES_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <mutex>
#include <list>

using exportToDBCallbackFunc_t = void (*)(const std::string &, const std::list<std::string> &);
using importFromCallbackFunc_t = void (*)(const std::string &, std::list<std::string> &);

namespace mhwimmc_sync_type_ns {

  /**
   * ucmsgexchg - structure used to represents the msg format between
   *              UI module and CMD module
   * @status:     bit field
   *                2 => CMD module has more msg to send
   *                1 => CMD module has just one msg to send
   *                0 => CMD module has no msg to send
   * @io_buf:     message buffer
   */
  struct ucmsgexchg {
    uint8_t status:2;
    std::string io_buf;
  };

  /* ucmutex - mutex used to make synchronization between UI and CMD */
  extern std::mutex ucmutex;

  /* cdbmutex - mutex used to make synchronization between CMD and DB */
  extern std::mutex cdbmutex;

  /* program_exit - boolean value used to indicates whether the program should stop */
  extern bool program_exit;

}

#endif
