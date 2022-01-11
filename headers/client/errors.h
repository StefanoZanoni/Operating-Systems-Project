#ifndef PROGETTOSOL_ERRORS_H
#define PROGETTOSOL_ERRORS_H

#define ESOCK 1
#define ECONN 2
#define EREADN 3
#define EWRITEN 4
#define ESERVER 5
#define ECALLOC 6
#define ECLOSE 7
#define EREALLOC 8
#define EFOPEN 9 

unsigned int local_errno;

/**
 * This function is used to print a standard error massage based on local_errno variable
 *
 * @param string the message to be printed with the error massage
 */
void my_perror(const char *string);

#endif //PROGETTOSOL_ERRORS_H
