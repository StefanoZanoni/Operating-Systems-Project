#ifndef PROGETTOSOL_CONFIGURATION_H
#define PROGETTOSOL_CONFIGURATION_H

typedef struct sconfig {
    char *sockname;
    char *logpath;
    unsigned long int num_workers,
                      num_files,
                      storage_capacity;
} server_configuration_t;

void get_config(const char *path, server_configuration_t* configuration);

#endif //PROGETTOSOL_CONFIGURATION_H
