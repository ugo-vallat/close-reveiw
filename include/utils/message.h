#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define SIZE_DATE 20
#define SIZE_TIME 20
#define SIZE_MSG_DATA 1024
#define SIZE_NAME 30

typedef struct s_Msg {
    char sender[SIZE_NAME];
    char date[SIZE_DATE];
    char time[SIZE_TIME];
    char buffer[SIZE_MSG_DATA];
} Msg;

Msg *createMsg(char *sender, const char *string);

void deleteMsg(Msg *msg);

void deleteMsgGen(void *msg);

char *msgGetSender(Msg *msg);

char *msgGetDate(Msg *msg);

char *msgGetTime(Msg *msg);

char *msgGetBuffer(Msg *msg);

Msg *msgFromChar(char *data);

char *msgToChar(Msg *msg);

void printMsg(Msg *msg);

#endif