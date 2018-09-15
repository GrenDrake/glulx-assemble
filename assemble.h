#ifndef ASSEMBLE_H
#define ASSEMBLE_H

#include <stdio.h>
#include "utility.h"

#define HEADER_SIZE     40
#define MAX_OPERANDS    12

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

enum token_type {
    tt_bad,
    tt_identifier,
    tt_integer,
    tt_float,
    tt_string,
    tt_colon,
    tt_local,
    tt_indirect,
    tt_eol
};

enum operand_type {
    ot_constant,
    ot_indirect,
    ot_local,
    ot_stack,
    ot_afterram
};

/* Stores the location in the original source file that a particular structure
 * originated from.
 */
struct origin {
    char *filename;     // name of the file item originated on
    int line, column;   // line and column item originated on
    int dynamic;        // item was dynamically generated; no origin file exists
};

struct local_list {
    char *name;
    struct local_list *next;
};

struct token {
    struct origin origin;

    enum token_type type;
    char *text;
    int i;

    struct token *prev;
    struct token *next;
};

struct lexer_state {
    struct origin origin;

    size_t text_pos;
    size_t text_length;
    char *text;
};

struct operand {
    enum operand_type type;
    int value;
    int known_value;
    int force_4byte;
    char *name;
    struct operand *next;
};

struct token_list {
    struct token *first;
    struct token *last;
};

struct backpatch {
    char *name;
    int position;
    int position_after;
    int value_final;
    int max_width;
    struct backpatch *next;
};

struct label_def {
    char *name;
    int pos;
    struct label_def *next;
};


struct mnemonic {
    const char *name;
    int opcode;
    int operand_count;
    int last_operand_is_relative;
};

struct program_info {
    const char *output_file;
    int stack_size;
    int in_header;

    int ram_start;
    int extended_memory;
    int end_memory;

    struct label_def *first_label;
    struct backpatch *patch_list;
};

struct output_state {
    struct program_info *info;
    int in_header;

    int code_position;
    const char *current_function;
    struct local_list *local_names;
    int local_count;

    FILE *out;
    FILE *debug_out;
};

void copy_origin(struct origin *dest, struct origin *src);
void copy_origin(struct origin *dest, struct origin *src);
void free_origin(struct origin *origin);

struct token* new_token(enum token_type type, const char *text, struct lexer_state *state);
struct token* new_rawint_token(int value, struct lexer_state *state);
const char *token_name(struct token *t);
struct token_list* init_token_list(void);
void add_token(struct token_list *list, struct token *new_token);
void remove_token(struct token_list *list, struct token *token);
void free_token(struct token *token);
void free_token_list(struct token_list *list);
void merge_token_list(struct token_list *dest, struct token_list *src, struct token *after);
void dump_token_list(FILE *dest, struct token_list *list);

struct token_list* lex_file(const char *filename);
struct token_list* lex_core(struct lexer_state *state);

int add_label(struct label_def **first_lbl, const char *name, int value);
struct label_def* get_label(struct label_def *first, const char *name);
void dump_labels(FILE *dest, struct label_def *first);
void free_labels(struct label_def *first);
void dump_patches(FILE *dest, struct program_info *info);
void free_patches(struct backpatch *first);

void expect_eol(struct token **current);
struct token* remove_line(struct token_list *list, struct token *start);
void skip_line(struct token **current);
void report_error(struct origin *origin, const char *err_text, ...);
int token_check_identifier(struct token *token, const char *text);

int parse_preprocess(struct token_list *tokens, struct program_info *info);
int parse_tokens(struct token_list *list, struct program_info *info);

extern struct mnemonic codes[];

#endif
