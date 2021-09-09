#ifndef PROGETTOSOL_SERVER_COMMANDS_H
#define PROGETTOSOL_SERVER_COMMANDS_H

#define CMD_OPEN_FILE 1
#define CMD_READ_FILE 2
#define CMD_READN_FILES 3
#define CMD_WRITE_FILE 4
#define CMD_APPEND_TO_FILE 5
#define CMD_LOCK_FILE 6
#define CMD_UNLOCK_FILE 7
#define CMD_CLOSE_FILE 8
#define CMD_REMOVE_FILE 9

#define O_CREATE (0x0001)
#define O_LOCK (0x0002)

typedef struct command {

	int cmd;
	int flags;
	size_t data_size;
	int num_files;
	const char *filepath;
	void *data;

} server_command_t;

typedef struct outcome {

	int res;
	int all_read;
	int ejected;
	size_t data_size;
	size_t name_len;
	const char *filename;
	void *data;

} server_outcome_t;

#endif //PROGETTOSOL_SERVER_COMMANDS_H
