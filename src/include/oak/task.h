#ifndef OAK_TASK_H
#define OAK_TASK_H

#include <oak/types.h>

#define KERNEL_USER 0
#define NORMAL_USER 1

#define TASK_NAME_LEN 16

typedef u32 target_t();

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
    u32 *stack;               // kernel stack
    task_state_t state;       // task status
    u32 priority;             // priority
    u32 ticks;                // left jiffies
    u32 jiffies;              // jiffies last ran
    char name[TASK_NAME_LEN]; // task name
    u32 uid;                  // user id
    u32 pde;                  // pde
    struct bitmap_t *vmap;    // virtual memory map
    u32 magic;                // magic number
} task_t;

// ABI
typedef struct task_frame_t {
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp;
    void (*eip)(void);
} task_frame_t;

task_t *running_task();
void schedule();

#endif // !OAK_TASK_H
