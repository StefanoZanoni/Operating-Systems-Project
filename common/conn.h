#ifndef PROGETTOSOL_CONN_H
#define PROGETTOSOL_CONN_H

#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/** Avoid partial readings
 *
 *   \retval -1   error (errno set)
 *   \retval  0   if while reading from fd i read EOF
 *   \retval size if it ends successfully
 */
static inline int readn(long fd, void *buf, size_t size) {

    size_t left = size;
    int r;
    char *bufptr = (char*)buf;

    while(left > 0) {

        if ( (r = read((int) fd ,bufptr, left)) == -1 ) {

    	    if (errno == EINTR) 
                continue;
            
    	    return -1;

	    }

    	if (r == 0) 
            return 0;   // EOF

        left    -= r;
    	bufptr  += r;

    }

    return size;

}

/** Avoid partial writing
 *
 *   \retval -1   error (errno set)
 *   \retval  0   0 if while writing the write returns 0
 *   \retval  1   if the writing ends with success
 */
static inline int writen(long fd, void *buf, size_t size) {

    size_t left = size;
    int r;
    char *bufptr = (char*)buf;

    while(left > 0) {

    	if ( (r = write((int) fd, bufptr, left)) == -1 ) {

    	    if (errno == EINTR) 
                continue;

    	    return -1;

    	}

    	if (r == 0) 
            return 0; 

        left    -= r;
    	bufptr  += r;

    }

    return 1;
}

#endif //PROGETTOSOL_CONN_H
