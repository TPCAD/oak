#include <oak/interrupt/pit.h>
#include <oak/io.h>

void pit_init() {
    outb(PIT_CTRL_REG, 0b00110100);
    outb(PIT_CHAN0_REG, CLOCK_COUNT & 0xff);
    outb(PIT_CHAN0_REG, (CLOCK_COUNT >> 8) & 0xff);
}
