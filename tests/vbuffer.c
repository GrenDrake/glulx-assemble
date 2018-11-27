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
        result = vbuffer_pushchar(buffer, i + 120);
        ASSERT_TRUE(result, "reported success");
        ASSERT_TRUE(buffer->length == i + 1, "correct size after push");
    }
    for (int i = 0; i < test_count; ++i) {
        ASSERT_TRUE(buffer->data[i] == (char)(i + 120), "buffer has correct contents");
    }

    vbuffer_free(buffer);
    return NULL;
}

const char* test_vbuffer_pushshort(void) {
    struct vbuffer *buffer = vbuffer_new();

    ASSERT_TRUE(buffer, "buffer is created");

    vbuffer_pushshort(buffer, 4660);
    vbuffer_pushshort(buffer, 22136);
    vbuffer_pushshort(buffer, 4776);
    ASSERT_TRUE(buffer->length == 6, "buffer is correct size");

    ASSERT_TRUE(buffer->data[0] == 18, "first byte correct");
    ASSERT_TRUE(buffer->data[1] == 52, "second byte correct");
    ASSERT_TRUE(buffer->data[2] == 86, "third byte correct");
    ASSERT_TRUE(buffer->data[3] == 120, "fourth byte correct");
    ASSERT_TRUE(buffer->data[4] == 18, "fifth byte correct");
    ASSERT_TRUE(buffer->data[5] == (char)168, "sixth byte correct");

    vbuffer_free(buffer);
    return NULL;
}

const char* test_vbuffer_pushword(void) {
    struct vbuffer *buffer = vbuffer_new();

    ASSERT_TRUE(buffer, "buffer is created");

    vbuffer_pushword(buffer, 930690356);
    vbuffer_pushword(buffer, 3986192095);
    vbuffer_pushword(buffer, 4099116557);
    ASSERT_TRUE(buffer->length == 12, "buffer is correct size");

    ASSERT_TRUE(buffer->data[0] == 55, "first byte correct");
    ASSERT_TRUE(buffer->data[1] == 121, "second byte correct");
    ASSERT_TRUE(buffer->data[2] == 53, "third byte correct");
    ASSERT_TRUE(buffer->data[3] == 52, "fourth byte correct");

    ASSERT_TRUE(buffer->data[4] == (char)237, "fifth byte correct");
    ASSERT_TRUE(buffer->data[5] == (char)152, "sixth byte correct");
    ASSERT_TRUE(buffer->data[6] == 118, "seventh byte correct");
    ASSERT_TRUE(buffer->data[7] == (char)223, "eigth byte correct");

    ASSERT_TRUE(buffer->data[8] == (char)244, "ninth byte correct");
    ASSERT_TRUE(buffer->data[9] == 83, "tenth byte correct");
    ASSERT_TRUE(buffer->data[10] == (char)142, "eleventh byte correct");
    ASSERT_TRUE(buffer->data[11] == 13, "twelfth byte correct");

    vbuffer_free(buffer);
    return NULL;
}

const char* test_vbuffer_setshort(void) {
    struct vbuffer *buffer = vbuffer_new();

    ASSERT_TRUE(buffer, "buffer is created");

    for (int i = 0; i < 5; ++i) {
        vbuffer_pushchar(buffer, 0);
    }
    ASSERT_TRUE(buffer->length == 5, "initial buffer is correct size");

    vbuffer_setshort(buffer, 14201, 1);
    vbuffer_setshort(buffer, 13779, 4);
    ASSERT_TRUE(buffer->length == 6, "final buffer is correct size");

    ASSERT_TRUE(buffer->data[1] == 55, "first byte correct");
    ASSERT_TRUE(buffer->data[2] == 121, "second byte correct");

    ASSERT_TRUE(buffer->data[4] == 53, "third byte correct");
    ASSERT_TRUE(buffer->data[5] == (char)211, "fourth byte correct");

    vbuffer_free(buffer);
    return NULL;
}

const char* test_vbuffer_setword(void) {
    struct vbuffer *buffer = vbuffer_new();

    ASSERT_TRUE(buffer, "buffer is created");

    for (int i = 0; i < 6; ++i) {
        vbuffer_pushchar(buffer, 0);
    }
    ASSERT_TRUE(buffer->length == 6, "initial buffer is correct size");

    vbuffer_setword(buffer, 930690356, 1);
    vbuffer_setword(buffer, 3986192095, 7);
    ASSERT_TRUE(buffer->length == 11, "final buffer is correct size");

    ASSERT_TRUE(buffer->data[1] == 55, "first byte correct");
    ASSERT_TRUE(buffer->data[2] == 121, "second byte correct");
    ASSERT_TRUE(buffer->data[3] == 53, "third byte correct");
    ASSERT_TRUE(buffer->data[4] == 52, "fourth byte correct");

    ASSERT_TRUE(buffer->data[7] == (char)237, "fifth byte correct");
    ASSERT_TRUE(buffer->data[8] == (char)152, "sixth byte correct");
    ASSERT_TRUE(buffer->data[9] == 118, "seventh byte correct");
    ASSERT_TRUE(buffer->data[10] == (char)223, "eigth byte correct");

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

const char* test_vbuffer_pad_by(void) {
    struct vbuffer *buffer = vbuffer_new();
    ASSERT_TRUE(buffer, "buffer is created");

    vbuffer_pad_by(buffer, 0x12, 21);
    ASSERT_TRUE(buffer->length == 21, "correctly added 21 bytes");
    ASSERT_TRUE(buffer->data[8] == 0x12, "bytes have correct value of 0x12");

    vbuffer_pad_by(buffer, 0x0E, 12);
    ASSERT_TRUE(buffer->length == 33, "correctly added 12 bytes");
    ASSERT_TRUE(buffer->data[25] == 0x0E, "bytes have correct value of 0x0E");

    vbuffer_free(buffer);
    return NULL;
}

const char* test_vbuffer_pad_to(void) {
    struct vbuffer *buffer = vbuffer_new();
    ASSERT_TRUE(buffer, "buffer is created");

    vbuffer_pad_to(buffer, 0x77, 17);
    ASSERT_TRUE(buffer->length == 17, "padded correctly to 17");
    ASSERT_TRUE(buffer->data[8] == 0x77, "first padding has correct value");

    vbuffer_pad_to(buffer, 0x45, 33);
    ASSERT_TRUE(buffer->length == 33, "padded correctly to 33");
    ASSERT_TRUE(buffer->data[20] == 0x45, "second padding has correct value");

    vbuffer_pad_to(buffer, 0x99, 10);
    ASSERT_TRUE(buffer->length == 40, "padded correctly to 40 (multiple of 10)");
    ASSERT_TRUE(buffer->data[35] == (char)0x99, "third padding has correct value");

    vbuffer_free(buffer);
    return NULL;
}

const char *test_suite_name = "vbuffer.c";
struct test_def test_list[] = {
    {   "new_vbuffer",                              test_new_vbuffer },
    {   "vbuffer_pushchar",                         test_vbuffer_pushchar },
    {   "vbuffer_pushshort",                        test_vbuffer_pushshort },
    {   "vbuffer_pushword",                         test_vbuffer_pushword },
    {   "vbuffer_setshort",                         test_vbuffer_setshort },
    {   "vbuffer_setword",                          test_vbuffer_setword },
    {   "vbuffer_readfile",                         test_vbuffer_readfile },
    {   "vbuffer_pad_to",                           test_vbuffer_pad_to },
    {   "vbuffer_pad_by",                           test_vbuffer_pad_by },

    {   NULL,                                       NULL }
};
