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

int files_list_add(struct references_list **list, struct my_file *file, pthread_mutex_t *mutex);
int files_list_remove(struct references_list **list, struct my_file file, pthread_mutex_t *mutex);
int files_list_search_file(struct references_list *list, char *filename, pthread_mutex_t *mutex);
struct my_file *files_list_get_file(struct references_list *list, char *filename,
                                    pthread_mutex_t *mutex);
void files_list_cleanup(struct references_list **list, pthread_mutex_t *mutex);

#endif //PROGETTOSOL_FILES_LIST_H