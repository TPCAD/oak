#ifndef OAK_STDLIB_H
#define OAK_STDLIB_H

#include <oak/types.h>

#define MAX(a, b) (a < b ? b : a)
#define MIN(a, b) (a < b ? a : b)

/* 将 v 向上取整至 k 的倍数，k 必须为 2 的幂 */
#define ROUNDUP(v, k) (((v) + (k) - 1) & ~((k) - 1))

#define DIV_ROUNDUP(n, d) (((n) + (d) - 1) / (d))

void itoa(int input, char *buffer);
int atoi(const char *str);

u8 bcd_to_bin(u8 value);
u8 bin_to_bcd(u8 value);

#endif // !OAK_STDLIB_H
