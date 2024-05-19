#ifndef __TLS_COM_H__
#define __TLS_COM_H__

#include <network/manager.h>
#include <openssl/ssl.h>

/* function to write packets received in the manager */
typedef void (*funPacketManagerOut)(Manager *manager, Packet *packet);

typedef enum e_tls_mode { TLS_SERVER = 1, TLS_CLIENT = 2, TLS_MAIN_SERVER = 3 } TLS_mode;

typedef struct s_tls_infos {
    /* info */
    char *ip;
    int port;
    TLS_mode mode;
    char *path_cert;
    char *path_key;
    bool end;

    /* structures */
    SSL_CTX *ctx;
    SSL *ssl;
    int sockfd;

    /* packetManager */
    Manager_module module;
    funPacketManagerOut P2P_manager;
    funPacketManagerOut MSG_manager;

} TLS_infos;

typedef enum e_tls_error {
    TLS_SUCCESS = 0,       /* success */
    TLS_ERROR = -1,        /* undifined error, check logs for more informations */
    TLS_NULL_POINTER = -2, /* a parameter was NULL */
} TLS_error;

/**
 * @brief Malloc and fill TLS_infos
 * @param[in] ip Server IP
 * @param[in] port Server port
 * @param[in] mode Mode for connection establishment
 * @param[in] path_cert Path to server's certificate (NULL if CLIENT mode)
 * @param[in] path_key Path to server's private key (NULL if CLIENT mode)
 * @param[in] Module Module to get read the pakect
 * @param[in] MSG_manager Manager of Msg Packets
 * @param[in] P2P_manager Manager of P2P_msg Packets
 * @return TLS_infos*
 * @note Use deleteTLSInfos() to delete this structure
 */
TLS_infos *initTLSInfos(const char *ip, const int port, TLS_mode tls_mode, char *path_cert,
                        char *path_key, Manager_module module, funPacketManagerOut MSG_manager,
                        funPacketManagerOut P2P_manager);

/**
 * @brief Delete structure TLS_infos and free memory
 *
 * @param[in] infos Structure to delete
 */
TLS_error deinitTLSInfos(TLS_infos **infos);

/**
 * @brief Establishes a secure communication channel with the remote host
 * @param[in] infos Information about the remote device
 * @param[in] timeout Set timeout in server mode
 * @return TLS_error
 */
TLS_error tlsOpenCom(TLS_infos *infos, struct timeval *timeout);

/**
 * @brief Accept new TLS connection from client (only for main server)
 *
 * @param[in] infos Infos server
 * @return TLS_infos*
 */
TLS_infos *tlsAcceptCom(TLS_infos *infos);

/**
 * @brief Close the TLS Manager module (SERVER or PEER) and shutdown communication
 *
 * @param[in] Manager manager
 * @param[in] module TLS module to close
 * @return TLS_error
 */
TLS_error tlsCloseTLSManagerModule(Manager *manager, Manager_module module);

#endif
