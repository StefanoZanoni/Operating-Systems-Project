#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "../../includes/util.h"
#include "../../includes/configuration.h"

void config_cleanup(cleanup_structure_t cstruct) {

    int i;

    if (cstruct.pointers != NULL) {
        for (i = 1; i < 4; i++) {
            if ( ((char **) cstruct.pointers)[i] != NULL ) {
                free( ((char **) cstruct.pointers)[i] );
            }
        }
    }

    //in case of error I have to free also sockname
    if (errno != 0) {
        if ( ((char **) cstruct.pointers)[0] != NULL ) {
                free( ((char **) cstruct.pointers)[0] );
            }
    }

    if (cstruct.files != NULL) {
        if ( cstruct.files[0] != NULL ) {
            fclose( cstruct.files[0] );
        }
    }

    free((char **) cstruct.pointers);
    free(cstruct.files);
}

void print_config(server_configuration_t configuration) {

    if (configuration.sockname != NULL) {
        printf("%lu\n", configuration.num_workers);
        printf("%lu\n", configuration.num_files);
        printf("%lu\n", configuration.storage_capacity);
        printf("%s\n", configuration.sockname);
    }
}

void get_config(char *path, server_configuration_t *configuration) {

    //I prepare the facility for any cleanup in the event of a fatal error
    cleanup_structure_t cstruct;
    memset(&cstruct, 0, sizeof(cstruct));
    cstruct.pointers = calloc(4, sizeof(char*));
    cstruct.files = calloc(1, sizeof(FILE*));
    cstruct.pointers[0] = configuration->sockname;

    FILE *config = fopen(path, "r");
    cstruct.files[0] = config;
    CHECK_EQ_EXIT(config_cleanup(cstruct), "fopen", config, NULL, "fopen", "");
    char *line = calloc(BUFSIZE, sizeof(char));
    int n = BUFSIZE;
    char *informations = calloc(BUFSIZE, sizeof(char));
    char *value = calloc(BUFSIZE, sizeof(char));
    cstruct.pointers[1] = line;
    cstruct.pointers[2] = informations;
    cstruct.pointers[3] = value;

    while ( getline(&line, (size_t *) &n, config) != -1 ) {

        //if the current line is empty, or it is a comment then continue
        if (line[0] == '#' || n <= 1)
            continue;

        sscanf(line, "%s = %s", informations, value);

        if ( strstr(informations, "Threads") != NULL ) {

            configuration->num_workers = strtoul(value, NULL, 10);

        }
        else if ( strstr(informations, "Files") != NULL ) {

            configuration->num_files = strtoul(value, NULL, 10);

        }
        else if ( strstr(informations, "storage") != NULL ) {

            configuration->storage_capacity = strtoul(value, NULL, 10);

        }
        else if ( strstr(informations, "socket") != NULL ) {

            int len = (int) strlen(value);
            configuration->sockname = calloc(len + 1, sizeof(char));
            strncpy(configuration->sockname, value, len);
        }

        memset(line, 0, n * sizeof(char));
    }

    CHECK_EQ_EXIT(config_cleanup(cstruct), "getline", errno, ENOMEM, "getline", "");
    CHECK_EQ_EXIT(config_cleanup(cstruct), "getline", errno, EINVAL, "getline", "");

    config_cleanup(cstruct);
}