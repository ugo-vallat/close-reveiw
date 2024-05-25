#include <network/chat.h>
#include <network/manager.h>
#include <stdlib.h>
#include <types/message.h>
#include <types/packet.h>
#include <utils/logger.h>

#define FILE_CHAT "chat.c"

void chatSendMessage(Manager *manager, char *txt_msg) {
    char *FUN_NAME = "sendMessage";
    assertl(manager, FILE_CHAT, FUN_NAME, -1, "manager NULL");
    assertl(txt_msg, FILE_CHAT, FUN_NAME, -1, "txt_msg NULL");

    if (managerGetState(manager, MANAGER_MOD_PEER) != MANAGER_STATE_OPEN) {
        warnl(FILE_CHAT, FUN_NAME, "manager peer not open");
        return;
    }

    Packet *packet;
    Msg *msg;
    Manager_error error;
    char *sender = managerGetUser(manager);

    /* create msg */
    msg = initMsg(sender, txt_msg);
    packet = initPacketMsg(msg);
    free(sender);
    deinitMsg(&msg);

    /* send message */
    error = managerSend(manager, MANAGER_MOD_PEER, packet);
    if (error != MANAGER_ERR_SUCCESS) {
        warnl(FILE_CHAT, FUN_NAME, "failed to send msg to peer : %s", managerErrorToString(error));
        deinitPacket(&packet);
        return;
    }
    error = managerSend(manager, MANAGER_MOD_OUTPUT, packet);
    if (error != MANAGER_ERR_SUCCESS) {
        warnl(FILE_CHAT, FUN_NAME, "failed to send msg to output : %s", managerErrorToString(error));
    }
    deinitPacket(&packet);
}
