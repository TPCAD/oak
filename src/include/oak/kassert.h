#ifndef OAK_KASSERT_H
#define OAK_KASSERT_H

void kassert_failure(char *exp, char *file, char *base, int line);

#define kassert(exp)                                                           \
    if (exp)                                                                   \
        ;                                                                      \
    else                                                                       \
        kassert_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)

void kpanic(const char *fmt, ...);

#endif // !OAK_KASSERT_H
