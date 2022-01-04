#ifndef PROGETTOSOL_SIGNALS_H
#define PROGETTOSOL_SIGNALS_H

#include "threadpool.h"

typedef struct arg {
	
	int signal_pipe_wfd;

} signal_handler_arg_t;

int block_signals(sigset_t *mask, sigset_t *oldmask);
void *signal_handler_thread(void *args);

#endif //PROGETTOSOL_SIGNALS_H