#ifndef REQUESTE_HANDLER_H
#define REQUESTE_HANDLER_H

#include <mysql.h>
#include <types/packet.h>
#include <network/tls-com.h>

void acceptHandler(Packet *p, MYSQL *conn, TLS_infos *info);

void rejecttHandler(Packet *p, MYSQL *conn, TLS_infos *info);

void requestP2PtHandler(Packet *p, MYSQL *conn, TLS_infos *info);

void getAvailableHandler(Packet *p, MYSQL *conn, TLS_infos *info);

#endif