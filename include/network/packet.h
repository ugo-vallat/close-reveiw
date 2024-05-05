#ifndef __PACKET_H__
#define __PACKET_H__

#include <utils/message.h>

typedef enum e_Packet_type { TXT = 1, FILE_PATH = 2 } Packet_type;

typedef struct s_Packet {
    Packet_type type;
    Msg msg;
} Packet;

#endif