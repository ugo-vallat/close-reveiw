#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <network/manager.h>
#include <types/genericlist.h>
#define SIZE_CMD_RES 1024
#define COMMAND_MAX_SIZE 32
#define NB_COMMANDS 7

#define UNKNOWN_TXT "\033[1;31mUnknown command.\033[0m\n"

#define HELP_TXT                                                                                   \
    "\033[1;34mAvailable Commands:\033[0m\n"                                                       \
    "\n"                                                                                           \
    "\033[1m/list\033[0m:\t\tDisplays a list of all currently available users.\n"                  \
    "\033[1m/request <user_id>\033[0m:\tSends a connection request to the user with the "          \
    "specified ID.\n"                                                                              \
    "\033[1m/accept <user_id>\033[0m:\tAccepts an incoming connection request.\n"                  \
    "\033[1m/reject <user_id>\033[0m:\tRejects an incoming connection request.\n"                  \
    "\033[1m/close <user_id>\033[0m:\tCloses an existing connection with the specified user.\n"    \
    "\033[1m/quit\033[0m:\t\tExits the application.\n"                                             \
    "\033[1m/help\033[0m:\t\tShows this help message.\n"

typedef enum e_cmd_error {
    CMD_ERR_SUCCESS = 0,
    CMD_ERR_WRONG_FUNCTION_CALL = -1,
    CMD_ERR_MISSING_ARG = -2,
    CMD_ERR_INVALID_ARG = -3,
} CMD_error;

typedef enum e_type_cmd {
    CMD_LIST = 0,
    CMD_REQUEST = 1,
    CMD_ACCEPT = 2,
    CMD_REJECT = 3,
    CMD_CLOSE = 4,
    CMD_QUIT = 5,
    CMD_HELP = 6,
    CMD_UNKNOWN = -1
} Type_cmd;

typedef struct s_command {
    Type_cmd cmd;
    GenList *args;
} Command;

Command *initCommand(char *buffer);

void deinitCommand(Command **command);

CMD_error commandList(Command *command, Manager *manager);

CMD_error commandRequest(Command *command, Manager *manager);

CMD_error commandAnswer(Command *command, Manager *manager);

CMD_error commandClose(Command *command, Manager *manager);

CMD_error commandQuit(Command *command, Manager *manager);

CMD_error commandHelp(Command *command, Manager *manager);

CMD_error commandUnknown(Command *command, Manager *manager);

#endif // !__COMMAND_H__
