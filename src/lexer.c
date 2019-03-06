#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"
#include "vbuffer.h"

#define TOKEN_BUF_LEN 2048

static int next_char(struct lexer_state *state);
static char* lexer_read_string(int quote_char, struct lexer_state *state);
static int is_identifier(int ch);

/* ************************************************************************* *
 * Core Lexer                                                                *
 * ************************************************************************* */

static int next_char(struct lexer_state *state) {
    if (state->text_pos >= state->text_length) {
        return 0;
    }

    int old_here = state->text[state->text_pos];
    ++state->text_pos;

    if (old_here == '\n') {
        ++state->origin.line;
        state->origin.column = 0;
    } else {
        ++state->origin.column;
    }
    return old_here;
}

static char* lexer_read_string(int quote_char, struct lexer_state *state) {
    struct lexer_state start = *state;
    int prev = 0, in;
    size_t string_start, string_end, string_size;
    char *string_start_ptr;

    string_start_ptr = &state->text[state->text_pos];
    string_start = state->text_pos;
    in = next_char(state);
    while (in != 0 && (in != quote_char || prev == '\\')) {
        if (prev == '\\')   prev = 0;
        else                prev = in;
        in = next_char(state);
    }
    string_end = state->text_pos;
    string_size = string_end - string_start - 1;

    if (in == 0) {
        report_error(&start.origin, "unterminated string");
        return NULL;
    } else {
        char *string_text = malloc(string_size + 1);
        if (string_size > 0) {
            strncpy(string_text, string_start_ptr, string_size);
            string_text[string_size] = 0;
        }
        return string_text;
    }
}

static int is_identifier(int ch) {
    return isalnum(ch) || ch == '_';
}

struct token_list* lex_file(const char *filename) {
    struct lexer_state state = { { NULL, 1, 1 } };
    struct vbuffer *buffer = vbuffer_new();
    int from_stdin = FALSE;
    if (strcmp(filename, "-") == 0) {
        state.origin.filename = str_dup("(stdin)");
        from_stdin =  TRUE;
    } else {
        state.origin.filename = str_dup(filename);
    }

    int result = vbuffer_readfile(buffer, from_stdin ? NULL : filename);
    if (!result) {
        report_error(NULL, "Could not open source file ~%s~.\n", filename);
        vbuffer_free(buffer);
        return NULL;
    }
    vbuffer_pushchar(buffer, '\0');

    state.text = buffer->data;
    state.text_length = buffer->length;
    struct token_list *tokens = lex_core(&state);
    free_origin(&state.origin);
    vbuffer_free(buffer);
    return tokens;
}

struct token_list* lex_core(struct lexer_state *state) {
    struct token_list *tokens = NULL;
    struct token *a_token;
    int has_errors = 0;
    char token_buf[TOKEN_BUF_LEN];
    int buf_pos = 0;

    tokens = init_token_list();
    int in = next_char(state);
    while (in != 0) {
        if (in == '\n' || in == '\r') {
            a_token = new_token(tt_eol, NULL, state);
            add_token(tokens, a_token);
            do {
                in = next_char(state);
            } while (in == '\n' || in == '\r');
        } else if (in == '\\') {
            in = next_char(state);
            if (in != '\n' && in != '\r') {
                report_error(&state->origin, "unexpected character; \\ only permitted at end of line");
            } else {
                in = next_char(state);
            }
        } else if (in == ';') {
            while (in != 0 && in != '\n') {
                in = next_char(state);
            }
        } else if (isspace(in)) {
            while (isspace(in)) {
                in = next_char(state);
            }
        } else if (in == ',') {
            a_token = new_token(tt_comma, NULL, state);
            add_token(tokens, a_token);
            in = next_char(state);
        } else if (in == '+') {
            a_token = new_token(tt_operator, NULL, state);
            a_token->i = op_add;
            add_token(tokens, a_token);
            in = next_char(state);
        } else if (in == '-') {
            a_token = new_token(tt_operator, NULL, state);
            a_token->i = op_subtract;
            add_token(tokens, a_token);
            in = next_char(state);
        } else if (in == '*') {
            a_token = new_token(tt_indirect, NULL, state);
            add_token(tokens, a_token);
            in = next_char(state);
        } else if (in == ':') {
            a_token = new_token(tt_colon, NULL, state);
            add_token(tokens, a_token);
            in = next_char(state);
        } else if (in == '$') {
            struct lexer_state start = *state;
            token_buf[0] = '$';
            buf_pos = 1;
            in = next_char(state);
            while (isxdigit(in)) {
                token_buf[buf_pos] = in;
                ++buf_pos;
                in = next_char(state);
            }
            token_buf[buf_pos] = 0;
            a_token = new_token(tt_integer, token_buf, &start);
            add_token(tokens, a_token);
        } else if (isdigit(in) || in == '-') {
            struct lexer_state start = *state;
            int found_dot = FALSE, bad_dot = FALSE;
            buf_pos = 0;
            if (in == '-') {
                token_buf[0] = '-';
                buf_pos = 1;
                in = next_char(state);
                if (!isdigit(in)) {
                    report_error(&start.origin, "expected numeric value after '-'");
                    has_errors = 1;
                }
            }
            while (isdigit(in) || in == '.') {
                if (in == '.') {
                    if (found_dot) {
                        bad_dot = TRUE;
                        report_error(&start.origin, "malformed floating point number");
                        has_errors = 1;
                    } else {
                        found_dot = TRUE;
                    }
                }
                token_buf[buf_pos] = in;
                ++buf_pos;
                in = next_char(state);
            }
            if (!bad_dot) {
                token_buf[buf_pos] = 0;
                a_token = new_token(found_dot ? tt_float : tt_integer, token_buf, &start);
                add_token(tokens, a_token);
            }
        } else if (is_identifier(in) || in == '.') {
            struct lexer_state start = *state;
            buf_pos = 0;
            do {
                token_buf[buf_pos] = in;
                ++buf_pos;
                in = next_char(state);
            } while (is_identifier(in));
            token_buf[buf_pos] = 0;
            if (token_buf[0] == '.' && token_buf[1] == 0) {
                report_error(&start.origin, "found zero length directive");
                has_errors = 1;
            }
            if (token_buf[0] == '.') {
                a_token = new_token(tt_directive, token_buf, &start);
            } else {
                a_token = new_token(tt_identifier, token_buf, &start);
            }
            add_token(tokens, a_token);
        } else if (in == '"') {
            struct lexer_state start = *state;
            char *text = lexer_read_string(in, state);
            in = next_char(state);
            if (text == NULL) {
                has_errors = TRUE;
                return NULL;
            } else {
                int bad_escape = cleanup_string(text);
                if (bad_escape) {
                    report_error(&start.origin, "string contains invalid escape code '\\%c'", text[bad_escape]);
                    has_errors = 1;
                }
                a_token = new_token(tt_string, text, &start);
                free(text);
                add_token(tokens, a_token);
            }
        } else if (in == '\'') {
            struct lexer_state start = *state;
            char *text = lexer_read_string(in, state);
            in = next_char(state);
            if (text == NULL) {
                has_errors = TRUE;
                return NULL;
            } else if (strlen(text) == 0) {
                report_error(&start.origin, "empty character literal");
                free(text);
                has_errors = TRUE;
            } else {
                int bad_escape = cleanup_string(text);
                if (bad_escape) {
                    report_error(&start.origin, "character literal contains invalid escape code '\\%c'", text[bad_escape]);
                    has_errors = 1;
                }
                int text_pos = 0;
                int cp = utf8_next_char(text, &text_pos);
                if (text[text_pos] != 0) {
                    report_error(&start.origin, "character literal too long");
                    has_errors = 1;
                }
                free(text);
                a_token = new_rawint_token(cp, &start);
                add_token(tokens, a_token);
            }
        } else {
            report_error(&state->origin, "unexpected character");
            has_errors = 1;
            in = next_char(state);
        }
    }

    if (has_errors) {
        free_token_list(tokens);
        return NULL;
    } else {
        add_token(tokens, new_token(tt_eol, NULL, state));
        return tokens;
    }
}
