//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                            Memory mapper module                            *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Handles configuration and initialising of all Z80 memory.
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
// v6.0.0 - 5 February 2017, uBee
// - Comment out the printf("file=...") line in sram_load().
// - Changed sram_save() to ignore an open new file error, now it only warns
//   if using verbose mode.
// v6.0.0 - 1 January 2017, K Duckmanton
// - Microbee memory is now an array of uint8_t rather than char.
//
// v5.8.0 - 14 December 2016, uBee
// - Added support for SRAM battery backup emulation for SRAM based models
//   using new sram_load() and sram_save() functions.
//
// v5.7.0 - 21 July 2015, uBee
// - Changes to sram_map_configure() to allow 0 - 32k SRAM emulation.
// - Changes to set_read_handler() and set_write_handler() to use new define
//   values of MEMMAP_MASK and MEMMAP_SHIFT allowing a finer control of 1k
//   blocks as compared to 4k used before.
// - Changed function names memmap_mode1() to memmap_mode1_w() and
//   memmap_mode2() to memmap_mode2_w().
// - Fixed set_roms_dram_handler() to ignore ROM2 and ROM3 when Premium Plus
//   model.
// - Fixed 64k DRAM model DRAM bank selection to act the same as a real 64k
//   DRAM Microbee.  If port 50h bit 0 is set then 0 is returned on reading
//   and nothing written if 0000-7FFF.  (Thanks to wizged for research on a
//   real 64k and providing the information required)
//
// v5.4.0 - 2 Jan 2012, uBee
// - Added Microbee Technology's p1024k/1024k (pplus) model emulation.
//
// v4.6.0 - 4 May 2010, uBee
// - Changes made to set_roms_sram_handler() to include the Teleterm model
//   when setting write handlers for BASIC ROM location.
// - Renamed log_port() calls to log_port_1().
//
// v4.3.0 - 31 July 2009, uBee
// - Added '2mhzdd' and 'dd' Dreamdisk models (workerbee).
//
// v4.1.0 - 4 July 2009, uBee
// - Added models SCF and PCF (Compact Flash CB Standard/Premium)
// - Loading of all ROMs moved to new roms.c module.
// - Many changes made to the memory management including 2MB emulation.
// - Made improvements to the logging process.
//
// v4.0.0 - 8 June 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Memory management has changed. Memory copies between banks has been
//   removed.  Access to Z80 memory is now handled directly.
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
//
// v3.1.0 - 22 April 2009, uBee
// - Added conditional message reporting based on --verbose option.
// - Removed all occurrences of console_output() function calls.
// - Added MD5 code to check for known ROMs and apply Y2K patches if requested.
// - Changed function name find_file_entry() to find_file_alias().
// - Added check for known bad 256TC v1.20 ROM (very damaged, no patching).
// - Changed all printf() calls to use a local xprintf() function.
//
// v2.8.0 - 30 July 2008, uBee
// - Redeclared "char const *block_ptrs[] =" to "char *const block_ptrs[] ="
//   to address some compilation warnings.
//
// v2.7.0 - 12 June 2008, uBee
// - rom1.bin is now BOOT_128K.ROM.
// - PREM-A.ROM and PREM-B.ROM changed to PPC85_A.ROM, and PPC85_B.ROM.
// - TTERM-A.ROM and TTERM-B.ROM changed to TTERM_A.ROM, and TTERM_B.ROM.
// - Added PC85_BASIC_A.ROM and PC85_BASIC_B.ROM entries.
// - Added find_file_entry() function call to the memmap_loadrom() function
//   to find what file to use for file name aliases defined in a
//   configuration file.  Added a variable to return the full file path.
// - New open_file() function in function.c module is now used by the
//   memmap_loadrom() function to determine the path to the ROMs.
// - Added structures emu_t, memmap_t  and moved variables into it.
// - rom1.bin was being loaded in to complete a 16K load if the 1st ROM in
//   the list was less than 16K in size.
//
// v2.6.0 - 3 May 2008, uBee
// - Fixed problem preventing rom2 and rom3 loading when using options --rom2
//   and --rom3.
// - Changes so that ROM models 0xA000-0xBFFF can use SRAM or ROM using
//   new modelc.basram structure member.
// - Changes so that ROM models 0xC000-0xDFFF can use SRAM or ROM.
// - Changes so that ROM models 0xE000-0xEFFF can use SRAM or ROM.
// - Test model type before loading ROMs.
//
// v2.5.0 - 20 March 2008, uBee
// - Added Teleterm model emulation.
// - The roms 1-3 and basic A-D ROMs can now be specified as user options to
//   override the built in model defaults.
// - Implement the modelc structure.
//
// v2.3.0 - 4 January 2008, uBee
// - Recoded this module in many areas to fix known banking issues.
// - Added RAM/DRAM initilization patterns to emulate real hardware, modio
//   option allows sequential bank numbers to be used instead.
// - Fixed 256TC mapping,  bit 5 of port 50h is used to select the extra 128K
//   of DRAM and there is no ROM2 and ROM3 used.
// - All DRAM models (except 256TC) now have ROM1 size changed so that 16K
//   ROMS can be loaded instead of just 8K.  Relaxed the rules in the loading
//   section of the code on what size is allowed.
// - Added modio_t structure.
//
// v2.1.0 - 27 October 2007, uBee
// - boot_data had incorrect ordering of p256k, 256k, and p128k, 128k models,
//   moved the 256k models above the 128k models.
// - Added optional DRAM model ROMs for each model type.
// - Implement the modelx information structure.
//
// v2.0.0 - 17 October 2007, uBee
// - All FDD models can now load separate boot ROMs and a fall-back ROM will be
//   loaded for some models if the default ROM is not found.
// - Added the Teleterm 256TC, PJB P256K, and 256K models.
// - APC model name changed to 56K for ROM loading.
//
// v1.4.0 - 26 September 2007, uBee
// - Changes for banked and non-banked models.  Models of 64K or less are
//   set up for VDU to fixed at address 0xF000.
// - Added alternive boot ROMs to be loaded depending on the model being
//   emulated,  for ROM machines this means a BASIC ROM.
// - Added memmap_basicrom() function for Premium ROM model basic upper/lower
//   half selection.
// - loadrom() function modified and is now global function memmap_loadrom()
// - Removed the non aplpha+ source build option.
// - Added changes to error reporting.
//
// v1.0.0 - 7 July 2007, uBee
// - added vdu_switchvideo_in() and vdu_switchvideo_out() function calls.
// - ROMs 1-3 required separate handlers as there will be DRAM at 0xC000-0xDFFF
//   when ROM3 is selected.  This change allows the DRAM area to be written to.
//   A smaller dupwrite handler is also activated if the top and bottom banks
//   are the same and ROM3 is selected. (there is an 8K DRAM window).
// - According to the Technical manual ROM3 appears to be placed into wrong
//   memory location (0xC000) and should be placed into 0xE000.  Also ROM3 is
//   documented as being 8K and ROM1 and ROM2 are 16K.  Have made changes in
//   various locations for this and have modified the load_rom function and
//   optioned out the roundup function for now.
//
// - Converted to 512K DRAM model Microbee (as used for PJB 512K BIOS).  Now
//   uses array of pointers for banks.
//
// v0.0.0 - 5 June 2007, uBee
// Start with "nanowasp" source distribution version 0.22. An emulator for the
// microbee 128k. Copyright (C) 2000-2003  David G. Churchill.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ubee512.h"
#include "memmap.h"
#include "support.h"
#include "vdu.h"
#include "z80.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
#ifndef MEMMAP_HANDLER_1
static int handler_rindex;
static int handler_windex;
#endif

static void memmap_init4164 (void);
static void memmap_init4256 (void);

static uint8_t memmap_rom_ppc85_read (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_rom_56k_read (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_rom_basic_read (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_rom_pak_read (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_rom_net_read (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_rom1_dram_read (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_rom2_dram_read (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_rom3_dram_read (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_rom3x_dram_read (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_unhandled_read (uint32_t addr, struct z80_memory_read_byte *mem_s);

static void memmap_rom_basic_write (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s);
static void memmap_rom_pak_write (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s);
static void memmap_rom_net_write (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s);
static void memmap_unhandled_write (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s);

static void memmap_romxwrite (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s);
static uint8_t memmap_read_lo (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_read_lo_z (uint32_t addr, struct z80_memory_read_byte *mem_s);
static uint8_t memmap_read_hi (uint32_t addr, struct z80_memory_read_byte *mem_s);
static void memmap_write_lo (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s);
static void memmap_write_lo_z (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s);
static void memmap_write_hi (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s);

struct z80_memory_write_byte z80_mem_w[MAXMEMHANDLERS] =
{ { -1, -1, NULL, NULL } };

struct z80_memory_read_byte z80_mem_r[MAXMEMHANDLERS] =
{ { -1, -1, NULL, NULL } };

static uint8_t
   block00[BLOCK_SIZE], block01[BLOCK_SIZE], block02[BLOCK_SIZE], block03[BLOCK_SIZE],
   block04[BLOCK_SIZE], block05[BLOCK_SIZE], block06[BLOCK_SIZE], block07[BLOCK_SIZE],
   block08[BLOCK_SIZE], block09[BLOCK_SIZE], block10[BLOCK_SIZE], block11[BLOCK_SIZE],
   block12[BLOCK_SIZE], block13[BLOCK_SIZE], block14[BLOCK_SIZE], block15[BLOCK_SIZE],

   block16[BLOCK_SIZE], block17[BLOCK_SIZE], block18[BLOCK_SIZE], block19[BLOCK_SIZE],
   block20[BLOCK_SIZE], block21[BLOCK_SIZE], block22[BLOCK_SIZE], block23[BLOCK_SIZE],
   block24[BLOCK_SIZE], block25[BLOCK_SIZE], block26[BLOCK_SIZE], block27[BLOCK_SIZE],
   block28[BLOCK_SIZE], block29[BLOCK_SIZE], block30[BLOCK_SIZE], block31[BLOCK_SIZE],

   block32[BLOCK_SIZE], block33[BLOCK_SIZE], block34[BLOCK_SIZE], block35[BLOCK_SIZE],
   block36[BLOCK_SIZE], block37[BLOCK_SIZE], block38[BLOCK_SIZE], block39[BLOCK_SIZE],
   block40[BLOCK_SIZE], block41[BLOCK_SIZE], block42[BLOCK_SIZE], block43[BLOCK_SIZE],
   block44[BLOCK_SIZE], block45[BLOCK_SIZE], block46[BLOCK_SIZE], block47[BLOCK_SIZE],

   block48[BLOCK_SIZE], block49[BLOCK_SIZE], block50[BLOCK_SIZE], block51[BLOCK_SIZE],
   block52[BLOCK_SIZE], block53[BLOCK_SIZE], block54[BLOCK_SIZE], block55[BLOCK_SIZE],
   block56[BLOCK_SIZE], block57[BLOCK_SIZE], block58[BLOCK_SIZE], block59[BLOCK_SIZE],
   block60[BLOCK_SIZE], block61[BLOCK_SIZE], block62[BLOCK_SIZE], block63[BLOCK_SIZE];

uint8_t *const block_ptrs[BLOCK_TOTAL] =
   {
    block00, block01, block02, block03,
    block04, block05, block06, block07,
    block08, block09, block10, block11,
    block12, block13, block14, block15,

    block16, block17, block18, block19,
    block20, block21, block22, block23,
    block24, block25, block26, block27,
    block28, block29, block30, block31,

    block32, block33, block34, block35,
    block36, block37, block38, block39,
    block40, block41, block42, block43,
    block44, block45, block46, block47,

    block48, block49, block50, block51,
    block52, block53, block54, block55,
    block56, block57, block58, block59,
    block60, block61, block62, block63,
   };

memmap_t memmap =
{
 .backup = 1,
 .load = 1,
 .save = 1
}; 

static int blocksel_x;

static char name[512];
extern char userhome_srampath[];
extern char *model_args[];

extern char basic[];
extern char basic_alphap[];
extern char paks[];
extern char netx[];
extern char rom1[];
extern char rom2[];
extern char rom3[];

extern int basofs;
extern int pakofs;
extern int netofs;

extern emu_t emu;
extern model_t modelx;
extern model_custom_t modelc;
extern modio_t modio;

//==============================================================================
// Load an SRAM file or initialise SRAM.
//
// Used by ROM based and 56k models. Loads the SRAM file for the model being
// emulated if one exists.
//
// This emulates the battery backup of SRAM models.
//
// Note:
// block01 is 0x0000-0x7fff
// block00 is 0x8000-0xffff (56k model)
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
static int sram_load (void)
{
 FILE *fp = NULL;
 long len;

 char filepath[SSIZE1];
 int load_sram_file = memmap.backup && memmap.load;
 int max_size;
 int blk0_size;
 int blk1_size;
 int error;

 if (load_sram_file == 1)
    {
     if (*memmap.filepath)
        snprintf(name, sizeof(name), "%s", memmap.filepath);
     else
        snprintf(name, sizeof(name), "%s%s.ram", userhome_srampath,
        model_args[emu.model]);
     
     fp = open_file(name, userhome_srampath, filepath, "rb");
     
     load_sram_file = fp != NULL;
     
     // printf("file=%s\n", filepath);
    }

 if (load_sram_file)
    {
     fseek(fp, 0, SEEK_END);                // get size of the SRAM file
     len = ftell(fp);
     fseek(fp, 0, SEEK_SET);                // back to start

     max_size = (emu.model == MOD_56K)? 0x10000:0x8000;

     if (len > max_size)
        {
         xprintf("sram_load: SRAM file is too big (> %d): %s\n",
         max_size, name);
         fclose(fp);
         return -1;
        }
     else
        {
         blk1_size = (len < 0x8001)? len:0x8000;
         blk0_size = len - blk1_size;

         error = fread(block01, blk1_size, 1, fp) != 1;
         if (! error && emu.model == MOD_56K && blk0_size)
            error = fread(block00, blk0_size, 1, fp) != 1;
             
         if (error)
            {
             fclose(fp);
             xprintf("rtc_init: Unable to read SRAM data from %s\n", name);
             return -1;
            }
        }
     fclose(fp);
     return 0;
    }

 // if we did not load an SRAM file we initialize instead
 switch (emu.model)
    {
     case   MOD_56K : memmap_init6116(block00, 16);
                      memmap_init6116(block01, 16);
                      break;
     case MOD_TTERM :
     case MOD_PPC85 :
     case  MOD_PC85 :
     case    MOD_PC : memmap_init6116(block00, 16);
                      memmap_init6116(block01, 16);
                      break;
            default : memmap_init6116(block00, 16);
                      memmap_init6116(block01, 16);
                      break;
    }
 return 0;
}

//==============================================================================
// Save SRAM to file.
//
// Used by ROM based and 56k models. Saves the SRAM memory for the model being
// emulated if enabled.
//
// This emulates the battery backup of SRAM models.
//
// Note:
// block01 is 0x0000-0x7fff
// block00 is 0x8000-0xffff
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
static int sram_save (void)
{
 FILE *fp;
 char filepath[SSIZE1];

 if (! (memmap.backup && memmap.save))
    return 0;

 if (*memmap.filepath)
    snprintf(name, sizeof(name), "%s", memmap.filepath);
 else
    snprintf(name, sizeof(name), "%s%s.ram", userhome_srampath,
    model_args[emu.model]);

 fp = open_file(name, userhome_srampath, filepath, "wb");

 // if we can't create the file then ignore it as an error and only warn if
 // using verbose mode
 if (! fp)
    {
     if (emu.verbose)
        {
         emu.exit_warning = 1;
         emu.runmode = 0;
         xprintf("sram_save: Unable to create SRAM file: %s\n", name);         
        }
     return 0;
    }

 fwrite(block01, 1, 0x8000, fp);

 // write the rest of the 56k
 if (emu.model == MOD_56K)
    fwrite(block00, 1, 0x6000, fp);
 
 fclose(fp);
 return 0;
}

//==============================================================================
// Initialise the memory map.
//
// Bank memory handling is used for models with more than 64K of RAM/DRAM.
// Models with less than 64K do not have bank switching and the VDU is fixed
// at 0xF000.
//
// DRAM and RAM is initialised to typical patterns for each type of memory
// used for the emulated model.
//
//   pass: void
// return: int                  0 if no errors, -1 if error
//==============================================================================
int memmap_init (void)
{
 int i;

 if ((emu.model == MOD_SCF) || (emu.model == MOD_PCF))
    {
     if (emu.cfmode)
        {
         emu.port50h = B8(00000001);
         emu.port51h = BANK_CF_PC85;
        }
     else
        {
         emu.port50h = 0;
         emu.port51h = 0;
        }
    }
 else
    emu.port50h = 0;

 if (modelx.ram >= 64)
    {
     if (modio.raminit)
        // for debugging purposes, fills each 32K DRAM block with numbers.
        {
         for (i = 0; i < BLOCK_TOTAL; i++)
            memset(block_ptrs[i], i, BLOCK_SIZE);
        }
     else
        {
         if (modelx.ram <= 128)
            memmap_init4164();
         else
            memmap_init4256();
        }
    }
 else
    // initialise for RAM models (< 64K)
    {
     blocksel_x = 1;  // block01 for 0x0000-0x7fff

     if (modio.raminit)
        // for debugging purposes, fills each 32K RAM section with numbers.
        {
         memset(block00, 0, 0x8000);
         memset(block01, 1, 0x8000);
        }
     else
        // load a SRAM battery backed up file or initialise static RAM to
        // typical pattern values
        if (sram_load() != 0)
           return -1;
    }

 memmap_configure();

 return 0;
}

//==============================================================================
// De-initialise the memory map.
//
//   pass: void
// return: int                  0
//==============================================================================
int memmap_deinit (void)
{
 if (modelx.ram < 64)
    return sram_save();
 return 0;
}

//==============================================================================
// Set memory map to the reset condition.
//
//   pass: void
// return: int                  0
//==============================================================================
int memmap_reset (void)
{
 if (modelx.ram >= 64)
    {
     emu.port50h = 0;
     emu.port51h &= BANK_CF_PC85; // refer to CF_Coreboard_MMU_notes.txt
     memmap_configure();
    }

 basofs = 0;

 return 0;
}

//==============================================================================
// Initialise RAM to look like typical 4164 DRAM Chips.
//
// Pattern: 128 x 00s - 128 x FFs - repeat
//
//   pass: void
// return: void
//==============================================================================
static void memmap_init4164 (void)
{
 int i, x, z, a;
 uint8_t *dram;

 for (i = 0; i < BLOCK_TOTAL; i++)
    {
     dram = block_ptrs[i];
     a = 0;
     for (x = 0; x < 128; x++)
        {
         for (z = 0; z < 128; z++)
            *(dram + a++) = 0x00;
         for (z = 0; z < 128; z++)
            *(dram + a++) = 0xFF;
        }
    }
}

//==============================================================================
// Initialise RAM to look like typical 4256 DRAM Chips.
//
// Pattern: 128 x 00:FF - 128 x FF:00 - repeat
//
//   pass: void
// return: void
//==============================================================================
static void memmap_init4256 (void)
{
 int i, x, z, a;
 uint8_t *dram;

 for (i = 0; i < BLOCK_TOTAL; i++)
    {
     dram = block_ptrs[i];
     a = 0;
     for (x = 0; x < 64; x++)
        {
         for (z = 0; z < 128; z++)
            {
             *(dram + a++) = 0x00;
             *(dram + a++) = 0xFF;
            }
         for (z = 0; z < 128; z++)
            {
             *(dram + a++) = 0xFF;
             *(dram + a++) = 0x00;
            }
        }
    }
}

//==============================================================================
// Initialise RAM (in 2K chunks) to look like typical 6264 static RAM chips.
//
// Pattern: 64 x FF:FF:00:00 - 64 x 00:00:FF:FF - repeat
//
//   pass: char *mem            pointer to the memory to be initialised
//   pass: int banks            number of 2K banks of consecutive memory
// return: void
//==============================================================================
void memmap_init6264 (uint8_t *mem, int banks)
{
 int x, z, a;

 a = 0;
 while (banks--)
    {
     for (x = 0; x < 4; x++)
        {
         for (z = 0; z < 64; z++)
            {
             *(mem + a++) = 0xFF;
             *(mem + a++) = 0xFF;
             *(mem + a++) = 0x00;
             *(mem + a++) = 0x00;
            }
         for (z = 0; z < 64; z++)
            {
             *(mem + a++) = 0x00;
             *(mem + a++) = 0x00;
             *(mem + a++) = 0xFF;
             *(mem + a++) = 0xFF;
            }
        }
    }
}

//==============================================================================
// Initialise RAM (in 2K chunks) to look like typical Hitachi 6116 static
// RAM chips.  Uses the patterns found on a typical 32K IC ROM Microbee.
//
//   pass: uint8_t *mem            pointer to the memory to be initialised
//   pass: int banks            number of 2K banks of consecutive memory
// return: void
//==============================================================================
void memmap_init6116 (uint8_t *mem, int banks)
{
 int x, z, a;

 a = 0;
 while (banks--)
    {
     for (x = 0; x < 4; x++)
        {
#if 0 // use the 6116 pattern
         for (z = 0; z < 64; z++)
            {
             *(mem + a++) = 0xFF;
             *(mem + a++) = 0x00;
             *(mem + a++) = 0x00;
             *(mem + a++) = 0xFF;
             *(mem + a++) = 0x00;
             *(mem + a++) = 0xFF;
             *(mem + a++) = 0xFF;
             *(mem + a++) = 0x00;
            }
#else // use the 6264 pattern
         for (z = 0; z < 64; z++)
            {
             *(mem + a++) = 0xFF;
             *(mem + a++) = 0xFF;
             *(mem + a++) = 0x00;
             *(mem + a++) = 0x00;
            }
         for (z = 0; z < 64; z++)
            {
             *(mem + a++) = 0x00;
             *(mem + a++) = 0x00;
             *(mem + a++) = 0xFF;
             *(mem + a++) = 0xFF;
            }
#endif
        }
    }
}

//==============================================================================
// Set memory map mode 1 - Port 0x50 write handler.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void memmap_mode1_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.mem)
    log_port_1("memmap_mode1_w", "data", port, data);

 if (modelx.ram < 64)
    return;

 if (data != emu.port50h)
    {
     emu.port50h = data;
     memmap_configure();
    }
}

//==============================================================================
// Set memory map mode 2 - Port 0x51 write handler.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void memmap_mode2_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.mem)
    log_port_1("memmap_mode2_w", "data", port, data);

 if (modelx.ram < 64)
    return;

 if (data != emu.port51h)
    {
     emu.port51h = data;
     memmap_configure();
    }
}

//==============================================================================
// Insert a memory read handler.
//
//   pass: int addr_l
//         int addr_h
//         void *f
// return: void
//==============================================================================
static void set_read_handler (int addr_l, int addr_h, void *f)
{
#ifdef MEMMAP_HANDLER_1
 int i = (addr_l & MEMMAP_MASK) >> MEMMAP_SHIFT;
 int h = (addr_h & MEMMAP_MASK) >> MEMMAP_SHIFT;

 while (i <= h)
    {
     if ((z80_mem_r[i].memory_call == memmap_unhandled_read) || (f == memmap_unhandled_read))
        z80_mem_r[i].memory_call = f;
     i++;
    }
#else
 if (f == NULL)
    return;
 z80_mem_r[handler_rindex].low_addr = addr_l;
 z80_mem_r[handler_rindex].high_addr = addr_h;
 z80_mem_r[handler_rindex].memory_call = f;
 z80_mem_r[handler_rindex].p_user_area = NULL;
 handler_rindex++;
#endif
}

//==============================================================================
// Insert a memory write handler.
//
//   pass: int addr_l
//         int addr_h
//         void *f
// return: void
//==============================================================================
static void set_write_handler (int addr_l, int addr_h, void *f)
{
#ifdef MEMMAP_HANDLER_1
 int i = (addr_l & MEMMAP_MASK) >> MEMMAP_SHIFT;
 int h = (addr_h & MEMMAP_MASK) >> MEMMAP_SHIFT;

 while (i <= h)
    {
     if ((z80_mem_w[i].memory_call == memmap_unhandled_write) || (f == memmap_unhandled_write))
        z80_mem_w[i].memory_call = f;
     i++;
    }
#else
 if (f == NULL)
    return;
 z80_mem_w[handler_windex].low_addr = addr_l;
 z80_mem_w[handler_windex].high_addr = addr_h;
 z80_mem_w[handler_windex].memory_call = f;
 z80_mem_w[handler_windex].p_user_area = NULL;
 handler_windex++;
#endif
}

//==============================================================================
// Insert ROMs handler for ROM based and CF models.
//
//   pass: void
// return: void
//==============================================================================
static void set_roms_sram_handler (void)
{
 // set write handlers for BASIC ROM location.
 if ((modelc.basram) || (emu.model == MOD_TTERM))
    {
     // if alpha+ model (PPC85 and Teleterm) then only the 8K ROM can be SRAM
     if ((emu.model == MOD_PPC85) || (emu.model == MOD_TTERM) || (emu.model == MOD_PCF))
        set_write_handler(0xA000, 0xBFFF, memmap_rom_basic_write);
     else
        set_write_handler(0x8000, 0xBFFF, memmap_rom_basic_write);
    }
 else
    set_write_handler(0x8000, 0xBFFF, memmap_romxwrite);

 // set write handler for PAK ROM location.
 if (modelc.pakram[modelc.paksel & 0x07])
    set_write_handler(0xC000, 0xDFFF, memmap_rom_pak_write);
 else
    set_write_handler(0xC000, 0xDFFF, memmap_romxwrite);

 // set write handler for Net ROM location.
 if (modelc.netram)
    set_write_handler(0xE000, 0xEFFF, memmap_rom_net_write);
 else
    set_write_handler(0xE000, 0xEFFF, memmap_romxwrite);

 // set BASIC ROM read handler if an alpha+ model (PPC85/Teleterm/PCF)
 if ((emu.model == MOD_PPC85) || (emu.model == MOD_TTERM) || (emu.model == MOD_PCF))
    set_read_handler(0x8000, 0x9FFF, memmap_rom_ppc85_read);

 // set read handlers for BASIC/PAK/Net ROM locations.
 set_read_handler(0x8000, 0xBFFF, memmap_rom_basic_read);
 set_read_handler(0xC000, 0xDFFF, memmap_rom_pak_read);
 set_read_handler(0xE000, 0xEFFF, memmap_rom_net_read);
}

//==============================================================================
// Insert ROMs handler for DRAM models.
//
// Notes:
// - The 256TC and Premium Plus models only have ROM1, ROM2 and ROM3 are not
//   used as the ROM bit is used to select extra DRAM banks.
// - ROM 1 is located at 8000-BFFF.
// - ROM 2 is located at C000-FFFF.
// - ROM 3 is located at E000-FFFF. (C000-DFFF is filled with FFs)
// - BANK_NOROMS = 00000100 (enable ROMs bit=0)
// - BANK_ROM3 = 00100000 (ROM 2 bit 5=0, ROM 3 bit 5=1)
//
//   pass: void
// return: void
//==============================================================================
static void set_roms_dram_handler (void)
{
 // insert ROM memory handlers if any ROMs are enabled.
 if (emu.port50h & BANK_NOROMS)
    return;

 set_read_handler(0x8000, 0xBFFF, memmap_rom1_dram_read);

 // set writes to ROM #1 to be ignored.
 set_write_handler(0x8000, 0xBFFF, memmap_romxwrite);

 // if a 256TC or Premium Plus model then exit as there is no ROM2 or ROM3
 if (emu.model == MOD_256TC || emu.model == MOD_1024K)
    return;

 if (emu.port50h & BANK_ROM3)
    {
     set_read_handler(0xE000, 0xFFFF, memmap_rom3_dram_read);

     // See Notes above.
     set_read_handler(0xC000, 0xDFFF, memmap_rom3x_dram_read);

     // set writes to ROM #3 to be ignored.
     set_write_handler(0xC000, 0xFFFF, memmap_romxwrite);
    }
 else
    {
     set_read_handler(0xC000, 0xFFFF, memmap_rom2_dram_read);

     // set writes to ROM #2 to be ignored.
     set_write_handler(0xC000, 0xFFFF, memmap_romxwrite);
    }
}

//==============================================================================
// Insert video handler for banked memory models.
//
// If VDU is enabled then set the VDU memory handler for 8000-8FFF or
// F000-FFFF.
//
// Notes:
// BANK_VRAM = 00001000 (enable VDU RAM bit=0)
// BANK_VADD = 00010000 (VDU RAM at F000 bit=0, 8000 bit=1)
//
//   pass: void
// return: void
//==============================================================================
static void set_video_banked_handler (void)
{
 // insert video memory handler if video RAM is enabled
 if (emu.port50h & BANK_VRAM)  // if video RAM disabled
    return;

 if (emu.port50h & BANK_VADD)  // if video RAM at 8000-8FFF
    {
     set_read_handler(0x8000, 0x8FFF, vdu_vidmem_r);
     set_write_handler(0x8000, 0x8FFF, vdu_vidmem_w);
    }
 else
    {
     set_read_handler(0xF000, 0xFFFF, vdu_vidmem_r);
     set_write_handler(0xF000, 0xFFFF, vdu_vidmem_w);
    }
}

//==============================================================================
// Insert video handler for non-banked memory models.
//
//   pass: void
// return: void
//==============================================================================
static void set_video_nonbanked_handler (void)
{
 // insert VDU read and write handlers
 set_read_handler(0xF000, 0xFFFF, vdu_vidmem_r);
 set_write_handler(0xF000, 0xFFFF, vdu_vidmem_w);
}

//==============================================================================
// Configure CF memory map.
//
// Notes:
// - See CF project 'CF_Coreboard_MMU_notes.txt' for more information
// - When no ROMs are in the memory map bit 1 of the DRAM select needs to be
//   inverted. (Possibly needed on select bits > 128K too)
//
//   pass: void
// return: void
//==============================================================================
static void cf_map_configure (void)
{
 // select one of the 64 available DRAM banks
 blocksel_x = ((emu.port50h & B8(11000000)) >> 4) | (emu.port50h & B8(00000011)) |
 ((emu.port51h & B8(00000011)) << 4);

 if (emu.port50h & BANK_NOROMS)
    blocksel_x ^= B8(00000010);

 // enable Pak and Net ports if in PC85 mode
 z80_cf_ports();

#ifdef MEMMAP_HANDLER_1
 // initialise the tables
 set_read_handler(0x0000, 0xFFFF, memmap_unhandled_read);
 set_write_handler(0x0000, 0xFFFF, memmap_unhandled_write);
#else
 handler_rindex = 0;
 handler_windex = 0;
#endif

 if (emu.port51h & BANK_CF_PC85)
    {
     // insert video memory handler
     set_video_nonbanked_handler();
     // insert BASIC ROM write handler if PC85 model (can not be RAM)
     set_write_handler(0x8000, 0xBFFF, memmap_romxwrite);
     // insert ROMs handler for SRAM models (if PC85 mode)
     set_roms_sram_handler();
    }
 else
    {
     // insert video memory handler (if enabled)
     set_video_banked_handler();
     // insert ROMs handler for DRAM models
     set_roms_dram_handler();
    }

 // insert main memory write handlers
 set_write_handler(0x0000, 0x7FFF, memmap_write_lo);
 set_write_handler(0x8000, 0xFFFF, memmap_write_hi);

 // insert main memory read handlers
 set_read_handler(0x0000, 0x7FFF, memmap_read_lo);
 set_read_handler(0x8000, 0xFFFF, memmap_read_hi);

#ifndef MEMMAP_HANDLER_1
 // should not get here!
 set_read_handler(0x0000, 0xFFFF, memmap_unhandled_read);
 set_write_handler(0x0000, 0xFFFF, memmap_unhandled_write);
#endif
}

//==============================================================================
// Configure DRAM model memory map.
//
// Notes:
// - VDU RAM takes precedence over ROM (tech86 8-10) and DRAM if port 50
//   bit 3=0 (VDU enabled)
// - Memory handlers are used in the order they are inserted,  the higher
//   precedence handler should be inserted before a lower precendence
//   handler.
// - When no ROMs are in the memory map bit 1 of the DRAM select needs to be
//   inverted. (Possibly needed on select bits > 128K too)
//
//   pass: void
// return: void
//==============================================================================
static void dram_map_configure (void)
{
 int invert_bits;

 // set blocksel_x values depending on the model emulated
 switch (modelx.ram)
    {
     case 1024 : // Premium Plus
        blocksel_x = ((emu.port50h & B8(11100000)) >> 3) |
        (emu.port50h & B8(00000011));
        invert_bits = B8(00000010);
        break;
     case 512 :
        blocksel_x = ((emu.port50h & B8(11000000)) >> 4) |
        (emu.port50h & B8(00000011));
        invert_bits = B8(00000010);
        break;
     case 256 :
        switch (emu.model)
           {
            case MOD_256TC : // 256TC Telecomputer
               blocksel_x = ((emu.port50h & B8(00100000)) >> 3) |
               (emu.port50h & B8(00000011));
               break;
            default:
               blocksel_x = ((emu.port50h & B8(01000000)) >> 4) |
               (emu.port50h & B8(00000011));
               break;
           }
        invert_bits = B8(00000010);
        break;
     case 128 :
        blocksel_x = emu.port50h & B8(00000011);
        invert_bits = B8(00000010);
        break;
     case 64 :
        blocksel_x = emu.port50h & B8(00000011);
        invert_bits = B8(00000010);
        break;
      default :
        blocksel_x = 0;
        invert_bits = 0;
        break;
    }

 // if no ROMs selected then invert the DRAM select bit 1
 if (emu.port50h & BANK_NOROMS)
    blocksel_x ^= invert_bits;

#ifdef MEMMAP_HANDLER_1
 // initialise the tables
 set_read_handler(0x0000, 0xFFFF, memmap_unhandled_read);
 set_write_handler(0x0000, 0xFFFF, memmap_unhandled_write);
#else
 handler_rindex = 0;
 handler_windex = 0;
#endif

 // insert main memory write handler
 if (modelx.ram == 64 && (emu.port50h & B8(00000001)))
    set_write_handler(0x0000, 0x7FFF, memmap_write_lo_z);
 else
    set_write_handler(0x0000, 0x7FFF, memmap_write_lo);

 // insert video memory handler (if enabled)
 set_video_banked_handler();

 // insert ROMs handler for DRAM models (if enabled)
 set_roms_dram_handler();

 // insert DRAM high bank memory write handler
 set_write_handler(0x8000, 0xFFFF, memmap_write_hi);

 // insert main memory read handlers
 if (modelx.ram == 64 && (emu.port50h & B8(00000001)))
    set_read_handler(0x0000, 0x7FFF, memmap_read_lo_z);
 else
    set_read_handler(0x0000, 0x7FFF, memmap_read_lo);
 set_read_handler(0x8000, 0xFFFF, memmap_read_hi);

#ifndef MEMMAP_HANDLER_1
 // should not get here!
 set_read_handler(0x0000, 0xFFFF, memmap_unhandled_read);
 set_write_handler(0x0000, 0xFFFF, memmap_unhandled_write);
#endif
}

//==============================================================================
// Configure SRAM models memory mapping handlers for ROM and 56K models.
//
// Notes:
// - Memory handlers are used in the order they are inserted,  the higher
//   precedence handler should be inserted before a lower precendence
//   handler.
//
//   pass: void
// return: void
//==============================================================================
static void sram_map_configure (void)
{
#ifdef MEMMAP_HANDLER_1
 // initialise the tables
 set_read_handler(0x0000, 0xFFFF, memmap_unhandled_read);
 set_write_handler(0x0000, 0xFFFF, memmap_unhandled_write);
#else
 handler_rindex = 0;
 handler_windex = 0;
#endif

 // insert main memory access handlers
 if (emu.model == MOD_56K || emu.model == MOD_2MHZDD || emu.model == MOD_DD)
    {
     set_read_handler(0x0000, 0x7FFF, memmap_read_lo);
     set_read_handler(0x8000, 0xDFFF, memmap_read_hi);
     set_write_handler(0x0000, 0x7FFF, memmap_write_lo);
     set_write_handler(0x8000, 0xDFFF, memmap_write_hi);
     set_read_handler(0xE000, 0xEFFF, memmap_rom_56k_read);
     set_write_handler(0xE000, 0xEFFF, memmap_romxwrite);
    }
 else
    {
     // insert handlers to suit the SRAM size
     if (modelx.ram > 0)
        {
         set_read_handler(0x0000, modelx.ram * 1024 - 1, memmap_read_lo);
         set_write_handler(0x0000, modelx.ram * 1024 - 1, memmap_write_lo);
        }
     if (modelx.ram < 32)
        {
         set_read_handler(modelx.ram * 1024, 0x7FFF, memmap_read_lo_z);
         set_write_handler(modelx.ram * 1024, 0x7FFF, memmap_write_lo_z);
        }

     // insert ROMs handler for ROM based models
     set_roms_sram_handler();
    }

 // insert VDU read and write handlers
 set_video_nonbanked_handler();

 // insert BASIC ROM write handler if an alpha+ model (PPC85 and Teleterm)
 // this 0x8000-0xBFFF location can not be SRAM
 if ((emu.model == MOD_PPC85) || (emu.model == MOD_TTERM))
    set_write_handler(0x8000, 0xBFFF, memmap_romxwrite);

#ifndef MEMMAP_HANDLER_1
 // should not get here!
 set_read_handler(0x0000, 0xFFFF, memmap_unhandled_read);
 set_write_handler(0x0000, 0xFFFF, memmap_unhandled_write);
#endif
}

//==============================================================================
// ROM read for PPC85 and Teleterm models (0x8000-0x9FFF)
//
// The hardware for these models have 2 x 8K banks selectable at 0x8000.  This
// handler must be inserted before the memmap_rom_basic_read() handler.
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_rom_ppc85_read (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return basic_alphap[basofs + (addr & 0x1FFF)];
}

//==============================================================================
// ROM read for 56K SRAM models (0xE000-0xEFFF)
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_rom_56k_read (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return rom1[addr & 0x0FFF];
}

//==============================================================================
// ROM read for BASIC (0x8000-0xBFFF)
//
// On alpha+ models the ROM is only 8K so must be loaded into the upper 8K
// section.  The mask value then works for both standard and alpha+ models.
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_rom_basic_read (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return basic[addr & 0x3FFF];
}

//==============================================================================
// ROM read for Pak (0xC000-0xDFFF)
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_rom_pak_read (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return paks[pakofs + (addr & 0x1FFF)];
}

//==============================================================================
// ROM read for Net (0xE000-0xEFFF)
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_rom_net_read (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return netx[netofs + (addr & 0x0FFF)];
}

//==============================================================================
// ROM #1 read for DRAM based models (0x8000-0xBFFF)
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_rom1_dram_read (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return rom1[addr & 0x3FFF];
}

//==============================================================================
// ROM #2 read for DRAM based models (0xC000-0xFFFF)
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_rom2_dram_read (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return rom2[addr & 0x3FFF];
}

//==============================================================================
// ROM #3 read for DRAM based models (0xE000-0xFFFF)
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_rom3_dram_read (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return rom3[addr & 0x1FFF];
}

//==============================================================================
// ROM #3x read for DRAM based models (0xE000-0xFFFF)
//
// Will come here if ROM #3 is in and 0xC000-0xDFFF is read.  An 0xFF value
// is returned as happens on a real Micorbee. (according to tests)
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_rom3x_dram_read (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return 0xFF;
}

//==============================================================================
// read a byte from an SRAM/DRAM memory location (0x0000-0x7FFF)
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_read_lo (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return *(block_ptrs[blocksel_x] + addr);
}

//==============================================================================
// read a 0 from an SRAM/DRAM memory location (0x0000-0x7FFF)
//
// This is required for 64K DRAM models when port 50h bit 0 is set
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_read_lo_z (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return 0;
}

//==============================================================================
// read a byte from an SRAM/DRAM memory location (0x8000-0xFFFF)
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_read_hi (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 return block00[addr & 0x7FFF];
}

//==============================================================================
// Unhandled read
//
// Each 1000H page of memory defaults to this handler. The set_read_handler()
// function overwrites with appropriate handlers.
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
static uint8_t memmap_unhandled_read (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
 xprintf("memmap_unhandled_read: addr=0x%04x\n", addr);
 return 0;
}

//==============================================================================
// ROM x write (0x8000-0xEFFF)
//
// This handler is installed for any addresses to ROMs.  This prevents writing
// to ROM image memory.
//
//   pass: uint32_t addr
//         uint8_t data
//         struct z80_memory_write_byte *mem_s
// return: void
//==============================================================================
static void memmap_romxwrite (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s)
{
}

//==============================================================================
// ROM BASIC write (0x8000-0xBFFF)
//
// This handler is installed if BASIC ROM is configured to be SRAM by using
// the --basram option or the Teleterm model is being emulated.
//
//   pass: uint32_t addr
//         uint8_t data
//         struct z80_memory_write_byte *mem_s
// return: void
//==============================================================================
static void memmap_rom_basic_write (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s)
{
 basic[addr & 0x3FFF] = data;
}

//==============================================================================
// ROM PAK write (0xC000-0xDFFF)
//
// This handler is installed if a PAK ROM is configured to be SRAM when using
// the --pakram option.
//
//   pass: uint32_t addr
//         uint8_t data
//         struct z80_memory_write_byte *mem_s
// return: void
//==============================================================================
static void memmap_rom_pak_write (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s)
{
 paks[pakofs + (addr & 0x1FFF)] = data;
}

//==============================================================================
// ROM Net write (0xE000-0xEFFF)
//
// This handler is installed if a Net ROM is configured to be SRAM when using
// the --netram option.
//
//   pass: uint32_t addr
//         uint8_t data
//         struct z80_memory_write_byte *mem_s
// return: void
//==============================================================================
static void memmap_rom_net_write (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s)
{
 netx[netofs + (addr & 0x0FFF)] = data;
}

//==============================================================================
// write a byte to an SRAM/DRAM memory location (0x0000-0x7FFF)
//
//   pass: uint32_t addr
//         uint8_t data
//         struct z80_memory_write_byte *mem_s
// return: void
//==============================================================================
static void memmap_write_lo (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s)
{
 *(block_ptrs[blocksel_x] + addr) = data;
}

//==============================================================================
// write nothing to an SRAM/DRAM memory location (0x0000-0x7FFF)
//
// This is required for 64K DRAM models when port 50h bit 0 is set
//
//   pass: uint32_t addr
//         uint8_t data
//         struct z80_memory_write_byte *mem_s
// return: void
//==============================================================================
static void memmap_write_lo_z (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s)
{
}

//==============================================================================
// write a byte to an SRAM/DRAM memory location (0x8000-0xFFFF)
//
//   pass: uint32_t addr
//         uint8_t data
//         struct z80_memory_write_byte *mem_s
// return: void
//==============================================================================
static void memmap_write_hi (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s)
{
 block00[addr & 0x7FFF] = data;
}

//==============================================================================
// Unhandled write.
//
// Each 1000H page of memory defaults to this handler. The set_write_handler()
// function overwrites with appropriate handlers.
//
//   pass: uint32_t addr
//         uint8_t data
//         struct z80_memory_write_byte *mem_s
// return: void
//==============================================================================
static void memmap_unhandled_write (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s)
{
 xprintf("memmap_unhandled_write: addr=0x%04x\n", addr);
}

//==============================================================================
// Get Z80 pointer to current SRAM/DRAM bank. The upper and lower banks will
// be determined by the address passed.
//
// This is intended to be used by the function module and will only work
// if the data being accessed does not cross a 0x8000 memory boundary!
//
// The Z80 address will need to be masked with 0x7FFF before using the pointer.
//
//   pass: int addr
// return: char *
//==============================================================================
uint8_t *memmap_get_z80_ptr (int addr)
{
 if (addr < 0x8000)
    return block_ptrs[blocksel_x];
 else
    return block00;
}

//==============================================================================
// Configure memory map for all models.
//
//   pass: void
// return: void
//==============================================================================
void memmap_configure (void)
{
 if ((emu.model == MOD_SCF) || (emu.model == MOD_PCF))
    {
     cf_map_configure();
     return;
    }

 if (modelx.ram >= 64)
    dram_map_configure();
 else
    sram_map_configure();
}
