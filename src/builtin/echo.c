#include <oak/stdio.h>
#include <oak/string.h>
#include <oak/syscall.h>
#include <oak/types.h>

int main(int argc, char const *argv[]) {
    for (size_t i = 1; i < argc; i++) {
        printf(argv[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}
