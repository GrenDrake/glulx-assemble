#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"

void free_string_table(struct string_table *table) {
    for (int i = 0; i < STRING_TABLE_BUCKETS; ++i) {
        struct string_table_entry *entry = table->buckets[i];
        while (entry) {
            struct string_table_entry *next = entry->next;
            free(entry);
            entry = next;
        }
        table->buckets[i] = NULL;
    }

    struct string_node *node = table->first;
    while (node) {
        struct string_node *next = node->next;
        free(node);
        node = next;
    }
}

void string_table_add(struct string_table *table, unsigned c) {
    unsigned hash = c % STRING_TABLE_BUCKETS;
    struct string_table_entry *entry;

    entry = table->buckets[hash];
    if (entry) {
        while (entry && entry->c != c) {
            entry = entry->next;
        }
    }

    if (entry) {
        ++entry->count;
    } else {
        entry = calloc(sizeof(struct string_table_entry), 1);
        entry->c = c;
        entry->count = 1;
        if (table->buckets[hash]) {
            entry->next = table->buckets[hash];
        }
        table->buckets[hash] = entry;
    }
}

void string_add_to_frequencies(struct string_table *table, const char *string) {
    int pos = 0;
    int c = 0;
    do {
        c = utf8_next_char(string, &pos);
        string_table_add(table, c);
    } while (c != 0);
}

static void node_list_add(struct string_node **first, struct string_node *node) {
    if (node == NULL) return;
    if (*first == NULL) {
        *first = node;
        return;
    }

    struct string_node *here = *first, *prev = NULL;
    while (here && here->weight < node->weight) {
        prev = here;
        here = here->next;
    }
    node->prev = prev;
    node->next = here;
    if (prev) prev->next = node;
    if (here) here->prev = node;
    if (*first == here) *first = node;
}

static void node_list_final(struct string_node **first, struct string_node *node) {
    if (!node) return;
    node_list_add(first, node);
    if (node->type == nt_branch) {
        node_list_final(first, node->d.branch.left);
        node_list_final(first, node->d.branch.right);
    }
}

int node_list_size(struct string_node *node) {
    int count = 0;
    while (node) {
        ++count;
        node = node->next;
    }
    return count;
}

int node_size(struct string_node *node) {
    switch(node->type) {
        case nt_branch:     return 9;
        case nt_end:        return 1;
        case nt_char:       return 2;
        case nt_unichar:    return 5;
        default:
            fprintf(stderr, "Unknown stringtable node type %d.\n", node->type);
            return 0;
    }
}

void string_build_tree(struct string_table *table) {
    struct string_node *first = NULL;
    struct string_table_entry *entry;

    for (int i = 0; i < STRING_TABLE_BUCKETS; ++i) {
        entry = table->buckets[i];
        while (entry) {
            struct string_node *node = calloc(1, sizeof(struct string_node));
            node->weight = entry->count;
            if (entry->c == 0) {
                node->type = nt_end;
            } else if (entry->c <= 127) {
                node->type = nt_char;
                node->d.a_char.c = entry->c;
            } else {
                node->type = nt_unichar;
                node->d.a_char.c = entry->c;
            }
            node_list_add(&first, node);
            entry = entry->next;
        }
    }

    if (!first) return;

    while (first->next) {
        struct string_node *branch = calloc(1, sizeof(struct string_node));
        branch->type = nt_branch;
        branch->d.branch.left = first;
        branch->d.branch.right = first->next;
        branch->weight = first->weight + first->next->weight;
        first = first->next->next;
        node_list_add(&first, branch);
    }

    table->root = first;
    node_list_final(&table->first, first);

    int position = 0;
    struct string_node *node = table->first;
    while (node) {
        node->position = position;
        position += node_size(node);
        node = node->next;
    }
}

int string_node_contains(struct string_node *node, int c) {
    switch(node->type) {
        case nt_char:
        case nt_unichar:
            return node->d.a_char.c == c;
        case nt_end:
            return c == 0;
        case nt_branch:
            return string_node_contains(node->d.branch.left, c)
                    || string_node_contains(node->d.branch.right, c);
            break;
        default:
            fprintf(stderr, "Unknown stringtable node type %d.\n", node->type);
            return FALSE;
    }
}

static unsigned char reverse_byte(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

static void step_byte(FILE *out, int *byte, int *byte_position, int *size, int flag) {
    if (flag == 2) {
        // make sure buffer is flushed
        if (*byte_position != 0) {
            while (*byte_position < 8) {
                *byte_position += 1;
                *byte <<= 1;
            }
            fputc(reverse_byte(*byte), out);
            *size += 1;
        }
        return ;
    }

    *byte <<= 1;
    if (flag) {
        *byte |= 1;
    }
    *byte_position += 1;
    if (*byte_position == 8) {
        *byte_position = 0;
        *size += 1;
        fputc(reverse_byte(*byte), out);
        *byte = 0;
    }
}

int encode_string(FILE *out, struct string_table *table, const char *text) {
    int size = 1;
    int byte = 0, byte_position = 0;
    int text_position = 0;

    table->input_bytes += strlen(text) + 1;

    fputc(0xE1, out);
    while (TRUE) {
        int c = utf8_next_char(text, &text_position);

        struct string_node *node = table->root;
        while (node->type == nt_branch) {
            if (string_node_contains(node->d.branch.left, c)) {
                step_byte(out, &byte, &byte_position, &size, FALSE);
                node = node->d.branch.left;
            } else if (string_node_contains(node->d.branch.right, c)) {
                step_byte(out, &byte, &byte_position, &size, TRUE);
                node = node->d.branch.right;
            } else {
                report_error(NULL, "(internal) Tried to encode character %d, but character not in encoding table!", c);
                return -1;
            }
        }

        if (c == 0) {
            step_byte(out, &byte, &byte_position, &size, 2);
            table->output_bytes += size;
            return size;
        }
    }
}

void dump_string_frequencies(FILE *dest, struct string_table *table) {
    fprintf(dest, "%d NODES\nROOT NODE AT %d\n\n", node_list_size(table->first), table->root->position);
    fprintf(dest, "  WEIGHT TP  POS CONTENT\n");
    struct string_node *node = table->first;
    while (node) {
        fprintf(dest, "%8d %2d %4d ", node->weight, node->type, node->position);
        switch(node->type) {
            case nt_end:
                fprintf(dest, " END\n");
                break;
            case nt_unichar:
                fprintf(dest, " UNI  %d\n", node->d.a_char.c);
                break;
            case nt_char:
                fprintf(dest, " CHAR %d ", node->d.a_char.c);
                if (isprint(node->d.a_char.c)) {
                    fprintf(dest, "'%c'", node->d.a_char.c);
                }
                fputc('\n', dest);
                break;
            case nt_branch:
                fprintf(dest, " BRANCH %d <> %d\n",
                              node->d.branch.left->position,
                              node->d.branch.right->position);
                break;
        }
        node = node->next;
    }
}
