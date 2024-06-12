#ifndef OAK_TYPES_H
#define OAK_TYPES_H

#define EOF -1           // End Of File
#define NULL ((void *)0) // empty pointer
#define EOS '\0'         // End Of String

#define CONCAT(x, y) x##y
#define RESERVED_TOKEN(x, y) CONCAT(x, y)
#define RESERVED RESERVED_TOKEN(reserved, __LINE__)

#define bool _Bool
#define true 1
#define false 0

#define weak __attribute__((__weak__))

#define noreturn __attribute__((__noreturn__))

#define _packed __attribute__((packed))

#define _ofp __attribute__((optimize("omit-frame-pointer")))

#define _inline __attribute__((always_inline)) inline

typedef unsigned int size_t;
typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef int32 pid_t;
typedef int32 dev_t;

typedef u32 time_t;
typedef u32 idx_t;

typedef u16 mode_t;

typedef int32 fd_t;
typedef enum std_fd_t { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO } std_fd_t;

typedef int32 off_t; // file offset

#endif // DEBUG
