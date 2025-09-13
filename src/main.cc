/**
 * Main
 * The jobs that main() have to do :
 *   1> initializes some resources will used by
 *      UI,Executor,Database
 *   2> Get environment variable HOME to locate
 *      current user's home
 *   3> Checks whether need to initialize this
 *      application.if have to,then ask user
 *      for some paths about this application
 *      and mhwi;if have not to,then attempt
 *      read config from config file
 *   4> Setup signal blocking.
 *   5> intialize global variable @pmhwiroot
 *      which will be used by Database
 *   6> initialize Database Register Helpers
 *   7> start thread works
 *   8> sigwait() async signals come
 *   9> send signal to threads for interrupt
 *      pending system call,and wait them
 *      join to main thread
 *  10> write config options to config file
 *      and exit
 */
#include "mhwimm_ui.h"
#include "mhwimm_executor.h"
#include "mhwimm_database.h"

#include "mhwimm_ui_thread.h"
#include "mhwimm_executor_thread.h"
#include "mhwimm_database_thread.h"

#include "mhwimm_sync_mechanism.h"
#include "mhwimm_config.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <cstdbool>
#include <cstring>
#include <mutex>
#include <thread>
#include <string>
#include <memory>
#include <functional>
#include <iostream>

constexpr const char *mhwimm_config_filename("mhwimm_config");
constexpr const char *mhwimmroot_name("mhwimm");
constexpr const char *mhwimm_db_name("mhwimm_db");

atomic_t program_exit = 0;

mhwimm_sync_mechanism_ns::conditionv exedb_condv_sync;
mhwimm_sync_mechanism_ns::uiexemsgexchg uiexe_ctrl_msg = {
  .status = mhwimm_sync_mechanism_ns::UIEXE_STATUS::EXE_NOMSG,
};
mhwimm_sync_mechanism_ns::mod_files_list mfl;

const typename mhwimm_config_ns::get_config_traits<mhwimm_config_ns::config_t>::skey_t *
pmhwiroot_path(nullptr);

static bool getenv_HOME(char *buf, std::size_t len);

static bool mhwimm_need_initialization(const std::string &config_path);

static bool ask_user_to_setup_mhwimmroot(mhwimm_config_ns::config_t &conf);

int main(void)
{
  std::cout << "Initializing..." << std::endl;

  mhwimm_config_ns::config_t conf = {
    .userhome = "nil",
    .mhwiroot = "nil",
    .mhwimmroot = "nil",
  };

  char *penv_buf(nullptr);
  std::size_t path_max(pathconf("/", _PC_PATH_MAX));

  try {
    penv_buf = new char[path_max];
  } catch (std::bad_alloc &) {
    std::cerr << "Failed to allocate memory for get environment variable."
              << std::endl;
    return -1;
  }
  std::unique_ptr<char> auto_deallocate(penv_buf);

  if (!getenv_HOME(penv_buf, path_max)) {
    std::cerr << "Failed to get HOME environment variable."
              << std::endl;
    return -1;
  }

  conf.userhome = penv_buf;
  conf.mhwimmroot = conf.userhome + "/" + mhwimmroot_name;
  std::string config_file_path(conf.mhwimmroot + "/" + mhwimm_config_filename);

  if (mhwimm_need_initialization(config_file_path)) {
    if (!ask_user_to_setup_mhwimmroot(conf)) {
      std::cerr << "main(): error: Failed to setup mhwimm." << std::endl;
      std::cerr << "config path: " << config_file_path << std::endl
                << "userhome: " << conf.userhome << std::endl
                << "mhwiroot: " << conf.mhwiroot << std::endl
                << "mhwimmroot: " << conf.mhwimmroot << std::endl;
      return -1;
    }
    
    struct stat ts = {0};
    if (stat(conf.userhome.c_str(), &ts) < 0) {
      std::cerr << "main(): error: Failed to retrieve file stat of $HOME." << std::endl;
      return -1;
    }

    errno = 0;
    /* create directory mhwimmroot */
    if (mkdir(conf.mhwimmroot.c_str(), ts.st_mode) < 0) {
      /* ignore error when directory been existed. */
      if (errno != EEXIST) {
        std::cerr << "main(): error: Failed to create directory " << conf.mhwimmroot << std::endl;
        return -1;
      }
    }

    /* create config file */
    if (!mhwimm_config_ns::makeup_config_file<mhwimm_config_ns::config_t>(&conf,
                                                        config_file_path.c_str())) {
      std::cerr << "main(): error: Failed to makeup config file." << std::endl;
      return -1;
    }
  }
  else if (!read_from_config<mhwimm_config_ns::config_t>(&conf, config_file_path.c_str())) {
    std::cerr << "main(): error: Failed to read from config file." << std::endl;
    return -1;
  }

  // setup signal mask
  sigset_t new_mask;
  sigemptyset(&new_mask);
  sigaddset(&new_mask, SIGINT);
  sigaddset(&new_mask, SIGTERM);
  if (sigprocmask(SIG_BLOCK,&new_mask, NULL) < 0) {
    std::cerr << "main(): Attempts to block SIGINT and SIGTERM was failed." << std::endl;
    return -1;
  }

  std::string db_path = conf.mhwimmroot;
  pmhwiroot_path = &conf.mhwiroot;
  // print the paths
  std::cout << "userhome: " << conf.userhome
            << "\nmhwiroot: " << conf.mhwiroot
            << "\nmhwimmroot: " << conf.mhwimmroot
            << std::endl;

  /* prepare threads */
  mhwimm_ui_ns::mhwimm_ui ui;
  mhwimm_executor_ns::mhwimm_executor exe(&conf);
  mhwimm_db_ns::mhwimm_db db(mhwimm_db_name, db_path.c_str());
  init_regDB_routines(&db);

  std::thread ui_thread(mhwimm_ui_thread_worker, std::ref(ui), std::ref(uiexe_ctrl_msg));
  std::thread exe_thread(mhwimm_executor_thread_worker, std::ref(exe), std::ref(uiexe_ctrl_msg),
                         std::ref(mfl));
  std::thread db_thread(mhwimm_db_thread_worker, std::ref(db));

  db_thread.join();
  exe_thread.join();
  ui_thread.join();

  std::cout << "main(): Ready to makeup config file." << std::endl;
  mhwimm_config_ns::makeup_config_file<mhwimm_config_ns::config_t>(&conf, config_file_path.c_str());
  std::cout << "main(): Completed." << std::endl;

  return 0;
}

static bool getenv_HOME(char *buf, std::size_t len)
{
  if (!buf)
    return false;

  char *p(getenv("HOME"));
  std::size_t lp(strlen(p));

  if (lp > len)
    return false;

  strncpy(buf, p, len);
  return true;
}

static inline bool mhwimm_need_initialization(const std::string &config_path)
{
  struct stat ts = {0};
  bool need(false);
  if (stat(config_path.c_str(), &ts) < 0)
    need = true;
  else if (!S_ISREG(ts.st_mode))
    need = true;

  return need;
}

static bool ask_user_to_setup_mhwimmroot(mhwimm_config_ns::config_t &conf)
{
  std::size_t path_max = pathconf("/", _PC_PATH_MAX);
  std::unique_ptr<char> buf(new char[path_max]);
  if (!buf) {
    std::cerr << "main(): error: Failed to allocate memory for get mhwiroot." << std::endl;
    return false;
  }

  std::cout << "full path of Monster Hunter World:Iceborne : ";
  std::cout.flush();
  std::cin.getline(buf.get(), path_max);
  if (std::cin.fail()) {
    std::cerr << "main(): error: error detected when reading user input." << std::endl;
    return false;
  }

  struct stat ts = {0};
  if (stat(buf.get(), &ts) < 0) {
    std::cerr << "main(): error: Can not verify the path." << std::endl;
    return false;
  }

  if (!S_ISDIR(ts.st_mode)) {
    std::cerr << "main(): error: Bad path." << std::endl;
    return false;
  }

  conf.mhwiroot = buf.get();
  return true;
}
