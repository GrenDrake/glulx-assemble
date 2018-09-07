#include <stdio.h>
#include <stdlib.h>

#include "assemble.h"

int main(int argc, char *argv[]) {
    const char *infile  = "input.ga";
    const char *outfile = "output.ulx";

    if (argc >= 2) {
        infile = argv[1];
    }
    if (argc >= 3) {
        outfile = argv[2];
    }

    struct token_list *tokens = lex_file(infile);
    if (tokens == NULL) {
        printf("Errors occured.\n");
        return 1;
    }

    if (!parse_preprocess(tokens)) {
        printf("Errors occured during preprocessing.\n");
        return 1;
    }

    parse_tokens(tokens, outfile);
    free_token_list(tokens);
    return 0;
}
