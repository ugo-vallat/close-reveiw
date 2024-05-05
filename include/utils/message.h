#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdbool.h>

#define SIZE_MSG_DATA 2048

typedef struct s_Msg {
    unsigned char buffer[SIZE_MSG_DATA];
    unsigned int size;
} Msg;

Msg *init_message(const char *string);

unsigned int get_size(Msg *message);

bool append_string(Msg *dest, const char *src);

bool append_message(Msg *dest, const Msg *src);

void print_message(Msg *message);

void deinit_message(Msg *message);

#endif