#ifndef OAK_TASK_H
#define OAK_TASK_H

#include <oak/fs.h>
#include <oak/list.h>
#include <oak/types.h>

#define KERNEL_USER 0
#define NORMAL_USER 1000

#define TASK_NAME_LEN 16
#define TASK_FILE_NR 16 // most file amount of process

typedef void target_t();

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
    u32 *stack;                         // kernel stack
    list_node_t node;                   // task blocked node
    task_state_t state;                 // task status
    u32 priority;                       // priority
    int ticks;                          // left jiffies
    u32 jiffies;                        // jiffies last ran
    char name[TASK_NAME_LEN];           // task name
    u32 uid;                            // user id
    u32 gid;                            // gid
    u32 pid;                            // task id
    u32 ppid;                           // task father id
    u32 pde;                            // pde
    struct bitmap_t *vmap;              // virtual memory map
    u32 text;                           // code section address
    u32 data;                           // data section address
    u32 end;                            // program end address
    u32 brk;                            // the highest address of heap memory
    int status;                         // process special status
    pid_t waitpid;                      // wait pid
    char *pwd;                          // current dir of process
    struct inode_t *ipwd;               //
    struct inode_t *iroot;              //
    struct inode_t *iexec;              // program file inode
    u16 umask;                          // process user privilege
    struct file_t *files[TASK_FILE_NR]; // 进程文件表
    u32 magic;                          // magic number
} task_t;

// ABI
typedef struct task_frame_t {
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp;
    void (*eip)(void);
} task_frame_t;

typedef struct intr_frame_t {
    u32 vector;

    u32 edi;
    u32 esi;
    u32 ebp;
    // esp changes constantly, therefore, it will be ignored by popad, although
    // pushad push it
    u32 esp_dummy;

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

    u32 eip;
    u32 cs;
    u32 eflags;
    u32 esp;
    u32 ss;
} intr_frame_t;

task_t *running_task();
void schedule();
pid_t task_fork();
void task_exit(int status);
pid_t task_waitpid(pid_t pid, int32 *status);
void task_yield();
void task_block(task_t *task, list_t *blist, task_state_t state);
void task_unblock(task_t *task);
void task_sleep(u32 ms);
void task_wakeup();
void task_to_user_mode();
pid_t sys_getpid();
pid_t sys_getppid();

fd_t task_get_fd(task_t *task);
void task_put_fd(task_t *task, fd_t fd);

#endif // !OAK_TASK_H
