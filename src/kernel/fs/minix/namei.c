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
 *  @return  dir 目录包含该文件的逻辑块 buffer
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

    if (match_name(name, "..", next) && (*dir)->idx == 1) {
        super_block_t *sb = super_search_by_devnum((*dir)->dev);
        inode_t *inode = *dir;
        (*dir) = sb->imount;
        (*dir)->count++;
        inode_free(inode);
    }

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
    for (size_t i = 0; i < strlen(name); i++) {
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

bool permission(inode_t *inode, u16 mask) {
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
char *strsep(const char *str) {
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
char *strrsep(const char *str) {
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
 *  @brief  计算 @a pathname 的绝对路径
 *  @return  return
 */
void abspath(char *pwd, const char *pathname) {
    char *cur = NULL;
    char *ptr = NULL;
    if (IS_SEPARATOR(pathname[0])) {
        cur = pwd + 1;
        *cur = 0;
        pathname++;
    } else {
        cur = strrsep(pwd) + 1;
        *cur = 0;
    }

    while (pathname[0]) {
        ptr = strsep(pathname);
        if (!ptr) {
            break;
        }

        int len = (ptr - pathname) + 1;
        *ptr = '/';
        if (!memcmp(pathname, "./", 2)) {
            /* code */
        } else if (!memcmp(pathname, "../", 3)) {
            if (cur - 1 != pwd) {
                *(cur - 1) = 0;
                cur = strrsep(pwd) + 1;
                *cur = 0;
            }
        } else {
            strncpy(cur, pathname, len + 1);
            cur += len;
        }
        pathname += len;
    }

    if (!pathname[0])
        return;

    if (!strcmp(pathname, "."))
        return;

    if (strcmp(pathname, "..")) {
        strcpy(cur, pathname);
        cur += strlen(pathname);
        *cur = '/';
        *(cur + 1) = '\0';
        return;
    }
    if (cur - 1 != pwd) {
        *(cur - 1) = 0;
        cur = strrsep(pwd) + 1;
        *cur = 0;
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

/**
 *  @brief  创建目录
 *  @param  pathname  目录路径
 *  @param  mode  目录属性
 */
int dentry_create(char *pathname, int mode) {
    char *next = NULL;
    buffer_t *entry_buf = NULL;
    inode_t *dir = named(pathname, &next);

    // 父目录不存在
    if (!dir)
        goto rollback;

    // 目录名为空
    if (!*next)
        goto rollback;

    // 父目录无写权限
    if (!permission(dir, P_WRITE))
        goto rollback;

    char *name = next;
    dentry_t *entry;

    // 在父目录 inode 寻找目录
    entry_buf = find_entry(&dir, name, &next, &entry);
    // 目录项已存在
    if (entry_buf)
        goto rollback;

    // 创建新目录 dentry
    entry_buf = add_entry(dir, name, &entry);
    entry_buf->dirty = true;
    entry->nr = inode_alloc_bit(dir->dev);

    // 创建新目录的 inode
    task_t *curr_task = task_current_running();
    inode_t *inode = build_inode(dir->dev, entry->nr);

    inode->inode->mode = (mode & 0777 & ~curr_task->umask) | IFDIR;
    inode->inode->size = sizeof(dentry_t) * 2; // '.' and '..'
    inode->inode->nlinks = 2;                  // '.' and self

    dir->buf->dirty = true;
    dir->inode->nlinks++; // '..'

    // 为新目录写入默认子目录。'.' 和 '..'。
    buffer_t *zone_buf =
        buffer_read(inode->dev, inode_calc_block(inode, 0, true));
    zone_buf->dirty = true;
    entry = (dentry_t *)zone_buf->data;
    strcpy(entry->name, ".");
    entry->nr = inode->idx;

    entry++;
    strcpy(entry->name, "..");
    entry->nr = dir->idx;

    inode_free(inode);
    inode_free(dir);

    buffer_release(entry_buf);
    buffer_release(zone_buf);
    return 0;

rollback:
    buffer_release(entry_buf);
    inode_free(dir);
    return -1;
}

/**
 *  @brief  检查目录是否为空
 *  @param  inode  目录 inode
 *  @return  为空则返回 1，不为空则返回 0
 */
static bool is_empty(inode_t *inode) {
    kassert(ISDIR(inode->inode->mode));

    // 空目录只有两个 dentry
    int entries = inode->inode->size / sizeof(dentry_t);
    if (entries < 2 || !inode->inode->zone[0]) {
        KDEBUG("bad directory on dev %d\n", inode->dev);
        return false;
    }

    u32 i = 0;
    u32 block = 0;
    buffer_t *buf = NULL;
    dentry_t *entry;
    int count = 0;

    for (; i < entries; i++, entry++) {
        if (!buf || (u32)entry >= (u32)buf->data + BLOCK_SIZE) {
            buffer_release(buf);
            block = inode_calc_block(inode, i / BLOCK_DENTRIES, false);
            kassert(block);

            buf = buffer_read(inode->dev, block);
            entry = (dentry_t *)buf->data;
        }
        if (entry->nr)
            count++;
    };

    buffer_release(buf);

    if (count < 2) {
        KDEBUG("bad directory on dev %d\n", inode->dev);
        return false;
    }

    return count == 2;
}

/**
 *  @brief  删除目录
 *  @param  pathname  目录路径
 *  @return  删除成功返回 0，删除失败返回 -1
 */
int dentry_remove(char *pathname) {
    char *next = NULL;
    buffer_t *entry_buf = NULL;
    inode_t *dir = named(pathname, &next);
    inode_t *inode = NULL;
    int ret = EOF;

    // 父目录不存在
    if (!dir)
        goto rollback;

    // 目录名为空
    if (!*next)
        goto rollback;

    // 父目录无写权限
    if (!permission(dir, P_WRITE))
        goto rollback;

    char *name = next;
    dentry_t *entry;

    // 在父目录 inode 寻找待删除的目录
    entry_buf = find_entry(&dir, name, &next, &entry);
    // 目录项不存在
    if (!entry_buf)
        goto rollback;

    inode = inode_search(dir->dev, entry->nr);
    // FIX: No need. Remove this.
    if (!inode)
        goto rollback;

    // FIX: No need. Remove this.
    if (inode == dir)
        goto rollback;

    // 非目录
    if (!ISDIR(inode->inode->mode))
        goto rollback;

    task_t *curr_task = task_current_running();
    // 无删除权限或不是目录拥有者
    if ((dir->inode->mode & ISVTX) && curr_task->uid != inode->inode->uid)
        goto rollback;

    // 当前 inode 还有引用
    if (dir->dev != inode->dev || inode->count > 1)
        goto rollback;

    // 确保要删除的目录是空目录
    if (!is_empty(inode))
        goto rollback;
    kassert(inode->inode->nlinks == 2);

    // 删除 inode 数据块，位图对应位
    inode_truncate(inode);
    inode_free_bit(inode->dev, inode->idx);

    inode->inode->nlinks = 0;
    inode->buf->dirty = true;
    inode->idx = 0;

    dir->inode->nlinks--;
    dir->ctime = dir->atime = dir->inode->mtime = time();
    dir->buf->dirty = true;
    kassert(dir->inode->nlinks > 0);

    entry->nr = 0;
    entry_buf->dirty = true;

    ret = 0;

rollback:
    inode_free(inode);
    inode_free(dir);
    buffer_release(entry_buf);
    return ret;
}

/**
 *  @brief  创建文件硬链接
 *  @param  oldname  原文件路径
 *  @param  newname  新文件路径
 *  @return  return
 */
int file_link(char *oldname, char *newname) {
    int ret = EOF;
    buffer_t *buf = NULL;
    inode_t *dir = NULL;
    inode_t *inode = namei(oldname);
    // 文件不存在
    if (!inode)
        goto rollback;

    // 不支持链接目录
    if (ISDIR(inode->inode->mode))
        goto rollback;

    char *next = NULL;
    dir = named(newname, &next);
    // 父目录不存在
    if (!dir)
        goto rollback;

    // 文件名为空
    if (!(*next))
        goto rollback;

    if (dir->dev != inode->dev)
        goto rollback;

    // 父目录无写权限
    if (!permission(dir, P_WRITE))
        goto rollback;

    char *name = next;
    dentry_t *entry;

    // 在父目录 inode 寻找文件
    buf = find_entry(&dir, name, &next, &entry);
    if (buf) // 文件已存在
        goto rollback;

    // 创建新文件 dentry
    buf = add_entry(dir, name, &entry);
    entry->nr = inode->idx;
    buf->dirty = true;

    // inode 链接加 1
    inode->inode->nlinks++;
    inode->ctime = time();
    inode->buf->dirty = true;
    ret = 0;

rollback:
    buffer_release(buf);
    inode_free(inode);
    inode_free(dir);
    return ret;
}

/**
 *  @brief  删除文件硬链接
 *  @param  filename  文件路径
 *  @return  return
 */
int file_unlink(char *filename) {
    int ret = EOF;
    char *next = NULL;
    inode_t *inode = NULL;
    buffer_t *buf = NULL;
    inode_t *dir = named(filename, &next);
    // 父目录不存在
    if (!dir)
        goto rollback;

    // 目录名为空
    if (!(*next))
        goto rollback;

    // 父目录无写权限
    if (!permission(dir, P_WRITE))
        goto rollback;

    char *name = next;
    dentry_t *entry;
    buf = find_entry(&dir, name, &next, &entry);
    if (!buf) // 目录项不存在
        goto rollback;

    inode = inode_search(dir->dev, entry->nr);
    if (ISDIR(inode->inode->mode))
        goto rollback;

    task_t *curr_task = task_current_running();
    // 无删除权限或不是文件拥有者
    if ((inode->inode->mode & ISVTX) && curr_task->uid != inode->inode->uid)
        goto rollback;

    // 待删除文件不存在
    if (!inode->inode->nlinks) {
        KDEBUG("deleting non exists file (%04x:%d)\n", inode->dev, inode->idx);
    }

    // 删除 dentry
    entry->nr = 0;
    buf->dirty = true;

    // 减少硬链接
    inode->inode->nlinks--;
    inode->buf->dirty = true;

    // 硬链接为 0，删除 inode
    if (inode->inode->nlinks == 0) {
        inode_truncate(inode);
        inode_free_bit(inode->dev, inode->idx);
    }

    ret = 0;

rollback:
    buffer_release(buf);
    inode_free(inode);
    inode_free(dir);
    return ret;
}

/**
 *  @brief  打开 inode
 *  @param  pathname  路径
 *  @param  flag  标志
 *  @param  mode  创建文件的 mode
 *  @return  要打开的 inode
 *
 *  获取对应 inode，并根据标志进行特殊处理
 */
inode_t *inode_open(char *pathname, int flag, int mode) {
    inode_t *dir = NULL;
    inode_t *inode = NULL;
    buffer_t *buf = NULL;
    dentry_t *entry = NULL;
    char *next = NULL;
    // 父目录 inode
    dir = named(pathname, &next);
    // 父目录不存在
    if (!dir)
        goto rollback;
    // 文件名为空
    if (!*next)
        return dir;

    if ((flag & O_TRUNC) && ((flag & O_ACCMODE) == O_RDONLY))
        flag |= O_RDWR;

    char *name = next;
    // 寻找文件 dentry
    buf = find_entry(&dir, name, &next, &entry);
    // 文件 dentry 存在，获取 inode
    if (buf) {
        inode = inode_search(dir->dev, entry->nr);
        goto makeup;
    }

    // 文件不存在，且不创建
    if (!(flag & O_CREAT))
        goto rollback;
    // 父目录无写权限
    if (!permission(dir, P_WRITE))
        goto rollback;

    // 文件不存在，创建新 dentry
    buf = add_entry(dir, name, &entry);
    entry->nr = inode_alloc_bit(dir->dev);
    inode = build_inode(dir->dev, entry->nr);

    task_t *curr_task = task_current_running();

    mode &= (0777 & ~curr_task->umask);
    mode |= IFREG;
    inode->inode->mode = mode;

makeup:
    if (!permission(inode, flag & O_ACCMODE)) {
        goto rollback;
    }
    // inode 是文件或权限不足
    if (ISDIR(inode->inode->mode) && ((flag & O_ACCMODE) != O_RDONLY)) {
        goto rollback;
    }

    inode->atime = time();

    // 有截断标志，释放所有数据块
    if (flag & O_TRUNC)
        inode_truncate(inode);

    buffer_release(buf);
    inode_free(dir);
    return inode;

rollback:
    buffer_release(buf);
    inode_free(dir);
    inode_free(inode);
    return NULL;
}

int inode_build_devfile_node(char *filename, int mode, int dev) {
    char *next = NULL;
    inode_t *dir = NULL;
    buffer_t *buf = NULL;
    inode_t *inode = NULL;
    int ret = EOF;

    dir = named(filename, &next);
    if (!dir)
        goto rollback;

    if (!(*next))
        goto rollback;

    if (!permission(dir, P_WRITE))
        goto rollback;

    char *name = next;
    dentry_t *entry;
    buf = find_entry(&dir, name, &next, &entry);
    if (buf) // 目录项存在
        goto rollback;

    buf = add_entry(dir, name, &entry);
    buf->dirty = true;
    entry->nr = inode_alloc_bit(dir->dev);

    inode = build_inode(dir->dev, entry->nr);

    inode->inode->mode = mode;
    if (ISBLK(mode) || ISCHR(mode))
        inode->inode->zone[0] = dev;

    ret = 0;

rollback:
    buffer_release(buf);
    inode_free(inode);
    inode_free(dir);
    return ret;
}

#include "oak/mm/pmm.h"

void dir_test() {
    inode_t *inode = namei("/home/../hello.txt");
    inode_truncate(inode);
    inode_free(inode);
    KDEBUG("delete inode\n");
}
