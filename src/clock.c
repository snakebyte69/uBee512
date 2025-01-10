//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                                clock module                                *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Change CPU clock speed.
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
// v4.6.0 - 27 April 2010, uBee
// - Changes made to clock_r() to use the 16 bit port value as was
//   originally intended and to only return 0.
//
// v4.1.0 - 25 June 2009, uBee
// - Minor code change.
//
// v4.0.0 - 19 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
//
// v3.1.0 - 2 November 2008, uBee
// - Changed all printf() calls to use a local xprintf() function.
//
// v2.7.0 - 28 May 2008, uBee
// - Modified set_clock_speed() function call.
// - Added structure emu_t and moved variables into it.
//
// v2.3.0 - 6 January 2008, uBee
// - Added modio_t structure.
//
// v2.1.0 - 19 October 2007, uBee
// - Created a new file and implement the CPU clock speed change code.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "ubee512.h"
#include "clock.h"
#include "z80api.h"
#include "z80.h"
#include "support.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
static int clock_changed;

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;

//==============================================================================
// Clock Initialise.
//
//   pass: void
// return: int                          0
//==============================================================================
int clock_init (void)
{
 return 0;
}

//==============================================================================
// Clock  de-initialise
//
//   pass: void
// return: int                          0
//==============================================================================
int clock_deinit (void)
{
 return 0;
}

//==============================================================================
// Clock reset
//
// Set the clock speed back to normal
//
//   pass: void
// return: int                          0
//==============================================================================
int clock_reset (void)
{
 if ((modelx.speed) && (clock_changed))
    {
     set_clock_speed(3.375, 0, 0);
     clock_changed = 0;
    }
 return 0;
}

//==============================================================================
// Clock speed change - Port function
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t                     0
//==============================================================================
uint16_t clock_r (uint16_t port, struct z80_port_read *port_s)
{
 int clock;

 if (! modelx.speed)
    return 0;

 clock = (port >> 8) & B8(00000011);

 if (modio.clock)
    log_port_16("clock_r", "clock", port, clock);

 // only change the speed if it needs it.
 if ((emu.cpuclock == 3375000) && (clock == B8(00000010)))
    {
     set_clock_speed(6.750, 0, 0);
     clock_changed = 1;
    }
 else
    {
     if ((emu.cpuclock == 6750000) && (clock == B8(00000000)))
        {
         set_clock_speed(3.375, 0, 0);
         clock_changed = 1;
        }
    }

 return 0;
}
