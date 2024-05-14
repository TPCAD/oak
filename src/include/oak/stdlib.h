#ifndef OAK_STDLIB_H
#define OAK_STDLIB_H

#include <oak/types.h>

void delay(u32 count);
void hang();

u8 bcd_to_bin(u8 value);
u8 bin_to_bcd(u8 value);

#endif // !OAK_STDLIB_H
