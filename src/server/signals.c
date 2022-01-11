#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#include "../../headers/server/signals.h"
#include "../../headers/server/log.h"

int block_signals(sigset_t *mask, sigset_t *oldmask) {

	sigemptyset(mask);

	sigaddset(mask, SIGINT);
    sigaddset(mask, SIGQUIT);
    sigaddset(mask, SIGHUP);

    int retval;

    retval = pthread_sigmask(SIG_BLOCK, mask, oldmask);
    
    return retval;
}

void *signal_handler_thread(void *args) {

	int fd = ((signal_handler_arg_t*) args)->signal_pipe_wfd;
	int end_server = 0;
	ssize_t retval;

	sigset_t set;
	siginfo_t info;

	sigemptyset(&set);

	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGHUP);

	while (1) {

		sigwaitinfo(&set, &info);

		if (info.si_signo == SIGINT || info.si_signo == SIGQUIT) {

			if (info.si_signo == SIGINT) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "Received SIGINT signal");
			}
			else {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "Received SIGQUIT signal");
			}

			end_server = 2;
            retval = write(fd, &end_server, sizeof(int));
			if (retval == -1) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "Signal handler: Error in writing the pipe");
			}

			return NULL;

		}
		else if (info.si_signo == SIGHUP) {

			const char *fmt = "%s\n";
			write_log(my_log, fmt, "Received SIGHUP signal");

			end_server = 1;
            retval = write (fd, &end_server, sizeof(int));
			if (retval == -1) {
				fmt = "%s\n";
				write_log(my_log, fmt, "Signal handler: Error in writing the pipe");
			}
						
			return NULL;

		}

	}
}
