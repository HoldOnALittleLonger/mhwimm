#include "mhwimmc_cmd.h"

#include <cstring>
#include <cstdbool>
#include <exception>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

extern bool program_exit;

namespace mhwimmc_cmd_ns {

#define ERROR_MSG_MEM "error: Failed to allocate memory."
#define ERROR_MSG_INCFORM "error: Incorrect format."
#define ERROR_MSG_UNKNOWN " error: Unknown error."
#define ERROR_MSG_FNOEXIST "error: No such file or directory."

  static calculate_key(const char *cmd_str) -> int32_t
  {
    int32_t result(0);
    while (*cmd_str) {
      result += *cmd_str++;
    }
    return result;
  };

  int Mmc_cmd::parseCMD(std::string &cmd_string)
  {
    current_status_ = WORKING;
    
    char *cmd_tmp_buf(nullptr);
    std::size_t buf_length(cmd_string.length() + 1);

    try {
      cmd_tmp_buf = new char[buf_length];
    } catch (std::bad_alloc &err) {
      generic_err_msg_output(ERROR_MSG_MEM);
      current_status_ = ERROR;
      return -1;
    }

    memset(cmd_tmp_buf, 0, buf_length);
    memcpy(cmd_tmp_buf, cmd_string.c_str(), buf_length - 1);

    const char *arg(strtok(cmd_tmp_buf, ' '));

    bool parse_more(false);

#define CD_CMD_KEY 199
#define LS_CMD_KEY 223
#define INSTALL_CMD_KEY 759
#define UNINSTALL_CMD_KEY 986
#define INSTALLED_CMD_KEY 960
#define CONFIG_CMD_KEY 630
#define EXIT_CMD_KEY 442

    switch (calculate_key(arg)) {
    case EXIT_CMD_KEY:
      current_cmd_ = EXIT;
      break;
    case LS_CMD_KEY:
      current_cmd_ = LS;
      break;
    case INSTALLED_CMD_KEY:
      current_cmd_ = INSTALLED;
      break;
    case CD_CMD_KEY:
      current_cmd_ = CD;
      parse_more = true;
      break;
    case INSTALL_CMD_KEY:
      cuurrent_cmd_ = INSTALL;
      parse_more = true;
      break;
    case UNINSTALL_CMD_KEY:
      current_cmd_ = UNINSTALL;
      parse_more = true;
      break;
    case CONFIG_CMD_KEY:
      current_cmd_ = CONFIG;
      parse_more = true;
      break;
    default:
      current_cmd_ = NOP;
    }

#undef CD_CMD_KEY
#undef LS_CMD_KEY
#undef INSTALL_CMD_KEY
#undef UNINSTALL_CMD_KEY
#undef INSTALLED_CMD_KEY
#undef CONFIG_CMD_KEY
#undef EXIT_CMD_KEY

    if (parse_more) {
      nparams_ = 0; // reset number of parameters
      while ((arg = strtok(cmd_tmp_buf, ' '))) {
        parameters_[nparams_++] = arg;
      }
    }

    current_status_ = IDLE;
    delete[] cmd_tmp_buf;
    return 0;
  }

  int Mmc_cmd::executeCurrentCMD(void)
  {
    if (current_status_ & (ERROR  | WORKING))
      return -1;
    current_status_ = WORKING;

    switch (current_cmd_) {
    case CD:
      return cd();
    case LS:
      return ls();
    case INSTALL:
      return install();
    case UNINSTALL:
      return uninstall();
    case INSTALLED:
      return installed();
    case CONFIG:
      return config();
    case EXIT:
      return exit();
    default:
      current_status_ = IDLE;
      return 0;
    }
  }

  int Mmc_cmd::cd(void)
  {
    // "cd" command only receives one parameter
    if (nparams_ != 1) {
      current_status_ = ERROR;
      return -1;
    }

    int ret(chdir(parameters_[0].c_str));

    return ret ? current_status_ = ERROR, -1 : current_status_ = IDLE, 0;
  }

  int Mmc_cmd::ls(void)
  {
    if (nparams_) {
      current_status_ = ERROR;
      return -1;
    }

    // open current work directory
    DIR *this_dir(opendir("."));
    struct dirent *dir(NULL);
    noutput_infos_ = 0;

    while ((dir = readdir(DIR))) {
      if (strncmp(dir->d_name, ".", 2) || 
          strncmp(dir->d_name, "..", 3))
        continue;
      cmd_output_infos_[noutput_infos_++] = std::string{dir->d_name};
    }

    current_status_ = IDLE;
    is_cmd_has_output_ = true;
    (void)closedir(this_dir);
    return 0;
  }

  int Mmc_cmd::exit(void)
  {
    program_exit = true;
    current_status_ = IDLE;
    return 0;
  }

  int Mmc_cmd::config(void)
  {
    if (nparams_ != 1) {
      current_status_ = ERROR;
      return -1;
    }

    bool is_correct(false);
    std::string::iterator equal_pos(parameters_[0].begin());

    auto check_if_correct_format = [&](void) -> void {
      std::size_t spaces_character(0);
      std::size_t equal_symbol(0);
      for (auto i(equal_pos); i != parameters_[0].end(); ++i) {
        if (x == '=') {
          ++equal_symbol;
          equal_pos = i;
        }
        if (x == ' ')
          ++spaces_character;
      }

      if (equal_symbol != 1 || spaces_character)
        is_correct = false;
      is_correct = true;
    };

    check_if_correct_format();

    if (!is_correct) {
      generic_err_msg_output(ERROR_MSG_INCFORM);
      current_status_ = ERROR;
      return -1;
    }

    config_skey_t key(
                      parameters_[0].substr(0, equal_pos - parameters_[0].begin() - 1));
    config_skey_t value(
                        parameters_[0].substr(equal_pos - parameters_[0].begin() + 1,
                                              parameters_[0].end() - equal_pos));

#define CONFIG_USERHOME 616
#define CONFIG_MHWIROOT 633
#define CONFIG_MHWIMMCROOT 854

    switch (calculate_key(key.c_str())) {
    case CONFIG_USERHOME:
      conf_->userhome = value;
      break;
    case CONFIG_MHWIROOT:
      conf_->mwhiroot = value;
      break;
    case CONFIG_MHWIMMCROOT:
      conf_->mwhimmcroot = value;
      break;
    default: // unknown config,do nothing
      ;
    }

#undef CONFIG_USERHOME
#undef CONFIG_MHWIROOT
#undef CONFIG_MHWIMMCROOT

    current_status_ = IDLE;
    return 0;
  }

  int Mmc_cm::install(void)
  {
    // install [ mod name ] [ mod directory ]
    if (nparams_ != 2) {
      generic_err_msg_output(ERROR_MSG_INCFORM);
      current_status_ = ERROR;
      return -1;
    }

    const the_config_skey_t &path(conf_->mhwiroot);
    char *cwd_path_buf(nullptr);
    long path_max(pathconf("/", _PC_PATH_MAX));

    try {
      cwd_path_buf = new char[path_max];
    } catch (std::alloc_bad &ec) {
      generic_err_msg_output(ERROR_MSG_MEM);
      current_status_ = ERROR;
      return -1;
    }
    
    // store current work directory path
    getcwd(cwd_path_buf, path_max);
    
    const std::string &mod_name(parameters_[0]);
    const std::string &mod_directory(parameters_[1]);

    // get into mod directory
    errno = 0;
    if (chdir(mod_directory.c_str()) < 0) {
      generic_err_msg_output(errno == ENOENT
                             ? ERROR_MSG_FNOEXIST : ERROR_MSG_UNKNOWN);
      delete[] cwd_path_buf;
      current_status_ = ERROR;
      return -1;
    }

    // list used to stores the files path records,
    // include directory files.
    std::list<std::string> mod_file_path_list;
    
    std::string mhwiroot(path);

    // temporary string object used during iterating mod directory.
    std::string tmp_dir(".");

    auto install_mod_files = [&](std::string &dir) -> void {
      DIR *current_dir = opendir(dir.c_str);
      if (!current_dir)
        return;

      // an object used to  backup the directory path of
      // recursion for this time.
      std::string cwd_backup(dir);

      struct *dirent dirent = NULL;

      while ((dirent = readdir(current_dir))) {
        if (strncmp(dirent->d_name, ".", 2) ||
            strncmp(dirent->d_name, "..", 3))
          continue;

        // build file path record
        std::string tmp_full_path(mhwiroot);
        tmp_full_path += "/";
        if (dir != "." ) {
          // skip "./"
          auto tmpstr(dir);
          if (tmpstr.substr(0, 2) == "./")
            tmpstr = tmpstr.substr(3, tmpstr.end() - tmpstr.begin() - 2);
          tmp_full_path += tmpstr;
          tmp_full_path += "/";
        }

        tmp_full_path += dirent->d_name;

        if (dirent->d_type == DT_REG) {
          link((dir + dirent->d_name).c_str(), tmp_full_path.c_str());
          mod_file_path_list.insert(tmp_full_path);
        } else if (dirent->d_type == DT_DIR) {
          link((dir + dirent->d_name).c_str(), tmp_full_path.c_str());
          mod_file_path_list.insert(tmp_full_path);

          // recursive to subdir
          dir += "/";
          dir += dirent->d_name;
          install_mod_files(dir);

          dir = cwd_backup;
        }

      }

      closedir(current_dir);
    };

    install_mod_files(tmp_dir);

    chdir(cwd_path_buf);

    delete[] cwd_path_buf;
    current_status_ = IDEL;
    return 0;
  }
}
