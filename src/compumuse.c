//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                       Copyright (C) 2007-2016 uBee                         *
//*                                                                            *
//*                             Compumuse module                               *
//*                   Copyright (C) 2011 Kalvis Duckmanton                     *
//*                                                                            *
//******************************************************************************
//
// This module emulates the EA Compumuse, described in the August, 1983
// issue.  This device is based on the TI 76489 sound synthesiser IC.
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
// v5.2.0 - 5 February 2011, K Duckmanton
// - Initial implementation
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <SDL.h>
#include <SDL_thread.h>

#include "ubee512.h"
#include "audio.h"
#include "sn76489an_core.h"
#include "parint.h"
#include "compumuse.h"
#include "z80api.h"
#include "pio.h"

//==============================================================================
// constants
//==============================================================================
/* Builders of the Compumuse had the option of setting the SN76489
 * clock frequency to 4.0MHz, 2.0MHz or 1.0MHz.  For this module we
 * default to 2.0MHz */
#define COMPUMUSE_CLOCK                 2000000L
#define COMPUMUSE_PROCESSING_TIME       32

//==============================================================================
// prototypes
//==============================================================================
int compumuse_deinit (void);
int compumuse_init (void);
int compumuse_reset (void);
uint8_t compumuse_r (void);
void compumuse_poll (void);
void compumuse_ready (void);
void compumuse_strobe (void);
void compumuse_w (uint8_t data);
void compumuse_clock (int cpuclock);

//==============================================================================
// structures and variables
//==============================================================================
extern audio_t audio;

compumuse_t compumuse = {
 .clock = COMPUMUSE_CLOCK,
};

parint_ops_t compumuse_ops = {
 .init = &compumuse_init,
 .deinit = &compumuse_deinit,
 .reset = &compumuse_reset,
 .poll = &compumuse_poll,
 .ready = &compumuse_ready,
 .strobe = &pio_porta_strobe,
 .read = NULL,                  // output-only peripheral
 .write = &compumuse_w,
};

extern modio_t modio;
extern emu_t emu;

//==============================================================================
// Compumuse reset.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int compumuse_reset (void)
{
 sn76489an_t *s = &compumuse.sn76489;

 if (modio.compumuse)
    xprintf("Compumuse: reset\n");
 return sn76489an_core_reset(s);
}

//==============================================================================
// Compumuse Initialise.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int compumuse_init (void)
{
 sn76489an_t *s = &compumuse.sn76489;

 if (modio.compumuse)
    xprintf("compumuse: init\n");
 compumuse.busy = 0;
 compumuse.strobe_due = 0;
 return
     sn76489an_core_init(s, "compumuse",
                         &compumuse_clock,
                         compumuse.clock, /* initial clock frequency */
                         compumuse.init   /* silence compumuse at startup */
         );
}

//==============================================================================
// Compumuse de-initialise.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int compumuse_deinit (void)
{
 sn76489an_t *s = &compumuse.sn76489;
 if (modio.compumuse)
    xprintf("Compumuse: deinit\n");
 return sn76489an_core_deinit(s);
}

//==============================================================================
// Compumuse clock.
//
// The Compumuse clock frequency is fixed and independent of the CPU clock.
//
//   pass: int clock
// return: void
//==============================================================================
void compumuse_clock (int clock)
{
 sn76489an_t *s = &compumuse.sn76489;

 if (emu.runmode)
    sn76489an_core_clock(s, clock);
 compumuse.clock = clock;
}

//==============================================================================
// Compumuse write.
//
//   pass: int data
// return: void
//==============================================================================
void compumuse_w (uint8_t data)
{
 sn76489an_t *s = &compumuse.sn76489;

 if (modio.compumuse)
    log_port_1("compumuse_w", "data", 0, data);
 sn76489an_core_w(s, 0, data);
}

//==============================================================================
/*
 * The CompuMuse connects the STB* output from the host computer to
 * the CE* input on the SN76489 via a pair of NAND gates.  STB* is
 * expected to go low when data is to be written into the sound
 * generator.  For the Microbee, since STB* needs to be generated from
 * ARDY, and ARDY goes high when data has been written to the output
 * port, an additional inverter is needed.
 *
 * Once CE* goes low, the SN76489 pulls its READY output low to signal
 * that a transfer is in progress.  To write data into a SN76489
 * register, the WE* input must be pulled low also; this is achieved
 * by tying READY to WE*.
 *
 * After 32 clock cycles or so the data is loaded into the SN76489 and
 * READY is released at this point. (I suspect that the actual number
 * depends on the timing of the clock output from the SN76489's
 * internal divide-by-32 stage relative to the CE* input).  READY is
 * connected to the READY* input on the host computer.  For the
 * Microbee, READY* is simply ARDY*, and the rising edge on this
 * signal generates an interrupt if interrupts are enabled.
 *
 * For the purposes of emulation, we'll introduce a fixed 32 clock
 * tick delay.
 */
//   pass: void
// return: void
//==============================================================================
void compumuse_ready (void)
{
 if (compumuse.busy)
    return;                     /* new data was written before the
                                 * previous data was acknowledged. */
 compumuse.busy = 1;
 compumuse.strobe_due = z80api_get_tstates() + COMPUMUSE_PROCESSING_TIME;
}

//==============================================================================
// Compumuse polling.
//
//   pass: void
// return: void
//==============================================================================
void compumuse_poll (void)
{
 if (compumuse.strobe_due != 0 &&
     z80api_get_tstates() > compumuse.strobe_due)
    {
     compumuse_strobe();
     compumuse.strobe_due = 0;
    }
}

//==============================================================================
// Compumuse strobe.
//
//   pass: void
// return: void
//==============================================================================
void compumuse_strobe (void)
{
 compumuse.busy = 0;
 (*compumuse_ops.strobe)();
}
