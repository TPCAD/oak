#ifndef OAK_TASK_H
#define OAK_TASK_H

#include "oak/mm/dmm.h"
#include <oak/list.h>
#include <oak/types.h>

#define KERNEL_USER 0
#define NORMAL_USER 1000

#define TASK_NAME_LEN 16

typedef enum task_state_t {
    TASK_INIT,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_WAITING,
    TASK_DIED,
} task_state_t;

typedef struct task_t {
    u32 *statck_addr;   // 进程栈地址
    list_node_t node;   // 链表结点
    task_state_t state; // 进程状态
    u32 priority;
    u32 ticks;
    u32 jiffies;
    char name[TASK_NAME_LEN];
    u32 uid;
    // u32 gid;
    pid_t pid;
    pid_t ppid;
    u32 pde;
    heap_context_t user_heap;
    u32 magic; // 魔数
} task_t;

typedef struct task_frame_t {
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp;
    void (*eip)(void); // 函数指针
} task_frame_t;

// 特权级改变时的中断上下文
typedef struct intr_context_t {
    u32 vector;

    // pusha 入栈的寄存器
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 esp_dummy; // 因为 esp 会不断变化，所以 popa 会忽略压入的 esp
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;

    u32 gs;
    u32 fs;
    u32 es;
    u32 ds;

    u32 vector0;
    u32 error;

    // 中断入栈的寄存器
    u32 eip;
    u32 cs;
    u32 eflags;

    // 特权级改变时入栈 ss 和 esp
    u32 esp;
    u32 ss;
} intr_context_t;

typedef void *target_t;

void task_schedule();
task_t *task_current_running();

void task_yield();

void task_block(task_t *task, list_t *blist, task_state_t state);
void task_unblock(task_t *task);

void task_sleep(u32 ms);
void task_wakeup();

pid_t task_getpid();
pid_t task_getppid();

pid_t task_fork();

#endif // !OAK_TASK_H
