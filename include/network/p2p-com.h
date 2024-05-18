#ifndef __P2P_COM_H__
#define __P2P_COM_H__

#include <network/manager.h>
#include <types/genericlist.h>

GenList *p2pGetUserAvailable(Manager *manager);

int p2pSendRequestConnection(Manager *manager, char *id_user);

int p2pRespondToRequest(Manager *manager, char *id_user, bool respond);

#endif