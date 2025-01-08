#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdbool.h>
#include <utils/project_constants.h>

typedef struct s_Msg {
    char sender[SIZE_NAME];
    char date[SIZE_DATE];
    char time[SIZE_TIME];
    char buffer[SIZE_MSG_DATA];
} Msg;

/**
 * @brief Init struct Msg
 *
 * @param[in] sender Sender of the message (size = SIZE_NAME)
 * @param[in] string String to store into the message (size = SIZE_MSG_DATA)
 * @return Msg* or NULL if error
 */
Msg *initMsg(char *sender, const char *string);

/**
 * @brief Deletes struct Msg and free the memory
 *
 * @param[in] msg Msg to delete
 */
void deinitMsg(Msg **msg);

/**
 * @brief Convert Msg into char*
 *
 * @param[in] msg Msg to convert
 * @return char*
 * @note max size = SIZE_TXT
 */
char *msgToTXT(Msg *msg);

/**
 * @brief Convert Msg into char* and store it in txt
 *
 * @param[in] msg Msg to convert
 * @param[out] txt TXT buffer
 * @return char*
 * @note max size = SIZE_TXT
 */
int msgIntoTXT(Msg *msg, char *txt);

/**
 * @brief Copy msg_src into msg_dst
 *
 * @param msg_dst Source message
 * @param msg_src Destination message
 */
void msgCopy(Msg *msg_dst, Msg *msg_src);

/* Msg getters */

char *msgGetSender(Msg *msg);

char *msgGetDate(Msg *msg);

char *msgGetTime(Msg *msg);

char *msgGetBuffer(Msg *msg);

#endif
