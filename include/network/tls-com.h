#ifndef __TLS_COM_H__
#define __TLS_COM_H__

#include "utils/genericlist.h"
#include <arpa/inet.h>
#include <network/packet.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdbool.h>

#define PACKET_BUFF_SIZE 512

#define CHAR_IP_SIZE 32

typedef struct s_tls_infos TLS_infos;

typedef enum e_mode { SERVER = 1, CLIENT = 2 } Mode;

typedef enum e_tls_error {
    TLS_SUCCESS = 0,       /* success */
    TLS_ERROR = -1,        /* undifined error, check logs for more informations */
    TLS_NULL_POINTER = -2, /* a parameter was NULL */
    TLS_CLOSE = -3,        /* communication is close, you should call closeComTLS */
    TLS_DISCONNECTED = -4, /* not connected to the peer, call openComTLS */
    TLS_RETRY = -5         /* retry later */
} TLS_error;

/**
 * @brief Malloc and fill TLS_infos
 * @param[in] ip Server IP
 * @param[in] port Server port
 * @param[in] path_cert Path to server's certificate (NULL if CLIENT mode)
 * @param[in] path_key Path to server's private key (NULL if CLIENT mode)
 * @return TLS_infos*
 * @note Use deleteTLSInfos() to delete this structure
 */
TLS_infos *initTLSInfos(const char *ip, const int port, Mode mode, char *path_cert, char *path_key);

/**
 * @brief Delete structure TLS_infos and free memory
 *
 * @param[in] infos Structure to delete
 */
TLS_error deleteTLSInfos(TLS_infos **infos);

/**
 * @brief Establishes a secure communication channel with the remote host
 * @param[in] infos Information about the remote device
 * @return TLS_error
 */
int openComTLS(TLS_infos *infos);

/**
 * @brief Closes the communication channel
 * @param[in] infos Channel to close
 * @param[out] lastReceived List of last packets received (NULL if ignored)
 * @return TLS_error
 */
TLS_error closeComTLS(TLS_infos *infos, GenList **lastReceived);

/**
 * @brief Sends the packet to the remote host
 * @param[in] infos Communication channel
 * @param[in] p Packet to send
 * @return TLS_error
 */
TLS_error sendPacket(TLS_infos *infos, Packet *p);

/**
 * @brief Indicates if packets have been received
 * @param[in] infos Communication channel
 * @return true if packets are pending, false otherwise
 */
bool isPacketReceived(TLS_infos *infos);

/**
 * @brief Stores the oldest received packet in p
 *
 * @param[in] infos Communication channel
 * @param[out] p Buffer to retrieve the packet
 * @return TLS_error
 */
TLS_error receivePacket(TLS_infos *infos, Packet **p);

#endif
