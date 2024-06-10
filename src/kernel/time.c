#include <oak/debug.h>
#include <oak/io.h>
#include <oak/rtc.h>
#include <oak/stdlib.h>
#include <oak/time.h>

#define CMOS_ADDR 0x70 // CMOS address register
#define CMOS_DATA 0x71 // CMOS data register

#define CMOS_SECOND 0x00  // seconds [0, 59]
#define CMOS_MINUTE 0x02  // minutes [0, 59]
#define CMOS_HOUR 0x04    // hours [0, 23]
#define CMOS_WEEKDAY 0x06 // day of the week [1,7], 1 for sunday
#define CMOS_DAY 0x07     // day of the month [1, 31]
#define CMOS_MONTH 0x08   // month [1, 12]
#define CMOS_YEAR 0x09    // years since 1900 [0, 99]
#define CMOS_CENTURY 0x32 // may not exist
#define CMOS_NMI 0x80

#define MINUTE 60          // seconds per minute
#define HOUR (60 * MINUTE) // seconds per hour
#define DAY (24 * HOUR)    // seconds per day
#define YEAR (365 * DAY)   // seconds per year

static int month[13] = {0, // placeholder
                        0,
                        (31),
                        (31 + 29),
                        (31 + 29 + 31),
                        (31 + 29 + 31 + 30),
                        (31 + 29 + 31 + 30 + 31),
                        (31 + 29 + 31 + 30 + 31 + 30),
                        (31 + 29 + 31 + 30 + 31 + 30 + 31),
                        (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31),
                        (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
                        (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
                        (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)};

time_t startup_time;
int century;

int elapsed_leap_years(int year) {
    int result = 0;
    result += (year - 1) / 4;
    result -= (year - 1) / 100;
    result += (year + 299) / 400;
    result -= (1970 - 1900) / 4;
    return result;
}

bool is_leap_year(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || ((year + 1900) % 400 == 0);
}

void localtime(time_t stamp, tm *time) {
    time->tm_sec = stamp % 60;

    time_t remain = stamp / 60;

    time->tm_min = remain % 60;
    remain /= 60;

    time->tm_hour = remain % 24;
    time_t days = remain / 24;

    time->tm_wday = (days + 4) % 7; // 1970-01-01 是星期四

    // 这里产生误差显然需要 365 个闰年，不管了
    int years = days / 365 + 70;
    time->tm_year = years;
    int offset = 1;
    if (is_leap_year(years))
        offset = 0;

    days -= elapsed_leap_years(years);
    time->tm_yday = days % (366 - offset);

    int mon = 1;
    for (; mon < 13; mon++) {
        if ((month[mon] - offset) > time->tm_yday)
            break;
    }

    time->tm_mon = mon - 1;
    time->tm_mday = time->tm_yday - month[time->tm_mon] + offset + 1;
}

time_t mktime(tm *time) {
    time_t res;

    int year; // years since 1970

    if (time->tm_year >= 70) {
        year = time->tm_year - 70;
    } else {
        year = time->tm_year - 70 + 100;
    }

    res = YEAR * year;

    res += DAY * ((year + 1) / 4); // add leap year that had passed

    res += month[time->tm_mon + 1] * DAY;

    if (time->tm_mon > 2 && ((year + 2) % 4)) {
        res -= DAY;
    }

    res += DAY * (time->tm_mday - 1);

    res += HOUR * time->tm_hour;

    res += MINUTE * time->tm_min;

    res += time->tm_sec;

    return res;
}

int get_yday(tm *time) {
    int res = month[time->tm_mon];
    res += time->tm_mday - 1;

    int year;
    if (time->tm_year >= 70) {
        year = time->tm_year - 70;
    } else {
        year = time->tm_year - 70 + 100;
    }

    if ((year + 2) % 4 && time->tm_mon > 2) {
        res -= 1;
    }
    return res;
}

void time_read_bcd(tm *time) {
    // accessing CMOS is slow, to reduce errors, read again if second in CMOS
    // changed after read all data.
    do {
        time->tm_sec = cmos_read(CMOS_SECOND);
        time->tm_min = cmos_read(CMOS_MINUTE);
        time->tm_hour = cmos_read(CMOS_HOUR);
        time->tm_wday = cmos_read(CMOS_WEEKDAY);
        time->tm_mday = cmos_read(CMOS_DAY);
        time->tm_mon = cmos_read(CMOS_MONTH);
        time->tm_year = cmos_read(CMOS_YEAR);
        century = cmos_read(CMOS_CENTURY);
    } while (time->tm_sec != cmos_read(CMOS_SECOND));
}

void time_read(tm *time) {
    time_read_bcd(time);
    time->tm_sec = bcd_to_bin(time->tm_sec);
    time->tm_min = bcd_to_bin(time->tm_min);
    time->tm_hour = bcd_to_bin(time->tm_hour);
    time->tm_wday = bcd_to_bin(time->tm_wday) - 1;
    time->tm_mday = bcd_to_bin(time->tm_mday);
    time->tm_mon = bcd_to_bin(time->tm_mon) - 1;
    time->tm_year = bcd_to_bin(time->tm_year);
    time->tm_yday = get_yday(time);
    time->tm_isdst = -1;
    century = bcd_to_bin(century);
}

void time_init() {
    tm time;
    time_read(&time);
    startup_time = mktime(&time);
    DEBUGK("startup time: %d%d-%02d-%02d %02d:%02d:%02d\n", century,
           time.tm_year, time.tm_mon + 1, time.tm_mday, time.tm_hour,
           time.tm_min, time.tm_sec);
    DEBUGK("%ld\n", startup_time);
}
