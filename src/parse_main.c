#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"
#include "vbuffer.h"


static int parse_string_data(struct token *first,
                             struct output_state *output,
                             int add_type_byte);
static int parse_unicode_data(struct token *first,
                              struct output_state *output);
static int data_zeroes(struct token *first, struct output_state *output);
static int data_bytes(struct token *first, struct output_state *output, int width);
static int start_function(struct token *first, struct output_state *output);
static void end_function(struct output_state *output);

struct operand* parse_operand(struct token **from, struct output_state *output);
void free_operands(struct operand *first_operand);
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

void write_variable(FILE *out, uint32_t value, int width) {
    switch(width) {
        case 1:
            write_byte(out, value);
            break;
        case 2:
            write_short(out, value);
            break;
        case 4:
            write_word(out, value);
            break;
    }
}

int value_fits(uint32_t value, int width) {
    switch(width) {
        case 1:
            return value == (value & 0xFF);
        case 2:
            return value == (value & 0xFFFF);
        case 4:
            return TRUE;
        default:
            return FALSE;
    }
}


/* ************************************************************************** *
 * DIRECTIVE PARSING                                                          *
 * ************************************************************************** */

static int parse_string_data(struct token *first,
                        struct output_state *output,
                        int add_type_byte) {
    struct token *here = first->next;

    if (here->type != tt_string) {
        report_error(&here->origin, "expected string");
        return FALSE;
    }

    if (output->info->debug_out) {
        fprintf(output->info->debug_out, "0x%08X string ~", output->code_position);
        dump_string(output->info->debug_out, here->text, 32);
        fprintf(output->info->debug_out, "~\n");
    }

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
    return TRUE;
}

static int parse_unicode_data(struct token *first,
                              struct output_state *output) {
    struct token *here = first->next;

    if (here->type != tt_string) {
        report_error(&here->origin, "expected string");
        return FALSE;
    }

    if (output->info->debug_out) {
        fprintf(output->info->debug_out, "0x%08X unicode ~", output->code_position);
        dump_string(output->info->debug_out, here->text, 32);
        fprintf(output->info->debug_out, "~\n");
    }
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
    return TRUE;
}

static int data_zeroes(struct token *first, struct output_state *output) {
    struct token *here = first->next;

    if (here->type != tt_integer) {
        report_error(&here->origin, "expected integer, found %s", token_name(here));
        return FALSE;
    }

    if (output->info->debug_out) {
        fprintf(output->info->debug_out, "0x%08X zeroes (%d)\n", output->code_position, here->i);
    }
    for (int i = 0; i < here->i; ++i) {
        fputc(0, output->out);
    }
    output->code_position += here->i;
    expect_eol(&here);
    return TRUE;
}

static int data_bytes(struct token *first, struct output_state *output, int width) {
    int has_errors = FALSE;
    struct token *here = first->next;
    if (output->info->debug_out) {
        fprintf(output->info->debug_out, "0x%08X data(%d)", output->code_position, width);
    }

    while (here && here->type != tt_eol) {
        struct token *op_start = here;
        struct operand *operand = parse_operand(&here, output);
        if (operand) {
            if (operand->type == ot_local) {
                report_error(&op_start->origin, "local variables cannot be used in data directive");
                free_operands(operand);
                has_errors = TRUE;
                continue;
            }
            if (operand->type != ot_constant) {
                report_error(&op_start->origin, "operand modes cannot be used in data directive");
                free_operands(operand);
                has_errors = TRUE;
                continue;
            }

            if (operand->known_value) {
                if (output->info->debug_out) {
                    fprintf(output->info->debug_out, " %d", here->i);
                }
                if (!value_fits(operand->value, width)) {
                    report_error(&op_start->origin, "value is larger than storage specification");
                    has_errors = TRUE;
                    free_operands(operand);
                    continue;
                }
                write_variable(output->out, operand->value, width);
                output->code_position += width;
            } else {
                struct backpatch *patch = malloc(sizeof(struct backpatch));
                patch->next = 0;
                patch->max_width = width;
                patch->name = str_dup(operand->name);
                patch->position = output->code_position;
                patch->position_after = 0;
                if (output->info->patch_list) {
                    patch->next = output->info->patch_list;
                }
                output->info->patch_list = patch;
                write_variable(output->out, 0, width);
                output->code_position += width;
            }
            free_operands(operand);
        }
    }
    if (output->info->debug_out) {
        fprintf(output->info->debug_out, "\n");
    }
    return !has_errors;
}

static int start_function(struct token *first, struct output_state *output) {
    int stack_based = FALSE;
    int found_errors = FALSE;
    struct token *here = first->next; // skip ".function"
    end_function(output);
    int start_pos = output->code_position;

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
                    report_error(&here->origin, "expected identifier, found %s", token_name(here));
                    found_errors = TRUE;
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

    if (output->info->debug_out) {
        fprintf(output->info->debug_out,
                "\n0x%08X FUNCTION %s   %d LOCALS",
                start_pos,
                stack_based ? "(stk)" : "",
                name_count);
        if (output->local_names) {
            fputc(':', output->info->debug_out);
            struct local_list *cur = output->local_names;
            while (cur) {
                fprintf(output->info->debug_out,
                        " %s",
                        cur->name);
                cur = cur->next;
            }
        }
        fputc('\n', output->info->debug_out);
    }

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

    return !found_errors;
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
            struct label_def *label = get_label(output->info->first_label, here->text);
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
        report_error(&here->origin, "unexpected token found");
        from = &here->next;
        free(op);
        return NULL;
    }

    *from = here->next;
    return op;
}

void free_operands(struct operand *first_operand) {
    while (first_operand) {
        struct operand *next = first_operand->next;
        free(first_operand);
        first_operand = next;
    }
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
            if (op->value >= -32768 && op->value <= 0x7FFF) return 2;
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
int parse_tokens(struct token_list *list, struct program_info *info) {
    struct output_state output = { info, TRUE };
    int has_errors = 0;

    FILE *out = fopen(info->output_file, "wb+");
    output.out = out;
    if (!out) {
        printf("could not open output file\n");
        return FALSE;
    }

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
            report_error(&here->origin, "expected identifier");
            has_errors = 1;
            skip_line(&here);
            continue;
        }

        if (here->next && here->next->type == tt_colon) {
            if (!add_label(&output.info->first_label, here->text, output.code_position)) {
                report_error(&here->origin, "could not create label (already exists?)");
                has_errors = TRUE;
            }
            here = here->next->next;
            continue;
        }

/* ************************************************************************** *
 * DIRECTIVE PROCESSING                                                       *
 * ************************************************************************** */
        if (strcmp(here->text, ".cstring") == 0) {
            if (!parse_string_data(here, &output, FALSE)) {
                has_errors = TRUE;
            }
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".string") == 0) {
            if (!parse_string_data(here, &output, TRUE)) {
                has_errors = TRUE;
            }
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".unicode") == 0) {
            if (!parse_unicode_data(here, &output)) {
                has_errors = TRUE;
            }
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".encoded") == 0) {
            here = here->next;
            if (here->type != tt_string) {
                report_error(&here->origin, "Expected string");
                skip_line(&here);
                continue;
            }
            int size = encode_string(output.out, &output.info->strings, here->text);
            if (size < 0) has_errors = TRUE;
            output.code_position += size;
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".byte") == 0) {
            if (!data_bytes(here, &output, 1)) {
                has_errors = TRUE;
            }
            skip_line(&here);
            continue;
        }
        if (strcmp(here->text, ".short") == 0) {
            if (!data_bytes(here, &output, 2)) {
                has_errors = TRUE;
            }
            skip_line(&here);
            continue;
        }
        if (strcmp(here->text, ".word") == 0) {
            if (!data_bytes(here, &output, 4)) {
                has_errors = TRUE;
            }
            skip_line(&here);
            continue;
        }
        if (strcmp(here->text, ".zero") == 0) {
            if (!data_zeroes(here, &output)) {
                has_errors = TRUE;
            }
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".function") == 0) {
            if (!start_function(here, &output)) {
                has_errors = TRUE;
            }
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".end_header") == 0) {
            if (!output.in_header) {
                report_error(&here->origin, "ended header when not in header");
                has_errors = TRUE;
                skip_line(&here);
                continue;
            }

            while (output.code_position % 256 != 0) {
                fputc(0, output.out);
                ++output.code_position;
            }
            output.in_header = FALSE;
            output.info->ram_start = output.code_position;
            add_label(&output.info->first_label, "_RAMSTART", output.info->ram_start);
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".extra_memory") == 0) {
            here = here->next;
            if (here->type != tt_integer) {
                report_error(&here->origin, "expected integer, found %s", token_name(here));
                has_errors = TRUE;
                skip_line(&here);
                continue;
            }
            if (here->i % 256) {
                report_error(&here->origin, "extra memory must be multiple of 256 (currently %d, next multiple %d)",
                            here->i,
                            (here->i / 256 + 1) * 256);
                has_errors = TRUE;
            }
            output.info->extended_memory = here->i;
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".stack_size") == 0) {
            here = here->next;
            if (here->type != tt_integer) {
                report_error(&here->origin, "expected integer, found %s", token_name(here));
                has_errors = TRUE;
                skip_line(&here);
                continue;
            }
            if (here->i % 256) {
                report_error(&here->origin, "stack size must be multiple of 256 (currently %d, next multiple %d)",
                            here->i,
                            (here->i / 256 + 1) * 256);
                has_errors = TRUE;
            }
            output.info->stack_size = here->i;
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".include") == 0
            || strcmp(here->text, ".define") == 0) {
            report_error(&here->origin,
                        "(internal) encountered %s directive after pre-processing",
                        here->text);
            has_errors = TRUE;
            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".include_binary") == 0) {
            here = here->next;
            if (here->type != tt_string) {
                report_error(&here->origin, "expected string, found %s", token_name(here));
                has_errors = TRUE;
                skip_line(&here);
                continue;
            }

            struct vbuffer *buffer = vbuffer_new();
            int result = vbuffer_readfile(buffer, here->text);
            if (!result) {
                report_error(&here->origin, "Could not read binary file ~%s~.", here->text);
                has_errors = TRUE;
                skip_line(&here);
                vbuffer_free(buffer);
                continue;
            }
            fwrite(buffer->data, buffer->length, 1, out);

            if (output.info->debug_out) {
                fprintf(output.info->debug_out, "0x%08X BINARY FILE ~%s~ (%d bytes)\n",
                        output.code_position,
                        here->text,
                        buffer->length);
            }

            vbuffer_free(buffer);
            output.code_position += buffer->length;

            skip_line(&here);
            continue;
        }

        if (strcmp(here->text, ".string_table") == 0) {
            if (output.info->strings.first == NULL) {
                // skip empty string tables
                skip_line(&here);
                continue;
            }

            output.info->string_table = output.code_position;
            int table_start = output.code_position + 12;

            int table_size = 12;
            struct string_node *node = info->strings.first;
            while (node) {
                table_size += node_size(node);
                node = node->next;
            }

            write_word(output.out, table_size); // table size (bytes)
            write_word(output.out, node_list_size(info->strings.first)); // table size (nodes)
            write_word(output.out, info->strings.root->position + table_start); // root node
            output.code_position += 12;

            node = info->strings.first;
            while (node) {
                switch(node->type) {
                    case nt_end:
                        write_byte(output.out, 1);
                        break;
                    case nt_branch:
                        write_byte(output.out, 0);
                        write_word(output.out, node->d.branch.left->position + table_start);
                        write_word(output.out, node->d.branch.right->position + table_start);
                        break;
                    case nt_char:
                        write_byte(output.out, 2);
                        write_byte(output.out, node->d.a_char.c);
                        break;
                    case nt_unichar:
                        write_byte(output.out, 4);
                        write_word(output.out, node->d.a_char.c);
                        break;
                }
                output.code_position += node_size(node);
                node = node->next;
            }
            skip_line(&here);
            continue;
        }

        if (here->text[0] == '.') {
            report_error(&here->origin, "unknown directive %s", here->text);
            has_errors = TRUE;
            skip_line(&here);
            continue;
        }

/* ************************************************************************** *
 * MNEMONIC PROCESSING                                                        *
 * ************************************************************************** */
        struct mnemonic customCode = { "custom opcode", -1, -1, FALSE };
        struct mnemonic *m = codes;
        struct token *mnemonic_start = here;
        if (strcmp(here->text, "opcode") == 0) {
            m = &customCode;
            here = here->next;
            if (token_check_identifier(here, "rel")) {
                here = here->next;
                customCode.last_operand_is_relative= TRUE;
            }
            if (!here->type) {
                report_error(&here->origin,
                            "expected integer, but found %s",
                            token_name(here));
            }
            customCode.opcode = here->i;
        } else {
            while (m->name && strcmp(m->name, here->text) != 0) {
                ++m;
            }
            if (m->name == NULL) {
                report_error(&mnemonic_start->origin, "unknown mnemonic %s", here->text);
                has_errors = TRUE;
                skip_line(&here);
                continue;
            }
        }

        if (output.info->debug_out) {
            fprintf(output.info->debug_out, "0x%08X ~%s~ %d/0x%x   (at 0x%lx)  ",
                    output.code_position,
                    here->text,
                    m->opcode,
                    m->opcode,
                    ftell(output.out));
        }

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
                has_errors = TRUE;
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

        if (m->operand_count >= 0 && operand_count != m->operand_count) {
            report_error(&mnemonic_start->origin,
                        "bad operand count for %s; expected %d, but found %d.",
                        m->name, m->operand_count, operand_count);
            free_operands(op_list);
            has_errors = TRUE;
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

        if (output.info->debug_out) {
            fprintf(output.info->debug_out, " types");
        }

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
                if (output.info->debug_out) {
                    fprintf(output.info->debug_out, " %X", type_byte);
                }
            } else {
                type_count = 1;
                type_byte = my_type;
            }

            cur_op = cur_op->next;
        }
        if (type_count) {
            fputc(type_byte, out);
            ++output.code_position;
            if (output.info->debug_out) {
                fprintf(output.info->debug_out, " %X", type_byte);
            }
        }


        // write operands to file
        cur_op = op_list;
        while (cur_op) {
            if (!cur_op->known_value) {
                struct backpatch *patch = malloc(sizeof(struct backpatch));
                patch->next = 0;
                patch->max_width = 4;
                patch->name = str_dup(cur_op->name);
                patch->position = output.code_position;
                patch->position_after = after_pos;
                if (output.info->patch_list) {
                    patch->next = output.info->patch_list;
                }
                output.info->patch_list = patch;
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
                    report_error(&here->origin, "(internal) Bad operand size");
                    has_errors = TRUE;
            }
            if (output.info->debug_out) {
                if (cur_op->type == ot_stack) {
                    fprintf(output.info->debug_out, " STACK");
                } else {
                    switch(cur_op->type) {
                        case ot_constant:   fputs(" c:", output.info->debug_out);   break;
                        case ot_local:      fputs(" l:", output.info->debug_out);   break;
                        case ot_indirect:   fputs(" i:", output.info->debug_out);   break;
                        case ot_afterram:   fputs(" a:", output.info->debug_out);   break;
                        default:
                            fprintf(output.info->debug_out, " (bad operand type %d", cur_op->type);
                    }
                    if (cur_op->known_value) {
                        fprintf(output.info->debug_out, "%d", cur_op->value);
                    } else {
                        fprintf(output.info->debug_out, "???");
                    }
                }
            }
            cur_op = cur_op->next;
        }
        free_operands(op_list);

        if (output.info->debug_out) {
            fprintf(output.info->debug_out, "\n");
        }
        skip_line(&here);
        continue;
    }


    struct origin objectfile_origin = { (char*)info->output_file, -1 };
/* ************************************************************************** *
 * FINAL BINARY OUPUT                                                         *
 * ************************************************************************** */
    if (output.in_header) {
        report_error(&objectfile_origin, "missing .end_header directive\n");
        has_errors = TRUE;
    }

    while (output.code_position % 256 != 0) {
        fputc(0, output.out);
        ++output.code_position;
    }
    output.info->end_memory = output.code_position;
    add_label(&output.info->first_label, "_EXTSTART", output.info->end_memory);
    add_label(&output.info->first_label, "_ENDMEM", output.info->end_memory + output.info->extended_memory);


/* ************************************************************************** *
 * PROCESS BACKPATCH LIST                                                     *
 * ************************************************************************** */
    struct backpatch *patch = output.info->patch_list;
    while (patch) {
        struct label_def *label = get_label(output.info->first_label, patch->name);
        if (label) {
            patch->value_final = label->pos;

            if (patch->position_after) {
                patch->value_final = patch->value_final - patch->position_after + 2;
            }

            fseek(out, patch->position, SEEK_SET);
            if (!value_fits(patch->value_final, patch->max_width)) {
                report_error(&objectfile_origin,
                        "(warning) value of ~%s~ is larger than storage specification and will be truncated\n",
                        patch->name);
            }
            write_variable(out, patch->value_final, patch->max_width);
        } else {
            report_error(&objectfile_origin, "unknown identifier ~%s~\n",
                    patch->name);
            has_errors = TRUE;
        }

        patch = patch->next;
    }


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
    write_word(out, output.info->ram_start);
    write_word(out, output.info->end_memory);
    write_word(out, output.info->end_memory + output.info->extended_memory);
    write_word(out, output.info->stack_size);

    struct label_def *label = get_label(output.info->first_label, "start");
    if (label) {
        unsigned start_address = label->pos;
        write_word(out, start_address);
    } else {
        write_word(out, 0);
        report_error(&objectfile_origin, "missing start label", info->output_file);
        has_errors = TRUE;
    }

    if (output.info->string_table == 0) {
        write_word(out, 0);
        if (output.info->strings.first != NULL) {
            report_error(&objectfile_origin, "source contains encoded strings but does not include .string_table directive");
        }
    } else {
        write_word(out, output.info->string_table);
    }
    write_word(out, 0); // checksum placeholder
    // gasm marker
    fputc('g', output.out);
    fputc('a', output.out);
    fputc('s', output.out);
    fputc('m', output.out);
    // twelve-byte timestamp
    for (int i = 0; i < MAX_TIMESTAMP_SIZE - 1; ++i) {
        write_byte(output.out, output.info->timestamp[i]);
    }

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
    return !has_errors;
}
