#include "oak/buffer.h"
#include "oak/debug/kassert.h"
#include "oak/fs/minix.h"
#include "oak/fs/stat.h"
#include "oak/types.h"
#include <oak/string.h>
#include <oak/syscall.h>

/**
 *  @brief  检查文件名是否与 dentry 中的文件名相等
 *  @param  name  路径
 *  @param  entry_name  dentry 中的文件名
 *  @param  next
 *  @return  相等则返回 1，不相等则返回 0
 *
 *  @a name 可能是一个含有目录的文件名，如 /home/alex/hello.txt。
 *  该函数只检查路径中的首个是否与 dentry 中的相等。
 */
static bool match_name(const char *name, const char *entry_name, char **next) {
    char *lhs = (char *)name;
    char *rhs = (char *)entry_name;
    while (*lhs == *rhs && *lhs != EOS && *rhs != EOS) {
        lhs++;
        rhs++;
    }
    // dentry_name 还未结束，不相等
    if (*rhs) {
        return false;
    }
    // name 还未结束，且不是分隔符，不相等
    if (*lhs && !IS_SEPARATOR(*lhs)) {
        return false;
    }
    // 均已结束，name 遇到分隔符，跳过分隔符
    if (IS_SEPARATOR(*lhs)) {
        lhs++;
    }
    // 去掉分隔符及其前面的内容
    *next = lhs;
    return true;
}

/**
 *  @brief  在 dir 目录下寻找 name
 *  @param  dir  inode 指针的指针
 *  @param  name  要寻找的文件/目录路径
 *  @param  next
 *  @param  result
 *  @return  找到的文件/目录的 buffer
 *
 *  查找 dir 目录下是否是 name 路径的一部分，若是则返回对应部分的 buffer，
 *  @a result 则返回对应的 dentry。
 *
 *  无法处理带根目录的路径，如 /dev/block。
 */
static buffer_t *find_entry(inode_t **dir, const char *name, char **next,
                            dentry_t **result) {
    // 保证 dir 是目录
    kassert(ISDIR((*dir)->inode->mode));

    // dir 的子目录数量（子项不一定目录）
    u32 entries = (*dir)->inode->size / sizeof(dentry_t);

    u32 block = 0; // zone 对应的块号
    buffer_t *buf = NULL;
    dentry_t *entry = NULL;

    for (u32 i = 0; i < entries; i++, entry++) {
        // buffer 为空（第一次循环）或当前 zone 块已经读完
        if (!buf || (u32)entry >= (u32)buf->data + BLOCK_SIZE) {
            buffer_release(buf);
            block = inode_calc_block((*dir), i / BLOCK_DENTRIES, false);
            kassert(block);

            // 读取下一个 zone
            buf = buffer_read((*dir)->dev, block);
            entry = (dentry_t *)buf->data;
        }
        if (match_name(name, entry->name, next)) {
            *result = entry;
            return buf;
        }
    }

    buffer_release(buf);
    return NULL;
}

static buffer_t *add_entry(inode_t *dir, const char *name, dentry_t **result) {
    char *next = NULL;

    buffer_t *buf = find_entry(&dir, name, &next, result);
    // @a name 已存在
    if (buf) {
        return buf;
    }

    // @a name 不能包括分隔符
    for (size_t i = 0; i < NAME_LEN; i++) {
        kassert(!IS_SEPARATOR(name[i]));
    }

    u32 i = 0;
    u32 block = 0;
    dentry_t *entry = NULL;

    for (; true; i++, entry++) {
        if (!buf || (u32)entry >= (u32)buf->data + BLOCK_SIZE) {
            buffer_release(buf);
            block = inode_calc_block(dir, i / BLOCK_DENTRIES, true);
            kassert(block);

            buf = buffer_read(dir->dev, block);
            entry = (dentry_t *)buf->data;
        }
        // 没有空 dentry，在末尾新建 dentry
        if (i * sizeof(dentry_t) >= dir->inode->size) {
            entry->nr = 0;
            dir->inode->size = (i + 1) * sizeof(dentry_t);
            dir->buf->dirty = true;
        }
        // 找一个空 dentry
        if (entry->nr)
            continue;

        // 构建 dentry
        strncpy(entry->name, name, NAME_LEN);
        buf->dirty = true;
        dir->inode->mtime = time();
        dir->buf->dirty = true;
        *result = entry;
        return buf;
    };
}

#include <oak/task.h>

void dir_test() {
    task_t *task = task_current_running();
    inode_t *inode = task->iroot;
    inode->count++;
    char *next = NULL;
    dentry_t *entry = NULL;
    buffer_t *buf = NULL;

    buf = find_entry(&inode, "hello.txt", &next, &entry);
    u32 nr = entry->nr;
    buffer_release(buf);

    buf = add_entry(inode, "world.txt", &entry);
    entry->nr = nr;

    inode_t *hello = inode_search(inode->dev, nr);
    hello->inode->nlinks++;
    hello->buf->dirty = true;

    inode_free(inode);
    inode_free(hello);
    buffer_release(buf);

    // char pathname[] = "/d1/d2/d3/d4";
    //
    // u32 dev = inode->dev;
    // char *name = pathname;
    // buf = find_entry(&inode, name, &next, &entry);
    // buffer_release(buf);
    //
    // inode_free(inode);
    // inode = inode_search(dev, entry->nr);
    //
    // name = next;
    // buf = find_entry(&inode, name, &next, &entry);
    // buffer_release(buf);
    //
    // inode_free(inode);
    // inode = inode_search(dev, entry->nr);
    //
    // name = next;
    // buf = find_entry(&inode, name, &next, &entry);
    // buffer_release(buf);
    //
    // inode_free(inode);
    // inode = inode_search(dev, entry->nr);
    //
    // name = next;
    // buf = find_entry(&inode, name, &next, &entry);
    // buffer_release(buf);
    // inode_free(inode);
}
