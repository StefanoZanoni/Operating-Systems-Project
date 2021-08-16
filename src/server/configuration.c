#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "../../includes/util.h"
#include "../../includes/configuration.h"

static FILE *config = NULL;

/*
    * this function is used to free the memory in case of error
    * or at the end of configuration file reading
*/
void config_cleanup(cleanup_structure_t cstruct) {

    int i = 3;

    if (cstruct.pointers != NULL) {

        while (cstruct.pointers[i] != NULL) {

            if ( ((char **) cstruct.pointers)[i] != NULL ) {
                free( ((char **) cstruct.pointers)[i] );
            }

            i++;
        }

        //in case of error I have to free also sockname and logpath
        if (errno != 0) {

            if ( ((char **) cstruct.pointers)[0] != NULL )
                free( ((char **) cstruct.pointers)[0] );
            if ( ((char **) cstruct.pointers)[1] != NULL )
                free( ((char **) cstruct.pointers)[1] );
            if ( ((char **) cstruct.pointers)[2] != NULL )
                free( ((char **) cstruct.pointers)[2] );
        }
    }

    if (config)
        fclose(config);

    free( (char **) cstruct.pointers );
}

void print_config(server_configuration_t configuration) {

    if (configuration.sockname != NULL && configuration.logpath != NULL) {
        printf("num_workers = %lu\n", configuration.num_workers);
        printf("num_files = %lu\n", configuration.num_files);
        printf("storage_capacity = %lu\n", configuration.storage_capacity);
        printf("sockname = %s\n", configuration.sockname);
        printf("logpath = %s\n", configuration.logpath);
    }
}

/*
    * this function is used to read the configuration file and save the configuration parameters 
    * into the server_cofiguration structure.
    * The configuration file is read line by line. 
*/
void get_config(char *path, server_configuration_t *configuration) {

    //I prepare the facility for any cleanup in the event of a fatal error
    cleanup_structure_t cstruct;
    memset(&cstruct, 0, sizeof(cstruct));
    cstruct.pointers = calloc(6, sizeof(char*));
    CHECK_EQ_EXIT(config_cleanup(cstruct), "calloc config.c", cstruct.pointers, NULL, "cstruct.pointers\n", "");

    config = fopen(path, "r");
    CHECK_EQ_EXIT(config_cleanup(cstruct), "fopen config.c", config, NULL, "config\n", "");

    //line and n are used to store either each line of config file and its length
    char *line = NULL;
    int n = 0;

    //informations is used to store the string before the actual server configuration data 
    char *informations = calloc(BUFSIZE, sizeof(char));
    CHECK_EQ_EXIT(config_cleanup(cstruct), "calloc config.c", informations, NULL, "informations\n", "");
    cstruct.pointers[3] = informations;

    //value is used to store acutal server configuration data
    char *value = calloc(BUFSIZE, sizeof(char));
    CHECK_EQ_EXIT(config_cleanup(cstruct), "calloc config.c", value, NULL, "value\n", "");
    cstruct.pointers[4] = value;
    cstruct.pointers[5] = NULL;

    while ( getline(&line, (size_t *) &n, config) != -1 ) {

        //in case of error on calloc "sockname" or "logpath", I need to free also line
        cstruct.pointers[2] = line;

        //if the current line is empty, or it is a comment then continue
        if (line[0] == '#' || n <= 1)
            continue;

        sscanf(line, "%s = %s", informations, value);

        if ( strstr(informations, "Threads" ) != NULL ) {

            configuration->num_workers = strtoul(value, NULL, 10);

        }
        else if ( strstr(informations, "Files" ) != NULL ) {

            configuration->num_files = strtoul(value, NULL, 10);

        }
        else if ( strstr(informations, "storage" ) != NULL ) {

            configuration->storage_capacity = strtoul(value, NULL, 10);

        }
        else if ( strstr(informations, "socket" ) != NULL ) {

            size_t len = strlen(value);
            configuration->sockname = calloc(len + 1, sizeof(char));
            cstruct.pointers[0] = configuration->sockname;
            CHECK_EQ_EXIT(config_cleanup(cstruct), "calloc config.c", configuration->sockname, NULL, "sockname\n", "");
            strncpy(configuration->sockname, value, len + 1);
        }
        else if ( strstr(informations, "log" ) != NULL ) {

            size_t len = strlen(value);
            configuration->logpath = calloc(len + 1, sizeof(char));
            cstruct.pointers[1] = configuration->logpath;
            CHECK_EQ_EXIT(config_cleanup(cstruct), "calloc config.c", configuration->logpath, NULL, "logpath\n", "");
            strncpy(configuration->logpath, value, len + 1);
        }

        //to avoid memory leaks, line is freed at each iteration and at the end of all iterations
        free(line);

        line = NULL;
        n = 0;
    }

    cstruct.pointers[2] = line;

    CHECK_EQ_EXIT(config_cleanup(cstruct), "getline config.c", errno, ENOMEM, "errno\n", "");
    CHECK_EQ_EXIT(config_cleanup(cstruct), "getline config.c", errno, EINVAL, "errno\n", "");

    free(line);

    config_cleanup(cstruct);
}
