#ifndef OAK_ASSERT_H
#define OAK_ASSERT_H

void assert_failure(char *exp, char *file, char *base, int line);

#define assert(exp)                                                            \
    if (exp)                                                                   \
        ;                                                                      \
    else                                                                       \
        assert_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)

void panic(const char *fmt, ...);

#endif // !OAK_ASSERT_H
