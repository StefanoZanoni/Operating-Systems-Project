#ifndef PROGETTOSOL_CLIENT_H
#define PROGETTOSOL_CLIENT_H

int sock_fd = 0;
char *r_dirname = NULL;
char *w_dirname = NULL;

void client_cleanup(argsQueue_t *queue, commandline_arg_t *curr_arg);

#endif //PROGETTOSOL_CLIENT_H