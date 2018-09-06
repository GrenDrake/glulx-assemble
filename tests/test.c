#include <stdio.h>
#include "test.h"

int main() {
    int errors = 0;
    int test_number = 0;

    printf("\n\x1B[1mRunning tests for %s\x1B[0m\n", test_suite_name);

    while(test_list[test_number].test_name) {
        printf("%s: ", test_list[test_number].test_name);
        if (!test_list[test_number].test_func()) {
            ++errors;
        } else {
            fputs("DONE\n", stdout);
        }
        ++test_number;
    }

    if (errors) fputs("\x1B[31m", stdout);
    else        fputs("\x1B[32m", stdout);
    printf("%s: RAN %d TESTS; %d FAILED",
            test_suite_name, test_number, errors);
    fputs("\x1B[0m", stdout);
    fputc('\n', stdout);

    return 0;
}
