#ifndef OAK_STDIO_H
#define OAK_STDIO_H

#include <oak/stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

#endif // !OAK_STDIO_H
