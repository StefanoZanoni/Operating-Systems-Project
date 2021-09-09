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

#include "../../headers/client.h"
#include "../../headers/queue.h"
#include "../../headers/api.h"
#include "../../headers/parsing.h"
#include "../../headers/commands_execution.h"

int sockfd = -1;
char *r_dir = NULL;
char *w_dir = NULL;
char *sockname = NULL;
struct timespec time_to_wait;

static void free_arg(commandline_arg_t *arg) {

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
	* This function is used either in case of error or in case of client termination
	* to clean up: the queue, the terminal current argument, the reading directory,
	* the writing directory, the sockname and to close the connection with the server.
*/
void client_cleanup(argsQueue_t *queue, commandline_arg_t *curr_arg) {

	if (queue) {

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

	}

	if (curr_arg) {
		free_arg(curr_arg);
		curr_arg = NULL;
	}

	if (sockfd >= 0) 
		closeConnection(sockname);

	if (r_dir) {
		free(r_dir);
		r_dir = NULL;
	}

	if (w_dir) {
		free(w_dir);
		w_dir = NULL;
	}

	if (sockname) {
		free(sockname);
		sockname = NULL;
	}
}

int main(int argc, char **argv) {

	if (argc == 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		execute_h();
		exit(EXIT_FAILURE);
	}

	clock_t begin = clock();

	memset(&time_to_wait, 0, sizeof(struct timespec));

	argsQueue_t queue;
	queue_init(&queue);

	commands_parsing(argc, argv, &queue);

	queue_sorting(&queue);

	while ( !is_queue_empty(queue) ) {

		argnode_t *command_to_execute = NULL;
		command_to_execute = dequeue(&queue);
		int retval = execute_command( *(command_to_execute->arg) );
		free(command_to_execute->arg);
		free(command_to_execute);
		if (retval == -1) {
			client_cleanup(&queue, NULL);
			exit(EXIT_FAILURE);
		}
		else if (retval == 1){
			client_cleanup(&queue, NULL);
			return 0;
		}
	}

	client_cleanup(&queue, NULL);

	clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("%lf\n\n", time_spent);

	return 0;
}
