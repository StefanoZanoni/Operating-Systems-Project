#ifndef PROGETTOSOL_CONNECTIONS_H
#define PROGETTOSOL_CONNECTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "files_list.h"

#define INITIAL_CAPACITY 4096

struct conn_elem {

	int client;
	struct conn_flist *list;
	bool taken;

};

struct conn_hash_table {

	pthread_mutex_t table_mutex;

	unsigned long int dim;
	unsigned long int current_dim;
	struct conn_elem *clients;
	
};

extern struct conn_hash_table conn_table;

/**
 * This function is used to initialize the connections hash table
 *
 * @param table hash table
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int initialize_conn_hash_table(struct conn_hash_table *table);

/**
 * This function is used to insert a new client in the hash table
 *
 * @param client new client
 * @param table hash table
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int insert_client(int client, struct conn_hash_table *table);

/**
 * This function is used to remove a client previously connected
 *
 * @param table hash table
 * @param client client to remove
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int remove_client(struct conn_hash_table *table, int client);

/**
 * This function is used to add a file to the client's locked files list
 *
 * @param client the current client
 * @param table hash table
 * @param file the file to add
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int conn_hash_table_add_file(int client, struct conn_hash_table *table, struct my_file *file);

/**
 * This function is used to remove a file from the client's locked files list
 *
 * @param client the current client
 * @param table hash table
 * @param file the file to remove
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int conn_hash_table_remove_file(int client, struct conn_hash_table *table, struct my_file file);

/**
 * This function is used to clean up the connections hash table
 *
 * @param table hash table
 */
void conn_hash_table_cleanup(struct conn_hash_table *table);

/**
 * This function is used to get the client's locked files list
 *
 * @param client the current client
 * @param table hash table
 *
 * @return NULL error (errno set)
 * @return current client's locked files list success
 */
struct references_list *get_client_list(int client, struct conn_hash_table table);

#endif //PROGETTOSOL_CONNECTIONS_H
