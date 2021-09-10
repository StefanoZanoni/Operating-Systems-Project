#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../common/util.h"
#include "../../headers/client/queue.h"
#include "../../headers/client/client.h"

/*
	* this function is used to read the arguments of the current option and divide them
	* into multiple strings so as to have in each node of the queue
	* the option and all its separate arguments 
*/
static char** tokenize(char *optarg, char opt) {

	//options accept a maximum of 100 arguments
	char **args = calloc(101, sizeof(char*));
	if (!args)
		return NULL;

	int i = 0;

	while (i < 101) {

		args[i++] = NULL;

	}

	int len = strlen(optarg) + 1;
	char *dup_optarg = strndup(optarg, len);

	//temp is used in order to maintain context between successive calls of strtok_r
	char *temp;
	char *token = strtok_r(dup_optarg, ",", &temp);

	i = 0;

	if (opt != 'w') {

		while (token && i < 100) {

			if (*token != '-') {

				len = strlen(token) + 1;
				args[i++] = strndup(token, len);

			}
			token = strtok_r(NULL, ",", &temp);
		}
	}
	//the -w option accepts a maximum of 2 arguments 
	else {

		args = realloc(args, 3 * sizeof(char*));
		if (!args)
			return NULL;

		while (i < 3) {

			args[i++] = NULL;

		}

		i = 0;

		while (token && i < 2) {

			if (*token != '-') {

				len = strlen(token) + 1;
				args[i++] = strndup(token, len);

			}
			token = strtok_r(NULL, ",", &temp);
		}
	}

	free(dup_optarg);
	dup_optarg = NULL;

	return args;
} 

/*
	* This function is used to parse the arguments passed by the terminal.
	* Options that take filenames as arguments ignore files with filenames starting with the '-'.
	* Also, all options ignore any arguments starting with the '-' character.
	* If the argument of an option is another option then ignore it and re-parsing that argument.
	* Every options that accept multiple arguments invoke tokenize function.
	* The options that don't accept any arguments store only their option name.
	* After parsing an option, it is stored in a queue.  
*/
void commands_parsing(int argc, char **argv, argsQueue_t *queue) {

	int opt = -1;

	while ( (opt = getopt(argc, argv, ":phf:w:W:D:R:r:d:t:l:u:c:")) != -1) {

		//arg is used to store the current argument passed by the terminal
		commandline_arg_t arg;
		memset(&arg, 0, sizeof(commandline_arg_t));

		switch (opt) {

			case 'h': {

				arg.option = opt;
				arg.args = NULL;
				enqueue(queue, &arg);

			} break;

			case 'f': {

				arg.option = opt;

				if (*optarg != '-') {

					arg.args = calloc(2, sizeof(char*));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args f\n", "");

					int len = strlen(optarg) + 1;
					arg.args[0] = calloc(len, sizeof(char));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args[0], NULL, "arg.args[0] f\n", "");
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

				arg.option = opt;
				arg.args = tokenize(optarg, opt);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args w\n", "");
				enqueue(queue, &arg);

			} break;

			case 'W': {

				arg.option = opt;
				arg.args = tokenize(optarg, opt);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args W\n", "");
				enqueue(queue, &arg);

			} break;

			case 'D': {

				arg.option = opt;

				if (*optarg != '-') {

					arg.args = calloc(2, sizeof(char*));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args D\n", "");

					int len = strlen(optarg) + 1;
					arg.args[0] = calloc(len, sizeof(char));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args[0], NULL, "arg.args[0] D\n", "");
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

				arg.option = opt;
				arg.args = tokenize(optarg, opt);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args r\n", "");
				enqueue(queue, &arg);

			} break;

			case 'R': {

				arg.option = opt;

				if (*optarg >= 48 && *optarg <= 57) {

					arg.args = calloc(2, sizeof(char*));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args R\n", "");
					long int temp = strtol(optarg, NULL, 10);
					int len = snprintf(NULL, 0, "%ld", temp);
					arg.args[0] = calloc(len + 1, sizeof(char));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args[0], NULL, "arg.args[0] R\n", "");
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

				arg.option = opt;

				if (*optarg != '-') {

					arg.args = calloc(2, sizeof(char*));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args d\n", "");

					int len = strlen(optarg) + 1;
					arg.args[0] = calloc(len, sizeof(char));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args[0], NULL, "arg.args[0] d\n", "");
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

				arg.option = opt;

				if (*optarg >= 48 && *optarg <= 57) {

					arg.args = calloc(2, sizeof(char*));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args R\n", "");
					long int temp = strtol(optarg, NULL, 10);
					int len = snprintf(NULL, 0, "%ld", temp);
					arg.args[0] = calloc(len + 1, sizeof(char));
					CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args[0], NULL, "arg.args[0] R\n", "");
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

				arg.option = opt;
				arg.args = tokenize(optarg, opt);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args l\n", "");
				enqueue(queue, &arg);

			} break;

			case 'u': {

				arg.option = opt;
				arg.args = tokenize(optarg, opt);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args u\n", "");
				enqueue(queue, &arg);

			} break;

			case 'c': {

				arg.option = opt;
				arg.args = tokenize(optarg, opt);
				CHECK_EQ_EXIT(client_cleanup(queue, &arg), "parsing.c calloc", arg.args, NULL, "arg.args c\n", "");
				enqueue(queue, &arg);

			} break;

			case 'p': {

				arg.option = opt;
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
