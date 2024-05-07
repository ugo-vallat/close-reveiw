#ifndef __P2P_COM_H__
#define __P2P_COM_H__

#include <network/tls-com.h>

typedef struct s_P2P_info P2PInfos;

/**
 * @brief Malloc and fill P2PInfos
 *
 * @return P2PInfos*
 * @note Use deleteP2PInfos() to delete this structure
 */
P2PInfos *initP2PInfos();

/**
 * @brief Delete structure P2PInfos and free memory
 *
 * @param infos Structure to delete
 */
void deleteP2PInfos(P2PInfos **infos);

/**
 * @brief Add a port forwarding in the NAT table of the local network
 *
 * @param[in] port_in Public port (routeur port)
 * @param[in] port_out Local port (host port)
 * @return if port_in == -1, return the public port forwarded else 0, -1 if error
 * @note if port_in == -1, try to find a free port to use and return it
 */
int forwardPortWithUpnp(int port_in, int port_out, long time);

/**
 * @brief Send a connection P2P request to the server
 *
 * @param[in] infos TEMPORARY channel with server
 * @param[in] id_user ID of the user to connect with
 */
void requestP2PConnection(TLSInfos *infos, int id_user);

/**
 * @brief Accept / Refuse connection request
 *
 * @param[in] infos TEMPORARY channel with server
 * @param[in] id_user ID of the user to accept/refuse
 * @param[in] accept True to accept, false otherwise
 */
void acceptP2PConnection(TLSInfos *infos, int id_user, bool accept);

/**
 * @brief Send list of methods available on this host to establish a P2P connection
 *
 * @param[in] infos TEMPORARY channel with server
 */
void sendMethodsEnable(TLSInfos *infos);

/**
 * @brief Create a Certificate Auto Certified used by the server mode
 *
 * @return path of the cert file
 */
char *createCertificateAutoCert();

/**
 * @brief Setup host as server and wait the remote host to connect
 *
 * @param[in] port Port to bind
 * @return TLSInfos * on success, NULL otherwise
 */
TLSInfos *connectToPeerServerMode(int port);

/**
 * @brief Setup host as client and try to connect with the remote host
 *
 * @param[in] ip IP of the remote host
 * @param[in] port Port of remote host
 * @return TLSInfos * on success, NULL otherwise
 */
TLSInfos *connectToPeerClientMode(char *ip, int port);

#endif