#ifndef PROGETTOSOL_REQUESTS_H
#define PROGETTOSOL_REQUESTS_H

#include "../../common/client_server.h"
#include "./threadpool.h"

int read_request(int client, server_command_t *request);
worker_func_t execute_request;

#endif //PROGETTOSOL_REQUESTS_H