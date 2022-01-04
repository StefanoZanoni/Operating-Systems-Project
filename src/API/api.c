#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h>
#include <sys/socket.h>

#include "../../common/conn.h"
#include "../../common/client_server.h"
#include "../../headers/client/client.h"
#include "../../headers/client/errors.h"
#include "api.h"

static int send_command(server_command_t command) {

	char *string = NULL;
	size_t len = 0;
	int retval;

	if (command.filepath)
		len = sizeof(command.cmd) + 1 + sizeof(command.flags) + 1 + sizeof(command.dir_is_set)
		 	+ 1 + sizeof(command.data_size) + 1 + sizeof(command.num_files) + 1 + (strlen(command.filepath) + 1);
	else
		len = sizeof(command.cmd) + 1 + sizeof(command.flags) + 1 + sizeof(command.dir_is_set)
			+ 1 + sizeof(command.data_size) + 1 + sizeof(command.num_files);

	string = calloc(len, sizeof(char));
	if (!string) {
		local_errno = ECALLOC;
		return -1;
	}
	snprintf(string, len, "%d|%d|%d|%zu|%d|%s", command.cmd, command.flags, command.dir_is_set,
                                                        command.data_size, command.num_files, command.filepath);

	retval = writen(sockfd, &len, sizeof(size_t));
	if (retval == -1) {
		local_errno = EWRITEN;
		free(string);
		return -1;
	}

	retval = writen(sockfd, string, len * sizeof(char));
	if (retval == -1) {
		local_errno = EWRITEN;
		free(string);
		return -1;
	}

	free(string);

	if (command.data) {
		retval = writen(sockfd, command.data, command.data_size);
		if (retval == -1) {
			local_errno = EWRITEN;
			return -1;
		}
	}

	return 0;
}

static int get_outcome(server_outcome_t *outcome) {

	int retval;
	size_t len = 0;
	char *string = NULL;

	retval = readn(sockfd, &(outcome->res), sizeof(int));
	if (retval == -1) {
		local_errno = EREADN;
		return -1;
	}
	if (outcome->res == -1) {
		local_errno = ESERVER;
		return -1;
	}

	retval = readn(sockfd, &len, sizeof(size_t));
	if (retval == -1) {
		local_errno = EREADN;
		return -1;
	}

    string = calloc(len, sizeof(char));
    if (!string) {
        local_errno = ECALLOC;
        return -1;
    }
	retval = readn(sockfd, string, len);
	if (retval == -1) {
		local_errno = EREADN;
		return -1;
	}

    size_t filename_len = len - (sizeof(int) + 1 + sizeof(int) + 1 + sizeof(size_t) + 1 + sizeof(size_t)
                                 + 1);

    if (filename_len > 0) {
        outcome->filename = calloc(filename_len, sizeof(char));
        if (!outcome->filename) {
            local_errno = ECALLOC;
            return -1;
        }
    }

	sscanf(string, "%d|%d|%zu|%zu|%s", &(outcome->all_read), &(outcome->ejected),
										  &(outcome->data_size), &(outcome->name_len),
										  outcome->filename);

    if (outcome->data_size > 0) {
        outcome->data = (char*) calloc(outcome->data_size, sizeof(char));
        if (!outcome->data) {
            local_errno = ECALLOC;
            return -1;
        }
        retval = readn(sockfd, outcome->data, outcome->data_size);
        if (retval == -1) {
            local_errno = EREADN;
            return -1;
        }
    }

    free(string);

	return 0;
}

static int write_inside_dir(const char *dirname, char *filepath, void *data, size_t data_size) {

	if (filepath != NULL) {

	    if ( strcmp(filepath, "(null)") != 0 ) {

	        size_t dirlen = strlen(dirname) + 1;

	        char *filename = basename(filepath);
	        if (!filename)
	            return -1;

	        char *path = calloc(dirlen + strlen(filename), sizeof(char));
	        strncpy(path, dirname, dirlen);
	        strcat(path, filename);

	        FILE *f = fopen(path, "wb");
	        if (!f) {
	            free(filepath);
	            local_errno = EFOPEN;
	            return -1;
	        }
	        fwrite(data, sizeof(char), data_size, f);

	        fclose(f);
	        free(path);
	        free(data);

	    }

	}

	return 0;
}

/*
	* This function is used to connect the client to the socket specified by sockname.
	* If a connection attempt fails, the client retries after msec milliseconds and 
	* for a maximum of abstime seconds.
*/
int openConnection(const char *sockname, int msec, const struct timespec abstime) {

	struct timespec ts;
	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;

	struct sockaddr_un sa;
	strcpy(sa.sun_path, sockname);
	sa.sun_family = AF_UNIX;

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1) {
		local_errno = ESOCK;
		return -1;
	}

	time_t start = time(NULL);
	unsigned int elapsed;

	do {

		time_t curr = time(NULL);
		elapsed = curr - start;

		errno = 0;
		if ( connect(sockfd, (struct sockaddr*) &sa, sizeof(sa)) == -1) {
			int retval;
			do {
				retval = nanosleep(&ts, &ts); 
			} while (retval && errno == EINTR);
		}
		else 
			break;

	} while ( elapsed < abstime.tv_sec );

	//connection failed in asbtime.tv_sec seconds
	if (errno != 0) {
		close (sockfd);
		local_errno = ECONN;
		return -1;
	}

	return 0;
}

/*
	* This function is used to close the connection with the socket
*/
int closeConnection(const char *sockname) {

	int retval;

	server_command_t command;
	memset(&command, 0, sizeof(server_command_t));

	command.cmd = CMD_END;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	if (close(sockfd) == -1) {
		local_errno = ECLOSE;
		return -1;
	}

	return 0;	
}

int openFile(const char *pathname, int flags) {

	int retval;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_OPEN_FILE;
	command.flags = flags;
	command.filepath = (char*) pathname;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	return 0;
}

/*
	* This function is used to read an entire file from the server.
	* All the content of the file is stored in the buf parameter
	* and the dimension of the buf is stored in size parameter (bytes dimension of the file).
*/
int readFile(const char *pathname, void **buf, size_t *size) {

	int retval;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_READ_FILE;
	command.filepath = (char*) pathname;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	*(char **) buf = strndup(outcome.data, outcome.data_size);
	*size = outcome.data_size;

	free(outcome.data);

	return 0;
}

/*
	* This function is used to read any N files from the server.
	* If N is undefined or 0, then all files stored in the server are read.
*/
int readNFiles(int N, const char *dirname) {

	int retval;
	size_t rbytes = 0;
	int num_read = 0;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));

	command.cmd = CMD_READ_FILE;
	command.num_files = N;
	if (dirname)
		command.dir_is_set = 1;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	do {

		memset(&outcome, 0, sizeof(server_outcome_t));
        retval = get_outcome(&outcome);
        if (retval == -1)
            return -1;

        rbytes += outcome.data_size;
        num_read++;

        write_inside_dir(dirname, outcome.filename, outcome.data, outcome.data_size);

        free( (char*) outcome.filename );
        N--;

    }
	while (N != 0 && !outcome.all_read && dirname);

	if (rbytes / 1000000 < 1) {
		double KB = (double) rbytes / 1000;
		printf("number of bytes read: %.3lf kB\n", KB);
	}
	else if (rbytes / 1000000000 < 1) {
		double MB = (double) rbytes / 1000000;
		printf("number of bytes read: %.3lf MB\n", MB);
	}

	return num_read;
}

int writeFile(const char *pathname, const char *dirname) {
	
	int retval;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_WRITE_FILE;
	command.filepath = (char*) pathname;
	if (dirname)
		command.dir_is_set = 1;

	retval = openFile(pathname, O_CREATE | O_LOCK);
	if (retval == -1)
		return -1;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	while (outcome.ejected && dirname) {
		retval = get_outcome(&outcome);
		if (retval == -1)
			return -1;
		write_inside_dir(dirname, outcome.filename, outcome.data, outcome.data_size);
	}

	if (outcome.filename)
		    free( (char*) outcome.filename );

	return 0;
}

/*
	* This function is used to add the size bytes of the buf pointer to the file pointed to by pathname.
	* If dirname is not equal to NULL, any files ejected from the server will be stored in dirname. 
*/
int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname) {

	int retval;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_APPEND_TO_FILE;
	command.data_size = size;
	command.filepath = (char*) pathname;
	command.data = buf;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	while (outcome.ejected && dirname) {
		retval = get_outcome(&outcome);
		if (retval == -1)
			return -1;
		write_inside_dir(dirname, outcome.filename, outcome.data, outcome.data_size);
	}

	if (outcome.filename)
		free( (char*) outcome.filename );
	if (outcome.data)
		free(outcome.data);

	return 0;	
}

/*
	* This is function is used by the client to request the acquisition of the lock on
	* the file pointed to by pathname.
*/
int lockFile(const char *pathname) {

	int retval;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_LOCK_FILE;
	command.filepath = (char*) pathname;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	return 0;
}

/*
	* This is function is used by the client to request the release of the lock on
	* the file pointed to by pathname.
*/
int unlockFile(const char *pathname) {

	int retval;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_UNLOCK_FILE;
	command.filepath = (char*) pathname;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	return 0;
}

/*
	* This is function is used by the client to request the closing
	* of the file pointed to by pathname.
*/
int closeFile(const char *pathname) {

	int retval;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_CLOSE_FILE;
	command.filepath = (char*) pathname;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	return 0;
}

/*
	* This is function is used by the client to request the removing
	* of the file pointed to by pathname.
*/
int removeFile(const char *pathname) {

	int retval;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_REMOVE_FILE;
	command.filepath = (char*) pathname;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	return 0;
}
