#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include "../../common/util.h"
#include "../../headers/server/log.h"
#include "../../headers/server/threadpool.h"
#include "../../headers/server/requests.h"

static struct tpool_work *tpool_work_create(server_command_t request, int client, int workers_pipe_wfd) {

	struct tpool_work *work;

	work = calloc(1, sizeof(struct tpool_work));
	if (!work) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "tpool_work_create: Error in work allocation");
		return NULL;
	}

	work->f = (worker_func_t) &execute_request;
	work->request = request;
	work->client = client;
	work->workers_pipe_wfd = workers_pipe_wfd;
	work->next = NULL;

	return work;
}

static struct tpool_work *tpool_work_get(struct tpool *tp) {

	struct tpool_work *work;

	work = tp->first_work;
	if (!work) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "Error in tpool_work_get: there is no work to get");
		return NULL;
	}

	if (!work->next) {
		tp->first_work = NULL;
		tp->last_work = NULL;
	}
	else {
		tp->first_work = work->next;
	}

	return work;

}

/**
 * This function is performed by each thread of the threadpool
 *
 * @param arg threadpool
 *
 * @return NULL
 */
static void *tpool_worker(void *arg) {

	struct tpool *tp = (struct tpool*) arg;
	struct tpool_work *work = NULL;

	while (1) {

		LOCK(&(tp->work_mutex))

		// wait for a job to be performed or for the exit to be notified 
		while (tp->first_work == NULL && !tp->stop) 
			WAIT(&(tp->work_cond), &(tp->work_mutex))
		
		// forced exit
		if (tp->stop > 1)
			break;

		// exit only if there are no other works to be carried out
		if (tp->stop == 1 && tp->current_work > tp->list_length)
			break;

		work = tpool_work_get(tp);
		if (!work) {
			long tid = (long) pthread_self();
			const char *fmt = "Thread [%ld]: error while retrieving work\n";
			write_log(my_log, fmt, tid);
		}
		tp->working_count++;
		tp->current_work++;

		UNLOCK(&(tp->work_mutex))

		// execution of parallel works
		if (work != NULL) {
            long tid = (long) pthread_self();
			if (work->f(work->request, work->client, work->workers_pipe_wfd) == -1) {
				const char *fmt = "Thread [%ld]: error while executing command no. %d on client %d\n";
				write_log(my_log, fmt, tid, (work->request).cmd, work->client);
			}
            const char *fmt = "Thread [%ld]: command no. %d executed on client %d\n";
            write_log(my_log, fmt, tid, (work->request).cmd, work->client);
            free(work->request.filepath);
            if (work->request.data)
                free(work->request.data);
            memset(&work->request, 0, sizeof(server_command_t));
			free(work);
			work = NULL;
		}

		LOCK(&(tp->work_mutex))

		tp->working_count--;

		if (!tp->stop && tp->working_count == 0 && tp->first_work == NULL)
			SIGNAL(&(tp->working_cond))

		UNLOCK(&(tp->work_mutex))

	}

	tp->thread_count--;
	SIGNAL(&(tp->working_cond))

	UNLOCK(&(tp->work_mutex))

	return NULL;

}

struct tpool *tpool_create(size_t thread_num, pthread_t **threads) {

	struct tpool *tp;
	size_t i;

	if (thread_num <= 0)
		thread_num = 2;

	tp = calloc(1, sizeof(struct tpool));
	if (!tp) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "tpool_create: Error in tp allocation");
		return NULL;
	}

	tp->thread_count = thread_num;
	pthread_mutex_init(&(tp->work_mutex), NULL);
	pthread_cond_init(&(tp->work_cond), NULL);
	pthread_cond_init(&(tp->working_cond), NULL);

	for (i = 0; i < thread_num; i++) 
		pthread_create(&(*threads)[i], NULL, tpool_worker, tp);

	return tp;

}

void tpool_wait(struct tpool *tp) {

	if (!tp)
		return;

	LOCK(&(tp->work_mutex))

	while (1) {

		if ( (tp->stop && tp->working_count > 0) || (tp->stop && tp->thread_count > 0) ) {
			WAIT(&(tp->working_cond), &(tp->work_mutex))
		}
		else
			break;

	}

	UNLOCK(&(tp->work_mutex))
}

int tpool_destroy(struct tpool *tp, int force, pthread_t *threads, size_t thread_num) {

	if (!tp || force < 0) {
		errno = EINVAL;
		const char *fmt = "%s tp = %p, force = %d\n";
		write_log(my_log, fmt, "Error in tpool_destroy parameters:", tp, force);
		return -1;
	}

	struct tpool_work *work;
	struct tpool_work *aux_work;

	LOCK(&(tp->work_mutex))

	tp->stop = 1 + force;

	if (tp->stop > 1) {
		work = tp->first_work;
		while (work != NULL) {
			aux_work = work->next;
			free(work);
			work = aux_work;
		}
	}
	BCAST(&(tp->work_cond))

	UNLOCK(&(tp->work_mutex))

	tpool_wait(tp);

	for (int i = 0; i < thread_num; i++) 
		pthread_join(threads[i], NULL);
			
	pthread_mutex_destroy(&(tp->work_mutex));
	pthread_cond_destroy(&(tp->work_cond));
	pthread_cond_destroy(&(tp->working_cond));

	free(tp);

	return 0;

}

int tpool_add_work(struct tpool *tp, server_command_t request, int client, int workers_pipe_wfd) {

	if (!tp)
		return -1;

	struct tpool_work *work;

	work = tpool_work_create(request, client, workers_pipe_wfd);
	if (!work) 
		return -1;

	LOCK(&(tp->work_mutex))

	if (tp->first_work == NULL) {
		tp->first_work = work;
		tp->last_work = tp->first_work;
	}
	else {
		tp->last_work->next = work;
		tp->last_work = work;
	}
	tp->list_length++;
	BCAST(&(tp->work_cond))

	UNLOCK(&(tp->work_mutex))

	return 0;

}

int tpool_list_is_empty(struct tpool tp) {

	if (tp.first_work == NULL && tp.last_work == NULL)
		return 1;

	return 0;
	
}
