#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../common/client_server.h"
#include "../../common/conn.h"
#include "../../common/util.h"
#include "../../headers/server/log.h"
#include "../../headers/server/cache.h"
#include "../../headers/server/connections.h"

static pthread_mutex_t open_mutex = PTHREAD_MUTEX_INITIALIZER;

int read_request(int client, server_command_t *request) {

	size_t len = 0;
	int retval;
	char *string = NULL;

	retval = readn(client, &len, sizeof(size_t));
	if (retval == -1) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "read_request: Error in reading client request length");
		return -1;
	}

	string = calloc(len, sizeof(char));
	if (!string) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "read_request: Error in string allocation");
		return -1;
	}

	retval = readn(client, string, len);
	if (retval == -1) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "read_request: Error in reading client string");
		free(string);
		return -1;
	}

    request->filepath = calloc(len, sizeof(char));

	sscanf(string, "%d|%d|%d|%zu|%d|%s", &(request->cmd), &(request->flags), &request->dir_is_set,
										 &(request->data_size), &(request->num_files), request->filepath);

	if (request->data_size > 0) {
        request->data = (char*) calloc(request->data_size, sizeof(char));
        if (!request->data) {
            const char *fmt = "%s\n";
            write_log(my_log, fmt, "read_request: Error in request->data allocation");
            return -1;
        }
		retval = readn(client, &(request->data), request->data_size);
		if (retval == -1) {
			free(string);
			return -1;
		}
	}

	free(string);

	return 0;
}

int send_outcome(int client, server_outcome_t outcome) {

	char *string = NULL;
	size_t len = 0;
	int retval;

	retval = writen(client, &(outcome.res), sizeof(int));
	if (retval == -1) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "send_outcome: Error in sending the operation result to the client");
		return -1;
	}
	if (outcome.res == -1)
		return 0;

	if (outcome.filename) {
		len = sizeof(int) + 1 + sizeof(int) + 1 + sizeof(size_t) + 1 + sizeof(size_t) + 1
		 															 + strlen(outcome.filename) + 1;
	}
	else
		len = sizeof(int) + 1 + sizeof(int) + 1 + sizeof(size_t) + 1 + sizeof(size_t) + 1;

	string = calloc(len, sizeof(char));
	if (!string) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "send_outcome: Error in string allocation");
		return -1;
	}

	snprintf(string, len, "%d|%d|%zu|%zu|%s", outcome.all_read, outcome.ejected, outcome.data_size,
                                                outcome.name_len, outcome.filename);

	retval = writen(client , &len, sizeof(size_t));
	if (retval == -1) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "send_outcome: Error in sending string length to the client");
		free(string);
		return -1;
	}

	retval = writen(client, string, len * sizeof(char));
	if (retval == -1) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "send_outcome: Error in sending string to the client");
		free(string);
		return -1;
	}

    free(string);

    if (outcome.data) {
        retval = writen(client, outcome.data, outcome.data_size);
        if (retval == -1) {
            const char *fmt = "%s\n";
            write_log(my_log, fmt, "send_outcome: Error in sending data to the client");
            return -1;
        }
    }

	return 0;
}

/**
 * This function is used to create a new file and add the latter to the open files list
 *
 * @param filepath the absolute path of the file
 * @param client the current client
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
static int create_file(char *filepath, int client) {

	LOCK( &open_mutex )

	int retval;

	if ( cache_search_file(cache_table, filepath) || files_list_search_file(flist, filepath, &flist_mutex)) {

		fprintf(stderr, "Error client %d: the file %s is already present on the server\n", client, filepath);
		UNLOCK( &open_mutex )
		return -1;

	}

	struct my_file *file = new_file(filepath, cache_table);
	if (!file) {
		const char *fmt = "Client %d CMD_OPEN_FILE: Error in new_file allocation %s";
		write_log(my_log, fmt, client, filepath);
		UNLOCK( &open_mutex )
		return -1;
	}

	retval = files_list_add(&flist, file, &flist_mutex);
	if (retval == -1) {
		const char *fmt = "Client %d CMD_OPEN_FILE: Error in files_list_add %s\n";
		write_log(my_log, fmt, client, filepath);
        free(file);
        UNLOCK( &open_mutex )
		return -1;
	}

	UNLOCK( &open_mutex )

	return 0;
}

/**
 * This function is used to create a copy of the file so that I have a separate copy of the file in the cache
 *
 * @param file the file to be copied
 *
 * @return NULL error (errno set)
 * @return copy success
 */
static struct my_file *file_copy(struct my_file *file) {

    struct my_file *copy = calloc(1, sizeof(struct my_file));
    if (!copy) {
        const char *fmt = "%s\n";
        write_log(my_log, fmt, "lock_file: Error in temp allocation");
        return NULL;
    }
    copy->pathname = calloc(strlen(file->pathname) + 1, sizeof(char));
    if (!copy->pathname) {
        const char *fmt = "%s\n";
        write_log(my_log, fmt, "lock_file: Error in temp->pathname allocation");
        free(copy);
        return NULL;
    }
    copy->content = (char*) calloc(file->file_size, sizeof(char));
    if (!copy->content) {
        const char *fmt = "%s\n";
        write_log(my_log, fmt, "lock_file: Error in temp->content allocation");
        free(copy->pathname);
        free(copy);
        return NULL;
    }
    copy->fidx = file->fidx;
    copy->file_size = file->file_size;
    copy->is_locked = file->is_locked;
    copy->pathname = strcpy(copy->pathname, file->pathname);
    copy->content = memcpy(copy->content, file->content, file->file_size);
    copy->f = file->f;

    return copy;

}

/**
 * This function is used to lock a file present in the cache or in the open files list
 *
 * @param filepath the absolute path of the file
 * @param client the current client
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
static int lock_file(char *filepath, int client) {

    if ( cache_search_file(cache_table, filepath) ) {

        struct my_file *file = cache_get_file(cache_table, filepath);
        if (file->is_locked == false) {

            file->is_locked = true;
            struct my_file *temp = calloc(1, sizeof(struct my_file));
            if (!temp) {
                const char *fmt = "%s\n";
                write_log(my_log, fmt, "lock_file: Error in temp allocation");
                return -1;
            }
            temp->pathname = calloc(strlen(filepath) + 1, sizeof(char));
            if (!temp->pathname) {
                const char *fmt = "%s\n";
                write_log(my_log, fmt, "lock_file: Error in temp->pathname allocation");
                free(temp);
                return -1;
            }
            temp->content = (char*) calloc(file->file_size, sizeof(char));
            if (!temp->content) {
                const char *fmt = "%s\n";
                write_log(my_log, fmt, "lock_file: Error in temp->content allocation");
                free(temp->pathname);
                free(temp);
                return -1;
            }
            temp->fidx = file->fidx;
            temp->file_size = file->file_size;
            temp->is_locked = file->is_locked;
            temp->pathname = strcpy(temp->pathname, file->pathname);
            temp->content = memcpy(temp->content, file->content, file->file_size);
            temp->f = file->f;

            conn_hash_table_add_file(client, &conn_table, temp);

        }
        else if ( !files_list_search_file(get_client_list(client, conn_table), filepath,
                                        &(conn_table.clients[client].list->conn_list_mutex)) ) {
            fprintf(stderr,
                    "The file %s that client %d is trying to lock is owned by another client\n",
                    filepath, client);
            return -1;
        }

    }
    else {

        if ( !files_list_search_file(flist, filepath, &flist_mutex) ) {
            fprintf(stderr,
                    "Error client %d: the file %s on which the lock was requested is not present on the server\n",
                    client, filepath);
            return -1;
        }
        else {

            struct my_file *file = files_list_get_file(flist, filepath, &flist_mutex);
            if (file->is_locked == false) {

                file->is_locked = true;
                struct my_file *temp = calloc(1, sizeof(struct my_file));
                if (!temp) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "lock_file: Error in temp allocation");
                    return -1;
                }
                temp->pathname = calloc(strlen(filepath) + 1, sizeof(char));
                if (!temp->pathname) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "lock_file: Error in temp->pathname allocation");
                    free(temp);
                    return -1;
                }
                temp->content = (char*) calloc(file->file_size, sizeof(char));
                if (!temp->content) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "lock_file: Error in temp->content allocation");
                    free(temp->pathname);
                    free(temp);
                    return -1;
                }
                temp->fidx = file->fidx;
                temp->file_size = file->file_size;
                temp->is_locked = file->is_locked;
                temp->pathname = strcpy(temp->pathname, file->pathname);
                temp->content = memcpy(temp->content, file->content, file->file_size);
                temp->f = file->f;
                conn_hash_table_add_file(client, &conn_table, temp);

            }
            else if ( !files_list_search_file(get_client_list(client, conn_table), filepath,
                                              &(conn_table.clients[client].list->conn_list_mutex)) ) {
                fprintf(stderr,
                        "The file %s that client %d is trying to lock is owned by another client\n",
                        filepath, client);
                return -1;
            }
        }

    }

    return 0;
}

int execute_request(server_command_t request, int client, int workers_pipe_wfd) {

	if (client < 0) {
		errno = EINVAL;
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "Error in execute_request parameters");
        return -1;
	}

    char *filepath = NULL;

    // the request to read a file may not contain the file name
    if (request.cmd != CMD_READ_FILE) {
        filepath = calloc(strlen(request.filepath) + 1, sizeof(char));
        if (!filepath) {
            const char *fmt = "%s\n";
            write_log(my_log, fmt, "execute_request: Error in filepath allocation");
            return -1;
        }
        filepath = strcpy(filepath, request.filepath);
    }
    // the request to read a file contain the file name
    else if (request.num_files == 0) {
        filepath = calloc(strlen(request.filepath) + 1, sizeof(char));
        if (!filepath) {
            const char *fmt = "%s\n";
            write_log(my_log, fmt, "execute_request: Error in filepath allocation");
            return -1;
        }
        filepath = strcpy(filepath, request.filepath);
    }
	int flags = request.flags;
	int dir_is_set = request.dir_is_set;
	int num_files = request.num_files;
	void *data = request.data;
	size_t data_size = request.data_size;
	int retval;
	server_outcome_t outcome;
	memset(&outcome, 0, sizeof(outcome));

    // this vector stores the ejected files during the cache replacement algorithm execution
	struct my_file *ejected = NULL;
    // this size of this vector
	unsigned long int ejected_dim = 0;

	switch (request.cmd) {

		case CMD_OPEN_FILE: {

			if (!filepath) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "Fatal error: filepath was not specified");
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
				return -1;
			}

			if (flags == O_CREATE) {
				
				retval = create_file(filepath, client);
				if (retval == -1) {
					outcome.res = -1;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						const char *fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
					}
                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
                    if (retval == -1) {
                        const char *fmt = "%s\n";
                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                    }
                    free(filepath);
					return -1;

				}

				outcome.res = 0;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					const char *fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
                    free(filepath);
					return -1;
				}

				const char *fmt = "Client %d: The open file is %s\n";
				write_log(my_log, fmt, client, filepath);
	
			}
			else if (flags == O_LOCK) {

				retval = lock_file(filepath, client);
				if (retval == -1) {
					outcome.res = -1;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						const char *fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
					}
                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
                    if (retval == -1) {
                        const char *fmt = "%s\n";
                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                    }
                    free(filepath);
					return -1;
				}

				outcome.res = 0;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					const char *fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
                    free(filepath);
					return -1;
				}

			}
			else if ( flags == (O_CREATE | O_LOCK) ) {

				if ( cache_search_file(cache_table, filepath) || files_list_search_file(flist, filepath, &flist_mutex) ) {

					retval = lock_file(filepath, client);
					if (retval == -1) {
						outcome.res = -1;
						retval = send_outcome(client, outcome);
						if (retval == -1) {
							const char *fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
							write_log(my_log, fmt, client);
						}
                        retval = writen(workers_pipe_wfd, &client, sizeof(int));
                        if (retval == -1) {
                            const char *fmt = "%s\n";
                            write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                        }
                        free(filepath);
						return -1;
					}

					outcome.res = 0;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						const char *fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
                        free(filepath);
						return -1;
					}

				}
				else {

					retval = create_file(filepath, client);
					if (retval == -1) {
						outcome.res = -1;
						retval = send_outcome(client, outcome);
						if (retval == -1) {
							const char *fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
							write_log(my_log, fmt, client);
						}
                        retval = writen(workers_pipe_wfd, &client, sizeof(int));
                        if (retval == -1) {
                            const char *fmt = "%s\n";
                            write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                        }
                        free(filepath);
						return -1;
					}
					retval = lock_file(filepath, client);
					if (retval == -1) {
						outcome.res = -1;
						retval = send_outcome(client, outcome);
						if (retval == -1) {
							const char *fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
							write_log(my_log, fmt, client);
						}
                        retval = writen(workers_pipe_wfd, &client, sizeof(int));
                        if (retval == -1) {
                            const char *fmt = "%s\n";
                            write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                        }
                        free(filepath);
						return -1;
					}

					outcome.res = 0;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						const char *fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
                        free(filepath);
						return -1;
					}

					const char *fmt = "Client %d: The open file is %s\n";
					write_log(my_log, fmt, client, filepath);

				}

			}
			else {

				if ( !cache_search_file(cache_table, filepath) && !files_list_search_file(flist, filepath, &flist_mutex)) {

					fprintf(stderr, "Error client %d: the file %s does not exist on the server and the flag O_CREATE has not been specified\n", client, filepath);
					outcome.res = -1;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						const char *fmt = "Client %d CMD_OPEN_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
					}
					retval = writen(workers_pipe_wfd, &client, sizeof(int));
                    if (retval == -1) {
                        const char *fmt = "%s\n";
                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                    }
                    free(filepath);
					return -1;

				}

			}

		} break;





		case CMD_READ_FILE: {

			struct my_file *file = NULL;

			if (filepath) {

				if (cache_search_file(cache_table, filepath)) {

					file = cache_get_file(cache_table, filepath);

					if (file->is_locked == true) {
						if ( !files_list_search_file(get_client_list(client, conn_table), filepath, &(conn_table.clients[client].list->conn_list_mutex)) ) {
							fprintf(stderr, "Error: the file on which the reading was requested is owned by another client\n");
							outcome.res = -1;
							retval = send_outcome(client, outcome);
							if (retval == -1) {
								const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
								write_log(my_log, fmt, client);
							}
                            retval = writen(workers_pipe_wfd, &client, sizeof(int));
                            if (retval == -1) {
                                const char *fmt = "%s\n";
                                write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                            }
                            free(filepath);
							return -1;
						}
					}

					reference_file(&queue, &cache_table, file, &ejected, &ejected_dim);
					outcome.data = file->content;
					outcome.data_size = file->file_size;
					outcome.res = 0;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
                        free(filepath);
						return -1;
					}

					const char *fmt = "Client %d: the read file is %s, size %ld\n";
					write_log(my_log, fmt, client, filepath, file->file_size);

				}
				else {

					fprintf(stderr, "Error client %d: the requested file is not present on the server\n", client);
					outcome.res = -1;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
					}
                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
                    if (retval == -1) {
                        const char *fmt = "%s\n";
                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                    }
                    free(filepath);
					return -1;

				}

			}
			else {

				if (dir_is_set) {

					if (num_files < 0) {

						for (int i = 0; i < cache_table.capacity; i++) {

							if (cache_table.files_references[i] != NULL) {

								file = cache_get_file(cache_table, cache_table.files_references[i]->file->pathname);

								int retval = 1;
								if (file->is_locked == true) {
									retval = files_list_search_file(get_client_list(client, conn_table), file->pathname,
	                                                             &(conn_table.clients[client].list->conn_list_mutex));
									if (retval == 0) {
										fprintf(stderr, "Error: the file on which the reading was requested	is owned by another client\n");
										outcome.res = -1;
										retval = send_outcome(client, outcome);
										if (retval == -1) {
											const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
											write_log(my_log, fmt, client);
										}
	                                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
	                                    if (retval == -1) {
	                                        const char *fmt = "%s\n";
	                                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
	                                    }
										return -1;
									}
									else if (retval == -1) {
										outcome.res = -1;
										retval = send_outcome(client, outcome);
										if (retval == -1) {
											const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
											write_log(my_log, fmt, client);
										}
	                                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
	                                    if (retval == -1) {
	                                        const char *fmt = "%s\n";
	                                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
	                                    }
										return -1;
									}
								}

								reference_file(&queue, &cache_table, file, &ejected, &ejected_dim);
								outcome.data = file->content;
								outcome.data_size = file->file_size;
								outcome.name_len = strlen(file->pathname) + 1;
								outcome.filename = file->pathname;
								outcome.res = 0;
								retval = send_outcome(client, outcome);
								if (retval == -1) {
									const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
									write_log(my_log, fmt, client);
									return -1;
								}
								memset(&outcome, 0, sizeof(outcome));

								const char *fmt = "Client %d: the read file is %s, size %ld\n";
								write_log(my_log, fmt, client, file->pathname, file->file_size);

							}

						}

						outcome.all_read = 1;
						outcome.res = 0;
						retval = send_outcome(client, outcome);
						if (retval == -1) {
							const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
							write_log(my_log, fmt, client);
							return -1;
						}

					}
					else {

						int i = 0;

						while (num_files > 0) {

							while (i < cache_table.capacity - 1 && cache_table.files_references[i] == NULL)
								i++;
							
							//this check is necessary if the last element is null
							if (cache_table.files_references[i])
								file = cache_get_file(cache_table, cache_table.files_references[i]->file->pathname);
							else 
								break;

							retval = 1;
							if (file->is_locked == true) {
								retval = files_list_search_file(get_client_list(client, conn_table), file->pathname,
                                                             &(conn_table.clients[client].list->conn_list_mutex));
								if (retval == 0) {
									fprintf(stderr, "Error: the file on which the reading was requested	is owned by another client\n");
									outcome.res = -1;
									retval = send_outcome(client, outcome);
									if (retval == -1) {
										const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
										write_log(my_log, fmt, client);
									}
                                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
                                    if (retval == -1) {
                                        const char *fmt = "%s\n";
                                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                                    }
									return -1;
								}
								else if (retval == -1) {
									outcome.res = -1;
									retval = send_outcome(client, outcome);
									if (retval == -1) {
										const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
										write_log(my_log, fmt, client);
									}
                                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
                                    if (retval == -1) {
                                        const char *fmt = "%s\n";
                                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                                    }
									return -1;
								}
							}

							reference_file(&queue, &cache_table, file, &ejected, &ejected_dim);
							outcome.data = file->content;
							outcome.data_size = file->file_size;
							outcome.name_len = strlen(file->pathname) + 1;
							outcome.filename = file->pathname;
							outcome.res = 0;
							retval = send_outcome(client, outcome);
							if (retval == -1) {
								const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
								write_log(my_log, fmt, client);
								return -1;
							}
							memset(&outcome, 0, sizeof(outcome));

							num_files--;
							i++;

							const char *fmt = "Client %d: the read file is %s, size %ld\n";
							write_log(my_log, fmt, client, file->pathname, file->file_size);
						}

						// this is necessary if I exit the loop before all required files have been read
						if (num_files > 0) {
                            outcome.all_read = 1;
                            outcome.res = 0;
                            retval = send_outcome(client, outcome);
                            if (retval == -1) {
                                const char *fmt = "Client %d CMD_READ_FILE: Error while sending the outcome\n";
                                write_log(my_log, fmt, client);
                                return -1;
                            }
                        }

					}
				}

			}

		} break;




		case CMD_WRITE_FILE: {

			if (!filepath) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "Fatal error: filepath was not specified");
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_WRITE_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
				return -1;
			}

			struct my_file *tmp = files_list_get_file(flist, filepath, &flist_mutex);

            if (!tmp) {

                fprintf(stderr, "The file %s has already been written to the server\n", filepath);
                outcome.res = -1;
                retval = send_outcome(client, outcome);
                if (retval == -1) {
                    const char *fmt = "Client %d CMD_WRITE_FILE: Error while sending the outcome\n";
                    write_log(my_log, fmt, client);
                }
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                free(filepath);
                return -1;

            }

            struct my_file *file = file_copy(tmp);

			if (file->is_locked == true) {
				if ( !files_list_search_file(get_client_list(client, conn_table), filepath,
                                             &(conn_table.clients[client].list->conn_list_mutex)) ) {
					fprintf(stderr, "Error: the file on which the writing was requested is owned by another client\n");
					outcome.res = -1;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						const char *fmt = "Client %d CMD_WRITE_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
					}
                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
                    if (retval == -1) {
                        const char *fmt = "%s\n";
                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                    }
                    free(file->pathname);
                    free(file->content);
                    free(file);
                    free(filepath);
					return -1;
				}
			}

			retval = enqueue(&queue, &cache_table, file, &ejected, &ejected_dim);
			if (retval == -1) {
				const char *fmt = "Client %d CMD_WRITE_FILE: Error in enqueue\n";
				write_log(my_log, fmt, client);
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_WRITE_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                files_list_remove(&flist, *tmp, &flist_mutex);
                free(file->pathname);
                free(file->content);
                free(file);
                free(filepath);
				return -1;
			}
			if (retval == -2) {
                conn_hash_table_remove_file(client, &conn_table, *file);
                free(file->pathname);
                free(file->content);
                fclose(file->f);
                free(file);
			}

			const char *fmt = NULL;

			if (retval != -2) {
	            fmt = "Client %d: the written file is %s, size %ld bytes\n";
	            write_log(my_log, fmt, client, filepath, file->file_size);
	        }

            retval = files_list_remove(&flist, *tmp, &flist_mutex);
            if (retval == -1) {
                fmt = "The file %s is already removed from flist\n";
                write_log(my_log, fmt, filepath);
                free(filepath);
                if (ejected) {
                    for (unsigned long int i = 0; i < ejected_dim; i++) {
                        free(ejected[i].pathname);
                        free(ejected[i].content);
                    }
                    free(ejected);
                }
                return -1;
            }

			if (ejected_dim > 0) {

                outcome.ejected = 1;

                struct my_file temp;
				memset(&temp, 0, sizeof(struct my_file));
                for (unsigned long int i = 0; i < ejected_dim && memcmp(&ejected[i], &temp, sizeof(struct my_file)) != 0; i++) {
                    conn_hash_table_remove_file(client, &conn_table, ejected[i]);
                }

            }

			outcome.res = 0;
			retval = send_outcome(client, outcome);
				if (retval == -1) {
				fmt = "Client %d CMD_WRITE_FILE: Error while sending the outcome\n";
				write_log(my_log, fmt, client);
                free(filepath);
                if (ejected) {
                    for (unsigned long int i = 0; i > ejected_dim; i++) {
                        free(ejected[i].pathname);
                        free(ejected[i].content);
                    }
                    free(ejected);
                }
				return -1;
			}

			if (ejected_dim > 0 && dir_is_set) {

				unsigned long int i = 0;

				// it is used as 0
				struct my_file temp;
				memset(&temp, 0, sizeof(struct my_file));

				while ( i < ejected_dim && memcmp(&ejected[i], &temp, sizeof(struct my_file)) != 0 ) {
					outcome.ejected = 1;
					outcome.filename = ejected[i].pathname;
					outcome.name_len = strlen(ejected[i].pathname) + 1;
					outcome.data = ejected[i].content;
					outcome.data_size = ejected[i].file_size;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						fmt = "Client %d CMD_WRITE_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
                        free(filepath);
                        for (unsigned long int j = i; j < ejected_dim; j++) {
                            free(ejected[j].pathname);
                            free(ejected[j].content);
                        }
                        free(ejected);
						return -1;
					}
					memset(&outcome, 0, sizeof(outcome));
                    free(ejected[i].pathname);
                    ejected[i].pathname = NULL;
                    free(ejected[i].content);
                    ejected[i].content = NULL;
					i++;
				}

				outcome.ejected = 0;
				outcome.res = 0;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_WRITE_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
                    free(filepath);
                    for (unsigned long int j = i; j < ejected_dim; j++) {
                        free(ejected[j].pathname);
                        free(ejected[j].content);
                    }
                    free(ejected);
                    return -1;
				}

                free(ejected);

			}

            if (ejected_dim > 0 && !dir_is_set) {

                unsigned long int i = 0;

                // it is used as 0
                struct my_file temp;
                memset(&temp, 0, sizeof(struct my_file));

                while ( i < ejected_dim && memcmp(&ejected[i], &temp, sizeof(struct my_file)) != 0 ) {
                    free(ejected[i].pathname);
                    free(ejected[i].content);
                    i++;
                }
                free(ejected);

            }
            ejected = NULL;
			
		} break;




		case CMD_APPEND_TO_FILE: {

			if (!filepath) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "Fatal error: filepath was not specified");
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_APPEND_TO_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
				return -1;
			}

			if ( !cache_search_file(cache_table, filepath) ) {
				fprintf(stderr, "Error: the file %s on which the appending was requested is not on the server\n", filepath);
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					const char *fmt = "Client %d CMD_APPEND_TO_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                free(filepath);
				return -1;
			}

			struct my_file *file = cache_get_file(cache_table, filepath);

			if (file->is_locked == true) {
				if ( !files_list_search_file(get_client_list(client, conn_table), filepath,
                                             &(conn_table.clients[client].list->conn_list_mutex)) ) {
					fprintf(stderr, "Error: the file on which the appending was requested is owned by another client\n");
					outcome.res = -1;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						const char *fmt = "Client %d CMD_APPEND_TO_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
					}
                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
                    if (retval == -1) {
                        const char *fmt = "%s\n";
                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                    }
                    free(filepath);
					return -1;
				}
			}

			// I need to reference the file here to ensure that it does not self-expel
			reference_file(&queue, &cache_table, file, &ejected, &ejected_dim);
			retval = cache_file_append(&queue, &cache_table, filepath, &ejected, &ejected_dim, data, data_size);
			if (retval == -1) {
				const char *fmt = "Client %d CMD_APPEND_TO_FILE: Error in cache_file_append\n";
				write_log(my_log, fmt, client);
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_APPEND_TO_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                free(filepath);
				return -1;
			}

			const char *fmt = "Client %d: the file to which the %ld bytes have been added is %s\n";
			write_log(my_log, fmt, client, file->file_size, filepath);

			if (ejected_dim > 0) {

                outcome.ejected = 1;

                struct my_file temp;
                memset(&temp, 0, sizeof(struct my_file));
                for (unsigned long int i = 0; i < ejected_dim && memcmp(&ejected[i], &temp, sizeof(struct my_file)) != 0; i++) {
                    conn_hash_table_remove_file(client, &conn_table, ejected[i]);
                }

            }

			outcome.res = 0;
			retval = send_outcome(client, outcome);
				if (retval == -1) {
				fmt = "Client %d CMD_WRITE_FILE: Error while sending the outcome\n";
				write_log(my_log, fmt, client);
                if (ejected) {
                    for (unsigned long int i = 0; i < ejected_dim; i++) {
                        free(ejected[i].pathname);
                        free(ejected[i].content);
                    }
                    free(ejected);
                }
                free(filepath);
				return -1;
			}

			if (ejected_dim > 0 && dir_is_set) {

				unsigned long int i = 0;

				// it is used as 0
				struct my_file temp;
				memset(&temp, 0, sizeof(struct my_file));

				while ( i < ejected_dim && memcmp(&ejected[i], &temp, sizeof(struct my_file)) != 0 ) {
					outcome.ejected = 1;
					outcome.filename = ejected[i].pathname;
					outcome.name_len = strlen(ejected[i].pathname) + 1;
					outcome.data = ejected[i].content;
					outcome.data_size = ejected[i].file_size;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						fmt = "Client %d CMD_APPEND_TO_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
                        for (unsigned long int j = i; j < ejected_dim; j++) {
                            free(ejected[j].pathname);
                            free(ejected[j].content);
                        }
                        free(ejected);
                        free(filepath);
						return -1;
					}
					memset(&outcome, 0, sizeof(outcome));
					i++;
				}

				outcome.ejected = 0;
				outcome.res = 0;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_APPEND_TO_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
                    for (unsigned long int j = i; j < ejected_dim; j++) {
                        free(ejected[j].pathname);
                        free(ejected[j].content);
                    }
                    free(ejected);
                    free(filepath);
					return -1;
				}

                while (i < ejected_dim) {
                    free(ejected[i].pathname);
                    free(ejected[i].content);
                }
                free(ejected);

			}

		} break;




		case CMD_LOCK_FILE: {

			if (!filepath) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "Fatal error: filepath was not specified");
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_LOCK_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
				return -1;
			}

			if ( !cache_search_file(cache_table, filepath) ) {

				// the file is neither in the files_list nor in the cache
				if (!files_list_search_file(get_client_list(client, conn_table), filepath,
                                            &(conn_table.clients[client].list->conn_list_mutex))) {

					fprintf(stderr, "The file %s on which the lock was requested by client [%d] does not exist on the server\n", filepath, client);
					outcome.res = -1;
					retval = send_outcome(client, outcome);
					if (retval == -1) {
						const char *fmt = "Client %d CMD_LOCK_FILE: Error while sending the outcome\n";
						write_log(my_log, fmt, client);
					}
                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
                    if (retval == -1) {
                        const char *fmt = "%s\n";
                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                    }
                    free(filepath);
					return -1;

				}

			}
            //the file is in the cache
            else {

                struct my_file *temp = cache_get_file(cache_table, filepath);

                if (temp->is_locked == true) {
                    if ( files_list_search_file(get_client_list(client, conn_table), filepath,
                                                &(conn_table.clients[client].list->conn_list_mutex)) ) {
                        outcome.res = 0;
                        retval = send_outcome(client, outcome);
                        if (retval == -1) {
                            const char *fmt = "Client %d CMD_LOCK_FILE: Error while sending the outcome\n";
                            write_log(my_log, fmt, client);
                            free(filepath);
                            return -1;
                        }
                        break;
                    }
                    else {

                        struct timespec ts;
                        ts.tv_sec = 100 / 1000;
                        ts.tv_nsec = (100 % 1000) * 1000000;
                        time_t start = time(NULL);

                        while (temp->is_locked == true && time(NULL) - start < 10) {
                            nanosleep(&ts, &ts);
                        }

                        if (time(NULL) - start < 3) {
	                        struct my_file *file = file_copy(temp);
	                        if (!file)
	                            return -1;

	                        conn_hash_table_add_file(client, &conn_table, file);
	                        file->is_locked = true;
	                        temp->is_locked = true;
	                        reference_file(&queue, &cache_table, temp, &ejected, &ejected_dim);

	                        outcome.res = 0;
	                        retval = send_outcome(client, outcome);
	                        if (retval == -1) {
	                            const char *fmt = "Client %d CMD_LOCK_FILE: Error while sending the outcome\n";
	                            write_log(my_log, fmt, client);
	                            free(filepath);
	                            return -1;
	                        }

	                        const char *fmt = "Client %d: The locked file is %s\n";
	                        write_log(my_log, fmt, client, filepath);
	                    }
	                    else {
	                    	printf("The file %s on which the lock was requested is owned by another client and the latter did not release it within 3 seconds\n", filepath);
	                    	outcome.res = -1;
							retval = send_outcome(client, outcome);
							if (retval == -1) {
								const char *fmt = "Client %d CMD_LOCK_FILE: Error while sending the outcome\n";
								write_log(my_log, fmt, client);
							}
		                    retval = writen(workers_pipe_wfd, &client, sizeof(int));
		                    if (retval == -1) {
		                        const char *fmt = "%s\n";
		                        write_log(my_log, fmt, "execute_request: Error sending client descriptor");
		                    }
		                    free(filepath);
							return -1;
			            }

                    }
                }
                else {

                    struct my_file *file = file_copy(temp);
                    if (!file)
                        return -1;

                    conn_hash_table_add_file(client, &conn_table, file);
                    temp->is_locked = true;
                    file->is_locked = true;
                    reference_file(&queue, &cache_table, temp, &ejected, &ejected_dim);

                    outcome.res = 0;
                    retval = send_outcome(client, outcome);
                    if (retval == -1) {
                        const char *fmt = "Client %d CMD_LOCK_FILE: Error while sending the outcome\n";
                        write_log(my_log, fmt, client);
                        free(filepath);
                        return -1;
                    }

                    const char *fmt = "Client %d: The locked file is %s\n";
                    write_log(my_log, fmt, client, filepath);

                }

            }

		} break;




		case CMD_UNLOCK_FILE: {

			if (!filepath) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "Fatal error: filepath was not specified");
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_UNLOCK_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
				return -1;
			}

            if (!cache_search_file(cache_table, filepath)) {
                fprintf(stderr, "The file %s on which the unlock was requested by client [%d] does not exist on the server\n", filepath, client);
                outcome.res = -1;
                retval = send_outcome(client, outcome);
                if (retval == -1) {
                    const char *fmt = "Client %d CMD_UNLOCK_FILE: Error while sending the outcome\n";
                    write_log(my_log, fmt, client);
                    return -1;
                }
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                free(filepath);
                return -1;
            }

			retval = files_list_search_file(get_client_list(client, conn_table), filepath,
                                            &(conn_table.clients[client].list->conn_list_mutex));
            if (retval == 1) {
                struct my_file *temp = cache_get_file(cache_table, filepath);
                conn_hash_table_remove_file(client, &conn_table, *temp);
                temp->is_locked = false;
                reference_file(&queue, &cache_table, temp, &ejected, &ejected_dim);
            }
			else {
				fprintf(stderr, "Error: the client %d that requested the lock on the file %s is not the owner of that file\n", client, filepath);
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					const char *fmt = "Client %d CMD_UNLOCK_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                free(filepath);
                return -1;
			}

			outcome.res = 0;
			retval = send_outcome(client, outcome);
			if (retval == -1) {
				const char *fmt = "Client %d CMD_UNLOCK_FILE: Error while sending the outcome\n";
				write_log(my_log, fmt, client);
                free(filepath);
				return -1;
			}

			const char *fmt = "Client %d: The unlocked file is %s\n";
			write_log(my_log, fmt, client, filepath);

		} break;




		case CMD_CLOSE_FILE: {

			if (!filepath) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "Fatal error: filepath was not specified");
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_CLOSE_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
				return -1;
			}

			if ( !files_list_search_file(flist, filepath, &flist_mutex) ) {

				fprintf(stderr, "The file %s on which the close was requested by client [%d] does not exist on the server\n", filepath, client);
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					const char *fmt = "Client %d CMD_CLOSE_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                free(filepath);
				return -1;

			}
			else {

				struct my_file *file = files_list_get_file(flist, filepath, &flist_mutex);

				files_list_remove(&flist, *file, &flist_mutex);
				outcome.res = 0;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					const char *fmt = "Client %d CMD_CLOSE_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
                    free(filepath);
					return -1;
				}

				const char *fmt = "Client %d: The closed file is %s\n";
				write_log(my_log, fmt, client, filepath);

			}

		} break;




		case CMD_REMOVE_FILE: {

			if (!filepath) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "Fatal error: filepath was not specified");
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					fmt = "Client %d CMD_REMOVE_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                free(filepath);
				return -1;
			}

			if ( !cache_search_file(cache_table, filepath) ) {
				fprintf(stderr, "The file %s on which the remove was requested by client [%d] does not exist on the server\n", filepath, client);
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					const char *fmt = "Client %d CMD_REMOVE_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                free(filepath);
				return -1;

			}

			struct my_file *file = cache_get_file(cache_table, filepath);

			if (file->is_locked == false) {
				fprintf(stderr, "The file %s on which the remove was requested by client [%d] is not locked\n", filepath, client);
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					const char *fmt = "Client %d CMD_REMOVE_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                free(filepath);
				return -1;
			}

			if ( file->is_locked == true && !files_list_search_file(get_client_list(client, conn_table), filepath,
                                                                    &(conn_table.clients[client].list->conn_list_mutex)) ) {
				fprintf(stderr, "The file %s on which the remove was requested by client [%d] is owned by another client\n", filepath, client);
				outcome.res = -1;
				retval = send_outcome(client, outcome);
				if (retval == -1) {
					const char *fmt = "Client %d CMD_REMOVE_FILE: Error while sending the outcome\n";
					write_log(my_log, fmt, client);
				}
                retval = writen(workers_pipe_wfd, &client, sizeof(int));
                if (retval == -1) {
                    const char *fmt = "%s\n";
                    write_log(my_log, fmt, "execute_request: Error sending client descriptor");
                }
                free(filepath);
				return -1;
			}

            conn_hash_table_remove_file(client, &conn_table, *file);
			cache_remove_file(&cache_table, &queue, filepath);

			outcome.res = 0;
			retval = send_outcome(client, outcome);
			if (retval == -1) {
				const char *fmt = "Client %d CMD_REMOVE_FILE: Error while sending the outcome\n";
				write_log(my_log, fmt, client);
                free(filepath);
				return -1;
			}

			const char *fmt = "Client %d: The removed file is %s\n";
				write_log(my_log, fmt, client, filepath);

		} break;



		default: {

			fprintf(stderr, "Unknown request from client [%d]\n", client);
			outcome.res = -1;
			retval = send_outcome(client, outcome);
			if (retval == -1) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "default: Error while sending the outcome");
			}
            free(filepath);
			return -1;
		}

	}

    // I tell the main thread to listen for this client's descriptor again
	retval = writen(workers_pipe_wfd, &client, sizeof(int));
	if (retval == -1) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "execute_request: Error sending client descriptor");
        free(filepath);
		return -1;
	}

    free(filepath);

	return 0;
}
