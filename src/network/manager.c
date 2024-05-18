
#include "network/p2p-msg.h"
#include "utils/genericlist.h"
#include "utils/logger.h"
#include "utils/token.h"
#include <network/gestionary-com.h>
#include <network/p2p-com.h>
#include <network/packet.h>
#include <network/tls-com.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/message.h>

Gestionary_com *createGestionaryCom(TLS_infos *tls) {
    if (!tls) {
        warnl("gestionary-com.c", "createGestionaryCom", "tls null");
        return NULL;
    }
    Gestionary_com *gest = malloc(sizeof(Gestionary_com));
    if (!gest) {
        warnl("gestionary-com.c", "createGestionaryCom", "error malloc Gestionary_com");
        return NULL;
    }
    gest->tls = tls;
    gest->msgReceived = createGenList(8);
    gest->p2pReceived = createGenList(8);
    gest->txtReceived = createGenList(8);
    gest->open = true;
    return gest;
}

void freeMsg(void *msg) {
    deleteMsg((Msg *)msg);
}

void freePacket(void *packet) {
    deletePacket((Packet *)packet);
}

void deleteGestionaryCom(Gestionary_com **gestionary) {
    deleteTLSInfos(&((*gestionary)->tls));
    deleteGenList(&((*gestionary)->msgReceived), deleteMsgGen);
    deleteGenList(&((*gestionary)->p2pReceived), deleteP2PMsgGen);
    deleteGenList(&((*gestionary)->txtReceived), free);
    *gestionary = NULL;
}

void addToBufferReceived(Gestionary_com *gestionary, Packet *packet) {
    Msg *msg;
    P2P_msg *p2p;
    char *txt;
    switch (packet->type) {
    case MSG:
        msg = malloc(sizeof(Msg));
        memcpy(msg, packet->data, sizeof(Msg));
        genListAdd(gestionary->msgReceived, (void *)msg);
        break;
    case P2P:
        p2p = malloc(sizeof(P2P_msg));
        memcpy(p2p, packet->data, sizeof(P2P_msg));
        genListAdd(gestionary->p2pReceived, p2p);
        break;
    case TXT:
        txt = malloc(packet->size + 1);
        memcpy(txt, packet->data, packet->size);
        genListAdd(gestionary->txtReceived, txt);
        break;
    }
}

void gestionaryCloseComTLS(Gestionary_com *gestionary) {
    GenList *last;
    gestionary->open = false;
    closeComTLS(gestionary->tls, &last);
    while (!genListIsEmpty(last)) {
        addToBufferReceived(gestionary, genListPop(last));
    }
}

int updateReceivedBuffer(Gestionary_com *gestionary) {
    int ret;
    Packet *p;
    while (gestionary->open && isPacketReceived(gestionary->tls)) {
        ret = receivePacket(gestionary->tls, &p);
        if (ret == TLS_SUCCESS) {
            addToBufferReceived(gestionary, p);
        } else if (ret == TLS_CLOSE) {
            gestionaryCloseComTLS(gestionary);
        } else {
            warnl("gestionary-com.c", "updateRaceivedBuffer", "error receivePacket");
            return -1;
        }
    }
    return 0;
}

int gestionarySendMsg(Gestionary_com *gestionary, Msg *msg) {
    if (!gestionary->open) {
        return -1;
    }
    Packet *p = createPacket(MSG, msg, sizeof(Msg));
    if (!p) {
        warnl("gestionary_com.c", "gestionarySendMsg", "fail create packet");
        return -1;
    }
    sendPacket(gestionary->tls, p);
    return 0;
}

int gestionarySendP2P(Gestionary_com *gestionary, P2P_msg *msg) {
    if (!gestionary->open) {
        return -1;
    }
    // char *data = p2pMsgToChar(msg);
    Packet *p = createPacket(P2P, msg, sizeof(P2P_msg));
    if (!p) {
        warnl("gestionary_com.c", "gestionarySendP2P", "fail create packet");
        return -1;
    }
    sendPacket(gestionary->tls, p);
    return 0;
}

int gestionarySendTxt(Gestionary_com *gestionary, char *txt) {
    if (!gestionary->open) {
        return -1;
    }
    Packet *p = createPacket(TXT, txt, strlen(txt) + 1);
    if (!p) {
        warnl("gestionary_com.c", "gestionarySendTxt", "fail create packet");
        return -1;
    }
    sendPacket(gestionary->tls, p);
    return 0;
}

int gestionaryReceiveMsg(Gestionary_com *gestionary, Msg **msg) {
    updateReceivedBuffer(gestionary);

    if (!genListIsEmpty(gestionary->msgReceived)) {
        *msg = (Msg *)genListRemove(gestionary->msgReceived, 0);
        return 1;
    }
    *msg = NULL;
    return 0;
}

int gestionaryReceiveP2P(Gestionary_com *gestionary, P2P_msg **msg) {
    updateReceivedBuffer(gestionary);
    if (!genListIsEmpty(gestionary->p2pReceived)) {
        *msg = (P2P_msg *)genListRemove(gestionary->p2pReceived, 0);
        return 1;
    }
    *msg = NULL;
    return 0;
}

int gestionaryReceiveTxt(Gestionary_com *gestionary, char **txt) {
    updateReceivedBuffer(gestionary);
    if (!genListIsEmpty(gestionary->txtReceived)) {
        *txt = (char *)genListRemove(gestionary->txtReceived, 0);
        return 1;
    }
    *txt = NULL;
    return 0;
}

bool gestionaryIsComOpen(Gestionary_com *gestionary) {
    return gestionary->open;
}
