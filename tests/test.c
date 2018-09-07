#include <stdio.h>
#include "test.h"

int main() {
    int errors = 0;
    int test_number = 0;

    while(test_list[test_number].test_name) {
        const char *msg = test_list[test_number].test_func();
        if (msg) {
            printf("%s: %s: \x1B[31mFAILED\x1B[0m %s\n",
                    test_suite_name,
                    test_list[test_number].test_name,
                    msg);
            ++errors;
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
