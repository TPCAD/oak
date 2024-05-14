#ifndef OAK_TIME_H
#define OAK_TIME_H

#include <oak/types.h>

typedef struct tm {
    int tm_sec;   // second [0, 59]
    int tm_min;   // minute [0, 59]
    int tm_hour;  // hour [0, 23]
    int tm_mday;  // days in a month [0, 31]
    int tm_mon;   // months[0, 11]
    int tm_year;  // years start from 1900
    int tm_wday;  // day in a week [0, 6], 0 for sunday
    int tm_yday;  // day in a year [0, 365]
    int tm_isdst; // daylight saving time sign
} tm;

void time_read_bcd(tm *time);
void time_read(tm *time);
time_t mktime(tm *time);

#endif // !OAK_TIME_H
