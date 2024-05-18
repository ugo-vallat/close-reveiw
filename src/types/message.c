#include "network/packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <utils/genericlist.h>
#include <utils/message.h>

Msg *createMsg(char *sender, const char *string) {
    time_t current_time;
    struct tm *local_time;

    /* init */
    Msg *msg = malloc(sizeof(Msg));
    memset(msg, 0, sizeof(Msg));

    /* data */
    strncpy(msg->sender, sender, SIZE_NAME);
    strncpy(msg->buffer, string, SIZE_MSG_DATA);

    /* time */
    current_time = time(NULL);
    local_time = localtime(&current_time);
    strftime(msg->date, SIZE_DATE, "%d-%m-%Y", local_time);
    strftime(msg->time, SIZE_TIME, "%H:%M:%S", local_time);
    return msg;
}

void deleteMsg(Msg *msg) {
    free(msg);
}

void deleteMsgGen(void *msg) {
    free((Msg *)msg);
}

char *msgGetSender(Msg *msg) {
    char *sender = malloc(SIZE_NAME);
    strncpy(sender, msg->sender, SIZE_NAME);
    return sender;
}

char *msgGetDate(Msg *msg) {
    char *date = malloc(SIZE_DATE);
    strncpy(date, msg->date, SIZE_DATE);
    return date;
}

char *msgGetTime(Msg *msg) {
    char *t = malloc(SIZE_TIME);
    strncpy(t, msg->time, SIZE_TIME);
    return t;
}

char *msgGetBuffer(Msg *msg) {
    char *buff = malloc(SIZE_MSG_DATA);
    strncpy(buff, msg->buffer, SIZE_MSG_DATA);
    return buff;
}

void printMsg(Msg *msg) {
    printf("[ %s ] %s (%s)\n", msg->sender, msg->date, msg->time);
    printf("%s\n", msg->buffer);
}

GenList *splitDataMsg(char *data) {
    char *string;
    bool end = false;
    GenList *list = createGenList(8);
    int i = 0, j = 0;
    string = malloc(SIZE_DATA_PACKET);
    while (!end && i < SIZE_DATA_PACKET) {
        if (data[i] == ';') {
            string[++j] = '\0';
            genListAdd(list, string);
            string = malloc(SIZE_DATA_PACKET);
            j = 0;
        } else if (data[i] == '\0') {
            string[++j] = '\0';
            genListAdd(list, string);
            end = true;
        } else {
            string[j] = data[i];
            j++;
        }
        i++;
    }
    return list;
}

Msg *msgFromChar(char *data) {
    Msg *msg = malloc(sizeof(Msg));
    GenList *list = splitDataMsg(data);
    strncpy(msg->sender, genListGet(list, 0), SIZE_NAME);
    strncpy(msg->date, genListGet(list, 1), SIZE_DATE);
    strncpy(msg->time, genListGet(list, 2), SIZE_TIME);
    strncpy(msg->buffer, genListGet(list, 3), SIZE_MSG_DATA);
    deleteGenList(&list, free);
    return msg;
}

char *msgToChar(Msg *msg) {
    char *data = malloc(SIZE_DATA_PACKET);
    snprintf(data, SIZE_DATA_PACKET, "%s;%s;%s;%s", msg->sender, msg->date, msg->time, msg->buffer);
    return data;
}
