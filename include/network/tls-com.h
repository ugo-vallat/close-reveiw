#ifndef __TLS_COM_H__
#define __TLS_COM_H__

#include <arpa/inet.h>
#include <network/packet.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdbool.h>

#define PACKET_BUFF_SIZE 512

#define CHAR_IP_SIZE 32

typedef struct s_tls_infos TLSInfos;

typedef enum e_mode { SERVER = 1, CLIENT = 2 } Mode;

/**
 * @brief Initialise la structure TLSinfo
 * @param ip Serveur IP
 * @param port Serveur port
 * @param path_cert Chemin d'accès au certificat (NULL si mode CLIENT)
 * @param path_key Chemin d'accès à la clé privée (NULL si mode CLIENT)
 * @return TLSInfos*
 * @note Structure malloc, utiliser deleteTLSInfos() pour supprimer
 */
TLSInfos *initTLSInfos(const char *ip, const int port, Mode mode, char *path_cert, char *path_key);

/**
 * @brief Supprimer la structure TLSInfos
 *
 * @param infos structure à supprimer
 */
void deleteTLSInfos(TLSInfos **infos);

/**
 * @brief Etablie un canal de communication sécurisé avec l'hôte distant
 *
 * @param infos Information sur l'appareil distant
 * @return 0 en cas de succès, -1 sinon
 */
int openComTLS(TLSInfos *infos);

/**
 * @brief Ferme le canal de communication
 *
 * @param infos Canal à fermer
 * @return 0 en cas de succès, -1 sinon
 */
int closeComTLS(TLSInfos *infos);

/**
 * @brief Envoie le paquet à l'hôte distant
 *
 * @param infos Canal de communication
 * @param p Paquet à envoyer
 * @return 0 en cas de succès, -1 sinon
 */
int sendPacket(TLSInfos *infos, Packet *p);

/**
 * @brief Indique si des paquets ont été reçus
 *
 * @param infos Canal de communication
 * @return true si des paquets sont en attentes, false sinon
 */
bool isBufferPacketEmpty(TLSInfos *infos);

/**
 * @brief Stock dans p le paquet reçu le plus ancien
 *
 * @param infos Canal de communication
 * @param p Buffer pour récupérer le paquet
 * @return 0 en cas de succès, -1 sinon
 */
int receivePacket(TLSInfos *infos, Packet **p);

#endif
