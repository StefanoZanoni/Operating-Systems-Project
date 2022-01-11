#ifndef PROGETTOSOL_PARSING_H
#define PROGETTOSOL_PARSING_H

/**
 * This function is used to parse the arguments passed by the terminal.
 * Options that take filenames as arguments ignore files with filenames starting with the '-'.
 * Also, all options ignore any arguments starting with the '-' character.
 * If the argument of an option is another option then ignore it and re-parsing that argument.
 * Every options that accept multiple arguments invoke tokenize function.
 * The options that don't accept any arguments store only their option name.
 * After parsing an option, it is stored in a queue.
 *
 * @param argc main argc
 * @param argv main argv
 * @param queue the list where I store all options
 */
void commands_parsing(int argc, char **argv, argsQueue_t *queue);

#endif //PROGETTOSOL_PARSING_H
