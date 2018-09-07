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
           where->source_file,
           where->line,
           where->column,
           msg_buf);
    va_end(args);
}

int token_check_identifier(struct token *token, const char *text) {
    if (!token || token->type != tt_identifier || strcmp(token->text, text) != 0) {
        return FALSE;
    }
    return TRUE;
}
