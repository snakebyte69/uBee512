//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                                 VDU module                                 *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulates the graphics hardware for Standard and Premium models (Alpha+)
//
// - Character ROM
// - Screen RAM
// - PCG RAM
// - Colour RAM
// - Attribute RAM
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
// - Refactored this module to only redraw those parts of the screen that
//   have been changed.
// - Moved all code relating to pixels, pixel formats, pixel handling and
//   colours to the vdu module.
// - The character ROM data is also cached in SDL surfaces; this allows
//   updates to be drawn much more quickly.
//
// v5.7.0 - 1 December 2013, uBee
// - Changed function name vdu_latchrom() to vdu_latchrom_w().
//
// v5.5.0 - 28 June 2013, B.Robinson, uBee
// - Changes to vdu_vidmem_r() to prevent reading from the dummy bank,
//   instead just return 0 (BR).  Some MWB code to confirm on real hardware
//   that 0 is returned (SK).
//
// v4.6.0 - 4 May 2010, uBee
// - Changes to vdu_init() function to clear the colour RAM with 0s to
//   prevent the checker board pattern on start-up. This is only a temporary
//   work around and the real solution may need some type of 6545
//   initialisation delay to make it work like a real model.
// - Renamed log_port() calls to log_port_1() and log_data() to log_port_0().
//
// v4.1.0 - 22 June 2009, uBee
// - Code speed up for video read/write functions.
// - Made improvements to the logging process.
// - Loading of character ROM and colour PROM moved to new roms.c module.
//
// v4.0.0 - 7 June 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Completely overhauled memory management method, there is no longer any
//   banked memory swapping.
// - Added crtc_set_redraw() call to vdu_lvdat_w() function to update state
//   of display (i.e Telcom v3 problem fix introduced in v3.1.0) as the
//   vdu_switchvideo_in() function is now removed.
// - Set some vdu.scr_ram[], vdu.pcg_ram[] locations to 0xff as some
//   Microbee applications expect RAM to come up with certain values in some
//   locations.  (Above 2 fixes submitted by workerbee)
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
//
// v3.1.0 - 22 April 2009, uBee
// - Removed all occurrences of console_output() function calls.
// - Fixed a display refresh problem introduced in v3.0.0/1 when running in
//   standard ROM model monochrome mode and Telcom versions 1.1 or 1.2.  The
//   top status line was incorrect, see vdu_colcont_w() function notes.
// - Fixed a display refresh problem introduced in v2.8.0. This problem was
//   noticable in Telcom v3 on PJB system disk and some other applications. A
//   crtc_set_redraw() call was added to vdu_switchvideo_in() function.
// - Changed all printf() calls to use a local xprintf() function.
//
// v3.0.0 - 21 September 2008, uBee
// - Recoded the vdu_colcont_w() function to test RGB intensity bits and VDU
//   PCG/colour RAM selection bits separately, this prevents a change in
//   either leading to unnecessary time consuming updates.
//
// v2.8.0 - 15 August 2008, uBee
// - Standard BG colour intensity was not updating after a write to the colour
//   port in vdu_colcont_w() function.  Added crtc_set_redraw() call.
// - Added a crtc.flashbits counter that will determine when any flashing
//   video attribute bits are set, this will reduce host CPU time when
//   emulating alpha+ models.
// - vdu_vidmem_w() function now tests current video state before making calls
//   to crtc_set_redraw() and crtc_redraw_char() functions to reduce the host
//   CPU time.
// - Cast uint8_t ic_82s23[] as char* in memmap_loadrom function call to
//   eliminate a sign warning.
//
// v2.7.0 - 4 June 2008, uBee
// - New open_file() function in function.c module is now used by the
//   memmap_loadrom() function to determine the path to the ROMs.  Added full
//   file path return variable.
// - Added structures emu_t, crtc_t, memmap_t and moved variables into it.
//
// v2.5.0 - 29 March 2008, uBee
// - Added code for --colprom option to override early built in colour
//   82s123 IC 7 PROM data.
// - Character ROM may now be a 2K (2716) or 4K (2732) image type.
// - The character ROM can now be specified as a user option.
// - Implement the modelc structure.
//
// v2.4.0 - 17 February 2008, uBee
// - VDU_MASK0 and VDU_MASK3 removed and used modelx.vdu instead.  The VDU
//   RAM (colour RAM) mask of 3 did not work as expected when software has
//   port 0x1c set to a non zero PCG bank and VDU selected with the
//   intention of accessing the standard VDU memory. It would appear
//   Microbee removed IC14 (Premium) and linked pins 2 and 23 of the 6264 to
//   0V effectively making it into a 2K RAM part so that any software that
//   did not do an out 0x1c,0x80 before attempting to access the standard 2K
//   VDU would still work correctly.
// - Last version incorrectly claims all alpha+ models shipped with 8K for
//   each VDU RAM.  Early models had 2K later increased to 8K.
//
// v2.3.0 - 23 January 2008, uBee
// - VDU and PCG mask variables and vdu_configure() function removed.  The
//   number of PCG banks is now handled correctly.  The VDU mask is now a
//   constant as all alpha+ models had 8K screen, attribute and colour RAM.
//   The VDU_MASK0 define is set to 0 for screen and attribute RAM, a value
//   of 3 is not working, 256TC boot ROM has incorrect graphics and locking
//   is evident when it's 3. VDU_MASK3 define is used for extended colour
//   RAM and appears to work correctly.
// - ROM Basic 5.22e colour failure mystery solved and fixed by emulating
//   static memory pattern.  Removed VDU init code from memory vdu_reset()
//   function.
// - Many changes made to the colour and alpha+ graphics read/write and the
//   video switching in/out functions.
// - Made changes to the vdu_switchvideo_in() function to suit all model types.
// - Changes to the vdu_lvdat_r() and vdu_lvdat_w() functions due to changes
//   made in z80.c
// - Added modio_t structure.
//
// v2.2.0 - 16 November 2007, uBee
// - Make the colour_cont port variable public as is needed for old colour
//   circuit emulation.
// - Port 8 read now returns 0.
//
// v2.1.0 - 20 October 2007, uBee
// - Implement the modelx information structure.
//
// v1.4.0 - 30 September 2007, uBee
// - Added vdu_configure() function to set PCG and VDU values for Premium
//   models.
// - Added Premium PC85 ROM model basic upper/lower half select to vdu_lvdat_w
//   function.
// - Removed the non aplpha+ source build option.
// - Change all error output to stdout.
// - Character ROM loading now uses the memmap_loadrom() function.
// - Added changes to error reporting.
//
// v1.0.0 - 21 June 2007, uBee
// - Added standard and standard+colour machine options to vdu_colcont_w and
//   vdu_lvdat_w functions.
// - Added vdu_lvdat_r and vdu_lvdat_w port handlers.
// - Implemented Alpha+ (Premium) graphics.
//
// v0.0.0 - 8 June 2007, uBee
// Start with "nanowasp" source distribution version 0.22. An emulator for the
// microbee 128k. Copyright (C) 2000-2003  David G. Churchill.
//==============================================================================

#include <stdio.h>
#include <string.h>
#include <SDL.h>

#include "vdu.h"
#include "crtc.h"
#include "video.h"
#include "z80.h"
#include "ubee512.h"
#include "memmap.h"
#include "roms.h"
#include "support.h"

#include "macros.h"

//==============================================================================
// random private constants
//==============================================================================
#define CHAR_SURFACE_ROM_BANKS               2
#define CHAR_SURFACE_PCG_BANKS               PCG_RAM_BANKS
#define CHAR_SURFACE_BANK_SIZE               128
#define CHAR_SURFACE_WIDTH_CHARS             64
#define CHAR_SURFACE_NUM_BANKS               (CHAR_SURFACE_ROM_BANKS + CHAR_SURFACE_PCG_BANKS)
#define CHAR_SURFACE_HEIGHT_CHARS            (CHAR_SURFACE_NUM_BANKS * CHAR_SURFACE_BANK_SIZE / CHAR_SURFACE_WIDTH_CHARS)
#define CHAR_SURFACE_WIDTH_PIXELS            (CHAR_SURFACE_WIDTH_CHARS * 8)
#define CHAR_SURFACE_HEIGHT_PIXELS           (CHAR_SURFACE_HEIGHT_CHARS * 16)
#define CHAR_SURFACE_ROM_BANK(x)             (x)
#define CHAR_SURFACE_PCG_BANK(x)             ((x) + CHAR_SURFACE_ROM_BANKS)


//==============================================================================
// structures and variables
//==============================================================================
vdu_t vdu;

extern emu_t emu;
extern model_t modelx;
extern model_custom_t modelc;
extern modio_t modio;
extern crtc_t crtc;
extern video_t video;

extern int basofs;              /* offset into alpha+ BASIC ROM */


SDL_Surface *char_data;

//==============================================================================
//
// Available colours for each colour model
//
//==============================================================================
 char *colour_args[] =
 {
  "black",
  "blue",
  "green",
  "cyan",
  "red",
  "magenta",
  "yellow",
  "lgrey",
  "dgrey",
  "lblue",
  "lgreen",
  "lcyan",
  "lred",
  "lmagenta",
  "lyellow",
  "white",
  ""
 };

SDL_Color col_table[64];

//==============================================================================
// User configurable monochrome monitor values
//==============================================================================
// indices into the colour table for each of the 4 "colours" in monochrome
// mode (background, high intensity background, foreground, high intensity
// foreground)

#define MONO_COLOUR_BG   0
#define MONO_COLOUR_BG_I 1
#define MONO_COLOUR_FG   2
#define MONO_COLOUR_FG_I 3

// amber on black colour table
const uint8_t monam_table[4][3] =
{
//       B              G              R
 {MONAM_BGB,    MONAM_BGG,    MONAM_BGR},
 {MONAM_BGB_I,  MONAM_BGG_I,  MONAM_BGR_I},
 {MONAM_FGB,    MONAM_FGG,    MONAM_FGR},
 {MONAM_FGB_I,  MONAM_FGG_I,  MONAM_FGR_I}
};

// green on black colour table
const uint8_t mongr_table[4][3] =
{
//       B              G              R
 {MONGR_BGB,    MONGR_BGG,    MONGR_BGR},
 {MONGR_BGB_I,  MONGR_BGG_I,  MONGR_BGR_I},
 {MONGR_FGB,    MONGR_FGG,    MONGR_FGR},
 {MONGR_FGB_I,  MONGR_FGG_I,  MONGR_FGR_I}
};

// black on white colour table
const uint8_t monbw_table[4][3] =
{
//       B              G              R
 {MONBW_BGB,    MONBW_BGG,    MONBW_BGR},
 {MONBW_BGB_I,  MONBW_BGG_I,  MONBW_BGR_I},
 {MONBW_FGB,    MONBW_FGG,    MONBW_FGR},
 {MONBW_FGB_I,  MONBW_FGG_I,  MONBW_FGR_I}
};

// white on black colour table
const uint8_t monwb_table[4][3] =
{
//       B              G              R
 {MONWB_BGB,    MONWB_BGG,    MONWB_BGR},
 {MONWB_BGB_I,  MONWB_BGG_I,  MONWB_BGR_I},
 {MONWB_FGB,    MONWB_FGG,    MONWB_FGR},
 {MONWB_FGB_I,  MONWB_FGG_I,  MONWB_FGR_I}
};

// user configurable colour table
uint8_t mon_table[4][3] =
{
//       B              G              R
 {MONUSR_BGB,    MONUSR_BGG,    MONUSR_BGR},
 {MONUSR_BGB_I,  MONUSR_BGG_I,  MONUSR_BGR_I},
 {MONUSR_FGB,    MONUSR_FGG,    MONUSR_FGR},
 {MONUSR_FGB_I,  MONUSR_FGG_I,  MONUSR_FGR_I}
};

//==============================================================================
// RGB analogue monitor colour values for the standard colour circuit.
//
// This table is for emulation of a colour monitor connected to X3, pins 11,
// 12 and 13.  There are 2 intensity levels per gun output colour.
//
// Reference: Microbee Technical manual 1986,  MB1217 schematic.
//
// The "xxbgrBGR" values determines the gun intensity (bgr) and colour (BGR)
// levels. This is the normal ordering in hardware so is implemented in the
// same way here.
//
// For a colour gun to be switched on requires the upper case gun bit
// position to be set (1).
//
// For the colour to be in high intensity the corresponding lower case gun
// letter bit position must also be set (1).
//
// For the colour to be in low intensity the corresponding lower case gun
// letter bit position must be clear (0).
//==============================================================================
static const uint8_t col_table_1[64][3] =
{
//   R     G     B              bgrBGR (00)
 {   0,    0,    0},    // 00 xx000000 black
 {   0,    0, C_HI},    // 04 xx000100 blue
 {   0, C_HI,    0},    // 02 xx000010 green
 {   0, C_HI, C_HI},    // 06 xx000110 cyan
 {C_HI,    0,    0},    // 01 xx000001 red
 {C_HI,    0, C_HI},    // 05 xx000101 magenta
 {C_HI, C_HI,    0},    // 03 xx000011 yellow
 {C_HI, C_HI, C_HI},    // 07 xx000111 grey

//   R     G     B              bgrBGR (01)
 {   0,    0,    0},    // 08 xx001000 black
 {   0,    0, C_FI},    // 12 xx001100 blue
 {   0, C_HI,    0},    // 10 xx001010 green
 {   0, C_HI, C_HI},    // 14 xx001110 cyan
 {C_FI,    0,    0},    // 09 xx001001 red
 {C_FI,    0, C_HI},    // 13 xx001101 magenta
 {C_FI, C_HI,    0},    // 11 xx001011 yellow
 {C_FI, C_HI, C_HI},    // 15 xx001111 grey

//   R     G     B              bgrBGR (02)
 {   0,    0,    0},    // 16 xx010000 black
 {   0,    0, C_HI},    // 20 xx010100 blue
 {   0, C_FI,    0},    // 18 xx010010 green
 {   0, C_FI, C_HI},    // 22 xx010110 cyan
 {C_HI,    0,    0},    // 17 xx010001 red
 {C_HI,    0, C_HI},    // 21 xx010101 magenta
 {C_HI, C_FI,    0},    // 19 xx010011 yellow
 {C_HI, C_FI, C_HI},    // 23 xx010111 grey

//   R     G     B              bgrBGR (03)
 {   0,    0,    0},    // 24 xx011000 black
 {   0,    0, C_HI},    // 28 xx011100 blue
 {   0, C_FI,    0},    // 26 xx011010 green
 {   0, C_FI, C_HI},    // 30 xx011110 cyan
 {C_FI,    0,    0},    // 25 xx011001 red
 {C_FI,    0, C_HI},    // 29 xx011101 magenta
 {C_FI, C_FI,    0},    // 27 xx011011 yellow
 {C_FI, C_FI, C_HI},    // 31 xx011111 grey

//   R     G     B              bgrBGR (04)
 {   0,    0,    0},    // 32 xx100000 black
 {   0,    0, C_FI},    // 36 xx100100 blue
 {   0, C_HI,    0},    // 34 xx100010 green
 {   0, C_HI, C_FI},    // 38 xx100110 cyan
 {C_HI,    0,    0},    // 33 xx100001 red
 {C_HI,    0, C_FI},    // 37 xx100101 magenta
 {C_HI, C_HI,    0},    // 35 xx100011 yellow
 {C_HI, C_HI, C_FI},    // 39 xx100111 grey

//   R     G     B              bgrBGR (05)
 {   0,    0,    0},    // 40 xx101000 black
 {   0,    0, C_FI},    // 44 xx101100 blue
 {   0, C_HI,    0},    // 42 xx101010 green
 {   0, C_HI, C_FI},    // 46 xx101110 cyan
 {C_FI,    0,    0},    // 41 xx101001 red
 {C_FI,    0, C_FI},    // 45 xx101101 magenta
 {C_FI, C_HI,    0},    // 43 xx101011 yellow
 {C_FI, C_HI, C_FI},    // 47 xx101111 grey

//   R     G     B              bgrBGR (06)
 {   0,    0,    0},    // 48 xx110000 black
 {   0,    0, C_FI},    // 52 xx110100 blue
 {   0, C_FI,    0},    // 50 xx110010 green
 {   0, C_FI, C_FI},    // 54 xx110110 cyan
 {C_HI,    0,    0},    // 49 xx110001 red
 {C_HI,    0, C_FI},    // 53 xx110101 magenta
 {C_HI, C_FI,    0},    // 51 xx110011 yellow
 {C_HI, C_FI, C_FI},    // 55 xx110111 grey

//   R     G     B              bgrBGR (07)
 {   0,    0,    0},    // 56 xx111000 black
 {   0,    0, C_FI},    // 60 xx111100 blue
 {   0, C_FI,    0},    // 58 xx111010 green
 {   0, C_FI, C_FI},    // 62 xx111110 cyan
 {C_FI,    0,    0},    // 57 xx111001 red
 {C_FI,    0, C_FI},    // 61 xx111101 magenta
 {C_FI, C_FI,    0},    // 59 xx111011 yellow
 {C_FI, C_FI, C_FI}     // 63 xx111111 white
};

//==============================================================================
// rgbRGB digital monitor colour values for the standard colour circuit.
//
// This table is for emulation of a rgbRGB colour monitor connected to X3,
// pins 3-8.
//
// Reference: Microbee Technical manual 1986,  MB1217 schematic(s).
//
// The "xxbgrBGR" values determines the gun intensity (bgr) and colour (BGR)
// levels. This is the normal ordering in hardware so is implemented in the
// same way here.
//==============================================================================
static const uint8_t col_table_2[64][3] =
{
//   R     G     B              bgrBGR (00)
 {   0,    0,    0},    // 00 xx000000 black
 {   0,    0, E_AA},    // 04 xx000100 blue
 {   0, E_AA,    0},    // 02 xx000010 green
 {   0, E_AA, E_AA},    // 06 xx000110 cyan
 {E_AA,    0,    0},    // 01 xx000001 red
 {E_AA,    0, E_AA},    // 05 xx000101 magenta
 {E_AA, E_AA,    0},    // 03 xx000011 yellow
 {E_AA, E_AA, E_AA},    // 07 xx000111 grey

//   R     G     B              bgrBGR (01)
 {   0,    0, E_55},    // 08 xx001000 black
 {   0,    0, E_FF},    // 12 xx001100 blue
 {   0, E_AA, E_55},    // 10 xx001010 green
 {   0, E_AA, E_FF},    // 14 xx001110 cyan
 {E_AA,    0, E_55},    // 09 xx001001 red
 {E_AA,    0, E_FF},    // 13 xx001101 magenta
 {E_AA, E_AA, E_55},    // 11 xx001011 yellow
 {E_AA, E_AA, E_FF},    // 15 xx001111 grey

//   R     G     B              bgrBGR (02)
 {   0, E_55,    0},    // 16 xx010000 black
 {   0, E_55, E_AA},    // 20 xx010100 blue
 {   0, E_FF,    0},    // 18 xx010010 green
 {   0, E_FF, E_AA},    // 22 xx010110 cyan
 {E_AA, E_55,    0},    // 17 xx010001 red
 {E_AA, E_55, E_AA},    // 21 xx010101 magenta
 {E_AA, E_FF,    0},    // 19 xx010011 yellow
 {E_AA, E_FF, E_AA},    // 23 xx010111 grey

//   R     G     B              bgrBGR (03)
 {   0, E_55, E_55},    // 24 xx011000 black
 {   0, E_55, E_FF},    // 28 xx011100 blue
 {   0, E_FF, E_55},    // 26 xx011010 green
 {   0, E_FF, E_FF},    // 30 xx011110 cyan
 {E_AA, E_55, E_55},    // 25 xx011001 red
 {E_AA, E_55, E_FF},    // 29 xx011101 magenta
 {E_AA, E_FF, E_55},    // 27 xx011011 yellow
 {E_AA, E_FF, E_FF},    // 31 xx011111 grey

//   R     G     B              bgrBGR (04)
 {E_55,    0,    0},    // 32 xx100000 black
 {E_55,    0, E_AA},    // 36 xx100100 blue
 {E_55, E_AA,    0},    // 34 xx100010 green
 {E_55, E_AA, E_AA},    // 38 xx100110 cyan
 {E_FF,    0,    0},    // 33 xx100001 red
 {E_FF,    0, E_AA},    // 37 xx100101 magenta
 {E_FF, E_AA,    0},    // 35 xx100011 yellow
 {E_FF, E_AA, E_AA},    // 39 xx100111 grey

//   R     G     B              bgrBGR (05)
 {E_55,    0, E_55},    // 40 xx101000 black
 {E_55,    0, E_FF},    // 44 xx101100 blue
 {E_55, E_AA, E_55},    // 42 xx101010 green
 {E_55, E_AA, E_FF},    // 46 xx101110 cyan
 {E_FF,    0, E_55},    // 41 xx101001 red
 {E_FF,    0, E_FF},    // 45 xx101101 magenta
 {E_FF, E_AA, E_55},    // 43 xx101011 yellow
 {E_FF, E_AA, E_FF},    // 47 xx101111 grey

//   R     G     B              bgrBGR (06)
 {E_55, E_55,    0},    // 48 xx110000 black
 {E_55, E_55, E_AA},    // 52 xx110100 blue
 {E_55, E_FF,    0},    // 50 xx110010 green
 {E_55, E_FF, E_AA},    // 54 xx110110 cyan
 {E_FF, E_55,    0},    // 49 xx110001 red
 {E_FF, E_55, E_AA},    // 53 xx110101 magenta
 {E_FF, E_FF,    0},    // 51 xx110011 yellow
 {E_FF, E_FF, E_AA},    // 55 xx110111 grey

//   R     G     B              bgrBGR (07)
 {E_55, E_55, E_55},    // 56 xx111000 black
 {E_55, E_55, E_FF},    // 60 xx111100 blue
 {E_55, E_FF, E_55},    // 58 xx111010 green
 {E_55, E_FF, E_FF},    // 62 xx111110 cyan
 {E_FF, E_55, E_55},    // 57 xx111001 red
 {E_FF, E_55, E_FF},    // 61 xx111101 magenta
 {E_FF, E_FF, E_55},    // 59 xx111011 yellow
 {E_FF, E_FF, E_FF}     // 63 xx111111 grey
};

//==============================================================================
// CGA Colour values for Premium, Teleterm and 256TC colour circuits
//
// 
//==============================================================================
uint8_t col_table_p[16][3] =
{
/*
 * The colour indices here have been determined from the Alpha+
 * circuit diagram and the order of the colour components is
 * consistent with the component ordering for the colour arrays for
 * the older colour board.
 *
 * The Cyan and Dark Yellow values have been modified as described on
 * the IBM Colour Graphics Adaptor Wikipedia page. Dark Yellow (aka
 * brown) is handled specially by most RGBI monitors as low intensity
 * Yellow looks bad.  See
 * http://en.wikipedia.org/wiki/Color_Graphics_Adapter
 */
 //  R     G     B                IBGR
 {   0,    0,    0},    // 00 xxxx0000 black
 {   C_HI, 0,    0},    // 01 xxxx0001 red
 {   0, C_HI,    0},    // 02 xxxx0010 green
 {C_HI, C_LI,    0},    // 03 xxxx0101 brown     <--- modified
 {   0,    0, C_HI},    // 04 xxxx0100 blue
 {C_HI,    0, C_HI},    // 05 xxxx0101 magenta
 {   0, C_HI, C_HI},    // 06 xxxx0110 cyan      <--- modified
 {C_HI, C_HI, C_HI},    // 07 xxxx0111 light grey

 {C_LI, C_LI, C_LI},    // 08 xxxx1000 dark grey
 {C_FI, C_LI, C_LI},    // 09 xxxx1001 light red
 {C_LI, C_FI, C_LI},    // 10 xxxx1010 light green
 {C_FI, C_FI, C_LI},    // 11 xxxx1011 yellow
 {C_LI, C_LI, C_FI},    // 12 xxxx1100 light blue
 {C_FI, C_LI, C_FI},    // 13 xxxx1101 light magenta
 {C_LI, C_FI, C_FI},    // 14 xxxx1110 light cyan
 {C_FI, C_FI, C_FI}     // 15 xxxx1111 white
};

//==============================================================================
// 82S123 PROM Colour table used on colour board (IC 7).
//
// The values found here have been reversed engineered by running some capture
// software on a standard colour model reading the values back in on the
// Microbee's parallel port.  The software took multiple samples to eliminate
// sync signals from the final result.
//
// See Microbee Disk System manaul section E-9.
//
// There are 26 unique colour combinations, or 27 if black is counted.
//
// Colours names with a leading '#' are colours that are not easy to describe
// and the colour description is intended as a guide only.
//==============================================================================
uint8_t ic_82s23[32] =
{
//                         nn Colour
 0x00,                  // 00 black
 0x09,                  // 01 blue
 0x12,                  // 02 green
 0x1b,                  // 03 cyan
 0x24,                  // 04 red
 0x2d,                  // 05 magenta
 0x36,                  // 06 yellow
 0x3f,                  // 07 white

 0x0b,                  // 08 #dark cyan
 0x13,                  // 09 #aqua
 0x0d,                  // 10 #purple
 0x2d,                  // 11 #pink
 0x16,                  // 12 #green
 0x26,                  // 13 #orange
 0x07,                  // 14 #grey
 0x3f,                  // 15 #white

 0x00,                  // 16 black
 0x01,                  // 17 blue II
 0x02,                  // 18 green II
 0x03,                  // 19 cyan II
 0x04,                  // 20 red II
 0x05,                  // 21 magenta II
 0x06,                  // 22 yellow II
 0x07,                  // 23 white II

 0x0f,                  // 24 #blue
 0x17,                  // 25 #green
 0x1f,                  // 26 #cyan
 0x27,                  // 27 #pink
 0x2f,                  // 28 #pink
 0x37,                  // 29 #beige
 0x07,                  // 30 #grey
 0x3f                   // 31 #white
};

//==============================================================================
// Standard colour Red/Blue bit swap for background colours.  This is required
// as the base colour values are arranged for the PROM lookup.
//
// See Microbee Disk System manaul section E-9.
//==============================================================================
uint8_t bg_standard_colour[8] =
{
//                         nn Colour
 B8(00000000),          // 00 black
 B8(00000100),          // 01 blue
 B8(00000010),          // 02 green
 B8(00000110),          // 03 cyan
 B8(00000001),          // 04 red
 B8(00000101),          // 05 magenta
 B8(00000011),          // 06 yellow
 B8(00000111)           // 07 grey/white
};

//==============================================================================
//
// Prototypes for internal functions
//
//==============================================================================
void vdu_destroy_char_surface(void);
void vdu_create_char_surface(void);
void vdu_fill_char_surface(void);

//==============================================================================
// VDU initialisation
//
// Make VDU RAM location look like how real static RAM would.  This is needed
// by Basic 5.22e (possibly 6.22e) to detect if hardware supports colour.
// It appears as if 5.22e Basic does not set 0xFFFF to 0xFF in the PCG RAM and
// just assumes it will be non zero from a cold start because of the typical
// static RAM pattern value when starting up the Microbee ?  Running TBASICC.COM
// works when loaded from CP/M because the system already set 0xFFFF to 0xFF
// when it made the inverse character set.
//
// Any unused alpha+ PCG banks will contain random data, testing on a Premium
// Microbee shows the values as mostly zero but can vary greatly between tests.
//
//   pass: void
// return: int                          0 if success else -1
//==============================================================================
int vdu_init (void)
{
 int i;

 // Alpha+ (Premium) variables, 8K for Screen, Colour and attribute, 32K for PCG
 if (modelx.alphap)
    {
     memmap_init6264(vdu.scr_ram, 4);
#if 0
     memmap_init6264(vdu.col_ram, 4);
#else  // prevent b&w checker board on start-up
     memset(vdu.col_ram, 0, sizeof(vdu.col_ram));
#endif
     memmap_init6264(vdu.att_ram, 4);

     for (i = 0; i < PCG_RAM_BANKS; i++)
        {
         if (i < modelx.pcg)
            memmap_init6264(vdu.pcg_ram + i * 0x0800, 1);
         else
            memset(vdu.pcg_ram + i * 0x0800, 0x00, 0x0800);
        }
    }
 else
    // Standard model 2K Screen, 2K Colour, 2K PCG
    {
     memmap_init6116(vdu.scr_ram, 1);
#if 0
     memmap_init6116(vdu.col_ram, 1);
#else  // prevent b&w checker board on start-up
     memset(vdu.col_ram, 0, sizeof(vdu.col_ram));
#endif
     memmap_init6116(vdu.pcg_ram, 1);
     // Initialise values in the Screen RAM for the PC85 shell ROM
     // and the PCG RAM for Microworld Basic 5.22e
     //
     vdu.pcg_ram[0x07ff] = 0xff;
     vdu.scr_ram[0x043e] = 0xff;
     vdu.scr_ram[0x043f] = 0xff;
    }

 if (modelx.alphap)
    {
     vdu.lv_dat = 0;                        // port (0x1c) value
     vdu.attribram = 0;                     // attribute RAM select
     vdu.extendram = 0;                // extended graphics select
     basofs = (vdu.lv_dat & B8(00100000)) ? 0x2000 : 0;
    }

 if (modelx.colour)
    {
     vdu.colour_cont = 0;
     vdu.x_colour_cont = 0;
     vdu.colourram = 0;                     // set to PCG
    }

 memset(vdu.redraw, 0, sizeof(vdu.redraw));

 vdu.scr_ptr = vdu.scr_ram;
 vdu.atr_ptr = vdu.att_ram;
 vdu.col_ptr = vdu.col_ram;
 vdu.pcg_ptr = vdu.pcg_ram;
 vdu.redraw_ptr = vdu.redraw;
 vdu.scr_mask = ~(~0 << 11);

 vdu_setcolourtable();
 vdu_create_char_surface();
 vdu_fill_char_surface();

 return 0;
}

//==============================================================================
// VDU de-initialisation
//
//   pass: void
// return: int                          0
//==============================================================================
int vdu_deinit (void)
{
 vdu_destroy_char_surface();
 return 0;
}

//==============================================================================
// VDU reset
//
//   pass: void
// return: int                          0
//==============================================================================
int vdu_reset (void)
{
 crtc.latchrom = 0;

 if (modelx.alphap)
    {
     vdu.lv_dat = 0;                        // port (0x1c) value
     vdu.x_lv_dat = 0;
     vdu.attribram = 0;                     // attribute RAM select
     vdu.extendram = 0;                // extended graphics select
     basofs = (vdu.lv_dat & B8(00100000)) ? 0x2000 : 0;
    }

 if (modelx.colour)
    {
     vdu.colour_cont = 0;
     vdu.x_colour_cont = 0;
     vdu.colourram = 0;                     // set to PCG
    }

 vdu.scr_mask = ~(~0 << 11);

 return 0;
}

//==============================================================================
// Video memory read.
//
// When reading PCG memory from a location that does not have RAM installed
// a 0 is returned when tested on a 16K PCG Premium 128K model.  This was
// tested under Premium MWB with the following code:
//
// 10 OUT 28,143          OUT 0x1c,8f selects PCG bank 15
// 20 POKE 63488,170      0xf800 = 0xaa
// 30 A = PEEK(63488)     get byte from 0xf800 (first address of PCG)
// 40 POKE 63488,85       0xf800 = 0x55
// 50 B = PEEK(63488)     get byte from 0xf800 (first address of PCG)
// 60 OUT 28,0            OUT 0x1c,0 switches out the extended PCG memory
// 70 PRINT A B           print the results
//
// run
// 0 0
//
//   pass: uint32_t addr
//         struct z80_memory_read_byte *mem_s
// return: uint8_t
//==============================================================================
uint8_t vdu_vidmem_r (uint32_t addr, struct z80_memory_read_byte *mem_s)
{
#if 0
 if (modio.vdumem)
    log_port_1("vdu_vidmem_r", "addr", port, (unsigned int)addr);
#endif

 if (addr & 0x0800)  // if PCG or Colour RAM
    {
     return
        vdu.colourram ? vdu.col_ptr[addr & 0x7FF] :
        vdu.pcg_ptr != NULL ? vdu.pcg_ptr[addr & 0x7FF] :
        0;
    }
 else if (! crtc.latchrom)
    {
     return (vdu.attribram ? vdu.atr_ptr : vdu.scr_ptr)[addr & 0x7FF];
    }
 else
    /* FIXME: 256TC extended character ROM selection */
    return vdu.chr_rom[((crtc.disp_start & 0x2000) >> 2) + (addr & 0x07FF)];
}

//==============================================================================
// Video memory write.
//
// All writes to VDU memory are first checked to see if the location will
// change. If the location is the same nothing is done, this prevents
// unnecessary time consuming video rendering taking place.
//
//   pass: uint32_t addr
//         uint8_t data
//         struct z80_memory_write_byte *mem_s
// return: void
//==============================================================================
void vdu_vidmem_w (uint32_t addr, uint8_t data,
                   struct z80_memory_write_byte *mem_s)
{
 uint8_t *vidmem_ptr;

#if 0
 if (modio.vdumem)
    log_port_2("vdu_vidmem_w", "addr", "data", port, (unsigned int)addr, data);
#endif

 /*
  *  Updates to colour, screen or attribute RAM
  */

 // if colour or PCG RAM
 if (addr & 0x0800)
    {
     vidmem_ptr = (!vdu.colourram) ? vdu.pcg_ptr : vdu.col_ptr;
    }
 else if (crtc.latchrom)
    {
     return;                    /* ignore writes to the character ROM */
    }
 else
    {
     vidmem_ptr = vdu.attribram ? vdu.atr_ptr : vdu.scr_ptr;
    }

 if (vidmem_ptr == NULL)
    return;                     /* write to non-existent memory */
 vidmem_ptr += addr & 0x7FF;
 if (*vidmem_ptr == data)
    return;                     /* Avoid drawing anything if the
                                 * screen location doesn't change. */
 if (!vdu.colourram && (addr & 0x0800))
    {
     // PCG write
     vdu_write_pcg_data(vdu.videobank, addr & 0x07FF, &data, 1);
     vdu.pcg_redraw[vdu.videobank * 128 + (addr & 0x07ff) / 16] = 1;
    }
 else
    {
     vdu.redraw_ptr[addr & 0x07FF] = 1; /* note that this screen location needs to be redrawn */
    }
 *vidmem_ptr = data;
 /*
  * Rendering of the changed character is deferred to the "update
  * interval"
  */
}

//==============================================================================
// Read Port 0x08 - colour control
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t                     0
//==============================================================================
uint16_t vdu_colcont_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.vdu)
    log_port_0("vdu_colcont_r", port);

 return 0;
}

//==============================================================================
// Write Port 0x08 - colour control
//
// Issue introduced in v3.0.0/1 when running in standard ROM model
// monochrome mode and Telcom versions 1.1 or 1.2.  The top status line
// (after 'BAS') has what appears to be the "emulated" uninitialised screen
// RAM.  Calling crtc_set_redraw() here for non-colour models fixes the
// problem but I'm not sure why or what changes in v3.0.0 caused this issue.
// It appears to be caused by applications that test for a colour model.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void vdu_colcont_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modelx.colour)
    {
     if (modio.vdu)
        log_port_1("vdu_colcont_w", "data", port, vdu.colour_cont);

     /*
      * The colour port's bits are assigned as follows
      *
      * Bit 7   not used
      * Bit 6   colour RAM enable
      * Bit 5   not used
      * Bit 4   not used
      * Bit 3   Background Blue intensity bit / unused
      * Bit 2   Background Green intensity bit / unused
      * Bit 1   Background Red intensity bit / unused[*]
      * Bit 0   unused / unused[*]
      *
      * Bits 3,2,1 were used on the original Microbee colour board and
      * are not used by the 256TC/Alpha+
      *
      * Bits 7, 1 and 0 are latched but not used on the Alpha+
      * motherboard
      */
     vdu.colour_cont = data;
     if (modelx.colour == 1)    /* FIXME: this should be an ENUM */
        {
         // If any of the RGB background intensity bits have changed,
         // the entire screen must be redrawn.
         if ((vdu.x_colour_cont & B8(00001110)) !=
             (vdu.colour_cont & B8(00001110)))
            crtc_set_redraw();
        }

     if ((vdu.x_colour_cont & B8(01000000)) != (vdu.colour_cont & B8(01000000)))
       vdu.colourram = vdu.colour_cont & B8(01000000);

     vdu.x_colour_cont = vdu.colour_cont;
    }
 else
    crtc_set_redraw();  // see above for the reason
}

//==============================================================================
// Read Port 0x09 - Colour wait off
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: UNINT16                      0
//==============================================================================
uint16_t vdu_colwait_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.vdu)
    log_port_0("vdu_colwait_r", port);

 return 0;
}

//==============================================================================
// Write Port 0x09 - Colour wait off
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void vdu_colwait_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.vdu)
    log_port_1("vdu_colwait_w", "data", port, data);
}

//==============================================================================
// Read Port 0x1C - LV DATA
//
// Will only be called if emulating an alpha+ model.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t                     lv_dat
//==============================================================================
uint16_t vdu_lvdat_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.vdu)
    log_port_1("vdu_lvdat_r", "lv_dat", port, vdu.lv_dat);

 if ((port & 0xff) == 0x1C)
    return vdu.lv_dat;
 else
    return 0;
}

//==============================================================================
// Write Port 0x1C - LV DATA
//
// Will only be called if emulating an alpha+ model.
//
// The dummy PCG bank is used when the requested PCG bank does not exist 

//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void vdu_lvdat_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.vdu)
    log_port_1("vdu_lvdat_w", "data", port, data);

 if ((port & 0xff) != 0x1C)
    return;

 vdu.lv_dat = data & B8(10111111);                  // port (0x1c) value
 if (vdu.x_lv_dat == vdu.lv_dat)
    return;
 
 basofs = (vdu.lv_dat & B8(00100000)) ? 0x2000 : 0;

 if ((vdu.x_lv_dat & ~B8(00100000)) != (vdu.lv_dat & ~B8(00100000)))
    {
     vdu.videobank = vdu.lv_dat & B8(00001111);         // bank bits 0-3
     vdu.attribram = vdu.lv_dat & B8(00010000);         // attribute RAM select
     vdu.extendram = vdu.lv_dat & B8(10000000);    // extended graphics select
     
     vdu.scr_mask = ~(~0 << 11);
     if (vdu.extendram)
        vdu.scr_mask |= modelx.vdu << 11;
     else
        vdu.videobank = 0;
     vdu.scr_ptr = vdu.scr_ram + (vdu.videobank & modelx.vdu) * 0x0800;
     vdu.atr_ptr = vdu.att_ram + (vdu.videobank & modelx.vdu) * 0x0800;
     vdu.col_ptr = vdu.col_ram + (vdu.videobank & modelx.vdu) * 0x0800;
     vdu.pcg_ptr = (vdu.videobank >= modelx.pcg) ? NULL : vdu.pcg_ram + vdu.videobank * 0x800;
     vdu.redraw_ptr = vdu.redraw + (vdu.videobank & modelx.vdu) * 0x0800; 
     crtc_set_redraw();
    }
 vdu.x_lv_dat = vdu.lv_dat;                         // port (0x1c) value
}

//==============================================================================
// Port 0x0B - Character ROM select
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void vdu_latchrom_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 crtc.latchrom = data & B8(00000001);

 if (modio.vdu)
    log_port_1("vdu_latchrom_w", "crtc.latchrom", port, crtc.latchrom);
}


//==============================================================================
//
// Propagate updates to the PCG ram back to the screen RAM
//
//==============================================================================
void vdu_propagate_pcg_updates(int maddr, int size)
{
 int pcgbank;
 uint8_t data;

 for (; size > 0; ++maddr, --size)
    {
     data = vdu.scr_ram[maddr & vdu.scr_mask];
     if (!(data & 0x80))
        continue;
     pcgbank = (vdu.extendram) ? (vdu.att_ram[maddr & vdu.scr_mask] & B8(00001111)) : 0;
     if (pcgbank >= modelx.pcg)
        continue;               /* The selected PCG bank isn't
                                 * physically present, so it cannot be
                                 * updated. */
     if (vdu.pcg_redraw[pcgbank * 128 + (data & 0x7f)])
        vdu_redraw_char(maddr);
    }
 memset(vdu.pcg_redraw, 0, sizeof(vdu.pcg_redraw));
}

//==============================================================================
//
// Propagate the flashing attribute bit
//
//==============================================================================
void vdu_propagate_flashing_attr(int maddr, int size)
{
 if (!(vdu.extendram))
    return;                     /* premium graphics not enabled */
 for (; size > 0; ++maddr, --size)
    {
     if (vdu.att_ram[maddr & vdu.scr_mask] & B8(10000000))
        vdu_redraw_char(maddr);
    }
}

/*
 * Note that the character at screen location addr must be redrawn
 */
void vdu_redraw_char(int maddr)
{
 vdu.redraw[maddr & vdu.scr_mask] = 1;
}

/*
 * Test whether the character at screen location addr must be redrawn
 */
uint8_t vdu_char_is_redrawn(int maddr)
{
 return vdu.redraw[maddr & vdu.scr_mask];
}

/*
 * Note that the character at screen location addr has been redrawn
 */
void vdu_char_clear_redraw(int maddr)
{
 vdu.redraw[maddr & vdu.scr_mask] = 0;
}

//==============================================================================
//
// Write data to the character data buffer.
//
//==============================================================================

void vdu_get_char_pos(int bank, int ch, int *x, int *y)
{
 int o = bank * CHAR_SURFACE_BANK_SIZE + ch;
 *x = (o % CHAR_SURFACE_WIDTH_CHARS) * 8;
 *y = (o / CHAR_SURFACE_WIDTH_CHARS) * 16 * video.yscale;
}

void vdu_write_pcg_data(int bank, int offset, uint8_t *data, int numbytes)
{
 vdu_write_char_data(bank + CHAR_SURFACE_ROM_BANKS, offset, data, numbytes);
}

void vdu_write_char_data(int bank, int offset, uint8_t *data, int numbytes)
{
 bank *= CHAR_SURFACE_BANK_SIZE;
 SDL_LockSurface(char_data);
 while (numbytes)
    {
     int line = offset % 16;
     int o = bank + offset / 16;
     int x = (o % CHAR_SURFACE_WIDTH_CHARS) * 8;
     int y = (o / CHAR_SURFACE_WIDTH_CHARS) * 16 + line;
     int y1;
     int i;
     uint8_t d;

     y *= char_data->pitch * video.yscale;
     for (y1 = 0; y1 < video.yscale; ++y1, y += char_data->pitch)
        {
         switch (char_data->format->BitsPerPixel)
            {
             case 1:
                /* 1bpp is even easier! */
                *(uint8_t *)(char_data->pixels + y + x / 8) = *data;
                break;
             case 8:
                for (i = 0, d = *data; i < 8; ++i, d <<= 1)
                   {
                    *(uint8_t *)(char_data->pixels + y + x * 1 + i) = 
                       (d & (1 << 7)) ? 1 : 0; 
                   }
                break;
                /* other bit depths are left as an exercise for the
                 * reader */
             case 16:
             case 24:
             default:
             break;
            }
        }
     ++offset;
     ++data;
     --numbytes;
    }
 SDL_UnlockSurface(char_data);
}

//==============================================================================
//
// Draw a character
//
//==============================================================================

void vdu_draw_char(SDL_Surface *screen, int x, int y,
                   int maddr,     /* CRTC address of character to draw */
                   uint8_t lines, /* number of lines to draw */
                   uint8_t hwflash, /* whether the character is flashing */
                   uint8_t cursor, uint8_t cur_start, uint8_t cur_end)
{
 SDL_Rect srcrect, dstrect;
 static SDL_Color colours[2];
 static SDL_Color inverse_colours[2];
 SDL_Color *cmap;
 SDL_Color *inv_cmap;           /* for the moment */
 int sx, sy;                    /* source X and Y */
 int bank;
 uint8_t ch;
 uint8_t attrib;
 uint8_t inverse = 0;           /* don't invert the foreground &
                                 * background */
 uint8_t colour;
 int regionheights[3];          /* heights of each of the 3 regions of
                                 * the character when the cursor is
                                 * drawn. */
 int fgc, bgc;

 ch = vdu.scr_ram[maddr & vdu.scr_mask];
 attrib = vdu.extendram
    ? vdu.att_ram[maddr & vdu.scr_mask]
    : 0;
 colour = vdu.col_ram[maddr & vdu.scr_mask];

 if (ch & 0x80)
    {
     /* Draw a space insted of the PCG character if the PCG bank isn't
      * physically present */
     int pcgbank = attrib & B8(00001111);
     if (pcgbank >= modelx.pcg)
        {
         ch = ' ';
         bank = 0;
        }
     else
        bank = CHAR_SURFACE_PCG_BANK(pcgbank);
    }
 else
    {
     /* FIXME: 256TC extended character rom selection */
     bank = (emu.model != MOD_2MHZ && (maddr & 0x2000)) 
        ? CHAR_SURFACE_ROM_BANK(1) 
        : CHAR_SURFACE_ROM_BANK(0);
    }
 ch &= 0x7f;

 if ((attrib & B8(10000000)))
    {
     /* flashing attribute bit set */
     if (hwflash == HFV4)
        {
         // Premium (v4 mb), 256tc/tterm models flash by alternating
         // between normal and blank.
         ch = ' ';
        }
     else if (hwflash == HFV3)
        {
         // Premium models (v3 mb) flash by alternating between normal
         // and inverted.
         inverse = !inverse;
        }
    }
 if ((attrib & B8(01000000)) && modelx.hwflash == HFV4)
    {
     /* Hardware inverse is available only on the 256TC & premium v4
      * motherboards */
     inverse = !inverse;
    }
 vdu_get_char_pos(bank, ch, &sx, &sy);

 if (!cursor)
    {
     regionheights[0] = lines;      /* top non-cursor region */
     regionheights[1] = 0;          /* middle cursor region */
     regionheights[2] = 0;          /* bottom non-cursor region */
    }
 else
    {
     int i, l;
     if (cur_start > cur_end)
        {
         inverse = !inverse;
         regionheights[0] = cur_end + 1;
         regionheights[1] = cur_start - cur_end + 1;
         regionheights[2] = 32 - cur_start - 1;
        }
     else
        {
         regionheights[0] = cur_start;
         regionheights[1] = cur_end - cur_start + 1;
         regionheights[2] = 32 - cur_end - 1;
        }
     for (i = 0, l = lines; i < 3; ++i)
        {
         if (regionheights[i] > l)
            {
             regionheights[i] = l;
            }
         l -= regionheights[i];
        }
    }

 /*
  * Construct the inverse and normal colour maps from the global
  * colour map
  */
 if (modelx.colour == 0 || crtc.monitor)
    {
     // monochrome
     fgc = 2;
     bgc = 0;
     if ((modelx.alphap) && (modelx.halfint))
        {
         if (colour & B8(00001000))
            fgc++;
         if (colour & B8(10000000))
            bgc++;
        }
    }
 else if (modelx.colour == MODCOL1)
    {
     // 56k colour board
     fgc = ic_82s23[colour & B8(00011111)];
     bgc = (bg_standard_colour[(vdu.colour_cont & B8(00001110)) >> 1] << 3)
        | (bg_standard_colour[(colour & B8(11100000)) >> 5]);
    }
 else
    {
     // premium/teleterm/256tc
     fgc = (colour & 0x0F);
     bgc = (colour >> 4);
    }
 colours[0] = col_table[bgc];
 colours[1] = col_table[fgc];
 inverse_colours[0] = col_table[fgc];
 inverse_colours[1] = col_table[bgc];

 cmap = inverse ? inverse_colours : colours;
 inv_cmap = inverse ? colours : inverse_colours;

 srcrect.x = sx;
 srcrect.y = sy;
 srcrect.w = 8;
 srcrect.h = 0;
 dstrect.x = x;
 dstrect.y = y;
 dstrect.w = dstrect.h = 0;

 /* top non-cursor region */
 srcrect.h  = regionheights[0] * video.yscale;
 SDL_SetColors(char_data, cmap, 0, 2);
 SDL_BlitSurface(char_data, &srcrect, screen, &dstrect);
 srcrect.y += srcrect.h;
 dstrect.y += srcrect.h;

 /* cursor region */
 srcrect.h  = regionheights[1] * video.yscale;
 SDL_SetColors(char_data, inv_cmap, 0, 2);
 SDL_BlitSurface(char_data, &srcrect, screen, &dstrect);
 srcrect.y += srcrect.h;
 dstrect.y += srcrect.h;

 /* bottom non-cursor region */
 srcrect.h  = regionheights[2] * video.yscale;
 SDL_SetColors(char_data, cmap, 0, 2);
 SDL_BlitSurface(char_data, &srcrect, screen, &dstrect);
 srcrect.y += srcrect.h;
 dstrect.y += srcrect.h;

 /* finally, mark the region as needing to be updated */
 dstrect.x = x;
 dstrect.y = y;
 dstrect.w = 8;
 dstrect.h = lines * video.yscale;
 video_update_region(dstrect);
}       


//==============================================================================
// VDU set colour table
// Updates the cached SDL pixel values for each possible colour for the current
// screen surface.
//
//   pass: void
// return: void
//==============================================================================
void vdu_setcolourtable()
{
 int i;
 const uint8_t (*coltable)[3];

 if (modelx.colour == 0 || crtc.monitor)
    {
     /* For monochrome models we use the first 4 entries in col_table.
        The entries are numbered like so:

        0: low intensity background
        1: high intensity background
        2: low intensity foreground
        3: high intensity foreground
        
        Note that if the 1/2 intensity monochrome hardware isn't being
        modeled, the pixel value for high and low intensity will be the same
     */

     // set the colour scheme according to the monitor type
     switch (crtc.monitor)
        {
         default:
         case 0 :
            // colour (default)
            coltable = mongr_table;
            break;
         case 1 :
            // amber on black
            coltable = monam_table;
            break;
         case 2 :
            // green on black
            coltable = mongr_table;
            break;
         case 3 :
            // black on white
            coltable = monbw_table;
            break;
         case 4 :
            // white on black
            coltable = monwb_table;
            break;
         case 5 :
            // user configurable monochrome
            coltable = (const uint8_t(*)[3])mon_table;
            break;
        }

     for (i = 0; i < 4; ++i)
        {
         col_table[i].r = coltable[i][2];
         col_table[i].g = coltable[i][1];
         col_table[i].b = coltable[i][0];
        }

     if (! modelx.alphap || ! modelx.halfint)
        {
         col_table[MONO_COLOUR_BG_I] = col_table[MONO_COLOUR_BG];
         col_table[MONO_COLOUR_FG] = col_table[MONO_COLOUR_FG_I];
        }
    }
 else
    {
     if (modelx.alphap)
        {
         // premium
         coltable = (const uint8_t (*)[3])col_table_p;
         i = sizeof(col_table_p) / sizeof(col_table_p[0]);
        }
     else if (crtc.std_col_type == 0)
        {
         // Analogue colour monitor
         coltable = col_table_1;
         i = sizeof(col_table_1) / sizeof(col_table_1[0]);
        }
     else
        {
         // Digital colour monitor
         coltable = col_table_2;
         i = sizeof(col_table_2) / sizeof(col_table_2[0]);
        }
        
     while (i--)
        {
         /* FIXME: yes, backwards compared to the monochrome case */
         col_table[i].r = coltable[i][0];
         col_table[i].g = coltable[i][1];
         col_table[i].b = coltable[i][2];
        }
    }
}

//==============================================================================
// VDU set monochrome colour table.
//
// Sets the RGB values for foreground, background full and half intensities.
//
//   pass: int pos
//         int col
// return: void
//==============================================================================
void vdu_set_mon_table (int pos, int col)
{
 mon_table[pos/3][pos%3] = col;
}

//==============================================================================
// VDU configure.
//
// Determines the displayed aspect ratio to use.
//
//   pass: int aspect
// return: void
//==============================================================================
void vdu_configure (int aspect)
{
 if (char_data)
    vdu_destroy_char_surface();
 vdu_create_char_surface();
 vdu_fill_char_surface();
}

void vdu_create_char_surface(void)
{
 /*
  * Create a secondary, "static" SDL surface which holds the bit
  * patterns for the Character generator ROM (up to 4k, the 256TC's
  * 16k character ROM is not emulated) and up to 16 2k banks of PCG
  * memory.
  */
 char_data =
    SDL_CreateRGBSurface(SDL_SWSURFACE,
                         CHAR_SURFACE_WIDTH_PIXELS,
                         CHAR_SURFACE_HEIGHT_PIXELS * video.yscale,
                         8, /* 8bpp to force the use of a
                             * colourmap. Note that 1bpp also - sort
                             * of - works, but SDL's blitter for
                             * 1->24bpp doesn't work well.  */
                         0, 0, 0, 0 /* no pixel masks */
       );                     
}

void vdu_destroy_char_surface(void)
{
 SDL_FreeSurface(char_data);
 char_data = NULL;
}

void vdu_fill_char_surface(void)
{
 int pcgbanks = modelx.alphap ? modelx.pcg : 1;
 int i;

 vdu_write_char_data(0, 0, vdu.chr_rom, 0x1000);
 for (i = 0; i < pcgbanks; ++i)
    {
     vdu_write_pcg_data(i, 0, vdu.pcg_ram + i * 0x0800, 0x0800);
    }
}
