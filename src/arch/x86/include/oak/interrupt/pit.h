#ifndef OAK_PIT_H
#define OAK_PIT_H

#define PIT_CHAN0_REG 0x40
#define PIT_CHAN2_REG 0x42
#define PIT_CTRL_REG 0x43

#define HZ 100 // 时钟频率 100 hz = 0.01 s = 10 ms
#define OSCILLATOR 1193182
#define CLOCK_COUNT (OSCILLATOR / HZ)
#define JIFFY (1000 / HZ) // 时间片长度 10 ms

void pit_init();

#endif // !OAK_PIT_H
