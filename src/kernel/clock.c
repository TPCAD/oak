#include "oak/debug.h"
#include "oak/io.h"
#include "oak/oak.h"
#include "oak/task.h"
#include "oak/types.h"
#include <oak/assert.h>
#include <oak/interrupt.h>
#define PIT_CHAN0_REG 0x40
#define PIT_CHAN2_REG 0x42
#define PIT_CTRL_REG 0x43

#define HZ 100
#define OSCILLATOR 1193182
#define CLOCK_COUNTER (OSCILLATOR / HZ)
#define JIFFY (1000 / HZ)

#define SPEAKER_REG 0x61
#define BEEP_HZ 440
#define BEEP_COUNTER (OSCILLATOR / BEEP_HZ)

extern void schedule();

u32 volatile jiffies = 0;
u32 jiffy = JIFFY;

u32 volatile beeping = 0;

void start_beep() {
    if (!beeping) {
        outb(SPEAKER_REG, inb(SPEAKER_REG) | 0b11);
    }
    beeping = jiffies + 5;
}

void stop_beep() {
    if (beeping && jiffies > beeping) {
        outb(SPEAKER_REG, inb(SPEAKER_REG) & 0xfc);
        beeping = 0;
    }
}

void clock_handler(int vector) {
    assert(vector == 0x20);
    send_eoi(vector);

    jiffies++;

    // stop pc speaker after five clock
    stop_beep();

    task_t *task = running_task();
    assert(task->magic == OAK_MAGIC);

    task->jiffies = jiffies;
    task->ticks--;
    if (!task->ticks) {
        schedule();
    }
}

void pit_init() {
    // init PIT
    outb(PIT_CTRL_REG, 0b00110100);
    outb(PIT_CHAN0_REG, CLOCK_COUNTER & 0xff);
    outb(PIT_CHAN0_REG, (CLOCK_COUNTER >> 8) & 0xff);

    // init pc speaker
    outb(PIT_CTRL_REG, 0b10110110);
    outb(PIT_CHAN2_REG, (u8)BEEP_COUNTER);
    outb(PIT_CHAN2_REG, (u8)(BEEP_COUNTER >> 8));
}

void clock_init() {
    pit_init();
    set_interrupt_handler(IRQ_CLOCK, clock_handler);
    set_interrupt_mask(IRQ_CLOCK, true);
}
