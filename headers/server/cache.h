#ifndef PROGETTOSOL_CACHE_H
#define PROGETTOSOL_CACHE_H

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include "configuration.h" 

struct my_file {

	unsigned long int fidx;
	unsigned long int file_size;
	bool is_locked;
	char *pathname;
	void *content;
	FILE *f;

};

struct cache_node {

	struct cache_node *prev,
				  	  *next;
	struct my_file *file;

};

struct cache_queue {

	pthread_mutex_t queue_mutex;
	unsigned long int max_capacity;
	unsigned long int curr_capacity;
	unsigned long int max_files;
	unsigned long int curr_files;
    unsigned long int num_ejected;
	struct cache_node *head,
		   			  *tail;

};

struct cache_hash_table {

	pthread_mutex_t table_mutex;
	unsigned long int capacity;
	struct cache_node **files_references;

};

extern struct cache_queue queue;
extern struct cache_hash_table cache_table;

int cache_initialization(server_configuration_t config, struct cache_hash_table *array, struct cache_queue *cqueue);
struct my_file *new_file(char *pathname, struct cache_hash_table table);
int is_queue_empty(struct cache_queue *cqueue);
int capacity_reached(struct cache_queue *cqueue);
int files_reached(struct cache_queue *cqueue);
struct cache_node* new_node(struct my_file *f);
int cache_remove_file(struct cache_hash_table *table, struct cache_queue *cqueue, char *filename);
int enqueue(struct cache_queue *cqueue, struct cache_hash_table *table, struct my_file *file,
            struct my_file **ejected, unsigned long int *ejected_dim);
void reference_file(struct cache_queue *cqueue, struct cache_hash_table *table, struct my_file *file,
                    struct my_file **ejected, unsigned long int *ejected_dim);
int cache_search_file(struct cache_hash_table table, char *filename);
struct my_file *cache_get_file(struct cache_hash_table table, char *filename);
int cache_file_append(struct cache_queue *cqueue, struct cache_hash_table *table, char *filename,
                      struct my_file **ejected, unsigned long int *ejected_dim, void *data, size_t data_size);
void cache_cleanup(struct cache_hash_table *array, struct cache_queue *cqueue);
void print_cache_data(struct cache_hash_table table);

#endif //PROGETTOSOL_CACHE_H
