#include "network/manager.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <network/tls-com.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <types/genericlist.h>
#include <types/message.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
#include <unistd.h>
#include <utils/logger.h>
#include <utils/project_constants.h>

#define ERROR_BUFF_SIZE 512

#define FILE_TLS_COM "tls-com.c"

TLS_infos *initTLSInfos(const char *ip, const int port, TLS_mode tls_mode, char *path_cert, char *path_key) {
    char FUN_NAME[32] = "initTLSInfos";
    if (tls_mode == TLS_CLIENT)
        assertl(ip, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "ip NULL");
    assertl(port > 0 & port < 65535, FILE_TLS_COM, FUN_NAME, TLS_ERROR, "invalid num port <%d>", port);
    assertl(tls_mode == TLS_CLIENT || tls_mode == TLS_SERVER || tls_mode == TLS_MAIN_SERVER, FILE_TLS_COM, FUN_NAME,
            TLS_ERROR, "invalid tls mode <%d>", tls_mode);
    if (tls_mode == TLS_SERVER || tls_mode == TLS_MAIN_SERVER) {
        assertl(path_cert, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "path_cert NULL");
        assertl(path_key, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "path_key NULL");
    }
    TLS_infos *tls = malloc(sizeof(TLS_infos));
    if (!tls) {
        warnl(FILE_TLS_COM, FUN_NAME, "fail malloc INFO_tls struct");
        return NULL;
    }

    memset(tls, 0, sizeof(TLS_infos));
    tls->port = port;
    if (tls_mode == TLS_CLIENT)
        strncpy(tls->ip, ip, SIZE_IP_CHAR);
    tls->mode = tls_mode;
    tls->open = false;

    tls->ctx = NULL;
    tls->ssl = NULL;
    tls->sockfd = -1;

    if (tls_mode == TLS_SERVER || tls_mode == TLS_MAIN_SERVER) {
        strcpy(tls->path_cert, path_cert);
        strcpy(tls->path_key, path_key);
    }
    return tls;
}

void deinitTLSInfos(TLS_infos **infos) {
    char FUN_NAME[32] = "deinitTLSInfos";
    assertl(infos, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "infos NULL");
    assertl(*infos, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "*infos NULL");

    TLS_infos *tls = *infos;

    if (tlsCloseCom(tls, NULL) != TLS_SUCCESS) {
        warnl(FILE_TLS_COM, FUN_NAME, "error closing tls com");
    }

    free(tls);
    *infos = NULL;
}

TLS_error tlsOpenCom(TLS_infos *infos, struct timeval *timeout) {
    char FUN_NAME[32] = "tlsOpenCom";
    assertl(infos, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "infos NULL");
    assertl(infos->mode != TLS_MAIN_SERVER, FILE_TLS_COM, FUN_NAME, TLS_ERROR,
            "cant call tlsOpenCom in TLS_MAIN_SERVER mode");

    /* close old connexion */
    if (tlsCloseCom(infos, NULL) != TLS_SUCCESS) {
        warnl(FILE_TLS_COM, FUN_NAME, "fail tlsCloseCom");
    }

    int ret;
    int result;
    struct sockaddr_in serv_addr, cli_addr;

    /* Open socket */
    infos->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (infos->sockfd == -1) {
        warnl(FILE_TLS_COM, FUN_NAME, "error open socket");
        tlsCloseCom(infos, NULL);
        return TLS_ERROR;
    }

    /* create addr */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(infos->port);
    if (infos->mode == TLS_SERVER) {
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, infos->ip, &serv_addr.sin_addr) <= 0) {
            warnl(FILE_TLS_COM, FUN_NAME, "error ip");
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        }
    }

    /* init context*/
    if (infos->mode == TLS_SERVER) {
        infos->ctx = SSL_CTX_new(TLS_server_method());
    } else {
        infos->ctx = SSL_CTX_new(TLS_client_method());
    }
    if (infos->ctx == NULL) {
        warnl(FILE_TLS_COM, FUN_NAME, "error context");
        ERR_print_errors_fp(stderr);
        tlsCloseCom(infos, NULL);
        return TLS_ERROR;
    }

    /* add certificate and key for server */
    if (infos->mode == TLS_SERVER) {
        ret = SSL_CTX_use_certificate_file(infos->ctx, infos->path_cert, SSL_FILETYPE_PEM);
        if (ret != 1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error add cert file (%s)", infos->path_cert);
            ERR_print_errors_fp(stderr);
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        }
        ret = SSL_CTX_use_PrivateKey_file(infos->ctx, infos->path_key, SSL_FILETYPE_PEM);
        if (ret != 1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error add key file(%s)", infos->path_key);
            ERR_print_errors_fp(stderr);
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        }
    }

    /* bind socket server*/
    if (infos->mode == TLS_SERVER) {
        if (bind(infos->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error bind server");
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        }
        if (listen(infos->sockfd, 5) == -1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error listen server");
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        }
        if (timeout) {
            if (setsockopt(infos->sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)timeout, sizeof(struct timeval)) < 0) {
                warnl(FILE_TLS_COM, FUN_NAME, "error set timeout");
                tlsCloseCom(infos, NULL);
                return TLS_ERROR;
            }
        }
    }

    /* connexion client / server */
    if (infos->mode == TLS_SERVER) {
        socklen_t cli_len = sizeof(cli_addr);
        int client_sockfd = accept(infos->sockfd, (struct sockaddr *)&cli_addr, &cli_len);
        if (client_sockfd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                warnl(FILE_TLS_COM, FUN_NAME, "accept() stoped by timeout");
                return TLS_RETRY;
            }
            warnl(FILE_TLS_COM, FUN_NAME, "error accept client");
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        } else {
            close(infos->sockfd);
            infos->sockfd = client_sockfd;
        }
    } else {
        if (timeout) {
            struct timeval start_time, current_time;
            int connected = 0;
            long elapsed_seconds, elapsed_microseconds;
            gettimeofday(&start_time, NULL);

            while (!connected) {
                int result = connect(infos->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                if (result == 0) {
                    /* connection success */
                    connected = 1;
                } else {
                    /* connectio failed */
                    switch (errno) {
                    case EINPROGRESS:
                    case EALREADY:
                    case EISCONN:
                    case ECONNREFUSED:
                        break;
                    default:
                        warnl(FILE_TLS_COM, FUN_NAME, "error connect to server");
                        tlsCloseCom(infos, NULL);
                        return TLS_ERROR;
                    }

                    gettimeofday(&current_time, NULL);
                    elapsed_seconds = current_time.tv_sec - start_time.tv_sec;
                    elapsed_microseconds = current_time.tv_usec - start_time.tv_usec;
                    if (elapsed_seconds >= timeout->tv_sec ||
                        (elapsed_seconds == timeout->tv_sec && elapsed_microseconds > timeout->tv_usec)) {
                        warnl(FILE_TLS_COM, FUN_NAME, "connect() stoped by timeout");
                        tlsCloseCom(infos, NULL);
                        return TLS_ERROR;
                    }
                    usleep(500000); // wait 0.5s
                }
            }
        } else {
            if (connect(infos->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    warnl(FILE_TLS_COM, FUN_NAME, "connect() stoped by timeout");
                    return TLS_RETRY;
                }
                warnl(FILE_TLS_COM, FUN_NAME, "error connect to server");
                tlsCloseCom(infos, NULL);
                return TLS_ERROR;
            }
        }
    }

    /* create SSL connexion */
    infos->ssl = SSL_new(infos->ctx);
    if (!infos->ssl) {
        warnl(FILE_TLS_COM, FUN_NAME, "error SSL_new");
        ERR_print_errors_fp(stderr);
        tlsCloseCom(infos, NULL);
        return TLS_ERROR;
    }
    if (SSL_set_fd(infos->ssl, infos->sockfd) != 1) {
        warnl(FILE_TLS_COM, FUN_NAME, "error SSL_set_fd");
        ERR_print_errors_fp(stderr);
        tlsCloseCom(infos, NULL);
        return TLS_ERROR;
    }

    /* establish SSL connexion */
    if (infos->mode == TLS_SERVER) {
        if (SSL_accept(infos->ssl) != 1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error SSL_accept");
            ERR_print_errors_fp(stderr);
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        }
    } else {
        if (SSL_connect(infos->ssl) != 1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error SSL_connect");
            ERR_print_errors_fp(stderr);
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        }
    }

    /* non blocking socket */
    int flags = fcntl(infos->sockfd, F_GETFL, 0);
    if (fcntl(infos->sockfd, F_SETFL, flags | O_NONBLOCK) != 0) {
        warnl(FILE_TLS_COM, FUN_NAME, "error nonblock socket");
        close(infos->sockfd);
        infos->sockfd = -1;
        return TLS_ERROR;
    }

    infos->open = true;

    return TLS_SUCCESS;
}

TLS_infos *tlsAcceptCom(TLS_infos *infos) {
    char FUN_NAME[32] = "tlsAcceptCom";
    assertl(infos, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "infos NULL");
    assertl(infos->mode == TLS_MAIN_SERVER, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER,
            "call tlsAcceptCom only in TLS_MAIN_SERVER mode");

    int ret;
    struct sockaddr_in serv_addr, cli_addr;
    TLS_infos *client;

    /* create addr */
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(infos->port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (infos->sockfd == -1) {
        /* Open socket */
        infos->sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (infos->sockfd == -1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error open socket");
            tlsCloseCom(infos, NULL);
            return NULL;
        }
        /* bind socket server*/
        if (bind(infos->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error bind server");
            tlsCloseCom(infos, NULL);
            return NULL;
        }
        if (listen(infos->sockfd, 99999) == -1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error listen server");
            tlsCloseCom(infos, NULL);
            return NULL;
        }
    }

    /* init context*/
    if (infos->ctx == NULL) {
        infos->ctx = SSL_CTX_new(TLS_server_method());
        if (infos->ctx == NULL) {
            warnl(FILE_TLS_COM, FUN_NAME, "error context");
            ERR_print_errors_fp(stderr);
            tlsCloseCom(infos, NULL);
            return NULL;
        }

        /* add certificate and key for server */
        ret = SSL_CTX_use_certificate_file(infos->ctx, infos->path_cert, SSL_FILETYPE_PEM);
        if (ret != 1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error add cert file (%s)", infos->path_cert);
            ERR_print_errors_fp(stderr);
            tlsCloseCom(infos, NULL);
            return NULL;
        }
        ret = SSL_CTX_use_PrivateKey_file(infos->ctx, infos->path_key, SSL_FILETYPE_PEM);
        if (ret != 1) {
            warnl(FILE_TLS_COM, FUN_NAME, "error add key file(%s)", infos->path_key);
            ERR_print_errors_fp(stderr);
            tlsCloseCom(infos, NULL);
            return NULL;
        }
    }

    /* connexion client / server */
    socklen_t cli_len = sizeof(cli_addr);
    int client_sockfd = accept(infos->sockfd, (struct sockaddr *)&cli_addr, &cli_len);
    if (client_sockfd == -1) {
        warnl(FILE_TLS_COM, FUN_NAME, "error accept client");
        tlsCloseCom(infos, NULL);
        return NULL;
    } else {
        client = initTLSInfos(infos->ip, infos->port, TLS_SERVER, infos->path_cert, infos->path_key);
        client->sockfd = client_sockfd;
    }

    /* create SSL connexion */
    client->ssl = SSL_new(infos->ctx);
    if (!client->ssl) {
        warnl(FILE_TLS_COM, FUN_NAME, "error SSL_new");
        ERR_print_errors_fp(stderr);
        return NULL;
    }
    if (SSL_set_fd(client->ssl, client->sockfd) != 1) {
        warnl(FILE_TLS_COM, FUN_NAME, "error SSL_set_fd");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    /* establish SSL connexion */
    if (SSL_accept(client->ssl) != 1) {
        warnl(FILE_TLS_COM, FUN_NAME, "error SSL_accept");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    /* non blocking socket */
    int flags = fcntl(client->sockfd, F_GETFL, 0);
    if (fcntl(client->sockfd, F_SETFL, flags | O_NONBLOCK) != 0) {
        warnl(FILE_TLS_COM, FUN_NAME, "error nonblock socket");
        close(client->sockfd);
        client->sockfd = -1;
        return NULL;
    }

    client->open = true;

    return client;
}

TLS_error tlsCloseCom(TLS_infos *infos, GenList *last_received) {
    char FUN_NAME[32] = "tlsCloseCom";
    assertl(infos, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "infos NULL");

    TLS_error error = TLS_SUCCESS;

    if (infos->ssl) {
        /* close ssl write */
        SSL_shutdown(infos->ssl);

        /* read last packets */
        Packet *packet = NULL;
        while (error == TLS_SUCCESS || error == TLS_RETRY) {
            error = tlsReceiveNonBlocking(infos, &packet);
            if (last_received && error == TLS_SUCCESS) {
                genListAdd(last_received, packet);
            } else if (error == TLS_SUCCESS) {
                deinitPacket(&packet);
            }
        }
    }

    /* free ssl */
    if (infos->ssl) {
        SSL_free(infos->ssl);
        infos->ssl = NULL;
    }

    /* close ctx */
    if (infos->ctx) {
        SSL_CTX_free(infos->ctx);
        infos->ctx = NULL;
    }

    /* close socket */
    if (infos->sockfd != -1) {
        if (close(infos->sockfd) != 0)
            warnl(FILE_TLS_COM, FUN_NAME, "error close socket");
        infos->sockfd = -1;
    }
    infos->open = false;
    return TLS_SUCCESS;
}

TLS_error tlsStartListenning(TLS_infos *infos, Manager *manager, Manager_module module, funTLSGetNextPacket next_packet,
                             funTLSPacketReceivedManager packet_manager_received) {
    char FUN_NAME[32] = "tlsStartListenning";
    assertl(infos, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "infos NULL");
    assertl(manager, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "manager NULL");
    assertl(next_packet, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "next_packet NULL");
    assertl(packet_manager_received, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "packet_manager_received NULL");

    TLS_error tls_error;
    Packet *p;

    if (!infos->open) {
        tls_error = tlsOpenCom(infos, NULL);
        if (tls_error != TLS_SUCCESS) {
            warnl(FILE_TLS_COM, FUN_NAME, "fail to open com tls");
            return TLS_ERROR;
        }
    }

    while (1) {
        /* send next packet */
        tls_error = next_packet(manager, module, &p);
        switch (tls_error) {
        case TLS_SUCCESS:
            tlsSend(infos, p);
            break;
        case TLS_RETRY:
            break;
        case TLS_CLOSE:
            /* end of communication */
            // warnl(FILE_TLS_COM, FUN_NAME, "close 1");
            tlsCloseCom(infos, NULL);
            return TLS_CLOSE;
        case TLS_ERROR:
        case TLS_NULL_POINTER:
            warnl(FILE_TLS_COM, FUN_NAME, "fail next_packet");
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        }

        /* receive packet */
        tls_error = tlsReceiveNonBlocking(infos, &p);
        switch (tls_error) {
        case TLS_SUCCESS:
            switch (p->type) {
            case PACKET_MSG:
            case PACKET_P2P_MSG:
            case PACKET_TXT:
                tls_error = packet_manager_received(manager, module, p);
                switch (tls_error) {
                case TLS_SUCCESS:
                    break;
                case TLS_CLOSE:
                    // warnl(FILE_TLS_COM, FUN_NAME, "close 2");
                    return TLS_CLOSE;
                default:
                    warnl(FILE_TLS_COM, FUN_NAME, "%s - packet_manager_receive failed", tlsErrorToString(tls_error));
                    break;
                }
                break;
            default:
                warnl(FILE_TLS_COM, FUN_NAME, "unexpected type <%d>", p->type);
            }
            deinitPacket(&p);
            break;
        case TLS_RETRY:
            break;
        case TLS_CLOSE:
            warnl(FILE_TLS_COM, FUN_NAME, "peer disconnected");
            return TLS_CLOSE;
        case TLS_ERROR:
        case TLS_NULL_POINTER:
            warnl(FILE_TLS_COM, FUN_NAME, "fail tlsReceiveNonBlocking - %s", tlsErrorToString(tls_error));
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        }
    }
}

TLS_error tlsManagerPacketReceived(Manager *manager, Manager_module module, Packet *packet) {
    char *FUN_NAME = "tlsP2PManagerPacketReceived";
    assertl(manager, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "manager NULL");
    assertl(packet, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "packet NULL");

    Manager_error manager_error;

    if (module == MANAGER_MOD_PEER) {
        switch (packet->type) {
        case PACKET_P2P_MSG:
            switch (p2pMsgGetType(&(packet->p2p))) {
            case P2P_REJECT:
            case P2P_CLOSE:
                managerSend(manager, MANAGER_MOD_OUTPUT, packet);
                return TLS_CLOSE;
                break;
            /* unexpected */
            case P2P_REQUEST_IN:
            case P2P_AVAILABLE:
            case P2P_GET_INFOS:
            case P2P_TRY_CLIENT_MODE:
            case P2P_TRY_SERVER_MODE:
            case P2P_ACCEPT:
            case P2P_CONNECTION_OK:
            case P2P_CONNECTION_KO:
            case P2P_CONNECTION_SERVER:
            case P2P_REQUEST_OUT:
            case P2P_GET_AVAILABLE:
            case P2P_CON_FAILURE:
            case P2P_CON_SUCCESS:
            case P2P_INFOS:
                warnl(FILE_TLS_COM, FUN_NAME, "packet %s unsupported",
                      p2pMsgTypeToString(p2pMsgGetType(&(packet->p2p))));
            }
            return TLS_ERROR;
            break;
        case PACKET_MSG:
        case PACKET_TXT:
            manager_error = managerSend(manager, MANAGER_MOD_OUTPUT, packet);
            return TLS_SUCCESS;
            break;
        }
    } else if (module == MANAGER_MOD_SERVER) {
        switch (packet->type) {
        case PACKET_P2P_MSG:
            switch (p2pMsgGetType(&(packet->p2p))) {
            case P2P_AVAILABLE:
            case P2P_REQUEST_IN:
                manager_error = managerSend(manager, MANAGER_MOD_OUTPUT, packet);
                return TLS_SUCCESS;
                break;
            case P2P_REJECT:
            case P2P_CLOSE:
            case P2P_GET_INFOS:
            case P2P_TRY_CLIENT_MODE:
            case P2P_TRY_SERVER_MODE:
            case P2P_ACCEPT:
                managerSend(manager, MANAGER_MOD_PEER, packet);
                return TLS_SUCCESS;
                break;
            case P2P_CONNECTION_OK:
            case P2P_CONNECTION_KO:
                managerSend(manager, MANAGER_MOD_INPUT, packet);
                managerSend(manager, MANAGER_MOD_OUTPUT, packet);
                return TLS_SUCCESS;
                break;
            /* unexpected */
            case P2P_CONNECTION_SERVER:
            case P2P_REQUEST_OUT:
            case P2P_GET_AVAILABLE:
            case P2P_CON_FAILURE:
            case P2P_CON_SUCCESS:
            case P2P_INFOS:
                warnl(FILE_TLS_COM, FUN_NAME, "packet %s unsupported",
                      p2pMsgTypeToString(p2pMsgGetType(&(packet->p2p))));
            }
            return TLS_ERROR;
            break;
        case PACKET_MSG:
        case PACKET_TXT:
            manager_error = managerSend(manager, MANAGER_MOD_OUTPUT, packet);
            return TLS_SUCCESS;
            break;
        }
    } else {
        warnl(FILE_TLS_COM, FUN_NAME, "unexpected Manager module");
        return TLS_ERROR;
    }
}

TLS_error tlsManagerPacketGetNext(Manager *manager, Manager_module module, Packet **packet) {
    char *FUN_NAME = "tlsManagerPacketGetNext";
    assertl(manager, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "manager NULL");
    assertl(packet, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "packet NULL");

    Manager_error manager_error;
    manager_error = managerReceiveNonBlocking(manager, module, packet);
    switch (manager_error) {
    case MANAGER_ERR_SUCCESS:
        /* packet is close request */
        if ((*packet)->type == PACKET_P2P_MSG && (*packet)->p2p.type == P2P_CLOSE)
            return TLS_CLOSE;
        return TLS_SUCCESS;
        break;
    case MANAGER_ERR_RETRY:
        return TLS_RETRY;
        break;
    case MANAGER_ERR_CLOSED:
        warnl(FILE_TLS_COM, FUN_NAME, "%s - manager closed", managerErrorToString(manager_error));
        return TLS_ERROR;
        break;
    case MANAGER_ERR_ERROR:
        warnl(FILE_TLS_COM, FUN_NAME, "%s - manager failed", managerErrorToString(manager_error));
        return TLS_ERROR;
        break;
    }
}

TLS_error tlsSend(TLS_infos *infos, Packet *packet) {
    char FUN_NAME[32] = "tlsSend";
    assertl(infos, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "infos NULL");
    assertl(packet, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "packet NULL");

    int ret;
    int error;
    char buff[ERROR_BUFF_SIZE];

    /* case com closed */
    if (!infos->open) {
        warnl(FILE_TLS_COM, FUN_NAME, "communication closed");
        return TLS_ERROR;
    }

    do {
        /* try send packet */
        ret = SSL_write(infos->ssl, packet, sizeof(Packet));
        if (ret > 0) {
            return TLS_SUCCESS;
        }

        /* handel error */
        error = SSL_get_error(infos->ssl, ret);
        switch (error) {
        case SSL_ERROR_NONE:
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            break;
        case SSL_ERROR_ZERO_RETURN:
            infos->open = false;
            return TLS_CLOSE;
            break;
        default:
            ERR_error_string_n(error, buff, sizeof(buff));
            warnl(FILE_TLS_COM, FUN_NAME, "SSL_write (%s) : %s", error, buff);
            infos->open = false;
            return TLS_CLOSE;
        }
    } while (1);
}

TLS_error tlsReceiveNonBlocking(TLS_infos *infos, Packet **packet) {
    char FUN_NAME[32] = "tlsReaceiveNonBlocking";
    assertl(infos, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "infos NULL");
    assertl(packet, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "packet NULL");

    int ret;
    int error;
    char buff[ERROR_BUFF_SIZE];

    /* if info->open == false, user can always read if packets are waiting to be received */
    if (!infos->ssl) {
        return TLS_ERROR;
    }

    Packet p;

    /* read packet */
    ret = SSL_read(infos->ssl, &p, sizeof(Packet));

    /* return packet if success */
    if (ret > 0) {
        *packet = malloc(sizeof(Packet));
        memcpy(*packet, &p, sizeof(Packet));
        return TLS_SUCCESS;
    }

    /* handel error */
    error = SSL_get_error(infos->ssl, ret);

    switch (error) {
    case SSL_ERROR_ZERO_RETURN:
        infos->open = false;
        return TLS_CLOSE;
        break;
    case SSL_ERROR_NONE:
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        return TLS_RETRY;
        break;
    case SSL_ERROR_SYSCALL:
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return TLS_RETRY;
            break;
        }
    default:
        ERR_error_string_n(error, buff, sizeof(buff));
        warnl(FILE_TLS_COM, FUN_NAME, "SSL_read (%d) : %s", error, buff);
        return TLS_ERROR;
    }
}

TLS_error tlsReceiveBlocking(TLS_infos *infos, Packet **packet) {
    char FUN_NAME[32] = "tlsReceiveBlocking";
    assertl(infos, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "infos NULL");
    assertl(packet, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "packet NULL");

    int ret;
    int error;
    char buff[ERROR_BUFF_SIZE];
    TLS_error tls_error;
    Packet p;

    /* if info->open == false, user can always read if packets are waiting to be received */
    if (!infos->ssl) {
        return TLS_ERROR;
    }

    /* blocking socket */
    int flags = fcntl(infos->sockfd, F_GETFL, 0);
    if (flags == -1) {
        warnl(FILE_TLS_COM, FUN_NAME, "error getting socket flags");
        return TLS_ERROR;
    }

    if (fcntl(infos->sockfd, F_SETFL, flags & ~O_NONBLOCK) != 0) {
        warnl(FILE_TLS_COM, FUN_NAME, "error setting blocking socket");
        close(infos->sockfd);
        infos->sockfd = -1;
        return TLS_ERROR;
    }

    /* read packet */
    ret = SSL_read(infos->ssl, &p, sizeof(Packet));

    /* return packet if success */
    if (ret > 0) {
        *packet = malloc(sizeof(Packet));
        *packet = packetCopy(&p);
    }

    /* handel error */
    error = SSL_get_error(infos->ssl, ret);

    switch (error) {
    case SSL_ERROR_NONE:
        tls_error = TLS_SUCCESS;
        break;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        tls_error = TLS_RETRY;
        break;
    case SSL_ERROR_ZERO_RETURN:
        infos->open = false;
        tls_error = TLS_CLOSE;
        break;
    default:
        ERR_error_string_n(error, buff, sizeof(buff));
        warnl(FILE_TLS_COM, FUN_NAME, "SSL_read (%d) : %s", error, buff);
        tls_error = TLS_ERROR;
    }

    /* non blocking socket */
    flags = fcntl(infos->sockfd, F_GETFL, 0);
    if (fcntl(infos->sockfd, F_SETFL, flags | O_NONBLOCK) != 0) {
        warnl(FILE_TLS_COM, FUN_NAME, "error nonblock socket");
        close(infos->sockfd);
        infos->sockfd = -1;
        tls_error = TLS_ERROR;
    }
    return tls_error;
}

char *tlsErrorToString(TLS_error error) {
    switch (error) {
    case TLS_ERROR:
        return "TLS_ERROR";
    case TLS_SUCCESS:
        return "TLS_SUCCESS";
    case TLS_NULL_POINTER:
        return "TLS_NULL_POINTER";
    case TLS_RETRY:
        return "TLS_RETRY";
    case TLS_CLOSE:
        return "TLS_CLOSE";
    default:
        return "Unknown";
    }
}
