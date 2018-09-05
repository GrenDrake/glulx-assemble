#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"

#define TOKEN_BUF_LEN 512

struct lexer_state {
    const char *file;
    int line;
    int column;

    size_t text_pos;
    size_t text_length;
    char *text;
};

static struct token* new_token(enum token_type type, const char *text, struct lexer_state *state);
struct token* new_rawint_token(int value, struct lexer_state *state);

static int next_char(struct lexer_state *state);
static int is_identifier(int ch);
static void lexer_error(struct lexer_state *state, const char *err_text);


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
        printf("%s:%d:%d  :  ",
               current->source_file,
               current->line,
               current->column);
        switch(current->type) {
            case tt_bad:            printf("bad token "); break;
            case tt_colon:          printf("colon "); break;
            case tt_eol:            printf("EOL "); break;
            case tt_identifier:     printf("identifier "); break;
            case tt_float:          printf("float (internal) "); break;
            case tt_indirect:       printf("indirect "); break;
            case tt_integer:        printf("integer "); break;
            case tt_local:          printf("local "); break;
            case tt_string:         printf("string "); break;
            default:
                printf("unknown tye %d", current->type);
        }
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
    struct token_list *tokens = NULL;
    struct token *a_token;
    int has_errors = 0;
    char token_buf[TOKEN_BUF_LEN];
    int buf_pos = 0;


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


    tokens = init_token_list();
    int in = next_char(&state);
    while (in != 0) {
        if (in == '\n') {
            a_token = new_token(tt_eol, NULL, &state);
            add_token(tokens, a_token);
            in = next_char(&state);
        } else if (in == ';') {
            while (in != 0 && in != '\n') {
                in = next_char(&state);
            }
        } else if (isspace(in)) {
            while (isspace(in)) {
                in = next_char(&state);
            }
        } else if (in == '#') {
            a_token = new_token(tt_local, NULL, &state);
            add_token(tokens, a_token);
            in = next_char(&state);
        } else if (in == '*') {
            a_token = new_token(tt_indirect, NULL, &state);
            add_token(tokens, a_token);
            in = next_char(&state);
        } else if (in == ':') {
            a_token = new_token(tt_colon, NULL, &state);
            add_token(tokens, a_token);
            in = next_char(&state);
        } else if (in == '$') {
            struct lexer_state start = state;
            token_buf[0] = '$';
            buf_pos = 1;
            in = next_char(&state);
            while (isxdigit(in)) {
                token_buf[buf_pos] = in;
                ++buf_pos;
                in = next_char(&state);
            }
            token_buf[buf_pos] = 0;
            a_token = new_token(tt_integer, token_buf, &start);
            add_token(tokens, a_token);
        } else if (isdigit(in) || in == '-') {
            struct lexer_state start = state;
            int found_dot = FALSE, bad_dot = FALSE;
            buf_pos = 0;
            if (in == '-') {
                token_buf[0] = '-';
                buf_pos = 1;
                in = next_char(&state);
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
                in = next_char(&state);
            }
            if (!bad_dot) {
                token_buf[buf_pos] = 0;
                a_token = new_token(found_dot ? tt_float : tt_integer, token_buf, &start);
                add_token(tokens, a_token);
            }
        } else if (is_identifier(in)) {
            struct lexer_state start = state;
            buf_pos = 0;
            while (is_identifier(in)) {
                token_buf[buf_pos] = in;
                ++buf_pos;
                in = next_char(&state);
            }
            token_buf[buf_pos] = 0;
            a_token = new_token(tt_identifier, token_buf, &start);
            add_token(tokens, a_token);
        } else if (in == '"') {
            struct lexer_state start = state;
            int prev = 0;
            in = next_char(&state);
            buf_pos = 0;
            while ((in != '"' || prev == '\\') && in != EOF) {
                token_buf[buf_pos] = in;
                ++buf_pos;
                prev = in;
                in = next_char(&state);
            }
            if (in == EOF) {
                lexer_error(&start, "unterminated string");
            } else {
                token_buf[buf_pos] = 0;
                if (cleanup_string(token_buf)) {
                    lexer_error(&state, "bad string escape");
                    fprintf(stderr, "%s\n", token_buf);
                    has_errors = 1;
                }
                a_token = new_token(tt_string, token_buf, &start);
                add_token(tokens, a_token);
                in = next_char(&state);
            }
        } else if (in == '\'') {
            struct lexer_state start = state;
            int prev = 0;
            in = next_char(&state);
            buf_pos = 0;
            while ((in != '\'' || prev == '\\') && in != EOF) {
                token_buf[buf_pos] = in;
                ++buf_pos;
                prev = in;
                in = next_char(&state);
            }
            if (in == EOF) {
                lexer_error(&start, "unterminated string");
            } else if (buf_pos == 0) {
                lexer_error(&state, "empty character literal");
                has_errors = 1;
            } else {
                token_buf[buf_pos] = 0;
                if (cleanup_string(token_buf)) {
                    lexer_error(&state, "bad string escape");
                    has_errors = 1;
                }
                if (strlen(token_buf) > 1) {
                    lexer_error(&state, "character literal contains too long");
                    has_errors = 1;
                }
                a_token = new_rawint_token(token_buf[0], &start);
                add_token(tokens, a_token);
                in = next_char(&state);
            }
        } else {
            lexer_error(&state, "unexpected character");
            has_errors = 1;
            in = next_char(&state);
        }
    }

    if (has_errors) {
        free_token_list(tokens);
        return NULL;
    } else {
        return tokens;
    }
}
