#ifndef _MHWIMM_SIG_ACTIONS_H_
#define _MHWIMM_SIG_ACTIONS_H_

#include <signal.h>
#include <stddef.h>

void SIGINT_handler(int sig);
void SIGTERM_handler(int sig);

#endif
