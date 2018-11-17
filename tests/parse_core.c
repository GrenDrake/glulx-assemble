#include <stdio.h>
#include <string.h>

#include "test.h"
#include "../src/assemble.h"



const char* test_remove_line_middle(void);
const char* test_remove_line_start(void);
const char* test_remove_line_end_noeol(void);
const char* test_remove_line_end_w_eol(void);
const char* test_remove_line_all_tokens(void);




const char *test_suite_name = "parse_core.c";
struct test_def test_list[] = {
    {   "remove_line_middle",                   test_remove_line_middle },
    {   "remove_line_start",                    test_remove_line_start },
    {   "remove_line_end_noeol",                test_remove_line_end_noeol },
    {   "remove_line_end_w_eol",                test_remove_line_end_w_eol },
    {   "remove_line_all_tokens",               test_remove_line_all_tokens },

    {   NULL,                       NULL }
};

const char* test_remove_line_middle(void) {
    struct token_list *list = init_token_list();

    struct token *t1 = new_rawint_token(1, NULL);
    add_token(list, t1);

    struct token *t2 = new_rawint_token(2, NULL);
    add_token(list, t2);

    struct token *t3 = new_rawint_token(3, NULL);
    add_token(list, t3);

    struct token *t4 = new_token(tt_eol, NULL, NULL);
    add_token(list, t4);

    struct token *t5 = new_rawint_token(4, NULL);
    add_token(list, t5);

    remove_line(list, t2);

    ASSERT_TRUE(list->first == t1, "first token is unchanged");
    ASSERT_TRUE(list->last == t5, "last token is unchanged");
    ASSERT_TRUE(list->first->next == t5, "next token after first is last token");
    ASSERT_TRUE(list->last->prev == t1, "prev token before last is first token");

    free_token_list(list);
    return NULL;
}

const char* test_remove_line_start(void) {
    struct token_list *list = init_token_list();

    struct token *t1 = new_rawint_token(1, NULL);
    add_token(list, t1);

    struct token *t2 = new_rawint_token(2, NULL);
    add_token(list, t2);

    struct token *t3 = new_token(tt_eol, NULL, NULL);
    add_token(list, t3);

    struct token *t4 = new_rawint_token(3, NULL);
    add_token(list, t4);

    struct token *t5 = new_rawint_token(4, NULL);
    add_token(list, t5);

    remove_line(list, t1);

    ASSERT_TRUE(list->first == t4, "first token is moved forward");
    ASSERT_TRUE(list->last == t5, "last token is unchanged");
    ASSERT_TRUE(list->first->prev == NULL, "not token before first token");

    free_token_list(list);
    return NULL;
}

const char* test_remove_line_end_noeol(void) {
    struct token_list *list = init_token_list();

    struct token *t1 = new_rawint_token(1, NULL);
    add_token(list, t1);

    struct token *t2 = new_rawint_token(2, NULL);
    add_token(list, t2);

    struct token *t3 = new_rawint_token(3, NULL);
    add_token(list, t3);

    remove_line(list, t2);

    ASSERT_TRUE(list->first == t1, "first token is unchanged");
    ASSERT_TRUE(list->last == t1, "last token is first token");
    ASSERT_TRUE(list->first->next == NULL, "no token after first token");

    free_token_list(list);
    return NULL;
}

const char* test_remove_line_end_w_eol(void) {
    struct token_list *list = init_token_list();

    struct token *t1 = new_rawint_token(1, NULL);
    add_token(list, t1);

    struct token *t2 = new_rawint_token(2, NULL);
    add_token(list, t2);

    struct token *t3 = new_rawint_token(3, NULL);
    add_token(list, t3);

    struct token *t4 = new_token(tt_eol, NULL, NULL);
    add_token(list, t4);

    remove_line(list, t2);

    ASSERT_TRUE(list->first == t1, "first token is unchanged");
    ASSERT_TRUE(list->last == t1, "last token is first token");
    ASSERT_TRUE(list->first->next == NULL, "no token after first token");

    free_token_list(list);
    return NULL;
}

const char* test_remove_line_all_tokens(void) {
    struct token_list *list = init_token_list();

    struct token *t1 = new_rawint_token(1, NULL);
    add_token(list, t1);

    struct token *t2 = new_rawint_token(2, NULL);
    add_token(list, t2);

    struct token *t3 = new_rawint_token(3, NULL);
    add_token(list, t3);

    remove_line(list, t1);

    ASSERT_TRUE(list->first == NULL, "no first token");
    ASSERT_TRUE(list->last == NULL, "no last token");

    free_token_list(list);
    return NULL;
}
