//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                                log module                                  *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// This module is used for logging to stdout/OSD console and the log file.
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
// v4.6.0 - 4 May 2010, uBee
// - Changed log_port_*() functions to output PC address-2.
// - Added binary string output to 1 and 2 x data output functions.
// - Added a log_port_16() function to show 16 bit port values.
// - Renamed log_data() function to log_data_1().
// - Renamed log_port() function to log_port_1().
// - Added a log_port_0() function.
//
// v4.2.0 - 20 July 2009, uBee
// - Changed format specifiers for non port data from '0x%02x' to '0x%x' as
//   data may have a value greater than 0xFF.
//
// v4.1.0 - 22 June 2009, uBee
// - Created new module to implement the logging functionality.
//==============================================================================

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "log.h"
#include "ubee512.h"
#include "z80.h"
#include "z80api.h"
#include "support.h"

//==============================================================================
// structures and variables
//==============================================================================
extern modio_t modio;

//==============================================================================
// Log initialise.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int log_init (void)
{
 if (modio.level > 0)
    {
     modio.log = fopen(LOGFILE, "w");
     if (modio.log == NULL)
        {
         xprintf("log_init: Unable to write to log file\n");
         return -1;
        }
     fprintf(modio.log, "modio level = %i\n", modio.level);
    }

 return 0;
}

//==============================================================================
// Log de-initialise.
//
//   pass: void
// return: int                          0
//==============================================================================
int log_deinit (void)
{
 if (modio.level > 0)
    fclose(modio.log);

 return 0;
}

//==============================================================================
// log port 0.
//
// Reports: PC port mesg
//
//   pass: char *mesg                   message string
//         int port                     port number
// return: void
//==============================================================================
void log_port_0 (char *mesg, int port)
{
 z80regs_t z80regs;

 port &= 0xff;
 z80api_get_regs(&z80regs);

 xprintf("PC=0x%04x %s: port=0x%02x (%d)\n",
 z80regs.pc-2, mesg, port, port);
 if (modio.level)
    fprintf(modio.log, "PC=0x%04x %s: port=0x%02x (%d)\n",
    z80regs.pc-2, mesg, port, port);
}

//==============================================================================
// log port 1.
//
// Reports: PC port mesg: mesg1=data1
//
//   pass: char *mesg                   message string
//         char *mesg1                  message string 1 (=)
//         int port                     port number
//         int data1                    data
// return: void
//==============================================================================
void log_port_1 (char *mesg, char *mesg1, int port, int data1)
{
 z80regs_t z80regs;
 char s[17];

 port &= 0xff;
 z80api_get_regs(&z80regs);

 xprintf("PC=0x%04x %s: port=0x%02x (%d) %s=0x%02x (%d) %sB\n",
 z80regs.pc-2, mesg, port, port, mesg1, data1, data1, i2b(data1, s));
 if (modio.level)
    fprintf(modio.log, "PC=0x%04x %s: port=0x%02x (%d) %s=0x%02x (%d) %sB\n",
    z80regs.pc-2, mesg, port, port, mesg1, data1, data1, i2b(data1, s));
}

//==============================================================================
// log port 2.
//
// Reports: PC port mesg: mesg1=data1 mesg2=data2
//
//   pass: char *mesg                   message string
//         char *mesg1                  message string 1 (=)
//         char *mesg2                  message string 2 (=)
//         int port                     port number
//         int data1                    data
//         int data2                    data
// return: void
//==============================================================================
void log_port_2 (char *mesg, char *mesg1, char *mesg2, int port, int data1, int data2)
{
 z80regs_t z80regs;
 char s[17];
 char s2[17];

 port &= 0xff;
 z80api_get_regs(&z80regs);

 xprintf("PC=0x%04x %s: port=0x%02x (%d) %s=0x%02x (%d) %sB %s=0x%02x (%d) %sB\n",
 z80regs.pc-2, mesg, port, port, mesg1, data1, data1, i2b(data1, s), mesg2, data2, data2, i2b(data2, s2));
 if (modio.level)
    fprintf(modio.log, "PC=0x%04x %s: port=0x%02x (%d) %s=0x%02x (%d) %sB %s=0x%02x (%d) %sB\n",
    z80regs.pc-2, mesg, port, port, mesg1, data1, data1, i2b(data1, s), mesg2, data2, data2, i2b(data2, s2));
}

//==============================================================================
// log 16 bit port.
//
// Reports: PC port mesg: mesg1=data1
//
//   pass: char *mesg                   message string
//         char *mesg1                  message string 1 (=)
//         int port                     16 bit port number
//         int data1                    data
// return: void
//==============================================================================
void log_port_16 (char *mesg, char *mesg1, int port, int data1)
{
 z80regs_t z80regs;
 char s[17];

 z80api_get_regs(&z80regs);

 xprintf("PC=0x%04x %s: port (16b)=0x%04x (%d) %s=0x%02x (%d) %sB\n",
 z80regs.pc-2, mesg, port, port, mesg1, data1, data1, i2b(data1, s));
 if (modio.level)
    fprintf(modio.log, "PC=0x%04x %s: port (16b)=0x%04x (%d) %s=0x%02x (%d) %sB\n",
    z80regs.pc-2, mesg, port, port, mesg1, data1, data1, i2b(data1, s));
}

//==============================================================================
// log data 1.
//
// Reports: PC mesg: mesg1=data1
//
//   pass: char *mesg                   message string
//         char *mesg1                  message string 1 (=)
//         int data1                    data
// return: void
//==============================================================================
void log_data_1 (char *mesg, char *mesg1, int data1)
{
 z80regs_t z80regs;
 char s[17];

 z80api_get_regs(&z80regs);

 xprintf("PC=0x%04x %s: %s=0x%x (%d) %sB\n",
 z80regs.pc, mesg, mesg1, data1, data1, i2b(data1, s));
 if (modio.level)
    fprintf(modio.log, "PC=0x%04x %s: %s=0x%x (%d) %sB\n",
    z80regs.pc, mesg, mesg1, data1, data1, i2b(data1, s));
}

//==============================================================================
// log data 2.
//
// Reports: PC mesg: mesg1=data1 mesg2=data2
//
//   pass: char *mesg                   message string
//         char *mesg1                  message string 1 (=)
//         char *mesg2                  message string 2 (=)
//         int data1                    data
//         int data2                    data
// return: void
//==============================================================================
void log_data_2 (char *mesg, char *mesg1, char *mesg2, int data1, int data2)
{
 z80regs_t z80regs;
 char s[17];
 char s2[17];

 z80api_get_regs(&z80regs);

 xprintf("PC=0x%04x %s: %s=0x%x (%d) %sB %s=0x%x (%d) %sB\n",
 z80regs.pc, mesg, mesg1, data1, data1, i2b(data1, s), mesg2, data2, data2, i2b(data2, s2));
 if (modio.level)
    fprintf(modio.log, "PC=0x%04x %s: %s=0x%x (%d) %sB %s=0x%x (%d) %sB\n",
 z80regs.pc, mesg, mesg1, data1, data1, i2b(data1, s), mesg2, data2, data2, i2b(data2, s2));
}

//==============================================================================
// log data 3.
//
// Reports: PC mesg: mesg1=data1 mesg2=data2 mesg3=data3
//
//   pass: char *mesg                   message string
//         char *mesg1                  message string 1 (=)
//         char *mesg2                  message string 2 (=)
//         char *mesg3                  message string 3 (=)
//         int data1                    data
//         int data2                    data
//         int data3                    data
// return: void
//==============================================================================
void log_data_3 (char *mesg, char *mesg1, char *mesg2, char *mesg3, int data1,
     int data2, int data3)
{
 z80regs_t z80regs;

 z80api_get_regs(&z80regs);

 xprintf("PC=0x%04x %s: %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d)\n",
 z80regs.pc, mesg, mesg1, data1, data1, mesg2, data2, data2, mesg3, data3, data3);
 if (modio.level)
    fprintf(modio.log, "PC=0x%04x %s: %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d)\n",
 z80regs.pc, mesg, mesg1, data1, data1, mesg2, data2, data2, mesg3, data3, data3);
}

//==============================================================================
// log data 4.
//
// Reports: PC mesg: mesg1=data1 mesg2=data2 mesg3=data3 mesg4=data4
//
//   pass: char *mesg                   message string
//         char *mesg1                  message string 1 (=)
//         char *mesg2                  message string 2 (=)
//         char *mesg3                  message string 3 (=)
//         char *mesg4                  message string 4 (=)
//         int data1                    data
//         int data2                    data
//         int data3                    data
//         int data4                    data
// return: void
//==============================================================================
void log_data_4 (char *mesg, char *mesg1, char *mesg2, char *mesg3, char *mesg4,
     int data1, int data2, int data3, int data4)
{
 z80regs_t z80regs;

 z80api_get_regs(&z80regs);

 xprintf("PC=0x%04x %s: %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d)\n",
 z80regs.pc, mesg, mesg1, data1, data1, mesg2, data2, data2, mesg3, data3, data3,
 mesg4, data4, data4);
 if (modio.level)
    fprintf(modio.log, "PC=0x%04x %s: %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d)\n",
 z80regs.pc, mesg, mesg1, data1, data1, mesg2, data2, data2, mesg3, data3, data3,
 mesg4, data4, data4);
}

//==============================================================================
// log data 5.
//
// Reports: PC mesg: mesg1=data1 mesg2=data2 mesg3=data3 mesg4=data4 mesg5=data5
//
//   pass: char *mesg                   message string
//         char *mesg1                  message string 1 (=)
//         char *mesg2                  message string 2 (=)
//         char *mesg3                  message string 3 (=)
//         char *mesg4                  message string 4 (=)
//         char *mesg5                  message string 5 (=)
//         int data1                    data
//         int data2                    data
//         int data3                    data
//         int data4                    data
//         int data5                    data
// return: void
//==============================================================================
void log_data_5 (char *mesg, char *mesg1, char *mesg2, char *mesg3, char *mesg4,
     char *mesg5, int data1, int data2, int data3, int data4, int data5)
{
 z80regs_t z80regs;

 z80api_get_regs(&z80regs);

 xprintf("PC=0x%04x %s: %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d)\n",
 z80regs.pc, mesg, mesg1, data1, data1, mesg2, data2, data2, mesg3, data3, data3,
 mesg4, data4, data4, mesg5, data5, data5);
 if (modio.level)
    fprintf(modio.log, "PC=0x%04x %s: %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d) %s=0x%x (%d)\n",
 z80regs.pc, mesg, mesg1, data1, data1, mesg2, data2, data2, mesg3, data3, data3,
 mesg4, data4, data4, mesg5, data5, data5);
}

//==============================================================================
// log message.
//
// Reports: mesg
//
//   pass: char *mesg                   message string
// return: void
//==============================================================================
void log_mesg (char *mesg)
{
 xprintf("%s\n", mesg);
 if (modio.level)
    fprintf(modio.log, "%s\n", mesg);
}
