#ifndef __GESTONARU_COM__
#define __GESTONARU_COM__

#include <pthread.h>
#include <types/genericlist.h>
#include <types/packet.h>

typedef enum e_manager_module {
    MANAGER_INPUT,
    MANAGER_OUTPUT,
    MANAGER_SERVER,
    MANAGER_PEER
} Manager_module;

typedef enum e_manager_state {
    MANAGER_IN_PROGRESS, /* thread is opening, not fully ready */
    MANAGER_OPEN,        /* thread is ready */
    MANAGER_STOPPED,     /* thread died, waiting for join */
    MANAGER_CLOSE        /* thread is close */
} Manager_state;

typedef struct s_buffer_input {
    Manager_state state;
    pthread_mutex_t mutex_read;
    pthread_mutex_t mutex_write;
    GenList *buff;
    pthread_t num_t;
} Buffer_module;

typedef struct s_manager {
    Buffer_module input;
    Buffer_module output;
    Buffer_module server;
    Buffer_module peer;
    Buffer_module main;
} Manager;

typedef enum e_manager_error { MANAGER_SUCCESS, MANAGER_ERROR, MANAGER_RETRY } Manager_error;

Manager *initManager();

void deinitManager(Manager **manager);

void managerSetState(Manager_module module, Manager_state state);

Manager_state managerGetState(Manager_module module);

Manager_error managerSend(Manager_module module, Packet *packet);

Manager_error managerReceiveBlocking(Manager_module module, Packet **packet);

Manager_error managerReceiveNonBloking(Manager_module module, Packet **packet);

Manager_error managerMainReceive(pthread_t *nbt);

#endif