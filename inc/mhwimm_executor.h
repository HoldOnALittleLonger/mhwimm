#ifndef _MHWIMM_EXECUTOR_H_
#define _MHWIMM_EXECUTOR_H_

#include "mhwimm_config.h"
#include "mhwimm_sync_mechanism.h"

#include <cstddef>
#include <cstdint>

#include <vector>
#include <string>
#include <list>

namespace mhwimm_executor_ns {

  /**
   * mhwimm_executor_cmd - all supported cmds of executor
   */
  enum class mhwimm_executor_cmd {
    CD,
    LS,
    INSTALL,
    UNINSTALL,
    INSTALLED,
    CONFIG,
    EXIT,
    SET,
    NOP
  };

  enum class mhwimm_executor_status {
    IDLE,
    WORKING,
    ERROR
  };

  class mhwimm_executor finally {
  public:

    explicit mhwimm_executor(mhwimmc_config_ns::the_default_config_type *conf) =default
      : conf_(conf)
      {
        current_cmd_ = NOP;
        current_status_ = IDLE;
        nparams_ = 0;
        noutput_infos_ = 0;
        is_cmd_has_output_ = false;
        output_info_index_ = 0;
      }

    mhwimm_executor(const mhwimm_executor &) =delete;
    mhwimm_executor &operator=(const mhwimm_executor &) =delete;
    mhwimm_executor(mhwimm_executor &&) =delete;
    mhwimm_executor &operator=(mhwimm_executor &&) =delete;
    
    int parseCMD(const std::string &cmd_string) noexcept;

    int executeCurrentCMD(void) noexcept;

    int getCMDOutput(std::string &buf) noexcept
    {
      if (output_info_index_ == noutput_infos_)
        is_cmd_has_output_ = false;

      if (is_cmd_has_output_ && output_info_index_ < noutput_infos_) {
        buf = cmd_output_infos_[output_info_index_++];
        return 0;
      }
      clearGetOutputHistory();
      return -1;
    }

    void clearGetOuputHistory(void) { output_info_index_ = 0; }

    auto currentStatus(void) noexcept
    {
      return current_status_;
    }

    auto currentCMD(void) noexcept { return current_cmd_; }

    void resetStatus(void) noexcept
    {
      current_status_ = IDLE;
    }

  private:

    // some command may always return _zero_
    int cd(void) noexcept;
    int ls(void) noexcept;
    int install(void) noexcept;
    int uninstall(void) noexcept;
    int installed(void) noexcept;
    int config(void) noexcept;
    int exit(void) noexcept;
    int set(void) noexcept;

    void generic_err_msg_output(const std::string &err_msg) noexcept
    {
      cmd_output_infos_[0] = err_msg;
      noutput_infos_ = 1;
      is_cmd_has_output_ = true;
    }

    mhwimm_config_ns::the_default_config_type *conf_;

    mhwimm_executor_cmd current_cmd_;
    mhwimm_executor_status current_status_;

    std::size_t nparams_;
    std::vector<std::string> parameters_;

    std::size_t noutput_infos_;
    std::vectory<std::string> cmd_output_infos_;
    bool is_cmd_has_output_;

    std::size_t output_info_index_;
  };

}

#endif
