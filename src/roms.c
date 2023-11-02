//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                                roms module                                 *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// This module is used for loading all ROMs on start-up and for selecting
// Pak and Net ROMs when emulating a ROM model.
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
// - Microbee memory is now an array of uint8_t rather than char.
//
// v5.5.0 - 1 July 2013, uBee
// - Added 256TC v1.31 ROM Y2K century mod to roms_md5_check().
// - Code cleanup of roms_load_config_boot_basic() to remove 'dest' variable
//   and set code as was unused and other tidy ups.
//
// v5.4.0 - 2 Jan 2012, uBee
// - Added Microbee Technology's p1024k/1024k (pplus) model emulation.
//
// v4.6.0 - 4 May 2010, uBee
// - Added roms_proc_pak_argument() function to process --pak arguments.
//   --pak(n) options can now also specify 4K ROMs for A and B.  The 2mhz
//   model can load 4K EPROM images by default.  See notes for
//   roms_load_paks().
// - Changes to roms_load_config_ppc85() to remove loading of TTERM_B.ROM as
//   there is no Basic ROM B for the Teleterm model (occupied by 8K SRAM)
// - Changes made to roms_nsel_r() to use the 16 bit port value as was
//   originally intended.
// - Changes and improvements made to roms_loadrom() to re-scan the ROMs
//   directory for MD5s if a ROM is not found, one more try is then made
//   before returning an error.
// - Fixed problem in roms_load_config_boot_basic() for the 'size'
//   calculation directly after the call to roms_loadrom().
// - Renamed log_port() calls to log_port_1().
//
// v4.5.0 - 2 April 2010, uBee
// - Added roms_create_md5() function to create a ROMs MD5 file. The old
//   'roms.md5' file is no longer used unless renamed to 'roms.md5.user' by
//   the user.  The ROMs MD5 file may now use 'roms.md5.user' or
//   'roms.md5.auto' in that order. The auto file will be generated if it
//   does not exist or the user forces it's creation with the --md5-create
//   option.
//
// v4.4.0 - 14 August 2009, uBee
// - Added MOD_PC85B model for Standard PC85 models that use 16K Pak ROMs.
//
// v4.3.0 - 31 July 2009, uBee
// - Added '2mhzdd' and 'dd' Dreamdisk models (workerbee).
//
// v4.1.0 - 7 July 2009, uBee
// - Created new file and moved code over from paknet.c and ROM code from
//   memmap.c.  paknet.c has been removed.  All ROMs are now handled from
//   this one file. The older Changelog notes are from paknet.c
// - All empty ROM locations are pre-initialised with 0xFFs.
// - Many changes to improve the code.
//
// v4.0.0 - 3 June 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Memory management has changed. Memory copies between banks has been
//   removed.  Access to Z80 memory is now handled directly.
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
//
// v3.1.0 - 17 March 2009, uBee
// - Added MD5 code to check for known bad ROMs to decide if to apply patches.
// - Fixed telcom v3.21 patch code as was incomplete,  Test code now passes
//   the ROM A, ROM B and Telcom ROM checks.
// - Changed all printf() calls to use a local xprintf() function.
//
// v2.7.0 - 12 June 2008, uBee
// - TTERM-X.ROM and PREM-X.ROM names have been changed.
// - PREM_NETWORK.ROM is now PPC85_NETWORK.ROM.
// - PC_EDASM.ROM and PC_WBEE.ROM are now PC_PAK0.ROM and PC_PAK1.ROM.
// - IC_WBEE.ROM is now IC_PAK0.ROM.
// - New open_file() function in function.c module is now used by the
//   memmap_loadrom() function to determine the path to the ROMs. Added full
//   file path return variable.
// - Fixed a warning in paknet_nsel_r() function to return 0.
// - Added structure emu_t and moved variables into it.
//
// v2.6.0 - 7 May 2008, uBee
// - Changes so that ROM models 0xC000-0xDFFF can be organised to be SRAM or
//   ROM images.
// - Added PAK1-PAK7 for 2mhz model as these can have additional PAKs if a ROM
//   PAK was used, switching use OUT 10,n then EDASM in BASIC < v5.22.
// - Copy lower half of PAK image to upper half if only 8K image,  this is
//   required for later ROM models that can use 16K ROMs.
//
// v2.5.0 - 9 March 2008, uBee
// - Added Teleterm model emulation.
// - The PAK and NET ROMs can now be specified as user options to override
//   the built in model defaults.
// - Implement the modelc structure.
//
// v2.3.0 - 24 December 2007, uBee
// - Added modio_t structure.
//
// v2.1.0 - 25 October 2007, uBee
// - Attempt a repair of a known bad Telcom v3.2.1 ROM image if detected.
// - Fixed bug in paknet_nsel_r() function. Switch statement had used incorrect
//   variable (rom_model) instead of (model) so value would have been always 1
//   and no matching case.  New code uses (modelx.model)
//
// v1.4.0 - 2 October 2007, uBee
// - Fully implemented PAK and NET ROMs selection for various ROM models.
//
// v1.0.0 - 3 July 2007, uBee
// - Created a new file and implement the PAK and NET ports
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "roms.h"
#include "z80api.h"
#include "ubee512.h"
#include "vdu.h"
#include "z80.h"
#include "memmap.h"
#include "support.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
int basofs;
int pakofs;
int netofs;

static int pakdata;
static int netbank;

uint8_t basic[0x4000];
uint8_t basic_alphap[0x4000];
uint8_t paks[0x4000 * 8];
uint8_t netx[0x4000];
uint8_t rom1[ROM1_SIZE];
uint8_t rom2[ROM2_SIZE];
uint8_t rom3[ROM3_SIZE];

extern char userhome[];
extern char userhome_romspath[];
extern uint8_t ic_82s23[];

extern vdu_t vdu;
extern emu_t emu;
extern model_t modelx;
extern modio_t modio;
extern model_custom_t modelc;

static int roms_load_all (void);

//==============================================================================
// Define all the possible ROM boot up options for disk and ROM based models.
//
// The boot_data table must match the enumeration table order containing
// MOD_NAMES (in ubee512.h)
//==============================================================================
static boot_t boot_data [MOD_TOTAL] =
   {
    // MOD_256TC
    {
     {"256TC.ROM", "", "", ""},
      rom1,
      0,
      0x4000,
      "",
      ""
     },

    // MOD_P1024K
    {
     {"P1024K.ROM", "BOOT_1024K.ROM", "", ""},
     rom1,
     0,
     0x4000,
     "P1024K_2.ROM",
     "P1024K_3.ROM",
    },

    // MOD_1024K
    {
     {"1024K.ROM", "BOOT_1024K.ROM", "", ""},
     rom1,
     0,
     0x4000,
     "1024K_2.ROM",
     "1024K_3.ROM",
    },

    // MOD_P512K
    {
     {"P512K.ROM", "BOOT_128K.ROM", "", ""},
     rom1,
     0,
     0x4000,
     "P512K_2.ROM",
     "P512K_3.ROM"
    },

    // MOD_512K
    {
     {"512K.ROM", "BOOT_128K.ROM", "", ""},
     rom1,
     0,
     0x4000,
     "512K_2.ROM",
     "512K_3.ROM"
    },

    // MOD_P256K
    {
     {"P256K.ROM", "BOOT_128K.ROM", "", ""},
     rom1,
     0,
     0x4000,
     "P256K_2.ROM",
     "P256K_3.ROM"
    },

    // MOD_256K
    {
     {"256K.ROM", "BOOT_128K.ROM", "", ""},
     rom1,
     0,
     0x4000,
     "256K_2.ROM",
     "256K_3.ROM"
    },

    // MOD_P128K
    {
     {"P128K.ROM", "BOOT_128K.ROM", "", ""},
     rom1,
     0,
     0x4000,
     "P128K_2.ROM",
     "P128K_3.ROM"
    },

    // MOD_128K
    {
     {"128K.ROM", "BOOT_128K.ROM", "", ""},
     rom1,
     0,
     0x4000,
     "128K_2.ROM",
     "128K_3.ROM"
    },

    // MOD_P64K
    {
     {"P64K.ROM", "", "", ""},
     rom1,
     0,
     0x4000,
     "P64K_2.ROM",
     "P64K_3.ROM"
    },

    // MOD_64K
    {
     {"64K.ROM", "", "", ""},
     rom1,
     0,
     0x4000,
     "64K_2.ROM",
     "64K_3.ROM"
    },

    // MOD_56K
    {
     {"56K.ROM", "", "", ""},
     rom1,
     0,
     0x1000,
     "",
     ""
    },

    // MOD_TTERM
    {
     {"TTERM_A.ROM", "", "", ""},
     basic_alphap,
     0,
     0x4000,
     "",
     ""
    },

    // MOD_PPC85
    {
     {"PPC85_A.ROM", "", "", ""},
     basic_alphap,
     0,
     0x4000,
     "",
     ""
    },

    // MOD_PC85B
    {
     {"PC85B_BASIC.ROM", "PC85B_BASIC_A.ROM", "PC85B_BASIC_B.ROM", ""},
     basic,
     0,
     0x4000,
     "",
     ""
    },

    // MOD_PC85
    {
     {"PC85_BASIC.ROM", "PC85_BASIC_A.ROM", "PC85_BASIC_B.ROM", ""},
     basic,
     0,
     0x4000,
     "",
     ""
    },

    // MOD_PC
    {
     {"PC_BASIC.ROM", "PC_BASIC_A.ROM", "PC_BASIC_B.ROM", ""},
     basic,
     0,
     0x4000,
     "",
     ""
    },

    // MOD_IC
    {
     {"IC_BASIC.ROM", "IC_BASIC_A.ROM", "IC_BASIC_B.ROM", ""},
     basic,
     0,
     0x4000,
     "",
     ""
    },

    // MOD_2MHZ
    {
     {"2MHZ_BASIC.ROM", "2MHZ_BASIC_A.ROM", "2MHZ_BASIC_B.ROM",
      "2MHZ_BASIC_C.ROM", "2MHZ_BASIC_D.ROM", ""},
     basic,
     0,
     0x4000,
     "",
     ""
    },

    // MOD_2MHZDD
    {
     {"2MHZDD.ROM", ""},
     rom1,
     0,
     0x1000,
     "",
     ""
    },

    // MOD_DD
    {
     {"DD.ROM", ""},
     rom1,
     0,
     0x1000,
     "",
     ""
    },

    // MOD_SCF
    {
     {"SCF.ROM", "", "", ""},
     NULL,
     0,
     0x40000,
     "",
     ""
    },

    // MOD_PCF
    {
     {"PCF.ROM", "", "", ""},
     NULL,
     0,
     0x40000,
     "",
     ""
    }
   };

//==============================================================================
// Load a ROM image from file.
//
// The messages reported in this function are not considered as fatal so are
// not forced to the console.
//
//   pass: char *name           pointer to file name
//         uint8_t *dest        pointer to destination
//         int size             expected size of the ROM to be loaded
//         char *filepath       full path to file loaded
// return: int                  number of bytes loaded
//==============================================================================
int roms_loadrom (char *name, uint8_t *dest, int size, char *filepath)
{
 FILE *romfp = NULL;
 int len;

 char filename[SSIZE1];

 int res = 0;

 if (dest == NULL)
    return 0;

 // see if the name has an alias file name entry
 if (emu.alias_roms)
    res = find_file_alias(ALIASES_ROMS, name, filename);
 else
    strcpy(filename, name);

 if (res != -1)
    romfp = open_file(filename, userhome_romspath, filepath, "rb");

 if ((res == 0) && (romfp == NULL))
    return 0;

 if ((res == -1) || ((res == 1) && (romfp == NULL)))
    {
     // no point in re-scanning ROMs if no alias file is being used
     if (! emu.alias_roms)
        return 0;

     // if we are not using the 'roms.md5.auto' file then it's the user's
     // problem
     if (emu.roms_md5_file == ROMS_MD5_USER)
        return 0;

     // if we have not forced the creation of the MD5 file earlier then
     // force it now
     if (! emu.roms_create_md5)
        {
         emu.roms_create_md5 = 1; // force creating it
         roms_create_md5();

         // give it another go!
         if (find_file_alias(ALIASES_ROMS, name, filename) != -1)
            {
             romfp = open_file(filename, userhome_romspath, filepath, "rb");
             if (romfp == NULL)
                return 0;
            }
         else
            return 0;
        }
     else
        return 0;
    }

 fseek(romfp, 0, SEEK_END);             // get size of ROM
 len = ftell(romfp);
 fseek(romfp, 0, SEEK_SET);             // back to start

 if (len != 0)
    {
     if (len > size)
        {
         if (emu.verbose)
            xprintf("roms_loadrom: ROM size (0x%x) is too large (0x%x) "
                    "(continuing): %s\n", len, size, filepath);
         return 0;
        }
     else
        if (len < size)
           size = len;
     len = fread(dest, 1, len, romfp);
     fclose(romfp);
    }

 if (len == 0)
    {
     if (emu.verbose)
        xprintf("roms_loadrom: Failed to load ROM (continuing): %s\n",
                filepath);
    }

 return len;
}

//==============================================================================
// Load all Pak ROM images from file or if Pak uses SRAM then initialise.
//
// Each model uses a unique file name for each Pak ROM location allowing the
// user to set the targetted ROM file in the roms.alias file.
//
// The 2MHz model used 4K EPROMs on the core board but 4 or 8K EPROM images
// may be used.  If the first image is 4K in size the second image will be
// loaded as well.  4K EPROM images may be used for all models by using
// --pak(n)=a,filename and --pak(n)=b,filename but only the 2MHz model will
// attempt 4K images by default.
//
//   pass: int config                   0 uses --pakrom and --pakram
//                                      1 uses configured ROMs
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_paks (int config)
{
 char filepath[SSIZE1];
 char romimage_a[512];
 char romimage_b[512];

 int paksize;                   // size of current Pak ROM
 int pak;

 char *PAK_TTERM[] =
    {
     "TTERM_PAK0.ROM",  //
     "TTERM_PAK1.ROM",  //
     "TTERM_PAK2.ROM",  //
     "TTERM_PAK3.ROM",  //
     "TTERM_PAK4.ROM",  //
     "TTERM_PAK5.ROM",  //
     "TTERM_PAK6.ROM",  //
     "TTERM_PAK7.ROM",  //
    };

 char *PAK_PPC85[] =
    {
     "PPC85_PAK0.ROM",  // default is Wordbee (ROM C)
     "PPC85_PAK1.ROM",  // default is Basic help (ROM E)
     "PPC85_PAK2.ROM",  // default is Busycalc/PC85 Menu (ROM F)
     "PPC85_PAK3.ROM",  // default is Graphics/Database (ROM G)
     "PPC85_PAK4.ROM",  // default is Vidotex
     "PPC85_PAK5.ROM",  // default is Shell (ROM I)
     "PPC85_PAK6.ROM",  // User Pak 6
     "PPC85_PAK7.ROM"   // User Pak 7
    };

 char *PAK_PC85B[] =
    {
     "PC85B_PAK0.ROM",  // User Pak 0
     "PC85B_PAK1.ROM",  // User Pak 1
     "PC85B_PAK2.ROM",  // User Pak 2
     "PC85B_PAK3.ROM",  // User Pak 3
     "PC85B_PAK4.ROM",  // User Pak 4
     "PC85B_PAK5.ROM",  // User Pak 5
     "PC85B_PAK6.ROM",  // User Pak 6
     "PC85B_PAK7.ROM"   // User Pak 7
    };

 char *PAK_PC85[] =
    {
     "PC85_PAK0.ROM",   // User Pak 0
     "PC85_PAK1.ROM",   // User Pak 1
     "PC85_PAK2.ROM",   // User Pak 2
     "PC85_PAK3.ROM",   // User Pak 3
     "PC85_PAK4.ROM",   // User Pak 4
     "PC85_PAK5.ROM",   // User Pak 5
     "PC85_PAK6.ROM",   // User Pak 6
     "PC85_PAK7.ROM"    // User Pak 7
    };

 char *PAK_PC[] =
    {
     "PC_PAK0.ROM",     // default is EDASM
     "PC_PAK1.ROM",     // default is Wordbee
     "PC_PAK2.ROM",     // User Pak 2
     "PC_PAK3.ROM",     // User Pak 3
     "PC_PAK4.ROM",     // User Pak 4
     "PC_PAK5.ROM",     // User Pak 5
     "PC_PAK6.ROM",     // User Pak 6
     "PC_PAK7.ROM"      // User Pak 7
    };

 char *PAK_IC[] =
    {
     "IC_PAK0.ROM",     // default is Wordbee
     "IC_PAK1.ROM",     // User Pak 1
     "IC_PAK2.ROM",     // User Pak 2
     "IC_PAK3.ROM",     // User Pak 3
     "IC_PAK4.ROM",     // User Pak 4
     "IC_PAK5.ROM",     // User Pak 5
     "IC_PAK6.ROM",     // User Pak 6
     "IC_PAK7.ROM"      // User Pak 7
    };

 char *PAK_2MHZ[] =
    {
     "2MHZ_PAK0.ROM",   // User Pak 0 ROM A, 4 or 8k
     "2MHZ_PAK0_B.ROM", // User Pak 0 ROM B, 4k
     "2MHZ_PAK1.ROM",   // User Pak 1 ROM A, 4 or 8k
     "2MHZ_PAK1_B.ROM", // User Pak 1 ROM B, 4k
     "2MHZ_PAK2.ROM",   // User Pak 2 ROM A, 4 or 8k
     "2MHZ_PAK2_B.ROM", // User Pak 2 ROM B, 4k
     "2MHZ_PAK3.ROM",   // User Pak 2 ROM A, 4 or 8k
     "2MHZ_PAK3_B.ROM", // User Pak 2 ROM B, 4k
     "2MHZ_PAK4.ROM",   // User Pak 2 ROM A, 4 or 8k
     "2MHZ_PAK4_B.ROM", // User Pak 2 ROM B, 4k
     "2MHZ_PAK5.ROM",   // User Pak 2 ROM A, 4 or 8k
     "2MHZ_PAK5_B.ROM", // User Pak 2 ROM B, 4k
     "2MHZ_PAK6.ROM",   // User Pak 2 ROM A, 4 or 8k
     "2MHZ_PAK6_B.ROM", // User Pak 2 ROM B, 4k
     "2MHZ_PAK7.ROM",   // User Pak 2 ROM A, 4 or 8k
     "2MHZ_PAK7_B.ROM", // User Pak 2 ROM B, 4k
    };

 for (pak = 0; pak < 8; pak++)
    {
     romimage_a[0] = 0;
     romimage_b[0] = 0;
     if (config)
        {
         // use a configured Pak ROM image
         if ((! modelc.pakram[pak]) && (! modelc.pak_a[pak][0]))
            switch (emu.model)
               {
                case MOD_TTERM :
                   strcpy(romimage_a, PAK_TTERM[pak]);
                   break;
                case MOD_PPC85 :
                   strcpy(romimage_a, PAK_PPC85[pak]);
                   break;
                case MOD_PC85B :
                   strcpy(romimage_a, PAK_PC85B[pak]);
                   break;
                case MOD_PC85 :
                   strcpy(romimage_a, PAK_PC85[pak]);
                   break;
                case MOD_PC :
                   strcpy(romimage_a, PAK_PC[pak]);
                   break;
                case MOD_IC :
                   strcpy(romimage_a, PAK_IC[pak]);
                   break;
                case MOD_2MHZ :
                   strcpy(romimage_a, PAK_2MHZ[pak * 2 + 0]);
                   strcpy(romimage_b, PAK_2MHZ[pak * 2 + 1]);
                   break;
                case MOD_SCF :
                case MOD_PCF :
                default :
                   break;
               }
        }
     else
        {
         if (modelc.pakram[pak])
            // Pak location uses SRAM,  initialise 16K
            memmap_init6264(&paks[pak * 0x4000], 8);
         else
            {
             // override the default Pak ROM(s) if a --pak(n) option was
             // specified
             if (modelc.pak_a[pak][0])
                strcpy(romimage_a, modelc.pak_a[pak]);
             if (modelc.pak_b[pak][0])
                strcpy(romimage_b, modelc.pak_b[pak]);
            }
        }

     // load the Pak ROM image
     if (romimage_a[0])
        {
         paksize = roms_loadrom(romimage_a, &paks[pak * 0x4000],
                   0x4000, filepath);
         if ((paksize == 0x1000) && romimage_b[0])
            paksize = roms_loadrom(romimage_b, &paks[pak * 0x4000 + 0x1000],
                      0x1000, filepath);
        }
    }

 return 0;
}

//==============================================================================
// Switch in a new Pak location.  The Pak may consist of ROM or SRAM.
//
// Standard PC85 models may use 16K ROMs depending on the board revision.
//
//   pass: pak                  Pak number
// return: void
//==============================================================================
static void roms_switch_pak (int pak)
{
 modelc.paksel = pak;
 pakdata = pak;

 pakofs = (pakdata & 0x07) * 0x4000;
 if ((emu.model == MOD_PPC85) || (emu.model == MOD_PC85B) ||
     (emu.model == MOD_TTERM) || (emu.model == MOD_PCF))
    pakofs += ((pakdata >> 3) & 0x01) * 0x2000;

 // configure map
 memmap_configure();
}

//==============================================================================
// Load a Net ROM image from file.
//
// Each model uses a different file name so that the user can link to the
// actual ROM file concerned.  Options can be used to overide the default
// ROM value for each model.
//
// A check is made for known problem ROM(s) and patched,  the data used is the
// output of the cmp command in Linux.  Two ROMs are compared,  the address is
// decimal and the data values are octal (must have a leading 0 !).
//
// The telcom-3.21 ROM considered as good and used for the cmp against the
// other two ROM files has an MD5 of acd9633cc6154c0aebe03feada8e9a88.
//
// cmp -l badrom.rom goodrom.rom
//
//   pass: int config                   0 uses --netrom and --netram
//                                      1 uses a configured ROM
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_net (int config)
{
 char filepath[SSIZE1];
 char romimage[512];
 char md5[33];
 int i;

 romfix_t *romfix_data;

 // md5=6935334cc0d27c1cd67147879d7d8ee4 ROM (from mbeepc)
 romfix_t romfix_telc321_1[] =
 {
// OFS+1  DATA
  {38,    0332},
  {39,    0357},
  {432,   0351},
  {1408,  0345},
  {1504,  0347},
  {2080,  0355},
  {2132,  0341},
  {2297,  0376},
  {2334,  0367},
  {2444,  0315},
  {2665,  0256},
  {2678,  0346},
  {3002,  0377},
  {3084,  0344},
  {3260,  0335},
  {3269,  0267},
  {3290,  0356},
  {3296,  0157},
  {3324,  0335},
  {3559,  0137},
  {3849,  0355},
  {3916,  0370},
  {3969,  0104},
  {7000,  0165},
  {7009,  0004},
  {8154,  0170},
  {-1,    0000}
 };

 // md5=9da8868f95631809ea3b910818ed80bd ROM (semi fixed)
 romfix_t romfix_telc321_2[] =
 {
// OFS+1  DATA
  {3969,  0104},
  {7000,  0165},
  {7009,  0004},
  {8154,  0170},
  {-1,    0000}
 };

 romimage[0] = 0;
 if (config)
    {
     // use a configured Net ROM image
     if ((! modelc.netram) && (! modelc.netrom[0]))
        switch (emu.model)
           {
            case MOD_TTERM :
               strcpy(romimage, "TTERM_NETWORK.ROM");
               break;
            case MOD_PPC85 :
               strcpy(romimage, "PPC85_NETWORK.ROM");
               break;
            case MOD_PC85B :
               strcpy(romimage, "PC85B_NETWORK.ROM");
               break;
            case MOD_PC85 :
               strcpy(romimage, "PC85_NETWORK.ROM");
               break;
            case MOD_PC :
               strcpy(romimage, "PC_NETWORK.ROM");
               break;
            case MOD_IC :
               strcpy(romimage, "IC_NETWORK.ROM");
               break;
            case MOD_2MHZ : strcpy(romimage, "2MHZ_NETWORK.ROM");
               break;
            case MOD_SCF :
            case MOD_PCF :
            default :
               break;
           }
    }
 else
    {
     if (modelc.netram)
         // Net location uses SRAM,  initialise 16K
         memmap_init6264(netx, 8);
     else
        if (modelc.netrom[0])  // if use another Net ROM
           strcpy(romimage, modelc.netrom);
    }

 if (romimage[0])
    {
     roms_loadrom(romimage, netx, 0x4000, filepath);

     create_md5(filepath, md5);

     // Check if this is a known bad Telcom v3.2.1 ROM image and patch it.
     // the original bad ROM (from mbeepc) or another semi fixed ROM
     if ((strcmp(md5, "6935334cc0d27c1cd67147879d7d8ee4") == 0) ||
        (strcmp(md5, "9da8868f95631809ea3b910818ed80bd") == 0)) 
        {
         xprintf("roms_loadnet: Bad version of Telcom v3.21 detected "
                 "and will be patched.\n");
         if (strcmp(md5, "6935334cc0d27c1cd67147879d7d8ee4") == 0)
            romfix_data = romfix_telc321_1;
         else
            romfix_data = romfix_telc321_2;
         i = 0;
         while (romfix_data[i].ofs != -1)
            {
             netx[romfix_data[i].ofs - 1] = romfix_data[i].data;
             i++;
            }
        }
    }

 return 0;
}

//==============================================================================
// ROMs initialise.
//
// Fill all empty ROM locations with 0xFFs.
//
// Loads the Character/Boot/Basic/Pak/Net ROMs.
//
//   pass: void
// return: int                          0
//==============================================================================
int roms_init (void)
{
 memset(vdu.chr_rom, 0xFF, sizeof(vdu.chr_rom));
 memset(rom1, 0xFF, sizeof(rom1));
 memset(rom2, 0xFF, sizeof(rom2));
 memset(rom3, 0xFF, sizeof(rom3));
 memset(basic, 0xFF, sizeof(basic));
 memset(basic_alphap, 0xFF, sizeof(basic_alphap));
 memset(paks, 0xFF, sizeof(paks));
 memset(netx, 0xFF, sizeof(netx));

 return roms_load_all();
}

//==============================================================================
// ROMs de-initialise.
//
//   pass: void
// return: int                          0
//==============================================================================
int roms_deinit (void)
{
 return 0;
}

//==============================================================================
// ROMs reset.
//
// A reset causes bank 0 of network ROM to be selected.  This should work for
// all ROM based models.
//
//   pass: void
// return: int                          0
//==============================================================================
int roms_reset (void)
{
 netbank = 0;
 netofs = 0;

 return 0;
}

//==============================================================================
// Net read - Port function.
//
// Load the Net SRAM/ROM bank ROM.
//
// Later models used bank switching of 4K ROM sections at E000.  The PC used
// bit 8 on an in instruction to select one of two banks.  Later ROM models
// used bits 8 and 9 allowing any one of 4 bank sections to be selected.
//
// in a,(0ah) places reg A onto A8-A15,  in r,(c) places reg B onto A8-A15
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t                     0xFFFF
//==============================================================================
uint16_t roms_nsel_r (uint16_t port, struct z80_port_read *port_s)
{
 int bank_mask;

 switch (emu.model)
    {
     case MOD_TTERM :
     case MOD_PPC85 :
     case MOD_PC85B :
     case MOD_PC85 :
     case MOD_SCF :
     case MOD_PCF :
        bank_mask = B8(00000011);
        break;
     case MOD_PC :
        bank_mask = B8(00000001);
        break;
     case MOD_IC :
     case MOD_2MHZ :
        bank_mask = B8(00000000);
        break;
     default : bank_mask = 0;
        break;
    }

 netbank = (port >> 8) & bank_mask;

 if (modio.roms)
    log_port_16("roms_nsel_r", "bank", port, netbank);

 netofs = 0x1000 * netbank;

 return 0;
}

//==============================================================================
// Pak write - Port function.
//
// Switch in a Pak (x) ROM/SRAM.
//
//   pass: uint16_t port
//         uint8_t data                 Pak# (0-255)
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void roms_psel_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.roms)
    log_port_1("roms_psel_w", "data", port, data);

 roms_switch_pak(data);
}

//==============================================================================
// Check the MD5 of a ROM and make corrections.
//
//   pass: char *filepath
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_md5_check (char *filepath)
{
 char md5[33];

 // get the MD5 for last loaded ROM
 create_md5(filepath, md5);

 // check for known bad ROMs
 if (strcmp(md5, "4c005ae71366eddd727b887953763c53") == 0)
    {
     xprintf("roms_load: This %s file is a damaged 256TC v1.20 ROM image.\n",
             filepath);
     return -1;
    }

 // modify some known rom1 century dates in memory
 if (emu.century)
    {
     // if ver 1.15 of 256TC boot ROM
     if (strcmp(md5, "13ddba203bd0b8228f748111421bad5f") == 0)
        {
         rom1[0x1c9d] = emu.century;              // replace with BCD century
         rom1[0x3fff] += (0x19 - emu.century);    // modify checksum value
        }
     else
     
     // if ver 1.20 of 256TC boot ROM
     if (strcmp(md5, "24d6682ff7603655b0cbf77be6731fb0") == 0)
        {
         rom1[0x1e56] = emu.century;           // replace with BCD century
         rom1[0x3fff] += (0x19 - emu.century); // modify checksum value
        }
     else

     // if ver 1.31 of 256TC boot ROM
     if (strcmp(md5, "4170a8bb9495aa189afb986c1d0424a4") == 0)
        {
         rom1[0x1bc8] = emu.century;           // replace with BCD century
         rom1[0x3fff] += (0x19 - emu.century); // modify checksum value
        }
    }

 return 0;
}

//==============================================================================
// Load the character ROM.
//
// The character ROM may be a 2K (2716) for 2MHz models or 4K (2732) type
// for all others, if the 2 MHZ model is emulated or a 2K ROM is used then
// the first 2K is copied to the second 2K space so that if the 6545
// accesses data in the upper 2K region it uses the same 16x8 font data as
// that contained in the lower 2K.
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_character (void)
{
 char filepath[SSIZE1];
 char romimage[512];

 int size;

 if (modelc.charrom[0])         // if use another character ROM
    strcpy(romimage, modelc.charrom);
 else
    strcpy(romimage, "charrom.bin");

 size = roms_loadrom(romimage, vdu.chr_rom, 0x1000, filepath);

 if ((size != 0x0800) && (size != 0x1000))
    {
     xprintf("roms_load: unable to load character ROM %s\n", filepath);
     return -1;
    }

 return 0;
}

//==============================================================================
// Load the standard colour model PROM.
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_colour_prom (void)
{
 char filepath[SSIZE1];
 char romimage[512];

 int size;

 if (modelc.colprom[0])         // if use other colour 82s123 IC 7 PROM data
    {
     strcpy(romimage, modelc.colprom);
     size = roms_loadrom(romimage, ic_82s23, 0x20, filepath);
     if (size != 0x20)
        {
         xprintf("roms_load: unable to load colour PROM %s\n", filepath);
         return -1;
        }
    }

 return 0;
}

//==============================================================================
// Load the ROMs for the DRAM models as specified with options --rom1, --rom2
// or --rom3.
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_romx (void)
{
 char filepath[SSIZE1];
 char romimage1[512];
 char romimage2[512];
 char romimage3[512];

 if (modelx.rom)
    return 0;

 // load the rom1 image as specified by option --rom1
 if (modelc.rom1[0])
    {
     strcpy(romimage1, modelc.rom1);
     if (roms_loadrom(romimage1, rom1, ROM1_SIZE, filepath))
        {
         if (emu.verbose)
            xprintf("roms_load: ROM 1 image loaded: %s\n", filepath);
         if (roms_md5_check(filepath) == -1)
            return -1;
        }
     else
        {
         xprintf("roms_load: ROM 1 image failed to be loaded: %s\n", filepath);
         return -1;
        }
    }

 if ((modelx.ram >= 64) && (emu.model != MOD_256TC))
    {
     if (modelc.rom2[0])        // if --rom2 was specified
        strcpy(romimage2, modelc.rom2);
     else
        strcpy(romimage2, boot_data[emu.model].romimage2);

     if (modelc.rom3[0])        // if --rom3 was specified
        strcpy(romimage3, modelc.rom3);
     else
        strcpy(romimage3, boot_data[emu.model].romimage3);

     if (romimage2[0])
        if (roms_loadrom(romimage2, rom2, ROM2_SIZE, filepath))
           {
            if (emu.verbose)
               xprintf("roms_load: optional ROM 2 image loaded: %s\n",
                       filepath);
           }

     if (romimage3[0])
        if (roms_loadrom(romimage3, rom3, ROM3_SIZE, filepath))
           {
            if (emu.verbose)
               xprintf("roms_load: optional ROM 3 image loaded: %s\n",
                       filepath);
           }
    }

 return 0;
}

//==============================================================================
// Load the basic/a/b/c/d ROM images as specified by options --basic/a/b/c/d
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_basic (void)
{
 int size;

 char filepath[SSIZE1];
 char romimage[512];

 uint8_t *dest;

 int loaded = 0;

 if (((modelx.rom) || (emu.model == MOD_SCF) || (emu.model == MOD_PCF)) &&
    (modelc.basica[0]))
    {
     if (modelx.alphap)
        dest = basic_alphap;
     else
        dest = basic;

     size = 0x4000;                     // 1  x 16K ROM images
     if ((modelc.basicb[0]) && (emu.model != MOD_PPC85) &&
        (emu.model != MOD_TTERM) && (emu.model != MOD_PCF))
        {
         size = 0x2000;                 // 2  x 8K ROM images
         if (modelc.basicc[0] && modelc.basicd[0])
            size = 0x1000;              // 4  x 4K ROM images
        }

     // load the first 4/8/16 KB ROM image
     strcpy(romimage, modelc.basica);
     if ((modelc.basram) && (size == 0x4000))
        loaded += roms_loadrom(romimage, dest + loaded, 0x2000, filepath);
     else
        loaded += roms_loadrom(romimage, dest + loaded, size, filepath);

     // load the second 4/8 KB ROM image
     if (size < 0x4000)
        {
         strcpy(romimage, modelc.basicb);
         if ((! modelc.basram) || (size == 0x1000))
            loaded += roms_loadrom(romimage, dest + loaded, size, filepath);
        }

     // load the third and forth 4 KB ROM images
     if ((size < 0x2000) && (! modelc.basram))
        {
         strcpy(romimage, modelc.basicc);
         loaded += roms_loadrom(romimage, dest + loaded, size, filepath);

         strcpy(romimage, modelc.basicd);
         loaded += roms_loadrom(romimage, dest + loaded, size, filepath);
        }

     if (! loaded)
        {
         xprintf("roms_load: Unable to load %s\n", filepath);
         return -1;
        }
     else
        if (! modelx.rom)
           {
            if (emu.verbose)
               xprintf("roms_load: boot ROM image loaded: %s\n", filepath);
           }
    }

 return 0;
}

//==============================================================================
// Load ROM overrides.
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_rom_overrides (void)
{
 if (roms_load_romx() == -1)
    return -1;
 if (roms_load_basic() == -1)
    return -1;
 if (roms_load_paks(0) == -1)
    return -1;
 if (roms_load_net(0) == -1)
    return -1;

 return 0;
}

//==============================================================================
// Load the 256K ROM.
//
// Load all the individual ROMs contained in the 256K ROM image. Other ROMs
// can be used by specifying ROM override options.  The 256K ROM may be
// ignored by specifying --rom256k=none.
//
// See CF project 'CF_Coreboard_MMU_notes.txt' for more information.
//
//  ROM Address  Contents  Destination  When
//  00000-03fff  Boot A    8000-bfff    mode=0
//  04000-07fff  Boot B    c000-ffff    mode=0 and rom_sel=0
//  08000-09fff  Boot C    e000-ffff    mode=0 and rom_sel=1
//  0a000-0bfff  Basic A   8000-9fff    mode=1 and prem_rom=0
//  0c000-0dfff  Basic B   a000-bfff    mode=1
//  0e000-0ffff  Basic C   8000-9fff    mode=1 and prem_rom=1
//  10000-11fff  NET 0     e000-ffff    mode=1 and net=0
//  12000-13fff  NET 1     e000-ffff    mode=1 and net=1
//  14000-15fff  NET 2     e000-ffff    mode=1 and net=2
//  16000-17fff  NET 3     e000-ffff    mode=1 and net=3
//  18000-19fff  PAK 0     c000-dfff    mode=1 and pak=0
//  1a000-1bfff  PAK 1     c000-dfff    mode=1 and pak=1
//  1c000-1dfff  PAK 2     c000-dfff    mode=1 and pak=2
//  1e000-1ffff  PAK 3     c000-dfff    mode=1 and pak=3
//  20000-21fff  PAK 4     c000-dfff    mode=1 and pak=4
//  22000-23fff  PAK 5     c000-dfff    mode=1 and pak=5
//  24000-25fff  PAK 6     c000-dfff    mode=1 and pak=6
//  26000-27fff  PAK 7     c000-dfff    mode=1 and pak=7
//  28000-29fff  PAK 8     c000-dfff    mode=1 and pak=8
//  2a000-2bfff  PAK 9     c000-dfff    mode=1 and pak=9
//  2c000-2dfff  PAK A     c000-dfff    mode=1 and pak=a
//  2e000-2ffff  PAK B     c000-dfff    mode=1 and pak=b
//  30000-31fff  PAK C     c000-dfff    mode=1 and pak=c
//  32000-33fff  PAK D     c000-dfff    mode=1 and pak=d
//  34000-35fff  PAK E     c000-dfff    mode=1 and pak=e
//  36000-37fff  PAK F     c000-dfff    mode=1 and pak=f
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_256k (void)
{
 char filepath[SSIZE1];
 char romimage[512];
 uint8_t buffer[0x40000];  // 256K buffer for 256K ROM

 int loaded = 0;

 // load the 256K image if specified by option --rom256k
 if (modelc.rom256k[0])
    strcpy(romimage, modelc.rom256k);
 else
    strcpy(romimage, boot_data[emu.model].romimage[0]);

 if (strcmp("none", romimage) != 0)
    {
     loaded += roms_loadrom(romimage, buffer, sizeof(buffer), filepath);

     if (loaded != sizeof(buffer))
        {
         if (loaded == 0)
            xprintf("roms_load: Unable to load 256K ROM, "
                    "ROM is empty or does not exist.\n");
         else
            xprintf("roms_load: Unable to continue ROM load operation, "
                    "ROM is incorrect size.\n");
         return -1;
        }

     memcpy(rom1, buffer+0x00000, 0x4000); // ROM1 ROM (boot)
     memcpy(rom2, buffer+0x04000, 0x4000); // ROM2 ROM
     memcpy(rom3, buffer+0x08000, 0x2000); // ROM3 ROM
     if (modelx.alphap)
        {
         memcpy(basic_alphap, buffer+0x0A000, 0x2000); // Basic A (lower 8K) ROM
         memcpy(basic+0x2000, buffer+0x0C000, 0x2000); // Basic B ROM (loaded
                                                       // into the upper 8K)
         memcpy(basic_alphap+0x2000, buffer+0x0E000, 0x2000); // Basic A
                                                              // (upper 8K) ROM
        }
     else
        {
         memcpy(basic, buffer+0x0A000, 0x2000); // Basic A ROM
         memcpy(basic+0x2000, buffer+0x0C000, 0x2000); // Basic B ROM
        }
     memcpy(netx+0x0000, buffer+0x10000, 0x1000); // Net 0 (only use first 4K)
     memcpy(netx+0x1000, buffer+0x12000, 0x1000); // Net 1 (only use first 4K)
     memcpy(netx+0x2000, buffer+0x14000, 0x1000); // Net 2 (only use first 4K)
     memcpy(netx+0x3000, buffer+0x16000, 0x1000); // Net 3 (only use first 4K)

     // these are the lower 8K banks
     memcpy(paks+0x00000, buffer+0x18000, 0x2000); // Pak 0
     memcpy(paks+0x04000, buffer+0x1a000, 0x2000); // Pak 1
     memcpy(paks+0x08000, buffer+0x1c000, 0x2000); // Pak 2
     memcpy(paks+0x0C000, buffer+0x1e000, 0x2000); // Pak 3
     memcpy(paks+0x10000, buffer+0x20000, 0x2000); // Pak 4
     memcpy(paks+0x14000, buffer+0x22000, 0x2000); // Pak 5
     memcpy(paks+0x18000, buffer+0x24000, 0x2000); // Pak 6
     memcpy(paks+0x1C000, buffer+0x26000, 0x2000); // Pak 7

     // these are the upper 8K banks
     memcpy(paks+0x02000, buffer+0x28000, 0x2000); // Pak 8
     memcpy(paks+0x06000, buffer+0x2a000, 0x2000); // Pak 9
     memcpy(paks+0x0A000, buffer+0x2c000, 0x2000); // Pak 10
     memcpy(paks+0x0E000, buffer+0x2e000, 0x2000); // Pak 11
     memcpy(paks+0x12000, buffer+0x30000, 0x2000); // Pak 12
     memcpy(paks+0x16000, buffer+0x32000, 0x2000); // Pak 13
     memcpy(paks+0x1A000, buffer+0x34000, 0x2000); // Pak 14
     memcpy(paks+0x1E000, buffer+0x36000, 0x2000); // Pak 15
    }

 return roms_load_rom_overrides();
}

//==============================================================================
// Load the configured Boot or Basic ROMs.
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_config_boot_basic (void)
{
 int i;
 int loaded;
 int size;

 char filepath[SSIZE1];
 char romimage[512];

 if (! modelx.rom && modelc.rom1[0])
    return 0;

 if (modelx.rom && modelc.basica[0])
    return 0;

 i = 0;
 loaded = 0;
 size = boot_data[emu.model].size;

 while (size > 0)
    {
     if (strcmp(boot_data[emu.model].romimage[i], "") == 0)
        {
         if (! loaded)
            {
             xprintf("roms_load: Unable to continue ROM load operation, "
                     "no ROM image file(s).\n");
             return -1;
            }
         else
            size = 0;
        }
     else
        {
         strcpy(romimage, boot_data[emu.model].romimage[i]);

         if ((modelx.rom) && (modelc.basram) &&
            (boot_data[emu.model].dest == basic) && (size >= 0x2000))
            size = 0x2000;

         loaded += roms_loadrom(romimage, boot_data[emu.model].dest +
                                loaded, size, filepath);
         size = boot_data[emu.model].size - loaded;
         i++;

         if ((modelx.rom) && (modelc.basram) &&
            (boot_data[emu.model].dest == basic) && (loaded >= 0x2000))
            size = 0;

         // for FDC models only attempt loading the required amount from one
         // ROM, and not from 2 or more ROMs.
         if ((! modelx.rom) && (loaded != 0))
            size = 0;
#if 0
         xprintf("roms_load: file=%s size=%d loaded=%d\n",
                 filepath, size, loaded);
#endif
        }
    }

 if (! loaded)
    {
     xprintf("roms_load: Unable to load %s\n", filepath);
     return -1;
    }
 else
    if (! modelx.rom)
       {
        if (emu.verbose)
           xprintf("roms_load: boot ROM image loaded: %s\n", filepath);
       }

 return roms_md5_check(filepath);
}

//==============================================================================
// Load Premium PC85 model ROM B.
//
// Special handling for the Premium PC85 model's Basic ROM B.
//
// The ROM B part is not loaded if the --basram option is in affect.
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_config_ppc85 (void)
{
 char filepath[SSIZE1];

 char romimage[512];

 if ((emu.model == MOD_PPC85) && (! modelc.basram))
    {
     if (modelc.basicb[0])
        strcpy(romimage, modelc.basicb);
     else
        {
         if (emu.model == MOD_PPC85)
            strcpy(romimage, "PPC85_B.ROM");
        }

     if (roms_loadrom(romimage, basic + 0x2000, 0x2000, filepath) != 0x2000)
        {
         xprintf("roms_load: Unable to load %s\n", filepath);
         return -1;
        }
    }

 return 0;
}

//==============================================================================
// Load the configured Pak ROMs.
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_config_paks (void)
{
 if (modelx.rom)
    {
     // load all the Pak locations with ROM images or initialised SRAM
     if (roms_load_paks(1) == -1)
        return -1;

     // configure map
     memmap_configure();
    }

 return 0;
}

//==============================================================================
// Load the configured Net ROM.
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_config_net (void)
{
 if (modelx.rom)
    {
     // load Net ROM
     if (roms_load_net(1) == -1)
        return -1;

     // configure map
     memmap_configure();
    }

 return 0;
}

//==============================================================================
// Load Character/Boot/Basic/Pak/Net ROM images.
//
// The character and colour PROM are loaded first.  When emulating the CF
// model the roms_load_256k() function handles loading the 256K ROM image
// then applies any overrides.  All other models handle overrides first
// followed by loading of the ROMs.
//
//   pass: void
// return: int                          0 if no errors, -1 if error
//==============================================================================
static int roms_load_all (void)
{
 // Load the character ROM
 if (roms_load_character() == -1)
    return -1;

 // Load the standard colour model PROM
 if (roms_load_colour_prom() == -1)
    return -1;

 // if emulating a model that needs the 256K ROM
 if ((emu.model == MOD_SCF) || (emu.model == MOD_PCF))
    return roms_load_256k();

 // Load any ROMs specified by options (overrides)
 if (roms_load_rom_overrides() == -1)
    return -1;

 // Load the configured boot/basic ROMs
 if (roms_load_config_boot_basic() == -1)
    return -1;

 // Load Premium PC85 and Teleterm model ROM B
 if (roms_load_config_ppc85() == -1)
    return -1;

 // Load the configured Pak ROMs
 if (roms_load_config_paks() == -1)
    return -1;

 // Load the configured Net ROM
 if (roms_load_config_net() == -1)
    return -1;

 return 0;
}

//==============================================================================
// Create a ROMs MD5 file.
//
// This function is called during start-up.
//
// A test is made for the presence of 'roms.md5.user' and if it exists will
// be used as the ROMs MD5 source.
//
// A 'roms.md5.auto' file will be created if there is no file by that name
// or --md5-create is set to on.
//
//   pass: void
// return: void
//==============================================================================
void roms_create_md5 (void)
{
 sup_file_t f;

 char md5[33];
 char dpn[1024];
 char fnwc[1024];
 char mfp[1024];
 char fpfnm[1024];

 char userfile[SSIZE1];
 char destfile[SSIZE1];

 FILE *textfp;

 int count = 0;

 f.dpn = dpn;
 f.fnwc = fnwc;
 f.mfp = mfp;
 f.fpfnm = fpfnm;

 // test for presence of 'roms.md5.user' and use it if present
 snprintf(destfile, sizeof(destfile), "%s"SLASHCHAR_STR"roms.md5.user",
          userhome);
 textfp = fopen(destfile, "r");
 if (textfp)
    {
     fclose(textfp);
     emu.roms_md5_file = ROMS_MD5_USER;
     return;
    }

 // test for presence of 'roms.md5.auto' and generate if not found.
 emu.roms_md5_file = ROMS_MD5_AUTO;
 snprintf(destfile, sizeof(destfile), "%s"SLASHCHAR_STR"roms.md5.auto",
          userhome);
 textfp = fopen(destfile, "r");
 if (textfp)
    fclose(textfp);
 else
    emu.roms_create_md5 = 1; // force creating it

 // exit if we are not creating a new 'roms.md5.auto' file
 if (! emu.roms_create_md5)
    return;

 // open a directory and match all files in it (*)
 sprintf(f.dpn, "%s*", userhome_romspath);

 sup_opendir(&f);
 if (f.res)
    return;

 // scan the directory and create an MD5 entry for each match
 for (;;)
    {
     sup_readdir(&f); // return the next directory entry
     if (f.res)
        {
         if (f.res != 4)  // don't want directory entries
            {
             if (! count)
                {
                 // create the 'roms.md5.auto' file here
                 if (emu.verbose)
                    xprintf("Generating MD5s for ROMs located in %s\n",
                            userhome_romspath);

                 snprintf(userfile, sizeof(userfile),
                          "%s"SLASHCHAR_STR"roms.md5.auto", userhome);
                 textfp = fopen(userfile, "w");

                 if (textfp)
                    {
                     fprintf(textfp,
"#===============================================================================\n"
"# This file was auto generated with "ICONSTRING"-"APPVER"\n"
"#\n"
"# Any manual changes to this file will be lost.  Create a 'roms.md5.user' file\n"
"# if a customised ROMs MD5 is required. uBee512 will use that file if present.\n"
"#\n"
"# See the 'roms.alias.sample' file for customised MD5 generation information.\n"
"#===============================================================================\n"
"\n");
                    }
                 else
                    {
                     xprintf("roms_create_md5: error, can't create %s",
                             userfile);
                     return;
                    }
                }

             create_md5(f.fpfnm, md5);
             // write an MD5 entry for this ROM
             fprintf(textfp, "%s  %s\n", md5, f.dpn);
             count++;
            }
        }
     else
        {
         closedir(f.fp.d);   // close directory
         if (count)
            fclose(textfp);  // close the 'roms.md5.auto' file
         break;
        }
    }
}

//==============================================================================
// Process --pak(n) options.
//
// --pak(n) filename            8-16K ROM image into 0xC000 (banking if 16K)
// --pak(n) a,filename          4K maximum ROM image into 0xC000.
// --pak(n) b,filename          4K maximum ROM image into 0xD000.
//
//   pass: int pak              Pak ROM number (0-7)
//         char *p              parameters
// return: int                  0 if no error else -1
//==============================================================================
int roms_proc_pak_argument (int pak, char *p)
{
 char *pak_args[] =
 {
  "a",
  "b",
  ""
 };

 char s1[SSIZE1];
 char s2[SSIZE1];
 char *c;
 char *s;
 int x;

 c = get_next_parameter(p, ',', s1, &x, sizeof(s1)-1);
 c = get_next_parameter(c, ',', s2, &x, sizeof(s2)-1);

 if (s1[0] && s2[0])
    {
     x = string_search(pak_args, s1);
     if (x == -1)
        return -1;

     if (x == 0)
        c = modelc.pak_a[pak];
     else
        c = modelc.pak_b[pak];
     s = s2;
    }
 else
    {
     c = modelc.pak_a[pak];
     s = s1;
    }

 strncpy(c, s, SSIZE1);
 *(c + SSIZE1-1) = 0;

 return 0;
}
