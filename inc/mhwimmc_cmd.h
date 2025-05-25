#ifndef _MHWIMMC_CMD_H_
#define _MHWIMMC_CMD_H_

#include "mhwimmc_config.h"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

using exportToDBCallbackFunc_t = void (*)(const std::string &, const std::list<std::string>);
using importFromCallbackFunc_t = void (*)(const std::string &, std::list<std::string>);

namespace mhwimmc_cmd_ns {

  enum class cmd {
    CD,
    LS,
    INSTALL,
    UNINSTALL,
    INSTALLED,
    CONFIG,
    EXIT,
    NOP
  };

  enum class mmc_cmd_status {
    IDLE,
    WORKING,
    ERROR
  };

  class Mmc_cmd finally {
  public:

    explicit Mmc_cmd(mhwimmc_config_ns::the_default_config_type *conf,
                       exportToDBCallbackFunc_t export_func,
                       importFromDBCallbackFunc_t import_func) =default
      : conf_(conf), exportToDBCallback_(export_func), importFromDBCallback_(import_func)
      {
        current_cmd_ = NOP;
        current_status_ = IDLE;
        nparams_ = 0;
        noutput_infos_ = 0;
        is_cmd_has_output_ = false;
      }
    
    int parseCMD(std::string &cmd_string) noexcept;

    int executeCurrentCMD(void) noexcept;

    int getCMDOutput(std::string &buf) noexcept
    {
      static std::size_t output_info_index(0);
      if (output_info_index == noutput_infos_)
        is_cmd_has_output_ = false;

      if (is_cmd_has_output_ && output_info_index < noutput_infos_) {
        buf = cmd_output_infos_[output_info_index++];
        return 0;
      }
      output_info_index = 0;
      return -1;
    }

    mmc_cmd_status currentStatus(void) noexcept
    {
      return current_status_;
    }

    void resetMmc_cmdStatus(void) noexcept
    {
      current_status_ = IDLE;
    }

  private:

    int cd(void) noexcept;
    void ls(void) noexcept;
    int install(void) noexcept;
    int uninstall(void) noexcept;
    void installed(void) noexcept;
    int config(void) noexcept;
    void exit(void) noexcept;

    void generic_err_msg_output(const std::string &err_msg) noexcept
    {
      cmd_output_infos_[0] = err_msg;
      noutput_infos_ = 1;
      is_cmd_has_output_ = true;
    }

    mhwimmc_config_ns::the_default_config_type *conf_;

    /* using the export/import callback,we are no longer have to care about db */
    exportToDBCallbackFunc_t exportToDBCallback_;
    importFromCallbackFunc_t importFromDBCallback_;

    cmd current_cmd_;
    mmc_cmd_status current_status_;

    std::size_t nparams_;
    std::vector<std::string> parameters_;

    std::size_t noutput_infos_;
    std::vectory<std::string> cmd_output_infos_;
    bool is_cmd_has_output_;
  };

}

#endif
