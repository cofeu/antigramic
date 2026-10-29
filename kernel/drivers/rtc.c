#include <rtc.h>
#include <serial.h>

static uint8_t cmos_read(uint8_t reg) {
    outb(0x70, (uint8_t)(reg & 0x7F));
    return inb(0x71);
}
static int snapshot(uint8_t values[7]) {
    for (unsigned i = 0; i < 100000; ++i) if (!(cmos_read(0x0A) & 0x80)) {
        values[0] = cmos_read(0x00); values[1] = cmos_read(0x02);
        values[2] = cmos_read(0x04); values[3] = cmos_read(0x06);
        values[4] = cmos_read(0x07); values[5] = cmos_read(0x08);
        values[6] = cmos_read(0x09);
        return 1;
    }
    return 0;
}
static uint8_t bcd(uint8_t value) { return (uint8_t)((value & 15) + ((value >> 4) * 10)); }

int rtc_read(rtc_time_t *time) {
    uint8_t a[7], b[7];
    if (!time) return 0;
    for (int retry = 0; retry < 4; ++retry) {
        if (!snapshot(a) || !snapshot(b)) return 0;
        int same = 1;
        for (int i = 0; i < 7; ++i) if (a[i] != b[i]) same = 0;
        if (same) break;
        if (retry == 3) return 0;
    }
    uint8_t status = cmos_read(0x0B);
    uint8_t hour = a[2];
    int pm = !!(hour & 0x80);
    hour &= 0x7F;
    if (!(status & 0x04)) {
        a[0] = bcd(a[0]); a[1] = bcd(a[1]); hour = bcd(hour);
        a[3] = bcd(a[3]); a[4] = bcd(a[4]); a[5] = bcd(a[5]); a[6] = bcd(a[6]);
    }
    if (!(status & 0x02)) { hour = (uint8_t)(hour % 12); if (pm) hour += 12; }
    uint8_t century = cmos_read(0x32);
    if (!(status & 0x04)) century = bcd(century);
    time->second = a[0]; time->minute = a[1]; time->hour = hour;
    time->weekday = a[3]; time->day = a[4]; time->month = a[5];
    time->year = century >= 19 && century <= 99 ? (uint16_t)(century * 100 + a[6])
                                                  : (uint16_t)(a[6] >= 70 ? 1900 + a[6] : 2000 + a[6]);
    return time->month >= 1 && time->month <= 12 && time->day >= 1 && time->day <= 31;
}
