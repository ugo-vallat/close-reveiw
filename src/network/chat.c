#include <network/chat.h>
#include <network/manager.h>
#include <stdlib.h>
#include <types/message.h>
#include <types/packet.h>
#include <utils/logger.h>


void chatSendMessage(Manager *manager, char *txt_msg) {
    ASSERTL(manager,-1, "manager NULL")
    ASSERTL(txt_msg,-1, "txt_msg NULL")

    if (managerGetState(manager, MANAGER_MOD_PEER) != MANAGER_STATE_OPEN) {
        WARNL("manager peer not open")
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
        WARNL("failed to send msg to peer : %s", managerErrorToString(error))
        deinitPacket(&packet);
        return;
    }
    error = managerSend(manager, MANAGER_MOD_OUTPUT, packet);
    if (error != MANAGER_ERR_SUCCESS) {
        WARNL("failed to send msg to output : %s", managerErrorToString(error))
    }
    deinitPacket(&packet);
}
