

#include <network/packet.h>
#include <network/tls-com.h>

struct s_tls_infos {
    SSL_CTX *ctx;
    SSL *ssl;
    int sockfd;
    enum { SERVER, CLIENT } mode;
    /* buffer received */
    unsigned int count;
    Packet *buff[PACKET_BUFF_SIZE];
};