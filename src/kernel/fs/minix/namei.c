#include "oak/buffer.h"
#include "oak/debug/kassert.h"
#include "oak/debug/kdebug.h"
#include "oak/fs/minix.h"
#include "oak/fs/stat.h"
#include "oak/mm/memory.h"
#include "oak/task.h"
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

#define P_EXEC IXOTH
#define P_READ IROTH
#define P_WRITE IWOTH

static bool permission(inode_t *inode, u16 mask) {
    u16 mode = inode->inode->mode;
    // 硬链接为 0，文件已被删除
    if (!inode->inode->nlinks)
        return false;

    // root 用户
    task_t *curr_task = task_current_running();
    if (curr_task->uid == KERNEL_USER)
        return true;

    if (curr_task->uid == inode->inode->uid) {
        mode >>= 6; // 文件拥有者
    } else if (curr_task->gid == inode->inode->gid) {
        mode >>= 3; // 相同用户组
    }

    // 比较权限
    if ((mode & mask & 0b111) == mask) {
        return true;
    }
    return false;
}

/**
 *  @brief  获取路径第一个分隔符
 *  @param  str  路径
 *  @return  指向第一个分隔符的指针
 */
static char *strsep(const char *str) {
    char *ptr = (char *)str;
    while (true) {
        if (IS_SEPARATOR(*ptr)) {
            return ptr;
        }
        if (*ptr++ == EOS) {
            return NULL;
        }
    }
}

/**
 *  @brief  获取路径最后一个分隔符
 *  @param  str  路径
 *  @return  指向最后一个分隔符的指针
 */
static char *strrsep(const char *str) {
    char *last = NULL;
    char *ptr = (char *)str;
    while (true) {
        if (IS_SEPARATOR(*ptr)) {
            last = ptr;
        }
        // 检验是否是最后一个分隔符
        if (*ptr++ == EOS) {
            return last;
        }
    }
}

/**
 *  @brief  获取路径父目录的 inode
 *  @param  pathname  路径
 *  @param  next
 *  @return  父目录 inode
 */
inode_t *named(char *pathname, char **next) {
    inode_t *inode = NULL;
    task_t *curr_task = task_current_running();
    char *left = pathname;

    // 跳过根目录
    if (IS_SEPARATOR(left[0])) {
        inode = curr_task->iroot;
        left++;
    } else if (left[0]) {
        inode = curr_task->ipwd;
    } else {
        return NULL;
    }

    inode->count++;
    *next = left;

    // 路径只有一层，根目录或进程工作目录
    if (!*left) {
        return inode;
    }

    char *right = strrsep(left);
    // 已经没有一个分隔符，当前 inode 就是父目录 inode
    // right < left ?
    if (!right || right < left) {
        return inode;
    }

    // basename
    right++;
    *next = left;
    dentry_t *entry = NULL;
    buffer_t *buf = NULL;
    while (true) {
        buffer_release(buf);

        // 逐层寻找目录
        buf = find_entry(&inode, left, next, &entry);
        if (!buf) {
            goto failure;
        }

        u32 dev = inode->dev;
        inode_free(inode);
        inode = inode_search(dev, entry->nr);
        // 不是目录或没有执行权限
        if (!ISDIR(inode->inode->mode) || !permission(inode, P_EXEC)) {
            goto failure;
        }
        // 找到最后一层
        if (right == *next) {
            goto success;
        }
        left = *next;
    }

success:
    buffer_release(buf);
    return inode;

failure:
    buffer_release(buf);
    inode_free(inode);
    return NULL;
}

/**
 *  @brief  获取路径的 inode
 *  @param  pathname  路径
 *  @return  路径 inode
 */
inode_t *namei(char *pathname) {
    char *next = NULL;
    // 寻找父目录 inode
    inode_t *dir = named(pathname, &next);
    if (!dir) {
        return NULL;
    }
    // pathname 是目录路径
    if (!(*next)) {
        return dir;
    }

    char *name = next;
    dentry_t *entry = NULL;
    // 在父目录 inode 中寻找文件
    buffer_t *buf = find_entry(&dir, name, &next, &entry);
    // 文件不存在
    if (!buf) {
        inode_free(dir);
        return NULL;
    }

    inode_t *inode = inode_search(dir->dev, entry->nr);
    inode_free(dir);
    return inode;
}

#include "oak/mm/pmm.h"

void dir_test() {
    inode_t *inode = namei("/home/../hello.txt");

    char *buf = (char *)pmm_alloc_kpage();
    int i = inode_read(inode, buf, 1024, 0);

    KDEBUG("content: %s\n", buf);

    memset(buf, 'A', PAGE_SIZE);
    inode_write(inode, buf, PAGE_SIZE, 0);
    KDEBUG("write 1024 bytes\n");

    memset(buf, 'B', PAGE_SIZE);
    inode_write(inode, buf, PAGE_SIZE, PAGE_SIZE);
    KDEBUG("write 1024 bytes\n");
}
