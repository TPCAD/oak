#ifndef OAK_TYPES_H
#define OAK_TYPES_H

#define __pack __attribute__((packed))

#define EOF -1
#define EOS '\0'

#define NULL ((void *)0)

#define bool _Bool
#define true 1
#define false 0

typedef unsigned int size_t;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#endif // !OAK_TYPES_H
