#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"

int main(int argc, char *argv[]) {
    struct program_info info = { "output.ulx", 2048 };
    const char *infile  = "input.ga";

    enum {
        ts_standard,
        ts_notime,
        ts_custom
    } flag_timestamp_type = ts_standard;
    int flag_dump_labels = FALSE;
    int flag_dump_pretokens = FALSE;
    int flag_dump_tokens = FALSE;
    int flag_dump_patches = FALSE;
    int flag_dump_stringtable = FALSE;
    int flag_dump_debug = FALSE;
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
        } else if (strcmp(argv[i], "-dump-stringtable") == 0) {
            flag_dump_stringtable = TRUE;
        } else if (strcmp(argv[i], "-dump-debug") == 0) {
            flag_dump_debug = TRUE;
        } else if (strcmp(argv[i], "-no-time") == 0) {
            flag_timestamp_type = ts_notime;
        } else if (strcmp(argv[i], "-timestamp") == 0) {
            ++i;
            if (i >= argc) {
                fprintf(stderr, "-timestamp passed but no timestamp provided\n");
                return 1;
            }
            if (strlen(argv[i]) >= MAX_TIMESTAMP_SIZE) {
                fprintf(stderr, "max custom timestamp length is %d; provided stamp has length of %ld\n",
                        MAX_TIMESTAMP_SIZE - 1, strlen(argv[i]));
                return 1;
            }
            strncpy(info.timestamp, argv[i], MAX_TIMESTAMP_SIZE - 1);
            flag_timestamp_type = ts_custom;
        } else {
            if (filename_counter == 0) {
                infile = argv[i];
                filename_counter = 1;
            } else if (filename_counter == 1) {
                info.output_file = argv[i];
                filename_counter = 2;
            } else {
                fprintf(stderr, "Unknown argument \"%s\" passed.\n", argv[i]);
            }
        }
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    switch(flag_timestamp_type) {
        case ts_custom:
            break;
        case ts_standard:
            snprintf(info.timestamp, MAX_TIMESTAMP_SIZE,
                    "%d%02d%02d%02d%02d",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                    tm.tm_hour, tm.tm_min);
            break;
        case ts_notime:
            snprintf(info.timestamp, MAX_TIMESTAMP_SIZE,
                    "%d%02d%02d",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
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

    if (!parse_preprocess(tokens, &info)) {
        printf("Errors occured during preprocessing.\n");
        return 1;
    }
    string_build_tree(&info.strings);

    if (flag_dump_stringtable) {
        FILE *strings_file = fopen("out_strings.txt", "wt");
        dump_string_frequencies(strings_file, &info.strings);
        fclose(strings_file);
    }

    if (flag_dump_tokens) {
        FILE *tokens_file = fopen("out_tokens.txt", "wt");
        dump_token_list(tokens_file, tokens);
        fclose(tokens_file);
    }

    if (flag_dump_debug) {
        info.debug_out = fopen("out_debug.txt", "wt");
        if (!info.debug_out) {
            info.debug_out = NULL;
            printf("could not open debug dump file\n");
        }
    }

    if (!parse_tokens(tokens, &info)) {
        printf("Errors occured during parse & build.\n");
        if (remove(info.output_file) != 0) {
            perror("Could not remove failed build file");
        }
        free_patches(info.patch_list);
        free_labels(info.first_label);
        free_token_list(tokens);
        return 1;
    }

    if (flag_dump_debug) {
        fclose(info.debug_out);
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

    if (info.strings.input_bytes > 0) {
        printf("Compressed %d bytes of text into %d bytes.\n",
                info.strings.input_bytes,
                info.strings.output_bytes);
    }

    free_patches(info.patch_list);
    free_labels(info.first_label);
    free_token_list(tokens);
    return 0;
}
