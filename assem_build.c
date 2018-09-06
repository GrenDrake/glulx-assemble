#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"

#define MAX_ERROR_LENGTH 1024


static void expect_eol(struct token **current);
static void skip_line(struct token **current);
static void parse_error(struct token *where, const char *err_msg, ...);

static int data_string(struct token *first,
                        struct output_state *output,
                        int add_type_byte);
static int data_unicode(struct token *first,
                        struct output_state *output);
static int data_zeroes(struct token *first, struct output_state *output);
static int data_bytes(struct token *first, struct output_state *output, int width);
static int start_function(struct token *first, struct output_state *output);
static void end_function(struct output_state *output);

struct operand* parse_operand(struct token **from, struct output_state *output);
int operand_size(const struct operand *op);


/* ************************************************************************** *
 * BINARY OUTPUT FUNCTIONS                                                    *
 * ************************************************************************** */

void write_byte(FILE *out, uint8_t value) {
    fputc(  value        & 0xFF, out);
}

void write_short(FILE *out, uint16_t value) {
    fputc( (value >> 8)  & 0xFF, out);
    fputc(  value        & 0xFF, out);
}

void write_word(FILE *out, uint32_t value) {
    fputc( (value >> 24) & 0xFF, out);
    fputc( (value >> 16) & 0xFF, out);
    fputc( (value >> 8)  & 0xFF, out);
    fputc(  value        & 0xFF, out);
}


/* ************************************************************************** *
 * GENERAL PARSING AND ERROR FUNCTIONS                                        *
 * ************************************************************************** */

static void expect_eol(struct token **current) {
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

static void skip_line(struct token **current) {
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

static void parse_error(struct token *where, const char *err_msg, ...) {
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


/* ************************************************************************** *
 * DIRECTIVE PARSING                                                          *
 * ************************************************************************** */

static int data_string(struct token *first,
                        struct output_state *output,
                        int add_type_byte) {
    struct token *here = first->next;

    if (here->type != tt_string) {
        parse_error(here, "expected string");
        return 0;
    }

#ifdef DEBUG
    fprintf(output->debug_out, "0x%08X string ~", output->code_position);
    dump_string(output->debug_out, here->text, 32);
    fprintf(output->debug_out, "~\n");
#endif
    if (add_type_byte) {
        fputc(0xE0, output->out);
    }
    int pos = 0;
    while (here->text[pos] != 0) {
        fputc(here->text[pos], output->out);
        ++pos;
    }
    fputc(0, output->out);
    output->code_position += pos + 1;
    if (add_type_byte) {
        ++output->code_position;
    }

    expect_eol(&here);
    return 1;
}

static int data_unicode(struct token *first,
                        struct output_state *output) {
    struct token *here = first->next;

    if (here->type != tt_string) {
        parse_error(here, "expected string");
        return 0;
    }

#ifdef DEBUG
    fprintf(output->debug_out, "0x%08X unicode ~", output->code_position);
    dump_string(output->debug_out, here->text, 32);
    fprintf(output->debug_out, "~\n");
#endif
    fputc(0xE2, output->out);
    fputc(0, output->out);
    fputc(0, output->out);
    fputc(0, output->out);

    int pos = 0, length = 0;
    int codepoint = utf8_next_char(here->text, &pos);
    while (codepoint != 0) {
        write_word(output->out, codepoint);
        ++length;
        codepoint = utf8_next_char(here->text, &pos);
    }
    write_word(output->out, 0);
    output->code_position += length * 4 + 8;

    expect_eol(&here);
    return 1;
}

static int data_zeroes(struct token *first, struct output_state *output) {
    struct token *here = first->next;

    if (here->type != tt_integer) {
        parse_error(here, "expected integer");
        return 0;
    }

#ifdef DEBUG
    fprintf(output->debug_out, "0x%08X zeroes (%d)\n", output->code_position, here->i);
#endif
    for (int i = 0; i < here->i; ++i) {
        fputc(0, output->out);
    }
    output->code_position += here->i;
    expect_eol(&here);
    return 1;
}

static int data_bytes(struct token *first, struct output_state *output, int width) {
    struct token *here = first->next;
#ifdef DEBUG
    fprintf(output->debug_out, "0x%08X data(%d)", output->code_position, width);
#endif

    while (here && here->type != tt_eol) {
        if (here->type != tt_integer) {
            printf("\n");
            parse_error(here, "expected integer");
            return 0;
        }

#ifdef DEBUG
        fprintf(output->debug_out, " %d", here->i);
#endif
        fwrite(&here->i, width, 1, output->out);
        output->code_position += width;
        here = here->next;
    }
#ifdef DEBUG
    fprintf(output->debug_out, "\n");
#endif
    return 1;
}

static int start_function(struct token *first, struct output_state *output) {
    int stack_based = FALSE;
    struct token *here = first->next; // skip ".function"
    end_function(output);
#ifdef DEBUG
    int start_pos = output->code_position;
#endif

    if (here && here->type == tt_identifier && strcmp(here->text, "stk") == 0) {
        stack_based = TRUE;
        here = here->next;
    }

    int name_count = 0;
    if (here->type != tt_eol) {
        if (here->type == tt_integer) {
            name_count = here->i;
        } else {
            struct local_list *last = NULL;
            while (here && here->type != tt_eol) {
                if (here->type != tt_identifier) {
                    parse_error(here, "expected identifier");
                } else {
                    struct local_list *local = malloc(sizeof(struct local_list));
                    local->name = str_dup(here->text);
                    local->next = NULL;
                    if (last) {
                        last->next = local;
                    } else {
                        output->local_names = local;
                    }
                    last = local;
                    ++name_count;
                }
                here = here->next;
            }
        }
    }

    if (stack_based) {
        fputc(0xC0, output->out);
    } else {
        fputc(0xC1, output->out);
    }
    output->code_position += 3;

#ifdef DEBUG
    fprintf(output->debug_out,
            "\n0x%08X FUNCTION %s   %d LOCALS",
            start_pos,
            stack_based ? "(stk)" : "",
            name_count);
    if (output->local_names) {
        fputc(':', output->debug_out);
        struct local_list *cur = output->local_names;
        while (cur) {
            fprintf(output->debug_out,
                    " %s",
                    cur->name);
            cur = cur->next;
        }
    }
    fputc('\n', output->debug_out);
#endif

    while (name_count > 0) {
        if (name_count > 255) {
            write_byte(output->out, 4);
            write_byte(output->out, 255);
        } else {
            write_byte(output->out, 4);
            write_byte(output->out, name_count);
        }
        output->code_position += 2;
        name_count -= 255;
    }
    // write terminator for local count
    write_byte(output->out, 0);
    write_byte(output->out, 0);

    return 1;
}

static void end_function(struct output_state *output) {
    struct local_list *cur = output->local_names;
    while (cur) {
        struct local_list *next = cur->next;
        free(cur->name);
        free(cur);
        cur = next;
    }
    output->current_function = NULL;
    output->local_names = NULL;
    output->local_count = 0;
}


struct operand* parse_operand(struct token **from, struct output_state *output) {
    struct token *here = *from;
    struct operand *op = malloc(sizeof(struct operand));
    op->next = NULL;
    op->known_value = FALSE;
    op->force_4byte = FALSE;

    op->type = ot_constant;
    if (here->type == tt_indirect) {
        op->type = ot_indirect;
        here = here->next;
    } else if (here->type == tt_local) {
        op->type = ot_local;
        here = here->next;
    }

    if (here->type == tt_integer) {
        op->value = here->i;
        if (op->type == ot_local) {
            op->value *= 4;
        }
        op->known_value = TRUE;
    } else if (here->type == tt_identifier) {
        if (strcmp(here->text, "sp") == 0) {
            op->type = ot_stack;
            op->value = 0;
            op->known_value = TRUE;
        } else {
            struct label_def *label = get_label(output->first_label, here->text);
            if (label) {
                op->value = label->pos;
                op->known_value = TRUE;
            } else {
                struct local_list *local = output->local_names;
                int counter = 0;
                while (local) {
                    if (strcmp(local->name, here->text) == 0) {
                        op->type = ot_local;
                        op->value = counter * 4;
                        op->known_value = TRUE;
                        break;
                    }
                    local = local->next;
                    ++counter;
                }

                if (!op->known_value) {
                    op->name = here->text;
                    op->value = 0;
                }
            }
        }
    } else {
        parse_error(here, "unexpected token found");
        from = &here->next;
        free(op);
        return NULL;
    }

    *from = here->next;
    return op;
}

int operand_size(const struct operand *op) {
    if (!op->known_value || op->force_4byte) {
        return 3;
    }

    switch(op->type) {
        case ot_stack:
            return 0;
        case ot_constant:
            if (op->value == 0)                             return 0;
            if (op->value >= -128   && op->value <= 0x7F)   return 1;
            if (op->value >= -32768 && op->value <= 0x7F)   return 2;
            else                                            return 3;
        case ot_local:
        case ot_indirect:
        case ot_afterram:
            if (op->value <= 0xFF)      return 1;
            if (op->value <= 0xFFFF)    return 2;
            else                                          return 3;
    }
    return 4;
}


/* ************************************************************************** *
 * PARSE_TOKENS FUNCTION                                                      *
 * ************************************************************************** */
int parse_tokens(struct token_list *list, const char *output_filename) {
    struct output_state output = { 2048, TRUE };
    int has_errors = 0;

    FILE *out = fopen(output_filename, "wb+");
    output.out = out;
    if (!out) {
        printf("could not open output file\n");
        return 0;
    }
#ifdef DEBUG
    output.debug_out = fopen("out_debug.txt", "wt");
    if (!output.debug_out) {
        printf("could not open output file\n");
        return 0;
    }
#endif

    // write empty header
    for (int i = 0; i < HEADER_SIZE; ++i) {
        fputc(0, out);
        ++output.code_position;
    }

    struct token *here = list->first;
    while (here) {
        if (here->type == tt_eol) {
            here = here->next;
            continue;
        }

        if (here->type != tt_identifier) {
            parse_error(here, "expected identifier");
            has_errors += 1;
            skip_line(&here);
            continue;
        }

        if (here->next && here->next->type == tt_colon) {
            if (!add_label(&output.first_label, here->text, output.code_position)) {
                parse_error(here, "could not create label (already exists?)");
            }
            here = here->next->next;
            continue;
        }

/* ************************************************************************** *
 * DIRECTIVE PROCESSING                                                       *
 * ************************************************************************** */
        if (strcmp(here->text, ".cstring") == 0) {
            data_string(here, &output, FALSE);
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".string") == 0) {
            data_string(here, &output, TRUE);
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".unicode") == 0) {
            data_unicode(here, &output);
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".byte") == 0) {
            data_bytes(here, &output, 1);
            skip_line(&here);
            continue;
        }
        if (strcmp(here->text, ".short") == 0) {
            data_bytes(here, &output, 2);
            skip_line(&here);
            continue;
        }
        if (strcmp(here->text, ".word") == 0) {
            data_bytes(here, &output, 4);
            skip_line(&here);
            continue;
        }
        if (strcmp(here->text, ".zero") == 0) {
            data_zeroes(here, &output);
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".function") == 0) {
            start_function(here, &output);
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".end_header") == 0) {
            if (!output.in_header) {
                parse_error(here, "ended header when not in header");
                skip_line(&here);
                continue;
            }

            while (output.code_position % 256 != 0) {
                fputc(0, output.out);
                ++output.code_position;
            }
            output.in_header = FALSE;
            output.ram_start = output.code_position;
            add_label(&output.first_label, "_RAMSTART", output.ram_start);
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".extra_memory") == 0) {
            here = here->next;
            if (here->type != tt_integer) {
                parse_error(here, "expected integer");
                skip_line(&here);
                continue;
            }
            if (here->i % 256) {
                parse_error(here, "extra memory must be multiple of 256 (currently %d, next multiple %d)",
                            here->i,
                            (here->i / 256 + 1) * 256);
            }
            output.extended_memory = here->i;
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".stack_size") == 0) {
            here = here->next;
            if (here->type != tt_integer) {
                parse_error(here, "expected intger");
                skip_line(&here);
                continue;
            }
            if (here->i % 256) {
                parse_error(here, "stack size must be multiple of 256 (currently %d, next multiple %d)",
                            here->i,
                            (here->i / 256 + 1) * 256);
            }
            output.stack_size = here->i;
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".define") == 0) {
            here = here->next;
            if (here->type != tt_identifier) {
                parse_error(here, "expected identifier");
                skip_line(&here);
                continue;
            }
            const char *name = here->text;
            here = here->next;
            if (here->type != tt_integer) {
                printf("%d\n", here->type);
                parse_error(here, "expected integer");
                skip_line(&here);
                continue;
            }

            if (!add_label(&output.first_label, name, here->i)) {
                parse_error(here, "error creating constant");
                skip_line(&here);
                continue;
            }
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".include") == 0) {
            here = here->next;
            if (here->type != tt_string) {
                parse_error(here, "expected string");
                skip_line(&here);
                continue;
            }

            struct token_list *new_tokens = lex_file(here->text);
            if (new_tokens == NULL) {
                printf("Errors occured.\n");
                return 1;
            }

            new_tokens->last->next = here->next;
            here->next = new_tokens->first;
            free(new_tokens);

            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".include_binary") == 0) {
            here = here->next;
            if (here->type != tt_string) {
                parse_error(here, "expected string");
                skip_line(&here);
                continue;
            }

            FILE *bindata = fopen(here->text, "rt");
            if (!bindata) {
                parse_error(here, "Could not read binary file ~%s~.", here->text);
                skip_line(&here);
                continue;
            }
            fseek(bindata, 0, SEEK_END);
            size_t binsize = ftell(bindata);
            fseek(bindata, 0, SEEK_SET);
            char *buffer = malloc(binsize);
            fread(buffer, binsize, 1, bindata);
            fclose(bindata);
            fwrite(buffer, binsize, 1, out);
#ifdef DEBUG
            fprintf(output.debug_out, "0x%08X BINARY FILE ~%s~ (%ld bytes)\n",
                    output.code_position,
                    here->text,
                    binsize);
#endif
            output.code_position += binsize;

            skip_line(&here);
            continue;
        }

        if (here->text[0] == '.') {
            parse_error(here, "unknown directive %s", here->text);
            skip_line(&here);
            continue;
        }

/* ************************************************************************** *
 * MNEMONIC PROCESSING                                                        *
 * ************************************************************************** */
        struct mnemonic *m = codes;
        while (m->name && strcmp(m->name, here->text) != 0) {
            ++m;
        }
        if (m->name == NULL) {
            parse_error(here, "unknown mnemonic %s", here->text);
            skip_line(&here);
            continue;
        }

#ifdef DEBUG
        fprintf(output.debug_out, "0x%08X ~%s~ %d/0x%x   (at 0x%lx)  ",
                output.code_position,
                here->text,
                m->opcode,
                m->opcode,
                ftell(output.out));
#endif

        if (m->opcode <= 0x7F) {
            write_byte(output.out, m->opcode);
            output.code_position += 1;
        } else if (m->opcode <= 0x3FFF) {
            write_short(output.out, m->opcode | 0x8000);
            output.code_position += 2;
        } else {
            write_word(output.out, m->opcode | 0xC0000000);
            output.code_position += 4;
        }

        here = here->next;
        int operand_count = 0;
        struct operand *op_list = NULL, *op_end = NULL;
        while (here && here->type != tt_eol) {
            ++operand_count;
            struct operand *op = parse_operand(&here, &output);
            if (op == NULL) {
                parse_error(here, "failed to parse operand.");
                skip_line(&here);
                continue;
            } else {
                if (op_list) {
                    op_end->next = op;
                    op_end = op;
                } else {
                    op_list = op;
                    op_end = op;
                }
            }
        }

        if (operand_count != m->operand_count) {
            parse_error(here, "bad operand count for %s; expected %d, but found %d.",
                        m->name, m->operand_count, operand_count);
            skip_line(&here);
            continue;
        }

        int after_pos = 0;
        if (m->last_operand_is_relative) {
            // find the end of the current instruction
            after_pos = output.code_position;
            int type_count = 0;
            struct operand *op = op_list;
            while (op) {
                if (type_count) {
                    type_count = 0;
                    ++after_pos;
                } else {
                    type_count = 1;
                }
                if (op == op_end) {
                    op->force_4byte = TRUE;
                    after_pos += 4;
                } else {
                    after_pos += operand_size(op);
                }
                op = op->next;
            }
            if (type_count) ++after_pos;
            if (op_end->known_value) {
                op_end->value = op_end->value - after_pos + 2;
            }
        }

#ifdef DEBUG
        fprintf(output.debug_out, " types");
#endif
        // write operand types to file
        struct operand *cur_op = op_list;
        int type_byte = 0;
        int type_count = 0;
        while (cur_op) {
            int my_type = 0;

            switch(cur_op->type) {
                case ot_constant:
                    my_type = 0;
                    break;
                case ot_indirect:
                    my_type = 4;
                    break;
                case ot_afterram:
                    my_type = 12;
                    break;
                case ot_local:
                    my_type = 8;
                    break;
                case ot_stack:
                    my_type = 8;
                    break;
            }
            my_type += operand_size(cur_op);
            if (type_count) {
                type_count = 0;
                type_byte |= my_type << 4;
                fputc(type_byte, out);
                ++output.code_position;
#ifdef DEBUG
        fprintf(output.debug_out, " %X", type_byte);
#endif
            } else {
                type_count = 1;
                type_byte = my_type;
            }

            cur_op = cur_op->next;
        }
        if (type_count) {
            fputc(type_byte, out);
            ++output.code_position;
#ifdef DEBUG
            fprintf(output.debug_out, " %X", type_byte);
#endif
        }


        // write operands to file
        cur_op = op_list;
        while (cur_op) {
            if (!cur_op->known_value) {
                struct backpatch *patch = malloc(sizeof(struct backpatch));
                patch->next = 0;
                patch->name = str_dup(cur_op->name);
                patch->position = output.code_position;
                patch->position_after = after_pos;
                if (output.patch_list) {
                    patch->next = output.patch_list;
                }
                output.patch_list = patch;
            }

            switch(operand_size(cur_op)) {
                case 0:
                    break;
                case 1:
                    write_byte(out, cur_op->value);
                    output.code_position += 1;
                    break;
                case 2:
                    write_short(out, cur_op->value);
                    output.code_position += 2;
                    break;
                case 3:
                    write_word(out, cur_op->value);
                    output.code_position += 4;
                    break;
                default:
                    parse_error(here, "Bad operand size");
            }
#ifdef DEBUG
            if (cur_op->type == ot_stack) {
                fprintf(output.debug_out, " STACK");
            } else {
                switch(cur_op->type) {
                    case ot_constant:   fputs(" c:", output.debug_out);   break;
                    case ot_local:      fputs(" l:", output.debug_out);   break;
                    case ot_indirect:   fputs(" i:", output.debug_out);   break;
                    case ot_afterram:   fputs(" a:", output.debug_out);   break;
                    default:
                        fprintf(output.debug_out, " (bad operand type %d", cur_op->type);
                }
                if (cur_op->known_value) {
                    fprintf(output.debug_out, "%d", cur_op->value);
                } else {
                    fprintf(output.debug_out, "???");
                }
            }
#endif



            cur_op = cur_op->next;
        }

#ifdef DEBUG
        fprintf(output.debug_out, "\n");
#endif
        skip_line(&here);
        continue;
    }


/* ************************************************************************** *
 * FINAL BINARY OUPUT                                                         *
 * ************************************************************************** */
    if (output.in_header) {
        fprintf(stdout, "%s: missing .end_header directive\n", output_filename);
    }

    while (output.code_position % 256 != 0) {
        fputc(0, output.out);
        ++output.code_position;
    }
    output.end_memory = output.code_position;
    add_label(&output.first_label, "_EXTSTART", output.end_memory);
    add_label(&output.first_label, "_ENDMEM", output.end_memory + output.extended_memory);


/* ************************************************************************** *
 * PROCESS BACKPATCH LIST                                                     *
 * ************************************************************************** */
    struct backpatch *patch = output.patch_list;
#ifdef DEBUG
    FILE *patch_file = fopen("out_patches.txt", "wt");
#endif
    while (patch) {
#ifdef DEBUG
        fprintf(patch_file,"%s  %x  %x  ", patch->name, patch->position, patch->position_after);
#endif

        struct label_def *label = get_label(output.first_label, patch->name);
        if (label) {
            int pos = label->pos;

            if (patch->position_after) {
                pos = pos - patch->position_after + 2;
            }

#ifdef DEBUG
            fprintf(patch_file, "=>  %d/%d\n", label->pos, pos);
#endif
            fseek(out, patch->position, SEEK_SET);
            write_word(out, pos);
        } else {
            fprintf(stdout, "%s: unknown identifier ~%s~\n",
                    output_filename, patch->name);
        }

        patch = patch->next;
    }
#ifdef DEBUG
    fclose(patch_file);
#endif


/* ************************************************************************** *
 * WRITE FILE HEADER                                                          *
 * ************************************************************************** */
    // WRITE HEADER
    fseek(output.out, 0, SEEK_SET);
    // magic number
    fputc(0x47, output.out);
    fputc(0x6C, output.out);
    fputc(0x75, output.out);
    fputc(0x6C, output.out);
    // glulx version
    fputc(0x00, output.out);
    fputc(0x03, output.out);
    fputc(0x01, output.out);
    fputc(0x02, output.out);
    // other fields
    write_word(out, output.ram_start);
    write_word(out, output.end_memory);
    write_word(out, output.end_memory + output.extended_memory);
    write_word(out, output.stack_size);

    struct label_def *label = get_label(output.first_label, "start");
    if (label) {
        unsigned start_address = label->pos;
        write_word(out, start_address);
    } else {
        write_word(out, 0);
        fprintf(stdout, "%s: missing start label\n", output_filename);
    }

    write_word(out, output.string_table);
    write_word(out, 0); // checksum placeholder
    // gasm marker
    fputc('g', output.out);
    fputc('a', output.out);
    fputc('s', output.out);
    fputc('m', output.out);

/* ************************************************************************** *
 * CALCULATE AND WRITE CHECKSUM                                               *
 * ************************************************************************** */
    fseek(out, 0, SEEK_SET);
    int checksum = 0;
    while (1) {
        int b1 = fgetc(out);
        if (feof(out)) {
            break;
        }
        int b2 = fgetc(out);
        int b3 = fgetc(out);
        int b4 = fgetc(out);
        int value = (b1 << 24) + (b2 << 16) + (b3 << 8) + b4;
        checksum += value;
    }
    fseek(out, 32, SEEK_SET);
    write_word(out, checksum); // checksum placeholder

    fclose(out);
#ifdef DEBUG
    FILE *label_file = fopen("out_labels.txt", "wt");
    dump_labels(label_file, output.first_label);
    fclose(label_file);
#endif
    free_labels(output.first_label);
    return !has_errors;
}
