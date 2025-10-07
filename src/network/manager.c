#include <network/manager.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <types/genericlist.h>
#include <types/packet.h>
#include <unistd.h>
#include <utils/logger.h>
#include <utils/project_constants.h>


char new_message = 'a';

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
    pipe(buffer->fd_alert);
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
    strncpy(manager->user_id, "<undefined>", SIZE_NAME);
    return manager;
}

void deinitManagerBuffer(Buffer_module *buffer) {
    pthread_mutex_destroy(buffer->mutex_wait_read);
    pthread_mutex_destroy(buffer->mutex_access_buffer);
    free(buffer->mutex_wait_read);
    free(buffer->mutex_access_buffer);
    deinitGenList(&(buffer->buff), deinitPacketGen);
}

void deinitManager(Manager **manager) {
        ASSERTL(manager,-1, "manager NULL")
    ASSERTL(*manager,-1, "*manager NULL")
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
        int ret_error = pthread_mutex_trylock(buffer->mutex_wait_read);
        (void)ret_error;
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
        int ret_error = pthread_mutex_trylock(buffer->mutex_wait_read);
        (void)ret_error;
        break;
    }
}

void managerSetState(Manager *manager, Manager_module module, Manager_state state) {
        ASSERTL(manager,-1, "manager NULL")

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
        ASSERTL(manager,-1, "manager NULL")
    ASSERTL(user_id,-1, "user_id NULL")

    strncpy(manager->user_id, user_id, SIZE_NAME);
}

char *managerGetUser(Manager *manager) {
        ASSERTL(manager,-1, "manager NULL")

    char *name = malloc(SIZE_NAME);
    if (manager->user_id[0] == 0) {
        strncpy(name, "Unknown", SIZE_NAME);
    } else {
        strncpy(name, manager->user_id, SIZE_NAME);
    }
    return name;
}

Manager_state managerGetState(Manager *manager, Manager_module module) {
        ASSERTL(manager,-1, "manager NULL")

    Manager_state state;
    Buffer_module *buffer;

    buffer = getModuleBuffer(manager, module);
    pthread_mutex_lock(buffer->mutex_access_buffer);
    state = buffer->state;
    pthread_mutex_unlock(buffer->mutex_access_buffer);

    return state;
}

Manager_error managerSend(Manager *manager, Manager_module module, Packet *packet) {
        ASSERTL(manager,-1, "manager NULL")
    ASSERTL(packet,-1, "packet NULL")

    Buffer_module *buffer;
    Manager_error error = MANAGER_ERR_SUCCESS;
    Packet *p_send = packetCopy(packet);

    buffer = getModuleBuffer(manager, module);
    pthread_mutex_lock(buffer->mutex_access_buffer);
    if (buffer->state == MANAGER_STATE_CLOSED) {
        WARNL("manager %s in STATE_CLOSED, failed to send packet",
              managerModuleToString(module))
        error = MANAGER_ERR_CLOSED;
        deinitPacket(&p_send);
    } else {
        genListAdd(buffer->buff, (void *)p_send);
        write(buffer->fd_alert[1], &new_message, sizeof(new_message));
        error = MANAGER_ERR_SUCCESS;
    }
    pthread_mutex_unlock(buffer->mutex_wait_read);
    pthread_mutex_unlock(buffer->mutex_access_buffer);
    return error;
}

Manager_error managerReceiveBlocking(Manager *manager, Manager_module module, Packet **packet) {
        ASSERTL(manager,-1, "manager NULL")
    ASSERTL(packet,-1, "packet NULL")

    Buffer_module *buffer;
    Manager_error error = MANAGER_ERR_SUCCESS;

    buffer = getModuleBuffer(manager, module);
    pthread_mutex_lock(buffer->mutex_wait_read);
    error = managerReceiveNonBlocking(manager, module, packet);
    if (error == MANAGER_ERR_RETRY) {
        WARNL("nothing to read")
    }
    return error;
}

Manager_error managerReceiveNonBlocking(Manager *manager, Manager_module module, Packet **packet) {
    ASSERTL(manager,-1, "manager NULL")
    ASSERTL(packet,-1, "packet NULL")

    Buffer_module *buffer;
    Manager_error error = MANAGER_ERR_SUCCESS;

    buffer = getModuleBuffer(manager, module);
    int ret_error = pthread_mutex_trylock(buffer->mutex_wait_read);
    (void)ret_error;
    pthread_mutex_lock(buffer->mutex_access_buffer);

    if (buffer->state == MANAGER_STATE_CLOSED) {
        WARNL("manager %s in STATE_CLOSED, failed to read packet",
              managerModuleToString(module))
        *packet = NULL;
        pthread_mutex_unlock(buffer->mutex_wait_read);
        error = MANAGER_ERR_CLOSED;
    } else if (genListSize(buffer->buff) == 0) {
        *packet = NULL;
        error = MANAGER_ERR_RETRY;
    } else {
        char buff;
        *packet = genListPop(buffer->buff);
        read(buffer->fd_alert[0], &buff, sizeof(buff));
        error = MANAGER_ERR_SUCCESS;

        if (genListSize(buffer->buff) > 0) {
            pthread_mutex_unlock(buffer->mutex_wait_read);
        }
    }

    pthread_mutex_unlock(buffer->mutex_access_buffer);
    return error;
}

Manager_error managerMainReceive(Manager *manager, pthread_t *num_t) {
        ASSERTL(manager,-1, "manager NULL")
    ASSERTL(num_t,-1, "num_t NULL")
    pthread_t *t;
    Manager_error error;

    /* blocking call */
    pthread_mutex_lock(manager->main.mutex_wait_read);
    pthread_mutex_lock(manager->main.mutex_access_buffer);

    if (manager->main.state == MANAGER_STATE_CLOSED) {
        /* manager closed */
        WARNL("manager close, failed to read packet")
        *num_t = 0;
        pthread_mutex_unlock(manager->main.mutex_wait_read);
        error = MANAGER_ERR_CLOSED;
    } else if (genListIsEmpty(manager->main.buff)) {
        /* buffer empty */
        WARNL("nothing to read")
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
        ASSERTL(manager,-1, "manager NULL")

    pthread_t *t = malloc(sizeof(pthread_t));
    t = malloc(sizeof(pthread_t));
    *t = num_t;
    pthread_mutex_lock(manager->main.mutex_access_buffer);
    genListAdd(manager->main.buff, t);
    pthread_mutex_unlock(manager->main.mutex_access_buffer);
    pthread_mutex_unlock(manager->main.mutex_wait_read);
    return MANAGER_ERR_SUCCESS;
}

bool isManagerModuleOpen(Manager *manager) {
        ASSERTL(manager,-1, "manager NULL")

    if (manager->input.state != MANAGER_STATE_CLOSED)
        return true;
    if (manager->output.state != MANAGER_STATE_CLOSED)
        return true;
    if (manager->server.state != MANAGER_STATE_CLOSED)
        return true;
    if (manager->peer.state != MANAGER_STATE_CLOSED)
        return true;
    return false;
}

char *managerErrorToString(Manager_error error) {
    switch (error) {
    case MANAGER_ERR_SUCCESS:
        return "MANAGER_ERR_SUCCESS";
    case MANAGER_ERR_ERROR:
        return "MANAGER_ERR_ERROR";
    case MANAGER_ERR_CLOSED:
        return "MANAGER_ERR_CLOSED";
    case MANAGER_ERR_RETRY:
        return "MANAGER_ERR_RETRY";
    default:
        return "Unknown";
    }
}

char *managerModuleToString(Manager_module module) {
    switch (module) {
    case MANAGER_MOD_INPUT:
        return "MANAGER_MOD_INPUT";
    case MANAGER_MOD_OUTPUT:
        return "MANAGER_MOD_OUTPUT";
    case MANAGER_MOD_SERVER:
        return "MANAGER_MOD_SERVER";
    case MANAGER_MOD_PEER:
        return "MANAGER_MOD_PEER";
    case MANAGER_MOD_MAIN:
        return "MANAGER_MOD_MAIN";
    }
}

int managerGetFDAlert(Manager *manager, Manager_module module) {
        ASSERTL(manager,-1, "manager NULL")
    ASSERTL(manager,-1, "fd NULL")

    Buffer_module *buffer;
    Manager_error error = MANAGER_ERR_SUCCESS;

    buffer = getModuleBuffer(manager, module);
    return buffer->fd_alert[0];
}

