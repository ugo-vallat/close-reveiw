#include <client/tui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Command get_command_type(char *command) {
    char *cases[NB_COMMANDS] = {"list",   "request", "connect", "accept",
                                "reject", "close",   "quit",    "help"};
    for (int i = 0; i < NB_COMMANDS; i++) {
        if (strncmp(command, cases[i], strnlen(command, COMMAND_MAX_SIZE)) == 0) {
            return i;
        }
    }
    return UNKNOWN;
}

bool is_valid_character(unsigned char character) {
    return character >= 'A' && character <= 'Z' || character >= 'a' && character <= 'z' ||
           character >= '0' && character <= '9' || character == '_';
}

bool is_valid_user_id(char *user_id) {
    size_t size_user_id = strnlen(user_id, SIZE_NAME);
    for (int i = 0; i < SIZE_NAME; i++) {
        if (!is_valid_character(user_id[i]))
            return false;
    }
    return true;
}

void print_messages(char *user, char *message) {
    printf("\e[1;34m%s\e[0m : %s", user, message);
}

char *get_user_input(char *user) {
    char *buffer = calloc(SIZE_DATA_PACKET, sizeof(char));
    size_t size_allocated = SIZE_DATA_PACKET;
    printf("\e[1;34m%s\e[0m : ", user);
    if (getline(&buffer, &size_allocated, stdin) == -1) {
        perror("Failed to get user input");
        free(buffer);
        return NULL;
    }
    return buffer;
}
