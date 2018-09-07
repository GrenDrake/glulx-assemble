#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"


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
