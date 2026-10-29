#ifndef KERNEL_RTC_H
#define KERNEL_RTC_H
#include <stdint.h>
typedef struct { uint8_t second, minute, hour, weekday, day, month; uint16_t year; } rtc_time_t;
int rtc_read(rtc_time_t *time);
#endif
