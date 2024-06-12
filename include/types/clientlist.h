#ifndef __CLIENTLIST_H__
#define __CLIENTLIST_H__

#include "utils/project_constants.h"
#include <network/tls-com.h>
#include <types/genericlist.h>

typedef enum e_etat_client {
    AVAILABLE, // client connect to the server and not in a p2p communication
    IN_CONNECTION, //client in a p2p connection
    TRY_CONNECTION //client trying to establish a p2p connection
    } Etat_client;

typedef struct s_client {
    int id;
    Etat_client etat;
    TLS_infos *info_user;
    GenList *request_by;
    char username[SIZE_NAME];
} Client;

typedef struct s_client_list ClientList;

/**
 * @brief Creates an empty list
 *
 * @param[in] memory_size Initial memory space (in number of elements)
 *
 * @return pointer to the list
 * @note Allocates memory but is not initialized (list size = 0)
 */
ClientList *initClientList(unsigned memory_size);

/**
 * @brief Deletes the list but does not delete the pointed data
 * @pre l != NULL
 * @pre *l != NULL
 *
 * @param[in] l list to delete
 */
void deinitClientList(ClientList **l);


/**
 * @brief Adds the element to the list which is sorted
 *
 * @param[in] l Pointer to the list
 * @param[in] v Value to add
 * @pre l != NULL
 */
void clientListAdd(ClientList *l, Client *c);


/**
 * @brief Removes the element at position i
 *
 * @param[in] l Pointer to the list
 * @param[in] i position
 * @pre l != NULL
 *
 * @pre i < listSize
 * @return Value before deletion
 */
Client *clientListRemove(ClientList *l, unsigned i);


/**
 * @brief free the element at position i
 *
 * @param[in] l Pointer to the list
 * @param[in] i position
 * @pre l != NULL
 *
 * @pre i < listSize
 * @return Value before deletion
 */
void clientListDelete(ClientList *l, unsigned i);

/**
 * @brief Reads the value at position i
 *
 * @param[in] l Pointer to the list
 * @param[in] i Position of the element
 *
 * @pre l != NULL
 * @pre i < list size
 *
 * @return Read value
 **/
Client *clientListGet(ClientList *l, unsigned i);

Client *clientListGetId(ClientList *l, int id);


/**
 *
 * @brief Returns if the list is empty
 *
 * @param[in] l Pointer to the list
 * @pre l != NULL
 *
 * @return true if empty, false otherwise
 */
bool clientListIsEmpty(ClientList *l);

Client *clientListPop(ClientList *l);


/**
 * @brief Returns the size of the list (position + 1 of the last element)
 *
 * @param[in] l Pointer to the list
 * @pre l != NULL
 *
 * @return size of the list
 *
 */

unsigned clientListSize(ClientList *l);

#endif