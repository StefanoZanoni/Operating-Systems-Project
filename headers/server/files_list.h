#ifndef PROGETTOSOL_FILES_LIST_H
#define PROGETTOSOL_FILES_LIST_H

#include <pthread.h>

#include "cache.h"

struct references_list {

	struct references_list *prev,
						   *next;
	struct my_file *file;

};

struct conn_flist {

    pthread_mutex_t conn_list_mutex;
    struct references_list *clist;

};

extern struct references_list *flist;
extern pthread_mutex_t flist_mutex;

/**
 * This function is used to add a file to a doubly linked list
 *
 * @param list doubly linked list
 * @param file the file to add
 * @param mutex the mutex of the current list
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int files_list_add(struct references_list **list, struct my_file *file, pthread_mutex_t *mutex);

/**
 * This function is used to remove a file from a doubly linked list
 *
 * @param list doubly linked list
 * @param file the file to remove
 * @param mutex the mutex of the current list
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int files_list_remove(struct references_list **list, struct my_file file, pthread_mutex_t *mutex);

/**
 * This function is used to search a file into the list
 *
 * @param list doubly linked list
 * @param filename the absolute path of the file
 * @param mutex the mutex of the current list
 *
 * @return -1 error (errno set)
 * @return 0 if the file is not present in the list
 * @return 1 otherwise
 */
int files_list_search_file(struct references_list *list, char *filename, pthread_mutex_t *mutex);

/**
 * This function is used to get a specific file from the list
 *
 * @param list doubly linked list
 * @param filename the absolute path of the file
 * @param mutex the mutex of the current list
 *
 * @return NULL error (errno set)
 * @return searched file success
 */
struct my_file *files_list_get_file(struct references_list *list, char *filename,
                                    pthread_mutex_t *mutex);

/**
 * This function is used to clean up the list
 *
 * @param list doubly linked list
 * @param mutex the mutex of the current list
 */
void files_list_cleanup(struct references_list **list, pthread_mutex_t *mutex);

#endif //PROGETTOSOL_FILES_LIST_H
