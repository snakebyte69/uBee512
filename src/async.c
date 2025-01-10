//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                            Asynchronous module                             *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// This module is used to provide asynchronous serial support for the RS232
// serial emulation.
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
// - Fixed async_configure() where memset(&options, sizeof(options), 0)
//   should have been memset(&options, 0, sizeof(options)).
//
// v4.0.0 - 13 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
//
// v3.1.0 - 22 April 2009, uBee
// - Removed all occurrences of console_output() function calls.
// - Changed all printf() calls to use a local xprintf() function.
//
// v1.4.0 - 26 September 2007, uBee
// - Added changes to error reporting.
//
// v1.3.0 - 1 September 2007, uBee
// - Created a new file and implement the asynchronous serial routines.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <SDL.h>

#ifdef MINGW
#include <windows.h>
#include <process.h>
#include <ctype.h>
#else
#include <fcntl.h>              // file control definitions
#include <errno.h>              // error number definitions
#include <termios.h>            // POSIX terminal control definitions
#endif

#include "async.h"
#include "ubee512.h"

//==============================================================================
// structures and variables
//==============================================================================
#ifdef MINGW
 int baudrate;
#else
 typedef struct termios termios_t;

 static termios_t options;
#endif

extern emu_t emu;

//==============================================================================
// Asynchronous Open
//
// Open the asynchronous serial device
//
//   pass: char *device                 name of serial device
// return: deschand_t                   file descriptor/handle, -1 if error
//==============================================================================
deschand_t async_open (char *device)
{
#ifdef MINGW
 deschand_t fd;                 // file handle for the port
 char device_x[256];
 int comsport;

 device_x[0] = 0;

// test if the user just passed a number for the port
 comsport = -1;
 sscanf(device, "%d", &comsport);
 if (comsport >= 0)             // if so then prepend "com" to the device
    {
     strcpy(device_x, "com");
     strcat(device_x, device);
    }
 else                           // else we just use what was passed
    strcpy(device_x, device);

 fd = CreateFile(device_x, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                 FILE_ATTRIBUTE_NORMAL, NULL);

 if (fd == INVALID_HANDLE_VALUE)
    return((deschand_t)-1);

 if (SetupComm(fd, SYST_RX_BUFSIZE, SYST_TX_BUFSIZE) == 0)
    return((deschand_t)-1);

 return fd;
#else
 deschand_t fd;                         // file descriptor for the port

 fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
 if (fd != -1)
    fcntl(fd, F_SETFL, O_NONBLOCK);
 return fd;
#endif
}

//==============================================================================
// Asynchronous Close.
//
// Close the asynchronous serial device
//
//   pass: deschand_t fd                file descriptor/handle
// return: int                          0 if success, -1 if error
//==============================================================================
int async_close (deschand_t fd)
{
#ifdef MINGW
 CloseHandle(fd);
 return 0;
#else
 close(fd);
 return 0;
#endif
}

#ifdef MINGW
#else
//==============================================================================
// Asynchronous baud speed value
//
//   pass: int baud                     baud rate
// return: speed_t                      Bx speed or 0 if not supported
//==============================================================================
static speed_t async_baud (int baud)
{
 switch (baud)
    {
     case    50: return B50;
     case    75: return B75;
     case   110: return B110;
     case   134: return B134;
     case   150: return B150;
     case   200: return B200;
     case   300: return B300;
     case   600: return B600;
     case  1200: return B1200;
     case  1800: return B1800;
     case  2400: return B2400;
     case  4800: return B4800;
     case  9600: return B9600;
     case 19200: return B19200;
     case 38400: return B38400;
        default:
                 {
                  xprintf("async_baud: Unsupported baud rate: %d\n", baud);
                  return 0;
                 }
    }
}
#endif

//==============================================================================
// Asynchronous configure
//
// Configure the asynchronous serial device
//
//   pass: deschand_t fd                file descriptor/handle
//         int baudtx                   TX baud rate
//         int baudrx                   RX baud rate
//         int data                     data bits
//         int stop                     stop bits
//         int hw                       hardware handshake
// return: int                          0 if no error, else -1
//==============================================================================
int async_configure (deschand_t fd, int baudtx, int baudrx, int data, int stop, int hw)
{
#ifdef MINGW
 COMMTIMEOUTS tout;
 DCB dcb;

// get the current serial port state
 if (GetCommState(fd, &dcb) == 0)
    return(-1);

 dcb.BaudRate = baudtx;         // use the TX rate for both TX/RX
 baudrate = baudtx;             // save a local copy for break signal generation
 dcb.ByteSize = data;
 dcb.Parity = 0;                // 0 = no parity
 if (stop == 1)
    dcb.StopBits = 0;           // 0 = 1 stop, 1=1.5, 2=2 stop bits
 else
    dcb.StopBits = 2;
 dcb.fAbortOnError = FALSE;
 if (hw)
    dcb.fOutxCtsFlow = TRUE;   // enable CTS control
 else
    dcb.fOutxCtsFlow = FALSE;  // disable CTS control

// set the new serial port state
 if (SetCommState(fd, &dcb) == 0)
    return(-1);

// get the current communication timeout values
 if (GetCommTimeouts(fd, &tout) == 0)
    return(-1);

 tout.ReadIntervalTimeout = MAXDWORD;
 tout.ReadTotalTimeoutMultiplier = 0;
 tout.ReadTotalTimeoutConstant = 0;
 tout.WriteTotalTimeoutMultiplier = 0;  // no write timeouts
 tout.WriteTotalTimeoutConstant = 0;

// set the new communication timeout values
 if (SetCommTimeouts(fd, &tout) == 0)
    return(-1);

 return 0;
#else
 speed_t baud_tx;
 speed_t baud_rx;

// we start with a cleared to zero options structure instead of using the
// current setup.  This way we only need to set what we want.

// clear all the options
 memset(&options, 0, sizeof(options));

// get the current options for the port
// tcgetattr(fd, &options);

// get the baud rate for TX
 baud_tx = async_baud(baudtx);
 if (baud_tx == 0)
    return -1;

// get the baud rate for RX
 baud_rx = async_baud(baudrx);
 if (baud_rx == 0)
    return -1;

// set baud rate for old and new termios methods
// options.c_cflag |= baud_tx;
// options.c_ospeed |= baud_tx;
// options.c_ispeed |= baud_rx;

 cfsetospeed(&options, baud_tx);
 cfsetispeed(&options, baud_rx);

// enable the receiver and set local mode
 options.c_cflag |= (CLOCAL | CREAD);

// select no parity, parity does not require emulation as this is handled
// by the Microbee application code itself.
 options.c_cflag &= ~PARENB;

// set the data size to 5-8 bits
 options.c_cflag &= ~CSIZE;     // mask the character size bits
 switch (data)
    {
     case 5 : options.c_cflag |= CS5;
              break;
     case 6 : options.c_cflag |= CS6;
              break;
     case 7 : options.c_cflag |= CS7;
              break;
     case 8 : options.c_cflag |= CS8;
              break;
    }

// set the number of stop bits to 1-2 bits
 if (stop == 2)
    options.c_cflag |= CSTOPB;
 else
    options.c_cflag &= ~CSTOPB;

// set/disable hardware handshaking for CTS/RTS
 if (hw)
    options.c_cflag |= CRTSCTS;         //CNEW_RTSCTS;
 else
    options.c_cflag &= ~CRTSCTS;        //CNEW_RTSCTS;

// set raw input in the local flags
 options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

// set the new options for the port
 tcflush(fd, TCIOFLUSH);
 tcsetattr(fd, TCSANOW, &options);

 return 0;
#endif
}

//==============================================================================
// Asynchronous read.
//
// Read 1 character from the asynchronous serial buffer.
//
//   pass: deschand_t fd                file descriptor/handle
// return: int                          serial character, or -1 if no character
//==============================================================================
int async_read (deschand_t fd)
{
#ifdef MINGW
 unsigned long BytesRead;
 uint8_t ch;

 ReadFile(fd, &ch, sizeof(ch), &BytesRead, NIL);
 if (BytesRead == 0)
    return -1;
 else
    return (int)ch;
#else
 uint8_t ch;
 int result;

 result = read(fd, &ch, sizeof(ch));

 if ((result == 0) || (result == -1))
    return -1;
 else
    return (int)ch;
#endif
}

//==============================================================================
// Asynchronous write.
//
// Write 1 character to the asynchronous serial device.
//
//   pass: deschand_t fd                file descriptor/handle
//         uint8_t ch                   character to write
// return: int                          number of bytes written, -1 if error
//==============================================================================
int async_write (deschand_t fd, uint8_t ch)
{
#ifdef MINGW
 unsigned long BytesWritten;

// wait for Win32 API to send the character
 while (WriteFile(fd, &ch, 1, &BytesWritten, NIL) == 0)
    {}
 return BytesWritten;
#else
 int result;

 do
    result = write(fd, &ch, sizeof(ch));
 while (result == 0);
 return result;
#endif
}

//==============================================================================
// Asynchronous write break.
//
// Under Windows
// -------------
// The CCITT modem recommendations require a break signal to be at minimum
// "2m+3" bits long, where the "m" represents the nominal number of bit
// times in one asynchronous byte, the minimum break period is then
// typically 23 bits, in this application we use 30.  The method used here
// will block but is not expected to be noticed in normal operations.
//
// Under POSIX
// -----------
// Send a break signal for 0.25 - 0.5 seconds
//
//   pass: deschand_t fd                file descriptor/handle
// return: void
//==============================================================================
void async_write_break (deschand_t fd)
{
#ifdef MINGW
 int delay;

 SetCommBreak(fd);
 delay = (int)(((float)1.0 / baudrate) * 30.0);
 SDL_Delay(delay);
 ClearCommBreak(fd);
#else
 tcsendbreak(fd, 0);
#endif
}
