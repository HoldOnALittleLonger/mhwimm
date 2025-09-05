/**
 * We will let the main thread wait for system signal coming.
 * We only catch the signals SIGINT and SIGTERM.
 * For these two signals,just simply set @program_exit to TRUE
 * as well.
 * Because UI will blocking on read system call,the signal will
 * interrupts it.at the next event cycle,it will detected
 * @program_exit become to TRUE,then it will exit.
 */

#include "mhwimm_sig_actions.h"
#include "mhwimm_sync_mechanism.h"

void SIGINT_handler(int sig)
{
  program_exit = 1;
}

void SIGTERM_handler(int sig)
{
  program_exit = 1;
}

