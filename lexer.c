#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"

#define TOKEN_BUF_LEN 512

static int next_char(struct lexer_state *state);
static int is_identifier(int ch);
static void lexer_error(struct lexer_state *state, const char *err_text);

/* ************************************************************************* *
 * Core Lexer                                                                *
 * ************************************************************************* */

static int next_char(struct lexer_state *state) {
    if (state->text_pos >= state->text_length) {
        return 0;
    }

    int next = state->text[state->text_pos];
    ++state->text_pos;
    if (next == '\n') {
        ++state->line;
        state->column = 0;
    } else {
        ++state->column;
    }
    return next;
}

static void lexer_error(struct lexer_state *state, const char *err_text) {
    printf("%s:%d:%d %s\n", state->file, state->line, state->column, err_text);
}

static int is_identifier(int ch) {
    return isalnum(ch) || ch == '.' || ch == '_';
}

struct token_list* lex_file(const char *filename) {
    struct lexer_state state = { filename, 1, 1 };
    FILE *source_file = fopen(filename, "rt");
    if (!source_file) {
        fprintf(stderr, "Could not open source file ~%s~.\n", filename);
        return NULL;
    }
    fseek(source_file, 0, SEEK_END);
    state.text_length = ftell(source_file);
    fseek(source_file, 0, SEEK_SET);
    state.text = malloc(state.text_length + 1);
    if (!state.text) {
        fprintf(stderr, "Could not allocate memory for source file ~%s~.\n", filename);
        return NULL;
    }
    fread(state.text, state.text_length, 1, source_file);
    state.text[state.text_length] = 0;
    fclose(source_file);

    struct token_list *result = lex_core(&state);
    free(state.text);
    return result;
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
        if (in == '\n') {
            a_token = new_token(tt_eol, NULL, state);
            add_token(tokens, a_token);
            in = next_char(state);
        } else if (in == '\\') {
            in = next_char(state);
            if (in != '\n') {
                lexer_error(state, "unexpected character; \\ only permitted at end of line");
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
        } else if (in == '#') {
            a_token = new_token(tt_local, NULL, state);
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
                    lexer_error(&start, "expected numeric value after '-'");
                    has_errors = 1;
                }
            }
            while (isdigit(in) || in == '.') {
                if (in == '.') {
                    if (found_dot) {
                        bad_dot = TRUE;
                        lexer_error(&start, "malformed floating point number");
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
        } else if (is_identifier(in)) {
            struct lexer_state start = *state;
            buf_pos = 0;
            while (is_identifier(in)) {
                token_buf[buf_pos] = in;
                ++buf_pos;
                in = next_char(state);
            }
            token_buf[buf_pos] = 0;
            a_token = new_token(tt_identifier, token_buf, &start);
            add_token(tokens, a_token);
        } else if (in == '"') {
            struct lexer_state start = *state;
            int prev = 0;
            in = next_char(state);
            buf_pos = 0;
            while ((in != '"' || prev == '\\') && in != EOF) {
                token_buf[buf_pos] = in;
                ++buf_pos;
                prev = in;
                in = next_char(state);
            }
            if (in == EOF) {
                lexer_error(&start, "unterminated string");
            } else {
                token_buf[buf_pos] = 0;
                if (cleanup_string(token_buf)) {
                    lexer_error(&start, "bad string escape");
                    fprintf(stderr, "%s\n", token_buf);
                    has_errors = 1;
                }
                a_token = new_token(tt_string, token_buf, &start);
                add_token(tokens, a_token);
                in = next_char(state);
            }
        } else if (in == '\'') {
            struct lexer_state start = *state;
            int prev = 0;
            in = next_char(state);
            buf_pos = 0;
            while ((in != '\'' || prev == '\\') && in != EOF) {
                token_buf[buf_pos] = in;
                ++buf_pos;
                prev = in;
                in = next_char(state);
            }
            if (in == EOF) {
                lexer_error(&start, "unterminated string");
            } else if (buf_pos == 0) {
                lexer_error(state, "empty character literal");
                has_errors = 1;
            } else {
                token_buf[buf_pos] = 0;
                if (cleanup_string(token_buf)) {
                    lexer_error(&start, "bad string escape");
                    has_errors = 1;
                }
                if (strlen(token_buf) > 1) {
                    lexer_error(&start, "character literal too long");
                    has_errors = 1;
                }
                a_token = new_rawint_token(token_buf[0], &start);
                add_token(tokens, a_token);
                in = next_char(state);
            }
        } else {
            lexer_error(state, "unexpected character");
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
