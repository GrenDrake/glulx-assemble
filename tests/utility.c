#include <stdio.h>
#include <string.h>

#include "test.h"

char *str_dup(const char *source);
int cleanup_string(char *text);


int test_str_dup();

int test_cleanup_string_escapes_midtext();
int test_cleanup_string_escapes_initialfinal();
int test_cleanup_string_escapes_codes();
int test_cleanup_string_escapes_invalid_code();
int test_cleanup_string_fix_multiline();
int test_cleanup_string_fix_multiline_codes();


const char *test_suite_name = "utility.c";
struct test_def test_list[] = {
    {   "str_dup",                              test_str_dup },

    {   "cleanup_string_escapes_midtext",       test_cleanup_string_escapes_midtext },
    {   "cleanup_string_escapes_initialfinal",  test_cleanup_string_escapes_initialfinal },
    {   "cleanup_string_escapes_codes",         test_cleanup_string_escapes_codes },
    {   "cleanup_string_escapes_invalid_code",  test_cleanup_string_escapes_invalid_code },
    {   "cleanup_string_fix_multiline",         test_cleanup_string_fix_multiline },
    {   "cleanup_string_fix_multiline_codes",   test_cleanup_string_fix_multiline_codes },

    {   NULL,                       NULL }
};


int test_str_dup(void) {
    const char *source = "Hello World!\n";
    char *copy = str_dup(source);

    ASSERT_TRUE(copy != NULL, "copy is non-NULL");
    ASSERT_TRUE(copy != source, "copy is distinct");
    ASSERT_TRUE(strcmp(copy, source) == 0, "copy has same content");

    return TRUE;
}


int test_cleanup_string_escapes_midtext() {
    char test_text[] = "text\\n\\nword";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result == 0,
                "correct return value");
    ASSERT_TRUE(strcmp(test_text, "text\n\nword") == 0,
                "handled mid-text newlines");

    return TRUE;
}

int test_cleanup_string_escapes_initialfinal() {
    char test_text[] = "\\ntext word\\n";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result == 0,
                "correct return value");
    ASSERT_TRUE(strcmp(test_text, "\ntext word\n") == 0,
                "handled initial and final newlines");

    return TRUE;
}

int test_cleanup_string_escapes_codes() {
    char test_text[] = "\\\\\\n\\\"\\'";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result == 0,
                "correct return value");
    ASSERT_TRUE(strcmp(test_text, "\\\n\"\'") == 0,
                "handled valid escape codes");

    return TRUE;
}

int test_cleanup_string_escapes_invalid_code() {
    char test_text[] = "\\q";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result != 0,
                "correct return value");

    return TRUE;
}

int test_cleanup_string_fix_multiline() {
    char test_text[] = "Hello there      \n\n     Everyone.";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result == 0,
                "correct return value");
    ASSERT_TRUE(strcmp(test_text, "Hello there Everyone.") == 0,
                "correct resulting string");

    return TRUE;
}

int test_cleanup_string_fix_multiline_codes() {
    char test_text[] = "Hello \\\"there\\\"      \n\n     \\\"Everyone\\\".";
    int result = cleanup_string(test_text);
    ASSERT_TRUE(result == 0,
                "correct return value");
    ASSERT_TRUE(strcmp(test_text, "Hello \"there\" \"Everyone\".") == 0,
                "correct resulting string");

    return TRUE;
}
