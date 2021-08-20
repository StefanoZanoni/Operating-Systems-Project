#ifndef PROGETTOSOL_LOG_H
#define PROGETTOSOL_LOG_H

FILE* start_log(char *path);
void write_log(FILE* log, char *fmt, ...);

#endif //PROGETTOSOL_LOG_H