#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include "../../includes/log.h"
#include "../../includes/util.h"

/*
	* this function is used to open the first time the log file  
*/
FILE* start_log(char *pathname) {

	FILE *log = fopen(pathname, "a");
	if (!log)
		return NULL;

	char *start_string = "--------------------------------START OF LOGS--------------------------------";

	time_t timer;
	char date[26];
	memset(date, 0, 26 * sizeof(char));
	struct tm *tm_info;

	timer = time(NULL);
	tm_info = localtime(&timer);
	strftime(date, 26, "\n[%Y-%m-%d %H:%M:%S] - ", tm_info);
	fputs(date, log);

	fwrite(start_string, sizeof(char), 77, log);

	return log;
}

/*
	* this function is used to write a variable number of messages on log file.
	* the first message has the following form: [yyyy-mm-dd hh:mm:ss] - -----------first message---------
	* any other messages are not preceded by the date
*/
void write_log(FILE* log, char *fmt, ...) {

	time_t timer;
	char date[26];
	memset(date, 0, 26 * sizeof(char));
	struct tm *tm_info;

	timer = time(NULL);
	tm_info = localtime(&timer);
	strftime(date, 26, "\n[%Y-%m-%d %H:%M:%S] - ", tm_info);
	fputs(date, log);

	va_list va;
	va_start(va, fmt);
	vfprintf(log, fmt, va);
	va_end(va);
}
