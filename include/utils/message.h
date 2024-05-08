#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdbool.h>

#define SIZE_DATE 8
#define SIZE_TIME 5
#define SIZE_MSG_DATA 2048

typedef struct s_Msg {
    char sender[30];
    char date[SIZE_DATE];
    char time[SIZE_TIME];
    char buffer[SIZE_MSG_DATA];
    unsigned int size;
} Msg;

Msg *init_message(const char *string);

unsigned int get_size(Msg *message);

bool append_string(Msg *dest, const char *src);

bool append_message(Msg *dest, const Msg *src);

void print_message(Msg *message);

void deinit_message(Msg *message);

#endif