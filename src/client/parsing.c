#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../common/util.h"
#include "../../headers/client/queue.h"
#include "../../headers/client/client.h"

/**
 * This function is used to read the arguments of the current option and divide them
 * into multiple strings so as to have in each node of the queue
 * the option and all its separate arguments.
 *
 * @param opt_arg
 * @return NULL error (errno set)
 */
static char** tokenize(char *opt_arg) {

	char **args = NULL;
	int i = 0;
	size_t len;
	char *dup_optarg = NULL;
	char *token = NULL;

	//options accept a maximum of 100 arguments
	args = calloc(101, sizeof(char*));
	if (!args)
		return NULL;

	len = strlen(opt_arg) + 1;
	dup_optarg = strndup(opt_arg, len);
	if (!dup_optarg) {
		free(args);
		return NULL;
	}

	token = strtok(dup_optarg, ",");

	while (token && i < 100) {

		if (*token != '-') {

			len = strlen(token) + 1;
			args[i++] = strndup(token, len);

		}

		token = strtok(NULL, ",");

	}

	free(dup_optarg);

	return args;
} 

void commands_parsing(int argc, char **argv, argsQueue_t *queue) {

	int opt;

	while ( (opt = getopt(argc, argv, ":phf:w:W:D:R:r:d:t:l:u:c:")) != -1 ) {

		//arg is used to store the current argument passed by the terminal
		commandline_arg_t arg;
		memset(&arg, 0, sizeof(commandline_arg_t));

		switch (opt) {

			case 'h': {

				arg.option = (char) opt;
				arg.args = NULL;
				enqueue(queue, &arg);

			} break;

			case 'f': {

				arg.option = (char) opt;

				if (*optarg != '-') {

					arg.args = calloc(2, sizeof(char*));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args f\n", "")

					size_t len = strlen(optarg) + 1;
					arg.args[0] = calloc(len, sizeof(char));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args[0], NULL, "arg.args[0] f\n", "")
					strncpy(arg.args[0], optarg, len);
					arg.args[1] = NULL;

					enqueue(queue, &arg);

				}
				else {

					fprintf(stderr, "option -f invalid argument\n");
					arg.args = NULL;
					optind--;
				}

			} break;

			case 'w': {

				arg.option = (char) opt;
				arg.args = tokenize(optarg);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args w\n", "")
				enqueue(queue, &arg);

			} break;

			case 'W': {

				arg.option = (char) opt;
				arg.args = tokenize(optarg);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args W\n", "")
				enqueue(queue, &arg);

			} break;

			case 'D': {

				arg.option = (char) opt;

				if (*optarg != '-') {

					arg.args = calloc(2, sizeof(char*));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args D\n", "")

					size_t len = strlen(optarg) + 1;
					arg.args[0] = calloc(len, sizeof(char));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args[0], NULL, "arg.args[0] D\n", "")
					strncpy(arg.args[0], optarg, len);
					arg.args[1] = NULL;

					enqueue(queue, &arg);
				}
				else {

					fprintf(stderr, "option -D invalid argument\n");
					arg.args = NULL;
					optind--;
				}

			} break;

			case 'r': {

				arg.option = (char) opt;
				arg.args = tokenize(optarg);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args r\n", "")
				enqueue(queue, &arg);

			} break;

			case 'R': {

				arg.option = (char) opt;

				if (*optarg >= 48 && *optarg <= 57) {

					arg.args = calloc(2, sizeof(char*));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args R\n", "")
					long int temp = strtol(optarg, NULL, 10);
					int len = snprintf(NULL, 0, "%ld", temp);
					arg.args[0] = calloc(len + 1, sizeof(char));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args[0], NULL, "arg.args[0] R\n", "")
					snprintf(arg.args[0], len + 1, "%ld", temp);
					arg.args[1] = NULL;

				}
				//in case of the argument of -R is neither an integer number nor an option
				else if (*optarg != '-') {

					arg.args = NULL;

				}
				else if (*optarg == '-') {

					arg.args = NULL;
					optind--;

				}

				enqueue(queue, &arg);

			} break;

			case 'd': {

				arg.option = (char) opt;

				if (*optarg != '-') {

					arg.args = calloc(2, sizeof(char*));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args d\n", "")

					size_t len = strlen(optarg) + 1;
					arg.args[0] = calloc(len, sizeof(char));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args[0], NULL, "arg.args[0] d\n", "")
					strncpy(arg.args[0], optarg, len);
					arg.args[1] = NULL;

					enqueue(queue, &arg);

				}
				else {

					fprintf(stderr, "option -d invalid argument\n");
					arg.args = NULL;
					optind--;
				}

			} break;

			case 't': {

				arg.option = (char) opt;

				if (*optarg >= 48 && *optarg <= 57) {

					arg.args = calloc(2, sizeof(char*));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args R\n", "")
					long int temp = strtol(optarg, NULL, 10);
					int len = snprintf(NULL, 0, "%ld", temp);
					arg.args[0] = calloc(len + 1, sizeof(char));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args[0], NULL, "arg.args[0] R\n", "")
					snprintf(arg.args[0], len + 1, "%ld", temp);
					arg.args[1] = NULL;

				}
				//in case of the argument of -t is neither an integer number nor an option
				else if (*optarg != '-') {
					arg.args = NULL;
				}
				else if (*optarg == '-') {
					arg.args = NULL;
					optind--;
				}

				enqueue(queue, &arg);

			} break;

			case 'l': {

				arg.option = (char) opt;
				arg.args = tokenize(optarg);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args l\n", "")
				enqueue(queue, &arg);

			} break;

			case 'u': {

				arg.option = (char) opt;
				arg.args = tokenize(optarg);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args u\n", "")
				enqueue(queue, &arg);

			} break;

			case 'c': {

				arg.option = (char) opt;
				arg.args = tokenize(optarg);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args c\n", "")
				enqueue(queue, &arg);

			} break;

			case 'p': {

				arg.option = (char) opt;
				arg.args = NULL;
				enqueue(queue, &arg);

			} break;
			
			case ':': {

				if (optopt != 'R') {

					fprintf(stderr, "option -%c is missing a required argument\n", optopt);

				}
				else {

					arg.option = 'R';
					arg.args = NULL;
					enqueue(queue, &arg);

				}

			} break;

			default: {

				fprintf(stderr, "invalid option -%c\n", optopt);

			} break;

		}
	}
}
