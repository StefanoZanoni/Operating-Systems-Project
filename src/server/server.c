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
#include "../../includes/log.h"
#include "../../includes/configuration.h"
#include "../../includes/cache.h"

static server_configuration_t configuration;
static cleanup_structure_t cstruct;

static queue_t queue;
static hashtable_t hashtable;

static FILE *log = NULL;

void server_cleanup() {

    if (configuration.sockname != NULL) {
        free(configuration.sockname);
    }

    if (configuration.logpath != NULL) {
        free(configuration.logpath);
    }

    if (log)
        fclose(log);

    if ( !is_queue_empty(&queue) )
        free_queue(&queue);
    
    free_hashtable(&hashtable);
}

int main(int argc, char **argv) {

    if (argc == 1) {
        fprintf(stderr, "Usage: %s config file absolute pathname\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    clock_t begin = clock();

    memset(&configuration, 0 , sizeof(configuration));
    memset(&cstruct, 0, sizeof(cstruct));

    //I mask the signals: SIGINT, SIGQUIT, SIGTERM, SIGHUP, until I have read the configuration file
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);

    CHECK_NEQ_EXIT(server_cleanup(), "pthread_sigmask server.c", pthread_sigmask(SIG_BLOCK, &mask, &oldmask), 0, "pthread_sigmask\n", "");

    get_config(argv[1], &configuration);
    //print_config(configuration);

    log = start_log(configuration.logpath);
    //I check the value of log here to clean either sockname and loghpath in case of error
    CHECK_EQ_EXIT(server_cleanup(), "start_log server.c", log, NULL, "log\n", "");

    char *fmt = "%s\nnum_workers = %d\nnum_files = %d\nstorage_capacity = %d\nsockname = %s\nlogpath = %s\n";
    write_log(log, fmt, "--------------------------------SERVER CONFIGURATION--------------------------------",
            configuration.num_workers, configuration.num_files, configuration.storage_capacity,
            configuration.sockname, configuration.logpath);

    if( cache_initialization(configuration, &hashtable, &queue) == -1 ) {
        server_cleanup();
        fprintf(stderr, "Error on cache initialization\n");
        exit(EXIT_FAILURE);
    }

    //I restore initial acceptance of signals
    CHECK_NEQ_EXIT(server_cleanup(), "pthread_sigmask server.c", pthread_sigmask(SIG_SETMASK, &oldmask, NULL), 0, "pthread_sigmask\n", "");

    server_cleanup();

    clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("%lf\n", time_spent);

    return 0;
}
