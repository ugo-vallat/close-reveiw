#ifndef __GESTONARU_COM__
#define __GESTONARU_COM__

#include "utils/genericlist.h"
#include "utils/token.h"
#include <network/p2p-msg.h>
#include <network/packet.h>
#include <network/tls-com.h>
#include <utils/message.h>

typedef struct s_gestionary_com {
    TLS_infos *tls;
    bool open;

    /* buffers */
    GenList *msgReceived;
    GenList *p2pReceived;
    GenList *txtReceived;
} Gestionary_com;

/**
 * @brief Create a Gestionary Com object
 *
 * @param[in] tls TLS com already open
 * @return Gestionary_com*
 */
Gestionary_com *createGestionaryCom(TLS_infos *tls);

/**
 * @brief Delete structure Gestionary_com and struct TLS_infos
 *
 * @param[in] gestionary Gestionary_com
 */
void deleteGestionaryCom(Gestionary_com **gestionary);

/**
 * @brief Send structre Msg to the peer
 *
 * @param[in] gestionary Gestionary_com
 * @param[in] msg Message to send
 * @return 0 if succes, -1 otherwise (check gestionaryIsComOpen)
 */
int gestionarySendMsg(Gestionary_com *gestionary, Msg *msg);

/**
 * @brief Send structre P2P_msg to the peer
 *
 * @param[in] gestionary Gestionary_com
 * @param[in] msg Message to send
 * @return 0 if succes, -1 otherwise (check gestionaryIsComOpen)
 */
int gestionarySendP2P(Gestionary_com *gestionary, P2P_msg *msg);

/**
 * @brief Send char *
 *
 * @param[in] gestionary Gestionary_com
 * @param[in] txt Text to send
 * @return 0 if succes, -1 otherwise (check gestionaryIsComOpen)
 */
int gestionarySendTxt(Gestionary_com *gestionary, char *txt);

/**
 * @brief Return the oldest struct Msg received
 *
 * @param[in] gestionary Gestionary_com
 * @param[out] msg Message received
 * @return 1 if message received, 0 if no message, -1 otherwise
 */
int gestionaryReceiveMsg(Gestionary_com *gestionary, Msg **msg);

/**
 * @brief Return the oldest struct P2P_msg received
 *
 * @param[in] gestionary Gestionary_com
 * @param[out] msg Message received
 * @return 1 if message received, 0 if no message, -1 otherwise
 */
int gestionaryReceiveP2P(Gestionary_com *gestionary, P2P_msg **msg);

/**
 * @brief Return the oldest char* received
 *
 * @param[in] gestionary Gestionary_com
 * @param[out] txt Text received
 * @return 1 if message received, 0 if no message, -1 otherwise
 */
int gestionaryReceiveTxt(Gestionary_com *gestionary, char **txt);

/**
 * @brief Check if communication tls is closed
 *
 * @param gestionary Gestionary_com
 * @return true if open, false otherwise
 * @note Only read is possible if communication is closed
 */
bool gestionaryIsComOpen(Gestionary_com *gestionary);

#endif