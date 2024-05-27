#include "oak/assert.h"
#include "oak/bitmap.h"
#include "oak/interrupt.h"
#include "oak/list.h"
#include "oak/oak.h"
#include "oak/syscall.h"
#include "oak/types.h"
#include <oak/debug.h>
#include <oak/memory.h>
#include <oak/printk.h>
#include <oak/string.h>
#include <oak/task.h>

#define NR_TASKS 64

extern bitmap_t kernel_map;
extern void task_switch(task_t *next);
extern void idle_thread();
extern void init_thread();

static task_t *task_table[NR_TASKS];
static list_t block_list;
static task_t *idle_task;

static task_t *get_free_task() {
    for (size_t i = 0; i < NR_TASKS; i++) {
        if (task_table[i] == NULL) {
            task_table[i] = (task_t *)alloc_kpage(1);
            return task_table[i];
        }
    }
    panic("No more tasks");
}

static task_t *task_search(task_state_t state) {
    assert(!get_interrupt_state());
    task_t *task = NULL;
    task_t *current = running_task();

    for (size_t i = 0; i < NR_TASKS; i++) {
        task_t *ptr = task_table[i];
        if (ptr == NULL) {
            continue;
        }
        if (ptr->state != state) {
            continue;
        }
        if (current == ptr) {
            continue;
        }
        if (task == NULL || task->ticks < ptr->ticks ||
            ptr->jiffies < task->jiffies) {
            task = ptr;
        }
    }
    if (task == NULL && state == TASK_READY) {
        task = idle_task;
    }
    return task;
}

void task_yield() { schedule(); }

void task_block(task_t *task, list_t *blist, task_state_t state) {
    assert(!get_interrupt_state());
    assert(task->node.next == NULL);
    assert(task->node.prev == NULL);

    if (blist == NULL) {
        blist = &block_list;
    }

    list_push(blist, &task->node);

    assert(state != TASK_READY && state != TASK_RUNNING);

    task->state = state;
    task_t *current = running_task();
    if (current == task) {
        schedule();
    }
}

void task_unblock(task_t *task) {
    assert(!get_interrupt_state());
    list_remove(&task->node);

    assert(task->node.next == NULL);
    assert(task->node.prev == NULL);

    task->state = TASK_READY;
}

task_t *running_task() {
    asm volatile("movl %esp, %eax\n"
                 "andl $0xfffff000, %eax\n");
}

void schedule() {
    assert(!get_interrupt_state());
    task_t *current = running_task();
    task_t *next = task_search(TASK_READY);

    assert(next != NULL);
    assert(next->magic == OAK_MAGIC);

    if (current->state == TASK_RUNNING) {
        current->state = TASK_READY;
    }

    if (!current->ticks) {
        current->ticks = current->priority;
    }

    next->state = TASK_RUNNING;
    if (next == current) {
        return;
    }

    task_switch(next);
}

static task_t *task_create(target_t target, const char *name, u32 priority,
                           u32 uid) {

    task_t *task = get_free_task();
    memset(task, 0, PAGE_SIZE);

    u32 stack = (u32)task + PAGE_SIZE;

    stack -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)stack;
    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (void *)target;

    strcpy((char *)task->name, name);

    task->stack = (u32 *)stack;
    task->priority = priority;
    task->ticks = task->priority;
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->vmap = &kernel_map;
    task->pde = KERNEL_PAGE_DIR;
    task->magic = OAK_MAGIC;

    return task;
}

static void task_setup() {
    task_t *task = running_task();
    task->magic = OAK_MAGIC;
    task->ticks = 1;

    memset(task_table, 0, sizeof(task_table));
}

void task_init() {
    list_init(&block_list);
    task_setup();

    idle_task = task_create((target_t *)idle_thread, "idle", 1, KERNEL_USER);
    task_create((target_t *)init_thread, "init", 5, NORMAL_USER);
}
