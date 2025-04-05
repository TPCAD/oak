#include "oak/cpu.h"
#include "oak/list.h"
#include "oak/mm/memory.h"
#include "oak/mm/pmm.h"
#include <oak/debug/kassert.h>
#include <oak/mm/vmm.h>
#include <oak/oak.h>
#include <oak/string.h>
#include <oak/task.h>

extern void task_switch(task_t *addr);

/* 记录系统运行中的任务，最多存在 64 个任务 */
#define NR_TASKS 64
static task_t *task_table[NR_TASKS];

// 阻塞链表
static list_t block_list;

/**
 *  @brief  从 `task_table` 中找到一个空位
 *  @return  新的 `task_t` 地址
 */
static task_t *find_free_task() {
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

static task_t *build_basic_task(target_t target, const char *name, u32 priority,
                                u32 uid) {
    task_t *task = find_free_task();

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
    task->ticks = task->priority;
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->magic = OAK_MAGIC;

    return task;
}

/**
 *  @brief  搜索指定状态的任务
 *  @param  state  任务状态
 *  @return  任务指针
 *
 *  从任务数组中搜索指定状态的运行时间最短或剩余时间片最少的任务，不包括当前任务
 */
static task_t *search_state_task(task_state_t state) {
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
        if (task == NULL || task->ticks < ptr->ticks ||
            ptr->jiffies < task->jiffies) {
            task = ptr;
        }
    }

    return task;
}

/**
 *  @brief  构建临时内核任务
 *
 *  内核被 bootloader 放在 0x10000 的位置，内核的栈底地址也是 0x10000。根据
 *  `task_t` 的结构，内核的 `task_t` 起始位置是 0xf000。为了在时钟中断到来时可以
 *  正确切换到其他任务，构建一个临时的 `task_t` 结构，这个结构只有 `magic` 和
 *  `ticks` 字段有值。
 *
 *  内核任务在切换到其他任务后就不会再被执行，因为 `task_table` 没有记录内核任务
 */
static void build_temp_kernel_task() {
    task_t *temp_kernel_task = task_current_running();
    temp_kernel_task->magic = OAK_MAGIC;
    temp_kernel_task->ticks = 1;
}

/**
 *  @brief  任务调度
 */
void task_schedule() {
    kassert(!cpu_get_intr_state());
    task_t *curr_task = task_current_running();
    task_t *found_task = search_state_task(TASK_READY);

    kassert(found_task != NULL);
    kassert(found_task->magic == OAK_MAGIC);

    if (curr_task->state == TASK_RUNNING) {
        curr_task->state = TASK_READY;
    }

    found_task->state = TASK_RUNNING;
    if (found_task == curr_task) {
        return;
    }

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
 *  若阻塞任务是当前运行任务则进行调度
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
 */
void task_unblock(task_t *task) {
    kassert(!cpu_get_intr_state());

    list_remove(&task->node);

    kassert(task->node.next == NULL);
    kassert(task->node.prev == NULL);

    task->state = TASK_READY;
}

/**
 *  @brief  获取当前运行的任务
 *  @return  当前运行的任务的地址
 *
 *  当前任务的栈和任务的 task_t 结构体位于同一页，且 task_t 结构体位于页起始
 *  地址，取当前 esp 值即可得到当前任务的 task_t 结构体地址。
 */
task_t *task_current_running() {
    asm volatile("movl %esp, %eax\n"
                 "andl $0xfffff000, %eax\n");
}

extern u32 thread_a();
extern u32 thread_b();
extern u32 thread_c();

/**
 *  @brief  初始化阻塞队列、任务表，构建临时内核任务，创建主要进程
 */
void task_init() {
    list_init(&block_list);

    build_temp_kernel_task();
    memset(task_table, 0, sizeof(task_table));

    build_basic_task(thread_a, "testA", 5, NORMAL_USER);
    build_basic_task(thread_b, "testB", 5, NORMAL_USER);
    // build_basic_task(thread_c, "testC", 5, NORMAL_USER);
}
