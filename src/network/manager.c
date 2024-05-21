#include <network/manager.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <types/genericlist.h>
#include <types/packet.h>
#include <utils/logger.h>
#include <utils/project_constants.h>

#define FILE_MANAGER "manager.c"

void initManagerBuffer(Buffer_module *buffer) {
    memset(buffer, 0, sizeof(Buffer_module));
    buffer->state = MANAGER_STATE_CLOSED;
    buffer->mutex_wait_read = malloc(sizeof(pthread_mutex_t));
    buffer->mutex_access_buffer = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(buffer->mutex_wait_read, NULL);
    pthread_mutex_init(buffer->mutex_access_buffer, NULL);
    pthread_mutex_unlock(buffer->mutex_wait_read);
    pthread_mutex_unlock(buffer->mutex_access_buffer);
    buffer->buff = initGenList(16);
}

Buffer_module *getModuleBuffer(Manager *manager, Manager_module module) {
    switch (module) {
    case MANAGER_MOD_INPUT:
        return &(manager->input);
    case MANAGER_MOD_OUTPUT:
        return &(manager->output);
    case MANAGER_MOD_SERVER:
        return &(manager->server);
    case MANAGER_MOD_PEER:
        return &(manager->peer);
    case MANAGER_MOD_MAIN:
        return &(manager->main);
    }
}

Manager *initManager() {
    Manager *manager = malloc(sizeof(Manager));
    initManagerBuffer(&(manager->input));
    initManagerBuffer(&(manager->output));
    initManagerBuffer(&(manager->server));
    initManagerBuffer(&(manager->peer));
    initManagerBuffer(&(manager->main));
    return manager;
}

void deinitManagerBuffer(Buffer_module *buffer) {
    pthread_mutex_destroy(buffer->mutex_wait_read);
    pthread_mutex_destroy(buffer->mutex_access_buffer);
    free(buffer->mutex_wait_read);
    free(buffer->mutex_access_buffer);
}

void deinitManager(Manager **manager) {
    char FUN_NAME[32] = "deinitManager";
    assertl(manager, FILE_MANAGER, FUN_NAME, -1, "manager NULL");
    assertl(*manager, FILE_MANAGER, FUN_NAME, -1, "*manager NULL");
    deinitManagerBuffer(&((*manager)->input));
    deinitManagerBuffer(&((*manager)->output));
    deinitManagerBuffer(&((*manager)->server));
    deinitManagerBuffer(&((*manager)->peer));
    deinitManagerBuffer(&((*manager)->main));
    free(*manager);
    *manager = NULL;
}

void setStateOpen(Buffer_module *buffer);
void setStateClose(Manager *manager, Buffer_module *buffer);
void setStateInProgress(Buffer_module *buffer);

void setStateClose(Manager *manager, Buffer_module *buffer) {
    switch (buffer->state) {
    case MANAGER_STATE_OPEN:
    case MANAGER_STATE_IN_PROGRESS:
        genListClear(buffer->buff, deinitPacketGen);
        buffer->state = MANAGER_STATE_CLOSED;
        pthread_mutex_unlock(buffer->mutex_wait_read);
        break;
    case MANAGER_STATE_CLOSED:
        break;
    }
}

void setStateOpen(Buffer_module *buffer) {
    switch (buffer->state) {
    case MANAGER_STATE_OPEN:
        break;
    case MANAGER_STATE_IN_PROGRESS:
    case MANAGER_STATE_CLOSED:
        buffer->state = MANAGER_STATE_OPEN;
        (void)pthread_mutex_trylock(buffer->mutex_wait_read);
        break;
    }
}

void setStateInProgress(Buffer_module *buffer) {
    switch (buffer->state) {
    case MANAGER_STATE_OPEN:
        buffer->state = MANAGER_STATE_IN_PROGRESS;
        break;
    case MANAGER_STATE_IN_PROGRESS:
        break;
    case MANAGER_STATE_CLOSED:
        buffer->state = MANAGER_STATE_IN_PROGRESS;
        (void)pthread_mutex_trylock(buffer->mutex_wait_read);
        break;
    }
}

void managerSetState(Manager *manager, Manager_module module, Manager_state state) {
    char FUN_NAME[32] = "managerSetState";
    assertl(manager, FILE_MANAGER, FUN_NAME, -1, "manager NULL");

    Buffer_module *buffer = getModuleBuffer(manager, module);
    pthread_mutex_lock(buffer->mutex_access_buffer);

    switch (state) {
    case MANAGER_STATE_OPEN:
        setStateOpen(buffer);
        break;
    case MANAGER_STATE_IN_PROGRESS:
        setStateInProgress(buffer);
        break;
    case MANAGER_STATE_CLOSED:
        setStateClose(manager, buffer);
    }

    pthread_mutex_unlock(buffer->mutex_access_buffer);
}

void managerSetUser(Manager *manager, char *user_id) {
    char FUN_NAME[32] = "managerSetUser";
    assertl(manager, FILE_MANAGER, FUN_NAME, -1, "manager NULL");
    assertl(user_id, FILE_MANAGER, FUN_NAME, -1, "user_id NULL");

    strncpy(manager->user_id, user_id, SIZE_NAME);
}

char *managerGetUser(Manager *manager) {
    char FUN_NAME[32] = "managerGetUser";
    assertl(manager, FILE_MANAGER, FUN_NAME, -1, "manager NULL");

    char *name = malloc(SIZE_NAME);
    if (manager->user_id[0] == 0) {
        strncpy(name, "Unknown", SIZE_NAME);
    } else {
        strncpy(name, manager->user_id, SIZE_NAME);
    }
    return name;
}

Manager_state managerGetState(Manager *manager, Manager_module module) {
    char FUN_NAME[32] = "managerGetState";
    assertl(manager, FILE_MANAGER, FUN_NAME, -1, "manager NULL");

    Manager_state state;
    Buffer_module *buffer;

    buffer = getModuleBuffer(manager, module);
    pthread_mutex_lock(buffer->mutex_access_buffer);
    state = buffer->state;
    pthread_mutex_unlock(buffer->mutex_access_buffer);

    return state;
}

Manager_error managerSend(Manager *manager, Manager_module module, Packet *packet) {
    char FUN_NAME[32] = "managerSend";
    assertl(manager, FILE_MANAGER, FUN_NAME, -1, "manager NULL");
    assertl(packet, FILE_MANAGER, FUN_NAME, -1, "packet NULL");

    Buffer_module *buffer;
    Manager_error error = MANAGER_ERR_SUCCESS;
    Packet *p_send = packetCopy(packet);

    buffer = getModuleBuffer(manager, module);
    pthread_mutex_lock(buffer->mutex_access_buffer);
    if (buffer->state == MANAGER_STATE_CLOSED) {
        warnl(FILE_MANAGER, FUN_NAME, "manager close, failed to send packet");
        error = MANAGER_ERR_CLOSED;
        deinitPacket(&p_send);
    } else {
        genListAdd(buffer->buff, (void *)p_send);
        error = MANAGER_ERR_SUCCESS;
    }
    pthread_mutex_unlock(buffer->mutex_wait_read);
    pthread_mutex_unlock(buffer->mutex_access_buffer);
    return error;
}

Manager_error managerReceiveBlocking(Manager *manager, Manager_module module, Packet **packet) {
    char FUN_NAME[32] = "managerReceiveBlocking";
    assertl(manager, FILE_MANAGER, FUN_NAME, -1, "manager NULL");
    assertl(packet, FILE_MANAGER, FUN_NAME, -1, "packet NULL");

    Buffer_module *buffer;
    Manager_error error = MANAGER_ERR_SUCCESS;

    buffer = getModuleBuffer(manager, module);
    pthread_mutex_lock(buffer->mutex_wait_read);
    error = managerReceiveNonBlocking(manager, module, packet);
    if (error == MANAGER_ERR_RETRY) {
        warnl(FILE_MANAGER, FUN_NAME, "nothing to read");
    }
    return error;
}

Manager_error managerReceiveNonBlocking(Manager *manager, Manager_module module, Packet **packet) {
    char FUN_NAME[32] = "managerReceiveNonBlocking";
    assertl(manager, FILE_MANAGER, FUN_NAME, -1, "manager NULL");
    assertl(packet, FILE_MANAGER, FUN_NAME, -1, "packet NULL");

    Buffer_module *buffer;
    Manager_error error = MANAGER_ERR_SUCCESS;

    buffer = getModuleBuffer(manager, module);
    (void)pthread_mutex_trylock(buffer->mutex_wait_read);
    pthread_mutex_lock(buffer->mutex_access_buffer);

    if (buffer->state == MANAGER_STATE_CLOSED) {
        warnl(FILE_MANAGER, FUN_NAME, "manager close, failed to read packet");
        *packet = NULL;
        pthread_mutex_unlock(buffer->mutex_wait_read);
        error = MANAGER_ERR_CLOSED;
    } else if (genListSize(buffer->buff) == 0) {
        *packet = NULL;
        error = MANAGER_ERR_RETRY;
    } else {
        *packet = genListPop(buffer->buff);
        error = MANAGER_ERR_SUCCESS;

        if (genListSize(buffer->buff) > 0) {
            pthread_mutex_unlock(buffer->mutex_wait_read);
        }
    }

    pthread_mutex_unlock(buffer->mutex_access_buffer);
    return error;
}

Manager_error managerMainReceive(Manager *manager, pthread_t *num_t) {
    char FUN_NAME[32] = "managerMainReceive";
    assertl(manager, FILE_MANAGER, FUN_NAME, -1, "manager NULL");
    assertl(num_t, FILE_MANAGER, FUN_NAME, -1, "num_t NULL");
    pthread_t *t;
    Manager_error error;

    /* blocking call */
    pthread_mutex_lock(manager->main.mutex_wait_read);
    pthread_mutex_lock(manager->main.mutex_access_buffer);

    if (manager->main.state == MANAGER_STATE_CLOSED) {
        /* manager closed */
        warnl(FILE_MANAGER, FUN_NAME, "manager close, failed to read packet");
        *num_t = 0;
        pthread_mutex_unlock(manager->main.mutex_wait_read);
        error = MANAGER_ERR_CLOSED;
    } else if (genListIsEmpty(manager->main.buff)) {
        /* buffer empty */
        warnl(FILE_MANAGER, FUN_NAME, "nothing to read");
        error = MANAGER_ERR_RETRY;
    } else {
        /* read */
        t = genListPop(manager->main.buff);
        *num_t = *t;
        free(t);
        error = MANAGER_ERR_SUCCESS;
        if (!genListIsEmpty(manager->main.buff)) {
            pthread_mutex_unlock(manager->main.mutex_wait_read);
        }
    }
    pthread_mutex_unlock(manager->main.mutex_access_buffer);
    return error;
}

Manager_error managerMainSendPthreadToJoin(Manager *manager, pthread_t num_t) {
    char FUN_NAME[32] = "managerMainSendPthreadToJoin";
    assertl(manager, FILE_MANAGER, FUN_NAME, -1, "manager NULL");

    pthread_t *t = malloc(sizeof(pthread_t));
    t = malloc(sizeof(pthread_t));
    *t = num_t;
    pthread_mutex_lock(manager->main.mutex_access_buffer);
    genListAdd(manager->main.buff, t);
    pthread_mutex_unlock(manager->main.mutex_access_buffer);
    pthread_mutex_unlock(manager->main.mutex_wait_read);
    return MANAGER_ERR_SUCCESS;
}
