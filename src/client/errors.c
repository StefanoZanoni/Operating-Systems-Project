#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "../../headers/client/errors.h"

void my_perror(const char *string) {

	switch (local_errno) {

		case ESOCK: {

			fprintf(stderr, "%s ", string);
			perror("socket()");

		} break;

		case ECONN: {

			fprintf(stderr, "%s ", string);
			perror("connect()");

		} break;

		case EREADN: {

			fprintf(stderr, "%s ", string);
			perror("readn()");

		} break;

		case EWRITEN: {

			fprintf(stderr, "%s ", string);
			perror("writen()");

		} break;

		case ESERVER: {

			fprintf(stderr, "%s server error", string);

		} break;

		case ECALLOC: {

			fprintf(stderr, "%s ", string);
			perror("calloc()");

		} break;

		case ECLOSE: {

			fprintf(stderr, "%s ", string);
			perror("close()");

		} break;

		case EREALLOC: {

			fprintf(stderr, "%s ", string);
			perror("realloc()");

		} break;

		case EFOPEN: {

			fprintf(stderr, "%s ", string);
			perror("fopen()");

		} break;

		default: {

			fprintf(stderr, "%s ", string);
			perror(NULL);

		} break;
	}
}
