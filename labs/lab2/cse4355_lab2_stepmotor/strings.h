#ifndef STRINGS_H_
#define STRINGS_H_

#include <stdint.h>

char* toAsciiHex(char* buffer, uint32_t value);
char* toAsciiDec(char* buffer, uint32_t value);
uint32_t str_cmp(const char* str1, const char* str2);
uint32_t str_len(const char* str);
char* str_cpy(char* paste, char* copy);

#endif
