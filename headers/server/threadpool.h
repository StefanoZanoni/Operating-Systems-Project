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

struct tpool *tpool_create(size_t thread_num, pthread_t **threads);
int tpool_destroy(struct tpool *tp, int force, pthread_t *threads, size_t thread_num);
int tpool_add_work(struct tpool *tp, server_command_t request, int client, int workers_pipe_wfd);
void tpool_wait(struct tpool *tp);
int tpool_list_is_empty(struct tpool tp);

#endif //PROGETTOSOL_THREADPOOL_H