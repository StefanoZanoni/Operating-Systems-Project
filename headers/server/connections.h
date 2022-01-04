#ifndef PROGETTOSOL_CONNECTIONS_H
#define PROGETTOSOL_CONNECTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "files_list.h"

#define INITIAL_CAPACITY 256

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

int initialize_conn_hash_table(struct conn_hash_table *table);
int insert_client(int client, struct conn_hash_table *table);
int remove_client(struct conn_hash_table *table, int client);
int conn_hash_table_add_file(int client, struct conn_hash_table *table, struct my_file *file);
int conn_hash_table_remove_file(int client, struct conn_hash_table *table, struct my_file file);
void conn_hash_table_cleanup(struct conn_hash_table *table);
struct references_list *get_client_list(int client, struct conn_hash_table table);

#endif //PROGETTOSOL_CONNECTIONS_H