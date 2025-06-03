#ifndef _MHWIMMC_CMD_H_
#define _MHWIMMC_CMD_H_

#include "mhwimmc_config.h"
#include "mhwimmc_sync_types.h"

#include <cstddef>
#include <cstdint>

#include <vector>
#include <string>
#include <list>

namespace mhwimmc_cmd_ns {

  using mhwimmc_sync_type_ns::exportToDBCallbackFunc_t;
  using mhwimmc_sync_type_ns::importFromCallbackFunct_;

  static int nullExportDBCallback(const std::string &name, const std::list<std::string> &records)
  {
  }

  static int nullImportDBCallback(const std::string &name, std::list<std::string> &records)
  {
  }

  enum class mhwimmc_cmd_cmd {
    CD,
    LS,
    INSTALL,
    UNINSTALL,
    INSTALLED,
    CONFIG,
    EXIT,
    NOP
  };

  enum class mhwimmc_cmd_status {
    IDLE,
    WORKING,
    ERROR
  };

  class mhwimmc_cmd finally {
  public:

    explicit mhwimmc_cmd(mhwimmc_config_ns::the_default_config_type *conf) =default
      : conf_(conf)
      {
        current_cmd_ = NOP;
        current_status_ = IDLE;
        nparams_ = 0;
        noutput_infos_ = 0;
        is_cmd_has_output_ = false;
        output_info_index_ = 0;
        exportToDBCallback_ = nullExportDBCallback;
        importFromDBCallback_ = nullImportDBCallback;
      }

    mhwimmc_cmd(const mhwimmc_cmd &) =delete;
    mhwimmc_cmd &operator=(const mhwimmc_cmd &) =delete;
    mhwimmc_cmd(mhwimmc_cmd &&) =delete;
    mhwimmc_cmd &operator=(mhwimmc_cmd &&) =delete;
    
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

    void *registerExportCallback(exportToDBCallbackFunc_t func)
    {
      auto x(exportToDBCallback_);
      exportToDBCallback_ = func;
      return x;
    }

    void *registerImportCallback(importFromDBCallbackFunc_t func)
    {
      auto x(importFromDBCallback_);
      importFromDBCallback_ = func;
      return x;
    }

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

    mhwimmc_cmd_cmd current_cmd_;
    mhwimmc_cmd_status current_status_;

    std::size_t nparams_;
    std::vector<std::string> parameters_;

    std::size_t noutput_infos_;
    std::vectory<std::string> cmd_output_infos_;
    bool is_cmd_has_output_;

    std::size_t output_info_index_;
  };

}

#endif
