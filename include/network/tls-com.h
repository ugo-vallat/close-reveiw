#ifndef __TLS_COM_H__
#define __TLS_COM_H__

#include <network/manager.h>
#include <openssl/ssl.h>
#include <types/genericlist.h>
#include <utils/project_constants.h>

typedef enum e_tls_error {
    TLS_SUCCESS,      /* success */
    TLS_ERROR,        /* undifined error, check logs for more informations */
    TLS_NULL_POINTER, /* a parameter was NULL */
    TLS_RETRY,        /* nothing done, retry later */
    TLS_CLOSE         /* communication has been closed by the peer, call tlsCloseCom() to close properly */
} TLS_error;

/* function to manage the packets received */
typedef TLS_error (*funTLSPacketReceivedManager)(Manager *manager, Manager_module module, Packet *packet);

/* function get the next packet to send */
typedef TLS_error (*funTLSGetNextPacket)(Manager *manager, Manager_module module, Packet **packet);

typedef enum e_tls_mode { TLS_SERVER = 1, TLS_CLIENT = 2, TLS_MAIN_SERVER = 3 } TLS_mode;

typedef struct s_tls_infos {
    /* info */
    char ip[SIZE_IP_CHAR];
    int port;
    TLS_mode mode;
    char path_cert[SIZE_DIRECTORY];
    char path_key[SIZE_DIRECTORY];
    bool open;

    /* structures */
    SSL_CTX *ctx;
    SSL *ssl;
    int sockfd;
} TLS_infos;

/**
 * @brief Malloc and fill TLS_infos
 * @param[in] ip Server IP
 * @param[in] port Server port
 * @param[in] mode Mode for connection establishment
 * @param[in] path_cert Path to server's certificate (NULL if TLS_CLIENT mode)
 * @param[in] path_key Path to server's private key (NULL if TLS_CLIENT mode)
 * @return TLS_infos*
 * @note Use deleteTLSInfos() to delete this structure
 */
TLS_infos *initTLSInfos(const char *ip, const int port, TLS_mode tls_mode, char *path_cert, char *path_key);

/**
 * @brief Delete structure TLS_infos and free memory
 *
 * @param[in] infos Structure to delete
 */
void deinitTLSInfos(TLS_infos **infos);

/**
 * @brief Establishes a secure communication channel with the remote host
 * @param[in] infos Information about the remote device
 * @param[in] timeout Set timeout (NULL if None)
 * @return TLS_error
 * @note infos in TLS_MAIN_SERVER mode, call tlsAcceptCom
 */
TLS_error tlsOpenCom(TLS_infos *infos, struct timeval *timeout);

/**
 * @brief Accept new TLS connection from client (only for TLS_MAIN_SEVER)
 *
 * @param[in] infos Infos server
 * @return TLS_infos*
 */
TLS_infos *tlsAcceptCom(TLS_infos *infos);

/**
 * @brief Close TLS connection channel and return the last packets received
 *
 * @param[in] infos Struct TLS_infos
 * @param[out] last_received GenList to store last packets received (if NULL packets will be
 * destroyed)
 * @return TLS_error
 */
TLS_error tlsCloseCom(TLS_infos *infos, GenList *last_received);

/**
 * @brief Call tlsOpenCom if communication is closed and start sending/receiving packets
 *
 * @param infos Struct TLS_infos
 * @param manager Manager
 * @param module Manager module
 * @param[in] next_packet Function use to get the next packet to send (must be non-blocking, NULL if
 * TLS_MAIN_SERVER mode)
 * @param[in] MSG_manager Manager of Msg Packets (NULL if TLS_MAIN_SERVER mode)
 * @param[in] P2P_manager Manager of P2P_msg Packets (NULL if TLS_MAIN_SERVER mode)
 * @return TLS_error
 */
TLS_error tlsStartListenning(TLS_infos *infos, Manager *manager, Manager_module module, funTLSGetNextPacket next_packet,
                             funTLSPacketReceivedManager packet_manager_received);

TLS_error tlsManagerPacketReceived(Manager *manager, Manager_module module, Packet *packet);

/**
 * @brief Function for tlsStartListenning, read the manager and send the received packet to tls
 *
 * @param[in] manager Manager
 * @param[in] module Module to read
 * @param[out] packet Buffer packet
 * @return TLS_error
 */
TLS_error tlsManagerPacketGetNext(Manager *manager, Manager_module module, Packet **packet);

/**
 * @brief Send packet on tls communication
 *
 * @param[in] infos TLS_infos
 * @param[in] packet Packet to send
 * @return TLS_error {TLS_CLOSE, TLS_ERROR}
 */
TLS_error tlsSend(TLS_infos *infos, Packet *packet);

/**
 * @brief Return the oldest packet received or TLS_RETRY if nothing to receive
 *
 * @param[in] infos TLS_infos
 * @param[out] packet Packet received, NULL if nothing
 * @return TLS_error
 */
TLS_error tlsReceiveNonBlocking(TLS_infos *infos, Packet **packet);

/**
 * @brief Return the oldest packet received
 *
 * @param[in] infos TLS_infos
 * @param[out] packet Packet received
 * @return TLS_error
 */
TLS_error tlsReceiveBlocking(TLS_infos *infos, Packet **packet);

/**
 * @brief Wait a packet on multiple tls socket
 *
 * @param[in] infos List of TLS_infos
 * @param[in] timeout  Timeout in milliseconds or -1
 * @return TLS_SUCCESS if any packet received, TLS_error otherwise
 */
TLS_error tlsWaitOnMultiple(GenList *infos, int timeout_ms);

/**
 * @brief Return the string associated to the TLS_error
 *
 * @param error TLS_error
 * @return const char*
 */
char *tlsErrorToString(TLS_error error);

#endif
