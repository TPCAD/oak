#include <oak/debug/kassert.h>
#include <oak/fs/minix.h>
#include <oak/task.h>
#include <oak/types.h>

#define FILE_NR 128

// 系统当前打开的所有文件，前三个文件是标准输入，标准输出，标准错误
file_t file_table[FILE_NR];

/**
 *  @brief  从 file_table 获取一个空闲 file_t
 *  @return  file_t 指针
 */
file_t *file_search_table() {
    for (size_t i = 3; i < FILE_NR; i++) {
        file_t *file = &file_table[i];
        if (!file->count) {
            file->count++;
            return file;
        }
    }
    kpanic("[fs] Exceed max open files...");
    return NULL; // no need
}

/**
 *  @brief  释放一个 file_t
 *  @param  file  要释放的 file_t
 */
void file_free_table(file_t *file) {
    kassert(file->count > 0);
    file->count--;
    if (!file->count) {
        inode_free(file->inode);
    }
}

void file_init() {
    for (size_t i = 0; i < FILE_NR; i++) {
        file_t *file = &file_table[i];
        file->mode = 0;
        file->count = 0;
        file->flags = 0;
        file->offset = 0;
        file->inode = NULL;
    }
}
