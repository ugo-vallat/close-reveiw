#include <bits/types/struct_timeval.h>
#include <fcntl.h>
#include <network/packet.h>
#include <network/tls-com.h>
#include <openssl/ssl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <utils/genericlist.h>
#include <utils/logger.h>

#define ERROR_BUFF_SIZE 512

struct s_tls_infos {
    /* info */
    char ip[SIZE_IP_CHAR];
    int port;
    Mode mode;
    char *path_cert;
    char *path_key;
    bool end;

    /* structures */
    SSL_CTX *ctx;
    SSL *ssl;
    int sockfd;

    /* buffer */
    GenList *send;
    GenList *received;

    /* thread info */
    pthread_t numt;
};

typedef enum e_tls_handler_error {
    H_SUCCESS = 0,      /* success */
    H_UNKWON = -1,      /* unkwon error */
    H_RETRY_LATER = -2, /* retry operation later */
    H_PEER_CLOSE = -3   /* peer closed the connection */
} TLS_handler_error;

/**
 * @brief Send packet on TLS com (for thread com)
 *
 * @param infos Infos
 * @param p Packet to send
 * @return TLS_handler_error value
 */
TLS_handler_error sendPacketTLS(TLS_infos *infos, Packet *p) {
    if (!infos->ssl) {
        warnl("tls-com.c", "sendPacketTLS", "ssl connexion closed");
        return -1;
    }
    int ret;
    int error;
    char buff[ERROR_BUFF_SIZE];

    /* send packet */
    while (1) {
        /* try sending */
        ret = SSL_write(infos->ssl, p, sizeof(Packet));
        if (ret > 0) {
            return H_SUCCESS;
        }
        /* handel error */
        error = SSL_get_error(infos->ssl, ret);
        switch (error) {
        case SSL_ERROR_NONE:
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            break;
        case SSL_ERROR_ZERO_RETURN:
            return H_PEER_CLOSE;
            break;
        default:
            ERR_error_string_n(error, buff, sizeof(buff));
            warnl("tls-com.c", "sendPacketTLS", "SSL_write : %s", buff);
            return H_UNKWON;
        }
    }
}

/**
 * @brief Try received packet from tls and add it to the buffer
 *
 * @param infos TLS_infos
 * @param buffRecieved List of packets received
 * @return TLS_handler_error value
 */
TLS_handler_error receivePacketTLS(TLS_infos *infos, GenList *buffRecieved) {
    if (!infos->ssl) {
        warnl("tls-com.c", "receivePacketTLS", "ssl connexion closed");
        return -1;
    }
    int ret;
    int error;
    char buff[ERROR_BUFF_SIZE];
    Packet *packet = malloc(sizeof(Packet));

    /* read packet */
    ret = SSL_read(infos->ssl, packet, sizeof(Packet));

    /* return packet if success */
    if (ret > 0) {
        genListAdd(buffRecieved, packet);
        return H_SUCCESS;
    }

    /* handel error */
    error = SSL_get_error(infos->ssl, ret);
    free(packet);

    switch (error) {
    case SSL_ERROR_ZERO_RETURN:
        return H_PEER_CLOSE;
        break;
    case SSL_ERROR_NONE:
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        return H_RETRY_LATER;
        break;
    default:
        ERR_error_string_n(error, buff, sizeof(buff));
        warnl("tls-com.c", "receivePacketTLS", "SSL_read : %s", buff);
        return H_UNKWON;
    }
}

/**
 * @brief Main function of thread handeling packets send and received
 *
 * @param arg TLS_infos *
 * @return NULL
 */
void *threadHandlerCom(void *arg) {
    TLS_infos *infos = (TLS_infos *)arg;
    TLS_handler_error error;

    while (!infos->end) {
        /* send packet */
        if (!genListIsEmpty(infos->send)) {
            error = sendPacketTLS(infos, genListRemove(infos->send, 0));
            switch (error) {
            case H_UNKWON:
                warnl("tls-com.c", "receivePacketTLS", "error sending packet");
                pthread_exit(NULL);
            default:
                break;
            }
        }

        /* receive packet */
        error = receivePacketTLS(infos, infos->received);
        switch (error) {
        case H_UNKWON:
            warnl("tls-com.c", "receivePacketTLS", "error sending packet");
            pthread_exit(NULL);
        case H_PEER_CLOSE:
            infos->end = true;
            break;
        default:
            break;
        }
    }

    /* end communication */
    /* send last packets */
    while (!genListIsEmpty(infos->send)) {
        error = sendPacketTLS(infos, genListRemove(infos->send, 0));
        if (error == H_UNKWON) {
            warnl("tls-com.c", "receivePacketTLS", "error sending packet");
            pthread_exit(NULL);
        }
    }
    /* close ssl write */
    SSL_shutdown(infos->ssl);
    /* read last packets */
    do {
        error = receivePacketTLS(infos, infos->received);
        if (error == H_UNKWON) {
            warnl("tls-com.c", "receivePacketTLS", "error receiving packet");
            pthread_exit(NULL);
        }
    } while (error != H_PEER_CLOSE);
    return NULL;
}

TLS_infos *initTLSInfos(const char *ip, const int port, Mode mode, char *path_cert,
                        char *path_key) {
    if (!ip) {
        warnl("tls-com.c", "initTLSInfos", "ip null");
        return NULL;
    }
    if (mode == SERVER && !path_cert) {
        warnl("tls-com.c", "initTLSInfos", "path_cert NULL");
        return NULL;
    }
    if (mode == SERVER && !path_key) {
        warnl("tls-com.c", "initTLSInfos", "path_key NULL");
        return NULL;
    }

    TLS_infos *info = malloc(sizeof(TLS_infos));
    memset(info, 0, sizeof(TLS_infos));
    strncpy(info->ip, ip, sizeof(info->ip));
    info->port = port;
    info->mode = mode;
    info->ctx = NULL;
    info->ssl = NULL;
    info->sockfd = -1;
    info->numt = -1;
    info->end = false;
    if (mode == SERVER || mode == MAIN_SERVER) {
        size_t path_size = strlen(path_cert);
        info->path_cert = malloc(path_size + 5);
        strncpy(info->path_cert, path_cert, path_size + 1);

        path_size = strlen(path_key);
        info->path_key = malloc(path_size + 5);
        strncpy(info->path_key, path_key, path_size + 1);
    } else {
        info->path_cert = NULL;
        info->path_key = NULL;
    }
    info->send = createGenList(8);
    info->received = createGenList(8);
    return info;
}

TLS_error deinitTLSInfos(TLS_infos **infos) {
    int ret = 0;
    if (!infos || !(*infos)) {
        warnl("tls-com.c", "deleteTLSInfos", "infos null");
        return TLS_NULL_POINTER;
    }

    ret = tlsCloseCom(*infos, NULL);

    if (((*infos)->send)) {
        deinitGenList(&((*infos)->send), deinitPacketGen);
    }
    if ((*infos)->received) {
        deinitGenList(&((*infos)->received), deinitPacketGen);
    }

    /* free structure */
    free(*infos);
    *infos = NULL;

    return ret;
}

TLS_error tlsOpenCom(TLS_infos *infos, struct timeval *timeout) {
    if (!infos) {
        warnl("tls-com.c", "tlsOpenCom", "infos NULL");
        deinitTLSInfos(&infos);
        return TLS_NULL_POINTER;
    }
    if (infos->mode == MAIN_SERVER) {
        warnl("tls-cmo.c", "tlsOpenCom", "trying to open in MAIN_SERVER mode");
        deinitTLSInfos(&infos);
        return TLS_ERROR;
    }
    /* close old connexion */
    if (tlsCloseCom(infos, NULL) != TLS_SUCCESS) {
        warnl("tls-cmo.c", "tlsOpenCom", "fail tlsCloseCom");
    }
    int ret;
    struct sockaddr_in serv_addr, cli_addr;

    /* init param */
    infos->end = false;

    /* Open socket */
    infos->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (infos->sockfd == -1) {
        warnl("tls-com.c", "tlsOpenCom", "error open socket");
        deinitTLSInfos(&infos);
        return TLS_ERROR;
    }

    /* create addr */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(infos->port);
    if (infos->mode == SERVER) {
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, infos->ip, &serv_addr.sin_addr) <= 0) {
            warnl("tls-com.c", "tlsOpenCom", "error ip");
            deinitTLSInfos(&infos);
            return TLS_ERROR;
        }
    }

    /* init context*/
    if (infos->mode == SERVER) {
        infos->ctx = SSL_CTX_new(TLS_server_method());
    } else {
        infos->ctx = SSL_CTX_new(TLS_client_method());
    }
    if (infos->ctx == NULL) {
        warnl("tls-com.c", "tlsOpenCom", "error context");
        ERR_print_errors_fp(stderr);
        deinitTLSInfos(&infos);
        return TLS_ERROR;
    }

    /* add certificate and key for server */
    if (infos->mode == SERVER) {
        ret = SSL_CTX_use_certificate_file(infos->ctx, infos->path_cert, SSL_FILETYPE_PEM);
        if (ret != 1) {
            warnl("tls-com.c", "tlsOpenCom", "error add cert file (%s)", infos->path_cert);
            ERR_print_errors_fp(stderr);
            return TLS_ERROR;
        }
        ret = SSL_CTX_use_PrivateKey_file(infos->ctx, infos->path_key, SSL_FILETYPE_PEM);
        if (ret != 1) {
            warnl("tls-com.c", "tlsOpenCom", "error add key file(%s)", infos->path_key);
            ERR_print_errors_fp(stderr);
            return TLS_ERROR;
        }
    }

    /* bind socket server*/
    if (infos->mode == SERVER) {
        if (bind(infos->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
            warnl("tls-com.c", "tlsOpenCom", "error bind server");
            return TLS_ERROR;
        }
        if (listen(infos->sockfd, 5) == -1) {
            warnl("tls-com.c", "tlsOpenCom", "error listen server");
            return TLS_ERROR;
        }
        if (timeout) {
            if (setsockopt(infos->sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)timeout,
                           sizeof(struct timeval)) < 0) {
                warnl("tls-com.c", "tlsOpenCom", "error set timeout");
                return TLS_ERROR;
            }
        }
    }

    /* connexion client / server */
    if (infos->mode == SERVER) {
        socklen_t cli_len = sizeof(cli_addr);
        int client_sockfd = accept(infos->sockfd, (struct sockaddr *)&cli_addr, &cli_len);
        if (client_sockfd == -1) {
            warnl("tls-com.c", "tlsOpenCom", "error accept client");
            return TLS_ERROR;
        } else {
            close(infos->sockfd);
            infos->sockfd = client_sockfd;
        }
    } else {
        if (connect(infos->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            warnl("tls-com.c", "tlsOpenCom", "error connect to server");
            return TLS_ERROR;
        }
    }

    /* create SSL connexion */
    infos->ssl = SSL_new(infos->ctx);
    if (!infos->ssl) {
        warnl("tls-com.c", "tlsOpenCom", "error SSL_new");
        ERR_print_errors_fp(stderr);
        return TLS_ERROR;
    }
    if (SSL_set_fd(infos->ssl, infos->sockfd) != 1) {
        warnl("tls-com.c", "tlsOpenCom", "error SSL_set_fd");
        ERR_print_errors_fp(stderr);
        return TLS_ERROR;
    }

    /* establish SSL connexion */
    if (infos->mode == SERVER) {
        if (SSL_accept(infos->ssl) != 1) {
            warnl("tls-com.c", "tlsOpenCom", "error SSL_accept");
            ERR_print_errors_fp(stderr);
            return TLS_ERROR;
        }
    } else {
        if (SSL_connect(infos->ssl) != 1) {
            warnl("tls-com.c", "tlsOpenCom", "error SSL_connect");
            ERR_print_errors_fp(stderr);
            return TLS_ERROR;
        }
    }

    /* non blocking socket */
    // if (infos->mode == SERVER) {
    int flags = fcntl(infos->sockfd, F_GETFL, 0);
    if (fcntl(infos->sockfd, F_SETFL, flags | O_NONBLOCK) != 0) {
        warnl("tls-com.c", "tlsOpenCom", "error nonblock socket");
        close(infos->sockfd);
        infos->sockfd = -1;
        return TLS_ERROR;
    }
    // }

    /* creat thread handler com */
    ret = pthread_create(&(infos->numt), NULL, threadHandlerCom, (void *)infos);
    if (ret != 0) {
        warnl("tls-com.c", "tlsOpenCom", "error create pthread");
        return TLS_ERROR;
    }

    return TLS_SUCCESS;
}

TLS_infos *tlsAcceptCom(TLS_infos *infos) {
    if (!infos) {
        warnl("tls-com.c", "acceptComTLS", "infos NULL");
        return NULL;
    }

    int ret;
    struct sockaddr_in serv_addr, cli_addr;
    TLS_infos *client;
    /* init param */
    infos->end = false;

    /* create addr */
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(infos->port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (infos->sockfd == -1) {
        /* Open socket */
        infos->sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (infos->sockfd == -1) {
            warnl("tls-com.c", "acceptComTLS", "error open socket");
            return NULL;
        }
        /* bind socket server*/
        if (bind(infos->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
            warnl("tls-com.c", "acceptComTLS", "error bind server");
            return NULL;
        }
        if (listen(infos->sockfd, 99999) == -1) {
            warnl("tls-com.c", "acceptComTLS", "error listen server");
            return NULL;
        }
    }

    /* init context*/
    if (infos->ctx == NULL) {
        infos->ctx = SSL_CTX_new(TLS_server_method());
        if (infos->ctx == NULL) {
            warnl("tls-com.c", "acceptComTLS", "error context");
            ERR_print_errors_fp(stderr);
            return NULL;
        }

        /* add certificate and key for server */
        ret = SSL_CTX_use_certificate_file(infos->ctx, infos->path_cert, SSL_FILETYPE_PEM);
        if (ret != 1) {
            warnl("tls-com.c", "acceptComTLS", "error add cert file (%s)", infos->path_cert);
            ERR_print_errors_fp(stderr);
            return NULL;
        }
        ret = SSL_CTX_use_PrivateKey_file(infos->ctx, infos->path_key, SSL_FILETYPE_PEM);
        if (ret != 1) {
            warnl("tls-com.c", "acceptComTLS", "error add key file(%s)", infos->path_key);
            ERR_print_errors_fp(stderr);
            return NULL;
        }
    }

    /* connexion client / server */
    socklen_t cli_len = sizeof(cli_addr);
    int client_sockfd = accept(infos->sockfd, (struct sockaddr *)&cli_addr, &cli_len);
    if (client_sockfd == -1) {
        warnl("tls-com.c", "acceptComTLS", "error accept client");
        return NULL;
    } else {
        client = initTLSInfos(infos->ip, -1, SERVER, infos->path_cert, infos->path_key);
        client->sockfd = client_sockfd;
    }

    /* create SSL connexion */
    client->ssl = SSL_new(infos->ctx);
    if (!client->ssl) {
        warnl("tls-com.c", "acceptComTLS", "error SSL_new");
        ERR_print_errors_fp(stderr);
        return NULL;
    }
    if (SSL_set_fd(client->ssl, client->sockfd) != 1) {
        warnl("tls-com.c", "acceptComTLS", "error SSL_set_fd");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    /* establish SSL connexion */
    if (SSL_accept(client->ssl) != 1) {
        warnl("tls-com.c", "acceptComTLS", "error SSL_accept");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    /* non blocking socket */
    int flags = fcntl(client->sockfd, F_GETFL, 0);
    if (fcntl(client->sockfd, F_SETFL, flags | O_NONBLOCK) != 0) {
        warnl("tls-com.c", "acceptComTLS", "error nonblock socket");
        close(client->sockfd);
        client->sockfd = -1;
        return NULL;
    }

    /* creat thread handler com */
    ret = pthread_create(&(client->numt), NULL, threadHandlerCom, (void *)client);
    if (ret != 0) {
        warnl("tls-com.c", "acceptComTLS", "error create pthread");
        return NULL;
    }

    return client;
}

TLS_error tlsCloseCom(TLS_infos *infos, GenList **lastReceived) {
    int ret = 0;
    if (!infos) {
        warnl("tls-com.c", "tlsCloseCom", "infos null");
        return TLS_NULL_POINTER;
    }
    infos->end = true;

    if (infos->numt != -1) {
        ret = pthread_join(infos->numt, NULL);
        if (ret != 0) {
            warnl("tls-com.c", "tlsCloseCom", "error pthread join");
        }
        infos->numt = -1;
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
            warnl("tls-com.c", "tlsCloseCom", "error close socket");
        infos->sockfd = -1;
    }

    /* copy last packets */
    if (lastReceived != NULL) {
        *lastReceived = genListCopy(infos->received);
    }

    genListClear(infos->received, deinitPacketGen);
    genListClear(infos->send, deinitPacketGen);

    return TLS_SUCCESS;
}

TLS_error tslSendPacket(TLS_infos *infos, Packet *p) {
    if (!infos) {
        warnl("tls-com.c", "sendPacket", "infos null");
        return TLS_NULL_POINTER;
    }
    if (!p) {
        warnl("tls-com.c", "sendPacket", "packet null");
        return TLS_NULL_POINTER;
    }
    if (!infos->ssl) {
        return TLS_DISCONNECTED;
    }
    if (infos->end) {
        return TLS_CLOSE;
    }
    genListAdd(infos->send, packetCopy(p));
    return 0;
}

bool tlsIsPacketReceived(TLS_infos *infos) {
    if (infos == NULL) {
        warnl("tls-com.c", "isPacketReceived", "infos null");
        return false;
    }
    return !genListIsEmpty(infos->received);
}

TLS_error tlsReceivePacket(TLS_infos *infos, Packet **p) {
    if (!infos) {
        warnl("tls-com.c", "receivePacket", "infos null");
        return TLS_NULL_POINTER;
    }
    if (!p) {
        warnl("tls-com.c", "receivePacket", "packet** null");
        return TLS_NULL_POINTER;
    }
    if (!infos->ssl) {
        return TLS_DISCONNECTED;
    }
    if (infos->end) {
        return TLS_CLOSE;
    }
    if (genListIsEmpty(infos->received)) {
        (*p) = NULL;
        return TLS_RETRY;
    }
    (*p) = genListRemove(infos->received, 0);
    return TLS_SUCCESS;
}
