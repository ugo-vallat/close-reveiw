#ifndef __GESTONARU_COM__
#define __GESTONARU_COM__

#include <network/p2p-msg.h>
#include <network/packet.h>
#include <network/tls-com.h>
#include <utils/genericlist.h>
#include <utils/message.h>
#include <utils/tmp-command.h>
#include <utils/token.h>

typedef struct s_buffer_input {
    bool is_set;
    pthread_mutex_t mutex;
    GenList *p2p;
} Buffer_input;

typedef struct s_buffer_output {
    bool is_set;
    pthread_mutex_t mutex;
    GenList *msg;
    GenList *cmd_result;
} Buffer_output;

typedef struct s_buffer_tls {
    bool is_set;
    GenList *send;
} Buffer_tls;

typedef struct s_gestionary {
    Buffer_input input;
    Buffer_output output;
    Buffer_tls server;
    Buffer_tls peer;
} Gestionary;

typedef enum e_module_gest {
    MOD_GEST_INPUT,
    MOD_GEST_OUTPUT,
    MOD_GEST_PEER,
    MOD_GEST_SERVER
} Module_gest;

typedef enum e_gest_error {
    GEST_SUCCESS,
    GEST_FAILURE,
    GEST_NULL_ARG,
    GEST_NOT_SET,
    GEST_RETRY,
    GEST_UNDEFINED
} Gest_error;

/**
 * @brief Inits struct Gestionary with all buffers not set
 *
 * @return Gestionary*
 * @note Malloced structure
 */
Gestionary *initGestionary();

/**
 * @brief Deletes struct Gestionary and frees the memory
 *
 * @param[in] gest Struct to delete
 */
void deinitGestionary(Gestionary *gest);

/**
 * @brief Sets the server's buffers of the specified module
 *
 * @param[in] gest Gestionary
 * @param[in] module Module to set
 * @return Gest_error
 */
Gest_error gestionarySetModule(Gestionary *gest, Module_gest module);

/**
 * @brief Delete the buffers of the specified module
 *
 * @param[in] gest Gestionary
 * @param[in] module Module to set
 * @return Gest_error
 */
Gest_error gestionaryUnsetModule(Gestionary *gest, Module_gest module);

/**
 * @brief Copies and adds packet to the gestionary
 *
 * @param gest Gestionary
 * @param p Packet to add
 * @return Gest_error
 */
Gest_error gestionaryServerAddPacket(Gestionary *gest, Packet *p);

/**
 * @brief Removes and returns the oldest Packet from the buffer
 *
 * @param[in] gest Gestionary
 * @param[out] p Store packet in p (NULL if nothing to get)
 * @return Gest_error
 * @note non-blocking operation, return GEST_RETRY if nothing to read
 */
Gest_error gestionaryServerPopPacket(Gestionary *gest, Packet **p);

/**
 * @brief Copies and adds packet to the gestionary
 *
 * @param gest Gestionary
 * @param p Packet to add
 * @return Gest_error
 */
Gest_error gestionaryPeerAddPacket(Gestionary *gest, Packet *p);

/**
 * @brief Removes and returns the oldest Packet from the buffer
 *
 * @param[in] gest Gestionary
 * @param[out] p Store packet in p (NULL if nothing to get)
 * @return Gest_error
 * @note non-blocking operation, return GEST_RETRY if nothing to read
 */
Gest_error gestionaryPeerPopPacket(Gestionary *gest, Packet **p);

/**
 * @brief Copies and adds struct Msg to gestionary
 *
 * @param gest Gestionary
 * @param msg Message to add
 * @return Gest_error
 */
Gest_error gestionaryInputAddMsg(Gestionary *gest, Msg *msg);

/**
 * @brief Copies and adds struct Command to gestionary
 *
 * @param gest Gestionary
 * @param cmd Command to add
 * @return Gest_error
 */
Gest_error gestionaryInputAddCmd(Gestionary *gest, Command *cmd);

/**
 * @brief Copies and adds struct P2P_msg to gestionary
 *
 * @param gest Gestionary
 * @param msg P2P_msg to add
 * @return Gest_error
 */
Gest_error gestionaryInputAddP2PMsg(Gestionary *gest, P2P_msg *msg);

/**
 * @brief Removes and returns the oldest P2P_msg from the buffer
 *
 * @param[in] gest Gestionary
 * @param[out] msg Store P2P_msg in msg (NULL if nothing to get)
 * @return Gest_error
 * @note blocking operation
 */
Gest_error gestionaryInputPopP2P(Gestionary *gest, P2P_msg **msg);

/**
 * @brief Removes and returns the oldest Msg from the buffer
 *
 * @param[in] gest Gestionary
 * @param[out] msg Store Msg in msg (NULL if nothing to get)
 * @return Gest_error
 * @note blocking operation
 */
Gest_error gestionaryOutputPopMsg(Gestionary *gest, Msg **msg);

/**
 * @brief Removes and returns the oldest Command from the buffer
 *
 * @param[in] gest Gestionary
 * @param[out] msg Store Command in cmd (NULL if nothing to get)
 * @return Gest_error
 * @note blocking operation
 */
Gest_error gestionaryOutputPopCmd(Gestionary *gest, Command **cmd);

/**
 * @brief Gets the state of the gestionary's module
 *
 * @param gest Gestionary
 * @param module Module
 * @return true if set, false otherwise
 */
bool gestionaryIsSet(Gestionary *gest, Module_gest module);

#endif