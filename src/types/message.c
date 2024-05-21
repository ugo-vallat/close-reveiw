#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <types/genericlist.h>
#include <types/message.h>
#include <utils/logger.h>
#include <utils/project_constants.h>

#define FILE_MESSAGES "message.c"

Msg *initMsg(char *sender, const char *string) {
    char FUN_NAME[32] = "initMsg";
    assertl(sender, FILE_MESSAGES, FUN_NAME, -1, "sender NULL");
    assertl(string, FILE_MESSAGES, FUN_NAME, -1, "string NULL");
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

void deinitMsg(Msg **msg) {
    char FUN_NAME[32] = "initMsg";
    assertl(msg, FILE_MESSAGES, FUN_NAME, -1, "msg NULL");
    assertl(*msg, FILE_MESSAGES, FUN_NAME, -1, "*msg NULL");

    free(*msg);
    *msg = NULL;
}

char *msgToTXT(Msg *msg) {
    char FUN_NAME[32] = "msgToTXT";
    assertl(msg, FILE_MESSAGES, FUN_NAME, -1, "msg NULL");
    char *txt = malloc(SIZE_TXT);
    snprintf(txt, SIZE_TXT, "[ %s ] %s (%s)\n%s\n", msg->sender, msg->date, msg->time, msg->buffer);
    return txt;
}

int msgIntoTXT(Msg *msg, char *txt) {
    char FUN_NAME[32] = "msgIntoTXT";
    assertl(msg, FILE_MESSAGES, FUN_NAME, -1, "msg NULL");
    assertl(txt, FILE_MESSAGES, FUN_NAME, -1, "txt NULL");
    snprintf(txt, SIZE_TXT, "[ %s ] %s (%s)\n%s\n", msg->sender, msg->date, msg->time, msg->buffer);
    return 0;
}

void msgCopy(Msg *msg_dst, Msg *msg_src) {
    char FUN_NAME[32] = "msgCopy";
    assertl(msg_src, FILE_MESSAGES, FUN_NAME, -1, "msg_src NULL");
    assertl(msg_dst, FILE_MESSAGES, FUN_NAME, -1, "msg_dst NULL");

    memcpy(msg_dst, msg_src, sizeof(Msg));
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
