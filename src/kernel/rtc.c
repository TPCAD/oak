#include <oak/assert.h>
#include <oak/debug.h>
#include <oak/interrupt.h>
#include <oak/io.h>
#include <oak/rtc.h>
#include <oak/stdlib.h>
#include <oak/time.h>

#define CMOS_ADDR 0x70 // CMOS address register
#define CMOS_DATA 0x71 // CMOS data register

#define CMOS_SECOND 0x01 // CMOS alarm second
#define CMOS_MINUTE 0x03 // CMOS alarm minute
#define CMOS_HOUR 0x05   // CMOS alarm hour

#define CMOS_A 0x0a
#define CMOS_B 0x0b
#define CMOS_C 0x0c
#define CMOS_D 0x0d
#define CMOS_NMI 0x80

u8 cmos_read(u8 addr) {
    outb(CMOS_ADDR, CMOS_NMI | addr);
    return inb(CMOS_DATA);
}

void cmos_write(u8 addr, u8 value) {
    outb(CMOS_ADDR, CMOS_NMI | addr);
    outb(CMOS_DATA, value);
}

static u32 volatile counter = 0;

// trigger RTC interrupt after `secs` seconds
void set_alarm(u32 secs) {
    tm time;
    time_read(&time);

    u8 sec = secs % 60;
    secs /= 60;
    u8 min = secs % 60;
    secs /= 60;
    u32 hour = secs;

    time.tm_sec += sec;
    if (time.tm_sec >= 60) {
        time.tm_sec %= 60;
        time.tm_min += 1;
    }

    time.tm_min += min;
    if (time.tm_min >= 60) {
        time.tm_min %= 60;
        time.tm_hour += 1;
    }

    time.tm_hour += hour;
    if (time.tm_hour >= 24) {
        time.tm_hour %= 24;
    }

    cmos_write(CMOS_HOUR, bin_to_bcd(time.tm_hour));
    cmos_write(CMOS_MINUTE, bin_to_bcd(time.tm_min));
    cmos_write(CMOS_SECOND, bin_to_bcd(time.tm_sec));
}

// RTC interrupt handler function
void rtc_handler(int vector) {
    assert(vector == 0x28);

    send_eoi(vector);

    // read CMOS register C to allow CMOS produce interrupt
    cmos_read(CMOS_C);

    set_alarm(1);

    DEBUGK("rtc handler %d...\n", counter++);
}

void rtc_init() {
    u8 prev;

    // cmos_write(CMOS_B, 0b01000010); // turn on periodical interrupt
    cmos_write(CMOS_B, 0b00100010); // turn on alarm interrupt
    cmos_read(CMOS_C); // read CMOS register C to allow CMOS produce interrupt

    set_alarm(2);

    // set interrupt frequency
    outb(CMOS_A, (inb(CMOS_A) & 0xf) | 0b1110);

    set_interrupt_handler(IRQ_RTC, rtc_handler);
    set_interrupt_mask(IRQ_RTC, true);
    set_interrupt_mask(IRQ_CASCADE, true);
}
