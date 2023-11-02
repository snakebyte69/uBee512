//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                       Copyright (C) 2007-2016 uBee                         *
//*                                                                            *
//*                        AY-3-8190 emulation module                          *
//*                Copyright (C) 2009-2010 Kalvis Duckmanton                   *
//*                                                                            *
//******************************************************************************
//
// This module emulates the GI AY-3-8910 programmable sound generator.
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
// v4.7.0 - 17 June 2010, K Duckmanton
// - Initial implementation
//==============================================================================

static const char rcs_id[]="$Id: ay38910.c,v 1.1.1.4 2011/03/27 06:04:42 krd Exp $";

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "ubee512.h"
#include "audio.h"
#include "function.h"
#include "ay38910.h"

#define PSG_COUNTER_RELOAD 0

/* the amplitude response is exponential from 0V to 1V. */
static uint8_t psg_amplitude[16] = {
#if 0
 /* linear response */
 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
#else
 /* exponential response.  The function used to calculate these values is:
  *
  *        127    exp(x / 15) - 1
  *    y = --- * -----------------
  *         3       exp(1) - 1
  *
  *   (since there are 3 channels and we don't want the output to overflow)
  */
 0, 1, 3, 5, 7, 9, 12, 14, 17, 20, 23, 26, 30, 34, 38, 42,
#endif
};

int psg_init(ay_3_8910_t *psg)
{
 memset(psg, 0, sizeof(*psg)); /* reset clears all registers */
 /* disable all audio sources; the memset has set the volumes to zero
  * for all channels */
 psg->reg[PSG_MIXER_IO_EN] = PSG_NOISE_SELECT_MASK | PSG_TONE_SELECT_MASK;
 /* set the initial state of all noise and tone bits to 1, to allow
  * the noise mixing logic to work correctly */
 psg->state = PSG_CHANNEL_A | PSG_CHANNEL_B | PSG_CHANNEL_C | PSG_NOISE_BIT;
 audio_circularbuf_init(&psg->scratch);
 return 0;
}

int psg_deinit(ay_3_8910_t *psg)
{
 audio_circularbuf_deinit(&psg->scratch);
 return 0;
}

uint8_t psg_r(ay_3_8910_t *psg, uint8_t reg)
{
 if ((reg & PSG_SELECT_MASK) != PSG_SELECT_VALUE)
    return 0;

 return psg->reg[reg & PSG_REGISTER_MASK];
}

void psg_w(ay_3_8910_t *psg, uint8_t reg, uint8_t data)
{
 int index;

 if ((reg & PSG_SELECT_MASK) != PSG_SELECT_VALUE)
    return;
 reg &= PSG_REGISTER_MASK;
 /* Mask out unused bits now so that we don't have to do it
  * later */
 switch (reg)
    {
     case PSG_TONE_A_COARSE:
     case PSG_TONE_B_COARSE:
     case PSG_TONE_C_COARSE:
        data &= (PSG_TONE_MASK >> 8);
        break;
     case PSG_NOISE:
        data &= PSG_NOISE_MASK;
        break;
     case PSG_AMPLITUDE_A:
     case PSG_AMPLITUDE_B:
     case PSG_AMPLITUDE_C:
        data &= (PSG_AMPLITUDE_MASK | PSG_AMPLITUDE_MODE_MASK);
        break;
     case PSG_ENVELOPE_SHAPE:
        data &= 0x0f;
        /* Writes to the envelope shape register cause the envelope
         * generator to restart the pattern output immediately if it
         * is idle or is going to be in the near future - i.e. if the
         * hold bit is set or the continue bit is clear */
        if ((psg->reg[PSG_ENVELOPE_SHAPE] & PSG_ENVELOPE_CYCLE_HOLD) ||
            !(psg->reg[PSG_ENVELOPE_SHAPE] & PSG_ENVELOPE_CYCLE_CONTINUE))
           {
            /*
             * If the generator is in the decay phase, this is
             * obvious, as the output is most definitely idle.
             */
            if (psg->envelope_state & PSG_ENVELOPE_STATE_DECAY)
               psg->envelope_state = 0;
            else
            /*
             * If the generator is in the attack phase, the output is
             * reset only if the generator output is set to decay
             * (i.e. the attack bit is off).  Less obvious.
             */
               if (!(psg->reg[PSG_ENVELOPE_SHAPE] & PSG_ENVELOPE_CYCLE_ATTACK))
                  psg->envelope_state = 0;
           }
        break;
    }

 psg->reg[reg] = data;
 // dump current AY state
#if 0
 switch(reg)
    {
     case PSG_TONE_A_FINE:
     case PSG_TONE_A_COARSE:
        xprintf("Tone generator A period: %d\n",
                (psg->reg[PSG_TONE_A_COARSE] << 8) | psg->reg[PSG_TONE_A_FINE]);
        break;
     case PSG_TONE_B_FINE:
     case PSG_TONE_B_COARSE:
        xprintf("Tone generator B period: %d\n",
                (psg->reg[PSG_TONE_B_COARSE] << 8) | psg->reg[PSG_TONE_B_FINE]);
        break;
     case PSG_TONE_C_FINE:
     case PSG_TONE_C_COARSE:
        xprintf("Tone generator C period: %d\n",
                (psg->reg[PSG_TONE_C_COARSE] << 8) | psg->reg[PSG_TONE_C_FINE]);
        break;
     case PSG_NOISE:
        xprintf("Noise generator C period: %d\n",
                psg->reg[PSG_NOISE]);
        break;
     case PSG_MIXER_IO_EN:
        xprintf("Mixer control register: %02x\n",
                psg->reg[PSG_MIXER_IO_EN]);
        break;
     case PSG_AMPLITUDE_A:
        xprintf("Tone generator A amplitude: ");
        if (psg->reg[PSG_AMPLITUDE_A] & PSG_AMPLITUDE_MODE_MASK)
           xprintf("(envelope generator)");
        else
           xprintf("%02x",      psg->reg[PSG_AMPLITUDE_A]);
        xprintf("\n");
        break;
     case PSG_AMPLITUDE_B:
        xprintf("Tone generator B amplitude: ");
        if (psg->reg[PSG_AMPLITUDE_B] & PSG_AMPLITUDE_MODE_MASK)
           xprintf("(envelope generator)");
        else
           xprintf("%02x",      psg->reg[PSG_AMPLITUDE_B]);
        xprintf("\n");
        break;
     case PSG_AMPLITUDE_C:
        xprintf("Tone generator C amplitude: ");
        if (psg->reg[PSG_AMPLITUDE_C] & PSG_AMPLITUDE_MODE_MASK)
           xprintf("(envelope generator)");
        else
           xprintf("%02x",      psg->reg[PSG_AMPLITUDE_C]);
        xprintf("\n");
        break;
     case PSG_ENVELOPE_FINE:
     case PSG_ENVELOPE_COARSE:
        xprintf("Envelope generator period: %d\n",
                (psg->reg[PSG_ENVELOPE_COARSE] << 8) | psg->reg[PSG_ENVELOPE_FINE]);
        break;
     case PSG_ENVELOPE_SHAPE:
        xprintf("Envelope shape register: %02x\n",
                psg->reg[PSG_MIXER_IO_EN]);
        break;
     default:
     break;
    }
#endif

 // Writes to some registers (e.g. any of the period registers)
 // have side effects - namely that the associated tone generator
 // output is toggled and the period counter is immediately
 // reloaded if it would have been reloaded had the new period
 // value been in effect at the start of the current tone cycle.
 // I.E. if (new_divisor_value >= current_count) for counters that
 // count UP.  For counters that count DOWN this gets a little
 // trickier :)

// This also needs to handle the case where the limit register has been
// set to 0 for the maximum period.

#define UPDATE_LIMIT_REG(TYPE, COUNTER, LIMIT, NEWLIMIT)        \
 do {                                                           \
  TYPE curlimit = (LIMIT);                                      \
  TYPE newlimit = (NEWLIMIT);                                   \
  if (curlimit == PSG_COUNTER_RELOAD)                           \
     COUNTER = PSG_COUNTER_RELOAD;                              \
  else if ((curlimit - COUNTER) >= newlimit)                    \
     COUNTER = PSG_COUNTER_RELOAD;                              \
  LIMIT = newlimit;                                             \
 } while(0)

 switch(reg)
    {
     case PSG_TONE_A_COARSE:
     case PSG_TONE_B_COARSE:
     case PSG_TONE_C_COARSE:
     case PSG_TONE_A_FINE:
     case PSG_TONE_B_FINE:
     case PSG_TONE_C_FINE:
        index = (reg - PSG_TONE_A_FINE) / 2;
        UPDATE_LIMIT_REG(uint16_t,
                         psg->tone_current[index],
                         psg->tone_per[index],
                         ((psg->reg[index * 2 + (PSG_TONE_A_COARSE-PSG_TONE_A_FINE) + PSG_TONE_A_FINE] << 8) |
                          (psg->reg[index * 2 + PSG_TONE_A_FINE])));
        break;
     case PSG_NOISE:
        UPDATE_LIMIT_REG(uint8_t,
                         psg->noise_current,
                         psg->noise_per,
                         data);
        break;
     case PSG_ENVELOPE_COARSE:
     case PSG_ENVELOPE_FINE:
        UPDATE_LIMIT_REG(uint16_t,
                         psg->envelope_current,
                         psg->envelope_per,
                         (psg->reg[PSG_ENVELOPE_COARSE] << 8) |
                         (psg->reg[PSG_ENVELOPE_FINE]   << 0));
        break;
     default:
     break;
    }
}

int psg_tick(ay_3_8910_t *psg)
{
 int result;

 /* All generators are clocked at a rate of Fin/16 */

 /* update the tone generators.  The BeeThoven software appears to
  * set the tone period register to 0 to disable the tone source
  * entirely. */
#define UPDATE_TONEGEN(PSG, N) \
 do {                                                   \
  if (PSG->tone_current[N] == PSG_COUNTER_RELOAD)       \
     {                                                  \
      PSG->tone_current[N] = psg->tone_per[N];          \
      if (PSG->tone_per[N])                             \
         PSG->state ^= (PSG_CHANNEL_A << N);            \
     }                                                  \
  PSG->tone_current[N]--;                               \
 } while (0)

 UPDATE_TONEGEN(psg, 0);        /* channel A */
 UPDATE_TONEGEN(psg, 1);        /* channel B */
 UPDATE_TONEGEN(psg, 2);        /* channel C */

 /* update the noise generator */
 if (psg->noise_current == PSG_COUNTER_RELOAD)
    {
     psg->noise_current = psg->noise_per;
     /*
      * The algorithm used here comes from a posting on a MSX mailing
      * list by Maarten ter Huurne at Wed, 17 Jan 2001 18:29:29 -0800.
      *
      * http://www.mail-archive.com/msx@stack.nl/msg14721.html
      *
      * Maarten asserts that the noise generator is a 17 bit linear
      * feedback shift register.
      */
     if (psg->noise & 1)
        psg->state |= PSG_NOISE_BIT;
     else
        psg->state &= ~PSG_NOISE_BIT;
     psg->noise >>= 1;
     psg->noise ^= (psg->state & PSG_NOISE_BIT) ? 0x02000 : 0x10000;
    }
 psg->noise_current--;
 /* update the envelope generator */
 /* The data sheet describes this but not in an especially clear
  * way.  Internally the envelope generator counter appears to be a
  * 16 bit counter, clocked at Fin/16, with the overflow from this
  * counter clocking a 4 bit envelope pattern generator counter. */
 if (psg->envelope_current == PSG_COUNTER_RELOAD)
    {
     /* underflow */
     psg->envelope_current = psg->envelope_per;
     /* update the envelope generator state variable */

     /* When the envelope generator 'alternate' bit is set, the
      * 'decay' bit of the envelope generator state variable indicates
      * whether the envelope is in the attacking (0) or decaying (1)
      * phase of its cycle. If the 'hold' bit is set, or the
      * 'continue' bit is clear, once this bit reaches 1 it is forced
      * to 1 afterwards */
     if ((psg->envelope_state & PSG_ENVELOPE_STATE_DECAY) &&
         ((psg->reg[PSG_ENVELOPE_SHAPE] & PSG_ENVELOPE_CYCLE_HOLD) ||
          !(psg->reg[PSG_ENVELOPE_SHAPE] & PSG_ENVELOPE_CYCLE_CONTINUE)))
        {
         psg->envelope_state++;
         psg->envelope_state |= PSG_ENVELOPE_STATE_DECAY;
        }
     else
        psg->envelope_state++;

     if (psg->envelope_state & PSG_ENVELOPE_STATE_DECAY)
        {
         /* second cycle */
         if (!(psg->reg[PSG_ENVELOPE_SHAPE] & PSG_ENVELOPE_CYCLE_CONTINUE))
            /* continue bit clear */
            psg->envelope_amplitude = 0;
         else if (psg->reg[PSG_ENVELOPE_SHAPE] & PSG_ENVELOPE_CYCLE_HOLD)
            {
             /* hold bit set */
             switch (psg->reg[PSG_ENVELOPE_SHAPE] &
                     (PSG_ENVELOPE_CYCLE_ALTERNATE | PSG_ENVELOPE_CYCLE_ATTACK))
                {
                 case 0:
                 case (PSG_ENVELOPE_CYCLE_ALTERNATE | PSG_ENVELOPE_CYCLE_ATTACK):
                    psg->envelope_amplitude = 0;
                    break;
                 case PSG_ENVELOPE_CYCLE_ALTERNATE:
                 case PSG_ENVELOPE_CYCLE_ATTACK:
                    psg->envelope_amplitude = 15;
                    break;
                }
            }
         else
            {
             /* hold bit clear, so whether attacking or decaying
              * depends on the attack and alternate bits. */
             switch (psg->reg[PSG_ENVELOPE_SHAPE] &
                     (PSG_ENVELOPE_CYCLE_ALTERNATE | PSG_ENVELOPE_CYCLE_ATTACK))
                {
                 case 0:
                 case (PSG_ENVELOPE_CYCLE_ALTERNATE | PSG_ENVELOPE_CYCLE_ATTACK):
                    psg->envelope_amplitude = PSG_ENVELOPE_CYCLE_VALUE(~psg->envelope_state);
                    break;
                 case PSG_ENVELOPE_CYCLE_ALTERNATE:
                 case PSG_ENVELOPE_CYCLE_ATTACK:
                    psg->envelope_amplitude = PSG_ENVELOPE_CYCLE_VALUE( psg->envelope_state);
                    break;
                }
            }
        }
     else
        {
         /* first cycle */
         /* The waveshape depends solely on the attack bit. */
         if (psg->reg[PSG_ENVELOPE_SHAPE] & PSG_ENVELOPE_CYCLE_ATTACK)
            psg->envelope_amplitude = PSG_ENVELOPE_CYCLE_VALUE( psg->envelope_state);
         else
            psg->envelope_amplitude = PSG_ENVELOPE_CYCLE_VALUE(~psg->envelope_state);
        }
    }
 psg->envelope_current--;

 /* Generate the current combined output of all 3 channels */
 result = 0;

 /* Noise/tone select bits are active low.  If the mixer settings have
  * disabled the tone and noise output, the output is determined by
  * the envelope generator */

 /* Where the envelope generator is being used to generate a tone
  * (nrrg!) we need to double the result and bias it by -max,
  * otherwise the resulting tone is 3dB quieter than the tone
  * generator */

 /* Where the envelope generator is not being used to generate a tone
  * AND both the noise and tone generators are off, we want to
  * contribute nothing to the output (this avoids ugly artefacts where
  * a tone can be generated by banging on the amplitude register for a
  * channel.  FIXME: It's not entirely clear whether a real device
  * changes the output amplitude in that case). */

#define MIX_TONE(PSG, N, RESULT)                                        \
 do {                                                                   \
  int amplitude =                                                       \
     psg_amplitude[                                                     \
        (PSG->reg[N + PSG_AMPLITUDE_A] & PSG_AMPLITUDE_MODE_MASK) ?     \
        PSG->envelope_amplitude :                                       \
        PSG->reg[N + PSG_AMPLITUDE_A]                                   \
        ];                                                              \
  if (                                                                  \
     (PSG->reg[PSG_MIXER_IO_EN] &                                       \
      ((PSG_CHANNEL_A | PSG_NOISE_CHANNEL_A) << N)) ==                  \
     ((PSG_CHANNEL_A | PSG_NOISE_CHANNEL_A) << N)                       \
     )                                                                  \
     {                                                                  \
      if (PSG->reg[N + PSG_AMPLITUDE_A] & PSG_AMPLITUDE_MODE_MASK)      \
         {                                                              \
          RESULT += 2 * amplitude - psg_amplitude[PSG_AMPLITUDE_MASK];  \
         }                                                              \
      else                                                              \
         {                                                              \
         }                                                              \
     }                                                                  \
  else                                                                  \
     {                                                                  \
      int bit = 1;                                                      \
      if (!(PSG->reg[PSG_MIXER_IO_EN] & (PSG_CHANNEL_A << N)) &&        \
          !(PSG->state                & (PSG_CHANNEL_A << N)))          \
         bit = 0;                                                       \
      if (!(PSG->reg[PSG_MIXER_IO_EN] & (PSG_NOISE_CHANNEL_A << N)) &&  \
          !(PSG->state                & (PSG_NOISE_BIT)))               \
         bit = 0;                                                       \
      RESULT += (bit ? amplitude : -amplitude);                         \
     }                                                                  \
 } while (0)

 MIX_TONE(psg, 0, result);
 MIX_TONE(psg, 1, result);
 MIX_TONE(psg, 2, result);

 return result;
}

/* ======================================================================== */
/*  PSG_ITERATE -- generate the requested number of samples into the
 *  sample buffer.  Returns the number of samples actually generated,
 *  or -1 if the sample buffer is full and needs to be drained.  */
/* ======================================================================== */
int psg_iterate(ay_3_8910_t *psg, int samples)
{
 int do_samp;

 if (samples <= 0)
    return 0;           /* no samples to generate! */
 audio_circularbuf_normalise(&psg->scratch, AUDIO_CIRCULARBUF_MASK);
 do_samp = audio_circularbuf_samples_remaining(&psg->scratch,
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
    audio_circularbuf_put_sample(&psg->scratch,
                                 AUDIO_CIRCULARBUF_MASK,
                                 psg_tick(psg));
 return samples;
}
