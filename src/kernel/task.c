#include "oak/assert.h"
#include "oak/bitmap.h"
#include "oak/interrupt.h"
#include "oak/list.h"
#include "oak/oak.h"
#include "oak/types.h"
#include <oak/debug.h>
#include <oak/global.h>
#include <oak/memory.h>
#include <oak/printk.h>
#include <oak/string.h>
#include <oak/task.h>

#define NR_TASKS 64

extern u32 volatile jiffies;
extern u32 jiffy; // ms per jiffiy
extern bitmap_t kernel_map;
extern tss_t tss;

extern void task_switch(task_t *next);
extern void idle_thread();
extern void init_thread();
extern void test_thread();

static task_t *task_table[NR_TASKS];
static list_t block_list;
static list_t sleep_list;
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

void task_sleep(u32 ms) {
    assert(!get_interrupt_state());

    u32 ticks = ms / jiffy;
    ticks = ticks > 0 ? ticks : 1;

    task_t *current = running_task();
    current->ticks = jiffies + ticks;

    list_t *list = &sleep_list;
    list_node_t *anchor = &list->tail;

    for (list_node_t *ptr = list->head.next; ptr != &list->tail;
         ptr = ptr->next) {
        task_t *task = element_entry(task_t, node, ptr);

        if (task->ticks > current->ticks) {
            anchor = ptr;
            break;
        }
    }

    assert(current->node.next == NULL);
    assert(current->node.prev == NULL);

    list_insert_before(anchor, &current->node);

    current->state = TASK_SLEEPING;

    schedule();
}

void task_wakeup() {
    assert(!get_interrupt_state());

    list_t *list = &sleep_list;
    for (list_node_t *ptr = list->head.next; ptr != &list->tail;) {
        task_t *task = element_entry(task_t, node, ptr);
        if (task->ticks > jiffies) {
            break;
        }

        ptr = ptr->next;

        task->ticks = 0;
        task_unblock(task);
    }
}

void task_active(task_t *task) {
    assert(task->magic == OAK_MAGIC);

    if (task->uid != KERNEL_USER) {
        tss.esp0 = (u32)task + PAGE_SIZE;
    }
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

    task_active(next);
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

void task_to_user_mode(target_t *target) {
    task_t *task = running_task();
    u32 addr = (u32)task + PAGE_SIZE;

    // push intr_frame into stack
    addr -= sizeof(intr_frame_t);

    intr_frame_t *iframe = (intr_frame_t *)addr;

    iframe->vector = 0x20;
    iframe->edi = 1;
    iframe->esi = 2;
    iframe->ebp = 3;
    iframe->esp_dummy = 4;
    iframe->ebx = 5;
    iframe->edx = 6;
    iframe->ecx = 7;
    iframe->eax = 8;

    iframe->gs = 0;
    iframe->ds = USER_DATA_SELECTOR;
    iframe->es = USER_DATA_SELECTOR;
    iframe->fs = USER_DATA_SELECTOR;
    iframe->ss = USER_DATA_SELECTOR;
    iframe->cs = USER_CODE_SELECTOR;

    iframe->error = OAK_MAGIC;

    u32 stack3 = alloc_kpage(1); // todo: replace to user stack

    iframe->eip = (u32)target;
    iframe->eflags = (0 << 12 | 0b10 | 1 << 9);
    iframe->esp = stack3 + PAGE_SIZE;

    asm volatile("movl %0, %%esp\n"
                 "jmp interrupt_exit\n" ::"m"(iframe));
}

static void task_setup() {
    task_t *task = running_task();
    task->magic = OAK_MAGIC;
    task->ticks = 1;

    memset(task_table, 0, sizeof(task_table));
}

void task_init() {
    list_init(&block_list);
    list_init(&sleep_list);
    task_setup();

    idle_task = task_create(idle_thread, "idle", 1, KERNEL_USER);
    task_create(init_thread, "init", 5, NORMAL_USER);
    task_create(test_thread, "test", 5, NORMAL_USER);
}
