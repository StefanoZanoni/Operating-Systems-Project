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

#include "../../headers/queue.h"
#include "../../headers/api.h"
#include "../../headers/parsing.h"
#include "../../headers/commands_execution.h"

static void free_arg(commandline_arg_t* arg) {

	if (arg) {

		if (arg->args) {

			int i = 0;
			while (arg->args[i] != NULL) {
				free(arg->args[i]);
				arg->args[i] = NULL;
				i++;
			}

			free(arg->args);
			arg->args = NULL;
		}
	}
}

/*
	* this function is used in case of error to clean up the queue and the terminal current argument
*/
void client_cleanup(argsQueue_t *queue, commandline_arg_t *curr_arg) {

	argnode_t *curr = queue->head;

	while(curr != NULL) {

		argnode_t *temp = curr;
		curr = curr->next;	
		free_arg(temp->arg);
		free(temp->arg);
		temp->arg = NULL;
		free(temp);
		temp = NULL;

	}

	free_arg(curr_arg);
}

int main(int argc, char **argv) {

	if (argc == 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		execute_h();
		exit(EXIT_FAILURE);
	}

	clock_t begin = clock();

	argsQueue_t queue;
	queue_init(&queue);

	commands_parsing(argc, argv, &queue);

	print_queue(&queue);

	queue_sorting(&queue);

	print_queue(&queue);

	while ( !is_queue_empty(queue) ) {

		argnode_t *command_to_execute = NULL;
		command_to_execute = dequeue(&queue);
		execute_command(command_to_execute, &queue);
		
	}

	clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("%lf\n", time_spent);

	return 0;
}
