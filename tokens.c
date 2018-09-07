#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"


/* ************************************************************************* *
 * Token Manipulation                                                        *
 * ************************************************************************* */

struct token* new_token(enum token_type type, const char *text, struct lexer_state *state) {
    struct token *current = malloc(sizeof(struct token));
    if (!current) return NULL;

    current->source_file = state->file;
    current->line = state->line;
    current->column = state->column;
    current->next = NULL;

    current->type = type;
    if (text) {
        current->text = str_dup(text);
    } else {
        current->text = NULL;
    }

    if (type == tt_integer) {
        char *endptr;
        if (text[0] == '$') {
            current->i = strtol(&text[1], &endptr, 16);
        } else {
            current->i = strtol(text, &endptr, 10);
        }
        if (*endptr != 0) {
            // bad integer value
            free(current);
            return NULL;
        }
    } else if (type == tt_float) {
        current->type = tt_integer;
        union {
            int i;
            float f;
        } punning;
        char *endptr;
        punning.f = strtof(text, &endptr);
        if (*endptr != 0) {
            // bad float value
            free(current);
            return NULL;
        } else {
            current->i = punning.i;
        }
    }

    return current;
}

struct token* new_rawint_token(int value, struct lexer_state *state) {
    struct token *current = malloc(sizeof(struct token));
    if (!current) return NULL;

    current->source_file = state->file;
    current->line = state->line;
    current->column = state->column;
    current->next = NULL;
    current->type = tt_integer;
    current->text = NULL;
    current->i = value;

    return current;
}

const char *token_name(struct token *t) {
    switch(t->type) {
        case tt_bad:            return "bad token";
        case tt_colon:          return "colon";
        case tt_eol:            return "EOL";
        case tt_identifier:     return "identifier";
        case tt_float:          return "float (internal)";
        case tt_indirect:       return "indirect";
        case tt_integer:        return "integer";
        case tt_local:          return "local";
        case tt_string:         return "string";
        default:                return "bad token type";
    }
}

/* ************************************************************************* *
 * Token List Manipulation                                                   *
 * ************************************************************************* */

struct token_list* init_token_list(void) {
    struct token_list *list = malloc(sizeof(struct token_list));
    list->first = NULL;
    list->last = NULL;
    return list;
}

void add_token(struct token_list *list, struct token *new_token) {
    if (list == NULL || new_token == NULL) return;

    new_token->next = NULL;
    if (list->first == NULL) {
        list->first = new_token;
        list->last = new_token;
    } else {
        list->last->next = new_token;
        list->last = new_token;
    }
}

void free_token_list(struct token_list *list) {
    if (list == NULL || list->first == NULL) return;

    struct token *current = list->first;

    while (current) {
        struct token *next = current->next;
        free(current->text);
        free(current);
        current = next;
    }

    free(list);
}

void dump_token_list(struct token_list *list) {
    if (list == NULL || list->first == NULL) return;

    struct token *current = list->first;

    while (current) {
        printf("%s:%d:%d  :  %s ",
               current->source_file,
               current->line,
               current->column,
               token_name(current));
        if (current->text == NULL) {
            printf("(null)");
        } else {
            putc('~', stdout);
            dump_string(stdout, current->text, 2000000000);
            putc('~', stdout);
        }
        if (current->type == tt_integer) {
            printf("  i:%d", current->i);
        }
        printf("\n");

        current = current->next;
    }
}
