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

	unsigned long int max_capacity;
	unsigned long int curr_capacity;
	unsigned long int max_files;
	unsigned long int curr_files;
    unsigned long int num_ejected;
	struct cache_node *head,
		   			  *tail;

};

struct cache_hash_table {

	unsigned long int capacity;
	struct cache_node **files_references;

};

extern struct cache_queue queue;
extern struct cache_hash_table cache_table;

void print_cache_data(struct cache_hash_table table);

/**
 * This function is used to initialize the cache queue and the cache hash table
 *
 * @param config the configuration parameters
 * @param array hash table
 * @param cqueue queue
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int cache_initialization(server_configuration_t config, struct cache_hash_table *array, struct cache_queue *cqueue);

/**
 * This function is used to create a new file
 *
 * @param pathname the absolute path of that file
 * @param table cache hash table
 *
 * @return NULL error (errno set)
 * @return new success
 */
struct my_file *new_file(char *pathname, struct cache_hash_table table);

/**
 *  This function is used to allocate a new cache queue node
 *
 * @param f the file to insert into the node
 *
 * @return NULL error (errno set)
 * @return new success
 */
struct cache_node* new_node(struct my_file *f);

/**
 * This function is used to check if the cache is empty
 *
 * @param cqueue cache queue
 *
 * @return 1 if the cache is empty
 * @return 0 otherwise
 */
int is_queue_empty(struct cache_queue *cqueue);

/**
 * This function is used to check if the cache has reached the maximum storage capacity
 *
 * @param cqueue cache queue
 *
 * @return 1 if the cache has reached the maximum storage capacity
 * @return 0 otherwise
 */
int capacity_reached(struct cache_queue *cqueue);

/**
 * This function is used to check if the cache has reached the maximum files capacity
 *
 * @param cqueue cache queue
 *
 * @return 1 if the cache has reached the maximum files capacity
 * @return 0 otherwise
 */
int files_reached(struct cache_queue *cqueue);

/**
 * This function is used to insert a new file in the cache. If the cache has reached or will reach,
 * its maximum storage capacity or the maximum files capacity, then I remove the least recently used file.
 *
 * @param cqueue cache queue
 * @param table cache hash table
 * @param file the file to add
 * @param ejected the ejected files to store the new file
 * @param ejected_dim the dimension of ejected array
 *
 * @return -2 if the file is larger than the maximum cache storage capacity
 * @return -1 error (errno set)
 * @return 0 success
 */
int enqueue(struct cache_queue *cqueue, struct cache_hash_table *table, struct my_file *file,
            struct my_file **ejected, unsigned long int *ejected_dim);

/**
 * This function is responsible for the correctness of the LRU policy used for cache management.
 * Whenever a file is used by a client it is moved to the head of the queue. If a file is not yet
 * in the cache, I insert it
 *
 * @param cqueue cache queue
 * @param table cache hash table
 * @param file the file recently used
 * @param ejected the ejected array to pass to the enqueue function
 * @param ejected_dim the dimension of ejected array
 */
void reference_file(struct cache_queue *cqueue, struct cache_hash_table *table, struct my_file *file,
                    struct my_file **ejected, unsigned long int *ejected_dim);

/**
 * This function is used to search a file in the cache
 *
 * @param table cache hash table
 * @param filename the absolute path of the file to be searched for
 *
 * @return 1 if the file was found
 * @return 0 otherwise
 */
int cache_search_file(struct cache_hash_table table, char *filename);

/**
 * This function is used to retrieve a specific file from the cache
 *
 * @param table cache hash table
 * @param filename the absolute path of the file
 *
 * @return NULL error (errno set)
 * @return the searched file success
 */
struct my_file *cache_get_file(struct cache_hash_table table, char *filename);

/**
 * This function is used to clean up the cache
 *
 * @param array cache hash table
 * @param cqueue cache queue
 */
void cache_cleanup(struct cache_hash_table *array, struct cache_queue *cqueue);

/**
 * This function is used to append the data_size bytes to the file pointed to filename
 *
 * @param cqueue cache queue
 * @param table cache hash table
 * @param filename the absolute path of the file
 * @param ejected ejected files array
 * @param ejected_dim the dimension of ejected array
 * @param data the data to be appended to the file
 * @param data_size the size of that data
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int cache_file_append(struct cache_queue *cqueue, struct cache_hash_table *table, char *filename,
                      struct my_file **ejected, unsigned long int *ejected_dim, void *data, size_t data_size);

/**
 * This function is used to remove a specific file from the cache
 *
 * @param table cache hash table
 * @param queue cache queue
 * @param filename the absolute path of the file to be removed
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int cache_remove_file(struct cache_hash_table *table, struct cache_queue *queue, char *filename);

#endif //PROGETTOSOL_CACHE_H
