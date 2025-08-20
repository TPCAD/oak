#ifndef OAK_PIC_H
#define OAK_PIC_H

#include <oak/types.h>

#define PIC_M_CTRL 0x20 // master control port
#define PIC_M_DATA 0x21 // master data port
#define PIC_S_CTRL 0xa0 // slave control port
#define PIC_S_DATA 0xa1 // slave data port
#define PIC_EOI 0x20    // end of interrupt

#define IRQ_MASTER_NR 0x20
#define IRQ_SLAVE_NR 0x28

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

void pic_send_eoi(int vector);

void pic_set_intr_mask(u32 irq, bool enable);

#endif // !OAK_PIC_H
