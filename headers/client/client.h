#ifndef PROGETTOSOL_CLIENT_H
#define PROGETTOSOL_CLIENT_H

#include "./queue.h"

extern int sockfd;
extern char *r_dir;
extern char *w_dir;
extern char *sockname;
extern struct timespec time_to_wait;

void client_cleanup(argsQueue_t *queue, commandline_arg_t *curr_arg);

#endif //PROGETTOSOL_CLIENT_H