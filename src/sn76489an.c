//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                       Copyright (C) 2007-2016 uBee                         *
//*                                                                            *
//*                        SN76489AN emulation module                          *
//*                   Copyright (C) 2010 Kalvis Duckmanton                     *
//*                                                                            *
//******************************************************************************
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
// v5.2.0 - 19 February 2011, K Duckmanton
// - Initial implementation
//==============================================================================

static const char
   rcs_id[]="$Id: sn76489an.c,v 1.1.1.1 2011/03/27 06:04:43 krd Exp $";

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "ubee512.h"
#include "z80api.h"
#include "audio.h"
#include "function.h"
#include "sn76489an.h"
#include "sn76489an_core.h"

#define SN76489AN_CLOCK         emu.cpuclock

//==============================================================================
// structures and variables
//==============================================================================
sn76489an_t snd;
extern modio_t modio;
extern model_t modelx;
extern audio_t audio;
extern emu_t emu;

//==============================================================================
// function prototypes
//==============================================================================
void sn76489an_clock (int cpuclock);

//==============================================================================
// sn76489an Initialise.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int sn76489an_init (void)
{
 sn76489an_t *s = &snd;

 if (!modelx.sn76489an)
     return 0;                  /* If the 76489an isn't being
                                 * emulated, return success */
 if (modio.sn76489an)
    xprintf("76489an: init\n");
 return
    sn76489an_core_init(s, "sn76489an",
                        &sn76489an_clock, /* The 76489 is clocked
                                           * from the CPU clock */
                        SN76489AN_CLOCK, /* initial clock frequency */
                        (modelx.sn76489an == 2) /* silence at startup */
        );
}

//==============================================================================
// sn76489an de-initialise.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int sn76489an_deinit (void)
{
 sn76489an_t *s = &snd;

 if (!modelx.sn76489an)
     return 0;                  /* If the 76489an isn't being
                                 * emulated, return success */
 if (modio.sn76489an)
    xprintf("sn76489an: deinit\n");
 return sn76489an_core_deinit(s);
}

//==============================================================================
// sn76489an reset.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int sn76489an_reset (void)
{
 sn76489an_t *s = &snd;

 if (modio.sn76489an)
    xprintf("sn76489an: reset\n");
 return sn76489an_core_reset(s);
}

//==============================================================================
// Set the sample rate conversion factor based on the current CPU
// clock and the current output sample frequency.
//
//   pass: int cpuclock             CPU clock speed in Hz
//
//   Globals used:
//         audio.mode               Set to AUDIO_PROPORTIONAL to keep the sound
//                                  pitch proportional to the CPU speed
//==============================================================================
void sn76489an_clock (int cpuclock)
{
 sn76489an_t *s = &snd;

 if (audio.mode != AUDIO_PROPORTIONAL)
    cpuclock = 3375000;
 sn76489an_core_clock(s, cpuclock);
}

//==============================================================================
// sn76489an read - Port function
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t             register data
//==============================================================================
uint16_t sn76489an_r (uint16_t port, struct z80_port_read *port_s)
{
 sn76489an_t *s = &snd;

 if (modio.sn76489an)
    log_port_1("sn76489an", "data", port, 0);
 return sn76489an_core_r(s, port);
}

//==============================================================================
// sn76489an write - Port function
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void sn76489an_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 sn76489an_t *s = &snd;

 if (modio.sn76489an)
    log_port_1("sn76489an_w", "data", port, data);

 sn76489an_core_w(s, port, data);
}
