#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../../headers/server/files_list.h"
#include "../../headers/server/log.h"
#include "../../common/util.h"

int files_list_add(struct references_list **list, struct my_file *file, pthread_mutex_t *mutex) {

	if (!file) {
		errno = EINVAL;
		const char *fmt = "%s file = %p\n";
		write_log(my_log, fmt, "Error in add parameters", file);
		return -1;
	}

    LOCK( mutex )

    if (!(*list)) {

        *list = calloc(1, sizeof(struct references_list));
        if (!*list) {
            const char *fmt = "%s\n";
            write_log(my_log, fmt, "files_list_add: Error in list allocation");
            UNLOCK( mutex )
            return -1;
        }
        (*list)->prev = NULL;
        (*list)->next = NULL;
        (*list)->file = file;

    }
    else {

        struct references_list *node = calloc(1, sizeof(struct references_list));
        if (!node) {
            const char *fmt = "%s\n";
            write_log(my_log, fmt, "files_list_add: Error in node allocation");
            UNLOCK(mutex)
            return -1;
        }
        node->prev = NULL;
        node->next = *list;
        node->file = file;

        (*list)->prev = node;
        *list = node;

    }

	UNLOCK( mutex )

	return 0;

}

struct my_file *files_list_get_file(struct references_list *list, char *filename, pthread_mutex_t *mutex) {

	if (!filename) {
		errno = EINVAL;
		const char *fmt = "%s filename = %p\n";
		write_log(my_log, fmt, "Error in files_list_get_file parameters", list, filename);
		return NULL;
	}

	LOCK( mutex )

	struct references_list *aux = list;

	while ( aux != NULL && strcmp(aux->file->pathname, filename) != 0 )
		aux = aux->next;

	if (!aux) {
        UNLOCK( mutex )
        return NULL;
    }

	UNLOCK( mutex )

	return aux->file;
}

int files_list_remove(struct references_list **list, struct my_file file, pthread_mutex_t *mutex) {

	if (!list) {
		errno = EINVAL;
		const char *fmt = "%s list = %p\n";
		write_log(my_log, fmt, "Error in remove parameters", list);
		return -1;
	}

	LOCK( mutex )

	struct references_list *aux = *list;

	while ( aux != NULL && strcmp(aux->file->pathname, file.pathname) != 0 )
		aux = aux->next;

    if (!aux) {
        UNLOCK( mutex )
        return -1;
    }

    if (aux == *list)
        *list = aux->next;
    else {
        if (aux->next)
            aux->next->prev = aux->prev;
        if (aux->prev)
            aux->prev->next = aux->next;
    }

    free(aux->file->pathname);
    free(aux->file->content);
    free(aux->file);
    free(aux);

	UNLOCK( mutex )

	return 0;
}

int files_list_search_file(struct references_list *list, char *filename, pthread_mutex_t *mutex) {

	if (!filename) {
		errno = EINVAL;
		const char *fmt = "%s filename = %p\n";
		write_log(my_log, fmt, "Error in files_list_search_file parameters", filename);
		return -1;
	}

	LOCK( mutex )

	struct references_list *aux = list;

	while (aux != NULL && aux->file != NULL && strcmp(aux->file->pathname, filename) != 0)
		aux = aux->next;

	if (!aux || !aux->file) {
		UNLOCK( mutex )
		return 0;
	}

	UNLOCK( mutex )

	return 1;
}

void files_list_cleanup(struct references_list **list, pthread_mutex_t *mutex) {

    pthread_mutex_destroy( mutex );

    if (list) {

        while (*list != NULL) {

            struct references_list *aux = *list;
            *list = (*list)->next;
            free(aux->file->pathname);
            free(aux->file->content);
           	free(aux->file);
            free(aux);

        }

    }

}