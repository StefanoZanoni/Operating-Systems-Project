#include <stdio.h>
#include <unistd.h>

#include "../../headers/client/errors.h"

void my_perror(const char *string) {

	switch (local_errno) {

		case ESOCK: {

			fprintf(stderr, "Error: %s ", string);
			perror("socket()");
			fprintf(stderr, "\n\n");

		} break;

		case ECONN: {

			fprintf(stderr, "Error: %s ", string);
			perror("connect()");
			fprintf(stderr, "\n\n");

		} break;

		case EREADN: {

			fprintf(stderr, "Error: %s ", string);
			perror("readn()");
			fprintf(stderr, "\n\n");

		} break;

		case EWRITEN: {

			fprintf(stderr, "Error: %s ", string);
			perror("writen()");
			fprintf(stderr, "\n\n");

		} break;

		case ESERVER: {

			fprintf(stderr, "Error: %s server error\n\n", string);

		} break;

		case ECALLOC: {

			fprintf(stderr, "Error: %s ", string);
			perror("calloc()");
			fprintf(stderr, "\n\n");

		} break;

		case ECLOSE: {

			fprintf(stderr, "Error: %s ", string);
			perror("close()");
			fprintf(stderr, "\n\n");

		} break;

		case EREALLOC: {

			fprintf(stderr, "Error: %s ", string);
			perror("realloc()");
			fprintf(stderr, "\n\n");

		} break;

		case EFOPEN: {

			fprintf(stderr, "Error: %s ", string);
			perror("fopen()");
			fprintf(stderr, "\n\n");

		} break;

		default: {

			fprintf(stderr, "Error: %s ", string);
			perror(NULL);
			fprintf(stderr, "\n\n");

		} break;
	}
}
