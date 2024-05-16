#ifndef __PACKET_H__
#define __PACKET_H__

#include <utils/message.h>
#include <utils/token.h>

#define SIZE_DATA_PACKET 4096

typedef enum e_Packet_type { MSG = 1, P2P = 2, TXT = 3 } Packet_type;

typedef struct s_Packet {
    Packet_type type;
    unsigned long size;
    unsigned char data[SIZE_DATA_PACKET];
} Packet;

/**
 * @brief Create a Packet object that can contain an object in the Packet_type enum
 *
 * @param type Type of object contained in the packet
 * @param buff Object
 * @param size Size of object
 * @return Packet* if success, NULL otherwise
 */
Packet *initPacket(Packet_type type, void *buff, unsigned long size);

/**
 * @brief Delete packet
 *
 * @param p Packet to delete
 */
void deinitPacket(Packet *p);

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
