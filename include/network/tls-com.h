#ifndef __TLS_COM_H__
#define __TLS_COM_H__

#include <arpa/inet.h>
#include <network/packet.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdbool.h>

#define PACKET_BUFF_SIZE 512

#define CHAR_IP_SIZE 32

typedef struct s_tls_infos TLSInfos;

typedef enum e_mode { SERVER = 1, CLIENT = 2 } Mode;

/**
 * @brief Malloc and fill TLSInfos
 * @param[in] ip Server IP
 * @param[in] port Server port
 * @param[in] path_cert Path to server's certificate (NULL if CLIENT mode)
 * @param[in] path_key Path to server's private key (NULL if CLIENT mode)
 * @return TLSInfos*
 * @note Use deleteTLSInfos() to delete this structure
 */
TLSInfos *initTLSInfos(const char *ip, const int port, Mode mode, char *path_cert, char *path_key);

/**
 * @brief Delete structure TLSInfos and free memory
 *
 * @param[in] infos Structure to delete
 */
void deleteTLSInfos(TLSInfos **infos);

/**
 * @brief Establishes a secure communication channel with the remote host
 * @param[in] infos Information about the remote device
 * @return 0 on success, -1 otherwise
 */
int openComTLS(TLSInfos *infos);

/**
 * @brief Closes the communication channel
 * @param[in] infos Channel to close
 * @return 0 on success, -1 otherwise
 */
int closeComTLS(TLSInfos *infos);

/**
 * @brief Sends the packet to the remote host
 * @param[in] infos Communication channel
 * @param[in] p Packet to send
 * @return 0 on success, -1 otherwise
 */
int sendPacket(TLSInfos *infos, Packet *p);

/**
 * @brief Indicates if packets have been received
 * @param[in] infos Communication channel
 * @return true if packets are pending, false otherwise
 */
bool isBufferPacketEmpty(TLSInfos *infos);

/**
 * @brief Stores the oldest received packet in p
 *
 * @param[in] infos Communication channel
 * @param[out] p Buffer to retrieve the packet
 * @return 0 on success, -1 otherwise
 */
int receivePacket(TLSInfos *infos, Packet **p);

#endif
