#ifndef __P2P_COM_H__
#define __P2P_COM_H__

#include <network/gestionary-com.h>
#include <network/p2p-msg.h>
#include <network/tls-com.h>
#include <utils/genericlist.h>

typedef struct s_P2P_info P2P_infos;

/**
 * @brief Malloc and fill P2P_infos
 *
 * @return P2P_infos*
 * @note Use deleteP2P_infos() to delete this structure
 */
P2P_infos *createP2P_infos(Gestionary_com *gestionary);

/**
 * @brief Delete structure P2P_infos and free memory
 *
 * @param infos Structure to delete
 */
void deleteP2P_infos(P2P_infos **infos);

/**
 * @brief Return the id list of known user connected
 *
 * @param infos P2P_infos
 * @return GenList* of user id (char[SIZE_NAME])
 */
GenList *p2pGetListUserConnected(P2P_infos *infos);

/**
 * @brief Return the id of user that requested to connect
 *
 * @param infos P2P_infos
 * @return char[SIZE_NAME] if there is a request, NULL otherwise
 */
char *p2pGetRequestConnection(P2P_infos *infos);

/**
 * @brief Send a connection P2P request to the server
 *
 * @param[in] infos TEMPORARY channel with server
 * @param[in] id_user ID of the user to connect with
 * @param[out] gest Return the gestionary if connection successed
 * @return 1 if success connection, 0 if echec connection, -1 otherwise
 */
int p2pSendRequestConnection(P2P_infos *infos, char *id_user, Gestionary_com **gest);

/**
 * @brief Accept / Refuse connection request
 *
 * @param[in] infos TEMPORARY channel with server
 * @param[in] id_user ID of the user to accept/refuse
 * @param[in] accept True to accept, false otherwise
 * @param[out] gest Return the gestionary if connection successed
 * @return 1 if success connection, 0 if echec connection, -1 otherwise
 */
int p2pAcceptConnection(P2P_infos *infos, char *id_user, bool accept, Gestionary_com **gest);

/**
 * @brief Add a port forwarding in the NAT table of the local network
 *
 * @param[in] port_in Public port (routeur port)
 * @param[in] port_out Local port (host port)
 * @return if port_in == -1, return the public port forwarded else 0, -1 if error
 * @note if port_in == -1, try to find a free port to use and return it
 */
int p2pForwardPortWithUpnp(int port_in, int port_out, long time);

#endif