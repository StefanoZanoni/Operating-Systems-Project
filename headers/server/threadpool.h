#ifndef PROGETTOSOL_THREADPOOL_H
#define PROGETTOSOL_THREADPOOL_H

#include "../../common/client_server.h"

typedef int (*worker_func_t)(server_command_t, int , int);

struct tpool_work {

	worker_func_t f; 				// the function to be executed
	server_command_t request; 		// the request received by the client
	int client; 					// the client who has requested this specific operation
	int workers_pipe_wfd; 			// the pipe on which the client descriptor is sent back to main
	struct tpool_work *next; 		// the next work to be executed

} ;

struct tpool {

	struct tpool_work *first_work;	// the first work in the list
	struct tpool_work *last_work;	// the last work in the list
	pthread_mutex_t work_mutex;		// a mutex used to lock the entire threadpool
	pthread_cond_t work_cond;		// a conditional used to signal the threads that there is work to be processed
	pthread_cond_t working_cond;	// a conditional used to signal when there are no threads processing
	size_t working_count;			// how many threads are actively processing work
	size_t thread_count;			// how many threads are alive
	unsigned int current_work;		// it is used to keep track of any remaining jobs
	size_t list_length;				// number of works to be carried out
	int stop;						// flag indicating termination

} ;

/**
 * This function is used to create a work for the threadpool
 *
 * @param thread_num number of threads
 * @param threads pthread_t vector to store all thread IDs
 *
 * @return NULL error (errno set)
 * @return work success
 */
struct tpool *tpool_create(size_t thread_num, pthread_t **threads);

/**
 * This function is used to clean up the threadpool
 *
 * @param tp threadpool
 * @param force exit status
 * @param threads thread IDs
 * @param thread_num number of threads
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int tpool_destroy(struct tpool *tp, int force, pthread_t *threads, size_t thread_num);

/**
 * This function is used to add a work to the threadpool
 *
 * @param tp threadpool
 * @param request the current request
 * @param client the client that made the current request
 * @param workers_pipe_wfd file descriptor of the pipe write entry
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int tpool_add_work(struct tpool *tp, server_command_t request, int client, int workers_pipe_wfd);

/**
 * This function is used to wait the termination of all threads
 *
 * @param tp threadpool
 */
void tpool_wait(struct tpool *tp);

/**
 * This function is used to check if the works list is empty
 *
 * @param tp threadpool
 *
 * @return 1 if the list is empty
 * @return 0 otherwise
 */
int tpool_list_is_empty(struct tpool tp);

#endif //PROGETTOSOL_THREADPOOL_H
