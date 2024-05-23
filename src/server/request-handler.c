#include <mysql.h>
#include <server/database-manager.h>
#include <server/request-handler.h>
#include <types/genericlist.h>
#include <types/p2p-msg.h>
#include <types/packet.h>

void acceptHandler(Packet *p, MYSQL *conn, TLS_infos *info);

void rejecttHandler(Packet *p, MYSQL *conn, TLS_infos *info);

void requestP2PtHandler(Packet *p, MYSQL *conn, TLS_infos *info){
    int user_nb;
    // SQLrequestP2P(conn, p2pMsgGetUserId(&p->p2p), , &user_nb);


    
}

void getAvailableHandler(Packet *p, MYSQL *conn, TLS_infos *info) {
    GenList *res = listUserAvalaible(conn);

    P2P_msg *msg = initP2PMsg(P2P_AVAILABLE);
    p2pMsgSetListUserOnline(msg, res);

    Packet *send = initPacketP2PMsg(msg);

    tlsSend(info, send);

    deinitPacket(&send);
    deinitP2PMsg(&msg);
}
