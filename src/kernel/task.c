#include "oak/cpu.h"
#include "oak/fs/minix.h"
#include "oak/fs/stat.h"
#include "oak/gdt.h"
#include "oak/list.h"
#include "oak/mm/memory.h"
#include "oak/mm/paging.h"
#include "oak/mm/pmm.h"
#include "oak/syscall.h"
#include "oak/types.h"
#include <oak/debug/kassert.h>
#include <oak/mm/vmm.h>
#include <oak/oak.h>
#include <oak/string.h>
#include <oak/task.h>

extern void task_switch(task_t *addr);
extern u32 jiffies; // 全局时间片
extern u32 jiffy;   // 时间片长度，单位 ms
extern tss_t tss;
extern void interrupt_exit();
extern file_t file_table[];

/* 记录系统运行中的任务页地址，最多存在 64 个任务 */
#define NR_TASKS 64
static task_t *task_table[NR_TASKS];

static list_t block_list; // 阻塞链表
static list_t sleep_list; // 睡眠链表

// 空闲任务。当系统没有可运行的任务时运行空闲任务，防止系统崩溃。
static task_t *idle_task = NULL;

/**
 *  @brief  从内核页中分配一页任务页
 *  @return  任务页地址
 *
 *  若 task_table 有空闲元素，从内核页中分配一页。页地址存入 task_table，对应的
 *  索引即位任务的 pid。若无空闲元素，内核 panic。
 */
static task_t *internal_alloc_task_page() {
    for (size_t i = 0; i < NR_TASKS; i++) {
        if (task_table[i] == NULL) {
            task_t *task = (task_t *)pmm_alloc_kpage();
            memset(task, 0, PAGE_SIZE);
            task->pid = i;
            task_table[i] = task;
            return task;
        }
    }
    kpanic("[task] No more tasks\n");
    return NULL; // no need
}

/**
 *  @brief  构建内核态任务
 *  @param  target  任务返回地址
 *  @param  name  任务名
 *  @param  priority  任务优先级
 *  @param  uid  用户 ID
 *  @return  任务页地址
 */
static task_t *internal_build_kernel_task(target_t target, const char *name,
                                          u32 priority, u32 uid) {
    task_t *task = internal_alloc_task_page();

    // 构建内核栈
    // 任务页的高地址作为内核栈栈底
    u32 stack = (u32)task + PAGE_SIZE - sizeof(task_frame_t);

    task_frame_t *frame = (task_frame_t *)stack;
    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (void *)target;

    strcpy((char *)task->name, name);

    task->statck_addr = (u32 *)stack;
    task->priority = priority;
    task->ticks = task->priority; // 优先级越大，初始时间片越多
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->gid = 0;
    task->pde = KERNEL_PAGE_DIR_ADDR;
    // TODO: 用户堆内存管理
    task->user_heap.start_addr = (void *)USER_EXEC_ADDR;
    task->user_heap.brk = (void *)USER_EXEC_ADDR;
    task->user_heap.max_addr = (void *)USER_EXEC_ADDR;
    // 用户程序段地址
    task->text = USER_EXEC_ADDR;
    task->data = USER_EXEC_ADDR;
    task->end = USER_EXEC_ADDR;
    // 任务关联 inode
    task->iexec = NULL;
    task->iroot = inode_get_root_inode();
    task->ipwd = inode_get_root_inode();
    task->iroot->count++;
    task->ipwd->count++;
    if (task->iexec) {
        task->iexec->count++;
    }
    // 任务工作目录字符串
    // TODO: 分配一页有些浪费，使用内核堆内存
    task->pwd = (void *)pmm_alloc_kpage();
    strcpy(task->pwd, "/");
    task->umask = 0022; // 对应 0755
    // 任务初始文件，标准输入输出
    task->files[STDIN_FILENO] = &file_table[STDIN_FILENO];
    task->files[STDOUT_FILENO] = &file_table[STDOUT_FILENO];
    task->files[STDERR_FILENO] = &file_table[STDERR_FILENO];
    task->files[STDIN_FILENO]->count++;
    task->files[STDOUT_FILENO]->count++;
    task->files[STDERR_FILENO]->count++;
    // 内核魔数，防止内核栈溢出
    task->magic = OAK_MAGIC;

    return task;
}

/**
 *  @brief  切换任务前修改 PDE 及保存内核栈地址
 *  @param  task  要切换到的任务页地址
 */
static void internal_pre_switch(task_t *task) {
    kassert(task->magic == OAK_MAGIC);
    // 切换 PDE
    if (task->pde != cpu_get_cr3()) {
        cpu_set_cr3(task->pde);
    }
    // 内核态切换至用户态时保存任务的内核栈到 TSS
    if (task->uid != KERNEL_USER) {
        // 每次切换都会清空内核栈
        tss.esp0 = (u32)task + PAGE_SIZE;
    }
}

/**
 *  @brief  搜索指定状态的任务
 *  @param  state  任务状态
 *  @return  任务页地址
 *
 *  从 task_table 中搜索指定状态的运行时间最短或剩余时间片最少的任务，
 *  不包括当前任务
 */
static task_t *internal_find_stated_task(task_state_t state) {
    kassert(!cpu_get_intr_state());

    task_t *task = NULL;
    task_t *curr_task = task_current_running();

    for (size_t i = 0; i < NR_TASKS; i++) {
        task_t *ptr = task_table[i];

        if (ptr == NULL) {
            continue;
        }
        if (ptr->state != state) {
            continue;
        }
        if (ptr == curr_task) {
            continue;
        }
        if (task == NULL || ptr->jiffies < task->jiffies) {
            task = ptr;
        }
    }

    if (task == NULL && state == TASK_READY) {
        task = idle_task;
    }

    return task;
}

/**
 *  @brief  进入用户态
 *  @param  target  返回地址
 *
 *  将内核态任务切换至用户态任务
 */
void internal_enter_user_mode(target_t target) {
    task_t *curr_task = task_current_running();

    // 拷贝并修改 PDE
    curr_task->pde = (u32)paging_copy_pde();
    cpu_set_cr3(curr_task->pde);

    // 在内核栈中构建中断上下文（带特权级转换）
    u32 kstack_addr = (u32)curr_task + PAGE_SIZE - sizeof(intr_context_t);
    intr_context_t *intr_context = (intr_context_t *)kstack_addr;

    intr_context->vector = 0x20;
    intr_context->edi = 1;
    intr_context->esi = 2;
    intr_context->ebp = 3;
    intr_context->esp_dummy = 4;
    intr_context->ebx = 5;
    intr_context->edx = 6;
    intr_context->ecx = 7;
    intr_context->eax = 8;

    intr_context->gs = 0;
    intr_context->fs = USER_DATA_SELECTOR;
    intr_context->es = USER_DATA_SELECTOR;
    intr_context->ds = USER_DATA_SELECTOR;

    intr_context->vector0 = 0x20;
    intr_context->error = OAK_MAGIC;

    intr_context->eip = (u32)target;
    intr_context->cs = USER_CODE_SELECTOR;
    intr_context->eflags = (0 << 12 | 0b10 | 1 << 9);
    intr_context->esp = USER_STACK_BOTTOM;
    intr_context->ss = USER_DATA_SELECTOR;

    asm volatile("movl %0, %%esp\n"
                 "jmp interrupt_exit\n" ::"m"(intr_context));
}

/**
 *  @brief  获取当前运行的任务
 *  @return  当前运行的任务页地址
 *
 *  执行该函数时必定是内核态，且 ESP 必定是当前任务页地址，因此只要获取 ESP 即可
 *  获得当前任务的 task_t 结构。
 */
task_t *task_current_running() {
    asm volatile("movl %esp, %eax\n"
                 "andl $0xfffff000, %eax\n");
}

/**
 *  @brief  任务调度
 */
void task_schedule() {
    kassert(!cpu_get_intr_state());
    task_t *curr_task = task_current_running();
    task_t *found_task = internal_find_stated_task(TASK_READY);

    kassert(found_task != NULL);
    kassert(found_task->magic == OAK_MAGIC);

    if (curr_task->state == TASK_RUNNING) {
        curr_task->state = TASK_READY;
    }

    found_task->state = TASK_RUNNING;
    if (found_task == curr_task) {
        return;
    }

    internal_pre_switch(found_task);
    task_switch(found_task);
}

/**
 *  @brief  任务主动进行调度
 *
 *  对 `task_schedule` 的包装，会被用作系统调用 `yield`
 */
void task_yield() { task_schedule(); }

/**
 *  @brief  阻塞一个任务
 *  @param  task  要阻塞的任务
 *  @param  blist  阻塞链表
 *  @param  state  阻塞状态
 *
 *  将任务更改为指定状态，并加入指定队列。若阻塞任务是当前运行任务则进行调度
 */
void task_block(task_t *task, list_t *blist, task_state_t state) {
    kassert(!cpu_get_intr_state());

    // 要阻塞的任务不能是已阻塞的任务
    kassert(task->node.next == NULL);
    kassert(task->node.prev == NULL);

    if (blist == NULL) {
        blist = &block_list;
    }

    list_push(blist, &task->node);

    // TODO: 细分阻塞状态
    kassert(state != TASK_READY && state != TASK_RUNNING);

    task->state = state;

    task_t *curr_task = task_current_running();
    if (task == curr_task) {
        task_schedule();
    }
}

/**
 *  @brief  将一个任务解除阻塞
 *  @param  task  要解除阻塞的任务
 *
 *  将任务从队列中删除并变为就绪状态
 */
void task_unblock(task_t *task) {
    kassert(!cpu_get_intr_state());

    list_remove(&task->node);

    kassert(task->node.next == NULL);
    kassert(task->node.prev == NULL);

    task->state = TASK_READY;
}

/**
 *  @brief  使当前任务睡眠
 *  @param  ms  睡眠的毫秒数
 */
void task_sleep(u32 ms) {
    kassert(!cpu_get_intr_state());

    // 需要多少个时间片
    u32 ticks = ms / jiffy;
    // 最小为 1
    ticks = ticks > 0 ? ticks : 1;

    task_t *curr_task = task_current_running();
    curr_task->ticks = jiffies + ticks;

    list_insert_sort(&sleep_list, &curr_task->node,
                     element_node_offset(task_t, node, ticks));

    curr_task->state = TASK_SLEEPING;

    task_schedule();
}

/**
 *  @brief  唤醒所有合适的任务
 *
 *  从睡眠链表中唤醒所有时间片小于当前全局时间片的任务
 */
void task_wakeup() {
    kassert(!cpu_get_intr_state());

    for (list_node_t *ptr = sleep_list.head.next; ptr != &sleep_list.tail;) {
        task_t *task = element_entry(task_t, node, ptr);
        if (task->ticks > jiffies) {
            break;
        }

        // 提前修改 ptr，因为 task_unblock 会将指针清空
        ptr = ptr->next;

        task->ticks = 0;
        task_unblock(task);
    }
}

/**
 *  @brief  获取任务 ID
 *  @return  任务 ID
 */
pid_t task_getpid() {
    task_t *curr_task = task_current_running();
    return curr_task->pid;
}

/**
 *  @brief  获取父任务 ID
 *  @return  任务 ID
 */
pid_t task_getppid() {
    task_t *curr_task = task_current_running();
    return curr_task->ppid;
}

/**
 *  @brief  创建子进程
 *  @return  父进程返回子进程 PID，子进程返回 0
 */
pid_t task_fork() {
    task_t *curr_task = task_current_running();

    // 当前进程必须是正在运行的进程
    kassert(curr_task->node.next == NULL && curr_task->node.prev == NULL &&
            curr_task->state == TASK_RUNNING);

    task_t *child_task = internal_alloc_task_page();
    pid_t child_pid = child_task->pid;

    // 拷贝当前任务到新任务
    memcpy(child_task, curr_task, PAGE_SIZE);

    child_task->pid = child_pid;
    child_task->ppid = curr_task->pid;
    child_task->ticks = child_task->priority;
    child_task->state = TASK_READY;

    // 拷贝页目录
    child_task->pde = (u32)paging_copy_pde();

    child_task->pwd = (char *)pmm_alloc_kpage();
    strncpy(child_task->pwd, curr_task->pwd, PAGE_SIZE);

    curr_task->ipwd->count++;
    curr_task->iroot->count++;

    // 文件引用加 1
    for (size_t i = 0; i < TASK_FILE_NR; i++) {
        file_t *file = child_task->files[i];
        if (file) {
            file->count++;
        }
    }

    // 构造子进程内核栈
    u32 addr = (u32)child_task + PAGE_SIZE;
    addr -= sizeof(intr_context_t);
    intr_context_t *intr_cont = (intr_context_t *)addr;
    intr_cont->eax = 0; // 子进程的返回值

    addr -= sizeof(task_frame_t);
    task_frame_t *tframe = (task_frame_t *)addr;
    tframe->ebp = 0xaa55aa55;
    tframe->ebx = 0xaa55aa55;
    tframe->esi = 0xaa55aa55;
    tframe->edi = 0xaa55aa55;

    tframe->eip = interrupt_exit;

    child_task->statck_addr = (u32 *)tframe;

    // 父进程返回值
    return child_task->pid;
}

/**
 *  @brief  退出进程
 *  @param  status  进程状态码
 */
void task_exit(int status) {
    task_t *curr_task = task_current_running();

    // 当前进程必须是正在运行的进程
    kassert(curr_task->node.next == NULL && curr_task->node.prev == NULL &&
            curr_task->state == TASK_RUNNING);

    curr_task->state = TASK_DIED;
    curr_task->status = status;

    paging_free_pde();

    pmm_free_kpage((void *)curr_task->pwd);
    inode_free(curr_task->ipwd);
    inode_free(curr_task->iroot);
    inode_free(curr_task->iexec);

    // 关闭文件
    for (size_t i = 0; i < TASK_FILE_NR; i++) {
        file_t *file = curr_task->files[i];
        if (file) {
            close(i);
        }
    }

    // 将子进程交给父进程
    for (size_t i = 2; i < NR_TASKS; i++) {
        task_t *child_task = task_table[i];
        if (!child_task) {
            continue;
        }
        if (child_task->ppid != curr_task->pid) {
            continue;
        }
        child_task->ppid = curr_task->ppid;
    }

    // 唤醒正在等待子进程退出的父进程
    task_t *parent_task = task_table[curr_task->ppid];
    if (parent_task->state == TASK_WAITING &&
        (parent_task->waitpid == -1 ||
         parent_task->waitpid == curr_task->pid)) {
        task_unblock(parent_task);
    }

    task_schedule();
}

/**
 *  @brief  等待指定子进程退出
 *  @param  pid  子进程 ID
 *  @param  status  子进程退出状态码指针
 *  @return  return
 */
pid_t task_waitpid(pid_t pid, i32 *status) {
    task_t *curr_task = task_current_running();
    task_t *child_task = NULL;

    while (true) {
        bool found_child = false;
        for (size_t i = 2; i < NR_TASKS; i++) {
            task_t *ptr = task_table[i];
            if (!ptr) {
                continue;
            }
            if (ptr->ppid != curr_task->pid) {
                continue;
            }
            if (ptr->pid != pid && pid != -1) {
                continue;
            }

            if (ptr->state == TASK_DIED) {
                child_task = ptr;
                task_table[i] = NULL;
                goto rollback;
            }

            found_child = true;
        }
        if (found_child) {
            curr_task->waitpid = pid;
            task_block(curr_task, NULL, TASK_WAITING);
            continue;
        }
        break;
    }

    // 没有符合的子进程
    return -1;

rollback:
    *status = child_task->status;
    u32 ret = child_task->pid;
    pmm_free_kpage((void *)child_task->pde);
    pmm_free_kpage((void *)child_task);
    return ret;
}

/**
 *  @brief  分配可用文件描述符
 *  @param  task 任务页地址
 *  @return  文件描述符
 */
fd_t task_alloc_fd(task_t *task) {
    fd_t i;
    for (i = 3; i < TASK_FILE_NR; i++) {
        if (!task->files[i])
            break;
    }
    if (i == TASK_FILE_NR) {
        kpanic("[task] Exceed task max open files.");
    }
    return i;
}
/**
 *  @brief  释放进程文件描述符
 *  @param  fd  文件描述符
 */
void task_free_fd(task_t *task, fd_t fd) {
    kassert(fd < TASK_FILE_NR);
    task->files[fd] = NULL;
}

/**
 *  @brief  获取进程当前工作目录字符串
 */
char *task_getcwd(char *buf, size_t size) {
    task_t *curr_task = task_current_running();
    strncpy(buf, curr_task->pwd, size);
    buf[strlen(curr_task->pwd)] = '\0';
    return buf;
}

extern void abspath(char *pwd, const char *pathname);
/**
 *  @brief  更改进程工作目录
 *  @param  pathname  目录路径
 *  @return  return
 */
int task_chdir(char *pathname) {
    task_t *curr_task = task_current_running();
    inode_t *inode = namei(pathname);
    if (!inode)
        goto rollback;
    if (!ISDIR(inode->inode->mode) || inode == curr_task->ipwd)
        goto rollback;

    abspath(curr_task->pwd, pathname);

    inode_free(curr_task->ipwd);
    curr_task->ipwd = inode;
    return 0;

rollback:
    inode_free(inode);
    return EOF;
}

/**
 *  @brief  更改进程根目录
 *  @param  pathname  目录路径
 *  @return  return
 */
int task_chroot(char *pathname) {
    task_t *curr_task = task_current_running();
    inode_t *inode = namei(pathname);

    if (!inode)
        goto rollback;
    if (!ISDIR(inode->inode->mode) || inode == curr_task->iroot)
        goto rollback;

    inode_free(curr_task->iroot);
    curr_task->iroot = inode;
    return 0;

rollback:
    inode_free(inode);
    return EOF;
}

extern void idle_thread();
extern void init_thread();
extern u32 test_thread();

/**
 *  @brief  初始化阻塞队列、任务表，构建临时内核任务，创建主要进程
 */
void task_init() {

    /**
     * bootloader 将内核加载到 0x10000，并把 esp 设为 0x10000。此时并没有真正意
     * 义上的任务。为了能正确切换到其他任务，需要构建一个临时的内核任务。因为当
     * 前栈栈底地址是 0x10000，所以将 0xf000 作为临时任务页。临时任务只有两个有
     * 效字段 task_t.ticks 和 task_t.magic。前者设为 1，适配时钟中断处理函数。
     * 后者是内核魔数，用于防止内核栈溢出。
     *
     * 因为 task_table 没有记录临时任务，所以在切换到其他任务后就不会再被执行
     */
    task_t *temp_kernel_task = task_current_running();
    temp_kernel_task->magic = OAK_MAGIC;
    temp_kernel_task->ticks = 1;

    list_init(&block_list);
    list_init(&sleep_list);

    memset(task_table, 0, sizeof(task_table));

    idle_task = internal_build_kernel_task(idle_thread, "idle", 1, KERNEL_USER);
    internal_build_kernel_task(init_thread, "init", 5, NORMAL_USER);
    internal_build_kernel_task(test_thread, "testA", 5, NORMAL_USER);
    internal_build_kernel_task(test_thread, "testB", 15, NORMAL_USER);
    internal_build_kernel_task(test_thread, "testC", 25, NORMAL_USER);
    // build_basic_task(thread_b, "testB", 5, NORMAL_USER);
    // build_basic_task(thread_c, "testC", 5, NORMAL_USER);
}
