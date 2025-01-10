//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                       Copyright (C) 2007-2016 uBee                         *
//*                                                                            *
//*                           TAP tape file module                             *
//*                                                                            *
//*                 Initial concept from Martin D.J. Rosenau                   *
//*                   final implementation by uBee                      *
//******************************************************************************
//
// TAP file support for Basic and 56K/64K model Boot ROMs. The process works
// by patching known memory locations at run time allowing fast read/write
// of tape using normal load/save (cload/csave on disk Basic) functionality. 
// This will only work for code that makes use of Basic or E000 functions
// for reading and writing tape bytes.  Machine code programs that uses it's
// own code to read/write will not work.
//
// The patching of code for read and write is based on the whether there is
// a corresponding input and output TAP file open and there is Basic in
// memory and for Boot ROM patching if it's the correct model.  This allows
// tape WAV files to be read (no TAP input open) and saved to a TAP file and
// vice-versa.
//
// Accessing the TAP functions may also be called from within a program
// designed for emulation by using similar code to that found in
// patch_code_input[] and patch_code_output[], the patched code does not
// have to be installed.
//
// If tape/disk Basic or the 64K model's Boot ROM is being used then the
// --tapfilei and --tapfileo options must be used after Basic has been
// loaded into memory or in the case of the 64K Boot ROM when the ROM has
// relocated itself to 0xe000 in RAM.
//
// Reference: Microbee Disk System Manual (E-4)
//
// 8012 and E012  RD_BYTE (DGOS COMPATIBLE)
// Assuming that interrupts are already disabled by the calling program,
// RD_BYTE reads in a character from the tape interface into A.  All other
// registers are preserved.  The tape speed is set by the value of
// (LO_CYCLES) at DF5A.  If (LO_CYCLES)=4 then 300 baud is set, if
// (LOCYCLES)=1 then 1200 baud is set.
//
// 8018 and E018  WR_BYTE (DGOS COMPATIBLE)
// Assuming interrupts are already disabled, write the byte in A out to the
// cassette interface.  All other registers are preserved.  Speed control as
// per RD_BYTE.
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
// v5.7.0 - 6 December 2013, uBee
// - Fixed Tapfile DGOS name display to use space characters if white space
//   and to mask off high bit.
//
// v5.4.0 - 26 July 2011, uBee
// - Added SPEED value to tapfile_list().
// - Made changes in tapfile_list() to not write host endian values back
//   into the 'dgos' structure and use other variables instead.
//
// v5.3.0 - 10 April 2011, uBee
// - Initial concept from Martin D.J. Rosenau, final implementation by
//   uBee.
//==============================================================================

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ubee512.h"
#include "support.h"
#include "tapfile.h"
#include "gui.h"
#include "console.h"
#include "z80api.h"

//==============================================================================
// structures and variables
//==============================================================================
uint8_t patch_code_input[] =
{
 0x3e, 0xf0,     /* ld   a,0xf0    ;tapfile input function         */
 0xd3, 0xff,     /* out  (0xff),a  ;emulator sets reg A on return  */
 0xb7,           /* or   a                                         */
 0x28, 0xf9,     /* jr   nz,PC-7   ;JR to top of patch             */
 0x3e, 0xf0,     /* ld   a,0xf0                                    */
 0xd3, 0xff,     /* out  (0xff),a  ;emulator sets reg A on return  */
 0xb7,           /* or   a                                         */
 0xc9            /* ret                                            */
};

uint8_t patch_code_output[] =
{
 0xc5,           /* push bc                                        */
 0x4f,           /* ld   c,a       ;pass the value in C            */
 0x3e, 0xf1,     /* ld   a,0xf1    ;tapfile output function        */
 0xd3, 0xff,     /* out  (0xff),a                                  */
 0x79,           /* ld   a,c       ;we had better put it back in A */
 0xc1,           /* pop  bc                                        */
 0xc9            /* ret                                            */
};

#define PATCH_INPUT_SIZE sizeof(patch_code_input)
#define PATCH_OUTPUT_SIZE sizeof(patch_code_output)

#define TAP_FILE_ID "TAP_DGOS_MBEE"

tapfile_t tapfile;

static uint8_t orig_code_8012[PATCH_INPUT_SIZE];
static uint8_t orig_code_e012[PATCH_INPUT_SIZE];
static uint8_t orig_code_8018[PATCH_OUTPUT_SIZE];
static uint8_t orig_code_e018[PATCH_OUTPUT_SIZE];

static int addr_8012;
static int addr_8018;
static int addr_e012;
static int addr_e018;

static int lastbyte = -1;

extern uint8_t basic[];
extern uint8_t rom1[];

extern char userhome_tapepath[];

extern model_t modelx;
extern modio_t modio;
extern emu_t emu;

//==============================================================================
// Set the A register without affecting the contents of F.
//
//   pass: int value
// return: void
//==============================================================================
static void set_register_a (int value)
{
 z80regs_t z80x;

 z80api_get_regs(&z80x);
 z80x.af = (z80x.af & 0x00ff) | (value << 8);
 z80api_set_regs(&z80x);
}

//==============================================================================
// Read data bytes from Z80 memory.
//
//   pass: uint8_t *s
//         int addr
//         int amount
// return: void
//==============================================================================
static void read_z80_memory (uint8_t *s, int addr, int amount)
{
 if (modio.tapfile)
    xprintf("tapfile: read_z80_memory\n");

 while (amount--)
    {
     *(s++) = z80api_read_mem(addr++);
     if (modio.tapfile)
        xprintf("tapfile: addr=0x%04x, data=0x%02x\n", addr-1, *(s-1));
    }
}

//==============================================================================
// Write data bytes to Z80 memory.
//
//   pass: uint8_t *s
//         int addr
//         int amount
// return: void
//==============================================================================
static void write_z80_memory (uint8_t *s, int addr, int amount)
{
 if (modio.tapfile)
    xprintf("tapfile: write_z80_memory\n");

 while (amount--)
    {
     if (modio.tapfile)
        xprintf("tapfile: addr=0x%04x, data=0x%02x\n", addr, *s);
     z80api_write_mem(addr++, *(s++));
    }   
}

//==============================================================================
// Get a Z80 16 bit address from a Z80 JP location in memory.
//
// The checks on the Basic memory locations are quite generous as Disk
// Basics are much larger so it's only a rough check.
//
//   pass: int addr                     address of the JP location
//         int method                   0=direct access, 1=use z80api functions
// return: int                          Z80 address or 0 if error
//==============================================================================
static int get_z80_jp_addr (int addr, int method)
{
 int res;
 
 if (method == 1)
    {
     if (z80api_read_mem(addr) != 0xc3)
        return 0;
     res = (z80api_read_mem(addr + 2) << 8) | z80api_read_mem(addr + 1);
     return res;   
    }

 if (addr >= 0xe000)
    {
     if (rom1[addr - 0xe000] != 0xc3)
        return 0;
     res = (rom1[addr - 0xe000 + 2] << 8) | rom1[addr - 0xe000 + 1];
     if ((res < 0xe000) || (res > 0xefff))
        return 0;
     return res;   
    }

 if (basic[addr - 0x8000] != 0xc3)
    return 0;
 res = (basic[addr - 0x8000 + 2] << 8) | basic[addr - 0x8000 + 1];
 if ((res < 0x8000) || (res > 0xefff))
    return 0;
 return res;
}

//==============================================================================
// Get data out of Z80 memory or ROM area.
//
//   pass: uint8_t *dest                pointer to destination
//         int addr                     address of Z80 source data
//         int amount                   amount to get
//         int method                   0=direct access, 1=use z80api functions
// return: void
//==============================================================================
static void get_z80_data (uint8_t *dest, int addr, int amount, int method)
{
 if (method == 1)
    {
     read_z80_memory(dest, addr, amount);
     return;
    }

 if (addr >= 0xe000)
    {
     memcpy(dest, &rom1[addr - 0xe000], amount);
     return;
    }
    
 memcpy(dest, &basic[addr - 0x8000], amount);    
}

//==============================================================================
// Put data into Z80 memory or ROM area.
//
//   pass: int addr                     destination address of Z80 data
//         uint8_t *src                 pointer to source
//         int amount                   amount to put
//         int method                   0=direct access, 1=use z80api functions
// return: void
//==============================================================================
static void put_z80_data (int addr, uint8_t *src, int amount, int method)
{
 if (method == 1)
    {
     write_z80_memory(src, addr, amount);
     return;
    }

 if (addr >= 0xe000)
    {
     memcpy(&rom1[addr - 0xe000], src, amount);
     return;
    }
    
 memcpy(&basic[addr - 0x8000], src, amount);    
}

//==============================================================================
// Check an already opened Tapfile is the correct format.
//
// The TAP file indentification format is as follows:
//
// TAP_000_FMT_MAC (NULL)
//
// TAP : Identifies the file as a TAP file.
// 000 : Represents a revision number if it's needed in the future.
// FMT : Tape format DGOS or KCS.
// MAC : ID string indicating what machine is the target. (MBEE)
// (NULL): A terminating NULL character for the string.
//
// A check is made for 'TAP_' is present at the beginning of the file and
// the '_DGOS_' value which may be anywhere in the string.
//
// For compatibility with earlier development files 'MBEE' may be used at
// the beginning of the string.  In this case naturally occuring NULLs sent
// by the tape routines will serve as a NULL termination.
//
// When files are saved the 'TAP_FMT_MAC (NULL)' format is used.
//
//   pass: FILE *fp
// return: int                          0 if success, -1 if error
//==============================================================================
static int check_tapfile_format (FILE *fp)
{
 char header[100];
 char c;
 int res;
 int i;

 // check to see if the file is a DGOS TAP file
 i = 0;
 c = 1;
 while ((res = (fread(&c, 1, 1, fp))) && c && (i < sizeof(header)-1))
    header[i++] = c;
 header[i] = 0;
 
 if (res)
    res = (((strstr(header, "TAP_") == header) && strstr(header, "_DGOS_")) ||
            (strstr(header, "MBEE") == header));
    
 if (! res)
    {
     fclose(fp);
     return -1;
    }

 return 0;
}

//==============================================================================
// Tapfile initialise.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int tapfile_init (void)
{
 if (tapfile.tapei[0] && (tapfile.tape_i_file == NULL))
    {
     if (tapfile_i_open(tapfile.tapei, 1) == -1)
        return -1;
    }    

 if (tapfile.tapeo[0] && (tapfile.tape_o_file == NULL))
    {
     if (tapfile_o_open(tapfile.tapeo, 1) == -1)
        return -1;
    }

 return 0;
}

//==============================================================================
// Tapfile de-initialise.
//
// For tapfile out the file is closed.
// For tapfile in the file is closed.
//
//   pass: void
// return: int                          0
//==============================================================================
int tapfile_deinit (void)
{
 tapfile_i_close();
 tapfile_o_close();

 return 0;
}

//==============================================================================
// Tapfile reset.
//
//   pass: void
// return: int                          0
//==============================================================================
int tapfile_reset (void)
{
 if (tapfile.tapei[0] && (tapfile.tape_i_file == NULL))
    {
     if (tapfile_i_open(tapfile.tapei, 1) == -1)
        return -1;
    }    

 if (tapfile.tapeo[0] && (tapfile.tape_o_file == NULL))
    {
     if (tapfile_o_open(tapfile.tapeo, 1) == -1)
        return -1;
    }

 return 0;
}

//==============================================================================
// Tapfile name check.
//
// Ensure that the tapfile.tapei and tapfile.tapeo file names will not be
// the same.
//
//   pass: char *s1                     file name of tape path #1
//         char *s2                     file name of tape path #2
// return: int                          0 if different, -1 if same
//==============================================================================
int tapfile_check (char *s1, char *s2)
{
 if (strcmp(s1, s2) == 0)
    {
     xprintf("ubee512: tapfile in and out can not be the same file\n");
     return -1;
    }

 return 0;
}

//==============================================================================
// Tapfile list.
//
// List all the DGOS tape file names contained in a TAP file.
//
//   pass: char *tapfile                TAP file name
// return: int                          0 if success, -1 if error
//==============================================================================
int tapfile_list (char *tapfile)
{
#define SOH 1

 char filepath[SSIZE1];
 FILE *fp;
 int res;
 uint8_t value;
 int count = 0;
 int i;
 
 uint16_t length;
 uint16_t load;
 uint16_t start;

 dgos_t dgos;
 
 fp = open_file(tapfile, userhome_tapepath, filepath, "rb");

 if (fp == NULL)
    {
     xprintf("tapfile_list: Unable to open tapfile: %s\n", tapfile);
     return -1;
    }

 // check to see if the file is a DGOS TAP file
 if (check_tapfile_format(fp) == -1)
    {
     xprintf("tapfile_list: %s is not a DGOS TAP file\n", filepath);
     return -1;
    }

 xprintf("NAME   TYPE  LENGTH  LOAD  START  SPEED  AUTO  SPARE  CRC\n");

 // loop for each file in the TAP file
 for (;;)
    {
     // find the header
     for (;;)
        {
         // find a starting NULL
         do
            res = fread(&value, 1, 1, fp);
         while ((res == 1) && (value != 0));
         if (! res)
            break;

         // find at least 16 NULLs
         i = 1;   
         while ((i++ < 16) && (value == 0) && (res == 1))
            res = fread(&value, 1, 1, fp);
         if (! res)
            break;

         // skip past all the leading NULLs
         while ((value == 0) && (res == 1))
            res = fread(&value, 1, 1, fp);
         break;
        }   

     if (! res)
        break;
        
     if (value != SOH)
        {
         xprintf("tapfile_list: Expected SOH character but"
                 " 0x%02x was found instead\n", value);
         break;
        }
     
     // read in the DGOS header block   
     res = fread(&dgos, sizeof(dgos), 1, fp);
     if (res != 1)
        break;

     // display the filename, blanks are output as spaces, some names use
     // high bit set (-char), need to cast on test and mask
     for (i = 0; i < 6; i++)
        {
         if ((uint8_t)dgos.name[i] > ' ')
            xprintf("%c", dgos.name[i] & 0x7f);
         else
            xprintf(" ");
        }

     // display the file type, addresses, etc.
     xprintf(" %c", dgos.type);
     length = leu16_to_host(dgos.length);
     xprintf("     %04x", length);
     load = leu16_to_host(dgos.load);
     xprintf("    %04x", load);
     start = leu16_to_host(dgos.start);
     xprintf("  %04x", start);
     xprintf("   %02x", dgos.speed);
     xprintf("     %02x", dgos.autos);
     xprintf("    %02x", dgos.spare);
     xprintf("     %02x\n", dgos.crc);
     
     count++;

     // seek to next saved file position, length does not include the CRC
     // values so 1 must be added for each block
     fseek(fp, length + ((length + 256) / 256), SEEK_CUR);

     if (modio.tapfile)
        xprintf("tapfile_list: Next file offset=0x%x\n",
                (unsigned int)ftell(fp));
    }

 fclose(fp); 
 xprintf("\nNumber of files in TAP file: %d\n", count);
 return 0;
}

//==============================================================================
// Tapfile input open.
//
// Open a tapfile for input and install the patch code.
//
// If a tapfile is already open it will be closed before opening the new file.
//
//   pass: char *s                      tapfile name path
//         int action                   0 saves file name only, 1 opens file
// return: int                          0 if success, -1 if error
//==============================================================================
int tapfile_i_open (char *s, int action)
{
 char filepath[SSIZE1];
 char vers[5];
 int method = -1;
 int res;

 strcpy(tapfile.tapei, s);
 if (action == 0)
    return 0;

 tapfile_i_close();

 tapfile.tape_i_file = open_file(tapfile.tapei,
                                 userhome_tapepath, filepath, "rb");

 if (tapfile.tape_i_file == NULL)
    {
     xprintf("tapfile_i_open: Unable to open tapfile input file: %s\n",
             tapfile.tapei);
     tapfile.tapei[0] = 0;
     tapfile.in_status = 0;
     gui_status_update();
     return -1;
    }

 // check to see if the file is a DGOS TAP file
 if (check_tapfile_format(tapfile.tape_i_file) == -1)
    {
     xprintf("tapfile_i_open: %s is not a DGOS TAP file\n", filepath);
     return -1;
    }

 // TAP files are always ready so first time won't require EMUKEY+T
 if (! tapfile.in_status)
    tapfile.in_status = 1;

 gui_status_update();

 res = get_mwb_version(0, vers);

 // install a patch for ROM and non-banked models if Basic is found. 
 if ((res != -1) && (res != 529))
    {
     if (emu.verbose)
        xprintf("tapfile_i_open: Microworld Basic version: %s\n", vers);

     if (modelx.rom)
        // for ROMs we can't use write_z80_memory()
        method = 0;
     else   
        // for tape/disk Basic in RAM we use write_z80_memory()
        // and read_z80_memory() functions
        method = 1;

     if ((addr_8012 = get_z80_jp_addr(0x8012, method)))
        {
         // install the Basic code patch
         get_z80_data(orig_code_8012, addr_8012,
                      PATCH_INPUT_SIZE, method);
         put_z80_data(addr_8012, patch_code_input,
                      PATCH_INPUT_SIZE, method);

         if (modio.tapfile)
            xprintf("tapfile: tapfile_i_open (0x8012), patch"
                    " install @ 0x%04x\n", addr_8012);
        }
    }
 else
    {
     addr_8012 = 0;
     if (emu.verbose)
        {
         if (res == -1)
            xprintf("tapfile_i_open: Basic not found.\n");
         else   
            xprintf("tapfile_i_open: Basic version '%s' not supported.\n",
                    vers);
        }
    }    
    
 // install a patch for the boot ROM
 if (modelx.rom)
    return 0;

 switch (emu.model)
    {
     case MOD_56K :
        // For the 56K model we can access the ROM directly in it's own
        // holding area for read/write access.  There is no copying of the
        // ROM into RAM so can be acted on from the command line.
        method = 0;
        break;
     case MOD_64K : 
        // Must skip if boot ROM code has not copied itself to E000 by the
        // boot process yet, we just check if enough CPU time has occured.
        if (emu.z80_cycles < 200000)
           {
            method = -1;
            if (emu.verbose)
               xprintf("tapfile_i_open: can't patch 0xE000 code until ROM"
                       " has had time to be copied.\n");
            break;
           }
        // For the 64K model we must use the special z80api_*() functions to
        // access memory as the ROMs are copied to RAM at E000 by the first
        // 0x100 bytes in the ROM but we know there is only 64K of DRAM and
        // the bank will normally be in the map.
        method = 1;
        break;
     default :
        // 128K and higher banked memory models gets tricky and will
        // need to be looked at later.
        if (emu.verbose)
           xprintf("tapfile_i_open: 128K DRAM and higher"
                   " models not supported for boot ROM.\n");
        method = -1;
        break;
    }

 if (method == -1)
    return 0;
    
 if ((addr_e012 = get_z80_jp_addr(0xe012, method)) == 0)
    return 0;

 get_z80_data(orig_code_e012, addr_e012, PATCH_INPUT_SIZE, method);
 put_z80_data(addr_e012, patch_code_input, PATCH_INPUT_SIZE, method);
 if (modio.tapfile)
    xprintf("tapfile: tapfile_i_open (0xe012), patch install @ 0x%04x\n",
            addr_e012);

 return 0;
}

//==============================================================================
// Tapfile input close.
//
// Close the tapfile input file and deinstall the patch code..
//
//   pass: void
// return: void
//==============================================================================
void tapfile_i_close (void)
{
 int method = -1;
 
 if (tapfile.tape_i_file == NULL)
    return;
    
 fclose(tapfile.tape_i_file);
 tapfile.tape_i_file = NULL;

 // uninstall the Basic code patch
 if (addr_8012)
    {
     if (modelx.rom)
        method = 0;
     else
        method = 1;

     put_z80_data(addr_8012, orig_code_8012, PATCH_INPUT_SIZE, method);

     if (modio.tapfile)
        xprintf("tapfile: tapfile_i_close (0x8012), patch removed @ 0x%04x\n",
                addr_8012);
     
     addr_8012 = 0;
    }

 // uninstall the boot ROM patch
 if (addr_e012)
    {
     switch (emu.model)
        {
         case MOD_56K :
            method = 0;
            break;
         case MOD_64K : 
            method = 1;
            break;
         default :
            method = -1;
            addr_e012 = 0;
            break;
        }
    }

 lastbyte = -1;
 tapfile.in_status = 0;
 gui_status_update();

 if (method == -1 || ! addr_e012)
    return;
    
 put_z80_data(addr_e012, orig_code_e012, PATCH_INPUT_SIZE, method);
        
 if (modio.tapfile)
    xprintf("tapfile: tapfile_i_close (0xe012), patch removed @ 0x%04x\n",
            addr_e012);

 addr_e012 = 0;
}

//==============================================================================
// Tapfile output open.
//
// Open a tapfile file for output and and install the patch code..
//
//   pass: char *s                      tapfile file name path
//         int action                   0 saves file name only, 1 creates file
// return: int                          0 if success, -1 if error
//==============================================================================
int tapfile_o_open (char *s, int action)
{
 char filepath[SSIZE1];
 char vers[5];
 int method = -1;
 int res;

 tapfile_o_close();
 strcpy(tapfile.tapeo, s);

 if (action == 0)
    return 0;

 tapfile.tape_o_file = open_file(tapfile.tapeo, userhome_tapepath,
                                 filepath, "wb");

 if (tapfile.tape_o_file == NULL)
    {
     xprintf("tapfile_o_open: Unable to create tapfile output file %s\n",
             filepath);
     tapfile.tapeo[0] = 0;
     return -1;
    }
 else
    // write the Microbee TAP file header ID
    fprintf(tapfile.tape_o_file, TAP_FILE_ID);

 gui_status_update();

 res = get_mwb_version(0, vers);

 // install a patch for ROM and non-banked models if Basic is found.
 if ((res != -1) && (res != 529))
    {
     if (emu.verbose)
        xprintf("tapfile_o_open: Microworld Basic version: %s\n", vers);

     if (modelx.rom)
        // for ROMs we can't use write_z80_memory()
        method = 0;
     else   
        // for tape/disk Basic in RAM we use write_z80_memory()
        // and read_z80_memory() functions
        method = 1;

     if ((addr_8018 = get_z80_jp_addr(0x8018, method)))
        {
         // install the Basic code patch
         get_z80_data(orig_code_8018, addr_8018,
                      PATCH_OUTPUT_SIZE, method);
         put_z80_data(addr_8018, patch_code_output,
                      PATCH_OUTPUT_SIZE, method);

         if (modio.tapfile)
            xprintf("tapfile: tapfile_o_open (0x8018), patch"
                    " install @ 0x%04x\n", addr_8018);
        }
    }
 else
    {
     addr_8018 = 0;
     if (emu.verbose)
        {
         if (res == -1)
            xprintf("tapfile_o_open: Basic not found.\n");
         else   
            xprintf("tapfile_o_open: Basic version '%s' not supported.\n",
                    vers);
        }

    }
    
 // install a patch for the boot ROM
 if (modelx.rom)
    return 0;

 switch (emu.model)
    {
     case MOD_56K :
        // For the 56K model we can access the ROM directly in it's own
        // holding area for read/write access.  There is no copying of the
        // ROM into RAM so can be acted on from the command line.
        method = 0;
        break;
     case MOD_64K : 
        // Must skip if boot ROM code has not copied itself to E000 by the
        // boot process yet, we just check if enough CPU time has occured.
        if (emu.z80_cycles < 200000)
           {
            addr_e018 = 0;
            method = -1;
            if (emu.verbose)
               xprintf("tapfile_o_open: can't patch 0xE000 code until"
                       " ROM has had time to be copied.\n");
            break;
           }
        // For the 64K model we must use the special z80api_*() functions to
        // access memory as the ROMs are copied to RAM at E000 by the first
        // 0x100 bytes in the ROM but we know there is only 64K of DRAM and
        // the bank will normally be in the map.
        method = 1;
        break;
     default :
        // 128K and higher banked memory models gets tricky and will
        // need to looked at later.
        xprintf("tapfile_o_open: 128K DRAM and higher models"
                " not supported for boot ROM.\n");
        addr_e018 = 0;
        method = -1;
        break;
    }

 if (method == -1)
    return 0;
 
 if ((addr_e018 = get_z80_jp_addr(0xe018, method)) == 0)
    return 0;
    
 get_z80_data(orig_code_e018, addr_e018, PATCH_OUTPUT_SIZE, method);
 put_z80_data(addr_e018, patch_code_output, PATCH_OUTPUT_SIZE, method);

 if (modio.tapfile && addr_e018)
    xprintf("tapfile: tapfile_o_open (0xe018), patch install @ 0x%04x\n",
            addr_e018);

 return 0;
}

//==============================================================================
// Tapfile output close.
//
// Close the tapfile output file and deinstall the patch code.
//
//   pass: void
// return: void
//==============================================================================
void tapfile_o_close (void)
{
 int method = -1;
 
 if (tapfile.tape_o_file == NULL)
    return;

 fclose(tapfile.tape_o_file);
 tapfile.tape_o_file = NULL;

 // uninstall the Basic code patch
 if (addr_8018)
    {
     if (modelx.rom)
        method = 0;
     else
        method = 1;

     get_z80_data(orig_code_8018, addr_8018, PATCH_OUTPUT_SIZE, method);
     
     if (modio.tapfile)
        xprintf("tapfile: tapfile_o_close (0x8018), patch removed @ 0x%04x\n",
                addr_8018);

     addr_8018 = 0;
    }

 // uninstall the boot ROM patch
 if (addr_e018)
    {
     switch (emu.model)
        {
         case MOD_56K :
            method = 0;
            break;
         case MOD_64K : 
            method = 1;
            break;
         default :
            method = -1;
            addr_e018 = 0;
            break;
        }
    }

 if (method == -1 || ! addr_e018)
    return;

 get_z80_data(orig_code_e018, addr_e018, PATCH_OUTPUT_SIZE, method);

 if (modio.tapfile)
    xprintf("tapfile: tapfile_o_close (0xe018), patch removed @ 0x%04x\n",
       addr_e018);

 addr_e018 = 0;
}

//==============================================================================
// Read tape file byte function.
//
// This function is called from function.c and the result passed back in the
// Z80 A register.
//
// Z80 read handler for the port used for TAPFILE simulation Returns 0x01,
// byte, 0x01, byte, ...  if there is data; Returns 0x00, 0x00, ...  if
// there is no data.
//
//   pass: void
// return: void
//==============================================================================
void tapfile_read (void)
{
 // if no file is open we do not have data and return, can only get here in
 // that case by some other Z80 code executing an OUT 0xff instruction!
 if (tapfile.tape_i_file == NULL)
    return;

 // pressing the Tape reset key will cause tapfile.in_status to be set to 2
 if (tapfile.in_status == 2)
    {
     tapfile_i_open(tapfile.tapei, 1);
     tapfile.in_status = 1;
     gui_status_update();
    }

 // last time we returned 0x01 and we have data now
 if (lastbyte >= 0)
    {
     set_register_a(lastbyte);
     lastbyte = -1;
     return;
    }

 lastbyte = fgetc(tapfile.tape_i_file);
 if (lastbyte < 0)
    set_register_a(0);
 else   
    set_register_a(1);

 return;
}

//==============================================================================
// Write tapfile byte function.
//
// This function is called from function.c, the value in the Z80 C register
// is written to the file.
//
//   pass: void
// return: void
//==============================================================================
void tapfile_write (void)
{
 z80regs_t z80x;

 if (tapfile.tape_o_file)
    {
     z80api_get_regs(&z80x);
     fputc((z80x.bc & 0x00ff), tapfile.tape_o_file);
    }
}

//==============================================================================
// Tapfile commands.
//
//   pass: int cmd                      tapfile command
// return: void
//==============================================================================
void tapfile_command (int cmd)
{
 switch (cmd)
    {
     case EMU_CMD_TAPEREW :
        if (tapfile.tape_i_file != NULL)
           {
            tapfile.in_status = 2;
            xprintf("Tapfile rewind.\n");
           }
        break;
    }
}
