#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../../common/util.h"
#include "../../common/conn.h"
#include "../../common/client_server.h"
#include "../../headers/server/log.h"
#include "../../headers/server/configuration.h"
#include "../../headers/server/cache.h"
#include "../../headers/server/signals.h"
#include "../../headers/server/requests.h"
#include "../../headers/server/connections.h"

#define CONFIG_PATH "config/config.txt"

struct cache_queue queue;
struct cache_hash_table cache_table;
struct conn_hash_table conn_table;
struct references_list *flist = NULL;
pthread_mutex_t flist_mutex = PTHREAD_MUTEX_INITIALIZER;

FILE *my_log = NULL;

static server_configuration_t configuration;

static int sockfd = -1;

static struct tpool *tp = NULL;

static int signal_pipe[2];
static int workers_pipe[2];

static pthread_t *threads;
static size_t thread_num = 0;

static unsigned long int max_conn = 0;
static unsigned long int curr_conn = 0;

void server_cleanup() {

    if (sockfd > 0)
        close(sockfd);
    if (signal_pipe[0] > 0)
        close(signal_pipe[0]);
    if (signal_pipe[1] > 0)
        close(signal_pipe[1]);
    if (workers_pipe[0] > 0)
        close(workers_pipe[0]);
    if (workers_pipe[1] > 0)
        close(workers_pipe[1]);

    unlink(configuration.sockname);

    if (configuration.sockname) 
        free(configuration.sockname);

    if (configuration.logpath)
        free(configuration.logpath);

    print_cache_data(cache_table);
    
    cache_cleanup(&cache_table, &queue);
    conn_hash_table_cleanup(&conn_table);
    files_list_cleanup(&flist, &flist_mutex);

    if (tp)
        if (tpool_destroy(tp, 1, threads, thread_num) == -1) {
            const char *fmt = "%s\n";
            write_log(my_log, fmt, "Fatal error while shutting down the threadpool");
        }
    free(threads);

    const char *fmt = "Maximum number of simultaneous connections: %ld\n";
    write_log(my_log, fmt, max_conn);

    if (my_log) {
        fmt = "%s\n\n\n";
        write_log(my_log, fmt, "Server cleanup finished successfully");
        fclose(my_log);
    }

}

int main(int argc, char **argv) {

    int retval;
    signal_pipe[0] = -1;
    signal_pipe[1] = -1;
    workers_pipe[0] = -1;
    workers_pipe[1] = -1;

    memset(&configuration, 0, sizeof(configuration));
    memset(&queue, 0, sizeof(struct cache_queue));
    memset(&cache_table, 0, sizeof(struct cache_hash_table));
    memset(&conn_table, 0, sizeof(struct conn_hash_table));

    // I mask the signals: SIGINT, SIGQUIT, SIGHUP
    sigset_t mask, oldmask;
    retval = block_signals(&mask, &oldmask);
    CHECK_NEQ_EXIT(server_cleanup(), "block_signals server.c", retval, 0, "pthread_sigmask\n", "")

    get_config(CONFIG_PATH, &configuration);

    my_log = start_log(configuration.logpath);

    // I check the value of my_log here to clean either sockname and logpath in case of error
    CHECK_EQ_EXIT(server_cleanup(), "start_my_log my_log.c", my_log, NULL, "fopen\n", "")

    const char *fmt = "%s\nnum_workers = %d\nnum_files = %d\nstorage_capacity = %d\nsockname = %s\nmy_logpath = %s\n\n-----------------------------------------------------------------------\n";
    write_log(my_log, fmt,
              "--------------------------------SERVER CONFIGURATION--------------------------------\n",
              configuration.num_workers, configuration.num_files, configuration.storage_capacity,
              configuration.sockname, configuration.logpath);

    retval = cache_initialization(configuration, &cache_table, &queue);
    CHECK_EQ_EXIT(server_cleanup(), "cache_initialization server.c", retval, -1, "calloc\n", "")
    fmt = "%s\n";
    write_log(my_log, fmt, "cache initialized successfully");

    // threadpool creation
    thread_num = configuration.num_workers;
    threads = calloc(thread_num, sizeof(pthread_t));
    tp = tpool_create(thread_num, &threads);
    if (!tp) {
        write_log(my_log, "Fatal error on threadpool creation");
        server_cleanup();
        exit(EXIT_FAILURE);
    }
    write_log(my_log, fmt, "threadpool started successfully");

    SYSCALL_EXIT(server_cleanup(), "pipe server.c", retval, pipe(signal_pipe), "retval\n", "")
    write_log(my_log, fmt, "signal_pipe created successfully");

    // socket creation
    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));
    strcpy(sa.sun_path, configuration.sockname);
    sa.sun_family = AF_UNIX;
    SYSCALL_EXIT(server_cleanup(), "socket server.c", sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "sockfd\n", "")
    int reuse_addr = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int));
    write_log(my_log, fmt, "socket created successfully");

    // socket preparation
    SYSCALL_EXIT(server_cleanup(), "bind server.c", retval, bind(sockfd, (struct sockaddr*) &sa, sizeof(sa)), "retval\n", "")
    SYSCALL_EXIT(server_cleanup(), "listen server.c", retval, listen(sockfd, SOMAXCONN), "retval\n", "")
    write_log(my_log, fmt, "listening socket");

    // starting of signals handling
    pthread_t signal_handler;
    signal_handler_arg_t args;

    args.signal_pipe_wfd = signal_pipe[1];
    CHECK_NEQ_EXIT(server_cleanup(), "pthread_create server.c", pthread_create(&signal_handler, NULL, signal_handler_thread, &args), 0, "", "")
    write_log(my_log, fmt, "signal handler started successfully");

    int end_server = 0;

    fd_set set, rset;
    int nfds = 0;
    FD_ZERO(&set);
    FD_SET(sockfd, &set);
    if (sockfd > nfds)
        nfds = sockfd;

    FD_SET(signal_pipe[0], &set);
    if (signal_pipe[0] > nfds)
        nfds = signal_pipe[0];

    server_command_t request;
    server_outcome_t outcome;
    memset(&request, 0 , sizeof(server_command_t));
    memset(&outcome, 0 , sizeof(server_outcome_t));

    SYSCALL_EXIT(server_cleanup(), "pipe server.c", retval, pipe(workers_pipe), "retval\n", "")
    write_log(my_log, fmt, "workers_pipe created successfully");

    FD_SET(workers_pipe[0], &set);
    if (workers_pipe[0] > nfds)
        nfds = workers_pipe[0];

    retval = initialize_conn_hash_table(&conn_table);
    if (retval == -1) {
        write_log(my_log, "Fatal error while initializing connections hash table");
        server_cleanup();
        exit(EXIT_FAILURE);
    }

    do {

        rset = set;
        SYSCALL_EXIT(server_cleanup(), "select server.c", retval, select(nfds + 1, &rset, NULL, NULL, NULL), "retval", "")

        for (int i = 0; i < nfds + 1; i++) {

            if (FD_ISSET(i, &rset)) {

                // the server is ready to accept new connections
                if (i == sockfd) {

                    int connfd;
                    SYSCALL_EXIT(server_cleanup(), "accept server.c", connfd, accept(sockfd, NULL, 0), "connfd", "")
                    fmt = "Client %d connected to the server\n";
                    write_log(my_log, fmt, connfd);
                    FD_SET(connfd, &set);
                    if (connfd > nfds)
                        nfds = connfd;
                    
                    retval = insert_client(connfd, &conn_table);
                    if (retval == -1) {
                        fmt = "%s\n";
                        write_log(my_log, fmt, "Fatal error while inserting the new client into the connections hash table");
                        server_cleanup();
                        exit(EXIT_FAILURE);
                    }

                    curr_conn++;
                    if (curr_conn > max_conn)
                        max_conn = curr_conn;
                    
                }
                else {

                    // a signal has arrived
                    if (i == signal_pipe[0]) {

                        SYSCALL_EXIT(server_cleanup(), "readn server.c", retval, readn(signal_pipe[0], &end_server, sizeof(int)), "retval", "")
                        FD_CLR(signal_pipe[0], &set);

                        if (end_server == 1) {
                            close(sockfd);
                            FD_CLR(sockfd, &set);
                            close(signal_pipe[0]);
                            close(signal_pipe[1]);
                        }
                        else {
                            close(signal_pipe[0]);
                            close(signal_pipe[1]);
                            break;
                        }

                        // I update nfds
                        int max = 0;
                        for (int j = 0; j < nfds + 1; j++) {
                            if (FD_ISSET(j, &set)) 
                                if (j > max)
                                    max = j;
                        }
                        nfds = max;

                    }

                    // a worker thread has finished its work, and I can hear the sent descriptor again
                    else if (i == workers_pipe[0]) {

                        int fd = -1;
                        SYSCALL_EXIT(server_cleanup(), "readn server.c", retval, readn(workers_pipe[0], &fd, sizeof(int)), "retval", "")
                        FD_SET(fd, &set);
                        if (fd > nfds)
                            nfds = fd;

                    }

                    // I read a request from a client previously connected
                    else {

                        retval = read_request(i, &request);
                        if (retval == -1) {
                            fmt = "%s\n";
                            write_log(my_log, fmt, "Error reading the request from the client [%d]\n", i);
                            server_cleanup();
                            exit(EXIT_FAILURE);
                        }
                        FD_CLR(i, &set);

                        // I update nfds
                        int max = 0;
                        for (int j = 0; j < nfds + 1; j++) {
                            if (FD_ISSET(j, &set)) 
                                if (j > max)
                                    max = j;
                        }
                        nfds = max;
                        
                        if (request.cmd == CMD_END) {
                            if (request.filepath)
                                free(request.filepath);
                            close(i);
                            curr_conn--;
                            fmt = "Client %d disconnected from the server\n";
                            write_log(my_log, fmt, i);
                            struct references_list *client_list = get_client_list(i, conn_table);
                            struct references_list *aux = client_list;
                            while (aux != NULL) {
                                struct my_file *temp = cache_get_file(cache_table, aux->file->pathname);
                                temp->is_locked = false;
                                aux = aux->next;
                            }
                            retval = remove_client(&conn_table, i);
                            if (retval == -1) {
                                fmt = "%s\n";
                                write_log(my_log, fmt, "Error removing client from connections hash table");
                                server_cleanup();
                                exit(EXIT_FAILURE);
                            }
                        }   
                        else {
                            retval = tpool_add_work(tp, request, i, workers_pipe[1]);
                            if (retval == -1) {
                                fmt = "%s\n";
                                write_log(my_log, fmt, "Error adding the work into the threadpool");
                                server_cleanup();
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
            }
        }

        if (end_server == 1 && tpool_list_is_empty(*tp))
            break;

    } while (end_server <= 1);

    pthread_join(signal_handler, NULL);
    fmt = "%s\n";
    write_log(my_log, fmt, "signal handler finished successfully");
    write_log(my_log, fmt, "Server shutdown...");

    server_cleanup();

    return 0;
}
