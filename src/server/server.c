#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../../includes/util.h"
#include "../../includes/configuration.h"

static server_configuration_t configuration;
static cleanup_structure_t cstruct;

void server_cleanup() {


}

int main(int argc, char **argv) {

    if (argc == 1) {
        fprintf(stderr, "Usage: %s config file absolute pathname\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //I mask the signals: SIGINT, SIGQUIT, SIGTERM, SIGHUP, until I have read the configuration file
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);

    CHECK_NEQ_EXIT(server_cleanup(), "pthread_sigmask", pthread_sigmask(SIG_BLOCK, &mask, &oldmask), 0, "pthread_sigmask", "");

    get_config(argv[1], &configuration);
    print_config(configuration);

    //I restore initial acceptance of signals
    CHECK_NEQ_EXIT(server_cleanup(), "pthread_sigmask", pthread_sigmask(SIG_SETMASK, &oldmask, NULL), 0, "pthread_sigmask", "");

    return 0;
}