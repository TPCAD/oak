#include <oak/stdio.h>
#include <oak/string.h>
#include <oak/syscall.h>
#include <oak/types.h>

int main(int argc, char const *argv[], char const *envp[]) {
    for (size_t i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    for (size_t i = 0; 1; i++) {
        if (!envp[i])
            break;
        int len = strlen(envp[i]);
        if (len >= 1022)
            continue;
        printf("%s\n", envp[i]);
    }
    return 0;
}
