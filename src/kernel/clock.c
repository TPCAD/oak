#include "oak/oak.h"
#include "oak/task.h"
#include <oak/clock.h>
#include <oak/debug/kassert.h>
#include <oak/debug/kdebug.h>
#include <oak/interrupt/idt.h>
#include <oak/interrupt/pic.h>
#include <oak/interrupt/pit.h>

// 全局时间片，每次时钟中断会加 1
u32 volatile jiffies = 0;

// 时间片长度，单位为 ms，改变量只是为了方便其他文件使用，这样不必引入多余头文件
u32 jiffy = JIFFY;

extern void task_wakeup();

void clock_handler(u32 vector) {
    kassert(vector == IRQ_MASTER_NR + IRQ_CLOCK);
    pic_send_eoi(vector);

    // 每个时间片都唤醒合适的任务
    task_wakeup();

    jiffies++;
    // KDEBUG("clock interrupt, jiffies: %d\n", jiffies);

    task_t *curr_task = task_current_running();
    kassert(curr_task->magic == OAK_MAGIC);

    curr_task->jiffies = jiffies;
    curr_task->ticks--;
    if (!curr_task->ticks) {
        curr_task->ticks = curr_task->priority;
        task_schedule();
    }
}

/**
 *  @brief  初始化时钟中断
 *
 *  初始化 PIT，注册中断函数，开启时钟中断
 */
void clock_init() {
    pit_init();
    idt_set_intr_handler(IRQ_CLOCK, clock_handler);
    pic_set_intr_mask(IRQ_CLOCK, true);
}
