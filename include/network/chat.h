#ifndef __CHAT_H__
#define __CHAT_H__

#include <network/manager.h>

/**
 * @brief Send message to peer and output
 *
 * @param manager Manager
 * @param txt_msg Message to send (char*)
 */
void chatSendMessage(Manager *manager, char *txt_msg);

#endif
