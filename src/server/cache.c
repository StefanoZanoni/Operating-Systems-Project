#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "../../includes/util.h"
#include "../../includes/log.h"
#include "../../includes/configuration.h"
#include "../../includes/cache.h"

static atomic_fidx_t idx;

int cache_initialization(server_configuration_t config, hashtable_t *table, queue_t *queue) {

	queue->head = NULL;
	queue->tail = NULL;
	queue->max_capacity = config.storage_capacity;
	queue->curr_capacity = 0;
	queue->max_files = config.num_files;
	queue->curr_files = 0;
	queue->num_ejected = 0;
	pthread_mutex_init( &(queue->queue_mutex), NULL );

	table->capacity = queue->max_files;
	pthread_mutex_init( &(table->table_mutex), NULL );
	table->files_references = calloc(table->capacity, sizeof(node_t*));
	if (!table->files_references)
		return -1;

	pthread_mutex_init( &(idx.fidx_mutex), NULL );
	idx.index = 0;

	return 0;
}

int is_queue_empty(queue_t *queue) {

	pthread_mutex_lock( &(queue->queue_mutex) );

	int to_return = queue->head == NULL;

	pthread_mutex_unlock( &(queue->queue_mutex) );

	return to_return;
}

int capacity_reached(queue_t *queue) {

	pthread_mutex_lock( &(queue->queue_mutex) );

	int to_return = queue->curr_capacity >= queue->max_capacity;

	pthread_mutex_unlock( &(queue->queue_mutex) );

	return to_return;
}

int files_reached(queue_t *queue) {

	pthread_mutex_lock( &(queue->queue_mutex) );

	int to_return =  queue->curr_files >= queue->max_files;

	pthread_mutex_unlock( &(queue->queue_mutex) );

	return to_return;
	
}

node_t* new_node(myfile_t f) {

	node_t *temp = calloc(1, sizeof(node_t));
	if (!temp)
		return temp;

	temp->prev = NULL;
	temp->next = NULL;
	*(temp->file) = f;

	return temp;
}

void dequeue(queue_t *queue) {

	if ( is_queue_empty(queue) )
		return;

	pthread_mutex_lock( &(queue->queue_mutex) );

	//if there is only one node, then change head
	if (queue->head == queue->tail)
		queue->head = NULL;

	node_t *temp = queue->tail;
	unsigned long int size_to_remove = temp->file->file_size;
	queue->tail = queue->tail->prev;

	if (queue->tail)
		queue->tail->next = NULL;

	if (temp)
		free(temp);

	queue->curr_files--;
	queue->curr_capacity -= size_to_remove;

	pthread_mutex_unlock( &(queue->queue_mutex) );
}

int enqueue(queue_t *queue, hashtable_t *table, myfile_t file) {

	pthread_mutex_lock( &(table->table_mutex) );

	if ( capacity_reached(queue) || files_reached(queue) ) {
		table->files_references[file.fidx] = NULL;
		dequeue(queue);
	}

	pthread_mutex_unlock( &(table->table_mutex) );

	node_t *temp = new_node(file);
	if (!temp)
		return -1;

	pthread_mutex_lock( &(idx.fidx_mutex) );

	temp->file->fidx = idx.index;
	idx.index++;

	pthread_mutex_unlock( &(idx.fidx_mutex) );

	pthread_mutex_lock( &(queue->queue_mutex) );

	temp->next = queue->head;

	if ( is_queue_empty(queue) ) 
		queue->tail = queue->head = temp;
	else {
		queue->head->prev = temp;
		queue->head = temp;
	}

	pthread_mutex_lock( &(table->table_mutex) );

	table->files_references[file.fidx] = temp;

	pthread_mutex_unlock( &(table->table_mutex) );

	unsigned long int size_to_add = file.file_size;
	queue->curr_files++;
	queue->curr_capacity += size_to_add;

	pthread_mutex_unlock( &(queue->queue_mutex) );

	return 0;
}

void reference_file(queue_t *queue, hashtable_t *table, myfile_t file) {

	pthread_mutex_lock( &(table->table_mutex) );

	node_t *ref = table->files_references[file.fidx];

	pthread_mutex_unlock( &(table->table_mutex) );

	if (!ref) 
		enqueue(queue, table, file);

	pthread_mutex_lock( &(queue->queue_mutex) );

	if (ref != queue->head) {

		ref->prev->next = ref->next;
		if (ref->next)
			ref->next->prev = ref->prev;

		if (ref == queue->tail) {
			queue->tail = ref->prev;
			queue->tail->next = NULL;
		}

		ref->next = queue->head;
		ref->prev = NULL;

		ref->next->prev = ref;

		queue->head = ref;
	}

	pthread_mutex_unlock( &(queue->queue_mutex) );
}

void free_queue(queue_t *queue) {

	node_t *temp = queue->head;

	while (temp) {

		free(temp->file);
		if (temp->prev)
			free(temp->prev);
		temp = temp->next;
	}

	free(temp->file);
	free(temp);
	pthread_mutex_destroy( &(queue->queue_mutex) );
}

void free_hashtable(hashtable_t *hashtable) {

	free(hashtable->files_references);
	pthread_mutex_destroy( &(hashtable->table_mutex) );
}
