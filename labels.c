#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"

/* ************************************************************************** *
 * LABEL FUNCTIONS                                                            *
 * ************************************************************************** */

int add_label(struct label_def **first_lbl, const char *name, int value) {
    struct label_def *cur = *first_lbl;

    struct label_def *existing = get_label(*first_lbl, name);
    if (existing) {
        return 0;
    }

    struct label_def *new_lbl = malloc(sizeof(struct label_def));
    if (!new_lbl) {
        return 0;
    }
    new_lbl->name = str_dup(name);
    new_lbl->pos = value;

    if (cur == NULL) {
        new_lbl->next = NULL;
    } else {
        new_lbl->next = cur;
    }
    *first_lbl = new_lbl;
    return 1;
}

struct label_def* get_label(struct label_def *first, const char *name) {
    struct label_def *current = first;

    while (current) {
        if (strcmp(name, current->name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

void dump_labels(FILE *dest, struct label_def *first) {
    struct label_def *cur = first;
    while (cur) {
        fprintf(dest, "0x%08X  %s\n", cur->pos, cur->name);
        cur = cur->next;
    }
}

void free_labels(struct label_def *first) {
    struct label_def *cur = first;
    while (cur) {
        struct label_def *next = cur->next;
        free(cur->name);
        free(cur);
        cur = next;
    }
}


/* ************************************************************************** *
 * BACKPATCH FUNCTIONS                                                        *
 * ************************************************************************** */

void dump_patches(FILE *dest, struct program_info *info) {
    struct backpatch *patch = info->patch_list;
    if (patch == NULL) fprintf(dest, "No backpatches found!\n");

    while (patch) {
        fprintf(dest, "%-30s  0x%08X = %d (",
                patch->name, patch->position,
                patch->value_final);

        struct label_def *label = get_label(info->first_label, patch->name);
        if (label) {
            fprintf(dest, "%d)\n", label->pos);
        } else {
            fprintf(dest, "-)\n");
        }

        patch = patch->next;
    }
}

void free_patches(struct backpatch *first) {
    struct backpatch *cur = first;
    while (cur) {
        struct backpatch *next = cur->next;
        free(cur->name);
        free(cur);
        cur = next;
    }
}
