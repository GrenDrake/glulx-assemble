#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"

#define TOKEN_BUF_LEN 2048

static int next_char(struct lexer_state *state);
static char* lexer_read_string(int quote_char, struct lexer_state *state);
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
        lexer_error(&start, "unterminated string");
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
            char *text = lexer_read_string(in, state);
            in = next_char(state);
            if (text == NULL) {
                has_errors = TRUE;
                return NULL;
            } else {
                if (cleanup_string(text)) {
                    lexer_error(&start, "bad string escape");
                    has_errors = 1;
                }
                a_token = new_token(tt_string, text, &start);
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
                lexer_error(&start, "empty character literal");
                has_errors = TRUE;
            } else {
                if (cleanup_string(text)) {
                    lexer_error(&start, "bad string escape");
                    has_errors = 1;
                }
                int text_pos = 0;
                int cp = utf8_next_char(text, &text_pos);
                if (text[text_pos] != 0) {
                    lexer_error(&start, "character literal too long");
                    has_errors = 1;
                }
                free(text);
                a_token = new_rawint_token(cp, &start);
                add_token(tokens, a_token);
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
