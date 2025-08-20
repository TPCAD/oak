#include "oak/fs/minix.h"
#include "oak/syscall.h"
#include <oak/stdio.h>
#include <oak/types.h>

#define BUFLEN 1024
char buf[BUFLEN];

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return EOF;
    }

    fd_t fd = open(argv[1], O_RDONLY, 0);
    if (fd == EOF) {
        printf("file %s not exists.\n", argv[1]);
        return EOF;
    }

    while (true) {
        int len = read(fd, buf, BUFLEN);
        if (len == EOF) {
            break;
        }
        write(STDOUT_FILENO, buf, len);
    }
    close(fd);
    return 0;
}
