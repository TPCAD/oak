#ifndef OAK_TASK_H
#define OAK_TASK_H

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
    u32 pid;
    // u32 ppid;
    u32 pde;
    u32 magic; // 魔数
} task_t;

typedef struct task_frame_t {
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp;
    void (*eip)(void); // 函数指针
} task_frame_t;

typedef void *target_t;

void task_schedule();
task_t *task_current_running();

void task_yield();

void task_block(task_t *task, list_t *blist, task_state_t state);
void task_unblock(task_t *task);

void task_sleep(u32 ms);
void task_wakeup();

#endif // !OAK_TASK_H
