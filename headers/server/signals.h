#ifndef PROGETTOSOL_SIGNALS_H
#define PROGETTOSOL_SIGNALS_H

#include "threadpool.h"

typedef struct arg {
	
	int signal_pipe_wfd;

} signal_handler_arg_t;

/**
 * This function is used to mask the signals: SIGINT, SIGQUIT, SIGHUP
 *
 * @param mask the new bits set
 * @param oldmask the old bits set
 *
 * @return 0 success
 * @return errno otherwise
 */
int block_signals(sigset_t *mask, sigset_t *oldmask);

/**
 * This function is performed by the thread handling the signals
 *
 * @param args file descriptor of the pipe write entry
 *
 * @return NULL
 */
void *signal_handler_thread(void *args);

#endif //PROGETTOSOL_SIGNALS_H
