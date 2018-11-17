#include <stdio.h>
#include <stdlib.h>

#include "vbuffer.h"

struct vbuffer* vbuffer_new(void) {
    struct vbuffer *buf = malloc(sizeof(struct vbuffer));
    if (!buf) return NULL;
    buf->length = 0;
    buf->capacity = INITIAL_BUFFER_CAPACITY;
    buf->data = malloc(INITIAL_BUFFER_CAPACITY);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    return buf;
}

void vbuffer_free(struct vbuffer *buffer) {
    if (!buffer) return;
    if (buffer->data) free(buffer->data);
    free(buffer);
}

int vbuffer_pushchar(struct vbuffer *buffer, char c) {
    if (!buffer) return 0;
    if (buffer->length >= buffer->capacity) {
        int new_capacity = buffer->capacity * 2;
        char *new_buffer = realloc(buffer->data, new_capacity);
        if (!new_buffer) {
            return 0;
        }
        buffer->data = new_buffer;
        buffer->capacity = new_capacity;
    }
    buffer->data[buffer->length] = c;
    ++buffer->length;
    return 1;
}

int vbuffer_readfile(struct vbuffer *buffer, const char *filename) {
    if (!buffer) return 0;
    FILE *source = fopen(filename, "rb");
    if (!source) return 0;

    while (1) {
        int c = getc(source);
        if (c == EOF) break;
        vbuffer_pushchar(buffer, c);
    }

    return 1;
}
