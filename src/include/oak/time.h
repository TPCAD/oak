#ifndef OAK_TIME_H
#define OAK_TIME_H

#include <oak/types.h>

typedef struct tm {
    u32 tm_sec;   // 秒数 [0，59]
    u32 tm_min;   // 分钟数 [0，59]
    u32 tm_hour;  // 小时数 [0，59]
    u32 tm_mday;  // 1 个月的天数 [0，31]
    u32 tm_mon;   // 1 年中月份 [0，11]
    u32 tm_year;  // 从 1900 年开始的年数
    u32 tm_wday;  // 1 星期中的某天 [0，6] (星期天 =0)
    u32 tm_yday;  // 1 年中的某天 [0，365]
    u32 tm_isdst; // 夏令时标志
} tm;

void time_read_bcd(tm *time);
void time_read(tm *time);
time_t mktime(tm *time);

#endif
