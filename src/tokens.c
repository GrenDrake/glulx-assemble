#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"

/* ************************************************************************* *
 * Manipulate origins                                                        *
 * ************************************************************************* */
void no_origin(struct origin *dest) {
    dest->line = 0;
    dest->column = 0;
    dest->filename = str_dup("no source");
    dest->dynamic = TRUE;
}

void copy_origin(struct origin *dest, struct origin *src) {
    dest->line = src->line;
    dest->column = src->column;
    dest->filename = str_dup(src->filename);
    dest->dynamic = FALSE;
}

void free_origin(struct origin *origin) {
    free(origin->filename);
}


/* ************************************************************************* *
 * Token Manipulation                                                        *
 * ************************************************************************* */

struct token* new_token(enum token_type type, const char *text, struct lexer_state *state) {
    struct token *current = malloc(sizeof(struct token));
    if (!current) return NULL;

    if (state)  copy_origin(&current->origin, &state->origin);
    else        no_origin(&current->origin);
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

    if (state)  copy_origin(&current->origin, &state->origin);
    else        no_origin(&current->origin);
    current->next = NULL;
    current->type = tt_integer;
    current->text = NULL;
    current->i = value;

    return current;
}

const char *token_name(struct token *t) {
    if (t == NULL) return "(null)";
    return token_type_name(t->type);
}

const char *token_type_name(enum token_type type) {
    switch(type) {
        case tt_bad:            return "bad token";
        case tt_colon:          return "colon";
        case tt_eol:            return "EOL";
        case tt_identifier:     return "identifier";
        case tt_directive:      return "directive";
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
        new_token->prev = NULL;
        list->first = new_token;
        list->last = new_token;
    } else {
        new_token->prev = list->last;
        list->last->next = new_token;
        list->last = new_token;
    }
}

void remove_token(struct token_list *list, struct token *token) {
    if (list->first == token) {
        list->first = token->next;
    }
    if (list->last == token) {
        list->last = token->prev;
    }

    if (token->prev) {
        token->prev->next = token->next;
    }
    if (token->next) {
        token->next->prev = token->prev;
    }
}

void free_token(struct token *token) {
    free(token->text);
    free_origin(&token->origin);
    free(token);
}

void free_token_list(struct token_list *list) {
    if (list == NULL || list->first == NULL) return;

    struct token *current = list->first;

    while (current) {
        struct token *next = current->next;
        free_token(current);
        current = next;
    }

    free(list);
}

void merge_token_list(struct token_list *dest, struct token_list *src, struct token *after) {
    // don't try to merge a list with itself
    if (dest == src) return;
    // don't try to merge an empty list
    if (src->first == NULL) {
        free_token_list(src);
        return;
    }
    // if dest list is empty, just transfer list over
    if (dest->first == NULL) {
        dest->first = src->first;
        dest->last = src->last;
        src->first = src->last = NULL;
        free_token_list(src);
    }
    // only otherwise do an actual merge
    if (after == NULL) {
        src->last->next = dest->first;
        dest->first->prev = src->last;
        dest->first = src->first;
    } else {
        struct token *old_next = after->next;
        after->next = src->first;
        src->first->prev = after;

        if (old_next) {
            src->last->next = old_next;
            old_next->prev = src->last;
        } else {
            dest->last = src->last;
        }
    }

    src->first = src->last = NULL;
    free_token_list(src);
}

void dump_token_list(FILE *dest, struct token_list *list) {
    if (list == NULL || list->first == NULL) return;

    struct token *current = list->first;

    while (current) {
        fprintf(dest, "%s:%d:%d  :  %s ",
               current->origin.filename,
               current->origin.line,
               current->origin.column,
               token_name(current));
        if (current->text == NULL) {
            fprintf(dest, "(null)");
        } else {
            fputc('~', dest);
            dump_string(dest, current->text, 2000000000);
            fputc('~', dest);
        }
        if (current->type == tt_integer) {
            fprintf(dest, "  i:%d", current->i);
        }
        fprintf(dest, "\n");

        current = current->next;
    }
}
