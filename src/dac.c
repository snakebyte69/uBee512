//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                DAC parallel port audio peripheral module                   *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// This module is used to emulate an audio DAC device connected to the
// parallel port.
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
// v5.0.0 - 6 August 2010, uBee
// - Start with the latest sound.c module and modify the speaker_* code for
//   DAC usage.  Code here uses the sound buffer management functions
//   defined in audio.c.
//==============================================================================

#ifdef MINGW
#include <windows.h>
#else
#include <sys/stat.h>
#include <signal.h>             // signal name macros, and the signal() prototype
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <SDL.h>

#include "parint.h"
#include "dac.h"
#include "ubee512.h"
#include "audio.h"
#include "z80api.h"
#include "support.h"
#include "gui.h"

//==============================================================================
// constants
//==============================================================================
#define DEBUG_DAC 0  // set to 1 to debug the operation of
                     // the microbee dac driver

//==============================================================================
// structures and variables
//==============================================================================

int dac_tick (audio_scratch_t *buf, const void *data,
              uint64_t start, uint64_t cycles);
void dac_clock (int cpuclock);
void dac_w (uint8_t data);

parint_ops_t dac_ops =
{
 .init = &dac_init,
 .deinit = &dac_deinit,
 .reset = &dac_reset,
 .poll = NULL,
 .ready = NULL,    // &dac_ready,
 .strobe = NULL,   // &pio_porta_strobe,
 .read = NULL,
 .write = &dac_w
};

extern audio_t audio;
extern modio_t modio;
extern emu_t emu;

#define DAC_HOLDOFF_TIME 50     /* ms */
#define DAC_IDLE_TIME 1000      /* ms */
#define DAC_DECAY_CONSTANT 50   /* ms */

#define DAC_AMPLITUDE (AUDIO_MAXVAL / 3)

typedef struct dac_t {
   audio_scratch_t snd_buf;
   int samples_since_write;     // counts samples since the dac
                                // port was last written to
   uint8_t state;               // current state of the dac
                                // output
   int idle;                    // set if the dac hasn't changed state
                                // during the last video frame
   uint64_t change_tstates;
   int samplenumber;
   int fraction;                // position of dac transition
                                // within a sample, used to
                                // interpolate the final value
   int last_sample;             // partial sample under construction
   int div_num, div_denom;      // numerator and denominator of the
                                // tstates -> samples conversion factor
   int idle_count;              // number of idle frames before this source
                                // stops generating samples
   int count;                   // counter
   int tau, decay;
} dac_t;

dac_t dac;


// this macro, given a time in CPU clocks, returns the number of clocks after
// the start of the current sample.  The numerator and denominator of the
// cpu clock -> sample clock conversion fraction are in S (a dac_t
// structure)
#define SAMPLE_TIME_FRACTION(S, TSTATES)                        \
   ((TSTATES) * (S)->div_denom % (S)->div_num / (S)->div_denom)

#define SAMPLE_TIME_FRACTION_REMAINING(S, TSTATES)                      \
   (((S)->div_num - (TSTATES) * (S)->div_denom % (S)->div_num) /        \
    (S)->div_denom)

// computes the value of a partial sample given the full sample value
// and a sample fraction
#define PARTIAL_SAMPLE(S, TSTATE_FRACTION, SAMPLE)                      \
   ((SAMPLE) * (TSTATE_FRACTION) * (S)->div_denom / (S)->div_num)

// computes the number of complete samples in a number of CPU clocks
#define SAMPLE_COUNT(S, TSTATES)                \
   ((TSTATES) * (S)->div_denom / (S)->div_num)

//==============================================================================
// DAC Initialise
//
//   pass: void
// return: int                  0 if success, -1 if error
//==============================================================================
int dac_init (void)
{
 // register a sound source for the Microbee dac
 audio_register(&dac.snd_buf, "dac",
                &dac_tick, (void *)&dac,
                &dac_clock,
                1,
                DAC_HOLDOFF_TIME);

 // framerate is in frames/s, so one frame is 1/framerate seconds.
 dac.idle_count = DAC_IDLE_TIME * emu.framerate / 1000;

 /* Make the audio output decay with a time constant of about
  * 50ms. Actual hardware doesn't do this; but on actual hardware
  * the sound output also never goes negative :) */
 dac.tau = audio.frequency * DAC_DECAY_CONSTANT / 1000;

 return 0;
}

//==============================================================================
// DAC de-initialise
//
//   pass: void
// return: int                          0
//==============================================================================
int dac_deinit (void)
{
 audio_deregister(&dac.snd_buf);
 return 0;
}

//==============================================================================
// Set the tstates->samples conversion factor based on the current CPU
// clock and the current output sample frequency.
//
//   pass: int cpuclock                 CPU clock speed in Hz
//==============================================================================
void dac_clock (int cpuclock)
{
 dac_t *s = &dac;
 uint64_t cycles_now = z80api_get_tstates();

 if (audio.mode != AUDIO_PROPORTIONAL)
    cpuclock = 3375000;

 dac.div_denom = audio.frequency;
 dac.div_num = cpuclock;

 {
  int a = dac.div_denom, b = dac.div_num, t;

  /* A should be > B */
  if (a < b)
     {
      t = a;
      a = b;
      b = t;
     }
  while (b != 0)                /* compute GCD */
     {
      t = b;
      b = a % b;
      a = t;
     }
  dac.div_num /= a;
  dac.div_denom /= a;
 }

 // The current sample number and partial sample counts also
 // need to be updated here
 s->samplenumber = SAMPLE_COUNT(s, cycles_now);
 s->fraction = SAMPLE_TIME_FRACTION(s, cycles_now);
}

//==============================================================================
// DAC sample.
//
//   pass: uint8_t data
// return: int
//==============================================================================
inline int dac_sample (uint8_t data)
{
 return data - 128;
}

//==============================================================================
// DAC sample fixup.  Integer rounding errors can accrue to the point
// where an accumulated sample doesn't quite add up to DAC_AMPLITUDE
// which leads to an annoying buzz in the output.
//
//   pass: int                          sample
// return: int
//==============================================================================
inline int dac_fixup_sample (int sample)
{
 return sample;
}

//==============================================================================
// DAC reset.
//
//   pass: void
// return: int                          sound init result
//==============================================================================
int dac_reset (void)
{
 dac_t *s = &dac;
 audio_scratch_t *sb = &s->snd_buf;

 s->state = 0;
 s->change_tstates = z80api_get_tstates();
 s->decay = 0;
 s->fraction = 0;
 s->last_sample = 0;

 // If there is an audio buffer under construction - dump it, the
 // next call to dac_fill will get a fresh one.
 if (audio_has_work_buffer(sb))
    audio_put_work_buffer(sb);

 return 0;
}

//==============================================================================
// DAC fill.
//
//   pass: dac_t *s
//         int sample
//         int count
// return: void
//==============================================================================
void dac_fill (dac_t *s, int sample, int count)
{
 audio_scratch_t *sb = &s->snd_buf;
#if DEBUG_DAC
    xprintf("dac_fill: writing %d of %d\n", count, sample);
#endif /* DEBUG_DAC */
 // fill the buffer with the dac value.
 while (count)
    {
     int n;
     /* flush the current work buffer if it is full */
     if (audio_space_remaining(sb) == 0)
         audio_put_work_buffer(sb);
     /* get a fresh sound buffer if necessary */
     if (!audio_has_work_buffer(sb))
        audio_get_work_buffer(sb);
     /* work out how many samples will fit in the current buffer */
     n = audio_space_remaining(sb);
     if (n > count)
        n = count;
     count -= n;
     while (n--)
        {
         s->decay -= ((sample * (1 << 16)) + s->decay) / s->tau;
         // delay applying the decay value until after it becomes
         // significant...  FIXME: necessary?
         if (s->decay > 2 * (1 << 16) || s->decay < -2 * (1 << 16))
            audio_put_sample(sb, audio_limit(sample + (s->decay / (1 << 16))));
         else
            audio_put_sample(sb, audio_limit(sample));
        }
    }
}

//==============================================================================
// DAC update.
//
// dac_update generates audio samples since the last dac bit change.
//
//   pass: dac_t *
//         uint_8
// return: void
//==============================================================================

void dac_update (dac_t *s, uint8_t data)
{
 uint64_t cycles_now;
 int samplenumber_now;
 int fraction_now;
 int fraction_diff;
 unsigned int n;
 int sample = dac_sample(s->state);
 int fractional_sample;

 cycles_now = z80api_get_tstates();

#if DEBUG_DAC
 xprintf("dac_update: cycles_now %llu, cycles_then %llu\n",
         cycles_now, s->change_tstates);
#endif /* DEBUG_DAC */

 /* if there is no current buffer, obtain one.  In this case the
  * audio source has been idle for some time, so we assume the
  * last sample to be zero and the last state change to be now */
 if (!audio_has_work_buffer(&s->snd_buf))
    {
     audio_get_work_buffer(&s->snd_buf);
     s->change_tstates = cycles_now;
     s->last_sample = 0;
     s->samplenumber = SAMPLE_COUNT(s, s->change_tstates);
     s->fraction = SAMPLE_TIME_FRACTION(s, s->change_tstates);
#if DEBUG_DAC
     xprintf("dac_update: "
             "initial partial sample: %d * %d/%d of %d = %d\n",
             s->fraction, s->div_denom, s->div_num,
             dac_sample(s->state), s->last_sample);
#endif /* DEBUG_DAC */
    }

 samplenumber_now = SAMPLE_COUNT(s, cycles_now);
 fraction_now = SAMPLE_TIME_FRACTION(s, cycles_now);

 if (samplenumber_now == s->samplenumber)
    {
     /* Only the partial sample needs to be updated, we don't need
      * to emit it yet */
#if DEBUG_DAC
     xprintf("dac_update: "
             "updated partial sample %d ", s->last_sample);
#endif /* DEBUG_DAC */
     fraction_diff = fraction_now - s->fraction;
     fractional_sample = PARTIAL_SAMPLE(s, fraction_diff, sample);
     s->last_sample += fractional_sample;
#if DEBUG_DAC
     xprintf("with %d * %d/%d of %d = %d ",
             fraction_diff, s->div_denom, s->div_num,
             sample, fractional_sample);
     xprintf("result %d\n", s->last_sample);
#endif /* DEBUG_DAC */
     /* the sample number remains unchanged */
    }
 else
    {
     // need to finish off the partial sample from the last call
     // to dac_update();
     fraction_diff = SAMPLE_TIME_FRACTION_REMAINING(s, s->change_tstates);
     fractional_sample = PARTIAL_SAMPLE(s, fraction_diff, sample);
     s->last_sample += fractional_sample;
     s->last_sample = dac_fixup_sample(s->last_sample);
#if DEBUG_DAC
     xprintf("dac_update: "
             "updated partial sample: %d * %d/%d of %d = %d\n",
             fraction_diff, s->div_denom, s->div_num,
             sample, fractional_sample);
#endif /* DEBUG_DAC */
#if DEBUG_DAC
     xprintf("dac_update: value %d\n", s->last_sample);
#endif /* DEBUG_DAC */
     assert(s->last_sample >= -(AUDIO_MAXVAL + 1) &&
            s->last_sample <= AUDIO_MAXVAL);
     dac_fill(s, s->last_sample, 1);
     s->samples_since_write++;
     // write out complete samples
     n = samplenumber_now - s->samplenumber - 1;
     dac_fill(s, sample, n);
     s->samples_since_write += n;
     // and record the final partial sample.
     s->last_sample = PARTIAL_SAMPLE(s, fraction_now, sample);
#if DEBUG_DAC
     xprintf("dac_update: "
             "created partial sample: %d * %d/%d of %d = %d\n",
             fraction_now, s->div_denom, s->div_num,
             sample, s->last_sample);
#endif /* DEBUG_DAC */
    }
 s->fraction = fraction_now;
 s->samplenumber = samplenumber_now;
 s->state = data;
 s->change_tstates = cycles_now;
}

//==============================================================================
// DAC write.
//
//   pass: uint8_t data                 upto 8 DAC bits
// return: void
//==============================================================================
void dac_w (uint8_t data)
{
 dac_t *s = &dac;

 // only do something if the dac state changes.
 if (audio.mute)
    return;
 if (data == s->state)
    return;

#if DEBUG_DAC
 xprintf("dac_w: writing %02x\n", data);
#endif /* DEBUG_DAC */
 // if this is the first update since the dac source was marked idle
 // and stopped generating samples, just update the last update time, don't
 // actually write anything into the buffer yet.
 if (s->idle && s->count == 0)
    {
     s->last_sample = 0;
     s->state = data;
     s->change_tstates = z80api_get_tstates();
    }
 else
    dac_update(s, data);
 s->idle = 0;
 s->count = s->idle_count;
 s->samples_since_write = 0;
}

//==============================================================================
// DAC tick function, called at the end of every block of Z80 instructions.
//
//   pass: audio_scratch_t *buf
//         const void *data
//         uint64_t start
//         uint64_t cycles
// return: int
//==============================================================================
int dac_tick (audio_scratch_t *buf, const void *data,
              uint64_t start, uint64_t cycles)
{
 dac_t *s = (dac_t *)data;

 if (!audio_has_work_buffer(&s->snd_buf))
    goto idle;

 if (s->change_tstates == start)
    {
     if (s->idle)
        {
         if (s->count > 0)
            s->count--;
         else
            goto idle;
        }
     else
        {
         s->idle = 1;
         s->count = s->idle_count;
        }
    }

#if DEBUG_DAC
// insert a very marked click into the audio stream
// dac_fill(s, 128, 1);
// dac_fill(s, -128, 1);
 xprintf("dac_tick:\n");
#endif /* DEBUG_DAC */
 dac_update(s, s->state);

 if (s->idle && s->count == 0)
 {
  audio_put_work_buffer(&s->snd_buf); // flush current buffer.
  s->decay = 0;                       // reset decay constant
 }
 return 1;

idle:
 s->change_tstates = start + cycles;
 return 0;
}
