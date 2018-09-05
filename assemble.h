#ifndef ASSEMBLE_H
#define ASSEMBLE_H

#include <stdio.h>

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

struct local_list {
    char *name;
    struct local_list *next;
};

struct token {
    const char *source_file;
    int line, column;

    enum token_type type;
    char *text;
    int i;

    struct token *next;
};

struct lexer_state {
    const char *file;
    int line;
    int column;

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

struct output_state {
    int stack_size;
    int in_header;

    int code_position;
    int ram_start;
    int extended_memory;
    int end_memory;
    int string_table;
    const char *current_function;
    int local_count;
    FILE *out;
    FILE *debug_out;
    struct local_list *local_names;
    struct label_def *first_label;
    struct backpatch *patch_list;
};


char *str_dup(const char *source);
int cleanup_string(char *text);
void dump_string(FILE *dest, const char *text, unsigned max_length);

const char *token_name(struct token *t);
struct token_list* init_token_list(void);
void add_token(struct token_list *list, struct token *new_token);
void free_token_list(struct token_list *list);
void dump_token_list(struct token_list *list);

struct token_list* lex_file(const char *filename);
struct token_list* lex_core(struct lexer_state *state);

int add_label(struct label_def **first_lbl, const char *name, int value);
struct label_def* get_label(struct label_def *first, const char *name);
void dump_labels(FILE *dest, struct label_def *first);
void free_labels(struct label_def *first);

int parse_tokens(struct token_list *list, const char *output_filename);

extern struct mnemonic codes[];

#endif
