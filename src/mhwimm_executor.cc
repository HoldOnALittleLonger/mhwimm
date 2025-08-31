#include "mhwimm_executor.h"

#include <cstring>
#include <cstdbool>
#include <exception>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

extern bool program_exit;

namespace mhwimm_executor_ns {

#define ERROR_MSG_MEM "error: Failed to allocate memory."
#define ERROR_MSG_ERRFORM "error: Incorrect format."
#define ERROR_MSG_UNKNOWNCMD "error: Unknown cmd."
#define ERROR_MSG_UNKNOWN " error: Unknown error."
#define ERROR_MSG_FNOEXIST "error: No such file or directory."
#define ERROR_MSG_DBREQ_FAILED "error: Database OP failed."
#define ERROR_MSG_TRAVERSE_DIR "error: Failed to traverse directory."

  static calculate_key(const char *cmd_str) -> int32_t
  {
    int32_t result(0);
    while (*cmd_str) {
      result += *cmd_str++;
    }
    return result;
  };

  int mhwimm_executor::parseCMD(const std::string &cmd_string)
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

  int mhwimm_executor::executeCurrentCMD(void)
  {
    noutput_msgs_ = 0;
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
      current_status_ = ERROR;
      generic_err_msg_output(ERROR_MSG_UNKNOWNCMD);
      return -1;
    }
  }

  int mhwimm_executor::cd(void)
  {
    // "cd" command only receives one parameter
    if (nparams_ != 1) {
      current_status_ = ERROR;
      return -1;
    }

    int ret(chdir(parameters_[0].c_str));
    return ret ? current_status_ = ERROR, -1 : current_status_ = IDLE, 0;
  }

  int mhwimm_executor::ls(void)
  {
    // open current work directory
    DIR *this_dir(opendir("."));
    struct dirent *dir(NULL);
    noutput_msgs_ = 0;

    while ((dir = readdir(DIR))) {
      if (strncmp(dir->d_name, ".", 2) || 
          strncmp(dir->d_name, "..", 3))
        continue;
      cmd_output_msgs_[noutput_msgs_++] = std::string{dir->d_name};
    }
    (void)closedir(this_dir);

    current_status_ = IDLE;
    is_cmd_has_output_ = true;
    return 0;
  }

  int mhwimm_executor::exit(void)
  {
    // we does not use concurrent protecting at there,
    // because of that the other control path will change this
    // indicator is the signal handler
    program_exit = true;
    current_status_ = IDLE;
    return 0;
  }

  int mhwimm_executor::config(void)
  {
    if (nparams_ != 1) {
      current_status_ = ERROR;
      return -1;
    }

    bool is_correct(false);
    auto equal_pos(parameters_[0].begin());

    auto check_if_correct_format = [&, this](void) -> void {
      std::size_t spaces_character(0);
      std::size_t equal_symbol(0);
      for (auto i(equal_pos); i != parameters_[0].end(); ++i) {
        if (*i == '=') {
          ++equal_symbol;
          equal_pos = i;
        }
        if (*i == ' ')
          ++spaces_character;
      }

      if (equal_symbol != 1 || spaces_character)
        is_correct = false;
      is_correct = true;
    };

    check_if_correct_format();

    if (!is_correct) {
      generic_err_msg_output(ERROR_MSG_ERRFORM);
      current_status_ = ERROR;
      return -1;
    }

    std::string key(parameters_[0].substr(0, equal_pos - parameters_[0].begin() - 1));
    std::string value(parameters_[0].substr(equal_pos - parameters_[0].begin() + 1,
                                            parameters_[0].end() - equal_pos));

#define CONFIG_USERHOME 616
#define CONFIG_MHWIROOT 633
#define CONFIG_MHWIMMCROOT 854

    switch (calculate_key(key.c_str())) {
    case CONFIG_USERHOME:
      conf_->userhome = static_cast<typename mhwimm_config_ns::the_default_config_type::skey_t>(value);
      break;
    case CONFIG_MHWIROOT:
      conf_->mwhiroot = static_cast<typename mhwimm_config_ns::the_default_config_type::skey_t>(value);
      break;
    case CONFIG_MHWIMMCROOT:
      conf_->mwhimmcroot = static_cast<typename mhwimm_config_ns::the_default_config_type::skey_t>(value);
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

  int mhwimm_executor::install(void)
  {
    // install [ mod name ] [ mod directory ]
    if (nparams_ != 2) {
      generic_err_msg_output(ERROR_MSG_ERRFORM);
      goto err_exit;
    }

    std::string modname(parameters_[0]);
    std::string moddir(parameters_[1]);

    /**
     * lf_traverse_dir - local function used to traverse the directory and makeup mod file list
     * @parent_dir:      parent directory
     * return:           0 OR -1
     * # front-inserting
     */
    auto lf_traverse_dir = [](const std::string &parent_dir) -> int {
      DIR *this_dir = opendir(parent_dir.c_str());
      if (!DIR) {
        generic_err_msg_output(ERROR_MSG_OPENDIR);
        return -1;
      }
      mfiles_list_->directory_list.insert(mfiles_list_->directory_list.begin(), parent_dir);

      int ret(0);

      while (struct dirent *dentry = readdir(this_dir)) {
        // skip this dir and parent dir
        switch (strlen(dentry->d_name)) {
        case 1:
          if (!strncmp(".", dentry->d_name, 1))
            continue;
        case 2:
          if (!strncmp("..", dentry->d_name, 2))
            continue;
        }
        
        struct stat dentry_stat = {0};
        if (stat(dentry->d_name, &dentry_stat) < 0) {
          ret = -1;
          break;
        } else if (dentry_stat.st_mode & S_IFDIR) {
          return lf_traverse_dir(parent_dir + "/" + dentry->d_name);
        } else if (dentry_stat.st_mode & S_IFLINK)
          continue;

        // regular file
        mfiles_list_->regular_file_list.insert(mfiles_list_->regular_file_list.begin(),
                                               std::string{parent_dir + "/" + dentry->d_name});
      }

      (void)closedir(this_dir);
      return ret;
    };

    // now we have to traverse the mod directory to makeup file list
    std::unique_lock<decltype(mfiles_list_->lock)> mfl_lock(&mfiles_list_->lock);
    if (lf_traverse_dir(moddir) < 0) {
      generic_err_msg_output(ERROR_MSG_TRAVERSE_DIR);
      goto err_exit;
    }

    mfiles_list_->directory_list.reverse();
    mfiles_list_->regular_file_list.reverse();

    auto mhwiroot(conf_->mhwiroot);

    // link directories
    for (auto i : mfiles_list_->directory_list) {
      if (link(i.c_str(), (mhwiroot + "/" + i).c_str()) < 0)
        goto err_exit;
    }

    // link regular files
    for (auto i : mfiles_list_->regular_file_list) {
      if (link(i.c_str(), (mhwiroot + "/" + i).c_str()) < 0)
        goto err_exit;
    }

    // now we have to issue SQL request for add records.
    // if SQL resulted in failure,then we must unlink the files
    // we have been linked previously.

    current_status_ = mhwimm_executor_status::IDLE;
    return 0;
  err_exit:
    current_status_ = mhwimm_executor_status::ERROR;
    return - 1;
  }

  int mhwimm_executor::uninstall(void)
  {
    // when removing mods from game root directory,we have to
    // take care of empty directory
    // if a directory were not empty but now it is empty because
    // we removed the mode files,then we should remove this
    // directory,too

    // uninstall [ mod name ]
    if (nparams_ != 1) {
      generic_err_msg_output(ERROR_MSG_INCFORM);
      current_status_ = ERROR;
      return -1;
    }

    std::list<std::string> db_records_list;
    if (importFromDBCallback(parameters_[0], db_records_list) < 0) {
      generic_err_msg_output(std::string{ERROR_MSG_DBREQ_FAILED} + "mod name : " + parameters_[1]);
      current_status_ = ERROR;
      return -1;
    }

    // first walk-through removes all regular files
    for (auto it(db_records_list.begin()); it != db_records_list.end(); ++it) {
      struct stat file_stat = 0;
      int ret = stat(it->c_str(), &file_stat);
      if (ret < 0)
        continue;
      else if (S_ISREG(file_stat.st_mode)) {
        unlink(it->c_str());
        db_records_list.erase(it);
      }
    }

    // second walk-through removes all directories
    for (auto x : db_records_list)
      rmdir(x.c_str());

    current_status_ = IDLE;
    return 0;
  }

  int mhwimm_executor::installed(void)
  {
    current_status_ = WORKING;
    std::list<std::string> db_records_list;
    if (importFromDBCallback("*", db_records_list) < 0) {
      generic_err_msg(ERROR_DBREQ_FAILED);
      current_status_ = ERROR;
      return -1;
    }
    noutput_msgs_ = 0;
    for (auo x : db_records_list) {
      cmd_output_msgs_[noutput_infos++] = x;
    }
    is_cmd_has_output_ = true;
    current_status_ = IDLE;
    return 0;
  }
}
