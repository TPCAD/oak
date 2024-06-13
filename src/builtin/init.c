#include <oak/stdio.h>
#include <oak/syscall.h>
#include <oak/types.h>

int main() {
    if (get_pid() != 1) {
        printf("init already running...\n");
        return 0;
    }

    while (true) {
        u32 status;
        pid_t pid = fork();
        if (pid) {
            pid_t child = waitpid(pid, &status);
            printf("wait pid %d status %d %d\n", child, status, time());
        } else {
            int err = execve("/bin/ash.out", NULL, NULL);
            printf("execve /bin/ash.out error %d\n", err);
            exit(err);
        }
    }
    return 0;
}
