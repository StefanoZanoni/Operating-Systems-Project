#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

#include "../../common/conn.h"
#include "./api.h"
#include "../../headers/client/client.h"
#include "../../headers/client/errors.h"
#include "../../headers/client/commandline.h"
#include "../../common/client_server.h"

#define UNIX_PATH_MAX 108

static int send_command(server_command_t command) {

	char *string = NULL;
	size_t len = 0;
	int retval = 0;

	if (command.filepath)
		len = sizeof(command.cmd) + 1 + sizeof(command.flags) + 1 + sizeof(command.data_size) + 1 + sizeof(command.num_files) + 1 + (strlen(command.filepath) + 1);
	else
		len = sizeof(command.cmd) + 1 + sizeof(command.flags) + 1 + sizeof(command.data_size) + 1 + sizeof(command.num_files);
	string = calloc(len, sizeof(char));
	if (!string) {
		local_errno = ECALLOC;
		return -1;
	}
	snprintf(string, len, "%d|%d|%zu|%d|%s", command.cmd, command.flags, command.data_size, command.num_files, command.filepath);

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

	int retval = 0;
	size_t len = 0;
	char *string = NULL;
	char *temp = NULL;

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
	retval = readn(sockfd, string, len);
	if (retval == -1) {
		local_errno = EREADN;
		return -1;
	}

	temp = strtok(string, "|");
	outcome->all_read = atoi(temp);

	temp = strtok(NULL, "|");
	outcome->ejected = atoi(temp);

	temp = strtok(NULL, "|");
	sscanf(temp, "%zu", &(outcome->data_size));

	temp = strtok(NULL, "|");
	sscanf(temp, "%zu", &(outcome->name_len));

	temp = strtok(NULL, "|");
	outcome->filename = strndup(temp, outcome->name_len);

	temp = strtok(NULL, "|");
	outcome->data = strndup(temp, outcome->data_size);

	return 0;
}

static int write_inside_dir(const char *dirname, const char *filename, size_t name_len, void *data, size_t data_size) {

	size_t dirlen = 0;
	char *filepath = NULL;
	FILE *f = NULL;

	// I allocate memory for path where the file read by server will be stored
	// only if the dirname is specified
	dirlen = strlen(dirname) + 1;
	filepath = calloc(dirlen + name_len, sizeof(char));
	if (!filepath) {
		local_errno = ECALLOC;
		return -1;
	}
	strncpy(filepath, dirname, dirlen);
	strncat(filepath, filename, name_len);

	f = fopen(filepath, "wb");
	if (!f) {
		free(filepath);
		local_errno = EFOPEN;
		return -1;
	}
	fwrite(data, sizeof(char), data_size, f);

	fclose(f);
	free(filepath);

	return 0;
}

/*
	* This function is used to connect the client to the socket specified by sockname.
	* If a connection attempt fails, the client retries after msec milliseconds and 
	* for a maximum of abstime seconds.
*/
int openConnection(const char *sockname, int msec, const struct timespec abstime) {

	useconds_t usec = msec * 1000;

	struct sockaddr_un sa;
	strncpy(sa.sun_path, sockname, UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1) {
		local_errno = ESOCK;
		return -1;
	}

	time_t start = time(NULL);
	unsigned int elapsed = 0;

	do {

		time_t curr = time(NULL);
		elapsed = curr - start;

		errno = 0;
		if ( connect(sockfd, (struct sockaddr *) &sa, sizeof(sa)) == -1)
			usleep(usec);

	} while ( elapsed < abstime.tv_sec );

	//connection failed in asbtime.tv_sec seconds
	if (errno != 0) {
		local_errno = ECONN;
		return -1;
	}

	return 0;
}

/*
	* This function is used to close the connection with the socket
*/
int closeConnection(const char *sockname) {

	if (close(sockfd) == -1) {
		local_errno = ECLOSE;
		return -1;
	}

	return 0;	
}

int openFile(const char *pathname, int flags) {

	int retval = 0;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_OPEN_FILE;
	command.flags = flags;
	command.filepath = pathname;

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
	* All of the content of the file is stored in the buf parameter
	* and the dimension of the buf is stored in size parameter (bytes dimension of the file).
*/
int readFile(const char *pathname, void **buf, size_t *size) {

	int retval = 0;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_READ_FILE;
	command.data_size = strlen(pathname) + 1;
	command.filepath = pathname;

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

	int retval = 0;
	int rbytes = 0;
	int num_read = 0;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_READN_FILES;
	command.num_files = N;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	while (N != 0 && !outcome.all_read) {

		int retval = get_outcome(&outcome);
		if (retval == -1)
			return -1;

		rbytes += outcome.data_size;
		num_read++;

		if (dirname)
			write_inside_dir(dirname, outcome.filename, outcome.name_len, outcome.data, outcome.data_size);

		free( (char*) outcome.filename );
		outcome.filename = NULL;
		free(outcome.data);
		outcome.data = NULL;
		N--;
	}

	if (rbytes / 1000000 < 1) {
		double KB = (double) rbytes / 1000;
		printf("number of bytes read: %.3lf kB\n\n", KB);
	}
	else if (rbytes / 1000000000 < 1) {
		double MB = (double) rbytes / 1000000;
		printf("number of bytes read: %.3lf MB\n\n", MB);
	}

	return num_read;
}

int writeFile(const char *pathname, const char *dirname) {

	int retval = 0;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_WRITE_FILE;
	command.filepath = pathname;

	retval = openFile(pathname, O_CREATE | O_LOCK);
	if (retval == -1)
		return -1;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	if (outcome.ejected && dirname) 
		write_inside_dir(dirname, outcome.filename, outcome.name_len, outcome.data, outcome.data_size);

	if (outcome.filename)
		free( (char*) outcome.filename );
	if (outcome.data)
		free(outcome.data);

	return 0;
}

/*
	* This function is used to add the size bytes of the buf pointer to the file pointed to by pathname.
	* If dirname is not equal to NULL, any files ejected from the server will be stored in dirname. 
*/
int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname) {

	int retval = 0;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_APPEND_TO_FILE;
	command.data_size = size;
	command.filepath = pathname;
	command.data = buf;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	if (outcome.ejected && dirname) 
		write_inside_dir(dirname, outcome.filename, outcome.name_len, outcome.data, outcome.data_size);

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

	int retval = 0;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_LOCK_FILE;
	command.filepath = pathname;

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

	int retval = 0;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_UNLOCK_FILE;
	command.filepath = pathname;

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

	int retval = 0;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_CLOSE_FILE;
	command.filepath = pathname;

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

	int retval = 0;

	server_command_t command;
	server_outcome_t outcome;
	memset(&command, 0, sizeof(server_command_t));
	memset(&outcome, 0, sizeof(server_outcome_t));

	command.cmd = CMD_REMOVE_FILE;
	command.filepath = pathname;

	retval = send_command(command);
	if (retval == -1)
		return -1;

	retval = get_outcome(&outcome);
	if (retval == -1)
		return -1;

	return 0;
}
