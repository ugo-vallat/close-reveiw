#include <utils/message.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Msg *init_message(const char *string) {
    Msg *message = malloc(sizeof(Msg));
    if (message == NULL) {
        perror("Malloc Failed (most likely ran out of memory):");
        exit(EXIT_FAILURE);
    }
    memset(message, 0, sizeof(Msg));
    if (string != NULL) {
        size_t size_str = strnlen(string, SIZE_MSG_DATA);
        memcpy(message->buffer, string, size_str);
        message->size = size_str;
    }
    return message;
}

unsigned int get_size(Msg *message) {
    return message->size;
}

bool append_string(Msg *dest, const char *string) {
    size_t size_str = strnlen(string, SIZE_MSG_DATA);
    if ((dest->size + size_str) >= SIZE_MSG_DATA)
        return false;
    for (int i = 0; i < size_str; i++) {
        dest->buffer[dest->size + i] = string[i];
    }
    dest->size += size_str;
    dest->buffer[dest->size] = '\0';
    return true;
}

bool append_message(Msg *dest, const Msg *src) {
    if ((dest->size + src->size) >= SIZE_MSG_DATA)
        return false;
    for (int i = 0; i < src->size; i++) {
        dest->buffer[dest->size + i] = src->buffer[i];
    }
    dest->size += src->size;
    dest->buffer[dest->size] = '\0';
    return true;
}

void print_message(Msg *message) {
    printf("%*s\n", message->size, message->buffer);
}

void deinit_message(Msg *message) {
    free(message);
}