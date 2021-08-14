#ifndef PROGETTOSOL_DATASTRUCTURES_H
#define PROGETTOSOL_DATASTRUCTURES_H

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include "configuration.h" 

typedef struct atomic_fidx {

	pthread_mutex_t fidx_mutex;
	unsigned long int index;

} atomic_fidx_t;

typedef struct my_file {

	unsigned long int fidx;
	unsigned long int file_size;
	bool is_locked;
	char *pathname;
	void *content;

} myfile_t;

typedef struct mynode {

	struct node *prev,
				*next;
	myfile_t *file;

} node_t;

typedef struct myqueue {

	pthread_mutex_t queue_mutex;
	unsigned long int max_capacity;
	unsigned long int curr_capacity;
	unsigned long int max_files;
	unsigned long int curr_files;
	unsigned long int num_ejected;
	node_t *head,
		   *tail;

} queue_t;

typedef struct hashtable {

	pthread_mutex_t table_mutex;
	unsigned long int capacity;
	node_t **files_references;

} hashtable_t;

void cache_initialization(server_configuration_t config, hashtable_t *table, queue_t *queue);
int is_queue_empty(queue_t *queue);
int capacity_reached(queue_t *queue);
int files_reached(queue_t *queue);
node_t* new_node(myfile_t f);
void dequeue(queue_t *queue);
void enqueue(queue_t *queue, hashtable_t *table, myfile_t file);
void enqueue(queue_t* queue, hashtable_t* table, myfile_t file);

#endif //PROGETTOSOL_LOG_H