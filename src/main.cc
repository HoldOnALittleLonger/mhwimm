#include "mhwimm_ui.h"
#include "mhwimm_executor.h"
#include "mhwimm_database.h"

#include "mhwimm_ui_thread.h"
#include "mhwimm_executor_thread.h"
#include "mhwimm_database_thread.h"

#include "mhwimm_sync_mechanism.h"
#include "mhwimm_sig_actions.h"
#include "mhwimm_config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <cstdbool>
#include <cstring>
#include <mutex>
#include <thread>
#include <string>
#include <memory>
#include <iostream>

constexpr char mhwimm_config_filename("mhwimm_config");
constexpr char mhwimmroot_name("mhwimm");
constexpr char mhwimm_db_name("mhwimm_db");

mhwimm_sync_mechanism_ns::uiexemsgexchg uiexe_ctrl_msg = {
  .status = mhwimm_sync_mechanism_ns::UIEXE_STATUS::EXE_NOMSG,
};

mhwimm_sync_mechanism_ns::mod_files_list mfl;

atomic_t program_exit = 0;

std::mutex exedb_sync_mutex;

static bool getenv_HOME(char *buf, std::size_t len);

static bool mhwimm_need_initialization(const std::string &config_path)

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
      std::cerr << "Failed to setup mhwimm." << std::endl;
      std::cerr << "config path: " << config_file_path << std::endl
                << "userhome: " << conf.userhome << std::endl
                << "mhwiroot: " << conf.mhwiroot << std::endl
                << "mhwimmroot: " << conf.mhwimmroot << std::endl;
      return -1;
    }
    
    struct stat ts = {0};
    if (stat(conf.userhome.c_str(), &ts) < 0) {
      std::cerr << "Failed to retrieve file stat of $HOME." << std::endl;
      return -1;
    }

    errno = 0;
    /* create directory mhwimmroot */
    if (mkdir(conf.mhwimmroot.c_str(), ts.st_mode) < 0) {
      /* ignore error when directory been existed. */
      if (errno != EEXIST) {
        std::cerr << "Failed to create directory " << conf.mhwimmroot << std::endl;
        return -1;
      }
    }

    /* create config file */
    if (!mhwimm_config_ns::makeup_config_file<config_t>(&conf,
                                                        config_file_path.c_str())) {
      std::cerr << "Failed to makeup config file." << std::endl;
      return -1;
    }
  }
  else if (!read_from_config<config_t>(&conf, config_file_path.c_str())) {
    std::cerr << "Failed to read from config file." << std::endl;
    return -1;
  }

  std::string db_path = conf.mhwimmroot + "/" + mhwimm_db_name;

  /* install signal handlers */
  struct sigaction siga = {
    .sa_handler = SIGINT_handler,
    .sa_flags = 0
  };

  sigemptyset(&siga.sa_mask);
  sigaddset(&siga.sa_mask, SIGTERM);

  if (sigaction(SIGINT, &siga, NULL) < 0) {
    std::cerr << "Failed to setup SIGINT handler." << std::endl;
    return -1;
  }

  siga.sa_handler = SIGTERM_handler;
  sigemptyset(&siga.sa_mask);
  sigaddset(&siga.sa_mask, SIGINT);

  if (sigaction(SIGTERM, &siga, NULL) < 0) {
    std::cerr << "Failed to setup SIGTERM handler." << std::endl;
  }

  sigemptyset(&siga.sa_mask);
  sigaddset(&siga.sa_mask, SIGINT);
  sigaddset(&siga.sa_mask, SIGTERM);

  if (sigprocmask(SIG_BLOCK, &siga.sa_mask, NULL) < 0) {
    std::cerr << "Failed to setup thread signal mask." <<std::endl;
    return false;
  }

  /* prepare threads */
  mhwimm_ui_ns::mhwimm_ui ui;
  mhwimm_executor_ns::mhwimm_executor exe(&conf);
  mhwimm_db_ns::mhwimm_db db(mhwimm_db_name, db_path.c_str());
  init_regDB_routines(&db);

  std::thread ui_thread(mhwimm_ui_thread_worker, ui, uiexe_ctrl_msg);
  std::thread exe_thread(mhwimm_executor_thread_worker, exe,uiexe_ctrl_msg, mfl);
  std::thread db_thread(mhwimm_database_thread_worker, db);

  int sig = 0;

 repeat_sigwait:

  if (sigwait(&siga.sa_mask, &sig) < 0) {
    program_exit = 1;
    std::cerr << "Error encountered when main thread doing signal wait." << std::endl;
  }

  if (sig != SIGINT || sig != SIGTERM)
    goto repeat_sigwait;

  db_thread.join();
  exe_thread.join();
  ui_thread.join();

  mhwimm_config_ns::makeup_config_file<config_t>(&conf, config_file_path.c_str());

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
    std::cerr << "Failed to allocate memory for get mhwiroot." << std::endl;
    return false;
  }

  std::cout << "full path of Monster Hunter World:Iceborne : ";
  std::count.flush();
  std::cin.getline(buf, path_max);
  if (std::cin.fail()) {
    std::cerr << "error detected when reading user input." << std::endl;
    return false;
  }

  struct stat ts = {0};
  if (stat(buf.get(), &ts) < 0) {
    std::cerr << "Can not verify the path." << std::endl;
    return false;
  }

  if (!S_ISDIR(ts.st_mode)) {
    std::cerr << "Bad path." << std::endl;
    return false;
  }

  conf.mhwiroot = buf.get();
  return true;
}
