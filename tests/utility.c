#include <stdio.h>
#include <string.h>

#include "test.h"
#include "../src/utility.h"



const char* test_str_dup();

const char* test_cleanup_string_escapes_midtext();
const char* test_cleanup_string_escapes_initialfinal();
const char* test_cleanup_string_escapes_codes();
const char* test_cleanup_string_escapes_invalid_code();
const char* test_cleanup_string_fix_multiline();
const char* test_cleanup_string_fix_multiline_codes();

const char* test_utf8_next_char();
const char* test_utf8_next_char_stray_continuation();
const char* test_utf8_next_char_malformed();



const char *test_suite_name = "utility.c";
struct test_def test_list[] = {
    {   "str_dup",                              test_str_dup },

    {   "cleanup_string_escapes_midtext",       test_cleanup_string_escapes_midtext },
    {   "cleanup_string_escapes_initialfinal",  test_cleanup_string_escapes_initialfinal },
    {   "cleanup_string_escapes_codes",         test_cleanup_string_escapes_codes },
    {   "cleanup_string_escapes_invalid_code",  test_cleanup_string_escapes_invalid_code },
    {   "cleanup_string_fix_multiline",         test_cleanup_string_fix_multiline },
    {   "cleanup_string_fix_multiline_codes",   test_cleanup_string_fix_multiline_codes },

    {   "utf8_next_char",                       test_utf8_next_char },
    {   "utf8_next_char_stray_continuation",    test_utf8_next_char_stray_continuation },
    {   "utf8_next_char_malformed",             test_utf8_next_char_malformed },

    {   NULL,                       NULL }
};


const char* test_str_dup(void) {
    const char *source = "Hello World!\n";
    char *copy = str_dup(source);

    ASSERT_TRUE(copy != NULL, "copy is non-NULL");
    ASSERT_TRUE(copy != source, "copy is distinct");
    ASSERT_TRUE(strcmp(copy, source) == 0, "copy has same content");

    return NULL;
}


const char* test_cleanup_string_escapes_midtext() {
    char test_text[] = "text\\n\\nword";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result == 0,
                "correct return value");
    ASSERT_TRUE(strcmp(test_text, "text\n\nword") == 0,
                "handled mid-text newlines");

    return NULL;
}

const char* test_cleanup_string_escapes_initialfinal() {
    char test_text[] = "\\ntext word\\n";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result == 0,
                "correct return value");
    ASSERT_TRUE(strcmp(test_text, "\ntext word\n") == 0,
                "handled initial and final newlines");

    return NULL;
}

const char* test_cleanup_string_escapes_codes() {
    char test_text[] = "\\\\\\n\\\"\\'";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result == 0,
                "correct return value");
    ASSERT_TRUE(strcmp(test_text, "\\\n\"\'") == 0,
                "handled valid escape codes");

    return NULL;
}

const char* test_cleanup_string_escapes_invalid_code() {
    char test_text[] = "\\q";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result != 0,
                "correct return value");

    return NULL;
}

const char* test_cleanup_string_fix_multiline() {
    char test_text[] = "Hello there      \n\n     Everyone.";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result == 0,
                "correct return value");
    ASSERT_TRUE(strcmp(test_text, "Hello there Everyone.") == 0,
                "correct resulting string");

    return NULL;
}

const char* test_cleanup_string_fix_multiline_codes() {
    char test_text[] = "Hello \\\"there\\\"      \n\n     \\\"Everyone\\\".";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result == 0,
                "correct return value");
    ASSERT_TRUE(strcmp(test_text, "Hello \"there\" \"Everyone\".") == 0,
                "correct resulting string");

    return NULL;
}

const char* test_utf8_next_char() {
    const char *text = "\x24\xC2\xA2\xE2\x82\xAC\xF0\x90\x8D\x88";
    int pos = 0;
    int codepoint = 0;

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == 0x24, "first character correct");
    ASSERT_TRUE(pos == 1, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == 0xA2, "second character correct");
    ASSERT_TRUE(pos == 3, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == 0x20AC, "third character correct");
    ASSERT_TRUE(pos == 6, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == 0x10348, "fourth character correct");
    ASSERT_TRUE(pos == 10, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == 0, "correctly found end of string");

    return NULL;
}

const char* test_utf8_next_char_stray_continuation() {
    const char *text = "\xA2";
    int pos = 0;
    int codepoint = 0;

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == UTF8_REPLACEMENT_CHAR, "first character returned invalid");
    ASSERT_TRUE(pos == 1, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == 0, "correctly found end of string");

    return NULL;
}

const char* test_utf8_next_char_malformed() {
    const char *text = "\xC2\xE2\x82\xF0\x90\x8D";
    int pos = 0;
    int codepoint = 0;

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == UTF8_REPLACEMENT_CHAR, "first character returned invalid");
    ASSERT_TRUE(pos == 1, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == UTF8_REPLACEMENT_CHAR, "second character returned invalid");
    ASSERT_TRUE(pos == 2, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == UTF8_REPLACEMENT_CHAR, "third character returned invalid");
    ASSERT_TRUE(pos == 3, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == UTF8_REPLACEMENT_CHAR, "fourth character returned invalid");
    ASSERT_TRUE(pos == 4, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == UTF8_REPLACEMENT_CHAR, "fifth character returned invalid");
    ASSERT_TRUE(pos == 5, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == UTF8_REPLACEMENT_CHAR, "sixth returned invalid");
    ASSERT_TRUE(pos == 6, "pos pointed at start of next character");

    codepoint = utf8_next_char(text, &pos);
    ASSERT_TRUE(codepoint == 0, "correctly found end of string");

    return NULL;
}
