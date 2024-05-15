#ifndef OAK_RTC_H
#define OAK_RTC_H

#include <oak/types.h>

u8 cmos_read(u8 addr);
void cmos_write(u8 addr, u8 value);
void set_alarm(u32 secs);

#endif // !OAK_RTC_H
