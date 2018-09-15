#include <stdio.h>
#include <string.h>

#include "test.h"
#include "../assemble.h"


const char* test_new_token_general(void);
const char* test_new_token_source_location(void);

const char* test_new_rawint_token(void);
const char* test_new_token_int_positive(void);
const char* test_new_token_int_negative(void);
const char* test_new_token_float_positive(void);
const char* test_new_token_float_negative(void);

const char* test_basic_token_list(void);
const char* test_token_list_remove_first(void);
const char* test_token_list_remove_middle(void);
const char* test_token_list_remove_last(void);

const char* test_token_list_merge_same_list(void);
const char* test_token_list_merge_second_empty(void);
const char* test_token_list_merge_first(void);
const char* test_token_list_merge_middle(void);
const char* test_token_list_merge_last(void);



const char *test_suite_name = "tokens.c";
struct test_def test_list[] = {
    {   "new_token_general",                        test_new_token_general },
    {   "new_token_source_location",                test_new_token_source_location },

    {   "new_rawint_token",                         test_new_rawint_token },
    {   "new_token_int_positive",                   test_new_token_int_positive },
    {   "new_token_int_negative",                   test_new_token_int_negative },
    {   "new_token_float_positive",                 test_new_token_float_positive },
    {   "new_token_float_negative",                 test_new_token_float_negative },

    {   "basic_token_list",                         test_basic_token_list },
    {   "token_list_remove_first",                  test_token_list_remove_first },
    {   "token_list_remove_middle",                 test_token_list_remove_middle },
    {   "token_list_remove_last",                   test_token_list_remove_last },

    {   "token_list_merge_same_list",               test_token_list_merge_same_list },
    {   "token_list_merge_second_empty",            test_token_list_merge_second_empty },
    {   "token_list_merge_first",                   test_token_list_merge_first },
    {   "token_list_merge_middle",                  test_token_list_merge_middle },
    {   "token_list_merge_last",                    test_token_list_merge_last },

    {   NULL,                                       NULL }
};



const char* test_new_token_general(void) {
    const char *source_string = "source_string";
    struct token *token = new_token(tt_identifier, source_string, NULL);

    ASSERT_TRUE(token->type == tt_identifier, "token has correct type");
    ASSERT_TRUE(token->text != source_string, "token has own copy of text");
    ASSERT_TRUE(strcmp(token->text, source_string) == 0, "token string is correct");

    free_token(token);
    return NULL;
}

const char* test_new_token_source_location(void) {
    char *filename = str_dup("source");
    struct lexer_state state = { { FALSE, filename, 5, 2 } };
    const char *source_string = "source_string";
    struct token *token = new_token(tt_identifier, source_string, &state);

    ASSERT_TRUE(token->origin.line == 5, "token has correct line number");
    ASSERT_TRUE(token->origin.column == 2, "token has correct line number");
    ASSERT_TRUE(token->origin.filename != filename, "token has own copy of source filename");
    ASSERT_TRUE(strcmp(token->origin.filename, filename) == 0, "token filename is correct");

    free_token(token);
    return NULL;
}



const char* test_new_rawint_token(void) {
    char *filename = str_dup("source");
    struct lexer_state state = { { FALSE, filename, 5, 2 } };

    struct token *token = new_rawint_token(69, &state);
    ASSERT_TRUE(token->type == tt_integer, "token has correct type");
    ASSERT_TRUE(token->i == 69, "token has correct int value");
    ASSERT_TRUE(token->text == NULL, "token text is NULL");
    ASSERT_TRUE(token->origin.line == 5, "token has correct line number");
    ASSERT_TRUE(token->origin.column == 2, "token has correct line number");
    ASSERT_TRUE(token->origin.filename != filename, "token has own copy of source filename");
    ASSERT_TRUE(strcmp(token->origin.filename, filename) == 0, "token filename is correct");

    free_token(token);
    return NULL;
}

const char* test_new_token_int_positive(void) {
    const char *int_string = "42";
    struct token *token = new_token(tt_integer, int_string, NULL);

    ASSERT_TRUE(token->type == tt_integer, "token has correct type");
    ASSERT_TRUE(token->i == 42, "token has correct int value");
    ASSERT_TRUE(token->text != int_string, "token has own copy of text");
    ASSERT_TRUE(strcmp(token->text, int_string) == 0, "token string is correct");

    free_token(token);
    return NULL;
}

const char* test_new_token_int_negative(void) {
    const char *int_string = "-56";
    struct token *token = new_token(tt_integer, int_string, NULL);

    ASSERT_TRUE(token->type == tt_integer, "token has correct type");
    ASSERT_TRUE(token->i == -56, "token has correct int value");
    ASSERT_TRUE(token->text != int_string, "token has own copy of text");
    ASSERT_TRUE(strcmp(token->text, int_string) == 0, "token string is correct");

    free_token(token);
    return NULL;
}


const char* test_new_token_float_positive(void) {
    const char *float_string = "3.789";
    struct token *token = new_token(tt_float, float_string, NULL);

    ASSERT_TRUE(token->type == tt_integer, "token has correct type");
    ASSERT_TRUE(token->i == 0x40727efa, "token has correct floating point value");
    ASSERT_TRUE(token->text != float_string, "token has own copy of text");
    ASSERT_TRUE(strcmp(token->text, float_string) == 0, "token string is correct");

    free_token(token);
    return NULL;
}

const char* test_new_token_float_negative(void) {
    const char *float_string = "-8.135";
    struct token *token = new_token(tt_float, float_string, NULL);

    ASSERT_TRUE(token->type == tt_integer, "token has correct type");
    ASSERT_TRUE(token->i == 0xc10228f6, "token has correct floating point value");
    ASSERT_TRUE(token->text != float_string, "token has own copy of text");
    ASSERT_TRUE(strcmp(token->text, float_string) == 0, "token string is correct");

    free_token(token);
    return NULL;
}



const char* test_basic_token_list(void) {
    struct token *token = NULL, *first = NULL;
    struct token_list *list = init_token_list();

    ASSERT_TRUE(list, "token list was allocated");

    token = new_rawint_token(10, NULL);
    ASSERT_TRUE(list, "token was allocated");
    add_token(list, token);
    first = list->first;
    ASSERT_TRUE(list->first == token, "list first token correct");
    ASSERT_TRUE(list->last == token, "list last token is correct");

    token = new_rawint_token(20, NULL);
    ASSERT_TRUE(list, "token was allocated");
    add_token(list, token);
    ASSERT_TRUE(list->first == first, "list first token correct");
    ASSERT_TRUE(list->last == token, "list last token is correct");

    token = new_rawint_token(30, NULL);
    ASSERT_TRUE(list, "token was allocated");
    add_token(list, token);
    ASSERT_TRUE(list->first == first, "list first token correct");
    ASSERT_TRUE(list->last == token, "list last token is correct");

    first = list->first;
    ASSERT_TRUE(first->i == 10, "first token correct");
    first = first->next;
    ASSERT_TRUE(first->i == 20, "second token correct");
    first = first->next;
    ASSERT_TRUE(first->i == 30, "third token correct");

    free_token_list(list);
    return NULL;
}

const char* test_token_list_remove_first(void) {
    struct token_list *list = init_token_list();
    struct token *first = new_rawint_token(10, NULL);
    struct token *middle = new_rawint_token(10, NULL);
    struct token *last = new_rawint_token(10, NULL);
    add_token(list, first);
    add_token(list, middle);
    add_token(list, last);

    remove_token(list, first);
    ASSERT_TRUE(list->first == middle, "first token updated");
    ASSERT_TRUE(list->last == last, "last token unchanged");
    ASSERT_TRUE(middle->prev == NULL, "old middle has no prev token");
    ASSERT_TRUE(middle->next == last, "old middle next is unchanged");
    ASSERT_TRUE(last->prev == middle, "old last has no prev token");
    ASSERT_TRUE(last->next == NULL, "old last next is unchanged");

    free_token(first);
    free_token_list(list);
    return NULL;
}

const char* test_token_list_remove_middle(void) {
    struct token_list *list = init_token_list();
    struct token *first = new_rawint_token(10, NULL);
    struct token *middle = new_rawint_token(10, NULL);
    struct token *last = new_rawint_token(10, NULL);
    add_token(list, first);
    add_token(list, middle);
    add_token(list, last);

    remove_token(list, middle);
    ASSERT_TRUE(list->first == first, "first token unchanged");
    ASSERT_TRUE(list->last == last, "last token unchanged");
    ASSERT_TRUE(first->prev == NULL, "old first prev unchanged");
    ASSERT_TRUE(first->next == last, "old first next updated");
    ASSERT_TRUE(last->prev == first, "old last prev updated");
    ASSERT_TRUE(last->next == NULL, "old last next is unchanged");

    free_token(middle);
    free_token_list(list);
    return NULL;
}

const char* test_token_list_remove_last(void) {
    struct token_list *list = init_token_list();
    struct token *first = new_rawint_token(10, NULL);
    struct token *middle = new_rawint_token(10, NULL);
    struct token *last = new_rawint_token(10, NULL);
    add_token(list, first);
    add_token(list, middle);
    add_token(list, last);

    remove_token(list, last);
    ASSERT_TRUE(list->first == first, "first token unchaged");
    ASSERT_TRUE(list->last == middle, "last token updated");
    ASSERT_TRUE(first->prev == NULL, "old first prev unchanged");
    ASSERT_TRUE(first->next == middle, "old first next is unchanged");
    ASSERT_TRUE(middle->prev == first, "old middle prev unchanged");
    ASSERT_TRUE(middle->next == NULL, "old middle next is updated");

    free_token(last);
    free_token_list(list);
    return NULL;
}

const char* test_token_list_merge_same_list(void) {
    struct token_list *list_one = init_token_list();
    struct token *one_first = new_rawint_token(10, NULL);
    struct token *one_last = new_rawint_token(10, NULL);
    add_token(list_one, one_first);
    add_token(list_one, one_last);

    merge_token_list(list_one, list_one, NULL);
    ASSERT_TRUE(list_one->first == one_first, "list first unchanged");
    ASSERT_TRUE(list_one->last == one_last, "list last unchanged");

    return NULL;
}

const char* test_token_list_merge_second_empty(void) {
    struct token_list *list_one = init_token_list();
    struct token *one_first = new_rawint_token(10, NULL);
    struct token *one_last = new_rawint_token(10, NULL);
    add_token(list_one, one_first);
    add_token(list_one, one_last);

    struct token_list *list_two = init_token_list();
    merge_token_list(list_one, list_two, NULL);
    ASSERT_TRUE(list_one->first == one_first, "list first unchanged");
    ASSERT_TRUE(list_one->last == one_last, "list last unchanged");

    return NULL;
}

const char* test_token_list_merge_first(void) {
    struct token_list *list_one = init_token_list();
    struct token_list *list_two = init_token_list();

    struct token *one_first = new_rawint_token(10, NULL);
    struct token *one_last = new_rawint_token(10, NULL);
    add_token(list_one, one_first);
    add_token(list_one, one_last);

    struct token *two_first = new_rawint_token(10, NULL);
    struct token *two_last = new_rawint_token(10, NULL);
    add_token(list_two, two_first);
    add_token(list_two, two_last);

    merge_token_list(list_one, list_two, NULL);

    ASSERT_TRUE(list_one->first == two_first, "list start is updated");
    ASSERT_TRUE(list_one->last == one_last, "list end is correct");

    ASSERT_TRUE(two_first->prev == NULL, "first token prev correct");
    ASSERT_TRUE(two_first->next == two_last, "first token next correct");
    ASSERT_TRUE(two_last->prev == two_first, "second token prev correct");
    ASSERT_TRUE(two_last->next == one_first, "second token next correct");
    ASSERT_TRUE(one_first->prev == two_last, "third token prev correct");
    ASSERT_TRUE(one_first->next == one_last, "third token next correct");
    ASSERT_TRUE(one_last->prev == one_first, "last token prev correct");
    ASSERT_TRUE(one_last->next == NULL, "last token next correct");

    free_token_list(list_one);
    return NULL;
}

const char* test_token_list_merge_middle(void) {
    struct token_list *list_one = init_token_list();
    struct token_list *list_two = init_token_list();

    struct token *one_first = new_rawint_token(10, NULL);
    struct token *one_last = new_rawint_token(10, NULL);
    add_token(list_one, one_first);
    add_token(list_one, one_last);

    struct token *two_first = new_rawint_token(10, NULL);
    struct token *two_last = new_rawint_token(10, NULL);
    add_token(list_two, two_first);
    add_token(list_two, two_last);

    merge_token_list(list_one, list_two, one_first);

    ASSERT_TRUE(list_one->first == one_first, "list start is correct");
    ASSERT_TRUE(list_one->last == one_last, "list end is correct");

    ASSERT_TRUE(one_first->prev == NULL, "first token prev correct");
    ASSERT_TRUE(one_first->next == two_first, "first token next correct");
    ASSERT_TRUE(two_first->prev == one_first, "second token prev correct");
    ASSERT_TRUE(two_first->next == two_last, "second token next correct");
    ASSERT_TRUE(two_last->prev == two_first, "third token prev correct");
    ASSERT_TRUE(two_last->next == one_last, "third token next correct");
    ASSERT_TRUE(one_last->prev == two_last, "last token prev correct");
    ASSERT_TRUE(one_last->next == NULL, "last token next correct");

    free_token_list(list_one);
    return NULL;
}

const char* test_token_list_merge_last(void) {
    struct token_list *list_one = init_token_list();
    struct token_list *list_two = init_token_list();

    struct token *one_first = new_rawint_token(10, NULL);
    struct token *one_last = new_rawint_token(10, NULL);
    add_token(list_one, one_first);
    add_token(list_one, one_last);

    struct token *two_first = new_rawint_token(10, NULL);
    struct token *two_last = new_rawint_token(10, NULL);
    add_token(list_two, two_first);
    add_token(list_two, two_last);

    merge_token_list(list_one, list_two, one_last);

    ASSERT_TRUE(list_one->first == one_first, "list start is correct");
    ASSERT_TRUE(list_one->last == two_last, "list end is correct");

    ASSERT_TRUE(one_first->prev == NULL, "first token prev correct");
    ASSERT_TRUE(one_first->next == one_last, "first token next correct");
    ASSERT_TRUE(one_last->prev == one_first, "second token prev correct");
    ASSERT_TRUE(one_last->next == two_first, "second token next correct");
    ASSERT_TRUE(two_first->prev == one_last, "third token prev correct");
    ASSERT_TRUE(two_first->next == two_last, "third token next correct");
    ASSERT_TRUE(two_last->prev == two_first, "last token prev correct");
    ASSERT_TRUE(two_last->next == NULL, "last token next correct");

    free_token_list(list_one);
    return NULL;
}
