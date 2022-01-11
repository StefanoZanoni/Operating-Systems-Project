#ifndef PROGETTOSOL_LOG_H
#define PROGETTOSOL_LOG_H

extern FILE *my_log;

/**
 * This function is used to open the first time the log file
 *
 * @param path the absolute path of the log file
 *
 * @return NULL error (errno set)
 * @return log success
 */
FILE* start_log(const char *path);

/**
 * This function is used to write a variable number of messages on log file.
 * The first message has the following form: [yyyy-mm-dd hh:mm:ss] - -----------first message---------
 * any other messages are not preceded by the date
 *
 * @param log the log file
 * @param fmt the format of the string
 * @param ...
 */
void write_log(FILE* log, const char *fmt, ...);

#endif //PROGETTOSOL_LOG_H
