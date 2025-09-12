/**
 * Monster Hunter World Iceborne Mod Manager Executor
 * This file contains definition of Executor.
 */
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
   * command / help
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

  /* mhwimm_executor_tatus - Executor status */
  enum class mhwimm_executor_status {
    IDLE,
    WORKING,
    ERROR
  };

  /**
   * mhwimm_executor - executor of mhwimm which processes the commands
   *                   from user input,and interactive with mhwimm
   *                   database
   */
  class mhwimm_executor final {
  public:

    // constructor
    explicit mhwimm_executor(mhwimm_config_ns::config_t *conf)
      : conf_(conf), mfiles_list_(nullptr)
      {
        current_cmd_ = mhwimm_executor_cmd::NOP;
        current_status_ = mhwimm_executor_status::IDLE;
        nparams_ = 0;
        noutput_msgs_ = 0;
        is_cmd_has_output_ = false;
        output_info_index_ = 0;
        parameters_.resize(8);
        cmd_output_msgs_.resize(8);
      }

    // no destructor,because the data members of this class
    // without dynamically allocating.

    // disabled copying,moving.
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

    void clearGetOutputHistory(void) noexcept
    {
      output_info_index_ = 0;
      is_cmd_has_output_ = false;
    }

    void bypassSyntaxChecking(std::size_t new_nparams) noexcept
    {
      nparams_ = new_nparams;
    }

    auto currentCMD(void) noexcept { return current_cmd_; }
    void setCMD(mhwimm_executor_cmd cmd) noexcept { current_cmd_ = cmd; }
    auto currentStatus(void) noexcept { return current_status_; }
    void resetStatus(void) noexcept
    {
      current_status_ = mhwimm_executor_status::IDLE;
      current_cmd_ = mhwimm_executor_cmd::NOP;
    }

    void setMFLImpl(mhwimm_sync_mechanism_ns::mod_files_list *mfl) { mfiles_list_ = mfl; }

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

    // syntax checkings
    bool syntaxChecking(std::size_t req_nparams)
    {
      return nparams_ == req_nparams;
    }

    bool cmd_cd_syntaxChecking(void) { return syntaxChecking(1); }
    bool cmd_ls_syntaxChecking(void) { return true; }
    bool cmd_install_syntaxChecking(void)
    {
      if (syntaxChecking(2)) {
        std::string mod_dir(parameters_[1]);
        if (mod_dir[0] == '/')
          return false;
        return true;
      }
      return false;
    }
    bool cmd_uninstall_syntaxChecking(void) { return syntaxChecking(1); }
    bool cmd_installed_syntaxChecking(void) { return true; }

    /**
     * cmd_config_syntaxChecking - do syntax checking for command "config",
     *                             and then splite parameters if no syntax
     *                             error detected
     */
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
      std::string key(parameters_[0].substr(0, equal_pos - parameters_[0].begin()));
      std::string val(parameters_[0].substr(equal_pos - parameters_[0].begin() + 1,
                                            parameters_[0].end() - equal_pos - 1));

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
    mhwimm_sync_mechanism_ns::mod_files_list *mfiles_list_;

    mhwimm_executor_cmd current_cmd_;
    mhwimm_executor_status current_status_;

    // resize vector if necessary
    template<typename _VecType>
    void rs_vec_if_necessary(_VecType &vec, unsigned int nstored)
    {
      if (nstored == vec.capacity())
        vec.resize(2 * vec.capacity());
    }

    std::size_t nparams_;
    std::vector<std::string> parameters_;

    std::size_t noutput_msgs_;
    std::vector<std::string> cmd_output_msgs_;
    bool is_cmd_has_output_;

    std::size_t output_info_index_;
  };

}

#endif
