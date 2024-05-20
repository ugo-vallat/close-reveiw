#include "types/genericlist.h"
#include "types/message.h"
#include "types/p2p-msg.h"
#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <network/tls-com.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <types/packet.h>
#include <unistd.h>
#include <utils/const-define.h>
#include <utils/logger.h>

#define ERROR_BUFF_SIZE 512

#define FILE_TLS_COM "tls-com.c"

TLS_infos *initTLSInfos(const char *ip, const int port, TLS_mode tls_mode, char *path_cert,
                        char *path_key) {
    char FUN_NAME[32] = "initTLSInfos";
    assertl(ip, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "ip NULL");
    assertl(port > 0 & port < 65535, FILE_TLS_COM, FUN_NAME, TLS_ERROR, "invalid num port <%d>",
            port);
    assertl(tls_mode == TLS_CLIENT || tls_mode == TLS_SERVER || tls_mode == TLS_MAIN_SERVER,
            FILE_TLS_COM, FUN_NAME, TLS_ERROR, "invalid tls mode <%d>", tls_mode);
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
            if (setsockopt(infos->sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)timeout,
                           sizeof(struct timeval)) < 0) {
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
            warnl(FILE_TLS_COM, FUN_NAME, "error accept client");
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        } else {
            close(infos->sockfd);
            infos->sockfd = client_sockfd;
        }
    } else {
        if (connect(infos->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            warnl(FILE_TLS_COM, FUN_NAME, "error connect to server");
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
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
        client =
            initTLSInfos(infos->ip, infos->port, TLS_SERVER, infos->path_cert, infos->path_key);
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

TLS_error tlsStartListenning(TLS_infos *infos, Manager *manager, Manager_module module,
                             funTLSGetNextPacket next_packet,
                             funTLSPacketReceivedManager MSG_manager,
                             funTLSPacketReceivedManager P2P_manager) {
    char FUN_NAME[32] = "tlsStartListenning";
    assertl(infos, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "infos NULL");
    assertl(manager, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "manager NULL");
    assertl(next_packet, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "next_packet NULL");
    assertl(MSG_manager, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "MSG_manager NULL");
    assertl(P2P_manager, FILE_TLS_COM, FUN_NAME, TLS_NULL_POINTER, "P2P_manager NULL");

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
                MSG_manager(manager, module, p);
                break;
            case PACKET_P2P_MSG:
                P2P_manager(manager, module, p);
                break;
            default:
                warnl(FILE_TLS_COM, FUN_NAME, "unexpected type <%d>", p->type);
                tlsCloseCom(infos, NULL);
                return TLS_ERROR;
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
            warnl(FILE_TLS_COM, FUN_NAME, "fail tlsReceiveNonBlocking");
            tlsCloseCom(infos, NULL);
            return TLS_ERROR;
        }
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
            warnl(FILE_TLS_COM, FUN_NAME, "SSL_write : %s", buff);
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
    default:
        ERR_error_string_n(error, buff, sizeof(buff));
        warnl(FILE_TLS_COM, FUN_NAME, "SSL_read : %s", buff);
        return TLS_ERROR;
    }
}
