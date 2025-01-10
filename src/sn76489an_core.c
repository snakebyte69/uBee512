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
// This module emulates the SN76489AN sound generator.
//
// Notes:
// The data sheet mentions that pitch changes do not take effect until up to
// 32 clock periods after the period registers are written to.  In the microbee,
// the CPU is WAITed for 32 clocks whenever the SN76489 is written to.  This
// implementation does NOT emulate this behaviour.  It is not clear whether
// this is important or not.
//
// References:
//
// [1] SN76489AN data sheet
// [2] The SN76489 notes at the BBC micro documentation archive
//          http://bbcdocs.com/filebase/hardware/datasheets/SN76489.txt
// [3] The Linear Feedback Shift Register notes at New Wave Instruments
//          http://www.newwaveinstruments.com/resources/articles/
//          m_sequence_linear_feedback_shift_register_lfsr.htm
// [4] The SN76489 Wikipedia page
//          http://en.wikipedia.org/wiki/Texas_Instruments_SN76489
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
// v5.2.0 - 06 August 2010, K Duckmanton
// - Initial implementation
//==============================================================================

static const char
   rcs_id[]="$Id: sn76489an_core.c,v 1.1.1.1.2.1 2011/03/27 07:38:28 krd Exp $";

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
#include "sn76489an_core.h"

//==============================================================================
// constants
//==============================================================================
#define SN76489AN_CLOCK         (s->clock_frequency)
#define SN76489AN_CLOCK_DIVISOR 16 /* from the data sheet */
#define SN76489AN_SAMPLE_RATE   (SN76489AN_CLOCK / SN76489AN_CLOCK_DIVISOR)

#define NUM_SN76489AN_SAMPLES   (SN76489AN_SAMPLE_RATE * 5 / 1000)

#define SN76489AN_COUNTER_RELOAD 0
#define SN76489AN_HOLDOFF_TIME   50    /* ms */
#define SN76489AN_IDLE_TIME      1000  /* ms */
#define SN76489AN_DECAY_CONSTANT 50    /* ms */

#define SN76489AN_AMPLITUDE     (AUDIO_MAXVAL / 4)
#define SN_COMMAND_MASK         (1 << 7)
#define SN_LO_BITS              4
#define SN_HI_BITS              6
#define SN_ATTEN_BITS           4
#define SN_REGISTER_NUMBER_MASK (((1 << 3) - 1) << SN_LO_BITS)
#define SN_REGISTER_MASK        (((1 << 10) - 1))
#define SN_LO_VALUE_MASK        ((1 << SN_LO_BITS) - 1)
#define SN_HI_VALUE_MASK        ((1 << SN_HI_BITS) - 1)
#define SN_ATTEN_VALUE_MASK     ((1 << SN_ATTEN_BITS) - 1)
#define SN_NOISE_INITIAL        (1 << 14)

//==============================================================================
// structures and variables
//==============================================================================
/*
 * The sn76489an's "amplitude" register is actually an attenuation
 * register.
 */
static uint8_t sn76489an_amplitude[16] = {
 /*
  * This elisp snippet calculates the resulting amplitude for each
  * selected attenuation factor.  The formula is
  *
  * amplitude = maxa * 10 ^ ( -2.0 * af / 20.0 )
  *
  * where maxa is the maximum amplitude (AUDIO_MAXVAL/4)

 (progn
  (setq i 0)
  (dotimes (i 15)
   (insert-string (format "%4.0f, " (* 32.0 (exp (* (log 10) (/ (* -2.0 i) 20.0))))))
     ))

  */

  32,   25,   20,   16,
  13,   10,    8,    6,
   5,    4,    3,    3,
   2,    2,    1,    0,
};

extern modio_t modio;
extern audio_t audio;
extern emu_t emu;

//==============================================================================
// function prototypes
//==============================================================================
int sn76489an_core_iterate (sn76489an_t *snd, int samples);
int sn76489an_core_tick (audio_scratch_t *buf, const void *data,
                         uint64_t frame_start, uint64_t cycles);

//==============================================================================
// sn76489an core init.
//
//   pass: sn76489an_t *s
//         char *name
//         audio_clock_fn_t clock_fn
//         int clock_frequency
//         int silence
// return: int                          0 if success, -1 if error
//==============================================================================
int sn76489an_core_init (sn76489an_t *s, char *name,
                         audio_clock_fn_t clock_fn,
                         int clock_frequency,
                         int silence)
{
 memset(s, 0, sizeof(*s));     /* FIXME: The registers are initialised
                                * to ... something ... on power up.
                                * Consult other emulator sources?
                                * Experiment? */
 /* From investigations with a sample SN76489AN it seems that the
  * initial register values are something like this:
  *
  * Tone channel 1: period = 0x00f, attenuation = 0x0 (7031Hz)
  * Tone channel 2: period = 0x000, attenuation = 0x0 (103Hz)
  * Tone channel 3: period = 0x000, attenuation = 0x0 (103Hz)
  * Noise channel: period = 0x0, attenuation = 0xf
  */
 s->regs[0] = 0xf;              /* channel 1 period */
 s->regs[7] = 0xf;              /* tone generator attenuation */
 if (silence)
 {
  /* silence all channels */
  s->regs[1] = s->regs[3] = s->regs[5] = s->regs[7] = 0xf;
 }

 s->noise = SN_NOISE_INITIAL;   /* the noise shift register must be
                                 * initialised to a non-zero value */
 audio_circularbuf_init(&s->scratch);
 /* -------------------------------------------------------------------- */
 /*  Register this as a sound peripheral with the SND driver.            */
 /* -------------------------------------------------------------------- */
 if (audio_register(&s->snd_buf,
                    name,
                    &sn76489an_core_tick, (void *)s,
                    clock_fn,
                    1,          /* 1 -> synchronise with CPU thread */
                    0           /* holdoff time */
                    ))
    return -1;
 s->clock_frequency = clock_frequency;
 audio_circularbuf_set_rate_conversion(&s->scratch,
                                       audio.frequency,
                                       clock_frequency /
                                       SN76489AN_CLOCK_DIVISOR);
 audio_circularbuf_set_decay_constant(&s->scratch,
                                      SN76489AN_DECAY_CONSTANT);
 return 0;
}

//==============================================================================
// sn76489an core de-initilisation.
//
//   pass: sn76489an_t *s
// return: int                          0
//==============================================================================
int sn76489an_core_deinit (sn76489an_t *s)
{
 audio_deregister(&s->snd_buf);
 audio_circularbuf_deinit(&s->scratch);
 return 0;
}

//==============================================================================
// sn76489an core reset.
//
//   pass: sn76489an_t *s
// return: int                          0
//==============================================================================
int sn76489an_core_reset (sn76489an_t *s)
{
 return 0;
}

//==============================================================================
// Set the sample rate conversion factor based on the current CPU
// clock and the current output sample frequency.
//
//   pass: int clock_frequency          New Sn76489 clock frequency
// return: void
//==============================================================================
void sn76489an_core_clock (sn76489an_t *s, int clock_frequency)
{
 audio_drain_samples(&s->snd_buf, &s->scratch);
 audio_circularbuf_set_rate_conversion(&s->scratch,
                                       audio.frequency,
                                       clock_frequency /
                                       SN76489AN_CLOCK_DIVISOR);
 s->clock_frequency = clock_frequency;
}

//==============================================================================
// sn76489an core read.
//
//   pass: sn76489an_t *s
//         uint16_t port
// return: int                          0
//==============================================================================
uint16_t sn76489an_core_r (sn76489an_t *s, uint16_t port)
{
 return 0;                      /* FIXME: can we read from this chip
                                 * at all? */
}

//==============================================================================
// sn76489an core write.
//
//   pass: sn76489an_t *s
//         uint16_t port
//         uint8_t data
// return: void
//==============================================================================
void sn76489an_core_w (sn76489an_t *s, uint16_t port, uint8_t data)
{
 sn_update_le_t *p;

 /* All register writes need to be deferred until the audio samples
  * are generated */
 if (data & SN_COMMAND_MASK)
    {
     s->current_register =
        (data & SN_REGISTER_NUMBER_MASK) >> SN_LO_BITS;
    }
 p = malloc(sizeof(*p));
 p->address = s->current_register;
 p->data = data;
 p->when = z80api_get_tstates();
 p->next = NULL;

 if (!s->update_head)
    {
     s->update_head = s->update_tail = p;
    }
 else
    {
     s->update_tail->next = p;
     s->update_tail = p;
    }
}

//==============================================================================
// sn76489an generate sample.
//
//   pass: sn76489an_t *s
// return: int
//==============================================================================
int sn76489an_gen_sample (sn76489an_t *s)
{
 int i;
 int smask;
 int sample = 0;
 uint16_t *periodp, *attenp;
 uint16_t *currperiodp;
 int v;

 smask = (1 << 0);
 periodp = &s->regs[0];
 attenp = &s->regs[1];
 currperiodp = &s->period_current[0];

 i = 0;
 while (i < 3)
    {
     if (--(*currperiodp) == 0)
        {
         s->state ^= smask;
         if ((*currperiodp = *periodp) == 0)
            // A period value of 0 is special - this ends up being a
            // division by 1024.
            *currperiodp = (1 << (SN_LO_BITS + SN_HI_BITS));;
        }
     v = sn76489an_amplitude[*attenp & SN_ATTEN_VALUE_MASK];
     if (s->state & smask)
        sample += v;
     else
        sample -= v;
     periodp += 2;
     attenp += 2;
     currperiodp += 1;
     smask <<= 1;
     ++i;
    }

 /* Update the noise channel. */
 if (--(*currperiodp) == 0)
    {
     /* The noise register's shift rate is controlled by the low two
      * bits of the "period" register */
     switch (*periodp & ((1 << 2) - 1))
        {
         case 0:
            *currperiodp = 0x20;
            break;
         case 1:
            *currperiodp = 0x40;
            break;
         case 2:
            *currperiodp = 0x80;
            break;
         case 3:
            *currperiodp = s->regs[2 * 2]; /* the divisor used is the
                                            * divisor for tone
                                            * generator 2 */
            break;
        }
     /* Bit 2 of the noise generator's period register controls the
      * noise mode - either "white" (1) or "periodic" (0) */
     if (*periodp & (1 << 2))
        {
         /* white noise.  In this mode the SN76489AN outputs a maximal
          * length PRNG sequence.  The LSFR reference[3] documents 2
          * ways of implementing an LSFR: a Fibonacci implementation,
          * where a modulo 2 sum of the binary weighted taps is fed
          * back to the input (this realisation is also the one
          * described in [2]), and a Galois implementation, where the
          * bits at each stage of the shift register are modified by
          * the weighted value of the output stage.
          *
          * [3] also notes that the sequence of weights for the
          * Fibonacci implementation is precisely the reverse of the
          * sequence for the Galois implementation, which makes it
          * easy to convert from one to the other.
          *
          * The Galois implementation is easier for a computer to
          * realise.
          *
          * [2] suggests several possibilities for the taps, depending
          * on the revision and manufacturer, though the description
          * is a little unclear.  [1] gives the length of shift
          * register as 15 bits for the TI chips, while [2] and [4]
          * also mention that clones of this chip (such as those made
          * by Sega) had a 16 stage shift register.
          *
          * [2] and [4] give different taps for the feedback network.
          * For this implementation we assume that the LFSR is to
          * produce a maximal length sequence; [3] gives 3
          * possibilities for generating a maximal length sequence
          * from a 15 stage shift register with 2 taps:
          *
          * (15, 14), (15, 11) and {15, 8) (with implied bit 0, the
          * output)
          *
          * A-B tests with a real SN76489A reveal that the sequence
          * produced by the first pair sounds very similar to the real
          * output.
          */
         if (s->noise & 1)
             s->noise ^= ((1 << 14) | (1 << 13)) << 1;
        }
     else
        {
         /* periodic noise.  In this mode the chip outputs a series of
          * single bit impulses from the 15 bit shift register. */
         if (s->noise & 1)
             s->noise = SN_NOISE_INITIAL << 1;
        }
     s->noise >>= 1;
    }
 v = sn76489an_amplitude[*attenp & SN_ATTEN_VALUE_MASK];

 if (s->noise & 1)
    sample += v;
 else
    sample -= v;

#if 0
 if (sample != 0)
    xprintf("sn76489an_gen_sample: sample %4d\n", sample);
#endif
 return sample;
}

//==============================================================================
// Register update.
//
//   pass: sn76489an_t *s
//         uint8_t address
//         uint8_t data
// return: int
//==============================================================================
static void register_update (sn76489an_t *s, uint8_t address, uint8_t data)
{
 if (data & SN_COMMAND_MASK)
    {
     /* If the command bit is set, writes update the 4 least
      * significant bits of the register */
     s->regs[address] =
         ((s->regs[address] & ~SN_LO_VALUE_MASK) |
          (data & SN_LO_VALUE_MASK)) & SN_REGISTER_MASK;
    }
 else
    {
     /* the second byte updates the most significant 6 bits of the
      * register if the register selected is not an attenuation
      * register and it is not the noise period register */
     if (address & (1 << 0) || address == 6)
        s->regs[address] = data;
     else
        s->regs[address] =
           ((s->regs[address] & ~(SN_HI_VALUE_MASK << SN_LO_BITS)) |
            ((data & SN_HI_VALUE_MASK) << SN_LO_BITS)) & SN_REGISTER_MASK;
    }
 /* If the noise register is written to, the noise shift register is
  * reset */
 if (address == 3 * 2)
    s->noise = (1 << 15);
}

//==============================================================================
// Sn76489an tick function.  Registered as a callback function in
// sn76489an_init() and called by audio_sources_update()
//
//   pass: audio_scratch_t
//         const void * (pointing to a sn76489an_t structure containing the
//                       sn76489an state)
//         uint64_t     (number of z80 tstates at the start of the frame)
//         uint64_t     (number of z80 tstates since the last time the tick
//                       function was called)
//
// return: int          0 - sound source quiescent
//                      not 0 - sound source active
//==============================================================================
int sn76489an_core_tick (audio_scratch_t *buf, const void *data,
                         uint64_t frame_start, uint64_t cycles)
{
 sn76489an_t *s = (sn76489an_t *)data;
 unsigned int ticks_per_sample;
 unsigned int num_samples;

#if DEBUG_SN76489AN
 xprintf("sn76489an: %9llu ticks, %9llu remainder",
         cycles, s->cycles_remainder);
#endif
 // If s->clock_frequency is zero, return zero straight away.  The
 // audio source is being drained before the emulator clock frequency
 // has been initialised.
 if (s->clock_frequency == 0)
    return 0;

 ticks_per_sample = emu.cpuclock / SN76489AN_SAMPLE_RATE;
 // add the leftover cycles from the last frame to the cycles for the
 // current frame, adjust the start time to be immediately after the
 // last sn76489an sample generated.
 cycles += s->cycles_remainder;
 frame_start -= s->cycles_remainder;
 // Compute the number of SN76489an samples that would have been
 // generated during the last block of CPU instructions.
 num_samples = cycles / ticks_per_sample;
 // Adjust the number of samples to be no more than the number of
 // samples that can be generated during 1 frame time
 if (num_samples > SN76489AN_SAMPLE_RATE / emu.framerate)
     num_samples = SN76489AN_SAMPLE_RATE / emu.framerate;
 // and the number of leftover CPU cycles which need to count
 // towards the next block
 s->cycles_remainder = cycles - num_samples * ticks_per_sample;
#if DEBUG_SN76489AN
 xprintf(" gives %6u samples with %9llu remainder\n",
         num_samples, s->cycles_remainder);
#endif

 // now generate samples
 while (num_samples)
    {
     int samples_generated;
     int n;
     sn_update_le_t *up, *uq;

     audio_drain_samples(&s->snd_buf, &s->scratch);
     do {
      /* apply all register updates that are due. */
      for (up = s->update_head;
           up && up->when <= frame_start;)
         {
          if (modio.sn76489an)
             {
              xprintf("Sn76489an: register update (z80 tstates %llu) r%02o = %02x\n",
                      up->when, up->address, up->data);
             }
          register_update(s, up->address, up->data);
          uq = up->next;
          free(up);
          up = uq;
         }
      s->update_head = up;
      if (up == NULL)         /* list now empty */
         s->update_tail = up;
      if (up == NULL)
         n = num_samples;       /* generate the requested number of
                                 * samples */
      else
         {
          /* ceil((when - frame_start) / ticks_per_sample) */
          n = (up->when - frame_start +
               ticks_per_sample - 1) / ticks_per_sample;
          if (n > num_samples)
             n = num_samples;   /* generate no more than the number of
                                 * samples originally requested */
         }
      samples_generated = sn76489an_core_iterate(s, n);
      if (samples_generated <= 0)
         ;                      /* the buffer needs draining */
      else
         {
          frame_start += samples_generated * ticks_per_sample;
          num_samples -= samples_generated;
         }
     } while (samples_generated > 0);
    }
 return 1;                      /* output is always generated */
}

//==============================================================================
// sn76489an core iterate.
//
// Generate the requested number of samples into the sample buffer.
//
//   pass: sn76489an_t *s
//         int samples
// return: int         Number of samples actually generated, or -1 if the
//                     sample buffer is full and needs to be drained.
//==============================================================================
int sn76489an_core_iterate (sn76489an_t *s, int samples)
{
 int do_samp;

 if (samples <= 0)
    return 0;           /* no samples to generate! */
 audio_circularbuf_normalise(&s->scratch, AUDIO_CIRCULARBUF_MASK);
 do_samp = audio_circularbuf_samples_remaining(&s->scratch,
                                               AUDIO_CIRCULARBUF_SIZE);
 // the circular buffer is deemed to be full when only 1 sample is
 // left - i.e. head is just behind tail
 if (do_samp <= 1)
    return -1;          /* sample buffer is full */
 if (do_samp > samples)
    do_samp = samples;  /* generate no more samples than
                         * requested (but can generate
                         * fewer!) */
 assert(do_samp > 0 && do_samp <= samples &&
        do_samp < AUDIO_CIRCULARBUF_SIZE);
 samples = do_samp;
 while (do_samp--)
    audio_circularbuf_put_sample(&s->scratch,
                                 AUDIO_CIRCULARBUF_MASK,
                                 sn76489an_gen_sample(s));
 return samples;
}
