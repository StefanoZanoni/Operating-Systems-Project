#ifndef PROGETTOSOL_COMMANDS_EXECUTION_H
#define PROGETTOSOL_COMMANDS_EXECUTION_H

#include "./queue.h"

/**
 * This function is used to print the help massage
 */
void execute_h();

/**
 * This function is used to execute a command in the commands list
 *
 * @param command the current command
 *
 * @return 1 calling execute_h
 * @return 0 success
 * @return -1 error (errno set)
 */
int execute_command(commandline_arg_t command);

#endif //PROGETTOSOL_COMMANDS_EXECUTION_H