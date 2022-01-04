#ifndef PROGETTOSOL_LOG_H
#define PROGETTOSOL_LOG_H

FILE* start_log(const char *path);
void write_log(FILE* log, const char *fmt, ...);

extern FILE *my_log;

#endif //PROGETTOSOL_LOG_H
