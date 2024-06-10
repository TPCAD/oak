#ifndef OAK_TIME_H
#define OAK_TIME_H

#include <oak/types.h>

typedef struct tm {
    int tm_sec;   // second [0, 59]
    int tm_min;   // minute [0, 59]
    int tm_hour;  // hours since midnight [0, 23]
    int tm_mday;  // day of the month [1, 31]
    int tm_mon;   // months since January [0, 11]
    int tm_year;  // years since 1900
    int tm_wday;  // days since Sunday [0, 6], 0 for sunday
    int tm_yday;  // days since January 1 [0, 365]
    int tm_isdst; // daylight saving time flag
} tm;

void time_read_bcd(tm *time);
void time_read(tm *time);
time_t mktime(tm *time); // timestamp start from 1970-01-01 00:00:00
void localtime(time_t stamp, tm *time);
#endif // !OAK_TIME_H
