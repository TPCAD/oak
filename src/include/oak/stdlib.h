#ifndef OAK_STDLIB_H
#define OAK_STDLIB_H

/* 将 v 向上取整至 k 的倍数，k 必须为 2 的幂 */
#define ROUNDUP(v, k) (((v) + (k) - 1) & ~((k) - 1))

void itoa(int input, char *buffer);

#endif // !OAK_STDLIB_H
