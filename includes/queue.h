#ifndef PROGETTOSOL_QUEUE_H
#define PROGETTOSOL_QUEUE_H

typedef struct cmdline_arg {

	unsigned int wbytes;
	unsigned int rbytes;
	char option;
	char **args;

} commandline_arg_t;

typedef struct node {

	commandline_arg_t *arg;
	struct node *next;

} argnode_t;

typedef struct args_queue {

	argnode_t *head;
	argnode_t *tail;

} argsQueue_t;

void queue_sorting(argsQueue_t *queue);
void queue_init(argsQueue_t *queue);
int is_queue_empty(argsQueue_t queue);
void enqueue(argsQueue_t *queue, commandline_arg_t *arg);
argnode_t* dequeue(argsQueue_t *queue);
void print_queue(argsQueue_t *queue);

#endif //PROGETTOSOL_QUEUE_H