#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

#include "../../headers/conn.h"
#include "../../headers/api.h"
#include "../../headers/client.h"
#include "../../headers/errors.h"

#define UNIX_PATH_MAX 108

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



	return 0;
}

/*
	* This function is used to read an entire file from the server.
	* All of the content of the file is stored in the buf parameter
	* and the dimension of the buf is stored in size parameter (bytes dimension of the file).
*/
int readFile(const char *pathname, void **buf, size_t *size) {

	int res = 0;

	size_t len = strlen(pathname) + 3;
	char *string = calloc(len, sizeof(char));
	if (!string) {
		local_errno = ECALLOC;
		return -1;
	}
	snprintf(string, len, "r|%s", pathname);

	//sending the operation to the server
	int retval = writen(sockfd, string, len);
	if (retval == -1) {
		free(string);
		local_errno = EWRITEN;
		return -1;
	}

	free(string);

	//I read the size of the file to read
	retval = readn(sockfd, size, sizeof(size_t));
	if (retval == -1) {
		if (*buf)
			free(*buf);
		*buf = NULL;
		*size = -1;
		local_errno = EREADN;
		return -1;
	}

	if (*buf == NULL) {
		*(char**) buf = calloc(*size, sizeof(char));
		if (!buf) {
			local_errno = ECALLOC;
			return -1;
		}
	}
	else {
		*(char**) buf = realloc(*buf, *size);
		if (!buf) {
			local_errno = EREALLOC;
			return -1;
		}
	}

	//I read the contents of the file
	retval = readn(sockfd, *buf, *size);
	if (retval == -1) {
		if (*buf)
			free(*buf);
		*buf = NULL;
		*size = -1;
		local_errno = EREADN;
		return -1;
	}

	// I read the outcome of the operation
	retval = readn(sockfd, &res, sizeof(int));
	if (retval == -1) {
		if (*buf)
			free(*buf);
		*buf = NULL;
		*size = -1;
		local_errno = EREADN;
		return -1;
	}
	//server operation error
	if (res == -1) {
		if (*buf)
			free(*buf);
		*buf = NULL;
		*size = -1;
		local_errno = ESERVER;
		return -1;
	}

	return 0;
}

/*
	* This function is used to read any N files from the server.
	* If N is undefined or 0, then all files stored in the server are read.
*/
int readNFiles(int N, const char *dirname) {

	unsigned int num_read = 0;
	int all_read = 0;

	size_t len = sizeof(int) + 3;
	char *string = calloc(len, sizeof(char));
	if (!string) {
		local_errno = ECALLOC;
		return -1;
	}

	snprintf(string, len, "N|%d", N);

	//sending the operation to the server
	int retval = writen(sockfd, string, len);
	if (retval == -1) {
		free(string);
		local_errno = EWRITEN;
		return -1;
	}

	int retval = 0;

	free(string);

	while (N != 0 && !all_read) {

		size_t size = 0;
		char *buf = NULL;
		char *filepath = NULL;
		FILE *f = NULL;

		// I allocate memory for path where the file read by server will be stored
		// only if the dirname is specified
		if (dirname) {

			size_t len = strlen(dirname) + 1;
			filepath = calloc(len + 100, sizeof(char));
			if (!filepath) {
				local_errno = ECALLOC;
				return -1;
			}
			strncpy(filepath, dirname, len);

		}

		//I read the size of the buffer that the server will send 
		retval = readn(sockfd, &size, sizeof(size_t));
		if (retval == -1) {
			if (filepath)
				free(filepath);
			local_errno = EREADN;
			return -1;
		}

		buf = calloc(size, sizeof(char));
		if (!buf) {
			if (filepath)
				free(filepath);
			local_errno = ECALLOC;
			return -1;
		}

		retval = readn(sockfd, buf, size);
		if (retval == -1) {
			if (filepath)
				free(filepath);
			free(buf);
			local_errno = EREADN;
			return -1;
		}

		// I store the file read by the sever in dirname directory
		// only if the latter has been specified
		if (dirname) {

			char *filename = strtok(buf, "-");
			strcat(filepath, filename);
			f = fopen(filepath, "wb");
			if (!f) {
				free(buf);
				free(filepath);
				local_errno = EFOPEN;
				return -1;
			}

			char *content = strtok(NULL, "\0");
			fwrite(content, sizeof(char), strlen(content) + 1, f);
			num_read++;

		}

		if (f)
			fclose(f);

		free(buf);
		free(filepath);
		N--;
	}

	return num_read;
}

int writeFile(const char *pathname, const char *dirname) {



	return 0;
}

int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname) {

	size_t len = strlen(pathname) + sizeof(buf) + sizeof(size) + 5;
	char *string = calloc(len, sizeof(char));
	if (!string) {
		local_errno = ECALLOC;
		return -1;
	}

	snprintf(string, len, "a|%s|%zu|%s", pathname, size, (char*) buf);

	//sending the operation to the server
	int retval = writen(sockfd, &len, sizeof(size_t));
	if (retval == -1) {
		local_errno = EWRITEN;
		return -1;
	}

	retval = writen(sockfd, string, len);
	if (retval == -1) {
		free(string);
		local_errno = EWRITEN;
		return -1;
	}

	free(string);

	//read the operation result
	int res = 0;
	retval = readn(sockfd, &res, sizeof(int));
	if (retval == -1) {
		local_errno = EREADN;
		return -1;
	}
	//server operation error
	if (res == -1) {
		local_errno = ESERVER;
		return -1;
	}

	//read the eject bit
	retval = readn(sockfd, &res, sizeof(int));
	if (retval == -1) {
		local_errno = EREADN;
		return -1;
	}

	//a file was ejected
	if (res == 1 && dirname != NULL) {

		char *filename = calloc(100, sizeof(char));
		if (!filename) {
			local_errno = ECALLOC;
			return -1;
		}
		size_t filesize = 0;
		retval = readn(sockfd, &filesize, sizeof(size_t));
		if (retval == -1) {
			local_errno = EREADN;
			return -1;
		}

		size_t dirlen = strlen(dirname) + 1;
		char *filepathname = calloc(dirlen + 100, sizeof(char));
		if (!filepathname) {
			local_errno = ECALLOC;
			return -1;
		}

		strncpy(filepathname, dirname, dirlen);

		retval = readn(sockfd, filename, 100);
		if (retval == -1) {
			local_errno = EREADN;
			return -1;
		}

		strcat(filepathname, filename);
		FILE *f = fopen(filepathname, "wb");
		if (f == NULL) {
			local_errno = EFOPEN;
			return -1;
		}

		free(filename);
		free(filepathname);

		//I tolerate files up to 100MB
		char *content = calloc(filesize, sizeof(char));
		if (!buf) {
			local_errno = ECALLOC;
			return -1;
		}

		retval = readn(sockfd, content, filesize);
		if (retval == -1) {
			local_errno = EREADN;
			return -1;
		}

		fwrite(content, sizeof(char), filesize, f);

		free(content);
		fclose(f);

	}
		
	return 0;	
}

int lockFile(const char *pathname) {

	size_t len = strlen(pathname) + 3;
	char *string = calloc(len, sizeof(char));
	if (!string) {
		local_errno = ECALLOC;
		return -1;
	}
	snprintf(string, len, "l|%s", pathname);

	//sending the operation to the server
	int retval = writen(sockfd, string, len);
	if (retval == -1) {
		free(string);
		local_errno = EWRITEN;
		return -1;
	}
	
	free(string);
	
	// I read the outcome of the operation
	int res = 0;
	retval = readn(sockfd, &res, sizeof(int));
	if (retval == -1) {
		local_errno = EREADN;
		return -1;
	}
	if (res == -1) {
		local_errno = ESERVER;
		return -1;
	}

	return 0;
}

int unlockFile(const char *pathname) {

	size_t len = strlen(pathname) + 3;
	char *string = calloc(len, sizeof(char));
	if (!string) {
		local_errno = ECALLOC;
		return -1;
	}
	snprintf(string, len, "u|%s", pathname);

	//sending the operation to the server
	int retval = writen(sockfd, string, len);
	if (retval == -1) {
		free(string);
		local_errno = EWRITEN;
		return -1;
	}

	free(string);

	// I read the outcome of the operation
	int res = 0;
	retval = readn(sockfd, &res, sizeof(int));
	if (retval == -1) {
		local_errno = EREADN;
		return -1;
	}
	if (res == -1) {
		local_errno = ESERVER;
		return -1;
	}

	return 0;
}

int closeFile(const char *pathname) {

	size_t len = strlen(pathname) + 3;
	char *string = calloc(len, sizeof(char));
	if (!string) {
		local_errno = ECALLOC;
		return -1;
	}
	snprintf(string, len, "k|%s", pathname);

	//sending the operation to the server
	int retval = writen(sockfd, string, len);
	if (retval == -1) {
		free(string);
		local_errno = EWRITEN;
		return -1;
	}

	free(string);

	// I read the outcome of the operation
	int res = 0;
	retval = readn(sockfd, &res, sizeof(int));
	if (retval == -1) {
		local_errno = EREADN;
		return -1;
	}
	if (res == -1) {
		local_errno = ESERVER;
		return -1;
	}

	return 0;
}

int removeFile(const char *pathname) {

	size_t len = strlen(pathname) + 3;
	char *string = calloc(len, sizeof(char));
	if (!string) {
		local_errno = ECALLOC;
		return -1;
	}
	snprintf(string, len, "c|%s", pathname);

	//sending the operation to the server
	int retval = writen(sockfd, string, len);
	if (retval == -1) {
		free(string);
		local_errno = EWRITEN;
		return -1;
	}

	free(string);

	// I read the outcome of the operation
	int res = 0;
	retval = readn(sockfd, &res, sizeof(int));
	if (retval == -1) {
		local_errno = EREADN;
		return -1;
	}
	if (res == -1) {
		local_errno = ESERVER;
		return -1;
	}

	return 0;
}
