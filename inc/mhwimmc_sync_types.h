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

  enum class SQL_OP : uint8_t { 
    SQL_ADD,
    SQL_DEL,
    SQL_ASK
  };

  /**
   * cdbmsgexchg - structure used to represents the msg format between
   *               UI module and DB module
   * @op:          operation
   *                 SQL_ADD => add a record
   *                 SQL_DEL => delete a record
   *                 SQL_ASK => try to find a record
   * @info:        an object is type of struct table_record is used to
   *               handles table record
   *               @name => name field
   *               @full_path => full path
   *               @date => date
   */
  struct cdbmsgexchg {
    uint8_t op;

    struct table_record {
      std::string name;
      std::string full_path;
      std::string date;
    } info;
  };

  /* ucmutex - mutex used to make synchronization between UI and CMD */
  extern std::mutex ucmutex;

  /* cdbmutex - mutex used to make synchronization between CMD and DB */
  extern std::mutex cdbmutex;

  /* program_exit - boolean value used to indicates whether the program should stop */
  extern bool program_exit;

}

#endif
