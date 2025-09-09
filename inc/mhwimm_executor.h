#ifndef _MHWIMM_EXECUTOR_H_
#define _MHWIMM_EXECUTOR_H_

#include "mhwimm_config.h"
#include "mhwimm_sync_mechanism.h"

#include <cstddef>
#include <cstdint>

#include <vector>
#include <string>
#include <list>

#include <assert.h>

namespace mhwimm_executor_ns {

  /**
   * mhwimm_executor_cmd - all supported cmds of executor
   * cd <pathname>
   * ls
   * install <mod name> <mod directory>
   * uninstall <mod name>
   * installed
   * config <key>=<value>
   * exit
   */
  enum class mhwimm_executor_cmd {
    CD,
    LS,
    INSTALL,
    UNINSTALL,
    INSTALLED,
    CONFIG,
    EXIT,
    COMMANDS,
    HELP = COMMANDS,
    NOP
  };

  enum class mhwimm_executor_status {
    IDLE,
    WORKING,
    ERROR
  };

  class mhwimm_executor finally {
  public:

    explicit mhwimm_executor(mhwimmc_config_ns::config_t *conf) =default
      : conf_(conf), mfiles_list_(nullptr)
      {
        current_cmd_ = NOP;
        current_status_ = IDLE;
        nparams_ = 0;
        noutput_msgs_ = 0;
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
      if (output_info_index_ == noutput_msgs_)
        is_cmd_has_output_ = false;

      if (is_cmd_has_output_ && output_info_index_ < noutput_msgs_) {
        buf = cmd_output_msgs_[output_info_index_++];
        return 0;
      }
      clearGetOutputHistory();
      return -1;
    }

    const auto &modNameForUNINSTALL(void) {
      assert(current_cmd_ == mhwimm_executor_cmd::UNINSTALL);
      return parameters_[0];
    }

    const auto &modNameForINSTALL(void) {
      assert(current_cmd_ == mhwimm_executor_cmd::INSTALL);
      return parameters_[0];
    }

    void clearGetOuputHistory(void) { output_info_index_ = 0; }

    auto currentStatus(void) noexcept
    {
      return current_status_;
    }

    auto currentCMD(void) noexcept { return current_cmd_; }
    void setCMD(mhwimm_executor_cmd cmd) const noexcept
    {
      current_cmd_ = cmd;
    }

    void resetStatus(void) noexcept
    {
      current_status_ = mhwimm_executor_status::IDLE;
    }

    void setMFLImpl(struct mod_files_list *mfl) { mfiles_list_ = mfl; }

  private:

    // some command may always return _zero_
    int cd(void) noexcept;
    int ls(void) noexcept;
    int install(void) noexcept;
    int uninstall(void) noexcept;
    int installed(void) noexcept;
    int config(void) noexcept;
    int exit(void) noexcept;
    int commands(void) noexcept;

    bool syntaxChecking(std::size_t req_nparams)
    {
      return nparams_ == req_nparams;
    }

    bool cmd_cd_syntaxChecking(void) { return syntaxChecking(1); }
    bool cmd_ls_syntaxChecking(void) { return true; }
    bool cmd_install_syntaxChecking(void) { return syntaxChecking(2); }
    bool cmd_uninstall_syntaxChecking(void) { return syntaxChecking(1); }
    bool cmd_installed_syntaxChecking(void) { return true; }
    bool cmd_config_syntaxChecking(void) {
      // first,must have "key = value" pair
      if (!syntaxChecking(1))
        return false;

      // second,checks if the pair is correct format
      std::size_t nspace_character(0);
      std::size_t nequal_character(0);
      auto equal_pos(parameters_[0].begin());

      for (auto idx(equal_pos); idx != parameters_[0].end(); ++idx) {
        char c(*idx);
        if (c == ' ')
          ++nspace_character;
        else if (c == '=') {
          ++nequal_character;
          equal_pos = idx;
        }
      }

      if (nspace_character || nequal_character > 1)
        return false;

      // third,splite "key=value" pair to two parameters
      std::string key(parameters_[0].substr(0, equal_pos - parameters_[0].begin() - 1));
      std::string val(parameters_[0].substr(equal_pos - parameters_[0].begin() + 1,
                                            parameters_[0].end() - equal_pos));

      parameters_[0] = key;
      parameters_[1] = val;
      return true;
    }
    bool cmd_exit_syntaxChecking(void) { return true; }
    bool cmd_commands_syntaxChecking(void) { return true; }
    bool cmd_help_syntaxChecking(void) { return cmd_commands_syntaxChecking(); }

    void generic_err_msg_output(const std::string &err_msg) noexcept
    {
      cmd_output_msgs_[0] = err_msg;
      noutput_msgs_ = 1;
      is_cmd_has_output_ = true;
    }

    // we need two external pointers
    // the first is : pointer to config structure
    // the second is : pointer to mod file list structure

    mhwimm_config_ns::config_t *conf_;
    struct mod_files_list *mfiles_list_;

    mhwimm_executor_cmd current_cmd_;
    mhwimm_executor_status current_status_;

    std::size_t nparams_;
    std::vector<std::string> parameters_;

    std::size_t noutput_msgs_;
    std::vectory<std::string> cmd_output_msgs_;
    bool is_cmd_has_output_;

    std::size_t output_info_index_;
  };

}

#endif
