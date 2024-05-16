#ifndef __TUI_H__
#define __TUI_H__

#include <stdbool.h>

#define COMMAND_MAX_SIZE 32
#define NB_COMMANDS 8
#define SIZE_NAME 30
#define SIZE_DATA_PACKET 2048

typedef enum {
    LIST = 0,
    REQUEST = 1,
    CONNECT = 2,
    ACCEPT = 3,
    REJECT = 4,
    CLOSE = 5,
    QUIT = 6,
    HELP = 7,
    UNKNOWN = 8
} Command;

Command get_command_type(char *command);

bool is_valid_character(unsigned char character);

bool is_valid_user_id(char *user_id);

void print_messages(char *user, char *message);

char *get_user_input(char *user);

#endif // !__TUI_H__
