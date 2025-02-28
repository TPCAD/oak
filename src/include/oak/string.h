#ifndef OAK_STRING_H
#define OAK_STRING_H

#include <oak/types.h>

size_t strlen(const char *str);
int strcmp(const char *lhs, const char *rhs);
char *strchr(const char *str, int ch);
char *strrchr(const char *str, int ch);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);

void *memchr(const void *ptr, int ch, size_t count);
int memcmp(const void *lhs, const void *rhs, size_t count);
void *memset(void *dest, int ch, size_t count);
void *memcpy(void *dest, const void *src, size_t count);

#endif // !OAK_STRING_H
