//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              function module                               *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Built in functions able to be called from Z80 code.
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
// v6.0.0 - 1 January 2017, K Duckmanton
// - Microbee memory is now an array of uint8_t rather than char, all
//   pointers to it must also be uint8_t*.
//
// v5.3.0 - 2 April 2011, uBee
// - Changed function_ubee_w() to allow commands 0xf0-0xff to be used
//   without having to pass a 2 byte data structure address, these commands
//   use the new function_nostruct() function.
// - Added emulation functions to function_nostruct() for reading and
//   writing to the new TAP file support.
//
// v5.0.0 - 4 August 2010, uBee
// - Replaced constant reset action numbers with EMU_RST_* defines.
// v5.0.0 - 13 July 2010, K Duckmanton
// - Removed all references to the 'sound' global variable and replaced them
//   with references to the 'audio' global instead.
//
// v4.6.0 - 28 May April 2010, uBee
// - Replaced z80debug_dump() function call with z80debug_dump_lines().
// - Renamed log_port() calls to log_port_1().
// - Fix bug in function_directory() (return the next directory entry (with
//   wildcards)) to initialise fx.fp.d with f->file.fp.d. (workerbee)
//
// v4.5.0 - 19 August 2009, uBee
// - Moved all functions not matching function_*() over to new support.c
//   module.
// - Changes to function_directory() to functions 0x00 and 0x02 so that dir
//   open and dir read uses new modified code now in support.c
//
// v4.1.0 - 22 June 2009, uBee
// - Fixed a Big Endian issue. 'leu16_to_host(ubee_addr | (data << 8))'
//   should have been just 'ubee_addr | (data << 8)' in function_ubee_w()
// - Made improvements to the logging process.
//
// v4.0.0 - 7 June 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Changes made to access the current Z80 DRAM/SRAM memory as the memory
//   management method has completely changed.
// - More Endian issues found and corrected (needed for Big Endian HW)
// - Changed MD5 file matching to keep trying other matching MD5 entries when
//   the file name is not found.
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
//
// v3.1.0 - 20 April 2009, uBee
// - Fixed swap_endian() function, was doing nothing!
// - Added create_md5() function.
// - Added optional MD5 lookup for ROMs and disks in the 'roms.alias' and
//   'disks.alias' files with new function find_file_alias().
// - Moved functions function_emu_command(), function_repeat_start(),
//   function_repeat_stop() and function_update() to keyb_*() in keyb.c.
// - Added time_get_secs() function to return timer ticks in seconds.
// - Changed all printf() calls to use a local xprintf() function.
// - Added EMU_CMD_CONSOLE function to enter console mode.
// - Changes to stop OpenGL functions being called when using SDL video mode.
// - Added OpenGL conditional code compilation using USE_OPENGL.
//
// v3.0.0 - 10 October 2008, uBee
// - Changed file_readline() function to remove leading spaces.
// - Added EMU_CMD_MWHEEL function to select mouse wheel association.
// - Added EMU_CMD_SCREENI and EMU_CMD_SCREEND  to function_emu_command()
// - Added EMU_CMD_VIDSIZE2 function to set screen size by set amount.
// - Added EMU_CMD_GL_FILTER toggling of OpenGL filter mode.
// - Changed the gui_toggledisplay() function to video_toggledisplay() as
//   code has moved to a new video.c module.
// - Added time_get_ms() function and removed repeated timer code.
// - Added time_delay_ms() function.
//
// v2.8.0 - 1 September 2008, uBee
// - Added new functions function_repeat_start(), function_repeat_stop() and
//   function_update().  These are used to make repeated calls to the
//   function_emu_command() function.
// - Added EMU_CMD_VOLUMEI and EMU_CMD_VOLUMED to function_emu_command()
// - Changes to the way gui_status_update() is called.
// - Added EMU_CMD_PAUSE to function_emu_command function.  Debug modes
//   modified to not activate while in a paused state.
// - Added function 0x06 to return the --file-list string count.
// - Added function 0x10 to set the current --file-list string position.
// - Modified function 0x00 function to return the current string for the
//   --file-list option parameter string. Compatible with earlier applications
//   that made use of this function.
// - Cleaned up some code that was producing 2 warnings.
//
// v2.7.0 - 27 June 2008, uBee
// - Added function_basic() functions.
// - Added a function_application() functions.
// - Added find_file_entry() function to find what file to use for file name
//   aliases defined in a configuration file.
// - Moved options_readline() function in options.c module over and is now
//   file_readline() function, with file pointer, and string size passed.
// - Added open_file() function.  Opens/creates/tests files, this also
//   provides alternate path checking and allows slash characters to be
//   win32 or unix style.
// - Added structure emu_t and moved variables into it.
// - Added structure func_t and moved variables into it.
// - Added EMU_CMD_MUTE to function_emu_command() function.
// - Added functions to function_control() to enable video and sound.
// - Added sound_t and crtc_t structure.
//
// v2.6.0 - 11 May 2008, uBee
// - Added a function_joystick() functions.
// - Added toupper_string() function.
// - Moved the string_search() function over from the options.c module.
// - Fixed an endianess issue in function_stdio() function for the 'putchar
//   function'.
// - File name matching in win32 fixed in function "case 0x02 return the next
//   directory entry (with wildcards)". Was only able to match file names
//   that used all lower case. CP/M support program HOST2CPM should now work.
//
// v2.3.0 - 23 January 2007, uBee
// - Added function_emu_command() function.
// - Added printf, hex and decimal output functions to the stdio group.
// - Added dump function to diagnostics group.
// - Added additional functions to status group.
// - Some function values that are intended to return 0 or 1 were missing
//   the little_endian conversion, so 1 would have returned as some other non
//   zero value on big endian platforms. If the test in an application was for
//   a non zero value then there won't be a problem.  Constant values of 0 now
//   also use the little_endian conversion for consistency.
// - Added modio_t debug_t and regdump_t structures.
//
// v2.2.0 - 11 December 2007, uBee
// - Added status, control, keyboard, stdio, and file and directory functions.
// - Changed the method for Application and Version reporting.
// - Added a Z80 memory dump function to stdio.
//
// v1.4.0 - 22 September 2007, uBee
// - Added copy_file() function.
//
// v1.2.0 - 19 August 2007, uBee
// - Created new module to implement the emulator function ports.
//==============================================================================

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>

#ifdef MINGW
#include <windows.h>
#else
#include <sys/types.h>          // various type definitions, like pid_t
#include <sys/time.h>
#endif
#include <stdint.h>

#include "ubee512.h"
#include "audio.h"
#include "function.h"
#include "support.h"
#include "z80debug.h"
#include "crtc.h"
#include "joystick.h"
#include "tapfile.h"

//==============================================================================
// structures and variables
//
// NOTE:
// - All Z80 structure members are in Little Endian format.  This requires
//   conversion to and from the host Endian format.  The exception is:
//
//   f->file.fp.d
//
//   This is a file pointer and is used in the host's native format.
//
// - Access to memory locations that cross a 0x8000 Z80 memory bank will not
//   work except for fread and fwrite functions which use local buffering
//   and copying data to/from the correct banks.
//==============================================================================
func_t func =
{
 .file_exit = 1
};

static const char emulator_id_str[] = APPIDSTR;  // defined in the Makefile
static const char emulator_ver_str[] = APPVER;   // defined in the Makefile
static int emulator_id_pos;

static int ubee_req;
static int ubee_addr;
static int ubee_command;

static uint8_t *z80mem_f;
static uint8_t *z80mem_x1;
static uint8_t *z80mem_x2;
static uint8_t *z80mem_x3;
static uint8_t *z80mem_x4;

extern emu_t emu;
extern audio_t audio;
extern const model_t model_data[];
extern model_t modelx;
extern modio_t modio;
extern debug_t debug;
extern regdump_t regdump;
extern joystick_t joystick;
extern crtc_t crtc;

//==============================================================================
// Function Initialise
//
//   pass: void
// return: int                  0
//==============================================================================
int function_init (void)
{
 ubee_req = 0;
 return 0;
}

//==============================================================================
// Function de-initialise
//
//   pass: void
// return: int                  0
//==============================================================================
int function_deinit (void)
{
 return 0;
}

//==============================================================================
// Function reset
//
//   pass: void
// return: int                  0
//==============================================================================
int function_reset (void)
{
 ubee_req = 0;
 return 0;
}

//==============================================================================
// uBee512 read - Special Ubee512 Port function
//
// This function allows a Z80 program to identify that it is running in the
// uBee512 emulator.  This is provided so that some extra functionality can
// be taken advantage off and must be called and checked before using any
// special uBee512 write function calls to prevent unknown results on a real
// Microbee.
//
// This port call just returns the program name as a string one character at
// a time in a circular loop, the APPIDSTR in the Makefile ultimately
// determines the name to be used.
//
// Version information can be returned by a using a status function call.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t             returns a null terminated string in a loop
//==============================================================================
uint16_t function_ubee_r (uint16_t port, struct z80_port_read *port_s)
{
 char c;

 c = emulator_id_str[emulator_id_pos++];
 if (! c)
    emulator_id_pos = 0;

 if (modio.func)
    log_port_1("function_ubee_r", "id char", port, c);

 return (uint16_t)(c);
}

//==============================================================================
// Status functions
//
//   pass: int addr             address of Z80 structure (relative to bank)
// return: void
//==============================================================================
static void function_status (int addr)
{
 int i;
 int x;
 char vers[6];
 int res;

 ub_func_u *f = (ub_func_u*)(z80mem_f + addr);

 switch (leu16_to_host(f->cmdres.cmd))
    {
     case 0x00 : // Get version information
        i = 0;

        x = 5;
        vers[0] = 0;
        while (isdigit(emulator_ver_str[i]) && x--)
           strncat(vers, &emulator_ver_str[i++], 1);
        sscanf(vers, "%d", &res);
        f->version.ver1 = host_to_leu16(res);
        i++;

        x = 5;
        vers[0] = 0;
        while (isdigit(emulator_ver_str[i]) && x--)
           strncat(vers, &emulator_ver_str[i++], 1);
        sscanf(vers, "%d", &res);
        f->version.ver2 = host_to_leu16(res);
        i++;

        x = 5;
        vers[0] = 0;
        while (isdigit(emulator_ver_str[i]) && x--)
           strncat(vers, &emulator_ver_str[i++], 1);
        sscanf(vers, "%d", &res);
        f->version.ver3 = host_to_leu16(res);
        i++;
        break;

     case 0x01 : // Get host system information
#ifdef MINGW
        f->cmdres.res = host_to_leu16(0);       // win32
#else
        f->cmdres.res = host_to_leu16(1);       // unices
#endif
        break;
     case 0x02 : // Get host integer size
        f->cmdres.res = host_to_leu16(sizeof(i));
        break;
     case 0x03 : // Get host endian format
        f->cmdres.res = host_to_leu16(IsBigEndian());
        break;

     case 0x10 : // Get model number emulated (subject to change)
        f->cmdres.res = host_to_leu16(emu.model);
        break;
     case 0x11 : // Get alpha+ model emulation status
        f->cmdres.res = host_to_leu16(modelx.alphap);
        break;
     case 0x12 : // Get ROM model emulation status
        f->cmdres.res = host_to_leu16(modelx.rom);
        break;
     case 0x13 : // Get model boot address status
        f->cmdres.res = host_to_leu16(modelx.bootaddr);
        break;
     case 0x14 : // Get RAM emulation size
        f->cmdres.res = host_to_leu16(modelx.ram);
        break;
     case 0x15 : // Get colour emulation status
        f->cmdres.res = host_to_leu16(modelx.colour);
        break;
     case 0x16 : // Get hardware flash emulation status
        f->cmdres.res = host_to_leu16(modelx.hwflash);
        break;
     case 0x17 : // Get light pen keys emulation status
        f->cmdres.res = host_to_leu16(modelx.lpen);
        break;
     case 0x18 : // Get speed switch emulation status
        f->cmdres.res = host_to_leu16(modelx.speed);
        break;
     case 0x19 : // Get speed piob7 emulation status
        f->cmdres.res = host_to_leu16(modelx.piob7);
        break;
     case 0x1A : // Get RTC emulation status
        f->cmdres.res = host_to_leu16(modelx.rtc);
        break;
     default :
        break;
    }
}

//==============================================================================
// Control functions
//
//   pass: int addr             address of Z80 structure (relative to bank)
// return: void
//==============================================================================
static void function_control (int addr)
{
 ub_func_u *f = (ub_func_u*)(z80mem_f + addr);

 if (leu16_to_host(f->cmdres.id) == 0xAA55)
    {
     f->file.id = host_to_leu16(0);  // clear it for next time (must be set each time)
     switch (leu16_to_host(f->cmdres.cmd))
        {
         case 0x00 :
            emu.reset = EMU_RST_RESET_NOW;
            break;
         case 0x01 :
            emu.done = 1;
            break;
         case 0x02 : // turn the video on
            crtc.video = 1;
            break;
         case 0x03 : // turn the sound on
            audio.mute = 0;
            break;
         case 0x04 :
            set_clock_speed(emu.cpuclock_def, emu.z80_divider, 0);
            break;
         default :
            break;
        }
    }
}

//==============================================================================
// Diagnostics functions
//
//   pass: int addr             address of Z80 structure (relative to bank)
// return: void
//==============================================================================
static void function_diagnostics (int addr)
{
 int addr1;
 int lines;
 int flags;

 ub_func_u *f = (ub_func_u*)(z80mem_f + addr);

 if (leu16_to_host(f->cmdres.id) == 0xAA55)
    {
     f->file.id = host_to_leu16(0);  // clear it for next time (must be set each time)
     switch (leu16_to_host(f->cmdres.cmd))
        {
         case 0x00 :
            addr1 = leu16_to_host(f->dump.addr);
            lines = leu16_to_host(f->dump.lines);
            flags = leu16_to_host(f->dump.htype);
            z80debug_dump_lines(NULL, addr1, lines, flags);
            break;
         default :
            break;
        }
    }
}

//==============================================================================
// Keyboard functions
//
//   pass: int addr             address of Z80 structure (relative to bank)
// return: void
//==============================================================================
static void function_keyboard (int addr)
{
// ub_func_u *f = (ub_func_u*)(z80mem_f + addr);
}

//==============================================================================
// stdio functions
//
//   pass: int addr             address of Z80 structure (relative to bank)
// return: void
//==============================================================================
static void function_stdio (int addr)
{
 int addr1;

 ub_func_u *f = (ub_func_u*)(z80mem_f + addr);

 switch (leu16_to_host(f->cmdres.cmd))
    {
     case 0x00 : // getchar function
        f->getchar.res = host_to_le16(getchar());
        break;
     case 0x01 : // putchar function
        f->putchar.res = host_to_le16(putchar(le16_to_host(f->putchar.val)));
        break;
     case 0x02 : // puts function
        addr1 = leu16_to_host(f->puts.addr);
        z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
        f->puts.res = host_to_le16(puts((char*)(z80mem_x1+addr1)));
        break;
     case 0x03 : // printf function (no variables)
        addr1 = leu16_to_host(f->puts.addr);
        z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
        xprintf("%s", (char*)(z80mem_x1+addr1));
        break;

     case 0x80 : // write 16 bit value as signed decimal
        xprintf("%d", le16_to_host(f->putchar.val));
        break;
     case 0x81 : // write 16 bit value as unsigned decimal
        xprintf("%ud", leu16_to_host(f->putchar.val));
        break;
     case 0x82 : // write 16 bit value as 2 hex characters
        xprintf("%02x", leu16_to_host(f->putchar.val));
        break;
     case 0x83 : // write 16 bit value as 4 hex characters
        xprintf("%04x", leu16_to_host(f->putchar.val));
        break;

     default :
        break;
    }
}

//==============================================================================
// File functions.
//
//   pass: int addr             address of Z80 structure (relative to bank)
// return: void
//==============================================================================
static void function_files (int addr)
{
 int amount_t;
 int amount_l;
 int amount_h;
 int inrange;

 int addr1;
 int addr2;
 int addr3;
 int z80_addr;

 char *ptrres;

 char buffer[0x10000];

 ub_func_u *f = (ub_func_u*)(z80mem_f + addr);

 if (leu16_to_host(f->file.id) == 0xAA55)
    {
     f->file.id = host_to_leu16(0);  // clear it for next time (must be set each time)
     switch (leu16_to_host(f->file.cmd))
        {
         case 0x00 : // fopen
            addr1 = leu16_to_host(f->file.addr1);
            addr2 = leu16_to_host(f->file.addr2);
            z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
            z80mem_x2 = get_z80mem_ptr_and_addr(&addr2);
            f->file.fp.p = fopen((char*)(z80mem_x1+addr1), (char*)(z80mem_x2+addr2));
            f->file.res = host_to_le16(f->file.fp.p == NULL);
            break;
         case 0x01 : // fclose
            f->file.res = host_to_le16(fclose(f->file.fp.p));
            break;
         case 0x02 : // fflush
            f->file.res = host_to_le16(fflush(f->file.fp.p));
            break;
         case 0x03 : // feof
            f->file.res = host_to_le16(feof(f->file.fp.p));
            break;
         case 0x04 : // ferror
            f->file.res = host_to_le16(ferror(f->file.fp.p));
            break;
         case 0x05 : // fgetc
            f->file.res = host_to_le16(fgetc(f->file.fp.p));
            break;
         case 0x06 : // fgetpos (RESERVED)
            break;
         case 0x07 : // fgets
            addr3 = leu16_to_host(f->file.addr3);
            z80mem_x3 = get_z80mem_ptr_and_addr(&addr3);
            ptrres = fgets((char*)(z80mem_x3+addr3), leu16_to_host(f->file.num), f->file.fp.p);
            f->file.res = host_to_leu16(ptrres != NULL);
            break;
         case 0x08 : // fputc
            f->file.res = host_to_le16(fputc(leu16_to_host(f->file.val1), f->file.fp.p));
            break;
         case 0x09 : // fputs
            addr3 = leu16_to_host(f->file.addr3);
            z80mem_x3 = get_z80mem_ptr_and_addr(&addr3);
            f->file.res = host_to_le16(fputs((char*)(z80mem_x3+addr3), f->file.fp.p));
            break;
         case 0x0A : // fread
            if ((leu16_to_host(f->file.size) * leu16_to_host(f->file.num)) <= sizeof(buffer))
               {
                amount_t = fread(buffer, leu16_to_host(f->file.size), leu16_to_host(f->file.num), f->file.fp.p);
                f->file.res = host_to_le16(amount_t);  // save the result of the read (bytes read)
                addr3 = leu16_to_host(f->file.addr3);
                z80_addr = addr3;
                if (z80_addr < 0x8000)  // if destination is to a low bank
                   {
                    z80mem_x3 = get_z80mem_ptr_and_addr(&addr3);  // get the current lower bank pointer
                    amount_l = 0x8000 - addr3;  // amount to be moved to this bank
                    if (amount_t < amount_l)    // if the amount read is less than max space then use that instead
                       amount_l = amount_t;
                    amount_h = amount_t - amount_l;

                    inrange = (amount_h <= 0x8000);
                    if (inrange)
                       {
                        memcpy(z80mem_x3+addr3, buffer, amount_l);
                        addr3 = 0x8000;
                        z80mem_x3 = get_z80mem_ptr_and_addr(&addr3);  // get the current upper bank pointer
                        memcpy(z80mem_x3, buffer+amount_l, amount_h);
                       }
                   }
                else
                   {
                    amount_h = amount_t;
                    amount_l = 0;  // needed for error checking!
                    inrange = ((z80_addr + amount_h) <= 0x10000);
                    if (inrange)
                       {
                        z80mem_x3 = get_z80mem_ptr_and_addr(&addr3);  // get the current upper bank pointer
                        memcpy(z80mem_x3+addr3, buffer, amount_h);
                       }
                   }
                if (modio.func)
                   {
                    log_data_5("function_files", "function", "Z80 addr", "read", "amount(LB)", "amount(HB)",
                    leu16_to_host(f->file.cmd), z80_addr, amount_t, amount_l, amount_h);
                   }
                if (! inrange)
                   {
                    log_data_3("function_files (data exceeds 32K limit)", "function", "amount(LB)", "amount(HB)",
                    leu16_to_host(f->file.cmd), amount_l, amount_h);
                    f->file.res = host_to_le16(0);
                   }
               }
            else
               {
                xprintf("function_files: function=0x%04x: internal buffer exceeded\n", leu16_to_host(f->file.cmd));
                f->file.res = host_to_le16(0);
               }
            break;
         case 0x0B : // freopen (RESERVED)
            break;
         case 0x0C : // fseek (RESERVED)
            break;
         case 0x0D : // fsetpos (RESERVED)
            break;
         case 0x0E : // ftell (RESERVED)
            break;
         case 0x0F : // fwrite
            amount_t = leu16_to_host(f->file.size) * leu16_to_host(f->file.num);
            if (amount_t <= sizeof(buffer))
               {
                addr3 = leu16_to_host(f->file.addr3);
                z80_addr = addr3;
                if (z80_addr < 0x8000)  // if source is from a low bank
                   {
                    z80mem_x3 = get_z80mem_ptr_and_addr(&addr3);  // get the current lower bank pointer
                    if ((addr3 + amount_t) > 0x8000)  // amount to be moved from this bank
                       amount_l = 0x8000 - addr3;
                    else
                       amount_l = amount_t;
                    amount_h = amount_t - amount_l;
                    memcpy(buffer, z80mem_x3+addr3, amount_l);

                    inrange = (amount_h <= 0x8000);
                    if (inrange)
                       {
                        addr3 = 0x8000;
                        z80mem_x3 = get_z80mem_ptr_and_addr(&addr3);  // get the current upper bank pointer
                        memcpy(buffer+amount_l, z80mem_x3, amount_h);
                       }
                   }
                else
                   {
                    amount_h = amount_t;
                    amount_l = 0;  // needed for error checking!
                    inrange = ((z80_addr + amount_h) <= 0x10000);
                    if (inrange)
                       {
                        z80mem_x3 = get_z80mem_ptr_and_addr(&addr3);  // get the current upper bank pointer
                        memcpy(buffer, z80mem_x3+addr3, amount_h);
                       }
                   }
                if (modio.func)
                   {
                    log_data_5("function_files", "function", "Z80 addr", "write", "amount(LB)", "amount(HB)",
                    leu16_to_host(f->file.cmd), z80_addr, amount_t, amount_l, amount_h);
                   }
                if (! inrange)
                   {
                    log_data_3("function_files (data exceeds 32K limit)", "function", "amount(LB)", "amount(HB)",
                    leu16_to_host(f->file.cmd), amount_l, amount_h);
                    f->file.res = host_to_le16(0);
                   }
                else
                   f->file.res = host_to_le16(fwrite(buffer, leu16_to_host(f->file.size), leu16_to_host(f->file.num), f->file.fp.p));
               }
            else
               {
                xprintf("function_files: function=0x%04x: internal buffer exceeded\n", leu16_to_host(f->file.cmd));
                f->file.res = host_to_le16(0);
               }
            break;
         case 0x10 : // clearerr
            clearerr(f->file.fp.p);
            break;
         case 0x11 : // rewind
            rewind(f->file.fp.p);
            break;
         case 0x12 : // remove
            addr1 = leu16_to_host(f->file.addr1);
            z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
            f->file.res = host_to_le16(remove((char*)(z80mem_x1+addr1)));
            break;
         case 0x13 : // rename
            addr1 = leu16_to_host(f->file.addr1);
            addr2 = leu16_to_host(f->file.addr2);
            z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
            z80mem_x2 = get_z80mem_ptr_and_addr(&addr2);
            f->file.res = host_to_le16(rename((char*)(z80mem_x1+addr1),
            (char*)(z80mem_x2+addr2)));
            break;
         case 0x14 : // tmpfile
            f->file.fp.p = tmpfile();
            break;
         case 0x15 : // tmpnam (RESERVED)
            break;
         case 0x16 : // ugetc (RESERVED)
            break;
         case 0x17 : // eofval constant
            f->file.res = host_to_le16(EOF);
            break;
         default :
            break;
        }
    }
}

//==============================================================================
// Directory functions.
//
//   pass: int addr             address of Z80 structure (relative to bank)
// return: void
//==============================================================================
static void function_directory (int addr)
{
 sup_file_t fx;

 char const illegal_cpm_char[] = "<>,;:=?*[]\%|()/\\";

 int l;
 int i;
 int addr1;
 int addr2;
 int addr3;
 int addr4;

 int res;
 int dot;

 ub_func_u *f = (ub_func_u*)(z80mem_f + addr);

 if (leu16_to_host(f->file.id) == 0xAA55)
    {
     f->file.id = host_to_leu16(0);  // clear it for next time (must be set each time)
     switch (leu16_to_host(f->file.cmd))
        {
         case 0x00 : // open a directory for reading
            addr1 = leu16_to_host(f->file.addr1); // pass directory path/name
            addr2 = leu16_to_host(f->file.addr2); // return file name or wildcards
            addr3 = leu16_to_host(f->file.addr3); // return modified file path here
            z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
            z80mem_x2 = get_z80mem_ptr_and_addr(&addr2);
            z80mem_x3 = get_z80mem_ptr_and_addr(&addr3);

            fx.dpn = (char *)(z80mem_x1+addr1);
            fx.fnwc = (char *)(z80mem_x2+addr2);
            fx.mfp = (char *)(z80mem_x3+addr3);

            sup_opendir(&fx);

            f->file.fp.d = fx.fp.d;
            f->file.val1 = host_to_le16(fx.val1);
            strcpy((char*)z80mem_x3+addr3, fx.mfp);  // return the file path
            f->file.val2 = host_to_le16(fx.val2);
            f->file.res = host_to_le16(fx.res);
            break;
         case 0x01 : // close directory
            f->file.res = host_to_le16(closedir(f->file.fp.d));
            break;
         case 0x02 : // return the next directory entry (with wildcards)
            addr1 = leu16_to_host(f->file.addr1); // return name match
            addr2 = leu16_to_host(f->file.addr2); // pass file name or wildcards
            addr3 = leu16_to_host(f->file.addr3); // pass file path
            addr4 = leu16_to_host(f->file.addr4); // return full path and file name match
            z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
            z80mem_x2 = get_z80mem_ptr_and_addr(&addr2);
            z80mem_x3 = get_z80mem_ptr_and_addr(&addr3);
            z80mem_x4 = get_z80mem_ptr_and_addr(&addr4);

            fx.dpn = (char *)(z80mem_x1+addr1);
            fx.fnwc = (char *)(z80mem_x2+addr2);
            fx.mfp = (char *)(z80mem_x3+addr3);
            fx.fpfnm = (char *)(z80mem_x3+addr4);
            fx.fp.d = f->file.fp.d;

            sup_readdir(&fx);

            f->file.res = host_to_leu16(fx.res);
            f->file.val1 = host_to_leu16(fx.val1);
            f->file.val2 = host_to_leu16(fx.val2);
            break;
         case 0x03 : // wildcards test
            z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
            z80mem_x2 = get_z80mem_ptr_and_addr(&addr2);
            addr1 = leu16_to_host(f->file.addr1); // pass file name to be tested
            addr2 = leu16_to_host(f->file.addr2); // pass wildcards
            f->file.res = host_to_le16(wildcardfit((char*)(z80mem_x2+addr2), (char*)(z80mem_x1+addr1)));
            break;
         case 0x04 : // check if filename is a legal CP/M 8.3 format
            addr1 = leu16_to_host(f->file.addr1); // pass file name to be tested
            res = 0;
            dot = 0;
            i = 0;
            l = strlen((char*)z80mem_x1+addr1);
            res = (l > 12);
            if (! res)
               {
                while ((i < l) && (! res))
                   {
                    if (z80mem_x1[addr1+i] == '.')
                       {
                        dot++;
                        res = (dot > 1) * 2;
                        if ((! res) && (l > 3))
                           res = (i < (l-4)) * 3;
                       }
                    else
                       res = (strchr(illegal_cpm_char, z80mem_x1[addr1+i]) != NULL) * 4;
                    i++;
                   }
               }
            if ((! res) && (! dot) && (l > 8))
               res = 5;
            f->file.res = host_to_le16(res);
            break;
         default :
            break;
        }
    }
}

//==============================================================================
// Joystick functions
//
//   pass: int addr             address of Z80 structure (relative to bank)
// return: void
//==============================================================================
static void function_joystick (int addr)
{
 ub_func_u *f = (ub_func_u*)(z80mem_f + addr);

 switch (leu16_to_host(f->cmdres.cmd))
    {
     case 0x00 : // enable/disable Microbee joystick
        joystick.mbee = le16_to_host(f->putchar.val);
        break;
     case 0x01 : // enable/disable joystick keys mapping
        joystick.kbd = le16_to_host(f->putchar.val);
        break;
     case 0x02 : // select a joystick keys mapping set
        joystick_kbjoy_select(le16_to_host(f->putchar.val), "");
        break;
     case 0x03 : // upload buttons for the Microbee joystick (RESERVED)
        break;
     case 0x04 : // upload buttons for a joystick key mapping set (RESERVED)
        break;
     default :
        break;
    }
}

//==============================================================================
// Application functions
//
//   pass: int addr             address of Z80 structure (relative to bank)
// return: void
//==============================================================================
static void function_application (int addr)
{
 int addr1;

 ub_func_u *f = (ub_func_u*)(z80mem_f + addr);

 switch (leu16_to_host(f->cmdres.cmd))
    {
     case 0x00 : // return a --file-list option parameter string
        addr1 = leu16_to_host(f->puts.addr);
        z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
        strcpy((char*)z80mem_x1+addr1, func.file_list[func.file_list_pos]);
        f->puts.res = host_to_le16(strlen((char*)z80mem_x1+addr1));
        break;
     case 0x01 : // return the --file-run option parameter string
        addr1 = leu16_to_host(f->puts.addr);
        z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
        strcpy((char*)z80mem_x1+addr1, func.file_run);
        f->puts.res = host_to_le16(strlen((char*)z80mem_x1+addr1));
        break;
     case 0x02 : // return the --file-app option parameter string
        addr1 = leu16_to_host(f->puts.addr);
        z80mem_x1 = get_z80mem_ptr_and_addr(&addr1);
        strcpy((char*)z80mem_x1+addr1, func.file_app);
        f->puts.res = host_to_le16(strlen((char*)z80mem_x1+addr1));
        break;
     case 0x03 : // return the --file-load option Z80 address
        f->puts.res = host_to_le16(func.file_load);
        break;
     case 0x04 : // return the --file-exec option Z80 address
        f->puts.res = host_to_le16(func.file_exec);
        break;
     case 0x05 : // return the --file-exit option status
        f->puts.res = host_to_le16(func.file_exit);
        break;
     case 0x06 : // return the --file-list string count
        f->puts.res = host_to_le16(func.file_list_count);
        break;

     case 0x10 : // set the current --file-list position
        if (le16_to_host(f->putchar.val) < (func.file_list_count))
           func.file_list_pos = le16_to_host(f->putchar.val);
        else
           func.file_list_pos = 0;
        break;
     default :
        break;
    }
}

//==============================================================================
// Basic functions.
//
// (This is to be removed at some stage when all CP/M support programs have
// been modified to use the function_nostruct() function instead!  Don't add
// any more code to this function!)
//
// Handles functions numbers 0xe0-0xef. These functions are acted on
// directly and have no parameters and return no results.
//
//   pass: int cmd
// return: void
//==============================================================================
static void function_basic (int cmd)
{
 switch (cmd)
    {
     case 0xe0 : // turn on video, sound and revert to default clock rate
        // turn the video on
        crtc.video = 1;
        // turn the sound on
        audio.mute = 0;
        audio_set_master_volume(audio.vol_percent);
        set_clock_speed(emu.cpuclock_def, emu.z80_divider, 0);
        break;
     default :
        break;
    }
}

//==============================================================================
// No structure functions.
//
// Handles functions numbers 0xf0-0xff. These functions do not use a data
// structure to pass/return values.
//
// New functions here should only be added when a quick and easy method is
// required to perform a task.  The advantage is that no data structure is
// needed so only one OUT call is required but the number of possible
// functions are limited and Z80 registers must be used to pass and return
// values if required.
//
//   pass: int cmd
// return: void
//==============================================================================
static void function_nostruct (int cmd)
{
 switch (cmd)
    {
     case 0xf0 : // read byte from TAP file
        tapfile_read();
        break;
     case 0xf1 : // write byte to TAP file
        tapfile_write();
        break;
     default :
        break;
    }
}

//==============================================================================
// uBee512 write - Special uBee512 Port function
//
// All commands pass a Z80 address to a structure in memory except for 0xF0
// functions which only requires the command.  The structure contents is
// dependent on the command but all share the same overhead.  Data passed
// and returned in the structure is always in little endian format.
//
// IMPORTANT: This function does not currently address banking issues so it
// is important to realise that data may get lost if ANY bank swapping occurs
// after this function call.  To avoid problems do not switch banks or call
// any OS calls before the returned data is used.  Essentially the main area
// of concern is the 32K DRAM bank at 0x8000 that may also be at 0x0000.
// Normally writes to these banks will be copied from one to the other, but
// this does not occur here as we are acting on Z80 memory outside of the
// MZ80 context.  Also do not have the structure in video memory for similar
// reasons,  unlikely you would want to do that any way.
//
// It is important to verify that the application is running under the emulator
// and not a real Microbee before calling this function.  Use the uBee512 read
// port function to check first.
//
// The calling sequence is:
// port[n] = command number (0..255)
// port[n] = LSB of structure address (0 if not using a structure)
// port[n] = MSB of structure address (0 if not using a structure)
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void function_ubee_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.func)
    log_port_1("function_ubee_w", "data", port, data);

 if (ubee_req)          // if we need to get the structure address
    {
     ubee_req--;
     if (ubee_req == 1)
        ubee_addr = data;       // LSB of the address
     else
        if (ubee_req == 0)
           {
            ubee_addr = ubee_addr | (data << 8);
            z80mem_f = get_z80mem_ptr_and_addr(&ubee_addr);

            switch (ubee_command & 0xf0)
               {
                case 0x00 : // status functions
                   function_status(ubee_addr);
                   break;
                case 0x10 : // control functions
                   function_control(ubee_addr);
                   break;
                case 0x20 : // diagnostics functions
                   function_diagnostics(ubee_addr);
                   break;
                case 0x30 : // keyboard functions
                   function_keyboard(ubee_addr);
                   break;
                case 0x40 : // stdio functions
                   function_stdio(ubee_addr);
                   break;
                case 0x50 : // file functions
                   function_files(ubee_addr);
                   break;
                case 0x60 : // directory functions
                   function_directory(ubee_addr);
                   break;
                case 0x70 : // joystick functions
                   function_joystick(ubee_addr);
                   break;
                case 0x80 : // application functions
                   function_application(ubee_addr);
                   break;
                case 0xe0 : // simple functions
                   function_basic(ubee_command);
                   break;
                default :
                   break;
               }
           }
    }
 else
    {
     ubee_command = data;
     if (ubee_command < 0xf0)
        ubee_req = 2;
     else
        function_nostruct(ubee_command);
    }
}
