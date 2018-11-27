#ifndef ASSEMBLE_H
#define ASSEMBLE_H

#include <stdio.h>
#include "utility.h"

#define MAX_TIMESTAMP_SIZE  13
#define HEADER_SIZE     64
#define MAX_OPERANDS    12
#define STRING_TABLE_BUCKETS    127

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

enum token_type {
    tt_bad,
    tt_identifier,
    tt_directive,
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

enum string_node_type {
    nt_branch   = 0,
    nt_end      = 1,
    nt_char     = 2,
    nt_unichar  = 4
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


struct string_node;
struct string_node_branch {
    struct string_node *left, *right;
};
struct string_node_char {
    unsigned c;
};
struct string_node {
    enum string_node_type type;
    int weight, position;
    struct string_node *prev, *next;
    union {
        struct string_node_branch branch;
        struct string_node_char a_char;
    } d;
};

struct string_table_entry {
    int c;
    int count;
    struct string_table_entry *next;
};
struct string_table {
    int input_bytes, output_bytes;
    struct string_table_entry *buckets[STRING_TABLE_BUCKETS];
    struct string_node *first;
    struct string_node *root;
};


struct program_info {
    const char *output_file;
    int stack_size;
    char timestamp[MAX_TIMESTAMP_SIZE];

    int ram_start;
    int extended_memory;
    int end_memory;
    int string_table;

    struct string_table strings;

    struct label_def *first_label;
    struct backpatch *patch_list;

    FILE *debug_out;
};

struct vbuffer;
struct output_state {
    struct program_info *info;
    int in_header;

    int code_position;
    const char *current_function;
    struct local_list *local_names;
    int local_count;

    FILE *out;
    struct vbuffer *output;
};

void copy_origin(struct origin *dest, struct origin *src);
void copy_origin(struct origin *dest, struct origin *src);
void free_origin(struct origin *origin);

void free_string_table(struct string_table *table);
void string_table_add(struct string_table *table, unsigned c);
void string_add_to_frequencies(struct string_table *table, const char *string);
int node_list_size(struct string_node *node);
int node_size(struct string_node *node);
void string_build_tree(struct string_table *table);
int encode_string(FILE *out, struct string_table *table, const char *text);
void dump_string_frequencies(FILE *dest, struct string_table *table);

struct token* new_token(enum token_type type, const char *text, struct lexer_state *state);
struct token* new_rawint_token(int value, struct lexer_state *state);
const char *token_name(struct token *t);
const char *token_type_name(enum token_type type);
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

int expect_eol(struct token **current);
int expect_type(struct token *current, enum token_type type);
struct token* remove_line(struct token_list *list, struct token *start);
void skip_line(struct token **current);
void report_error(struct origin *origin, const char *err_text, ...);
int matches_text(struct token *token, enum token_type type, const char *text);

int parse_preprocess(struct token_list *tokens, struct program_info *info);
int parse_tokens(struct token_list *list, struct program_info *info);

extern struct mnemonic codes[];

#endif
