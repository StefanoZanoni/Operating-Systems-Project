#ifndef PROGETTOSOL_REQUESTS_H
#define PROGETTOSOL_REQUESTS_H

#include "../../common/client_server.h"
#include "./threadpool.h"

/**
 * This function is performed in parallel by each thread of the threadpool
 *
 * @param request client's request
 * @param client the current client
 * @param workers_pipe_wfd file descriptor of the pipe write entry
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
worker_func_t execute_request;

/**
 * This function is used to read a request from a client
 *
 * @param client the current client
 * @param request structure on which the request will be stored
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int read_request(int client, server_command_t *request);

#endif //PROGETTOSOL_REQUESTS_H
