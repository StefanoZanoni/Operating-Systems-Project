#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../../common/util.h"
#include "../../headers/client/queue.h"
#include "../../headers/client/client.h"

static void move_to_the_head(argsQueue_t *queue, argnode_t *node) {

	argnode_t *curr = queue->head;

	if (curr) {

		while (curr->next != node) {

			curr = curr->next;
		}

		curr->next = node->next;
		node->next = queue->head;
		queue->head = node;
	}
}

static int is_at_the_head(argsQueue_t *queue, argnode_t *node) {

	if (queue->head == node)
		return 1;

	return 0;
}

static argnode_t* opt_research(argsQueue_t *queue, char opt) {

	argnode_t *curr = queue->head;

	while (curr) {

		if (curr->arg->option == opt)
			return curr;

		else curr = curr->next;

	}

	return NULL;
}

// -h -> -p -> -f -> -t -> -D -> -d -> any other option
void queue_sorting(argsQueue_t *queue) {

	argnode_t *to_move = NULL;

	if ( (to_move = opt_research(queue, 'd')) ) 
		if (!is_at_the_head(queue, to_move)) 
			move_to_the_head(queue, to_move);

	if ( (to_move = opt_research(queue, 'D')) ) 
		if (!is_at_the_head(queue, to_move))
			move_to_the_head(queue, to_move);

	if ( (to_move = opt_research(queue, 't')) )
		if (!is_at_the_head(queue, to_move))
			move_to_the_head(queue, to_move);

	if ( (to_move = opt_research(queue, 'f')) )
		if (!is_at_the_head(queue, to_move))
			move_to_the_head(queue, to_move);

	if ( (to_move = opt_research(queue, 'p')) )
		if (!is_at_the_head(queue, to_move))
			move_to_the_head(queue, to_move);

	if ( (to_move = opt_research(queue, 'h')) )
		if (!is_at_the_head(queue, to_move))
			move_to_the_head(queue, to_move);

	if (opt_research(queue, 'w') || opt_research(queue, 'W')) 
		if (!opt_research(queue, 'D'))
			fprintf(stderr, "Warning: no folder was specified to store the files eject by the server.\nEjected files will not be stored on disk.\n");
	
	if (opt_research(queue, 'r') || opt_research(queue, 'R')) 
		if (!opt_research(queue, 'd'))
			fprintf(stderr, "Warning: no folder was specified to store the files read by the server.\nRead files will not be stored on disk.\n");
}

void print_queue(argsQueue_t queue) {

	if (queue.head == NULL)
		fprintf(stderr, "Empty queue\n");

	argnode_t *curr = queue.head;

	while (curr != NULL) {

		if (curr->arg != NULL) {

			printf("-%c ", curr->arg->option);

			if (curr->arg->args != NULL) {

				int i = 0;
				while (curr->arg->args[i] != NULL) {

					if (curr->arg->args[i + 1] != NULL) 
						printf("%s,", curr->arg->args[i]);
					else
						printf("%s", curr->arg->args[i]);

					i++;

				}

			}
		}
		
		printf("\n");

		curr = curr->next;
	}
}

int is_queue_empty(argsQueue_t queue) {

	if (queue.head == NULL)
		return 1;

	return 0;
}

void queue_init(argsQueue_t *queue) {

	queue->head = NULL;
	queue->tail = NULL;
}

void enqueue(argsQueue_t *queue, commandline_arg_t *curr_arg) {

	if (queue->head == NULL) {

		queue->head = calloc(1, sizeof(argnode_t));
		CHECK_EQ_EXIT(client_cleanup(queue, curr_arg), "queue.c calloc", queue->head, NULL, "queue->head\n", "");
		queue->head->arg = calloc(1, sizeof(commandline_arg_t));
		CHECK_EQ_EXIT(client_cleanup(queue, curr_arg), "queue.c calloc", queue->head->arg, NULL, "queue->head->arg\n", "");
		memcpy(queue->head->arg, curr_arg, sizeof(commandline_arg_t));
		queue->head->next = NULL;
		queue->tail = queue->head;

	}
	else {

		queue->tail->next = calloc(1, sizeof(argnode_t));
		CHECK_EQ_EXIT(client_cleanup(queue, curr_arg), "queue.c calloc", queue->tail->next, NULL, "queue->tail->next\n", "");
		queue->tail = queue->tail->next;
		queue->tail->arg = calloc(1, sizeof(commandline_arg_t));
		CHECK_EQ_EXIT(client_cleanup(queue, curr_arg), "queue.c calloc", queue->tail->arg, NULL, "queue->tail->arg\n", "");
		memcpy(queue->tail->arg, curr_arg, sizeof(commandline_arg_t));
		queue->tail->next = NULL;

	}
}

argnode_t* dequeue(argsQueue_t *queue) {

	if (queue->head == NULL)

		return NULL;

	else {

		argnode_t *temp = calloc(1, sizeof(argnode_t));
		CHECK_EQ_EXIT(client_cleanup(queue, NULL), "queue.c calloc", temp, NULL, "temp\n", "");
		memcpy(temp, queue->head, sizeof(argnode_t));

		argnode_t *aux = queue->head;
		queue->head = queue->head->next;
		free(aux);

		return temp;
	}
}
