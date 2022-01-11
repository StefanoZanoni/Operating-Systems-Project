#ifndef PROGETTOSOL_QUEUE_H
#define PROGETTOSOL_QUEUE_H

#include "./commandline.h"

typedef struct node {

	commandline_arg_t *arg;
	struct node *next;

} argnode_t;

typedef struct args_queue {

	argnode_t *head;
	argnode_t *tail;

} argsQueue_t;

/**
 * This function is used to sort the options provided by the command line
 *
 * @param queue
 */
void queue_sorting(argsQueue_t *queue);

/**
 * This function is used to initialize the queue
 *
 * @param queue
 */
void queue_init(argsQueue_t *queue);

/**
 * This function is used to check if the queue is empty
 *
 * @param queue
 * @return 1 if the queue is empty
 * @return 0 otherwise
 */
int is_queue_empty(argsQueue_t queue);

/**
 * This function is used to add an option in the queue
 *
 * @param queue
 * @param arg
 */
void enqueue(argsQueue_t *queue, commandline_arg_t *arg);

/**
 * This function is used to remove an option from the queue
 *
 * @param queue
 * @return
 */
argnode_t* dequeue(argsQueue_t *queue);

#endif //PROGETTOSOL_QUEUE_H