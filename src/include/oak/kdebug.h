#ifndef OAK_KDEBUG_H
#define OAK_KDEBUG_H

void kdebug(char *file, int line, const char *fmt, ...);

#define BMB asm volatile("xchgw %bx, %bx") // bochs magic breakpoint
#define KDEBUG(fmt, args...) kdebug(__BASE_FILE__, __LINE__, fmt, ##args)

#endif // !OAK_KDEBUG_H
