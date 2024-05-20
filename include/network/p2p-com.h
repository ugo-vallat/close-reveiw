#ifndef __P2P_COM_H__
#define __P2P_COM_H__

#include <network/manager.h>
#include <network/tls-com.h>
#include <types/genericlist.h>

/**
 * @brief Display the list of the users available in the output
 *
 * @param manager Manager
 */
void p2pGetlistUsersAvailable(Manager *manager);

/**
 * @brief Send a request connection, start the peer thread and wait for response
 *
 * @param manager Manager
 * @param id_user Id of the user requested
 */
void p2pSendRequestConnection(Manager *manager, char *id_peer);

/**
 * @brief Send response to the connection request and start peer thread if respond == true
 *
 * @param manager Manager
 * @param id_user User requesting
 * @param respond Response to send
 */
void p2pRespondToRequest(Manager *manager, char *id_user, bool respond);

/**
 * @brief Start direct TLS_connection without passing through the server
 *
 * @param manager Manager
 * @param mode Mode (TLS_CLIENT / TLS_SERVER)
 * @param ip Ip of the peer (ignore if mode = TLS_SERVER)
 * @param port Port of the peer / port to listen
 */
void p2pStartDirectConnection(Manager *manager, TLS_mode mode, char *ip, int port);

/**
 * @brief Close the peer connection with the user peer_id
 *
 * @param manager Manager
 * @param peer_id User connected in P2P
 */
void p2pCloseCom(Manager *manager, char *peer_id);

#endif