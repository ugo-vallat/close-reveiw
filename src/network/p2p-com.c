#include <network/p2p-com.h>
#include <network/tls-com.h>
#include <utils/token.h>

struct s_P2P_info {
    TLSInfos *tls;
    Token *token;
};
