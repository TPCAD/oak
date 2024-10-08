#include <oak/assert.h>
#include <oak/debug.h>
#include <oak/device.h>
#include <oak/fifo.h>
#include <oak/fs.h>
#include <oak/stat.h>
#include <oak/stdio.h>
#include <oak/string.h>
#include <oak/syscall.h>
#include <oak/task.h>
#include <oak/types.h>

int pipe_read(inode_t *inode, char *buf, int count) {
    fifo_t *fifo = (fifo_t *)inode->desc;
    int nr = 0;
    while (nr < count) {
        if (fifo_empty(fifo)) {
            assert(inode->rxwaiter == NULL);
            inode->rxwaiter = running_task();
            task_block(inode->rxwaiter, NULL, TASK_BLOCKED);
        }
        buf[nr++] = fifo_get(fifo);
        if (inode->txwaiter) {
            task_unblock(inode->txwaiter);
            inode->txwaiter = NULL;
        }
    }
    return nr;
}

int pipe_write(inode_t *inode, char *buf, int count) {
    fifo_t *fifo = (fifo_t *)inode->desc;
    int nr = 0;
    while (nr < count) {
        if (fifo_full(fifo)) {
            assert(inode->txwaiter == NULL);
            inode->txwaiter = running_task();
            task_block(inode->txwaiter, NULL, TASK_BLOCKED);
        }
        fifo_put(fifo, buf[nr++]);
        if (inode->rxwaiter) {
            task_unblock(inode->rxwaiter);
            inode->rxwaiter = NULL;
        }
    }
    return nr;
}

int sys_pipe(fd_t pipefd[2]) {
    inode_t *inode = get_pipe_inode();

    task_t *task = running_task();
    file_t *files[2];

    pipefd[0] = task_get_fd(task);
    files[0] = task->files[pipefd[0]] = get_file();

    pipefd[1] = task_get_fd(task);
    files[1] = task->files[pipefd[1]] = get_file();

    files[0]->inode = inode;
    files[0]->flags = O_RDONLY;

    files[1]->inode = inode;
    files[1]->flags = O_WRONLY;

    return 0;
}
