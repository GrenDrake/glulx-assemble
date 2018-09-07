#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"


void expect_eol(struct token **current);
void skip_line(struct token **current);
void parse_error(struct token *where, const char *err_msg, ...);
int token_check_identifier(struct token *token, const char *text);


int token_check_identifier(struct token *token, const char *text) {
    if (!token || token->type != tt_identifier || strcmp(token->text, text) != 0) {
        return FALSE;
    }
    return TRUE;
}


int parse_preprocess(struct token_list *tokens) {
    int found_errors = FALSE;
    struct token *here = tokens->first;

    while (here) {

        // skip labels
        if (here->type == tt_identifier && here->next && here->next->type == tt_colon) {
            here = here->next->next;
            continue;
        }

        // included files
        if (token_check_identifier(here, ".include")) {
            struct token *before = here->prev;
            struct token *start = here;
            struct token_list *new_tokens = NULL;

            if (!here->next) {
                parse_error(here, "Unexpected end of tokens");
                return FALSE;
            }
            here = here->next;

            if (here->type != tt_string) {
                parse_error(here, "Expected string");
                skip_line(&here);
                found_errors = TRUE;
                continue;
            }

            if (here->next && here->next->type != tt_eol) {
                parse_error(here, "Expected EOL");
                skip_line(&here);
                found_errors = TRUE;
                continue;
            }

            new_tokens = lex_file(here->text);

            struct token *start_next = start->next;
            remove_token(tokens, start_next);
            free_token(start_next);
            remove_token(tokens, start);
            free_token(start);

            if (new_tokens == NULL) {
                skip_line(&here);
                found_errors = TRUE;
                continue;
            }
            merge_token_list(tokens, new_tokens, before);
            continue;
        }

        // otherwise nothing for us on this line, skip to the next
        skip_line(&here);

    }

    return !found_errors;
}


