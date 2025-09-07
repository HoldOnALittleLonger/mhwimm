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
#include <signal.h>
#include <unistd.h>

#include <cstdbool>
#include <cstring>
#include <mutex>
#include <thread>
#include <string>

constexpr char mhwimm_config_filename("mhwimm_config");

mhwimm_sync_mechanism_ns::uiexemsgexchg uiexe_ctrl_msg = {
  .status = mhwimm_sync_mechanism_ns::UIEXE_STATUS::EXE_NOMSG,
};

mhwimm_sync_mechanism_ns::mod_files_list mfl;

atomic_t program_exit = 0;

std::mutex exedb_sync_mutex;

static bool getenv_HOME(char *buf, std::size_t len);

static int parse_cmd_params(int argc, char *argv[],
                            mhwimm_config_ns::the_default_config_type &conf);

static void parse_config(std::string_view path_of_config,
                         mhwimm_config_ns::the_default_config_type &conf);

int main(int argc, char *argv[])
{
  std::cout << "Initializing..." << std::endl;

  mhwimm_config_ns::the_default_config_type conf;
  if (parse_cmd_params(argc, argv, conf) < 0) {
    std::cerr << "Failed to parse cmd parameters." << std::enl;
    return -1;
  };

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




                                  

  mhwimm_ui_ns::mhwimm_ui ui;
  mhwimm_executor_ns::mhwimm_executor exe;
  mhwimm_db_ns::mhwimm_db db;

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

