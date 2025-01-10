//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                                 z80 module                                 *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Provides an init/deinit and an interface to the Z80 routines.
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
// v5.7.0 - 1 Decemeber 2013, uBee
// - Added z80_ports_set() to conditionally set various ports based on flags
//   passed.  This is required for a new --expansion-port option in the future.
// - Changes to some port handler names to use trailing '_r' and '_w'.
// - z80_unhandled_r() and z80_unhandled_w() no longer declared static.
//
// v5.5.0 - 8 July 2013, uBee
// - Added emu.port58h_use to determine if the HDD model uses additional
//   port 0x58 to associate ports 0x40-0x47 with WD1002-5 or WD2793.
//
// v5.2.0 - 21 February 2011, K Duckmanton
// - Changes made for the new sn76489an sound emulation,  the conditional
//   SOUND compilation option has been removed.
//
// v4.6.0 - 4 May 2010, uBee
// - Renamed log_data() calls to log_port_0() and log_port() to log_port_1().
// - Added port_out_state[] and port_inp_state[] arrays to store port states.
//
// v4.2.0 - 15 July 2009, uBee
// - Added WD1002-5 Winchester/Floppy drive controller card emulation (hdd).
// - Improvements made to ports configuration.
//
// v4.1.0 - 22 June 2009, uBee
// - Added models SCF and PCF (Compact Flash CB Standard/Premium)
// - Added IDE HDD and extra CF memory mapping port.
// - Added ports configuration to z80_init() function.
// - Made improvements to the logging process.
//
// v4.0.0 - 8 June 2009, uBee
// - This module will now act as a common interface for various Z80
//   emulators.  All other modules needing access to the Z80 emultor should
//   only call a function defined here.
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
//
// v3.1.0 - 19 March 2009, uBee
// - Added z80_disabled_r() and z80_disabled_w() functions for disabled HW.
// - Added code to z80_init() to disable WD2793 emulation if requested.
// - Changes to z80_init() to initialise z80.z80MemRead and z80.z80MemWrite
//   with z80_mem_r and z80_mem_w instead of NULLs.  This is needed to prevent
//   a segmentation error if compiling in MZ80 v3.5 'C' source.
// - Changed all printf() calls to use a local xprintf() function.
//
// v2.7.0 - 22 May 2008, uBee
// - Added structure emu_t and moved variables into it.
//
// v2.5.0 - 20 March 2008, uBee
// - Use new member 'tckeys' in modelx structure as Teleterm model emulation
//   has been added.
//
// v2.3.0 - 9 January 2008, uBee
// - Added z80_regdump() function to dump mz80context and Z80 CPU registers.
// - Moved 'struct mz80context z80' from z80_init() function to global area
//   so that the z80 structure is initialized with 0s.  Removed all the other
//   'struct mz80context z80' in other functions and use the local one instead.
// - Added Serial Comminications Controller (SCC) ports 0x68 and 0x69.
// - Removed the alpha+ conditional compilations as model type is determined
//   at run time using the new alphap member in the model_t structure.
// - Added Port interface to call the appropriate port handler code depending
//   on the model emulated.
// - Port 0x18-0x1B write had incorrect handler and was fixed with the other
//   port changes.
// - Added modio_t structure.
//
// v2.1.0 - 28 October 2007, uBee
// - Implement the modelx information structure.
// - Add clock speed port 0x09 for read.
//
// v2.0.0 - 13 October 2007, uBee
// - Ports enabled for the Teleterm model and RTC emulation.
//
// v1.4.0 - 26 September 2007, uBee
// - Boot address (boot_addr) is now dependent on the model being emulated.
// - Added mz80init() to the z80_init() function.  This does not affect 386
//   ASM code builds but is required for 'C' builds of MZ80.  The 'C' build
//   is not currently working on i386 systems and possibly others.
// - Added changes to error reporting.
//
// v1.2.0 - 19 August 2007, uBee
// - Added a special emulator port to access built in emulator commands.
//
// v1.0.0 - 1 August 2007, uBee
// - Added alpha+ (Premimum) and other ports with compile time options.
//
// v0.0.0 - 5 June 2007, uBee
// Start with "nanowasp" source distribution version 0.22. An emulator for the
// microbee 128k. Copyright (C) 2000-2003  David G. Churchill.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ubee512.h"
#include "z80.h"
#include "z80api.h"
#include "crtc.h"
#include "memmap.h"
#include "vdu.h"
#include "fdc.h"
#include "hdd.h"
#include "ide.h"
#include "pio.h"
#include "roms.h"
#include "rtc.h"
#include "keytc.h"
#include "clock.h"
#include "scc.h"
#include "sn76489an.h"
#include "function.h"
#include "macros.h"

uint8_t port_out_state[256];
uint8_t port_inp_state[256];

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;

//==============================================================================
// Any port if used requires an appropriate C module to be compiled in.  The
// first part of the function name indicates which C file the function can
// found in.  The C modules are:
//
// pio.c        Z80 PIO (PC speaker, serial, tape and parallel port).
// rtc.c        Real Time Clock.
// vdu.c        Colour control & waitoff, latch ROM, lvdat (premium graphics).
// roms.c       NET and PAK selection.
// crtc.c       CRT 6845 emulation.
// soundchip.c  76489AN Sound chip emulation.
// keytc.c      Teleterm keyboard.
// fdc.c        Floppy Disk Controller - WD2793 emulation.
// scc.c        Serial SCC emulation.
// memmap.c     Memory map handler.
//==============================================================================

//==============================================================================
// Read ports handlers structure.
//
// All port associations are handled at initilisation time or set on the fly
// using the --expansion-port option or by other means.
//
// This method defines all 256 8 bit ports for maximum speed and can be used
// by the z80ex Z80 emulator and others.
//==============================================================================
uint16_t (*z80_ports_r[256])(uint16_t, struct z80_port_read *) =
{
 // 00-0F
 /* 00 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 04 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 08 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 0C */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // 10-1F
 /* 10 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 14 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 18 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 1C */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // 20-2F
 /* 20 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 24 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 28 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 2C */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // 30-3F
 /* 30 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 34 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 38 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 3C */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // 40-4F
 /* 40 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 44 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 48 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 4C */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // 50-5F
 /* 50 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 54 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 58 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 5C */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // 60-6F
 /* 60 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 64 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 68 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 6C */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // 70-7F
 /* 70 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 74 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 78 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 7C */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // 80-8F
 /* 80 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 84 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 88 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 8C */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // 90-9F
 /* 90 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 94 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 98 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* 9C */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // A0-AF
 /* A0 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* A4 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* A8 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* AC */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // B0-BF
 /* B0 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* B4 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* B8 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* BC */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // C0-CF
 /* C0 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* C4 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* C8 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* CC */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // D0-DF
 /* D0 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* D4 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* D8 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* DC */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // E0-EF
 /* E0 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* E4 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* E8 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* EC */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,

 // F0-FF
 /* F0 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* F4 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* F8 */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r,
 /* FC */  z80_unhandled_r, z80_unhandled_r, z80_unhandled_r, z80_unhandled_r
};

//==============================================================================
// Write ports handlers structure.
//
// All port associations are handled at initilisation time or set on the fly
// using the --expansion-port option or by other means.
//
// This method defines all 256 8 bit ports for maximum speed and can be used
// by the z80ex Z80 emulator and others.
//==============================================================================
void (*z80_ports_w[256])(uint16_t, uint8_t, struct z80_port_write *) =
{
 // 00-0F
 /* 00 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 04 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 08 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 0C */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // 10-1F
 /* 10 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 14 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 18 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 1C */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // 20-2F
 /* 20 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 24 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 28 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 2C */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // 30-3F
 /* 30 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 34 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 38 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 3C */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // 40-4F
 /* 40 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 44 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 48 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 4C */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // 50-5F
 /* 50 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 54 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 58 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 5C */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // 60-6F
 /* 60 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 64 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 68 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 6C */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // 70-7F
 /* 70 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 74 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 78 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 7C */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // 80-8F
 /* 80 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 84 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 88 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 8C */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // 90-9F
 /* 90 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 94 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 98 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* 9C */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // A0-AF
 /* A0 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* A4 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* A8 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* AC */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // B0-BF
 /* B0 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* B4 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* B8 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* BC */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // C0-CF
 /* C0 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* C4 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* C8 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* CC */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // D0-DF
 /* D0 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* D4 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* D8 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* DC */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // E0-EF
 /* E0 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* E4 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* E8 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* EC */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,

 // F0-FF
 /* F0 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* F4 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* F8 */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w,
 /* FC */  z80_unhandled_w, z80_unhandled_w, z80_unhandled_w, z80_unhandled_w
};

//==============================================================================
// Define the FDC ports.
//==============================================================================
uint16_t (*z80_ports_fdc_r[12])(uint16_t, struct z80_port_read *) =
{
 // 40-4F
 /* 40 */  fdc_status_r,    fdc_track_r,     fdc_sect_r,      fdc_data_r,
 /* 44 */  fdc_status_r,    fdc_track_r,     fdc_sect_r,      fdc_data_r,
 /* 48 */  fdc_ext_r,       fdc_ext_r,       fdc_ext_r,       fdc_ext_r
};

void (*z80_ports_fdc_w[12])(uint16_t, uint8_t, struct z80_port_write *) =
{
 // 40-4F
 /* 40 */  fdc_cmd_w,       fdc_track_w,     fdc_sect_w,      fdc_data_w,
 /* 44 */  fdc_cmd_w,       fdc_track_w,     fdc_sect_w,      fdc_data_w,
 /* 48 */  fdc_ext_w,       fdc_ext_w,       fdc_ext_w,       fdc_ext_w
};

//==============================================================================
// Define the WD1002-5 card ports.
//==============================================================================
uint16_t (*z80_ports_hdd_r[12])(uint16_t, struct z80_port_read *) =
{
 // 40-4F
 /* 40 */  hdd_data_r,      hdd_error_r,     hdd_sectorcount_r, hdd_sector_r,
 /* 44 */  hdd_cyl_low_r,   hdd_cyl_high_r,  hdd_sdh_r,         hdd_status_r,
 /* 48 */  hdd_fd_side_r,   hdd_fd_side_r,   hdd_fd_side_r,     hdd_fd_side_r
};

void (*z80_ports_hdd_w[12])(uint16_t, uint8_t, struct z80_port_write *) =
{
 // 40-4F
 /* 40 */  hdd_data_w,      hdd_precomp_w,   hdd_sectorcount_w, hdd_sector_w,
 /* 44 */  hdd_cyl_low_w,   hdd_cyl_high_w,  hdd_sdh_w,         hdd_cmd_w,
 /* 48 */  hdd_fd_side_w,   hdd_fd_side_w,   hdd_fd_side_w,     hdd_fd_side_w
};

//==============================================================================
// Define the memory mapping ports for DRAM models.
//==============================================================================
void (*z80_ports_mode1_w[8])(uint16_t, uint8_t, struct z80_port_write *) =
{
 // 50-57
 /* 50 */  memmap_mode1_w,  memmap_mode1_w,  memmap_mode1_w,    memmap_mode1_w,
 /* 54 */  memmap_mode1_w,  memmap_mode1_w,  memmap_mode1_w,    memmap_mode1_w
};

//==============================================================================
// Define the read/write ports for 3rd party IDE HDD add-on access.  These
// are used by the CF model and possibly others.
//==============================================================================
uint16_t (*z80_ports_ide_r[8])(uint16_t, struct z80_port_read *) =
{
 // 60-67
 /* 60 */  ide_data_r,      ide_error_r,     ide_sectorcount_r, ide_sector_r,
 /* 64 */  ide_cyl_low_r,   ide_cyl_high_r,  ide_drv_head_r,    ide_status_r
};

void (*z80_ports_ide_w[8])(uint16_t, uint8_t, struct z80_port_write *) =
{
 // 60-67
 /* 60 */  ide_data_w,      ide_error_w,     ide_sectorcount_w, ide_sector_w,
 /* 64 */  ide_cyl_low_w,   ide_cyl_high_w,  ide_drv_head_w,    ide_cmd_w
};

//==============================================================================
// Set a Z80 read port.
//
// This function is intended for setting read ports external to this module
// except for z80ex_api.c which requires direct access.
//
//   pass: int port                     port number to set
//         void *handler                handler function
// return: void
//==============================================================================
void z80_port_rset (int port, void *handler)
{
 z80_ports_r[port] = handler;
}

//==============================================================================
// Set a Z80 write port.
//
// This function is intended for setting write ports external to this module
// except for z80ex_api.c which requires direct access.
//
//   pass: int port                     port number to set
//         void *handler                handler function
// return: void
//==============================================================================
void z80_port_wset (int port, void *handler)
{
 z80_ports_w[port] = handler;
}

//==============================================================================
// Z80 ports set.
//
// Set ports based on the model being emulated and request flags. The idea
// here is that ALL ports to be installed are based on the value of the
// 'modelx' structure members and request flags.  The 'modelx' values
// determines what each model being emulated needs.
//
// The 'ports' request flags provides another level to determine whether a
// port is installed.  It's purpose is to allow flexibilty of the emulated
// system.  It is possible to substitute or re-map port usage using the
// --expansion-port option.  The main purpose for this is to allow external
// hardware (or internal board hacks) to be used.  The external hardware may
// be official Microbee or 3rd party devices modules.
//
// Standard models where the port number is 0x00-0x0f are duplicated at
// ports 0x10-0x1f.  Alpha+ models DO NOT duplicate these ports and ports
// 0x10-0x1f are used for other purposes.  Standard model software that uses
// the duplicated ports will not work on the Alpha+ series of models
// (Premium/256TC/Teleterm).
//
//   pass: int ports                    flag bits of ports to be set
// return: void
//==============================================================================
void z80_ports_set (int ports)
{
 int i;
 
 //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // Set all ports to unhandled ones for now (clears expansion-port settings)
 if (ports == Z80_PORTS_ALL || ports == Z80_PORTS_NONE)
    {
     for (i = 0; i < 256; i++)
        {
         z80_ports_r[i] = z80_unhandled_r;
         z80_ports_w[i] = z80_unhandled_w;
        }
    }

 //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // Set up ports for uBee512 function support.
 if (ports & Z80_PORTS_UBEE512)
    {
     z80_ports_r[0xFF] = function_ubee_r;
     z80_ports_w[0xFF] = function_ubee_w;
    }

 //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // Set up ports common to all Microbee models.

 // Z80 PIO A
 if (ports & Z80_PORTS_PIOA)
    {
     z80_ports_r[0x00] = pio_r;
     z80_ports_r[0x01] = pio_r;
     z80_ports_w[0x00] = pio_w;
     z80_ports_w[0x01] = pio_w;
    }

 // Z80 PIO B
 if (ports & Z80_PORTS_PIOB)
    {
     z80_ports_r[0x02] = pio_r;
     z80_ports_r[0x03] = pio_r;
     z80_ports_w[0x02] = pio_w;
     z80_ports_w[0x03] = pio_w;
    }

 // Character ROM latch
 if (ports & Z80_PORTS_ROMLATCH)
    z80_ports_w[0x0B] = vdu_latchrom_w;

 // CRTC 6545/6845
 if (ports & Z80_PORTS_CRTC)
    {
     z80_ports_r[0x0C] = crtc_status_r;
     z80_ports_r[0x0D] = crtc_data_r;
     z80_ports_r[0x0E] = crtc_status_r;
     z80_ports_r[0x0F] = crtc_data_r;

     z80_ports_w[0x0C] = crtc_address_w;
     z80_ports_w[0x0D] = crtc_data_w;
     z80_ports_w[0x0E] = crtc_address_w;
     z80_ports_w[0x0F] = crtc_data_w;
    }

 //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // Set up ports common to various Microbee models depending on 'modelx' and
 // other variables.

 // if RTC emulated
 if (modelx.rtc && (ports & Z80_PORTS_RTC))
    {
     z80_ports_r[0x04] = rtc_r;
     z80_ports_r[0x07] = rtc_r;

     z80_ports_w[0x04] = rtc_w;
     z80_ports_w[0x06] = rtc_w;
     z80_ports_w[0x07] = rtc_w;
    }

 // if colour model emulation
 if (modelx.colour && (ports & Z80_PORTS_COLOUR))
    {
     z80_ports_r[0x08] = vdu_colcont_r;
     z80_ports_w[0x08] = vdu_colcont_w;
    }

 // if CPU clock speed change
 if (modelx.speed && (ports & Z80_PORTS_CPUCLOCK))
    z80_ports_r[0x09] = clock_r;

 // colour wait-off
 if (ports & Z80_PORTS_COLWOFF)
    z80_ports_w[0x09] = vdu_colwait_w;

 // if Pak and Net selection
 if (modelx.rom && (ports & Z80_PORTS_PAKNET))
    {
     z80_ports_r[0x0A] = roms_nsel_r;
     z80_ports_w[0x0A] = roms_psel_w;
    }

 // if emulating a FDC
 if ((emu.hardware & HW_WD2793) && (! modelx.rom) && (ports & Z80_PORTS_FDC))
    {
     memcpy(z80_ports_r + 0x40, z80_ports_fdc_r, sizeof(z80_ports_fdc_r));
     memcpy(z80_ports_w + 0x40, z80_ports_fdc_w, sizeof(z80_ports_fdc_w));
    }

 // if emulating a DRAM model set the memory mapping port
 if (modelx.ram >= 64 && (ports & Z80_PORTS_MEMMAP))
    memcpy(z80_ports_w + 0x50, z80_ports_mode1_w, sizeof(z80_ports_mode1_w));

 // if emulating the Compact Flash CB
 if ((emu.model == MOD_SCF || emu.model == MOD_PCF) &&
 (ports & Z80_PORTS_CFCB))
    z80_ports_w[0x51] = memmap_mode2_w;

 // if emulating an IDE HDD
 if (modelx.ide && (ports & Z80_PORTS_IDE))
    {
     memcpy(z80_ports_r + 0x60, z80_ports_ide_r, sizeof(z80_ports_ide_r));
     memcpy(z80_ports_w + 0x60, z80_ports_ide_w, sizeof(z80_ports_ide_w));
     z80_ports_w[0x70] = ide_dsr_w;
    }

 // if a DRAM model (SCC)
 if (modelx.ram >= 64 && (ports & Z80_PORTS_SCC)) 
    {
     z80_ports_r[0x68] = scc_r;
     z80_ports_r[0x69] = scc_r;
     z80_ports_w[0x68] = scc_w;
     z80_ports_w[0x69] = scc_w;
    }

 //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // Set up ports for Alpha+ models
 if (modelx.alphap)
    {
     if (modelx.sn76489an && (ports & Z80_PORTS_SN76489AN))
	{
	 // TI sn76489an sound IC
	 z80_ports_r[0x10] = sn76489an_r;
	 z80_ports_r[0x11] = sn76489an_r;
	 z80_ports_r[0x12] = sn76489an_r;
	 z80_ports_r[0x13] = sn76489an_r;

	 z80_ports_w[0x10] = sn76489an_w;
	 z80_ports_w[0x11] = sn76489an_w;
	 z80_ports_w[0x12] = sn76489an_w;
	 z80_ports_w[0x13] = sn76489an_w;
	}

     // Telecomputer or Teleterm keys
     if (modelx.tckeys && (ports & Z80_PORTS_TCKEYS))
        {
         z80_ports_r[0x18] = keytc_r;
         z80_ports_r[0x19] = keytc_r;
         z80_ports_r[0x1A] = keytc_r;
         z80_ports_r[0x1B] = keytc_r;

         z80_ports_w[0x18] = keytc_w;
         z80_ports_w[0x19] = keytc_w;
         z80_ports_w[0x1A] = keytc_w;
         z80_ports_w[0x1B] = keytc_w;
        }

     // LV data;
     if (ports & Z80_PORTS_LVDAT)
        {
         z80_ports_r[0x1C] = vdu_lvdat_r;
         z80_ports_r[0x1D] = vdu_lvdat_r;
         z80_ports_r[0x1E] = vdu_lvdat_r;
         z80_ports_r[0x1F] = vdu_lvdat_r;

         z80_ports_w[0x1C] = vdu_lvdat_w;
         z80_ports_w[0x1D] = vdu_lvdat_w;
         z80_ports_w[0x1E] = vdu_lvdat_w;
         z80_ports_w[0x1F] = vdu_lvdat_w;
        }
    }

 //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // Set up ports for Standard models. Adds the duplicatated ports (port + 0x10).
 else
    {
     // Z80 PIO A
     if (ports & Z80_PORTS_PIOA)
        {
         z80_ports_r[0x10] = pio_r;
         z80_ports_r[0x11] = pio_r;
         z80_ports_w[0x10] = pio_w;
         z80_ports_w[0x11] = pio_w;
        }

     // Z80 PIO B
     if (ports & Z80_PORTS_PIOB)
        {
         z80_ports_r[0x12] = pio_r;
         z80_ports_r[0x13] = pio_r;
         z80_ports_w[0x12] = pio_w;
         z80_ports_w[0x13] = pio_w;
        }

     // RTC
     if (modelx.rtc && (ports & Z80_PORTS_RTC))
        {
         z80_ports_r[0x14] = rtc_r;
         z80_ports_r[0x17] = rtc_r;

         z80_ports_w[0x14] = rtc_w;
         z80_ports_w[0x16] = rtc_w;
         z80_ports_w[0x17] = rtc_w;
        }

     // if colour model emulation
     if (modelx.colour && (ports & Z80_PORTS_COLOUR))
        {
         z80_ports_r[0x18] = vdu_colcont_r;
         z80_ports_w[0x18] = vdu_colcont_w;
        }

     // CPU clock speed
     if (modelx.speed && (ports & Z80_PORTS_CPUCLOCK))
        z80_ports_r[0x19] = clock_r;

     // colour wait-off
     if (ports & Z80_PORTS_COLWOFF)
        z80_ports_w[0x19] = vdu_colwait_w;

     // Pak and Net selection
     if (modelx.rom && (ports & Z80_PORTS_PAKNET))
        {
         z80_ports_r[0x1A] = roms_nsel_r;
         z80_ports_w[0x1A] = roms_psel_w;
        }

     // Character ROM latch
     if (ports & Z80_PORTS_ROMLATCH)
         z80_ports_w[0x1B] = vdu_latchrom_w;

     // CRTC 6545/6845
     if (ports & Z80_PORTS_CRTC)
        {
         z80_ports_r[0x1C] = crtc_status_r;
         z80_ports_r[0x1D] = crtc_data_r;
         z80_ports_r[0x1E] = crtc_status_r;
         z80_ports_r[0x1F] = crtc_data_r;

         z80_ports_w[0x1C] = crtc_address_w;
         z80_ports_w[0x1D] = crtc_data_w;
         z80_ports_w[0x1E] = crtc_address_w;
         z80_ports_w[0x1F] = crtc_data_w;
        }
    }
}

//==============================================================================
// Z80 Initilisation.
//
//   pass: int ports                    flag bits of ports to be set
// return: int                          0
//==============================================================================
int z80_init (void)
{
 z80_ports_set(Z80_PORTS_ALL);

 return z80api_init();
}

//==============================================================================
// Z80 De-Initilization
//
//   pass: void
// return: int                          0
//==============================================================================
int z80_deinit (void)
{
 return z80api_deinit();
}

//==============================================================================
// Z80 Reset
//
//   pass: void
// return: int                          0
//==============================================================================
int z80_reset (void)
{
 return z80api_reset();
}

//==============================================================================
// Initilise Pak and Net ports for CF model.
//
// These 2 ports are not enabled when port 0x51 bit 7 is equal to 0 so must
// be changed on the fly.
//
//   pass: void
// return: void
//==============================================================================
void z80_cf_ports (void)
{
 if (emu.port51h & BANK_CF_PC85)
    {
     modelx.rom = 1;
     z80_ports_r[0x0A] = roms_nsel_r;
     z80_ports_w[0x0A] = roms_psel_w;
     if (emu.model == MOD_SCF)
        {
         z80_ports_r[0x1A] = roms_nsel_r;
         z80_ports_w[0x1A] = roms_psel_w;
        }
    }
 else
    {
     modelx.rom = 0;
     z80_ports_r[0x0A] = z80_unhandled_r;
     z80_ports_w[0x0A] = z80_unhandled_w;
     if (emu.model == MOD_SCF)
        {
         z80_ports_r[0x1A] = z80_unhandled_r;
         z80_ports_w[0x1A] = z80_unhandled_w;
        }
    }
}

//==============================================================================
// Initialise port 0x40-x47 with WD2793 FDC or WD1002-5 (HDD) handlers
// depending on whether port 0x58 is emulated (3rd party addition).
//
// If port 0x58 is emulated then it's value determines whether the floppy
// drive controller is associated with the WD1002-5 or the core board.
//
//   pass: void
// return: void
//==============================================================================
void z80_hdd_ports (void)
{
 // if it's a standard Microbee HDD model then port 0x58 does not exist in
 // which case the WD1002-5 is used for floppy access.
 if (! emu.port58h_use)
    {
     memcpy(z80_ports_r + 0x40, z80_ports_hdd_r, sizeof(z80_ports_hdd_r));
     memcpy(z80_ports_w + 0x40, z80_ports_hdd_w, sizeof(z80_ports_hdd_w));

     return;
    }

 if (emu.port58h)
    {
     memcpy(z80_ports_r + 0x40, z80_ports_hdd_r, sizeof(z80_ports_hdd_r));
     memcpy(z80_ports_w + 0x40, z80_ports_hdd_w, sizeof(z80_ports_hdd_w));
    }
 else
    {
     memcpy(z80_ports_r + 0x40, z80_ports_fdc_r, sizeof(z80_ports_fdc_r));
     memcpy(z80_ports_w + 0x40, z80_ports_fdc_w, sizeof(z80_ports_fdc_w));
    }
}

//==============================================================================
// Initialise port 0x58 for modified HDD Microbees where it is used to
// associate ports 0x40-0x47 to the WD1002-5 or Coreboard WD2793 controller if
// the WD1002-5 (HDD) is being used.
//
// This port is used to select ports 0x40-0x47 assignmnts to the FDC or HDD
// (WD1002-5 card).
//
//   pass: void
// return: void
//==============================================================================
void z80_set_port_58h (void)
{
 z80_ports_w[0x58] = hdd_fdc_select_w;
}

//==============================================================================
// Z80 Unhandled read port
//
// Not reported normally as this may be deliberate.  Reporting may affect the
// emulation speed of the program.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t                     0
//==============================================================================
uint16_t z80_unhandled_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.z80)
    log_port_0("z80_unhandled_r", port);
 return 0;
}

//==============================================================================
// Z80 Unhandled write port
//
// Not reported normally as this may be deliberate.  Reporting may affect the
// emulation speed of the program.
//
// Applications                         port (hex)
// Astoroids Plus (tape conv to disk)   FD
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void z80_unhandled_w (uint16_t port, uint8_t data,
                      struct z80_port_write *port_s)
{
 if (modio.z80)
    log_port_1("z80_unhandled_w", "data", port & 0xff, data);
}
