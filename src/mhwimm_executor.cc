#include "mhwimm_executor.h"

#include <cstring>
#include <cstdbool>
#include <cstdint>
#include <exception>
#include <functional>

#ifdef DEBUG
// for debug
#include <iostream>
#endif

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>

#include <string.h>

namespace mhwimm_executor_ns {

#define ERROR_MSG_MEM "error: Failed to allocate memory."
#define ERROR_MSG_CHDIR "error: Failed enter the directory."
#define ERROR_MSG_OPENDIR "error: Failed to open directory."
#define ERROR_MSG_MKDIR "error: Failed to make a directory."
#define ERROR_MSG_STATPATH "error: Can not retrieve the stat of path."
#define ERROR_MSG_ERRFORM "error: Incorrect format."
#define ERROR_MSG_UNKNOWNCMD "error: Unknown cmd."
#define ERROR_MSG_UNKNOWNCONF "error: Unknown conf."
#define ERROR_MSG_TRAVERSE_DIR "error: Failed to traverse directory."
#define ERROR_MSG_LINK "error: Failed to install mod."
#define ERROR_MSG_UNINSTALL "error: Failed to uninstall mod."
#define ERROR_MSG_NOMODINS "error: No mod been installed."

  static int32_t calculate_key(const char *cmd_str)
  {
    if (!cmd_str)
      return 0;
    int32_t result(0);
    while (*cmd_str) {
      result += *cmd_str++;
    }
    return result;
  }

  int mhwimm_executor::parseCMD(const std::string &cmd_string) noexcept
  {

#define CD_CMD_KEY 199
#define LS_CMD_KEY 223
#define INSTALL_CMD_KEY 759
#define UNINSTALL_CMD_KEY 986
#define INSTALLED_CMD_KEY 960
#define CONFIG_CMD_KEY 630
#define EXIT_CMD_KEY 442
#define COMMANDS_CMD_KEY 850
#define HELP_CMD_KEY 425

    current_status_ = mhwimm_executor_status::WORKING;
    
    char *cmd_tmp_buf(nullptr);
    std::size_t buf_length(cmd_string.length() + 1);

    try {
      cmd_tmp_buf = new char[buf_length];
    } catch (std::bad_alloc &) {
      generic_err_msg_output(ERROR_MSG_MEM);
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }

    memset(cmd_tmp_buf, 0, buf_length);
    memcpy(cmd_tmp_buf, cmd_string.c_str(), buf_length - 1);

    const char *arg(strtok(cmd_tmp_buf, " "));

    bool parse_more(false);
    nparams_ = 0; // reset number of parameters

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
    case COMMANDS_CMD_KEY:
    case HELP_CMD_KEY:
      setCMD(mhwimm_executor_cmd::COMMANDS);
      break;
    default:
      setCMD(mhwimm_executor_cmd::NOP);
    }

    if (parse_more) {
      while ((arg = strtok(NULL, " "))) {
        parameters_[nparams_++] = arg;
      }
    }

#undef CD_CMD_KEY
#undef LS_CMD_KEY
#undef INSTALL_CMD_KEY
#undef UNINSTALL_CMD_KEY
#undef INSTALLED_CMD_KEY
#undef CONFIG_CMD_KEY
#undef EXIT_CMD_KEY
#undef COMMANDS_CMD_KEY
#undef HELP_CMD_KEY

    current_status_ = mhwimm_executor_status::IDLE;
    delete[] cmd_tmp_buf;
    return 0;
  }

  int mhwimm_executor::executeCurrentCMD(void) noexcept
  {
    if (current_status_ == mhwimm_executor_status::ERROR ||
        current_status_ == mhwimm_executor_status::WORKING)
      return -1;

#ifdef DEBUG
    std::cerr << "noutput_msgs_ = " << noutput_msgs_ << std::endl;
    std::cerr << "nparams_ = " << nparams_ << std::endl;
#endif

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
    case mhwimm_executor_cmd::UNINSTALL:
      if (cmd_uninstall_syntaxChecking())
        return uninstall();
      goto err_syntax;
    case mhwimm_executor_cmd::INSTALLED:
      if (cmd_installed_syntaxChecking())
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
    case mhwimm_executor_cmd::COMMANDS:
      if (cmd_commands_syntaxChecking())
        return commands();
      goto err_syntax;
    case mhwimm_executor_cmd::NOP:
      current_status_ = mhwimm_executor_status::IDLE;
      return 0;
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

  int mhwimm_executor::cd(void) noexcept
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

  int mhwimm_executor::ls(void) noexcept
  {
    current_status_ = mhwimm_executor_status::WORKING;

    // open current work directory
    DIR *this_dir(NULL);
    if (!(this_dir = opendir("."))) {
      generic_err_msg_output(ERROR_MSG_OPENDIR);
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }

    struct dirent *dentry(NULL);
    noutput_msgs_ = 0;

#ifdef DEBUG
    int ndentries(0);
#endif

    while ((dentry = readdir(this_dir))) {

#ifdef DEBUG
      ++ndentries;
#endif

      rs_vec_if_necessary(cmd_output_msgs_, noutput_msgs_);
      cmd_output_msgs_[noutput_msgs_++] = std::string{dentry->d_name};
    }
    (void)closedir(this_dir);

#ifdef DEBUG
    std::cerr << "number of dentries : " << ndentries << std::endl;
#endif

    current_status_ = mhwimm_executor_status::IDLE;
    is_cmd_has_output_ = true;
    return 0;
  }

  int mhwimm_executor::exit(void) noexcept
  {
    // we does not use concurrent protecting at there,
    // because of that the other control path will change this
    // indicator is the signal handler
    current_status_ = mhwimm_executor_status::WORKING;
    kill(getpid(), SIGINT); // trigger SIGINT handler
    current_status_ = mhwimm_executor_status::IDLE;
    return 0;
  }

  int mhwimm_executor::config(void) noexcept
  {
#define CONFIG_USERHOME 616
#define CONFIG_MHWIROOT 633
#define CONFIG_MHWIMMCROOT 854

    current_status_ = mhwimm_executor_status::WORKING;

    const auto &key(parameters_[0]);
    const auto &val(parameters_[1]);

    switch (calculate_key(key.c_str())) {
    case CONFIG_USERHOME:
      conf_->userhome = static_cast<typename
                                    mhwimm_config_ns::get_config_traits<
                                      mhwimm_config_ns::config_t>::skey_t>(val);
      break;
    case CONFIG_MHWIROOT:
      conf_->mhwiroot = static_cast<typename
                                    mhwimm_config_ns::get_config_traits<
                                      mhwimm_config_ns::config_t>::skey_t>(val);
      break;
    case CONFIG_MHWIMMCROOT:
      conf_->mhwimmroot = static_cast<typename
                                       mhwimm_config_ns::get_config_traits<
                                         mhwimm_config_ns::config_t>::skey_t>(val);
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

  int mhwimm_executor::install(void) noexcept
  {
    current_status_ = mhwimm_executor_status::WORKING;

    std::string modname(parameters_[0]);
    std::string moddir(parameters_[1]);

    /**
     * lf_traverse_dir - local function used to traverse the directory and makeup mod file list
     * @parent_dir:      parent directory
     * return:           0 OR -1
     * # front-inserting
     */
    std::function<int(const std::string &, const std::string &)> lf_traverse_dir =
      [&, this](const std::string &moddir, const std::string &subpath) -> int {
      std::string opendir_path(moddir + "/" + subpath);

      DIR *this_dir = opendir(opendir_path.c_str());
      if (!this_dir) {
        generic_err_msg_output(ERROR_MSG_OPENDIR);
        return -1;
      }

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
        
        /**
         * append dentry->d_name to @parent_dir.
         */

        std::string dentry_path(opendir_path + "/" + dentry->d_name);
        std::string dentry_subpath(subpath + "/" + dentry->d_name);
        struct stat dentry_stat = {0};

        if (stat(dentry_path.c_str(), &dentry_stat) < 0) {
          ret = -1;
          break;
        } else if (dentry_stat.st_mode & S_IFDIR) {
          mfiles_list_->directory_list.insert(mfiles_list_->directory_list.end(), dentry_subpath);
          ret = lf_traverse_dir(moddir, dentry_subpath);
          if (ret) // returned -1
            return ret;
        } else if (dentry_stat.st_mode & S_IFREG) {
          mfiles_list_->regular_file_list.insert(mfiles_list_->regular_file_list.end(),
                                                 dentry_subpath);
        } else if (dentry_stat.st_mode & S_IFLNK) // skip symlink
          continue;
      }

      (void)closedir(this_dir);
      return ret;
    };

    // now we have to traverse the mod directory to makeup file list
    std::unique_lock<decltype(mfiles_list_->lock)> mfl_lock(mfiles_list_->lock);
    mfiles_list_->regular_file_list.clear();
    mfiles_list_->directory_list.clear();

    if (lf_traverse_dir(moddir, std::string{"\0"}) < 0) {
      generic_err_msg_output(ERROR_MSG_TRAVERSE_DIR);
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }

    auto mhwiroot(conf_->mhwiroot);

#ifdef DEBUG
    std::cerr << "Debug : " << std::endl;
    for (auto i : mfiles_list_->directory_list)
      std::cerr << " dentry: " << i << std::endl;
    for (auto i : mfiles_list_->regular_file_list)
      std::cerr << " dentry: " << i << std::endl;
#endif

    char path_tmp[256] = {0};
    std::string cwd(getcwd(path_tmp, 256));
    struct stat mhwiroot_stat = {0};

    if (stat(mhwiroot.c_str(), &mhwiroot_stat) < 0) {
      generic_err_msg_output(ERROR_MSG_STATPATH);
      goto err_exit;
    }

    // mkdirs
    for (auto i : mfiles_list_->directory_list) {
      std::string newpath(mhwiroot + i);

#ifdef DEBUG
      errno = 0;
#endif
      if (mkdir(newpath.c_str(), mhwiroot_stat.st_mode) < 0) {
#ifdef DEBUG
        if (errno != EEXIST)
          std::cerr << strerror(errno) << std::endl;
#endif
        generic_err_msg_output(ERROR_MSG_MKDIR);
        goto err_exit_remove_dir;
      }
    }

    // link regular files
    for (auto i : mfiles_list_->regular_file_list) {
      std::string oldpath(cwd + "/" + moddir + i);
      std::string newpath(mhwiroot + i);

#ifdef DEBUG
      errno = 0;
#endif
      if (link(oldpath.c_str(), newpath.c_str()) < 0) {
#ifdef DEBUG
        std::cerr << strerror(errno) << std::endl;
#endif
        generic_err_msg_output(ERROR_MSG_LINK);
        goto err_exit_unlink_file;
      }
    }

    current_status_ = mhwimm_executor_status::IDLE;
    return 0;

  err_exit_unlink_file:
    for (auto i : mfiles_list_->regular_file_list)
      unlink((mhwiroot + i).c_str());

  err_exit_remove_dir:
    // we need to reverse elements that is because we
    // insert them at end of container,and the last
    // element must bethe last directory,because we
    // traverse directory followed the rule -
    // always start a new recursion for traverse the
    // new directory whenever we encountered it.
    mfiles_list_->directory_list.reverse();
    for (auto i : mfiles_list_->directory_list)
      rmdir((mhwiroot + i).c_str());

  err_exit:
    current_status_ = mhwimm_executor_status::ERROR;
    return - 1;
  }

  int mhwimm_executor::uninstall(void) noexcept
  {
    // uninstall [ mod name ]
    // but Executor does not ask DB to returns records of
    // mod files.
    // this work will hand over to worker thread.
    current_status_ = mhwimm_executor_status::WORKING;

    // acquire list lock
    std::unique_lock<decltype(mfiles_list_->lock)> mfl_lock(mfiles_list_->lock);

    /**
     * step1 : remove all regular files
     * step2 : remove all directories
     * the records returned by DB would follow this format :
     *   /aaa/bbb/ccc
     */

    auto mhwiroot(conf_->mhwiroot);
    noutput_msgs_++;

    uint8_t rf_err(0);
    for (auto i : mfiles_list_->regular_file_list) {
      std::string filepath(mhwiroot + i);
      if (unlink(filepath.c_str()) < 0) {
        rf_err = 1;
        rs_vec_if_necessary(cmd_output_msgs_, noutput_msgs_);
        // record the file's path which unlink failed on
        cmd_output_msgs_[noutput_msgs_++] = filepath;
      }
    }

    uint8_t d_err(0);
    for (auto i : mfiles_list_->directory_list) {
      std::string dirpath(mhwiroot + i);
      if (rmdir(dirpath.c_str()) < 0) {
        d_err = 1;
        rs_vec_if_necessary(cmd_output_msgs_, noutput_msgs_);
        cmd_output_msgs_[noutput_msgs_++] = dirpath;
      }
    }

    if (rf_err || d_err) {
      std::size_t nmsgs(noutput_msgs_);
      generic_err_msg_output(ERROR_MSG_UNINSTALL);
      noutput_msgs_ = nmsgs;
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }

    noutput_msgs_ = 0;
    current_status_ = mhwimm_executor_status::IDLE;
    return 0;
  }

  int mhwimm_executor::installed(void) noexcept
  {
    // because Executor do not interactive with DB,
    // thus worker must ask DB to returns the installed
    // mods list.
    current_status_ = mhwimm_executor_status::WORKING;

    std::unique_lock<decltype(mfiles_list_->lock)> mfl_lock(mfiles_list_->lock);
    std::size_t nes = mfiles_list_->mod_name_list.size();
    if (!nes) {
      generic_err_msg_output(ERROR_MSG_NOMODINS);
      current_status_ = mhwimm_executor_status::ERROR;
      return -1;
    }

#ifdef DEBUG
    std::cerr << "Command - installed - number of elements in list: "
              << nes
              << std::endl;
#endif

    for (auto e : mfiles_list_->mod_name_list) {
      rs_vec_if_necessary(cmd_output_msgs_, noutput_msgs_);
      cmd_output_msgs_[noutput_msgs_++] = e;
    }

    is_cmd_has_output_ = true;
    current_status_ = mhwimm_executor_status::IDLE;
    return 0;
  }

  int mhwimm_executor::commands(void) noexcept
  {
    constexpr const char *cd_description = "cd <path> - change current work directory";
    constexpr const char *ls_description = "ls - list files under current work direcotry";
    constexpr const char *install_description = "install <mod name> <mod directory> - install mod @mode_name,its files are existed in @mod_direcotry";
    constexpr const char *unintall_description = "unintall <mod name> - unintall mod @mod_name";
    constexpr const char *config_description = "config <key>=<value> - set config,implemented @userhome @mhwiroot, @mhwimmroot";
    constexpr const char *exit_description = "exit - exit application";
    constexpr const char *commands_description = "commands - list commands and print description";
    constexpr const char *help_description = "help - help message,implemented as cmd commands";

    constexpr uint8_t ndescriptions = 8;

    constexpr const char *descriptions[ndescriptions] = {
      cd_description, ls_description, install_description, unintall_description,
      config_description, exit_description, commands_description, help_description
    };

    current_status_ = mhwimm_executor_status::WORKING;

    for (uint8_t i(0); i < ndescriptions; ++i) {
      cmd_output_msgs_[i] = descriptions[i];
    }

    is_cmd_has_output_ = true;
    noutput_msgs_ = ndescriptions;
    current_status_ = mhwimm_executor_status::IDLE;
    return 0;
  }
}
