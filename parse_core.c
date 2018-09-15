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
        report_error(&here->origin, "expected EOL (ignoring excess tokens)");
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

void report_error(struct origin *origin, const char *err_text, ...) {
    char msg_buf[MAX_ERROR_LENGTH];

    if (origin) {
        fputs(origin->filename, stderr);
        if (origin->line >= 0) {
            fprintf(stderr, ":%d:%d", origin->line, origin->column);
        }
        fputc(' ', stderr);
    }

    va_list args;
    va_start(args, err_text);
    vfprintf(stderr, err_text, args);
    va_end(args);
}

int token_check_identifier(struct token *token, const char *text) {
    if (!token || token->type != tt_identifier || strcmp(token->text, text) != 0) {
        return FALSE;
    }
    return TRUE;
}
