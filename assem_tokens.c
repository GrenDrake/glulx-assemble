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
};

static int is_identifier(int ch);
static void lexer_error(struct lexer_state *state, const char *err_text);
static int next_char(FILE *fp, struct lexer_state *state);
static struct token* new_token(enum token_type type, const char *text, struct lexer_state *state);
struct token* new_rawint_token(int value, struct lexer_state *state);


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

void free_tokens(struct token_list *list) {
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

void dump_tokens(struct token_list *list) {
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

static int next_char(FILE *fp, struct lexer_state *state) {
    int next = fgetc(fp);
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
    struct token_list *tokens = NULL;
    struct token *a_token;

    int has_errors = 0;
    FILE *fp = fopen(filename, "rt");
    if (fp) {
        char token_buf[TOKEN_BUF_LEN];
        int buf_pos = 0;
        struct lexer_state state = { filename, 1, 1 };
        tokens = init_token_list();

        int in = fgetc(fp);
        while (in != EOF) {
            if (in == '\n') {
                a_token = new_token(tt_eol, NULL, &state);
                add_token(tokens, a_token);
                in = next_char(fp, &state);
            } else if (in == ';') {
                while (!feof(fp) && in != '\n') {
                    in = next_char(fp, &state);
                }
            } else if (isspace(in)) {
                while (isspace(in)) {
                    in = next_char(fp, &state);
                }
            } else if (in == '#') {
                a_token = new_token(tt_local, NULL, &state);
                add_token(tokens, a_token);
                in = next_char(fp, &state);
            } else if (in == '*') {
                a_token = new_token(tt_indirect, NULL, &state);
                add_token(tokens, a_token);
                in = next_char(fp, &state);
            } else if (in == ':') {
                a_token = new_token(tt_colon, NULL, &state);
                add_token(tokens, a_token);
                in = next_char(fp, &state);
            } else if (in == '$') {
                struct lexer_state start = state;
                token_buf[0] = '$';
                buf_pos = 1;
                in = next_char(fp, &state);
                while (isxdigit(in)) {
                    token_buf[buf_pos] = in;
                    ++buf_pos;
                    in = next_char(fp, &state);
                }
                token_buf[buf_pos] = 0;
                a_token = new_token(tt_integer, token_buf, &start);
                add_token(tokens, a_token);
            } else if (isdigit(in)) {
                struct lexer_state start = state;
                int found_dot = FALSE, bad_dot = FALSE;
                buf_pos = 0;
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
                    in = next_char(fp, &state);
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
                    in = next_char(fp, &state);
                }
                token_buf[buf_pos] = 0;
                a_token = new_token(tt_identifier, token_buf, &start);
                add_token(tokens, a_token);
            } else if (in == '"') {
                struct lexer_state start = state;
                int prev = 0;
                in = next_char(fp, &state);
                buf_pos = 0;
                while ((in != '"' || prev == '\\') && in != EOF) {
                    token_buf[buf_pos] = in;
                    ++buf_pos;
                    prev = in;
                    in = next_char(fp, &state);
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
                    in = next_char(fp, &state);
                }
            } else if (in == '\'') {
                struct lexer_state start = state;
                int prev = 0;
                in = next_char(fp, &state);
                buf_pos = 0;
                while ((in != '\'' || prev == '\\') && in != EOF) {
                    token_buf[buf_pos] = in;
                    ++buf_pos;
                    prev = in;
                    in = next_char(fp, &state);
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
                    in = next_char(fp, &state);
                }
            } else {
                lexer_error(&state, "unexpected character");
                has_errors = 1;
                in = next_char(fp, &state);
            }
        }

        fclose(fp);
    } else {
        printf("could not open %s\n", filename);
    }

    if (has_errors) {
        free_tokens(tokens);
        return NULL;
    } else {
        return tokens;
    }
}
