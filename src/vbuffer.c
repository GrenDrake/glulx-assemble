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

int vbuffer_pushshort(struct vbuffer *buffer, unsigned c) {
    if (!buffer) return 0;
    c &= 0xFFFF;
    vbuffer_pushchar(buffer, (c & 0xFF00) >> 8);
    vbuffer_pushchar(buffer, c & 0xFF);
    return 1;
}

int vbuffer_pushword(struct vbuffer *buffer, unsigned c) {
    if (!buffer) return 0;
    vbuffer_pushchar(buffer, (c & 0xFF000000) >> 24);
    vbuffer_pushchar(buffer, (c & 0xFF0000) >> 16);
    vbuffer_pushchar(buffer, (c & 0xFF00) >> 8);
    vbuffer_pushchar(buffer, c & 0xFF);
    return 1;
}

int vbuffer_setshort(struct vbuffer *buffer, unsigned new_value, unsigned position) {
    if (!buffer) return 0;
    new_value &= 0xFFFF;
    while (position + 2 > buffer->length) {
        vbuffer_pushchar(buffer, 0);
    }

    buffer->data[position] = (new_value & 0xFF00) >> 8;
    buffer->data[position + 1] = new_value & 0xFF;
    return 1;
}

int vbuffer_setword(struct vbuffer *buffer, unsigned new_value, unsigned position) {
    if (!buffer) return 0;
    while (position + 4 > buffer->length) {
        vbuffer_pushchar(buffer, 0);
    }

    buffer->data[position]     = (new_value & 0xFF000000) >> 24;
    buffer->data[position + 1] = (new_value & 0xFF0000) >> 16;
    buffer->data[position + 2] = (new_value & 0xFF00) >> 8;
    buffer->data[position + 3] = new_value & 0xFF;
    return 1;
}

int vbuffer_readfile(struct vbuffer *buffer, const char *filename) {
    if (!buffer) return 0;
    FILE *source = NULL;
    if (filename)   source = fopen(filename, "rb");
    else            source = stdin;
    if (!source) return 0;

    while (1) {
        int c = getc(source);
        if (c == EOF) break;
        vbuffer_pushchar(buffer, c);
    }

    if (source != stdin) {
        fclose(source);
    }
    return 1;
}

int vbuffer_writefile(struct vbuffer *buffer, const char *filename) {
    int success = 1;
    if (!buffer) return 0;
    FILE *source = NULL;
    if (filename)   source = fopen(filename, "wb");
    else            source = stdout;
    if (!source) return 0;

    int count = fwrite(buffer->data, buffer->length, 1, source);
    if (count != 1) {
        success = 0;
    }

    if (source != stdout) {
        fclose(source);
    }
    return success;
}
