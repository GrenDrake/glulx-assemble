#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>

#define UTF8_REPLACEMENT_CHAR 0xFFFD

char *str_dup(const char *source);
int cleanup_string(char *text);
void dump_string(FILE *dest, const char *text, unsigned max_length);
int utf8_next_char(const char *text, int *pos);

#endif
