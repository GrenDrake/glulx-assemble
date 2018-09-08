#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"

int main(int argc, char *argv[]) {
    const char *infile  = "input.ga";
    const char *outfile = "output.ulx";

    int flag_dump_labels = FALSE;
    int flag_dump_pretokens = FALSE;
    int flag_dump_tokens = FALSE;
    int flag_dump_patches = FALSE;
    int filename_counter = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-dump-pretokens") == 0) {
            flag_dump_pretokens = TRUE;
        } else if (strcmp(argv[i], "-dump-tokens") == 0) {
            flag_dump_tokens = TRUE;
        } else if (strcmp(argv[i], "-dump-labels") == 0) {
            flag_dump_labels = TRUE;
        } else if (strcmp(argv[i], "-dump-patches") == 0) {
            flag_dump_patches = TRUE;
        } else {
            if (filename_counter == 0) {
                infile = argv[i];
                filename_counter = 1;
            } else if (filename_counter == 1) {
                outfile = argv[i];
                filename_counter = 2;
            } else {
                fprintf(stderr, "Unknown argument \"%s\" passed.\n", argv[i]);
            }
        }
    }

    struct token_list *tokens = lex_file(infile);
    if (tokens == NULL) {
        printf("Errors occured during lexing.\n");
        return 1;
    }

    if (flag_dump_pretokens) {
        FILE *tokens_file = fopen("out_pretokens.txt", "wt");
        dump_token_list(tokens_file, tokens);
        fclose(tokens_file);
    }

    struct program_info info = { outfile, 2048 };

    if (!parse_preprocess(tokens, &info)) {
        printf("Errors occured during preprocessing.\n");
        return 1;
    }

    if (flag_dump_tokens) {
        FILE *tokens_file = fopen("out_tokens.txt", "wt");
        dump_token_list(tokens_file, tokens);
        fclose(tokens_file);
    }

    if (!parse_tokens(tokens, &info)) {
        printf("Errors occured during parse & build.\n");
        if (remove(info.output_file) != 0) {
            perror("Could not remove failed build file");
        }
        return 1;
    }

    if (flag_dump_labels) {
        FILE *label_file = fopen("out_labels.txt", "wt");
        dump_labels(label_file, info.first_label);
        fclose(label_file);
    }

    if (flag_dump_patches) {
        FILE *patch_file = fopen("out_patches.txt", "wt");
        dump_patches(patch_file, &info);
        fclose(patch_file);
    }

    free_patches(info.patch_list);
    free_labels(info.first_label);
    free_token_list(tokens);
    return 0;
}
