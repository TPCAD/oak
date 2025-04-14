#ifndef OAK_TYPES_H
#define OAK_TYPES_H

#define __packed __attribute__((packed))
#define __inline __attribute__((always_inline))

#define EOF -1
#define EOS '\0'

#define NULL ((void *)0)

#define bool _Bool
#define true 1
#define false 0

#define CONCAT(x, y) x##y
#define RESERVED_TOKEN(x, y) CONCAT(x, y)
#define RESERVED RESERVED_TOKEN(reserved, __LINE__)

typedef unsigned int size_t;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef i32 pid_t;

typedef u32 time_t;

typedef u16 mode_t;

typedef i32 fd_t;
typedef enum std_fd_t {
    stdin,
    stdout,
    stderr,
} std_fd_t;

#endif // !OAK_TYPES_H
