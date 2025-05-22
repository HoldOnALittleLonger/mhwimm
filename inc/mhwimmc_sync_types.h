#ifndef _MHWIMMC_SYNC_TYPES_H_
#define _MHWIMMC_SYNC_TYPES_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <mutex>
#include <atomic>

/**
 * ucmsgexchg - structure used to represents the msg format between
 *              UI module and CMD module
 * @status:     bit field
 *                0 => CMD module have more msg to be sent
 *                1 => CMD module finished work,now UI module should
 *                     tell user that the next command event cycle has
 *                     been started
 * @io_buf:     message buffer
 */
struct ucmsgexchg {
  uint8_t status:1;
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
    std::string_view name;
    std::string_view full_path;
    std::string_view date;
  } info;
};

/* ucmutex - mutex used to make synchronization between UI and CMD */
extern std::mutex ucmutex;

/* cdbmutex - mutex used to make synchronization between CMD and DB */
extern std::mutex cdbmutex;

/* program_exit - boolean value used to indicates whether the program should stop */
extern bool program_exit;

#endif
