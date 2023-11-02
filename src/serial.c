//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              Serial module                                 *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// This module is used to emulate the standard serial port.  It will require
// that the emulator is configured to match the settings used in a Microbee
// application.
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
// v5.7.0 - 9 March 2015, uBee
// - Changes to serial_config() to allow 4 and 6.750 MHz clock in calculation.
//
// v4.2.0 - 13 July 2009, uBee
// - Added the serial_write() function, other peripherals may be added to
//   this later.
//
// v4.0.0 - 29 May 2009, uBee
// - Changes made to serial receive code.
// - Added conditional message reporting based on --verbose option.
//
// v3.1.0 - 22 April 2009, uBee
// - Removed all occurrences of console_output() function calls.
// - Added serial_open(), and serial_close() functions and reorganised
//   opening and closing of serial devices.
// - Changed all printf() calls to use a local xprintf() function.
//
// v2.8.0 - 12 August 2008, uBee
// - Added type cast to default assignment: deschand_t coms1 = (deschand_t)-1
// - Changed documentation references from XTAL to CPU clock.
//
// v2.7.0 - 22 May 2008, uBee
// - Added structure emu_t and moved variables into it.
//
// v2.6.0 - 11 May 2008, uBee
// - Serial variables have been placed into structure serial_t.
//
// v1.4.0 - 26 September 2007, uBee
// - Added changes to error reporting.
//
// v1.3.0 - 1 September 2007, uBee
// - Created a new file and implement the serial emulation at the top level.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef MINGW
#include <windows.h>
#include <process.h>
#include <ctype.h>
#else
#endif

#include "serial.h"
#include "ubee512.h"
#include "z80api.h"
#include "pio.h"
#include "async.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
serial_t serial =
{
 .tx_baud = SERIAL_TX_BAUD,
 .rx_baud = SERIAL_RX_BAUD,
 .databits = SERIAL_DATABITS,
 .stopbits = SERIAL_STOPBITS,
 .byte_rx = -1
};

deschand_t coms1 = (deschand_t)-1;

static int serialrx = -1;
static uint64_t cycles_before_rx;
static uint64_t serial_intr_tstate;
static int serial_divval_rx;
static int serial_interrupt;
static int serial_saved_rx;

static uint64_t cycles_before_tx;
static uint8_t serial_before_tx=SERIAL_STOPBIT_TX;
static uint8_t serial_byte_tx;
static int serial_bitcount_tx;
static int serial_status_tx;
static int serial_divval_tx;

extern emu_t emu;
extern pio_t pio_b;

//==============================================================================
// Serial Initialise.
//
// Open the host serial device
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int serial_init (void)
{
 return serial_open(serial.coms1, 0, 1);
}

//==============================================================================
// Serial de-initialise.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int serial_deinit (void)
{
 return serial_close(0);
}

//==============================================================================
// Serial reset.
//
//   pass: void
// return: int                          0
//==============================================================================
int serial_reset (void)
{
 if (coms1 != (deschand_t)-1)
    serialrx = -1;

 serial_interrupt = 0;
 serial_intr_tstate = 0;

 return 0;
}

//==============================================================================
// Serial open.
//
// Open a serial port. Only one serial port is supported at present.
//
//   pass: char *s                      host serial port
//         int port                     always 0
//         int action                   0 saves port name only, 1 opens serial
// return: int                          0 if success, -1 if error
//==============================================================================
int serial_open (char *s, int port, int action)
{
 strcpy(serial.coms1, s);

 if (action == 0)
    return 0;

 serial_close(0);

 if (serial.coms1[0])
    {
     coms1 = async_open(serial.coms1);
     serialrx = -1;
     if (coms1 == (deschand_t)-1)
        {
         xprintf("serial_open: Failed to open serial device: %s\n",
         serial.coms1);
         return -1;
        }
     else
        serial_config(emu.cpuclock);
    }
 return 0;
}

//==============================================================================
// Serial close.
//
// Close a serial port. Only one serial port is supported at present.
//
//   pass: int port                     always 0
// return: int                          0 if success, -1 if error
//==============================================================================
int serial_close (int port)
{
 int res;

 if (coms1 != (deschand_t)-1)
    {
     res = async_close(coms1);
     coms1 = (deschand_t)-1;
     return res;
    }
 return 0;
}

//==============================================================================
// Serial read poll.
//
//   pass: void
// return: int                          serial character or -1 if none ready
//==============================================================================
static int serial_readpoll (void)
{
 int c;

 if ((coms1 != (deschand_t)-1) && (serial.byte_rx == -1))
    {
     // reset the last serial rx byte so that start of the data is seen.
     if (serial_interrupt)
        {
         c = serial_saved_rx;
         serial_interrupt = 0;
         serial.byte_rx = -1;
         return c;
        }

     serial_saved_rx = async_read(coms1);
     return serial_saved_rx;
    }

 return -1;
}

//==============================================================================
// Serial write.
//
//   pass: int c
// return: void;
//==============================================================================
static void serial_write (void)
{
 // send out our emulated TX byte (time shifted by 1 byte time)
 if (serial_bitcount_tx == serial.databits)
    async_write(coms1, serial_byte_tx);
 else
    {
     if (emu.verbose)
        xprintf("serial_w: Break signal sent, serial_bitcount_tx=%d\n",
        serial_bitcount_tx);
     async_write_break(coms1);
    }
}

//==============================================================================
// Serial interrupt adjust.
//
// This gets called when a serial interrupt has been detected in the PIO
// module.  It's used to adjust the time detection (tstates) on a start bit
// as the PIO is only polled periodically and requires the serial byte to be
// reset when reading the serial RX line.
//
//   pass: void
// return: void
//==============================================================================
void serial_interrupt_adjust (void)
{
 if (! serial_interrupt)
    {
     serial_interrupt = 1;
     serial.byte_rx = -1;
     serial_intr_tstate = z80api_get_tstates();
    }
}

//==============================================================================
// Serial read.
//
// Once a character has started to be rotated out, the serial.byte_rx
// variable will no longer be -1 until the rotating process has completed.
// It should be kept in mind that this process can be started by any read of
// this port for other unrelated reasons and this is fine and it all works
// as if it was a real Microbee.
//
// This works because the data bits are based on the current Z80 tstate
// count and not the number of calls made to this function and so is not
// robbing any other calls of any data.
//
// The Microbee port does not invert data for RX.
//
//   pass: void
// return: int                          serial input bit value
//==============================================================================
int serial_r (void)
{
 int count;

 static uint64_t cycles_now_rx;
 static uint64_t cycles_elapsed_rx;

 // exit if no where to receive the data from
 if (coms1 == (deschand_t)-1)
    return PIO_B_RS232_CTS;  // stop bit, and CTS always true for now

 cycles_now_rx = z80api_get_tstates();
 cycles_elapsed_rx = cycles_now_rx - cycles_before_rx;

 // calculate the number of serial bit times we have so far
 count = cycles_elapsed_rx / serial_divval_rx;

 if (serial.byte_rx == -1)
    {
     serial.byte_rx = serial_readpoll();

     if (serial.byte_rx != -1)          // if we have some data to work with
        {
         cycles_before_rx = cycles_now_rx;

         // this re-adjusts the tstate time for a start bit when an
         // interrupt occurs
         if (serial_intr_tstate)
            {
             cycles_before_rx -= (z80api_get_tstates() - serial_intr_tstate);
             serial_intr_tstate = 0;
            }

         serial.byte_rx ^= 0xff;
         serial.byte_rx &= ((1 << serial.databits) - 1); // keep only n data bits
         serial.byte_rx = (serial.byte_rx << 1) | 0x01;  // add in the start bit
         count = 0;             // start from the beginning of the character
        }
    }

 // return stop bits if no data is ready
 if (serial.byte_rx == -1)
    return PIO_B_RS232_CTS;     // stop bit, and CTS always true for now

 // return start and data bits depending on count value
 if (count < (serial.databits + 1))
    return (((serial.byte_rx >> count) & 0x01) << 4) |
    PIO_B_RS232_CTS; // get count pos bit

 // return stop bit(s) depending on count value
 if (count < (serial.databits + 3))
    return PIO_B_RS232_CTS;     // stop bit(s), and CTS always true for now

 // end of the character and stop bit(s), reset for a new character
 serial.byte_rx = -1;
 return PIO_B_RS232_CTS;        // stop bit, and CTS always true for now
}

//==============================================================================
// Serial write.
//
// The Microbee port inverts data for TX in hardware after the PIO.
//
//   pass: int data
// return: void
//==============================================================================
void serial_w (uint8_t data)
{
 int count;
 uint8_t serial_bit;

 static uint8_t serial_now_tx;

 static uint64_t cycles_now_tx;
 static uint64_t cycles_elapsed_tx;

 // exit if no where to send the data to
 if (coms1 == (deschand_t)-1)
   return;

 // mask out the serial output bit
 serial_now_tx = (data & PIO_B_RS232_TX) >> 5;

 cycles_now_tx = z80api_get_tstates();
 cycles_elapsed_tx = cycles_now_tx - cycles_before_tx;

 // determine the number of bit times we have so far
 count = cycles_elapsed_tx / serial_divval_tx;

 // if we are after a start bit for a new byte
 if (serial_status_tx == 0)      // if a start bit is expected
    {
     cycles_before_tx = cycles_now_tx;
     if (serial_now_tx == SERIAL_STARTBIT_TX)
        {
         serial_status_tx++;     // want start bit delay
         serial_byte_tx = 0;
         serial_bitcount_tx = 0;
         serial_before_tx = serial_now_tx;
        }
     return;
    }

 // must wait for at least 1 bit time to occur
 if (! count)
    return;

 // if we are after the start bit time
 if (serial_status_tx == 1)      // if a start bit time is needed
    {
     cycles_before_tx = cycles_now_tx;
     serial_status_tx++;     // want data bits

     if (serial_now_tx != serial_before_tx)
        serial_before_tx = serial_now_tx;
     return;
    }

 if (serial_status_tx == 2)      // if data building in progress
    {
     cycles_before_tx = cycles_now_tx;

     serial_bit = serial_before_tx;

     if (serial_now_tx != serial_before_tx)
         serial_before_tx = serial_now_tx;

     while ((count) && (serial_bitcount_tx != serial.databits))
        {
         serial_byte_tx |= (serial_bit << serial_bitcount_tx);
         serial_bitcount_tx++;
         count--;
        }

     if (serial_bitcount_tx >= serial.databits)
        {
         serial_status_tx = 0;     // want start bit

         serial_write();
        }
    }
}

//==============================================================================
// Serial configuration.
//
// The divider for TX is reduced by 5% and is required by the code section that
// uses it,  a 3% value is not enough.
//
// TAKE NOTE: THE ABOVE IS PROBABLY DUE TO ROUNDING ERRORS.
// TO BE LOOKED AT AGAIN.
//
// The calcuations are always based on either a 3.375 or 2 MHz CPU clock as
// these were the original speeds.  The correct application software is
// required for each CPU clock frequency.  i.e. Old 2 MHz requires old BASIC.
//
//   pass: int cpuclock         CPU clock frequency in Hz.
// return: void
//==============================================================================
void serial_config (int cpuclock)
{
 if (coms1 != (deschand_t)-1)
    {
     if (cpuclock != 2000000 && cpuclock != 4000000 && cpuclock != 6750000)
        cpuclock = 3375000;
        
     serial_divval_tx = (int)((float)cpuclock *
     ((float)(1.0)/serial.tx_baud) * (95.0/100.0));
     serial_divval_rx = (int)((float)cpuclock *
     ((float)(1.0)/serial.rx_baud));

     async_configure(coms1, serial.tx_baud, serial.rx_baud,
     serial.databits, serial.stopbits, 0);
    }
}
