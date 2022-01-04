#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#include "../../common/util.h"
#include "../../headers/server/log.h"
#include "../../headers/server/configuration.h"
#include "../../headers/server/cache.h"

static unsigned long int max_size = 0;
static unsigned long int num_max_files = 0;
static unsigned long int num_replacements = 0;

void print_cache_data(struct cache_hash_table table) {

    double m_size = (double) max_size / 1000000;
	printf("Maximum number of files stored on the server: %ld\n", num_max_files);
	printf("Maximum size reached by the server: %.3f MB\n", m_size);
	printf("Number of times the cache replacement algorithm was run: %ld\n", num_replacements);
	printf("List of files still contained in the server:\n");
	for (int i = 0; i < table.capacity; i++) {
		if (table.files_references[i] != NULL)
			printf("%s\n", table.files_references[i]->file->pathname);
	}

}

unsigned long cache_hash(unsigned char *filename) {

	unsigned long int hash = 5381;
	int c;

	while ( (c = *filename++) ) 
		hash = ( (hash << 5) + hash) + c;

	return hash;
	
}

int cache_initialization(server_configuration_t config, struct cache_hash_table *table,
                         struct cache_queue *cqueue) {

	if (!table || !cqueue) {
		errno = EINVAL;
		const char *fmt = "%s table = %p, cqueue = %p\n";
		write_log(my_log, fmt, "Error in cache_initialization parameters:", table, cqueue);
		return -1;
	}

    cqueue->head = NULL;
    cqueue->tail = NULL;
    cqueue->max_capacity = config.storage_capacity;
    cqueue->curr_capacity = 0;
    cqueue->max_files = config.num_files;
    cqueue->curr_files = 0;
    cqueue->num_ejected = 0;
	pthread_mutex_init(&(cqueue->queue_mutex), NULL );

	table->capacity = config.num_files;
	pthread_mutex_init( &(table->table_mutex), NULL );
	table->files_references = calloc(config.num_files, sizeof(struct cache_node*));
	if (!table->files_references) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "cache_initialization: Error in table->files_references allocation");
		return -1;
	}

	return 0;
}

struct my_file *new_file(char *pathname, struct cache_hash_table table) {

	struct my_file *new = calloc(1, sizeof(struct my_file));
	if (!new) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "new_file: Error in new allocation");
		return NULL;
	}

	new->pathname = calloc(strlen(pathname) + 1, sizeof(char));
    if (!new->pathname) {
        const char *fmt = "%s\n";
        write_log(my_log, fmt, "new_file: Error in new->pathname allocation");
        return NULL;
    }
    new->pathname = strcpy(new->pathname, pathname);
	new->is_locked = false;

	LOCK ( &(table.table_mutex) )

	new->fidx = cache_hash( (unsigned char*) pathname ) % table.capacity;
	while (table.files_references[new->fidx] != NULL) {
		if (new->fidx == table.capacity -1) {
			new->fidx = 0;
			continue;
		}
		new->fidx++;
	}

	UNLOCK ( &(table.table_mutex) )

	new->f = fopen(pathname, "a+b");
	if (!new->f) {
		const char *fmt = "%s %s\n";
		write_log(my_log, fmt, "new_file: Error while opening", pathname);
		free(new);
		return NULL;
	}

	fseek(new->f, 0, SEEK_END);
	new->file_size = ftell(new->f);
	fseek(new->f, 0, SEEK_SET);
	new->content = (char*) calloc(new->file_size, sizeof(char));
	if (!new->content) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "new_file: Error in new->content allocation");
		fclose(new->f);
		free(new);
		return NULL;
	}
	if (fread(new->content, 1, new->file_size, new->f) < new->file_size) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "new_file: Error in new->content reading");
		fclose(new->f);
		free(new);
		return NULL;
	}

	return new;

}

struct cache_node *new_node(struct my_file *f) {

	struct cache_node *temp = calloc(1, sizeof(struct cache_node));
	if (!temp) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "new_node: Error in temp allocation");
		return NULL;
	}

	temp->prev = NULL;
	temp->next = NULL;
	temp->file = f;

	return temp;
}

int is_queue_empty(struct cache_queue *cqueue) {

	return cqueue->head == NULL;

}

int capacity_reached(struct cache_queue *cqueue) {

	return cqueue->curr_capacity >= cqueue->max_capacity;

}

int files_reached(struct cache_queue *cqueue) {

	return cqueue->curr_files >= cqueue->max_files;
	
}

static unsigned long int dequeue(struct cache_queue *cqueue) {

    LOCK( &(cqueue->queue_mutex) )

	if ( is_queue_empty(cqueue) ) {
        UNLOCK( &(cqueue->queue_mutex) )
        return -1;
    }

	// if there is only one node, then change head
	if (cqueue->head == cqueue->tail)
        cqueue->head = NULL;

	struct cache_node *temp = cqueue->tail;
	unsigned long int size_to_remove = temp->file->file_size;
    cqueue->tail = cqueue->tail->prev;

	if (cqueue->tail)
        cqueue->tail->next = NULL;

	unsigned long int fidx = temp->file->fidx;

	const char *fmt = "The file %s has been expelled\n";
	write_log(my_log, fmt, temp->file->pathname);

	cqueue->curr_files--;
    cqueue->curr_capacity -= size_to_remove;

	UNLOCK( &(cqueue->queue_mutex) )

	return fidx;
}

int enqueue(struct cache_queue *cqueue, struct cache_hash_table *table, struct my_file *file,
            struct my_file **ejected, unsigned long int *ejected_dim) {

	if (!cqueue || !table || !file || !ejected || !ejected_dim) {
		const char *fmt = "%s cqueue = %p, table = %p, file = %p, ejected = %p, ejected_dim = %p\n";
		write_log(my_log, fmt, "Error in enqueue parameters:", cqueue, table, file, ejected, ejected_dim);
		return -1;
	}

	unsigned long int num_ejected = 0;

	LOCK( &(cqueue->queue_mutex) )

	if (file->file_size > cqueue->max_capacity) {
		fprintf(stderr,
                "Error: the file to be inserted in the server exceeds the maximum capacity of the latter\n");
        UNLOCK( &(cqueue->queue_mutex) )
		return -1;
	}

	LOCK( &(table->table_mutex) )

	while (capacity_reached(cqueue) || files_reached(cqueue)
           || (cqueue->curr_capacity + file->file_size > cqueue->max_capacity) ) {

        UNLOCK( &(cqueue->queue_mutex) )

		unsigned long int available;

		const char *fmt = "%s\n";
		write_log(my_log, fmt, "The cache replacement algorithm has started");

		if ( (available = dequeue(cqueue)) == -1 ) {
			fmt = "%s\n";
			write_log(my_log, fmt, "enqueue: Error during replacement");
			UNLOCK( &(table->table_mutex) )
			return -1;
		}
		else {

            table->files_references[available]->file->is_locked = false;

			num_ejected++;

			if (num_ejected > *ejected_dim) {

				if (*ejected_dim == 0)
					*ejected_dim = 1;
				else
					*ejected_dim *= 2;

                if (!*ejected) {
                    *ejected = calloc(*ejected_dim, sizeof(struct my_file));
                    if (!*ejected) {
                        fmt = "%s\n";
                        write_log(my_log, fmt, "enqueue: Error in *ejected allocation");
                        UNLOCK( &(table->table_mutex) )
                        return -1;
                    }
                }
                else {

                    struct my_file *temp = *ejected;
                    *ejected = calloc(*ejected_dim, sizeof(struct my_file));
                    if (!*ejected) {
                        fmt = "%s\n";
                        write_log(my_log, fmt, "enqueue: Error in *ejected re-allocation");
                        UNLOCK( &(table->table_mutex) )
                        for (unsigned long int i = 0; i < *ejected_dim / 2; i++) {
                            free(temp[i].pathname);
                            free(temp[i].content);
                        }
                        free(temp);
                        return -1;
                    }

                    for (unsigned long int i = 0; i < *ejected_dim / 2; i++) {
                        (*ejected)[i].fidx = temp[i].fidx;
                        (*ejected)[i].file_size = temp[i].file_size;
                        (*ejected)[i].is_locked = temp[i].is_locked;
                        (*ejected)[i].pathname = calloc(strlen(temp[i].pathname) + 1, sizeof(char));
                        if (!(*ejected)[i].pathname) {
                            fmt = "%s\n";
                            write_log(my_log, fmt, "enqueue: Error in (*ejected)[i].pathname allocation");
                            UNLOCK(&(table->table_mutex))
                            for (unsigned long int j = 0; j < i; j++) {
                                free( (*ejected)[i].pathname );
                                free( (*ejected)[i].content );
                            }
                            free(*ejected);
                            for (unsigned long int j = i; j < *ejected_dim / 2; j++) {
                                free(temp[j].pathname);
                                free(temp[j].content);
                            }
                            free(temp);
                            return -1;
                        }
                        (*ejected)[i].content = (char*) calloc(temp[i].file_size, sizeof(char));
                        if (!(*ejected)[i].content) {
                            fmt = "%s\n";
                            write_log(my_log, fmt, "enqueue: Error in (*ejected)[i].content allocation");
                            UNLOCK(&(table->table_mutex))
                            for (unsigned long int j = 0; j < i; j++) {
                                free( (*ejected)[i].pathname );
                                free( (*ejected)[i].content );
                            }
                            free( (*ejected)[i].pathname);
                            free(*ejected);
                            for (unsigned long int j = i; j < *ejected_dim / 2; j++) {
                                free(temp[j].pathname);
                                free(temp[j].content);
                            }
                            free(temp);
                            return -1;
                        }
                        strcpy( (*ejected)[i].pathname, temp[i].pathname );
                        memcpy( (*ejected)[i].content, temp[i].content, temp[i].file_size );
                        (*ejected)[i].f = temp[i].f;

                        free(temp[i].pathname);
                        temp[i].pathname = NULL;
                        free(temp[i].content);
                        temp[i].content = NULL;
                    }
                    free(temp);

                }

			}

            (*ejected)[num_ejected - 1].pathname = calloc(strlen(table->files_references[available]->file->pathname) + 1,
                                                          sizeof(char));
            if (!(*ejected)[num_ejected - 1].pathname) {
                fmt = "%s\n";
                write_log(my_log, fmt, "enqueue: Error in (*ejected)[num_ejected - 1].pathname allocation");
                UNLOCK( &(table->table_mutex) )
                for (unsigned long int i = 0; i < num_ejected - 1; i++) {
                    free((*ejected)[i].pathname);
                    free((*ejected)[i].content);
                }
                free(ejected);
                return -1;
            }
            (*ejected)[num_ejected - 1].content = (char*) calloc(table->files_references[available]->file->file_size,
                                                                 sizeof(char));
            if (!(*ejected)[num_ejected - 1].content) {
                fmt = "%s\n";
                write_log(my_log, fmt, "enqueue: Error in (*ejected)[num_ejected - 1].content allocation");
                UNLOCK( &(table->table_mutex) )
                free((*ejected)[num_ejected - 1].pathname);
                for (unsigned long int i = 0; i < num_ejected - 1; i++) {
                    free((*ejected)[i].pathname);
                    free((*ejected)[i].content);
                }
                free(ejected);
                return -1;
            }
            (*ejected)[num_ejected - 1].fidx = table->files_references[available]->file->fidx;
            (*ejected)[num_ejected - 1].file_size = table->files_references[available]->file->file_size;
            (*ejected)[num_ejected - 1].is_locked = table->files_references[available]->file->is_locked;
            strcpy((*ejected)[num_ejected - 1].pathname, table->files_references[available]->file->pathname);
            (*ejected)[num_ejected - 1].f = table->files_references[available]->file->f;
            memcpy((*ejected)[num_ejected - 1].content, table->files_references[available]->file->content,
                   table->files_references[available]->file->file_size);

            free(table->files_references[available]->file->pathname);
			free(table->files_references[available]->file->content);
			fclose(table->files_references[available]->file->f);
            free(table->files_references[available]->file);
			free(table->files_references[available]);
			table->files_references[available] = NULL;

			num_replacements++;

		}

        LOCK( &(cqueue->queue_mutex) )

	}

    UNLOCK( &(cqueue->queue_mutex) )
	UNLOCK( &(table->table_mutex) )

	struct cache_node *temp = new_node(file);
	if (!temp) {
		const char *fmt = "%s\n";
		write_log(my_log, fmt, "enqueue: Error while creating the new node");
		return -1;
	}

	LOCK( &(cqueue->queue_mutex) )

	temp->next = cqueue->head;

	if ( is_queue_empty(cqueue) )
        cqueue->tail = cqueue->head = temp;
	else {
        cqueue->head->prev = temp;
        cqueue->head = temp;
	}

	LOCK( &(table->table_mutex) )

	table->files_references[file->fidx] = temp;

	UNLOCK( &(table->table_mutex) )

	unsigned long int size_to_add = file->file_size;
	cqueue->curr_files++;
    cqueue->curr_capacity += size_to_add;
	if (cqueue->curr_files > num_max_files)
		num_max_files = cqueue->curr_files;
	if (cqueue->curr_capacity > max_size)
		max_size = cqueue->curr_capacity;

	UNLOCK( &(cqueue->queue_mutex) )

	return 0;
}

void reference_file(struct cache_queue *cqueue, struct cache_hash_table *table, struct my_file *file,
                    struct my_file **ejected, unsigned long int *ejected_dim) {

	LOCK( &(table->table_mutex) )

	struct cache_node *ref = table->files_references[file->fidx];

	UNLOCK( &(table->table_mutex) )

	if (!ref) 
		enqueue(cqueue, table, file, ejected, ejected_dim);

	LOCK( &(cqueue->queue_mutex) )

	if (ref != cqueue->head) {

		ref->prev->next = ref->next;
		if (ref->next)
			ref->next->prev = ref->prev;

		if (ref == cqueue->tail) {
            cqueue->tail = ref->prev;
            cqueue->tail->next = NULL;
		}

		ref->next = cqueue->head;
		ref->prev = NULL;

		ref->next->prev = ref;
        cqueue->head = ref;
	}

	UNLOCK( &(cqueue->queue_mutex) )
}

int cache_search_file(struct cache_hash_table table, char *filename) {

	LOCK( &(table.table_mutex) )

	unsigned long int fidx = cache_hash( (unsigned char*) filename) % table.capacity;

	int i = 0;

	while (i < table.capacity) {

        // this loop is used when the cache is empty
        while (!table.files_references[fidx] && i < table.capacity) {
            if (fidx == table.capacity - 1) {
                fidx = 0;
                i++;
                continue;
            }
            fidx++;
            i++;
        }
        if (i == table.capacity) {
            UNLOCK ( &(table.table_mutex) )
            return 0;
        }

		if ( strcmp(table.files_references[fidx]->file->pathname, filename) == 0) {
            UNLOCK ( &(table.table_mutex) )
            return 1;
        }
		else {
			if (fidx == table.capacity - 1) {
				fidx = 0;
				i++;
				continue;
			}
			fidx++;
			i++;
		}

	}

	UNLOCK( &(table.table_mutex) )

	return 0;
}

struct my_file *cache_get_file(struct cache_hash_table table, char *filename) {

	if (!filename) {
		errno = EINVAL;
		const char *fmt = "%s filename = %p\n";
		write_log(my_log, fmt, "Error in cache_get_file parameters:");
		return NULL;
	}

	LOCK( &(table.table_mutex) )

	unsigned long int fidx = cache_hash( (unsigned char*) filename) % table.capacity;
    while (strcmp(table.files_references[fidx]->file->pathname, filename) != 0) {
        if (fidx == table.capacity - 1) {
            fidx = 0;
            continue;
        }
        fidx++;
    }

    struct my_file *temp = table.files_references[fidx]->file;

	UNLOCK( &(table.table_mutex) )

	return temp;
}

void cache_cleanup(struct cache_hash_table *table, struct cache_queue *cqueue) {

	LOCK( &(cqueue->queue_mutex) )

	struct cache_node *current = cqueue->head;

	while (current) {

		struct cache_node *temp = current;
		current = current->next;
		free(temp->file->pathname);
		free(temp->file->content);
		fclose(temp->file->f);
		free(temp->file);
		free(temp);
		temp = NULL;
	}

	UNLOCK( &(cqueue->queue_mutex) )
	pthread_mutex_destroy( &(cqueue->queue_mutex) );

	LOCK( &(table->table_mutex) )

	free(table->files_references);

	UNLOCK( &(table->table_mutex) )
	pthread_mutex_destroy( &(table->table_mutex) );
	
}

int cache_file_append(struct cache_queue *cqueue, struct cache_hash_table *table, char *filename,
                      struct my_file **ejected, unsigned long int *ejected_dim, void *data, size_t data_size) {

	if (!cqueue || !table || !filename || !ejected || !ejected_dim || !data) {
		const char *fmt = "%s cqueue = %p, table = %p, filename = %p, ejected = %p, ejected_dim = %p, data = %p\n";
		write_log(my_log, fmt, "Error in enqueue parameters:", cqueue, table, filename, ejected, ejected_dim);
		return -1;
	}

	LOCK( &(table->table_mutex) )

	unsigned long int fidx = cache_hash( (unsigned char *) filename ) % table->capacity;
	while ( strcmp(table->files_references[fidx]->file->pathname, filename) != 0 ) {
		if (fidx == table->capacity - 1) {
			fidx = 0;
			continue;
		}
		fidx++;
	}

	LOCK( &(cqueue->queue_mutex) )

	struct my_file *temp = table->files_references[fidx]->file;

	unsigned long int num_ejected = 0;

	if (cqueue->curr_capacity + data_size > cqueue->max_capacity) {

		if (cqueue->curr_files == 1 && strcmp(cqueue->head->file->pathname, filename) == 0) {
			fprintf(stderr,
                    "The file %s is the only file in the server and it already uses the full capacity "
                    "of the server. I cannot append more bytes to the file\n", filename);
            UNLOCK( &(cqueue->queue_mutex) )
            UNLOCK( &(table->table_mutex) )
			return -1;
		}

		while (cqueue->curr_capacity + data_size > cqueue->max_capacity ) {

			unsigned long int available;

            UNLOCK( &(cqueue->queue_mutex) )

			if ((available = dequeue(cqueue)) == -1) {
				const char *fmt = "%s\n";
				write_log(my_log, fmt, "cache_file_append: Error during dequeue");
				UNLOCK( &(table->table_mutex) )
                UNLOCK( &(cqueue->queue_mutex) )
				return -1;
			}
			else {

				num_ejected++;
				if (num_ejected > *ejected_dim) {

					if (*ejected_dim == 0)
						*ejected_dim = 1;
					else
						*ejected_dim *= 2;

					*ejected = realloc(*ejected, *ejected_dim);
					for (unsigned long int i = num_ejected; i < *ejected_dim; i++) {
						memset(&(*ejected)[i], 0, sizeof(struct my_file));
					}

				}
				(*ejected)[num_ejected - 1] = *(table->files_references[available]->file);
				free(table->files_references[available]->file->content);
				fclose(table->files_references[available]->file->f);
				free(table->files_references[available]);
				table->files_references[available] = NULL;
			}

            LOCK( &(cqueue->queue_mutex) )

		}

		fwrite( (char*) data, sizeof(char), data_size, temp->f );
		temp->content = realloc(temp->content, temp->file_size + data_size);
		memcpy( &( ((char**) temp->content)[temp->file_size]), data, data_size );
		temp->file_size += data_size;
        cqueue->curr_capacity += data_size;

	}
	else {

		fwrite( (char*) data, sizeof(char), data_size, temp->f );
		temp->content = realloc(temp->content, temp->file_size + data_size);
		memcpy( &( ((char**) temp->content)[temp->file_size]), data, data_size );
		temp->file_size += data_size;
        cqueue->curr_capacity += data_size;

	}

	UNLOCK( &(table->table_mutex) )
	UNLOCK( &(cqueue->queue_mutex) )

	return 0;
}

int cache_remove_file(struct cache_hash_table *table, struct cache_queue *cqueue, char *filename) {

	if (!table || !filename) {
		const char *fmt = "%s table = %p, filename = %p\n";
		write_log(my_log, fmt, "Error in enqueue parameters:", table, filename);
		return -1;
	}

	LOCK( &(table->table_mutex) )

	unsigned long int fidx = cache_hash( (unsigned char *) filename) % table->capacity;
	while ( strcmp(table->files_references[fidx]->file->pathname, filename) != 0 ) {
		if (fidx == table->capacity - 1) {
			fidx = 0;
			continue;
		}
		fidx++;
	}

	struct cache_node *temp = table->files_references[fidx];
    table->files_references[fidx] = NULL;

    UNLOCK( &(table->table_mutex) )

    LOCK( &(cqueue->queue_mutex) )

    if (temp == cqueue->head) {
        cqueue->head = temp->next;
    }
    else {
        if (temp->next)
            temp->next->prev = temp->prev;
        if (temp->prev)
            temp->prev->next = temp->next;
    }
    fclose(temp->file->f);
    free(temp->file->content);
    free(temp->file->pathname);
    free(temp->file);
    free(temp);

    UNLOCK( &(cqueue->queue_mutex) )

	return 0;
	
}
