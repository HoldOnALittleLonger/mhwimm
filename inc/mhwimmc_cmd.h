#ifndef _MHWIMMC_CMD_H_
#define _MHWIMMC_CMD_H_

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

struct sqlite3;

namespace CMD {

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

    Mmc_cmd(sqlite3 *db_handler, struct config_struct *conf) =default
      : db_handler_(db_handler), conf_(conf)
    {
      current_cmd_ = NOP;
      current_status_ = IDLE;
      nparams_ = 0;
      noutput_infos_ = 0;
      is_cmd_has_output_ = false;
    }
    
    int parseCMD(std::string &cmd_string) noexcept;

    int executeCurrentCMD(void) noexcept;

    int getCMDOutput(std::string &buf)
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

    mmc_cmd_status currentStatus(void)
    {
      return current_status_;
    }

    void resetMmc_cmdStatus(void)
    {
      current_status_ = IDLE;
    }

  private:

    int cd(void);
    int ls(void);
    int install(void);
    int uninstall(void);
    int installed(void);
    int config(void);
    int exit(void);

    sqlite3 *db_handler_;
    struct config_struct *conf_;

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
