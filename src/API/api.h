#ifndef PROGETTOSOL_API_H
#define PROGETTOSOL_API_H

/**
 * This function is used to connect the client to the socket specified by sockname.
 *
 * @param sockname the name of the socket
 * @param msec number of milliseconds after which the connection is attempted again
 * @param abstime absolute time beyond which no more attempts are made to establish the connection
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int openConnection(const char *sockname, int msec, struct timespec abstime);

/**
 * This function is used to close the connection with the socket specified by sockname
 *
 * @param sockname the name of the socket
 *
 * @retun -1 error (errno set)
 * @return 0 success
*/
int closeConnection(const char *sockname);

/**
 * This function is used to open a file on the server
 *
 * @param pathname the absolute path of the file
 * @param flags how I can open the file
 *              O_CREATE to create the file if it is has not already been created
 *              O_LOCK to lock the file
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int openFile(const char *pathname, int flags);

/**
 * This function is used to read a file from the server
 *
 * @param pathname the absolute path of the file to be read from the server
 * @param buf the buffer where the contents of the file are stored
 * @param size the size of the buffer
 *
 * @return -1 error (errno set, buf and size are not valid)
 * @return 0 success
 */
int readFile(const char *pathname, void **buf, size_t *size);

/**
 * This function is used to read N random files from the server
 *
 * @param N the number of files to be read. If N <= 0 I read all of the files stored on the server
 * @param dirname the directory where the read files will be stored
 *
 * @return -1 error (errno set)
 * @retun >= 0 number of read files
 */
int readNFiles(int N, const char *dirname);

/**
 * This function is used to write a file on the server
 *
 * @param pathname the absolute path of the file to be written to the server
 * @param dirname the directory where the ejected files will be stored
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int writeFile(const char *pathname, const char *dirname);

/**
 * This function is used to append the "size" bytes contained in the "buf" in the "pathname" file
 *
 * @param pathname the absolute path of the file
 * @param buf the buffer that contains the bytes to add to the file
 * @param size the size of the buffer
 * @param dirname the directory where the ejected files will be stored
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname);

/**
 * This function is used to lock a file stored on the server
 *
 * @param pathname the absolute path of the file
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int lockFile(const char *pathname);

/**
 * This function is used to lock a file stored on the server
 *
 * @param pathname the absolute path of the file
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int unlockFile(const char *pathname);

/**
 * This function is used to close a file stored on the server
 *
 * @param pathname the absolute path of the file
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int closeFile(const char *pathname);

/**
 * This function is used to remove a file stored on the server
 *
 * @param pathname the absolute path of the file
 *
 * @return -1 error (errno set)
 * @return 0 success
 */
int removeFile(const char *pathname);

#endif //PROGETTOSOL_API_H