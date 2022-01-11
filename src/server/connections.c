#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "../../headers/server/log.h"
#include "../../headers/server/connections.h"
#include "../../common/util.h"

static unsigned int conn_hash(unsigned int x) {

	x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;

    return x;
}

int initialize_conn_hash_table(struct conn_hash_table *table) {

	if (!table) {
		errno = EINVAL;
		const char *fmt = "%s table = %p\n";
		write_log(my_log, fmt, "Error in initialize_hash_table parameters:", table);
		return -1;
	}

    pthread_mutex_init(&(table->table_mutex), NULL);
	table->dim = INITIAL_CAPACITY;
	table->current_dim = 0;
	table->clients = calloc(table->dim, sizeof(struct conn_elem));
	if (!table->clients) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "initialize_hash_table: Error in table->clients allocation");
		return -1;
	}
	for (int i = 0; i < table->dim; i++) {
		table->clients[i].list = calloc(1, sizeof(struct conn_flist));
        if (!table->clients[i].list) {
            const char *fmt = "%s\n";
            write_log(my_log, fmt, "initialize_conn_hash_table: Error in table->clients[i].list allocation");
            return -1;
        }
		pthread_mutex_init( &(table->clients[i].list->conn_list_mutex), NULL );
	}

	return 0;

}

/**
 * This function is used to reallocate the connections hash table in case the connected clients
 * exceed the actual maximum client capacity of the table
 *
 * @param table hash table
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
static int conn_hash_table_realloc(struct conn_hash_table *table) {

	struct conn_hash_table *temp = calloc(1, sizeof(struct conn_hash_table));
	if (!temp) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "conn_hash_table_realloc: Error in temp allocation");
		return -1;
	}

	temp->dim = table->dim;
	temp->current_dim = table->current_dim;
	for (int i = 0; i < temp->dim; i++) {
		memcpy(temp->clients[i].list, table->clients[i].list, sizeof(struct conn_flist));
		temp->clients[i].client = table->clients[i].client;
		temp->clients[i].taken = table->clients[i].taken;
	}

	conn_hash_table_cleanup(table);

	table->dim *= 2;
	table->current_dim = 0;
	table->clients = calloc(table->dim, sizeof(struct conn_elem));

	for (unsigned long int i = 0; i < temp->dim; i++) {
			unsigned long int index = conn_hash(temp->clients[i].client) % table->dim;
			memcpy( &(table->clients[index]), &(temp->clients[i]), sizeof(struct conn_elem) );
	}
	for (unsigned long int i = temp->dim; i < table->dim; i++) {
        table->clients[i].list = calloc(1, sizeof(struct conn_flist));
		pthread_mutex_init(&(table->clients[i].list->conn_list_mutex), NULL);
	}

	conn_hash_table_cleanup(temp);

	return 0;

}

int insert_client(int client, struct conn_hash_table *table) {

	if (client < 0 || !table) {
		errno = EINVAL;
		const char *fmt = "%s client = %d, table = %p\n";
		write_log(my_log, fmt, "Error in hash_table_insert_client parameters:", client, table);
		return -1;
	}

	LOCK( &(table->table_mutex) )

    if ( (double) table->current_dim / (double) table->dim > 0.75 ) {
        int retval = conn_hash_table_realloc(table);
        if (retval == -1) {
            UNLOCK( &(table->table_mutex) )
            return -1;
        }
    }

    unsigned int index = conn_hash(client) % table->dim;
    while (table->clients[index].taken == true) {
        if (index == table->dim - 1) {
            index = 0;
            continue;
        }
        index++;
    }
    table->clients[index].client = client;
    table->clients[index].taken = true;
    table->current_dim++;

	UNLOCK( &(table->table_mutex) )

	return 0;

}

int remove_client(struct conn_hash_table *table, int client) {

	if (client < 0 || !table) {
		errno = EINVAL;
		const char *fmt = "%s client = %d, table = %p\n";
		write_log(my_log, fmt, "Error in hash_table_insert_client parameters:", client, table);
		return -1;
	}

	LOCK( &(table->table_mutex) )

	unsigned int index = conn_hash( (unsigned int) client ) % table->dim;
    while (table->clients[index].client != client) {
        if (index == table->dim - 1) {
            index = 0;
            continue;
        }
        index++;
    }
	table->clients[index].taken = false;
    table->clients[index].client = -1;
	files_list_cleanup(&(table->clients[index].list->clist), &(table->clients[index].list->conn_list_mutex));
    pthread_mutex_init(&(table->clients[index].list->conn_list_mutex), NULL);

	UNLOCK( &(table->table_mutex) )

	return 0;
}

int conn_hash_table_add_file(int client, struct conn_hash_table *table, struct my_file *file) {

    if (client < 0 || !table || !file) {
        errno = EINVAL;
        const char *fmt = "%s client = %d, table = %p, file = %p\n";
        write_log(my_log, fmt, "Error in add_file parameters", client, table, file);
        return -1;
    }

    LOCK( &(table->table_mutex) )

    unsigned int index = conn_hash(client) % table->dim;
    while (table->clients[index].client != client) {
        if (index == table->dim - 1) {
            index = 0;
            continue;
        }
        index++;
    }

    files_list_add(&(table->clients[index].list->clist), file, &(table->clients[index].list->conn_list_mutex));

    UNLOCK( &(table->table_mutex) )

    return 0;
}

int conn_hash_table_remove_file(int client, struct conn_hash_table *table, struct my_file file) {

	if (client < 0 || !table) {
		errno = EINVAL;
		const char *fmt = "%s client = %d, table = %p\n";
		write_log(my_log, fmt, "Error in add_file parameters", client, table);
		return -1;
	}

	LOCK( &(table->table_mutex) )

	unsigned int index = conn_hash(client) % table->dim;
    while (table->clients[index].client != client) {
        if (index == table->dim - 1) {
            index = 0;
            continue;
        }
        index++;
    }

	files_list_remove(&(table->clients[index].list->clist), file, &(table->clients[index].list->conn_list_mutex));

	UNLOCK( &(table->table_mutex) )

	return 0;
}

void conn_hash_table_cleanup(struct conn_hash_table *table) {

	for (int i = 0; i < table->dim; i++) {
		if (table->clients[i].list) {
			files_list_cleanup(&(table->clients[i].list->clist), &(table->clients[i].list->conn_list_mutex));
			free(table->clients[i].list);
			table->clients[i].list = NULL;
		}
	}
	free(table->clients);
	pthread_mutex_destroy( &(table->table_mutex) );

}

struct references_list *get_client_list(int client, struct conn_hash_table table) {

    if (client < 0 ) {
        errno = EINVAL;
        const char *fmt = "%s client = %d";
        write_log(my_log, fmt, "Error in hash_table_insert_client parameters:", client);
        return NULL;
    }

    LOCK( &(table.table_mutex) )

    unsigned int index = conn_hash(client) % table.dim;
    while (table.clients[index].client != client) {
        if (index == table.dim - 1) {
            index = 0;
            continue;
        }
        index++;
    }

    UNLOCK( &(table.table_mutex) )

    return table.clients[index].list->clist;
}
