#include "types/genericlist.h"
#include "types/p2p-msg.h"
#include "types/packet.h"
#include <server/requeste-handler.h>
#include <server/database-manager.h>
#include <mysql.h>

void acceptHandler(Packet *p, MYSQL *conn, TLS_infos *info);

void rejecttHandler(Packet *p, MYSQL *conn, TLS_infos *info);

void requestP2PtHandler(Packet *p, MYSQL *conn, TLS_infos *info);

void getAvailableHandler(Packet *p, MYSQL *conn, TLS_infos *info){
    GenList *res = listUserAvalaible(conn);

    P2P_msg *msg = initP2PMsg(P2P_AVAILABLE);
    p2pMsgSetListUserOnline(msg, res);

    Packet *send = initPacketP2PMsg(msg);

    tlsSend(info,send);

    deinitPacket(&send);
    deinitP2PMsg(&msg);
    deinitGenList(&res, free);


}