
#include "utils/genericlist.h"
#include <network/packet.h>
#include <network/tls-com.h>
#include <openssl/ssl.h>
#include <string.h>
#include <unistd.h>
#include <utils/logger.h>

#define ERROR_BUFF_SIZE 512

struct s_tls_infos {
    /* info */
    char ip[CHAR_IP_SIZE];
    int port;
    Mode mode;
    char *path_cert;
    char *path_key;

    /* structures */
    SSL_CTX *ctx;
    SSL *ssl;
    int sockfd;

    /* buffer received */
    GenList *buff;
};

typedef void (*fun)(void *);

TLSInfos *initTLSInfos(const char *ip, const int port, Mode mode, char *path_cert, char *path_key) {
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

    TLSInfos *info = malloc(sizeof(TLSInfos));
    memset(info, 0, sizeof(TLSInfos));
    strncpy(info->ip, ip, sizeof(info->ip));
    info->port = port;
    info->mode = mode;
    info->ctx = NULL;
    info->ssl = NULL;
    info->sockfd = -1;
    if (mode == SERVER) {
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
    info->buff = createGenList(8);
    return info;
}

void deleteTLSInfos(TLSInfos **infos) {
    int ret = 0;
    if (!infos) {
        warnl("tls-com.c", "deleteTLSInfos", "infos null");
        return;
    }

    closeComTLS(*infos);

    deleteGenList(&((*infos)->buff), packetDelete);

    /* free structure */
    free(*infos);
    *infos = NULL;
}

int openComTLS(TLSInfos *infos) {
    if (!infos) {
        warnl("tls-com.c", "openComTLS", "infos NULL");
        return -1;
    }
    int ret;
    struct sockaddr_in serv_addr, cli_addr;

    /* Open socket */
    if (infos->sockfd >= 0) {
        warnl("tls-com.c", "openComTLS", "socket already open");
    } else {
        infos->sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (infos->sockfd == -1) {
            warnl("tls-com.c", "openComTLS", "error open socket");
            return -1;
        }
    }

    /* create addr */
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(infos->port);
    if (infos->mode == SERVER) {
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, infos->ip, &serv_addr.sin_addr) <= 0) {
            warnl("tls-com.c", "openComTLS", "error ip");
            return -1;
        }
    }

    /* init context*/
    if (infos->ctx == NULL) {
        if (infos->mode == SERVER) {
            infos->ctx = SSL_CTX_new(TLS_server_method());
        } else {
            infos->ctx = SSL_CTX_new(TLS_client_method());
        }
        if (infos->ctx == NULL) {
            warnl("tls-com.c", "openComTLS", "error context");
            ERR_print_errors_fp(stderr);
            return -1;
        }

        /* add certificate and key for server */
        if (infos->mode == SERVER) {
            ret = SSL_CTX_use_certificate_file(infos->ctx, infos->path_cert, SSL_FILETYPE_PEM);
            if (ret != 1) {
                warnl("tls-com.c", "openComTLS", "error add cert file (%s)", infos->path_cert);
                ERR_print_errors_fp(stderr);
                return -1;
            }
            ret = SSL_CTX_use_PrivateKey_file(infos->ctx, infos->path_key, SSL_FILETYPE_PEM);
            if (ret != 1) {
                warnl("tls-com.c", "openComTLS", "error add key file(%s)", infos->path_key);
                ERR_print_errors_fp(stderr);
                return -1;
            }
        }
    }

    /* bind socket server*/
    if (infos->mode == SERVER) {
        if (bind(infos->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
            warnl("tls-com.c", "openComTLS", "error bind server");
            return -1;
        }
        if (listen(infos->sockfd, 5) == -1) {
            warnl("tls-com.c", "openComTLS", "error listen server");
            return -1;
        }
    }

    /* connexion client / server */
    if (infos->mode == SERVER) {
        socklen_t cli_len = sizeof(cli_addr);
        int client_sockfd = accept(infos->sockfd, (struct sockaddr *)&cli_addr, &cli_len);
        if (client_sockfd == -1) {
            warnl("tls-com.c", "openComTLS", "error accept client");
            return -1;
        } else {
            close(infos->sockfd);
            infos->sockfd = client_sockfd;
        }
    } else {
        if (connect(infos->sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            warnl("tls-com.c", "openComTLS", "error connect to server");
            return -1;
        }
    }

    /* create SSL connexion */
    infos->ssl = SSL_new(infos->ctx);
    if (!infos->ssl) {
        warnl("tls-com.c", "openComTLS", "error SSL_new");
        ERR_print_errors_fp(stderr);
        return -1;
    }
    if (SSL_set_fd(infos->ssl, infos->sockfd) != 1) {
        warnl("tls-com.c", "openComTLS", "error SSL_set_fd");
        ERR_print_errors_fp(stderr);
        return -1;
    }

    /* establish SSL connexion */
    if (infos->mode == SERVER) {
        if (SSL_accept(infos->ssl) != 1) {
            warnl("tls-com.c", "openComTLS", "error SSL_accept");
            ERR_print_errors_fp(stderr);
            return -1;
        }
    } else {
        if (SSL_connect(infos->ssl) != 1) {
            warnl("tls-com.c", "openComTLS", "error SSL_connect");
            ERR_print_errors_fp(stderr);
            return -1;
        }
    }
    return 0;
}

int closeComTLS(TLSInfos *infos) {
    int ret = 0;
    if (!infos) {
        warnl("tls-com.c", "closeComTLS", "infos null");
        return -1;
    }
    /* close ssl */
    if (infos->ssl) {
        ret = SSL_shutdown(infos->ssl);
        if (ret == 0) {
            /* === bidirectional shutdown handshake not implemented yet === */
            Packet p;
            while (SSL_read(infos->ssl, &p, sizeof(Packet)) > 0) {
                printf("Packet not read : <%s>\n", p.msg.buffer);
            }
            // warnl("tls-com.c", "closeComTLS", "SSL_shutdown return 0");
            // Packet p;
            // SSL_read(infos->ssl, &p, sizeof(p));
            // ret = SSL_shutdown(infos->ssl);
            // if (ret != 1)
            //     warnl("tls-com.c", "closeComTLS", "SSL_shutdown retry return %d", ret);
        } else if (ret < 0) {
            int retError = SSL_get_error(infos->ssl, ret);
            warnl("tls-com.c", "closeComTLS", "SSL_shutdown return %d - %d", ret, retError);
            ERR_print_errors_fp(stderr);
        }
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
            warnl("tls-com.c", "closeComTLS", "error close socket");
        infos->sockfd = -1;
    }
    return 0;
}

int sendPacket(TLSInfos *infos, Packet *p) {
    if (!infos) {
        warnl("tls-com.c", "sendPacket", "infos null");
        return -1;
    }
    if (!p) {
        warnl("tls-com.c", "sendPacket", "packet null");
        return -1;
    }
    if (!infos->ssl) {
        warnl("tls-com.c", "sendPacket", "ssl connexion closed");
        return -1;
    }
    int ret;
    unsigned long code;
    char buff[ERROR_BUFF_SIZE];
    /* send packet */
    ret = SSL_write(infos->ssl, p, sizeof(Packet));
    if (ret < 0) {
        code = SSL_get_error(infos->ssl, ret);
        ERR_error_string_n(code, buff, sizeof(buff));
        warnl("tls-com.c", "sendPacket", "SSL_write : %s", buff);
    }
    /* send packet again */
    if (ret < 0) {
        if (ret != SSL_ERROR_SSL && ret != SSL_ERROR_SYSCALL) {
            printl("retry to send\n");
            ret = SSL_write(infos->ssl, p, sizeof(Packet));
            if (ret < 0) {
                code = SSL_get_error(infos->ssl, ret);
                ERR_error_string_n(code, buff, sizeof(buff));
                warnl("tls-com.c", "sendPacket", "SSL_write : %s", buff);
                return -1;
            }
        } else {
            return -1;
        }
    }
    return 0;
}

bool isBufferPacketEmpty(TLSInfos *infos);

int receivePacket(TLSInfos *infos, Packet **p) {
    if (!infos) {
        warnl("tls-com", "receivePacket", "infos null");
        return -1;
    }
    if (!p) {
        warnl("tls-com", "receivePacket", "&packet null");
        return -1;
    }
    if (!infos->ssl) {
        warnl("tls-com.c", "receivePacket", "ssl connexion closed");
        return -1;
    }
    int ret;
    unsigned long code;
    char buff[ERROR_BUFF_SIZE];
    Packet *packet = malloc(sizeof(Packet));
    ret = SSL_read(infos->ssl, packet, sizeof(Packet));
    if (ret <= 0) {
        code = SSL_get_error(infos->ssl, ret);
        ERR_error_string_n(code, buff, sizeof(buff));
        warnl("tls-com.c", "receivePacket", "SSL_read : %s", buff);
        return -1;
    }
    *p = packet;
    return 0;
}