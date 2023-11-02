//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                       Copyright (C) 2007-2016 uBee                         *
//*                                                                            *
//*                             BeeThoven module                               *
//*                Copyright (C) 2009-2010 Kalvis Duckmanton                   *
//*                                                                            *
//******************************************************************************
//
// This module emulates the Microbee BeeThoven peripheral.  This device is based
// on a General Instruments AY-3-8910 Programmable Sound Generator chip.
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
// - Do not call pio_a_strobe to cause a PIO interrupt when the beethoven is
//   read from or written to, as the real hardware doesn't do this
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
#include "ay38910.h"
#include "parint.h"
#include "beethoven.h"
#include "z80api.h"
#include "pio.h"

//==============================================================================
// constants
//==============================================================================
/* The BeeThoven is clocked with a 1.0MHz signal. */
#define BEETHOVEN_CLOCK         1000000L
#define BEETHOVEN_SAMPLE_RATE   (BEETHOVEN_CLOCK / PSG_CLOCK_DIVISOR)

#define NUM_BEETHOVEN_SAMPLES   (BEETHOVEN_SAMPLE_RATE * 5 / 1000)
#define BEETHOVEN_DECAY_CONSTANT 100 /* ms */

#define DEBUG_BEETHOVEN 0       /* set to 1 to debug sample generation */

//==============================================================================
// prototypes
//==============================================================================
int beethoven_reset (void);
int beethoven_init (void);
int beethoven_deinit (void);
void beethoven_strobe (void);
void beethoven_w (uint8_t data);
uint8_t beethoven_r(void);
void beethoven_ready(void);
void beethoven_drain_samples(audio_scratch_t *a, audio_circularbuf_t *cb);
void beethoven_init_rate_conversion(audio_circularbuf_t *cb, int dst_rate, int src_rate);
int beethoven_tick(audio_scratch_t *a, const void *data,
                   uint64_t start, uint64_t cycles);

//==============================================================================
// structures and variables
//==============================================================================
extern audio_t audio;

beethoven_t beethoven;

parint_ops_t beethoven_ops = {
 .init = &beethoven_init,
 .deinit = &beethoven_deinit,
 .reset = &beethoven_reset,
 .poll = NULL,
 .ready = &beethoven_ready,
 .strobe = NULL,                // Not used, see comments in beethoven_ready()
 .read = &beethoven_r,
 .write = &beethoven_w,
};

extern modio_t modio;
extern emu_t emu;

//==============================================================================
// Beethoven reset.
//
//   pass: void
// return: int                          0
//==============================================================================
int beethoven_reset (void)
{
 if (modio.beethoven)
    xprintf("Beethoven: reset\n");
 return 0;
}

//==============================================================================
// Beethoven Initialise.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int beethoven_init (void)
{
 if (modio.beethoven)
    xprintf("Beethoven: init\n");

 beethoven.addrsel = 1;         /* first write selects register
                                 * addresses */

 beethoven.ay_update_head = beethoven.ay_update_tail = NULL;

 if (psg_init(&beethoven.ay_3_8910))
    return -1;

 /* -------------------------------------------------------------------- */
 /*  Register this as a sound peripheral with the SND driver.            */
 /* -------------------------------------------------------------------- */
 if (audio_register(&beethoven.snd_buf,
                    "beethoven",
                    &beethoven_tick, (void *)&beethoven,
                    NULL,       /* sound pitch is independent of CPU speed */
                    1,          /* 1 -> synchronise with CPU thread */
                    0           /* holdoff time */
                    ))
    return -1;

 audio_circularbuf_set_rate_conversion(&beethoven.ay_3_8910.scratch,
                                       audio.frequency,
                                       BEETHOVEN_SAMPLE_RATE);

 audio_circularbuf_set_decay_constant(&beethoven.ay_3_8910.scratch,
                                      BEETHOVEN_DECAY_CONSTANT);

 return 0;                      /* success! */
}

//==============================================================================
// Beethoven de-initialise.
//
//   pass: void
// return: int                          0
//==============================================================================
int beethoven_deinit (void)
{
 if (modio.beethoven)
    xprintf("Beethoven: deinit\n");
 audio_deregister(&beethoven.snd_buf);
 psg_deinit(&beethoven.ay_3_8910);
 return 0;
}

//==============================================================================
// Beethoven write.
//
//   pass: int data
// return: void
//==============================================================================
void beethoven_w (uint8_t data)
{
 if (beethoven.addrsel)
    {
     beethoven.address = data;
    }
 else if (beethoven.address == PSG_IO_A ||
          beethoven.address == PSG_IO_B)
    {
     return;      /* quietly drop writes to the IO ports */
    }
 else
    {
     ay_update_le_t *p = malloc(sizeof(*p));

     p->address = beethoven.address;
     p->data = data;
     p->when = z80api_get_tstates();
     p->next = NULL;

     // Add the current register update to the AY register update
     // list
     if (!beethoven.ay_update_head)
        {
         beethoven.ay_update_head = beethoven.ay_update_tail = p;
        }
     else
        {
         beethoven.ay_update_tail->next = p;
         beethoven.ay_update_tail = p;
        }
    }
}

//==============================================================================
// Beethoven read.
//
//   pass: void
// return: uint8_t data
//==============================================================================
uint8_t beethoven_r (void)
{
 if (beethoven.addrsel)
    {
     return 0;          /* AY isn't selected for reading */
    }
 else
    {
     uint8_t r;

     // read from AY registers
     if (beethoven.address == PSG_IO_A ||
         beethoven.address == PSG_IO_B)
        r = 0xf9;  /* the two IO ports have bit 1 forced on? */
     else
        {
         r = psg_r(&beethoven.ay_3_8910, beethoven.address);
        }
     return r;
    }
}

//
// ARDY is connected to ASTB* on the beethoven board, and also to a
// flipflop which selects either the AY-3-8910 address register or the
// AY-3-8910 data register.
//
// In this case, when PIO port A is read from in input mode or written to
// in output mode, a short positive pulse is generated on ARDY, which is
// precisely one PIO clock period wide.  ARDY goes low after the next
// falling edge of the PIO clock.
//
// The data sheet suggests that the rising edge on ASTB* will generate
// an interrupt; however experiments and more careful study of the
// datasheet (particularly the timing diagram for input mode, mode 1),
// suggests that the rising edge on ASTB* appears to be a signal for the
// PIO to sample the ASTB* input a short while after the next falling edge
// of the PIO clock; if the input is high an interrupt is generated.
// Since ASTB* has gone low again (as it's tied to ARDY), there is no
// interrupt!
//
// So here we eschew calling beethoven_ops.strobe().
//
void beethoven_ready(void)
{
 beethoven.addrsel = !beethoven.addrsel; /* address flipflop */
}

//==============================================================================
// Beethoven tick function.  Registered as a callback function in
// beethoven_init() and called by audio_sources_update()
//
//   pass: audio_scratch_t
//         const void * (pointing to a beethoven_t structure containing the
//                       beethoven state)
//         uint64_t     (number of z80 tstates at the start of the frame)
//         uint64_t     (number of z80 tstates since the last time the tick
//                       function was called)
//      
// return: int          0 - sound source quiescent
//                      not 0 - sound source active
//==============================================================================
int beethoven_tick(audio_scratch_t *buf, const void *data,
                   uint64_t frame_start, uint64_t cycles)
{
 beethoven_t *b = (beethoven_t *)data;
 unsigned int ticks_per_sample = emu.cpuclock / BEETHOVEN_SAMPLE_RATE;
 unsigned int num_samples;

#if DEBUG_BEETHOVEN
 xprintf("beethoven: %9llu ticks, %9llu remainder",
         cycles, b->cycles_remainder);
#endif
 // add the leftover cycles from the last frame to the cycles for the
 // current frame, adjust the start time to be immediately after the
 // last beethoven sample generated.
 cycles += b->cycles_remainder;
 frame_start -= b->cycles_remainder;
 // Compute the number of Beethoven samples that would have been
 // generated during the last block of CPU instructions.
 num_samples = cycles / ticks_per_sample;
 // Adjust the number of samples to be no more than the number of
 // samples that can be generated during 1 frame time
 if (num_samples > BEETHOVEN_SAMPLE_RATE / emu.framerate)
     num_samples = BEETHOVEN_SAMPLE_RATE / emu.framerate;
 // and the number of leftover CPU cycles which need to count
 // towards the next block
 b->cycles_remainder = cycles - num_samples * ticks_per_sample;
#if DEBUG_BEETHOVEN
 xprintf(" gives %6u samples with %9llu remainder\n",
         num_samples, b->cycles_remainder);
#endif

 // now generate samples
 while (num_samples)
    {
     int samples_generated;
     int n;
     ay_update_le_t *ayup, *ayuq;

     audio_drain_samples(&b->snd_buf, &b->ay_3_8910.scratch);
     do {
      /* apply all register updates that are due. */
      for (ayup = b->ay_update_head;
           ayup && ayup->when <= frame_start;)
         {
          if (modio.beethoven)
             {
              xprintf("Beethoven: register update (z80 tstates %llu) r%02o = %02x\n",
                      ayup->when, ayup->address, ayup->data);
             }
          psg_w(&b->ay_3_8910, ayup->address, ayup->data);
          ayuq = ayup->next;
          free(ayup);
          ayup = ayuq;
         }
      b->ay_update_head = ayup;
      if (ayup == NULL)         /* list now empty */
         b->ay_update_tail = ayup;
      if (ayup == NULL)
         n = num_samples;       /* generate the requested number of
                                 * samples */
      else
         {
          /* ceil((when - frame_start) / ticks_per_sample) */
          n = (ayup->when - frame_start +
               ticks_per_sample - 1) / ticks_per_sample;
          if (n > num_samples)
             n = num_samples;   /* generate no more than the number of
                                 * samples originally requested */
         }
      samples_generated = psg_iterate(&b->ay_3_8910, n);
      if (samples_generated <= 0)
         ;                      /* the buffer needs draining */
      else
         {
          frame_start += samples_generated * ticks_per_sample;
          num_samples -= samples_generated;
         }
     } while (samples_generated > 0);
    }

 return 1;                      /* beethoven always generates
                                 * output */
}

