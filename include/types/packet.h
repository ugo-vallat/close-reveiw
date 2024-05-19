#ifndef __PACKET_H__
#define __PACKET_H__

#include <types/message.h>
#include <types/p2p-msg.h>
#include <utils/const-define.h>
#include <utils/token.h>

typedef enum e_packet_type { PACKET_TXT, PACKET_MSG, PACKET_P2P_MSG } Packet_type;

typedef struct s_packet {
    Packet_type type;
    union {
        char txt[SIZE_TXT];
        Msg msg;
        P2P_msg p2p;
    };
} Packet;

/**
 * @brief Create a Packet object that can contain an object in the Packet_type enum
 *
 * @return Packet* if success, NULL otherwise
 */
Packet *initPacketTXT(char *txt);

Packet *initPacketMsg(Msg *msg);

Packet *initPacketP2PMsg(P2P_msg *msg);

/**
 * @brief Delete packet
 *
 * @param p Packet to delete
 */
void deinitPacket(Packet **p);

/**
 * @brief Delete packet as void*
 *
 * @param p Packet
 */
void deinitPacketGen(void *p);

/**
 * @brief Create new packet with data duplicated
 *
 * @param p Packet to copy
 * @return Copy of the packet, NULL if error
 */
Packet *packetCopy(Packet *p);

#endif
