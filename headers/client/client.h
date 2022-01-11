#ifndef PROGETTOSOL_CLIENT_H
#define PROGETTOSOL_CLIENT_H

#include "./queue.h"

// the connection socket file descriptor
extern int sockfd;

// the directory where the client stores the files read from the server
extern char *r_dir;

// the directory where the client stores the files ejected by the server
extern char *w_dir;

// the nome of the server socket
extern char *sockname;

// the time to wait between two requests
extern struct timespec time_to_wait;

/**
 * This function is used either in case of error or in case of client termination
 * to clean up: the queue, the terminal current argument, the reading directory,
 * the writing directory, the sockname and to close the connection with the server.
 *
 * @param queue the list of the terminal arguments
 * @param curr_arg the current argument
 */
void client_cleanup(argsQueue_t *queue, commandline_arg_t *curr_arg);

#endif //PROGETTOSOL_CLIENT_H
