#include <stdio.h>
#include <string.h>

#include "../src/vbuffer.h"
#include "test.h"


const char* test_new_vbuffer(void) {
    struct vbuffer *buffer = vbuffer_new();

    ASSERT_TRUE(buffer, "buffer is created");
    ASSERT_TRUE(buffer->length == 0, "no initial content");
    ASSERT_TRUE(buffer->capacity == INITIAL_BUFFER_CAPACITY, "no initial capacity");
    ASSERT_TRUE(buffer->data, "initial buffer allocated");

    vbuffer_free(buffer);
    return NULL;
}

const char* test_vbuffer_pushchar(void) {
    struct vbuffer *buffer = vbuffer_new();

    ASSERT_TRUE(buffer, "buffer is created");

    int test_count = 20, result;
    for (int i = 0; i < test_count; ++i) {
        result = vbuffer_pushchar(buffer, i);
        ASSERT_TRUE(result, "reported success");
        ASSERT_TRUE(buffer->length == i + 1, "correct size after push");
    }
    for (int i = 0; i < test_count; ++i) {
        ASSERT_TRUE(buffer->data[i] == i, "buffer has correct contents");
    }

    vbuffer_free(buffer);
    return NULL;
}

const char* test_vbuffer_readfile(void) {
    struct vbuffer *buffer = vbuffer_new();

    ASSERT_TRUE(buffer, "buffer is created");
    int result = vbuffer_readfile(buffer, "tests/testdata.bin");

    ASSERT_TRUE(result, "reported success");
    ASSERT_TRUE(buffer->length == 270, "read correct amount of data from file");
    ASSERT_TRUE(buffer->data[0x00]  == (char)0x41, "correct byte at pos 0x00");
    ASSERT_TRUE(buffer->data[0x45]  == (char)0x9C, "correct byte at pos 0x45");
    ASSERT_TRUE(buffer->data[0xB8]  == (char)0x23, "correct byte at pos 0xB8");
    ASSERT_TRUE(buffer->data[0x10D] == (char)0xF5, "correct byte at pos 0x10D");

    vbuffer_free(buffer);
    return NULL;
}

const char *test_suite_name = "vbuffer.c";
struct test_def test_list[] = {
    {   "new_vbuffer",                              test_new_vbuffer },
    {   "vbuffer_pushchar",                         test_vbuffer_pushchar },
    {   "vbuffer_readfile",                         test_vbuffer_readfile },

    {   NULL,                                       NULL }
};
