/* RTC Header */

#ifndef HEADER_RTC_H
#define HEADER_RTC_H

#include <inttypes.h>
#include "z80.h"

#define RTC_A_UIP       B8(10000000)    // Update in progress
#define RTC_A_DV2       B8(01000000)    // divider selection bit 2
#define RTC_A_DV1       B8(00100000)    // divider selection bit 1
#define RTC_A_DV0       B8(00010000)    // divider selection bit 0
#define RTC_A_RS3       B8(00001000)    // rate selection bit 3
#define RTC_A_RS2       B8(00000100)    // rate selection bit 2
#define RTC_A_RS1       B8(00000010)    // rate selection bit 1
#define RTC_A_RS0       B8(00000001)    // rate selection bit 0

#define RTC_B_SET       B8(10000000)    // SET
#define RTC_B_PIE       B8(01000000)    // Periodic interrupt enable
#define RTC_B_AIE       B8(00100000)    // Alarm interrupt enable
#define RTC_B_UIE       B8(00010000)    // Update-ended interrupt enable
#define RTC_B_SQWE      B8(00001000)    // Square wave output enable
#define RTC_B_DM        B8(00000100)    // Data mode, 1=Binary, 0=BCD
#define RTC_B_2412      B8(00000010)    // Hours mode, 24=1 12=0
#define RTC_B_DSE       B8(00000001)    // Daylight savings enable

#define RTC_C_IRQF      B8(10000000)    // Interrupt request flag
#define RTC_C_PF        B8(01000000)    // Periodic interrupt flag
#define RTC_C_AF        B8(00100000)    // Alarm interrupt flag
#define RTC_C_UF        B8(00010000)    // Update-ended interrupt flag

#define RTC_D_VRT       B8(10000000)    // Valid RAM and time

enum
   {
    seconds,                            // seconds
    seconds_alarm,                      // seconds alarm
    minutes,                            // minutes
    minutes_alarm,                      // minutes alarm
    hours,                              // hours
    hours_alarm,                        // hours alarm
    wday,                               // day of week
    mday,                               // day of month
    month,                              // month
    year,                               // year
    reg_a,                              // read/write register
    reg_b,                              // MC146818 read/write register
    reg_c,                              // MC146818 read only register
    reg_d,                              // MC146818 read only register
    userram                             // user RAM
   };

#pragma pack(push, 1)  // push current alignment, alignment to 1 byte boundary

// MC146818 address map
typedef struct rtc_mem_t
   {
    uint8_t seconds;                    // seconds (0-59)
    uint8_t seconds_alarm;              // seconds alarm (0-59)
    uint8_t minutes;                    // minutes (0-59)
    uint8_t minutes_alarm;              // minutes alarm (0-59)
    uint8_t hours;                      // hours (1-12) or (0-23)
    uint8_t hours_alarm;                // hours alarm (1-12) or (0-23)
    uint8_t wday;                       // day of week (1-7, 1=Sunday)
    uint8_t mday;                       // day of month (1-31)
    uint8_t month;                      // month (1-12)
    uint8_t year;                       // year (0-99)
    uint8_t reg_a;                      // read/write register
    uint8_t reg_b;                      // MC146818 read/write register
    uint8_t reg_c;                      // MC146818 read only register
    uint8_t reg_d;                      // MC146818 read only register
    uint8_t userram[50];                // user RAM
   }rtc_mem_t;

typedef union rtc_u
   {
    rtc_mem_t member;
    uint8_t ram[64];
   }rtc_u;

#pragma pack(pop)       // restore original alignment from stack

int rtc_init (void);
int rtc_deinit (void);
int rtc_reset (void);
uint16_t rtc_r (uint16_t port, struct z80_port_read *port_s);
void rtc_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
int rtc_poll (void);
void rtc_regdump (void);
void rtc_clock (int cpuclock);

#endif     /* HEADER_RTC_H */
