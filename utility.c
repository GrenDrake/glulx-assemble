#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *str_dup(const char *source) {
    size_t length = strlen(source);
    char *new_str = malloc(length + 1);
    if (new_str == NULL) return NULL;
    strcpy(new_str, source);
    return new_str;
}


int cleanup_string(char *text) {
    // remove newlines originating from original source code
    size_t length = strlen(text);
    for (size_t i = 0; i < length; ++i) {
        if (text[i] == '\n') {
            size_t start = i + 1;
            size_t end = i + 1;
            while (start > 0 && isspace(text[start - 1])) {
                --start;
            }
            while (end < length && isspace(text[end])) {
                ++end;
            }

            if (start > 1 && text[start - 1] == 'n' && text[start - 2] == '\\') {
                memmove(&text[start], &text[end], length - start);
            } else {
                memmove(&text[start + 1], &text[end], length - start - 1);
                text[start] = ' ';
            }
            i = start;
        }
    }

    // handle string escapes
    length = strlen(text);
    for (size_t i = 0; i < length; ++i) {

        if (text[i] != '\\') continue;

        ++i;
        switch(text[i]) {
            case '"':
            case '\'':
            case '\\':
                text[i - 1] = text[i];
                break;
            case 'n':
                text[i - 1] = '\n';
                break;
            default:
                return i;
        }

        for (size_t j = i; j < length; ++j) {
            text[j] = text[j + 1];
        }
        --i;
    }

    return 0;
}


void dump_string(FILE *dest, const char *text, unsigned max_length) {
    if (text == NULL) {
        fputs("(null)", dest);
        return;
    }

    int was_truncated = 0;
    size_t length = strlen(text);
    if (length > max_length) {
        length = max_length;
        was_truncated = 1;
    }

    for (unsigned i = 0; i < length; ++i) {
        int c = text[i];
        switch(c) {
            case '\n':
                fputs("\\n", dest);
                break;
            case '\r':
                fputs("\\r", dest);
                break;
            case '\t':
                fputs("\\t", dest);
                break;
            case '\\':
                fputs("\\", dest);
                break;
            default:
                fputc(c, dest);
        }
    }
    if (was_truncated) {
        fputs("...", dest);
    }
}
