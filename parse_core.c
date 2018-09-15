#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"


void expect_eol(struct token **current) {
    struct token *here = *current;

    if (here == NULL || here->next == NULL) {
        return;
    }

    here = here->next;
    if (here->type != tt_eol) {
        parse_error(here, "expected EOL (ignoring excess tokens)");
        while (here && here->type != tt_eol) {
            here = here->next;
        }
    }

    if (here) {
        *current = here->next;
    } else {
        *current = NULL;
    }
}

/*
    Removes all tokens from the list beginning at *start* and ending at the
    next EOL token (or the end of the token list if not EOL token is found).
    Returns the next token after the removed EOL token, or NULL if there are
    no more tokens in the list.
*/
struct token* remove_line(struct token_list *list, struct token *start) {
    struct token *next = NULL, *current = start;

    while (current && current->type != tt_eol) {
        next = current->next;
        remove_token(list, current);
        free_token(current);
        current = next;
    }


    if (current == NULL) {
        if (list->first == start) {
            // we've removed all the tokens
            list->first = list->last = NULL;
        }
        return NULL;
    }


    next = current->next;
    if (list->first == start) {
        list->first = next;
    }
    remove_token(list, current);
    free_token(current);
    return next;
}

void skip_line(struct token **current) {
    struct token *here = *current;

    while (here && here->type != tt_eol) {
        here = here->next;
    }

    if (here) {
        *current = here->next;
    } else {
        *current = NULL;
    }
}

void parse_error(struct token *where, const char *err_msg, ...) {
    char msg_buf[MAX_ERROR_LENGTH];

    va_list args;
    va_start(args, err_msg);
    vsnprintf(msg_buf, MAX_ERROR_LENGTH, err_msg, args);

    printf("%s:%d:%d %s\n",
           where->origin.filename,
           where->origin.line,
           where->origin.column,
           msg_buf);
    va_end(args);
}

int token_check_identifier(struct token *token, const char *text) {
    if (!token || token->type != tt_identifier || strcmp(token->text, text) != 0) {
        return FALSE;
    }
    return TRUE;
}
