#include <oak/interrupt/rtc.h>
#include <oak/io.h>

#define CMOS_ADDR 0x70 // CMOS 地址寄存器
#define CMOS_DATA 0x71 // CMOS 数据寄存器

#define CMOS_SECOND 0x01
#define CMOS_MINUTE 0x03
#define CMOS_HOUR 0x05

#define CMOS_A 0x0a
#define CMOS_B 0x0b
#define CMOS_C 0x0c
#define CMOS_D 0x0d
#define CMOS_NMI 0x80

/**
 *  @brief  读 cmos 寄存器的值
 *  @param  addr  寄存器地址
 *  @return  寄存器值
 */
u8 cmos_read(u8 addr) {
    outb(CMOS_ADDR, CMOS_NMI | addr);
    return inb(CMOS_DATA);
};

/**
 *  @brief  写 cmos 寄存器的值
 *  @param  addr  寄存器地址
 *  @param  value  要写入的值
 */
void cmos_write(u8 addr, u8 value) {
    outb(CMOS_ADDR, CMOS_NMI | addr);
    outb(CMOS_DATA, value);
}
