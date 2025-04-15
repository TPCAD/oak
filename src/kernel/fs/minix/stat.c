#include <oak/assert.h>
#include <oak/fs/minix.h>
#include <oak/fs/stat.h>
#include <oak/task.h>

static void copy_stat(inode_t *inode, stat_t *statbuf) {
    statbuf->dev = inode->dev;              // 文件所在的设备号
    statbuf->nr = inode->idx;               // 文件 i 节点号
    statbuf->mode = inode->inode->mode;     // 文件属性
    statbuf->nlinks = inode->inode->nlinks; // 文件的连接数
    statbuf->uid = inode->inode->uid;       // 文件的用户 id
    statbuf->gid = inode->inode->gid;       // 文件的组 id
    statbuf->rdev =
        inode->inode->zone[0]; // 设备号(如果文件是特殊的字符文件或块文件)
    statbuf->size =
        inode->inode->size;        // 文件大小（字节数）（如果文件是常规文件）
    statbuf->atime = inode->atime; // 最后访问时间
    statbuf->mtime = inode->inode->mtime; // 最后修改时间
    statbuf->ctime = inode->ctime;        // 最后节点修改时间
}

int file_stat(char *filename, stat_t *statbuf) {
    inode_t *inode = namei(filename);

    if (!inode) {
        return EOF;
    }

    copy_stat(inode, statbuf);
    inode_free(inode);
    return 0;
}

int file_fstat(fd_t fd, stat_t *statbuf) {

    if (fd >= TASK_FILE_NR) {
        return EOF;
    }
    task_t *curr_task = task_current_running();
    file_t *file = curr_task->files[fd];
    if (!file) {
        return EOF;
    }

    inode_t *inode = file->inode;
    assert(inode);

    copy_stat(inode, statbuf);
    return 0;
}
