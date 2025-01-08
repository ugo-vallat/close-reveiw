#ifndef __GESTONARU_COM__
#define __GESTONARU_COM__

#include <pthread.h>
#include <types/genericlist.h>
#include <types/packet.h>
#include <utils/project_constants.h>

typedef enum e_manager_module {
    MANAGER_MOD_INPUT,  /* manager of the input thread (user input) */
    MANAGER_MOD_OUTPUT, /* manager of the output thread (user output) */
    MANAGER_MOD_SERVER, /* manager of the server communication thread */
    MANAGER_MOD_PEER,   /* manager of the p2p communication thread */
    MANAGER_MOD_MAIN    /* manager of the main thread */
} Manager_module;

typedef enum e_manager_state {
    MANAGER_STATE_IN_PROGRESS, /* thread is opening, not fully ready */
    MANAGER_STATE_OPEN,        /* thread is ready */
    MANAGER_STATE_CLOSED       /* thread is close */
} Manager_state;

typedef enum e_manager_error {
    MANAGER_ERR_SUCCESS, /* function succeeded */
    MANAGER_ERR_ERROR,   /* error occurred */
    MANAGER_ERR_RETRY,   /* nothing to do, retry later */
    MANAGER_ERR_CLOSED   /* manager has been closed */
} Manager_error;

typedef struct s_buffer_input {
    Manager_state state;
    pthread_mutex_t *mutex_wait_read;
    pthread_mutex_t *mutex_access_buffer;
    GenList *buff;
} Buffer_module;

typedef struct s_manager {
    Buffer_module input;
    Buffer_module output;
    Buffer_module server;
    Buffer_module peer;
    Buffer_module main;
    char user_id[SIZE_NAME];
} Manager;

/**
 * @brief Init the Manager structure with all module closed (MANAGER_STATE_CLOSED)
 *
 * @return Manager*
 */
Manager *initManager(void);

/**
 * @brief Delete the manager structure and free the memory
 *
 * @param[in] manager
 */
void deinitManager(Manager **manager);

/**
 * @brief Set the state of the module in the state requested
 *
 * @param[in] manager Manager
 * @param[in] module Module to set
 * @param[in] state New state of the module
 * @note if state == MANAGER_STATE_CLOSED, send join request to the main thread
 */
void managerSetState(Manager *manager, Manager_module module, Manager_state state);

/**
 * @brief Return the state of the module
 *
 * @param[in] manager Manager
 * @param[in] module Module
 * @return Manager_state
 */
Manager_state managerGetState(Manager *manager, Manager_module module);

/**
 * @brief Set the local user id
 *
 * @param[in] manager Manager
 * @param[in] user_id user id
 * @return char*
 */
void managerSetUser(Manager *manager, char *user_id);

/**
 * @brief Returns the local user id
 *
 * @param[in] manager Manager
 * @return char*
 */
char *managerGetUser(Manager *manager);

/**
 * @brief Try to send the packet to the module
 *
 * @param[in] manager Manager
 * @param[in] module Targeted module
 * @param[in] packet Packet to send
 * @return Manager_error
 * @note send a copy of the packet
 */
Manager_error managerSend(Manager *manager, Manager_module module, Packet *packet);

/**
 * @brief Wait next packet to read
 *
 * @param[in] manager Manager
 * @param[in] module Module to read
 * @param[out] packet Buffer to store the packet received
 * @return Manager_error
 * @note If multiple threads try to read in the module, this function can return MANAGER_ERR_RETRY
 * @remark It's recommended to have only one thread read a module at a time
 */
Manager_error managerReceiveBlocking(Manager *manager, Manager_module module, Packet **packet);

/**
 * @brief Try to read a packet (return MANAGER_ERR_RETRY if nothing to read)
 *
 * @param[in] manager Manager
 * @param[in] module Module to read
 * @param[out] packet Buffer to store the packet received
 * @return Manager_error
 * @remark It's recommended to have only one thread read a module at a time
 */
Manager_error managerReceiveNonBlocking(Manager *manager, Manager_module module, Packet **packet);

/**
 * @brief Wait next id (pthread_t) of a closed thread
 *
 * @param[in] manager Manager
 * @param[out] num_t Buffer to store the id
 * @return Manager_error
 * @remark It's recommended to have only one thread read a module at a time
 */
Manager_error managerMainReceive(Manager *manager, pthread_t *num_t);

/**
 * @brief Send the id of the thread to join to the main
 *
 * @param manager Manager
 * @param num_t Id of the thread
 * @return Manager_error
 */
Manager_error managerMainSendPthreadToJoin(Manager *manager, pthread_t num_t);

/**
 * @brief Check if a module state is set to MANAGER_STATE_OPEN or MANAGER_STATE_IN_PROGRESS
 *
 * @param manager Manager
 * @return true if a module is open, false otherwise
 */
bool isManagerModuleOpen(Manager *manager);

/**
 * @brief Return the string associated to the Manager_error
 *
 * @param error Manager_error
 * @return const char*
 */
char *managerErrorToString(Manager_error error);

/**
 * @brief Return the string associated to the module
 *
 * @param module Module of manager
 * @return char* (don't free)
 */
char *managerModuleToString(Manager_module module);

#endif
