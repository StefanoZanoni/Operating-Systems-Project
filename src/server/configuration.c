#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "../../common/util.h"
#include "../../headers/server/configuration.h"

/*
    * this function is used to free the memory in case of error
    * or at the end of configuration file reading
*/
static void config_cleanup(char **vectors, FILE *config) {

    if (vectors) {

        if (vectors[3])
            free(vectors[3]);

        if (vectors[4])
            free(vectors[4]);

        //in case of error I have to free also sockname and logpath
        if (errno != 0) {

            if ( vectors[0] ) 
                free( vectors[0] );

            if ( vectors[1] ) 
                free( vectors[1] );
                
            if ( vectors[2] ) 
                free( vectors[2] );
                
        }

    }

    free(vectors);

    if (config)
        fclose(config);
}

/*
    * this function is used to read the configuration file and save the configuration parameters 
    * into the server_configuration structure.
    * The configuration file is read line by line. 
*/
void get_config(const char *path, server_configuration_t *configuration) {

    if (!path || !configuration) {
        errno = EINVAL;
        fprintf(stderr, "Error reading server configuration");
        exit(EXIT_FAILURE);
    }

    //I prepare the facility for any cleanup in the event of a fatal error
    char **vectors = NULL;
    vectors = calloc(5, sizeof(char*));
    CHECK_EQ_EXIT(config_cleanup(vectors, NULL), "calloc config.c", vectors, NULL, "vectors\n", "")

    char *resolved_path = NULL;
    resolved_path = realpath(path, NULL);
    CHECK_EQ_EXIT(config_cleanup(vectors, NULL), "realpath config.c", resolved_path, NULL, "resolved_path\n", "")
    FILE *config = fopen(resolved_path, "r");
    free(resolved_path);
    CHECK_EQ_EXIT(config_cleanup(vectors, config), "fopen config.c", config, NULL, "config\n", "")

    //line and n are used to store either each line of config file and its length
    char *line = NULL;
    int n = 0;

    //informations is used to store the string before the actual server configuration data 
    char *informations = calloc(BUFSIZE, sizeof(char));
    CHECK_EQ_EXIT(config_cleanup(vectors, config), "calloc config.c", informations, NULL, "informations\n", "")
    vectors[3] = informations;

    //value is used to store acutal server configuration data
    char *value = calloc(BUFSIZE, sizeof(char));
    CHECK_EQ_EXIT(config_cleanup(vectors, config), "calloc config.c", value, NULL, "value\n", "")
    vectors[4] = value;

    while ( getline(&line, (size_t *) &n, config) != -1 ) {

        //in case of error on calloc "sockname" or "logpath", I need to free also line
        vectors[2] = line;

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

            size_t len = strlen(value) + 1;
            configuration->sockname = calloc(len, sizeof(char));
            vectors[0] = configuration->sockname;
            CHECK_EQ_EXIT(config_cleanup(vectors, config), "calloc config.c", configuration->sockname, NULL, "sockname\n", "")
            strcpy(configuration->sockname, value);

        }
        else if ( strstr(informations, "log" ) != NULL ) {

            configuration->logpath = realpath(value, configuration->logpath);
            vectors[1] = configuration->logpath;
            CHECK_EQ_EXIT(config_cleanup(vectors, config), "calloc config.c", configuration->logpath, NULL, "logpath\n", "")
            
        }

        //to avoid memory leaks, line is freed at each iteration and at the end of all iterations
        free(line);

        line = NULL;
        n = 0;
    }

    vectors[2] = line;

    CHECK_EQ_EXIT(config_cleanup(vectors, config), "getline config.c", errno, ENOMEM, "errno\n", "")
    CHECK_EQ_EXIT(config_cleanup(vectors, config), "getline config.c", errno, EINVAL, "errno\n", "")

    free(line);

    config_cleanup(vectors, config);
}
