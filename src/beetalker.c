//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                       Copyright (C) 2007-2016 uBee                         *
//*                                                                            *
//*                             BeeTalker module                               *
//*                Copyright (C) 2009-2010 Kalvis Duckmanton                   *
//*                                                                            *
//******************************************************************************
//
// This module emulates the Microbee BeeTalker peripheral.  This device is based
// on a General Instruments SP0256-AL2 speech synthesiser chip.
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
// v5.0.0 - 13 July 2010, K Duckmanton
// - Removed all references to the 'sound' global variable and replaced them
//   with references to the 'audio' global instead.
//
// v4.7.0 - 17 June 2010, K Duckmanton
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
#include "sp0256.h"
#include "parint.h"
#include "beetalker.h"
#include "z80api.h"
#include "pio.h"

//==============================================================================
// constants
//==============================================================================
/* The BeeTalker's SP0256 is clocked at 3.120MHz, which is the
 * suggested clock frequency given on the GI/MicroChip datasheets.  A
 * careful reading of the SP0256 and SP0250 data sheets suggests that
 * new samples are generated at 1/312th of the CPU clock rate, which
 * for the suggested 3.120MHz clock works out to be 10KHz exactly. */


#define BEETALKER_CLOCK         3120000L
#define BEETALKER_SAMPLE_RATE   (BEETALKER_CLOCK / SP0256_CLOCK_DIVISOR)

#define NUM_BEETALKER_SAMPLES   (BEETALKER_SAMPLE_RATE * 5 / 1000)
//==============================================================================
// prototypes
//==============================================================================
int beetalker_reset (void);
int beetalker_init (void);
int beetalker_deinit (void);
void beetalker_strobe (void);
void beetalker_w (uint8_t data);
uint8_t beetalker_r(void);
void beetalker_ready(void);
int beetalker_worker(void *data);

//==============================================================================
// structures and variables
//==============================================================================
extern audio_t audio;

beetalker_t beetalker;

parint_ops_t beetalker_ops = {
 .init = &beetalker_init,
 .deinit = &beetalker_deinit,
 .reset = &beetalker_reset,
 .poll = NULL,
 .ready = &beetalker_ready,
 .strobe = &pio_porta_strobe,
 .read = NULL,
 .write = &beetalker_w,
};

extern modio_t modio;

//==============================================================================
// Beetalker reset.
//
//   pass: void
// return: int                          0
//==============================================================================
int beetalker_reset (void)
{
 if (modio.beetalker)
    xprintf("Beetalker: reset\n");
 return 0;                      /* does nothing significant */
}

//==============================================================================
// BeeTalker Initialise.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int beetalker_init (void)
{
 if (modio.beetalker)
    xprintf("Beetalker: init\n");

 if (sp0256_init(&beetalker.sp0256))
    return -1;

 audio_circularbuf_set_rate_conversion(&beetalker.sp0256.scratch,
                                       audio.frequency,
                                       BEETALKER_SAMPLE_RATE);

 audio_circularbuf_set_decay_constant(&beetalker.sp0256.scratch,
                                      0);

 /* -------------------------------------------------------------------- */
 /*  Register this as a sound peripheral with the SND driver.            */
 /* -------------------------------------------------------------------- */
 if (audio_register(&beetalker.snd_buf,
                    "beetalker",
                    NULL, NULL,
                    NULL,       /* sound pitch is independent of CPU speed */
                    0, 0))
    return -1;

 /* -------------------------------------------------------------------- */
 /*  Fire off a worker thread to continuously generate samples.          */
 /* -------------------------------------------------------------------- */
 beetalker.sp0256_mutex = SDL_CreateMutex();
 beetalker.workerthread = SDL_CreateThread(beetalker_worker, NULL);
 if (!beetalker.workerthread)
    return -1;

 return 0;                      /* success! */
}

//==============================================================================
// BeeTalker de-initialise.
//
//   pass: void
// return: int                          0
//==============================================================================
int beetalker_deinit (void)
{
 if (modio.beetalker)
    xprintf("Beetalker: deinit\n");
 if (beetalker.workerthread)
    {
     int status;

     beetalker.terminate = 1;
     SDL_WaitThread(beetalker.workerthread, &status);
    }
 SDL_DestroyMutex(beetalker.sp0256_mutex);
 audio_deregister(&beetalker.snd_buf);
 sp0256_deinit(&beetalker.sp0256);
 return 0;
}

//==============================================================================
// Beetalker strobe.  Called from the poll function when the SP0256 halts.
//
//   pass: void
// return: void
//==============================================================================
void beetalker_strobe (void)
{
 if (modio.beetalker)
    xprintf("Beetalker: strobe\n");
 (*beetalker_ops.strobe)();
}

//==============================================================================
// Beetalker write.
//
//   pass: int data
// return: void
//==============================================================================
void beetalker_w (uint8_t data)
{
 data &= 0x3f;          /* top 2 bits are ignored. */
 if (modio.beetalker)
    xprintf("Beetalker: write %02x\n", data);
 beetalker.data = data;
}

void beetalker_ready(void)
{
 if (modio.beetalker)
    xprintf("Beetalker: ready\n");
 if (beetalker.sp0256.lrq == 0)
    return;                     /* new data has been written before
                                 * the previous data was acknowledged */
 SDL_LockMutex(beetalker.sp0256_mutex);
 sp0256_ald(&beetalker.sp0256, beetalker.data);
 SDL_UnlockMutex(beetalker.sp0256_mutex);
}

//==============================================================================
// Beetalker worker thread.  Continuously runs the sp0256 core generating
// samples for the sound thread to pick up.
//
//   pass: int data
// return: void
//==============================================================================
int beetalker_worker(void *data)
{
 int samples;

 beetalker.terminate = 0;

 /* -------------------------------------------------------------------- */
 /*  Iterate the sound engine.                                           */
 /* -------------------------------------------------------------------- */
 while (!beetalker.terminate)
    {
     audio_drain_samples(&beetalker.snd_buf, &beetalker.sp0256.scratch);

     /* ---------------------------------------------------------------- */
     /*   For each iteration of the loop, try to generate 5ms
      *   worth of samples.
      */
     samples = NUM_BEETALKER_SAMPLES;

     /* ---------------------------------------------------------------- */
     /*  Process the current set of filter coefficients as long as the   */
     /*  repeat count holds up and we have room in our circular buffer.  */
     /* ---------------------------------------------------------------- */
     while (samples > 0)
        {
         int r;

         SDL_LockMutex(beetalker.sp0256_mutex);
         r = sp0256_iterate(&beetalker.sp0256, samples);
         SDL_UnlockMutex(beetalker.sp0256_mutex);
         if (r == -2)
            {
             // the speech processor can accept more data.
             beetalker_strobe();
            }
         else if (r == -1)
            break;              /* circular buffer needs draining */
         samples -= r;
        }
    }

 return 0;                      /* successful termination */
}
