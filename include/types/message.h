#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <utils/const-define.h>

typedef struct s_Msg {
    char sender[SIZE_NAME];
    char date[SIZE_DATE];
    char time[SIZE_TIME];
    char buffer[SIZE_MSG_DATA];
} Msg;

/**
 * @brief Init struct Msg
 *
 * @param msg Msg* to init
 * @param sender Sender of the message (size = SIZE_NAME)
 * @return O if success, -1 otherwise
 */
int initMsg(Msg *msg, char *sender, const char *string);

/**
 * @brief Convert Msg into char*
 *
 * @param msg Msg to convert
 * @return char*
 * @note max size = SIZE_TXT
 */
char *msgToTXT(Msg *msg);

/* Msg getters */

char *msgGetSender(Msg *msg);

char *msgGetDate(Msg *msg);

char *msgGetTime(Msg *msg);

char *msgGetBuffer(Msg *msg);

#endif