//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                        Real Time Clock (RTC) module                        *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulates the MC146818 RTC circuit.
//
//==============================================================================
/*
 *  uBee512 - An emulator for the Microbee Z80 ROM, FDD and HDD based models.
 *  Copyright (C) 2007-2016 uBee   
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
//==============================================================================
// ChangeLog (most recent entries are at top)
//==============================================================================
// v4.7.0 - 29 June 2010, uBee
// - Changes made to fread() function to use the result as some compilers
//   report warning: declared with attribute warn_unused_result.
//
// v4.6.0 - 4 May 2010, uBee
// - Renamed log_data() calls to log_data_1() and log_port() to log_port_1().
// - Changes to rtc_regdump() to add binary output.
//
// v4.1.0 - 22 June 2009, uBee
// - Made improvements to the logging process.
//
// v4.0.0 - 19 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
//
// v3.1.0 - 22 April 2009, uBee
// - Removed all occurrences of console_output() function calls.
// - Changes made to allow time to be set and kept independently of the
//   host time.
// - VRT bit now set when register D is read and only set in rtc_init() if
//   an RTC intilisation file was succesfully loaded.
// - Changed all printf() calls to use a local xprintf() function.
//
// v2.8.0 - 31 August 2008, uBee
// - Added emulation of the RTC_A_UIP bit and implemented the Periodical
//   interrupt for all 16 divider rates.
// - Reorganised the rtc_poll() function coding.
// - The RTC_C_UF flag in the rtc_poll() function is now only set if
//   RTC_B_SET is clear.
// - Added rtc_clock() function.
//
// v2.7.0 - 28 May 2008, uBee
// - Added structure emu_t and moved variables into it.
//
// v2.5.0 - 26 February 2008, uBee
// - Added modelx.systname to the RTC file names to allow different operating
//   systems have unique files.
// - Implement the modelc structure.
//
// v2.3.0 - 7 January 2008, uBee
// - Added rtc_regdump() function to dump RTC registers.
// - Renamed rtc to rtcx an created a new rtc structure to keep the real RTC
//   contents in. rtcx is used as the working copy.
// - Added modio_t structure.
//
// v2.1.0 - 20 October 2007, uBee
// - Added alarm flag and interrupt set to rtc_poll() function.
// - Changes to return the AM/PM hours and alarm values correctly.
// - Changes to save RTC image with data that a real MC146818 would contain.
// - Implement the modelx information structure.
//
// v2.0.0 - 15 October 2007, uBee
// - Created a new file and implement the RTC emulation.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "rtc.h"
#include "z80api.h"
#include "ubee512.h"
#include "support.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
static char *rtc_regs_names[] =
{
 "seconds",
 "seconds_alarm",
 "minutes",
 "minutes_alarm",
 "hours",
 "hours_alarm",
 "wday",
 "mday",
 "month",
 "year",
 "reg_a",
 "reg_b",
 "reg_c",
 "reg_d"
};

// Values in table are based on 32.768 kHz crystal
static float periodic_interrupt_rate[16] =
{
// PIR (sec)      RS3 RS2 RS1 RS0
 0.000000000, //   0   0   0   0
 0.003906250, //   0   0   0   1
 0.007815000, //   0   0   1   0
 0.000122070, //   0   0   1   1
 0.000244141, //   0   1   0   0
 0.000488281, //   0   1   0   1
 0.000976562, //   0   1   1   0
 0.001953125, //   0   1   1   1
 0.003906250, //   1   0   0   0
 0.007812500, //   1   0   0   1
 0.015625000, //   1   0   1   0
 0.031250000, //   1   0   1   1
 0.062500000, //   1   1   0   0
 0.125000000, //   1   1   0   1
 0.250000000, //   1   1   1   0
 0.500000000  //   1   1   1   1
};

static int days_in_month[12] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static uint8_t addr;

static rtc_u rtc;
static rtc_u rtcx;

static int clocks_sec;
static int clocks_uip;
static int clocks_pf;

static int rtcpf_before;

static uint64_t rtc_time_ref;
static int rtc_secs_before;

static char name[512];

extern char userhome_rtcpath[];
extern char *model_args[];

extern emu_t emu;
extern model_t modelx;
extern model_custom_t modelc;
extern modio_t modio;

//==============================================================================
// Set clock date and time from host.
//
// struct tm
//
// This structure is used to hold the values representing a calendar time.
// It contains the following members, with the meanings as shown.
//
// int tm_sec      seconds after minute [0-61] (61 allows for 2 leap-seconds)
// int tm_min      minutes after hour [0-59]
// int tm_hour     hours after midnight [0-23]
// int tm_mday     day of the month [1-31]
// int tm_mon      month of year [0-11]
// int tm_year     current year-1900
// int tm_wday     days since Sunday [0-6]
// int tm_yday     days since January 1st [0-365]
// int tm_isdst    daylight savings indicator
//
//   pass: void
// return: void
//==============================================================================
static void rtc_setclockfromhost (void)
{
 typedef struct tm tm_t;

 time_t result;
 tm_t resultp;

 time(&result);
#ifdef MINGW
 memcpy(&resultp, localtime(&result), sizeof(resultp));
#else
 localtime_r(&result, &resultp);
#endif
 if (resultp.tm_sec > 59)               // 2 second leap-seconds ignore
    resultp.tm_sec = 59;
 rtcx.member.seconds = resultp.tm_sec;
 rtcx.member.minutes = resultp.tm_min;
 rtcx.member.hours = resultp.tm_hour;
 rtcx.member.mday = resultp.tm_mday;
 rtcx.member.month = resultp.tm_mon + 1;
 rtcx.member.year = resultp.tm_year - 100;
 rtcx.member.wday = resultp.tm_wday + 1;

 if (rtcx.member.year % 4)
    days_in_month[1] = 28;
 else
    days_in_month[1] = 29;
}

//==============================================================================
// Check for 1 second of elapsed time from host system and update the RTC
// time and date values.
//
//   pass: void
// return: int                          1 if updated, else 0
//==============================================================================
static int rtc_timer_update_cycle (void)
{
 int secs_behind;
 int secs_now;

 secs_now = (time_get_ms() - rtc_time_ref) / 1000;

 if (secs_now == rtc_secs_before)
    return 0;

 secs_behind = secs_now - rtc_secs_before;

 rtc_secs_before = secs_now;

 while (secs_behind--)  // make up all the lost seconds
    {
     if (++rtcx.member.seconds == 60)
        {
         rtcx.member.seconds = 0;
         if (++rtcx.member.minutes == 60)
            {
             rtcx.member.minutes = 0;
             if (++rtcx.member.hours == 24)
                {
                 rtcx.member.hours = 0;
                 if (++rtcx.member.wday > 7)
                    rtcx.member.wday = 1;
                 if (++rtcx.member.mday > days_in_month[rtcx.member.month-1])
                    {
                     rtcx.member.mday = 1;
                     if (++rtcx.member.month > 12)
                        {
                         rtcx.member.month = 1;
                         if (++rtcx.member.year > 99)
                            rtcx.member.year = 0;
                         if (rtcx.member.year % 4)
                            days_in_month[1] = 28;
                         else
                            days_in_month[1] = 29;
                        }
                    }
                }
            }
        }
    }
 return 1;
}

//==============================================================================
// Convert time data value to format specified by RTC_B_DM
//
//   pass: int time                     time in binary or BDC format
// return: int                          time in format specified by RTC_B_DM
//==============================================================================
static inline int rtc_time_convert (int time)
{
 if ((rtcx.member.reg_b & RTC_B_DM) == 0)     // if BCD format required
    time = ((time / 10) << 4) | (time % 10);
 return time;
}

//==============================================================================
// Convert time data value to binary format if RTC_B_DM is 0
//
//   pass: int time                     time in binary or BCD format
// return: int                          time in binary format
//==============================================================================
static inline int rtc_time_convert_bcdtobin (int time)
{
 if ((rtcx.member.reg_b & RTC_B_DM) == 0)    // if in BCD format
    time = ((time >> 4) * 10) + (time & 0x0f);
 return time;
}

//==============================================================================
// Convert binary 24 hour time format to the format as specified by RTC_B_2412
// and RTC_B_DM bits.
//
//   pass: int hours                    hours in AM/PM or 24 hour format
// return: int                          hours in required format
//==============================================================================
static inline int rtc_hours_convert (int hours)
{
 if ((rtcx.member.reg_b & RTC_B_2412) == 0)
    {
     if (hours == 0)                    // if 12 AM midnight
        return rtc_time_convert(12);
     else
        if (hours > 12)
           return rtc_time_convert(hours % 12) | B8(10000000);
        else
           return rtc_time_convert(hours);
    }
 else
    return rtc_time_convert(hours);
}

//==============================================================================
// Convert hours value to 24 hour binary format if RTC_B_2412 is 0
//
//   pass: int hours                    hours in AM/PM or 24 hour format
// return: int                          hours in 24 hour format
//==============================================================================
static inline int rtc_hours_convert_12to24 (int hours)
{
 int pm;

 pm = hours & B8(10000000);
 hours = rtc_time_convert_bcdtobin(hours & B8(01111111));

 if ((rtcx.member.reg_b & RTC_B_2412) == 0)  // if AM/PM mode
    {
     if ((hours == 12) && (pm))         // if 12 PM midday
        return 12;                      // 1200
     if (pm)
        return hours + 12;              // 1300 - 2300
     if (hours == 12)                   // if 12 AM midnight
        return 0;                       // 0000
     return hours;                      // 0100 - 1100
    }

 return hours;
}

//==============================================================================
// RTC Set the real RTC values.
//
// Stores the RTC values in the normal format as would be found inside
// the RTC IC.  rtcx is used as the working structure,  rtc contains the real
// time structure.
//
//   pass: void
// return: void
//==============================================================================
void rtc_setvalues (void)
{
 if (modelx.rtc)
    {
     memcpy(&rtc, &rtcx, sizeof(rtc));
     rtc.member.seconds = rtc_time_convert(rtcx.member.seconds);
     rtc.member.seconds_alarm = rtc_time_convert(rtcx.member.seconds_alarm);
     rtc.member.minutes = rtc_time_convert(rtcx.member.minutes);
     rtc.member.minutes_alarm = rtc_time_convert(rtcx.member.minutes_alarm);
     rtc.member.hours = rtc_hours_convert(rtcx.member.hours);
     rtc.member.hours_alarm = rtc_hours_convert(rtcx.member.hours_alarm);
     rtc.member.mday = rtc_time_convert(rtcx.member.mday);
     rtc.member.month = rtc_time_convert(rtcx.member.month);
     rtc.member.year = rtc_time_convert(rtcx.member.year);
     rtc.member.wday = rtc_time_convert(rtcx.member.wday);
    }
}

//==============================================================================
// RTC Initialise.
//
// Load the RTC file for the model being emulated if one exists.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int rtc_init (void)
{
 FILE *fp;
 long len;

 if (modelx.rtc)
    {
     if (strlen(modelc.systname))
        snprintf(name, sizeof(name), "%s%s-%s.rtc", userhome_rtcpath, model_args[emu.model], modelc.systname);
     else
        snprintf(name, sizeof(name), "%s%s.rtc", userhome_rtcpath, model_args[emu.model]);
     fp = fopen(name, "rb");

     if (fp != NULL)
        {
         fseek(fp, 0, SEEK_END);                // get size of the RTC file
         len = ftell(fp);
         fseek(fp, 0, SEEK_SET);                // back to start

         if (len != sizeof(rtcx))
            {
             xprintf("rtc_init: RTC file is incorrect size (delete and retry): %s\n", name);
             if ((modio.rtc) && (modio.level))
                fprintf(modio.log, "rtc_init: RTC file is incorrect size (delete and retry): %s\n", name);
             fclose(fp);
             return -1;
            }
         else
            {
             if (fread(&rtcx, sizeof(rtcx), 1, fp) == 1)
                rtcx.member.reg_d = RTC_D_VRT;
             else
                {
                 fclose(fp);
                 xprintf("rtc_init: Unable to read RTC data from %s\n", name);
                 return -1;
                }
            }
         fclose(fp);
        }
     else
        memset(&rtcx, 0, sizeof(rtcx));

     addr = 0;
     rtc_setclockfromhost();
     rtc_time_ref = time_get_ms();
     rtc_secs_before = 0;
    }
 return 0;
}

//==============================================================================
// RTC de-initialise.
//
// Before the RTC memory is saved some values need to be converted to the
// correct format as would be found inside the RTC IC.
//
//   pass: void
// return: int                          0
//==============================================================================
int rtc_deinit (void)
{
 FILE *fp;

 if (modelx.rtc)
    {
     rtc_setvalues();

     if (strlen(modelc.systname))
        snprintf(name, sizeof(name), "%s%s-%s.rtc", userhome_rtcpath, model_args[emu.model], modelc.systname);
     else
        snprintf(name, sizeof(name), "%s%s.rtc", userhome_rtcpath, model_args[emu.model]);
     fp = fopen(name, "wb");

     if (fp != NULL)
        {
         fwrite(&rtc, 1, sizeof(rtc), fp);
         fclose(fp);
         return 0;
        }
     else
        {
         xprintf("rtc_deinit: Unable to create RTC file: %s\n", name);
         if ((modio.rtc) && (modio.level))
            fprintf(modio.log, "rtc_deinit: Unable to create RTC file: %s\n", name);
         return -1;
        }
    }
 return 0;
}

//==============================================================================
// RTC reset.
//
//   pass: void
// return: int                          0
//==============================================================================
int rtc_reset (void)
{
 if (modelx.rtc)
    {
     rtcx.member.reg_b &= (0xff ^ (RTC_B_PIE | RTC_B_AIE | RTC_B_UIE | RTC_B_SQWE));
     rtcx.member.reg_c = 0;
    }
 return 0;
}

//==============================================================================
// RTC read register data - Port function
//
// RTC_A_UIP
// ---------
// See the rtc_clock() function information.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t register            register data
//==============================================================================
uint16_t rtc_r (uint16_t port, struct z80_port_read *port_s)
{
 uint8_t data;
 uint64_t cycles_now;
 int rtcpf;

 int p = port & 0x00ff; // remove the appended register value

 if (modelx.rtc)
    {
     switch (p & 0x0f)
        {
         case 0x04 :
            if (modio.rtc)
               log_port_1("rtc_r", "rtcreg", port, addr);
            return addr;
         case 0x06 :
            if (modio.rtc)
               log_port_1("rtc_r", "data", port, 0);
            return 0;
         case 0x07 :
            data = rtcx.ram[addr];
            if (addr < reg_a)
               {
                if ((addr == hours) || (addr == hours_alarm))
                   data = rtc_hours_convert(data);
                else
                   data = rtc_time_convert(data);
               }
            else
               if (addr == reg_a)
                  {
                   cycles_now = z80api_get_tstates();

                   if (! (rtcx.member.reg_b & RTC_B_SET))
                      {
                       if ((cycles_now % clocks_sec) > clocks_uip)
                          rtcx.member.reg_a |= RTC_A_UIP;
                       else
                          rtcx.member.reg_a &= (0xFF ^ RTC_A_UIP);
                      }

                   if (clocks_pf)
                      {
                       rtcpf = (cycles_now / clocks_pf);
                       if (rtcpf != rtcpf_before)
                          {
                           rtcpf_before = rtcpf;
                           rtcx.member.reg_c |= RTC_C_PF;
                          }
                      }

                   data = rtcx.member.reg_a;
                  }
               else
                  if (addr == reg_c)
                     rtcx.member.reg_c = 0;     // clear all flag bits
                  else
                     if (addr == reg_d)
                        rtcx.member.reg_d = RTC_D_VRT;  // set the Valid RAM and time bit

            if (modio.rtc)
               log_port_2("rtc_r", "rtcaddr", "rtcdata", port, addr, data);
            return data;
        }
    }
 return 0;
}

//==============================================================================
// RTC write register data - Port function
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void rtc_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 int p = port & 0x00ff; // remove the appended register value

 if (modelx.rtc)
    {
     switch (p & 0x0f)
        {
         case 0x04 :
            addr = (data & 0x3F);
            break;
         case 0x06 :
            if (addr < reg_a)
               {
                if ((addr == hours) || (addr == hours_alarm))
                   rtcx.ram[addr] = rtc_hours_convert_12to24(data);
                else
                   rtcx.ram[addr] = rtc_time_convert_bcdtobin(data);
                if (rtcx.member.year % 4)
                   days_in_month[1] = 28;
                else
                   days_in_month[1] = 29;
               }
            else
               if (addr == reg_a)
                  {
                   clocks_pf = (int)(periodic_interrupt_rate[data & B8(00001111)] * clocks_sec);
                   rtcx.ram[addr] &= RTC_A_UIP;
                   rtcx.ram[addr] |= (data & (0xff ^ RTC_A_UIP));
                   rtc_time_ref = time_get_ms();
                   rtc_secs_before = 0;
                  }
               else
                  if (addr == reg_b)
                     {
                      // if the SET bit goes from 0 to 1 reset the UIE flag
                      if (((rtcx.member.reg_b & RTC_B_SET) == 0) && (data & RTC_B_SET))
                         data &= (0xff ^ RTC_B_UIE);

                      // if the SET bit is 1 then clear the UIP flag
                      if (data & RTC_B_SET)
                         rtcx.member.reg_a &= (0xff ^ RTC_A_UIP);

                      rtcx.ram[addr] = data;
                     }
                  else
                     if (addr > reg_d)
                        rtcx.ram[addr] = data;
            break;
         case 0x07 :
            break;
        }
     if (modio.rtc)
        log_port_2("rtc_w", "rtcaddr", "rtcdata", port, addr, data);
    }
}

//==============================================================================
// RTC poll
//
// Read the host system time and check for any RTC flags that may need setting
// and any interrupts to be generated.
//
// This function should only be called from the pio_poll() function as only
// the current accumulated Z80 cycles are used here.
//
// The accuracy of the periodic flag is very dependent on the polling rate
// and emulated CPU clock rate.
//
//   pass: void
// return: int                          RTC interrupt status
//==============================================================================
int rtc_poll (void)
{
 int rtcpf;

 if (modelx.rtc)
    {
     if (rtc_timer_update_cycle())
        {
         if (! (rtcx.member.reg_b & RTC_B_SET))
            {
             // Check and set alarm flag and interrupts if time matches
             if (
                 (rtcx.member.seconds_alarm == rtcx.member.seconds) &&
                 (rtcx.member.minutes_alarm == rtcx.member.minutes) &&
                 (rtcx.member.hours_alarm == rtcx.member.hours)
                )
                {
                 rtcx.member.reg_c |= RTC_C_AF;      // set alarm flag
                 if (rtcx.member.reg_b & RTC_B_AIE)
                    rtcx.member.reg_c |= RTC_C_IRQF; // set IRQF flag
                }

            if (! (rtcx.member.reg_b & RTC_B_SET))
               rtcx.member.reg_c |= RTC_C_UF;   // set update ended flag

            if ((rtcx.member.reg_b & rtcx.member.reg_c) & RTC_B_UIE)
               rtcx.member.reg_c |= RTC_C_IRQF; // set IRQF flag
           }
        }

     // Check and set Periodic flag and interrupts
     if (clocks_pf)
        {
         rtcpf = (emu.z80_cycles / clocks_pf);
         if (rtcpf != rtcpf_before)
            {
             rtcpf_before = rtcpf;
             rtcx.member.reg_c |= RTC_C_PF;
             if ((rtcx.member.reg_b & rtcx.member.reg_c) & RTC_B_PIE)
                rtcx.member.reg_c |= RTC_C_IRQF;        // set IRQF flag
            }
        }

     // Return the interrupt status
     if (rtcx.member.reg_c & RTC_C_IRQF)
        return B8(10000000);
    }

 return 0;
}

//==============================================================================
// RTC register dump
//
// Dump the contents of the RTC registers.
//
//   pass: void
// return: void
//==============================================================================
void rtc_regdump (void)
{
 int i;
 char s[17];

 rtc_setvalues();

 xprintf("\n");
 xprintf("MC146818 RTC Registers             Hex  Dec    Binary\n");
 xprintf("------------------------------------------------------\n");

 for (i = 0; i < 14; i++)
    xprintf("0x%02x (%02dd) %-22s  %02x %5d %10s\n", i, i, rtc_regs_names[i],
    rtc.ram[i], rtc.ram[i], i2b(rtc.ram[i],s));
}

//==============================================================================
// RTC clock values - set values determined by the CPU clock frequency.
//
// RTC_A_UIP
// ---------
// The RTC_A_UIP (Update In Progress bit) is set 1984uS (@ 3375000Hz) before
// a 1 sec interval of Z80 CPU clock cycles.  1984uS is 6696 Z80 CPU clock
// cycles. The detection point is equal to 3375000 - 6696.
//
//   pass: int cpuclock         CPU clock frequency in Hz.
// return: void
//==============================================================================
void rtc_clock (int cpuclock)
{
 clocks_sec = cpuclock;
 clocks_uip = cpuclock - (int)((float)cpuclock * 0.001984);
}
