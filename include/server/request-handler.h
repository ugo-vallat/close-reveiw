#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "types/list.h"
#include <mariadb/mysql.h>
#include <types/packet.h>
#include <types/packet.h>
#include <types/clientlist.h>

extern MYSQL *conn;
extern ClientList *user;
extern List *thread;
extern pthread_t nb_main;


extern bool end;

void acceptHandler(Packet *p, Client *c); // TODO

void rejecttHandler(Packet *p, Client *c);

void requestP2PtHandler(Packet *p, Client *c);

void getAvailableHandler(Packet *p, Client *c);

void *startConnection(void *arg);

void *accepteUser(void *arg);

void *requestHandler(void *arg);

bool genListContainsString(GenList *l, char *name);

void tryServer(char *arg);

void okServer(char *arg);

void koServer(char *arg);

#endif