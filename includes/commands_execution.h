#ifndef PROGETTOSOL_COMMANDS_EXECUTION_H
#define PROGETTOSOL_COMMANDS_EXECUTION_H

#include "./queue.h"

void execute_h();
int execute_command(argnode_t *command, argsQueue_t *queue);

#endif //PROGETTOSOL_COMMANDS_EXECUTION_H