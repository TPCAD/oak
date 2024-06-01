#ifndef OAK_CONSOLE_H
#define OAK_CONSOLE_H

#include <oak/types.h>

void console_init();
void console_clear();
int32 console_write(char *buf, u32 count);

#endif // !OAK_CONSOLE_H
