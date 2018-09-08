#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assemble.h"

int main(int argc, char *argv[]) {
    const char *infile  = "input.ga";
    const char *outfile = "output.ulx";

    int flag_dump_tokens = FALSE;
    int filename_counter = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-dump-tokens") == 0) {
            flag_dump_tokens = TRUE;
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
        printf("Errors occured.\n");
        return 1;
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
    parse_tokens(tokens, &info);
    free_token_list(tokens);
    return 0;
}
