#include "mhwimm_executor.h"

#include <cstring>
#include <cstdbool>
#include <cstdint>
#include <exception>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

namespace mhwimm_executor_ns {

#define ERROR_MSG_MEM "error: Failed to allocate memory."
#define ERROR_MSG_CHDIR "error: Failed enter the directory."
#define ERROR_MSG_ERRFORM "error: Incorrect format."
#define ERROR_MSG_UNKNOWNCMD "error: Unknown cmd."
#define ERROR_MSG_UNKNOWNCONF "error: Unknown conf."
#define ERROR_MSG_FNOEXIST "error: No such file or directory."
#define ERROR_MSG_DBREQ_FAILED "error: Database OP failed."
#define ERROR_MSG_TRAVERSE_DIR "error: Failed to traverse directory."
#define ERROR_MSG_LINK "error: Failed to install mod."
#define ERROR_MSG_UNLINK "error: Failed to uninstall mod."

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
    current_status_ = mhwimm_executor_status::WORKING;

    // clear all elements in the vector.
    parameters_.clear();
    
    char *cmd_tmp_buf(nullptr);
    std::size_t buf_length(cmd_string.length() + 1);

    try {
      cmd_tmp_buf = new char[buf_length];
    } catch (std::bad_alloc &err) {
      generic_err_msg_output(ERROR_MSG_MEM);
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }

    memset(cmd_tmp_buf, 0, buf_length);
    memcpy(cmd_tmp_buf, cmd_string.c_str(), buf_length - 1);

    const char *arg(strtok(cmd_tmp_buf, ' '));

    bool parse_more(false);

    switch (calculate_key(arg)) {
    case EXIT_CMD_KEY:
      setCMD(mhwimm_executor_cmd::EXIT);
      break;
    case LS_CMD_KEY:
      setCMD(mhwimm_executor_cmd::LS);
      break;
    case INSTALLED_CMD_KEY:
      setCMD(mhwimm_executor_cmd::INSTALLED);
      break;
    case CD_CMD_KEY:
      setCMD(mhwimm_executor_cmd::CD);
      parse_more = true;
      break;
    case INSTALL_CMD_KEY:
      setCMD(mhwimm_executor_cmd::INSTALL);
      parse_more = true;
      break;
    case UNINSTALL_CMD_KEY:
      setCMD(mhwimm_executor_cmd::UNINSTALL);
      parse_more = true;
      break;
    case CONFIG_CMD_KEY:
      setCMD(mhwimm_executor_cmd::CONFIG);
      parse_more = true;
      break;
    default:
      setCMD(mhwimm_executor_cmd::NOP);
    }

    if (parse_more) {
      nparams_ = 0; // reset number of parameters
      while ((arg = strtok(cmd_tmp_buf, ' '))) {
        parameters_[nparams_++] = arg;
      }
    }

    current_status_ = mhwimm_executor_status::IDLE;
    delete[] cmd_tmp_buf;
    return 0;
  }

  int mhwimm_executor::executeCurrentCMD(void)
  {
    if (current_status_ & (mhwimm_executor_status::ERROR  |
                           mhwimm_executor_status::WORKING))
      return -1;

    current_status_ = mhwimm_executor_status::WORKING;
    noutput_msgs_ = 0;

    switch (current_cmd_) {
    case mhwimm_executor_cmd::CD:
      if (cmd_cd_syntaxChecking())
        return cd();
      goto err_syntax;
    case mhwimm_executor_cmd::LS:
      if (cmd_ls_syntaxChecking())
        return ls();
      goto err_syntax;
    case mhwimm_executor_cmd::INSTALL:
      if (cmd_install_syntaxChecking())
        return install();
      goto err_syntax;
    case UNINSTALL:
      if (cmd_uninstall_syntaxChecking())
        return mhwimm_executor_cmd::uninstall();
      goto err_syntax;
    case mhwimm_executor_cmd::INSTALLED:
      if (cmd_uninstall_syntaxChecking())
        return installed();
      goto err_syntax;
    case mhwimm_executor_cmd::CONFIG:
      if (cmd_config_syntaxChecking())
        return config();
      goto err_syntax;
    case mhwimm_executor_cmd::EXIT:
      if (cmd_exit_syntaxChecking())
        return exit();
      goto err_syntax;
    default:
      current_status_ = mhwimm_executor_status::ERROR;
      generic_err_msg_output(ERROR_MSG_UNKNOWNCMD);
      return -1;
    }

  err_syntax:
    generic_err_msg_output(ERROR_MSG_ERRFORM);
    current_status_ = mhwimm_executor_status::ERROR;
    return -1;
  }

  int mhwimm_executor::cd(void)
  {
    current_status_ = mhwimm_executor_status::WORKING;
    if (chdir(parameters_[0].c_str()) < 0) {
      generic_err_msg_output(ERROR_MSG_CHDIR);
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }
    current_status_ = mhwimm_executor_status::IDLE;
    return 0;
  }

  int mhwimm_executor::ls(void)
  {
    current_status_ = mhwimm_executor_status::WORKING;

    // open current work directory
    DIR *this_dir(NULL);
    if (!(this_dir = opendir("."))) {
      generic_err_msg_output(ERROR_MSG_OPENDIR);
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }

    struct dirent *dir(NULL);
    noutput_msgs_ = 0;

    while ((dentry = readdir(DIR))) {

      switch(strlen(dentry->d_name)) {
      case 1:
        if ((*(dentry->d_name)) == '.')
          continue;
      case 2:
        if ((*(dentry->d_name)) == '.' &&
            (*(dentry->d_name + 1)) == '.')
          continue;
      }

      cmd_output_msgs_[noutput_msgs_++] = std::string{dentry->d_name};
    }
    (void)closedir(this_dir);

    current_status_ = mhwimm_executor_status::IDLE;
    is_cmd_has_output_ = true;
    return 0;
  }

  int mhwimm_executor::exit(void)
  {
    // we does not use concurrent protecting at there,
    // because of that the other control path will change this
    // indicator is the signal handler
    program_exit = true;
    current_status_ = mhwimm_executor_status::IDLE;
    return 0;
  }

  int mhwimm_executor::config(void)
  {
#define CONFIG_USERHOME 616
#define CONFIG_MHWIROOT 633
#define CONFIG_MHWIMMCROOT 854

    current_status_ = mhwimm_executor_status::WORKING;

    const auto &key(parameters_[0]);
    const auto &val(parameters_[1]);

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
    default:
      generic_err_msg_output(ERROR_MSG_UNKNOWNCONF);
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }

    current_status_ = mhwimm_executor_status::IDLE;
    return 0;

#undef CONFIG_USERHOME
#undef CONFIG_MHWIROOT
#undef CONFIG_MHWIMMCROOT
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
    mfiles_list_->regular_file_list.clear();
    mfiles_list_->directory_list.clear();


    if (lf_traverse_dir(moddir) < 0) {
      generic_err_msg_output(ERROR_MSG_TRAVERSE_DIR);
      goto err_exit;
    }

    auto mhwiroot(conf_->mhwiroot);

    // link directories
    for (auto i : mfiles_list_->directory_list) {
      if (link(i.c_str(), (mhwiroot + "/" + i).c_str()) < 0) {
        generic_err_msg_output(ERROR_MSG_LINK);
        goto err_exit_unlink_dir;
      }
    }

    // link regular files
    for (auto i : mfiles_list_->regular_file_list) {
      if (link(i.c_str(), (mhwiroot + "/" + i).c_str()) < 0) {
        generic_err_msg_output(ERROR_MSG_LINK);
        goto err_exit_unlink_file;
      }
    }

    current_status_ = mhwimm_executor_status::IDLE;
    return 0;

  err_exit_unlink_file:
    for (auto i : mfiles_list_->regular_file_list)
      unlink((mhwiroot + "/" + i).c_str());

  err_exit_unlink_dir:
    for (auto i : mfiles_list_->directory_list)
      unlink((mhwiroot + "/" + i).c_str());

  err_exit:
    current_status_ = mhwimm_executor_status::ERROR;
    return - 1;
  }

  int mhwimm_executor::uninstall(void)
  {
    // uninstall [ mod name ]
    // but Executor does not ask DB to returns records of
    // mod files.
    // this work will hand over to worker thread.

    if (nparams_ != 1) {
      generic_err_msg_output(ERROR_MSG_INCFORM);
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }
    current_status_ = mhwimm_executor_status::WORKING;

    // acquire list lock
    std::unique_lock<decltype(mfiles_list_->lock)> mfl_lock(&mfiles_list_->lock);

    /**
     * step1 : remove all regular files
     * step2 : remove all directories
     */

    auto mhwiroot(conf_->mhwiroot);

    uint8_t rf_err(0);
    for (auto i : mfiles_list_->regular_file_list) {
      if (unlink((mhwiroot + "/" + i).c_str()) < 0)
        rf_err = 1;
    }

    uint8_t d_err(0);
    for (auto i : mfiles_list_->directory_list) {
      if (unlink((mhwiroot + "/" + i).c_str()) < 0)
        d_err = 1;
    }

    if (rf_err || d_err) {
      generic_err_msg_output(ERROR_MSG_UNLINK);
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }

    current_status_ = mhwimm_executor_status::IDLE;
    return 0;
  }

  int mhwimm_executor::installed(void)
  {
    // because Executor do not interactive with DB,
    // thus worker must ask DB to returns the installed
    // mods list.
    current_status_ = mhwimm_executor_status::WORKING;

    std::unique_lock<decltype(mfiles_list_->lock)> mfl_lock(&mfiles_list_->lock);
    noutput_msgs_ = mfiles_list_->mod_name_list.size();
    if (!noutput_msgs_) {
      cmd_output_msgs_[noutput_infos++] = "No mods been installed.";
    } else {
      uint8_t idx(0);
      for (auto e : mfiles_list_->mod_name_list) {
        cmd_output_msgs_[idx++] = e;
      }
    }

    is_cmd_has_output_ = true;
    current_status_ = mhwimm_executor_status::IDLE;
    return 0;
  }
}
