#ifndef PROGETTOSOL_CONFIGURATION_H
#define PROGETTOSOL_CONFIGURATION_H

typedef struct sconfig {

    char *sockname;
    char *logpath;
    unsigned long int num_workers,
                      num_files,
                      storage_capacity;

} server_configuration_t;

/**
 * This function is used to read the configuration file and save the configuration parameters
 * into the server_configuration structure. The configuration file is read line by line.
 *
 * @param path the absolute path to the configuration file
 * @param configuration the structure in which to store the configuration parameters
 */
void get_config(const char *path, server_configuration_t* configuration);

#endif //PROGETTOSOL_CONFIGURATION_H
