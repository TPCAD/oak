#include "oak/debug/kassert.h"
#include "oak/debug/kdebug.h"
#include "oak/interrupt/idt.h"
#include "oak/interrupt/pic.h"
#include "oak/io.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_CTRL_PORT 0x64

void keyboard_handler(u32 vector) {
    kassert(vector == 0x21);
    pic_send_eoi(vector);

    u16 scancode = inb(KEYBOARD_DATA_PORT);
    KDEBUG("keyboard input 0x%x\n", scancode);
}

void keyboard_init() {
    idt_set_intr_handler(IRQ_KEYBOARD, keyboard_handler);
    pic_set_intr_mask(IRQ_KEYBOARD, true);
}
