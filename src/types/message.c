#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <types/genericlist.h>
#include <types/message.h>
#include <utils/logger.h>
#include <utils/project_constants.h>


Msg *initMsg(char *sender, const char *string) {
        ASSERTL(sender,-1, "sender NULL")
    ASSERTL(string,-1, "string NULL")
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
        ASSERTL(msg,-1, "msg NULL")
    ASSERTL(*msg,-1, "*msg NULL")

    free(*msg);
    *msg = NULL;
}

char *msgToTXT(Msg *msg) {
        ASSERTL(msg,-1, "msg NULL")
    char *txt = malloc(SIZE_TXT);
    snprintf(txt, SIZE_TXT, "[ %s ] %s (%s)\n%s\n", msg->sender, msg->date, msg->time, msg->buffer);
    return txt;
}

int msgIntoTXT(Msg *msg, char *txt) {
        ASSERTL(msg,-1, "msg NULL")
    ASSERTL(txt,-1, "txt NULL")
    snprintf(txt, SIZE_TXT, "[ %s ] %s (%s)\n%s\n", msg->sender, msg->date, msg->time, msg->buffer);
    return 0;
}

void msgCopy(Msg *msg_dst, Msg *msg_src) {
        ASSERTL(msg_src,-1, "msg_src NULL")
    ASSERTL(msg_dst,-1, "msg_dst NULL")

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
