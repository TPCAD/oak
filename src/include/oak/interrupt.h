#ifndef OAK_INTERRUPT_H
#define OAK_INTERRUPT_H

#include <oak/types.h>

#define IDT_SIZE 256

#define IRQ_CLOCK 0      // clock
#define IRQ_KEYBOARD 1   // keyboard
#define IRQ_CASCADE 2    // slave controller
#define IRQ_SERIAL_2 3   // serial port 2
#define IRQ_SERIAL_1 4   // serial port 1
#define IRQ_PARALLEL_2 5 // parallel port 2
#define IRQ_FLOPPY 6     // floppy controller
#define IRQ_PARALLEL_1 7 // parallel port 1
#define IRQ_RTC 8        // real time clock
#define IRQ_REDIRECT 9   // redirecttion IRQ2
#define IRQ_MOUSE 12     // mouse
#define IRQ_MATH 13      // coprocessor x87
#define IRQ_HARDDISK 14  // ATA disk first channel
#define IRQ_HARDDISK2 15 // ATA disk second channel

#define IRQ_MASTER_NR 0x20 // master start vector
#define IRQ_SLAVE_NR 0x28  // slave start vector

typedef struct gate_t {
    u16 offset_low;  // offset 0 ~ 15 bits
    u16 selector;    // segment selector
    u8 reserved;     // reserved
    u8 type : 4;     // 0b1110 presents interrupt gate
    u8 segment : 1;  // 0 for system segment
    u8 DPL : 2;      // Descriptor Privilege Level
    u8 present : 1;  // must be 1 to be valid
    u16 offset_high; // offset 16 ~ 31 bits
} _packed gate_t;

typedef void *handler_t;

void send_eoi(int vector);

void set_interrupt_handler(u32 irq, handler_t handler);
void set_interrupt_mask(u32 irq, bool enable);

bool interrupt_diable();              // clear IF bit, return the former value
bool get_interrupt_state();           // get IF bit
void set_interrupt_state(bool state); // set IF bit

#endif // !OAK_INTERRUPT_H
