//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                       Copyright (C) 2007-2024 uBee                         *
//*                                                                            *
//*                           SP0256 emulation module                          *
//*                  Copyright (C) 1999-2000 Joseph Zbiciak                    *
//*                Copyright (C) 2009-2010 Kalvis Duckmanton                   *
//*                                                                            *
//******************************************************************************
//
// This module emulates the General Instruments SP0256 speech processor.
//
//==============================================================================
/*
 *  uBee512 - An emulator for the Microbee Z80 ROM, FDD and HDD based models.
 *  Copyright (C) 2007-2024 uBee   
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
// v4.7.0 - 17 June 2010, K Duckmanton
// - Initial implementation, based on work by Joseph Zbiciak
//==============================================================================

//#define SINGLE_STEP
//#define DEBUG
#ifdef DEBUG
#define DPRINTF(x)                              \
   do {                                         \
    if (modio.beetalker)                        \
       {                                        \
        xprintf x ;                             \
       }                                        \
   } while (0)
#else
#define DPRINTF(x)
#endif

#undef HIGH_QUALITY
#define PER_PAUSE    (64)               /* Equiv timing period for pauses.  */
#define PER_NOISE    (64)               /* Equiv timing period for noise.   */

#define PAGESIZE 4096                   /* number of bytes in a ROM page */

static const char rcs_id[]="$Id: sp0256.c,v 1.1.1.4 2011/03/27 06:04:42 krd Exp $";

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "ubee512.h"
#include "roms.h"
#include "audio.h"
#include "support.h"
#include "sp0256.h"

extern modio_t modio;

/* ======================================================================== */
/*  Internal function prototypes.                                           */
/* ======================================================================== */
static inline uint32_t         bitrev(uint32_t val);
static void                    lpc12_regdec(lpc12_t *f);
static uint32_t                sp0256_getb(sp0256_t *ivoice, int len);

/* ======================================================================== */
/*  IVOICE_QTBL  -- Coefficient Quantization Table.  This comes from a      */
/*                  SP0250 data sheet, and should be correct for SP0256.    */
/* ======================================================================== */
static const int16_t qtbl[128] =
{
 0,      9,      12,     25,     33,     41,     49,     57,
 65,     73,     81,     89,     97,     105,    113,    121,
 129,    137,    145,    153,    161,    169,    177,    185,
 193,    201,    209,    217,    225,    233,    241,    249,
 257,    265,    273,    281,    289,    297,    301,    305,
 309,    313,    317,    321,    325,    329,    333,    337,
 341,    345,    349,    353,    357,    361,    365,    369,
 373,    377,    381,    385,    389,    393,    397,    401,
 405,    409,    413,    417,    421,    425,    427,    429,
 431,    433,    435,    437,    439,    441,    443,    445,
 447,    449,    451,    453,    455,    457,    459,    461,
 463,    465,    467,    469,    471,    473,    475,    477,
 479,    481,    482,    483,    484,    485,    486,    487,
 488,    489,    490,    491,    492,    493,    494,    495,
 496,    497,    498,    499,    500,    501,    502,    503,
 504,    505,    506,    507,    508,    509,    510,    511
};

/* ======================================================================== */
/*  LPC12_UPDATE     -- Update the 12-pole filter, outputting samples.      */
/* ======================================================================== */
int lpc12_update(lpc12_t *f, int num_samp, audio_circularbuf_t *cb)
{
 int i, j;
 int16_t samp;
 int do_int;

 /* -------------------------------------------------------------------- */
 /*  Iterate up to the desired number of samples.  We actually may       */
 /*  break out early if our repeat count expires.                        */
 /* -------------------------------------------------------------------- */
 for (i = 0; i < num_samp; i++)
    {
     /* ---------------------------------------------------------------- */
     /*  Generate a series of periodic impulses, or random noise.        */
     /* ---------------------------------------------------------------- */
     do_int = 0;
     samp   = 0;
     if (f->per)
        {
         if (f->cnt <= 0)
            {
             f->cnt += f->per;
             samp    = f->amp;
             f->rpt--;
             do_int  = f->interp;

             for (j = 0; j < 6; j++)
                f->z_data[j][0] = f->z_data[j][1] = 0;
            } else
               {
                samp = 0;
                f->cnt--;
               }

        } else
           {
            int bit;

            if (--f->cnt <= 0)
               {
                do_int = f->interp;
                f->cnt = PER_NOISE;
                f->rpt--;
                for (j = 0; j < 6; j++)
                   f->z_data[j][0] = f->z_data[j][1] = 0;
               }

            bit = f->rng & 1;
            f->rng = (f->rng >> 1) ^ (bit ? 0x14000 : 0);

            if (bit) { samp =  f->amp; }
            else     { samp = -f->amp; }
           }

     /* ---------------------------------------------------------------- */
     /*  If we need to, process the interpolation registers.             */
     /* ---------------------------------------------------------------- */
     if (do_int)
        {
         f->r[0] += f->r[14];
         f->r[1] += f->r[15];

         f->amp   = (f->r[0] & 0x1F) << (((f->r[0] & 0xE0) >> 5) + 0);
         f->per   = f->r[1];

         do_int   = 0;
        }

     /* ---------------------------------------------------------------- */
     /*  Stop if we expire our repeat counter and return the actual      */
     /*  number of samples we did.                                       */
     /* ---------------------------------------------------------------- */
     if (f->rpt <= 0) break;

     /* ---------------------------------------------------------------- */
     /*  Each 2nd order stage looks like one of these.  The App. Manual  */
     /*  gives the first form, the patent gives the second form.         */
     /*  They're equivalent except for time delay.  I implement the      */
     /*  first form.   (Note: 1/Z == 1 unit of time delay.)              */
     /*                                                                  */
     /*          ---->(+)-------->(+)----------+------->                 */
     /*                ^           ^           |                         */
     /*                |           |           |                         */
     /*                |           |           |                         */
     /*               [B]        [2*F]         |                         */
     /*                ^           ^           |                         */
     /*                |           |           |                         */
     /*                |           |           |                         */
     /*                +---[1/Z]<--+---[1/Z]<--+                         */
     /*                                                                  */
     /*                                                                  */
     /*                +---[2*F]<---+                                    */
     /*                |            |                                    */
     /*                |            |                                    */
     /*                v            |                                    */
     /*          ---->(+)-->[1/Z]-->+-->[1/Z]---+------>                 */
     /*                ^                        |                        */
     /*                |                        |                        */
     /*                |                        |                        */
     /*                +-----------[B]<---------+                        */
     /*                                                                  */
     /* ---------------------------------------------------------------- */
     for (j = 0; j < 6; j++)
        {
         samp += (((int)f->b_coef[j] * (int)f->z_data[j][1]) >> 9);
         samp += (((int)f->f_coef[j] * (int)f->z_data[j][0]) >> 8);

         f->z_data[j][1] = f->z_data[j][0];
         f->z_data[j][0] = samp;
        }

#ifdef HIGH_QUALITY /* Higher quality than the original, but who cares? */
     audio_circularbuf_put_sample(sc, AUDIO_CIRCULARBUF_MASK, limit(samp) << 2);
#else
     // NOTE! The maximum value of the sample needs to be kept in
     // mind here.  Maximum and minimum values are +/- 0xf80, this
     // needs to be scaled back by 4 or 5 bits in order to fit into
     // a char (esp. when offset by 128).  Division is used rather
     // than a bit shift in order to to preserve the sign
     audio_circularbuf_put_sample(cb, AUDIO_CIRCULARBUF_MASK, samp / (1<<4));
#endif
    }

 return i;
}

/*static int stage_map[6] = { 4, 2, 0, 5, 3, 1 };*/
/*static int stage_map[6] = { 3, 0, 4, 1, 5, 2 };*/
/*static int stage_map[6] = { 3, 0, 1, 4, 2, 5 };*/
static int stage_map[6] = { 0, 1, 2, 3, 4, 5 };

/* ======================================================================== */
/*  LPC12_REGDEC -- Decode the register set in the filter bank.             */
/* ======================================================================== */
static void lpc12_regdec(lpc12_t *f)
{
 int i;

 /* -------------------------------------------------------------------- */
 /*  Decode the Amplitude and Period registers.  Force the 'cnt' to 0    */
 /*  to get an initial impulse.  We compensate elsewhere by setting      */
 /*  the repeat count to "repeat + 1".                                   */
 /* -------------------------------------------------------------------- */
 f->amp = (f->r[0] & 0x1F) << (((f->r[0] & 0xE0) >> 5) + 0);
 f->cnt = 0;
 f->per = f->r[1];

 /* -------------------------------------------------------------------- */
 /*  Decode the filter coefficients from the quant table.                */
 /* -------------------------------------------------------------------- */
 for (i = 0; i < 6; i++)
    {
#define IQ(x) (((x) & 0x80) ? qtbl[0x7F & -(x)] : -qtbl[(x)])

     f->b_coef[stage_map[i]] = IQ(f->r[2 + 2*i]);
     f->f_coef[stage_map[i]] = IQ(f->r[3 + 2*i]);
    }

 /* -------------------------------------------------------------------- */
 /*  Set the Interp flag based on whether we have interpolation parms    */
 /* -------------------------------------------------------------------- */
 f->interp = f->r[14] || f->r[15];

 return;
}

/* ======================================================================== */
/*  MASK table                                                              */
/* ======================================================================== */
static const uint8_t mask[4097] =
{
 0xE8, 0xBB, 0xE8, 0x87, 0xE8, 0x17, 0xE8, 0x37, 0xE8, 0xF7, 0xE8, 0x8F,
 0xE8, 0xCF, 0xE2, 0xD8, 0xE2, 0x9A, 0xE2, 0x89, 0xE2, 0xDD, 0xE2, 0x37,
 0xE2, 0x2F, 0xEA, 0x04, 0xEA, 0x54, 0xEA, 0x4C, 0xEA, 0xD2, 0xEA, 0x8A,
 0xEA, 0x8E, 0xEA, 0xB1, 0xEA, 0xFD, 0xEA, 0x53, 0xEA, 0xAB, 0xEA, 0x47,
 0xEA, 0xCF, 0xEA, 0xFF, 0xE6, 0x10, 0xE6, 0x48, 0xE6, 0x3C, 0xE6, 0x62,
 0xE6, 0x8A, 0xE6, 0xBA, 0xE6, 0x76, 0xE6, 0x5E, 0xE6, 0xC1, 0xE6, 0xB1,
 0xE6, 0xCB, 0xEE, 0xC8, 0xEE, 0x98, 0xEE, 0xF8, 0xEE, 0xC2, 0xEE, 0x1E,
 0xEE, 0x7E, 0xEE, 0x2D, 0xEE, 0x6D, 0xEE, 0x1D, 0xEE, 0x5D, 0xEE, 0x3D,
 0x18, 0x2B, 0x15, 0xC0, 0x39, 0x24, 0x43, 0xE2, 0x1F, 0x00, 0x18, 0x23,
 0x24, 0xC0, 0x28, 0x23, 0x62, 0xC6, 0x1D, 0xA5, 0x03, 0x20, 0x66, 0x52,
 0x0C, 0x95, 0x03, 0x00, 0x19, 0x2C, 0x0C, 0x80, 0x31, 0x12, 0x62, 0xA7,
 0x1C, 0x00, 0x18, 0x2C, 0x0C, 0xC0, 0x29, 0x94, 0xE0, 0x64, 0x9C, 0x85,
 0x02, 0x38, 0x85, 0x12, 0x9C, 0x8C, 0x03, 0x00, 0x10, 0x35, 0xE7, 0x55,
 0xAD, 0x6D, 0x7F, 0x26, 0x91, 0x85, 0xD4, 0x3C, 0xAB, 0xD6, 0xCF, 0x99,
 0x7A, 0x00, 0x10, 0x34, 0x6F, 0xA1, 0x86, 0xCF, 0x3E, 0xAB, 0x0D, 0xBB,
 0x86, 0x7C, 0x6C, 0xB5, 0x6D, 0xCF, 0x24, 0xB2, 0x88, 0x9E, 0xA7, 0x16,
 0xF3, 0xA9, 0xD2, 0xE6, 0x3D, 0xD5, 0x55, 0xFD, 0x01, 0x00, 0x10, 0x32,
 0x74, 0x98, 0xA9, 0xB7, 0x81, 0x1E, 0xA9, 0x87, 0xF4, 0x66, 0xA3, 0xFC,
 0x8B, 0xD2, 0x96, 0x94, 0xFB, 0xFF, 0x10, 0x03, 0x80, 0x8E, 0x16, 0x0D,
 0x00, 0x10, 0x32, 0x7C, 0x90, 0xAB, 0xB7, 0x81, 0x1E, 0xA9, 0xA7, 0x6E,
 0xF7, 0x22, 0xDD, 0xC7, 0xAA, 0xFE, 0xA5, 0x9C, 0xDE, 0xCC, 0x7E, 0xF4,
 0x2E, 0xAC, 0xFA, 0xC7, 0xD9, 0x91, 0xA5, 0xA5, 0xE4, 0xDC, 0x5F, 0xF4,
 0x2B, 0x9D, 0xFC, 0x03, 0x00, 0x10, 0x31, 0x8F, 0xDC, 0xFF, 0x8C, 0x7C,
 0x97, 0xF6, 0x41, 0xE6, 0xE3, 0xF4, 0xF4, 0xF6, 0x47, 0x23, 0xC2, 0x84,
 0xB6, 0x85, 0x74, 0xFF, 0xD0, 0xDD, 0xCF, 0xEE, 0x3F, 0xB7, 0xEB, 0x01,
 0x00, 0x74, 0x7B, 0xA3, 0xDC, 0x2D, 0x3A, 0x5A, 0xB7, 0x56, 0xEE, 0x45,
 0xDF, 0x5B, 0xDA, 0xBF, 0x68, 0xE9, 0x3B, 0xFD, 0x1F, 0xF5, 0x78, 0x27,
 0xFF, 0xA2, 0x4E, 0xF2, 0xDC, 0x1F, 0x00, 0x10, 0x36, 0x76, 0x9B, 0xA9,
 0xB7, 0xBD, 0x1A, 0x1F, 0x66, 0xD4, 0x85, 0xA3, 0xBB, 0xCB, 0x95, 0x83,
 0x00, 0x10, 0x32, 0x6E, 0xDA, 0x27, 0xBB, 0x7D, 0x22, 0x1F, 0xC6, 0x94,
 0x16, 0x9C, 0xDE, 0x97, 0xD6, 0xA5, 0xD3, 0x7F, 0x52, 0x72, 0x58, 0xF2,
 0x4F, 0xD7, 0x85, 0x03, 0x00, 0x10, 0x32, 0x35, 0x96, 0xA9, 0xB9, 0xBD,
 0x1A, 0x1F, 0x86, 0xCE, 0x6E, 0x13, 0x3D, 0x09, 0xE9, 0xF6, 0x00, 0x10,
 0x32, 0x7B, 0x94, 0xAB, 0xB7, 0x81, 0x1E, 0xA9, 0x87, 0x6E, 0xAF, 0x1B,
 0xDD, 0xF9, 0xAA, 0xFE, 0xA4, 0x57, 0xE6, 0xCC, 0x5E, 0xF4, 0x36, 0xAD,
 0xFA, 0xC7, 0xD5, 0xB5, 0xA4, 0xA5, 0xED, 0xDC, 0x5F, 0xF4, 0x73, 0x9E,
 0xFC, 0x03, 0x00, 0x10, 0x32, 0xF7, 0x9F, 0xA9, 0xBD, 0x3F, 0x22, 0x11,
 0x86, 0x6E, 0xCF, 0xA3, 0xDB, 0xFB, 0x46, 0xEB, 0xC8, 0xE9, 0x3F, 0x00,
 0x10, 0x32, 0xAC, 0x98, 0x27, 0xBD, 0x81, 0x22, 0x1F, 0x87, 0xAE, 0x7E,
 0x1C, 0x6D, 0x81, 0xE7, 0xFF, 0x72, 0xE4, 0x20, 0x00, 0xF1, 0xE1, 0x00,
 0x00, 0x11, 0xFC, 0x13, 0xFF, 0x13, 0xFF, 0x00, 0xFE, 0x13, 0xFF, 0x00,
 0x11, 0xFF, 0x00, 0xFF, 0x00, 0xF7, 0x00, 0x18, 0x32, 0xDD, 0xA0, 0x7D,
 0x81, 0x0F, 0xC7, 0x03, 0xE3, 0xEA, 0x53, 0xC6, 0x75, 0xAB, 0xF0, 0x41,
 0xE8, 0x9E, 0x17, 0x73, 0xA1, 0xD2, 0xDC, 0x62, 0xF6, 0x14, 0x34, 0x4D,
 0x0F, 0x8C, 0xB7, 0x54, 0x99, 0x5A, 0xCB, 0x5F, 0x80, 0x84, 0x6D, 0x88,
 0xF3, 0x65, 0x2A, 0x73, 0xBD, 0xF5, 0x77, 0x50, 0xAD, 0x5D, 0xEF, 0xA1,
 0x5A, 0xF5, 0x45, 0x3C, 0x80, 0x53, 0x14, 0x83, 0xC8, 0xBC, 0xC9, 0x05,
 0x60, 0x09, 0x03, 0x68, 0xB0, 0xAF, 0xA9, 0x81, 0x00, 0x38, 0x78, 0xD8,
 0x8F, 0xD9, 0x61, 0xA2, 0x35, 0x77, 0x90, 0x7F, 0x07, 0xD3, 0xDA, 0x80,
 0xFF, 0xEC, 0xB4, 0x66, 0xDF, 0x31, 0xD8, 0xD8, 0x89, 0xBF, 0x65, 0x9B,
 0x9D, 0x5E, 0x82, 0x3E, 0x12, 0x24, 0x21, 0x6F, 0xFC, 0x24, 0x83, 0x03,
 0x00, 0xF2, 0xF3, 0x1F, 0x5C, 0x3E, 0x48, 0x90, 0x60, 0x0D, 0xEE, 0x03,
 0xA5, 0x8B, 0x00, 0x00, 0x1A, 0xFD, 0x38, 0x50, 0xA6, 0x00, 0xF0, 0x03,
 0x21, 0x6E, 0xC7, 0x8D, 0xD9, 0xF3, 0xA0, 0x30, 0xD2, 0x6F, 0x22, 0xF1,
 0x1A, 0x95, 0x71, 0x89, 0x0C, 0x44, 0x8A, 0xC6, 0xA7, 0xD1, 0x6B, 0xA2,
 0x33, 0xAF, 0x9A, 0x41, 0xD1, 0xCE, 0xFC, 0x2E, 0x3B, 0x4D, 0x74, 0xC6,
 0x24, 0x13, 0x18, 0x91, 0x61, 0x9E, 0x94, 0xD7, 0x75, 0xCE, 0xD4, 0x53,
 0x0A, 0x24, 0x2A, 0xDB, 0x8F, 0xF2, 0x34, 0xD0, 0x19, 0x5B, 0x6A, 0x80,
 0x64, 0x47, 0x79, 0xD7, 0x2D, 0xF7, 0x39, 0x53, 0x4B, 0x09, 0x90, 0xC8,
 0x68, 0x1F, 0xAB, 0xBD, 0x46, 0x69, 0xDA, 0x26, 0x85, 0x08, 0xA2, 0xFE,
 0x71, 0xF1, 0x55, 0xA9, 0xA4, 0x74, 0xE0, 0x87, 0x0F, 0x1E, 0x65, 0xCC,
 0xDC, 0x48, 0x06, 0x2C, 0x2A, 0xF3, 0xDB, 0xE6, 0xB8, 0x52, 0x9A, 0x7D,
 0xA8, 0xA0, 0x46, 0x85, 0x7E, 0x97, 0x0D, 0x47, 0x3A, 0x63, 0xFB, 0xD4,
 0x2B, 0xB0, 0x28, 0xBE, 0x50, 0xC2, 0x44, 0x67, 0xDE, 0xA1, 0x88, 0x16,
 0x19, 0xE6, 0x53, 0x39, 0x96, 0x28, 0x3F, 0x86, 0x49, 0x05, 0x80, 0xC7,
 0x06, 0x10, 0x49, 0x27, 0x71, 0x00, 0x10, 0xC9, 0xF8, 0x46, 0xDB, 0x33,
 0x5F, 0x51, 0xFB, 0x00, 0x0B, 0xCE, 0x76, 0x9F, 0x68, 0x36, 0xA6, 0x0D,
 0xB2, 0x67, 0xA8, 0x59, 0x19, 0xA6, 0x0A, 0xD8, 0x57, 0x2A, 0x30, 0x84,
 0x24, 0xE0, 0x22, 0x32, 0x8D, 0x6B, 0xB4, 0xCF, 0x60, 0xB3, 0xF4, 0xDF,
 0xDF, 0x82, 0xC5, 0xA0, 0x69, 0x91, 0x0C, 0x7A, 0x76, 0xAC, 0x1F, 0xC9,
 0x42, 0xAD, 0x32, 0xAF, 0x98, 0x41, 0x8B, 0x8A, 0xF5, 0x37, 0x59, 0x8A,
 0x75, 0xC6, 0xDE, 0x63, 0xC8, 0xD8, 0xC9, 0x1E, 0x57, 0xC3, 0x91, 0xCE,
 0xB8, 0x88, 0xEE, 0x15, 0x22, 0x8B, 0x13, 0x0E, 0xB3, 0xD0, 0x7D, 0x68,
 0x03, 0xF3, 0xFB, 0x18, 0x23, 0x1C, 0x00, 0x29, 0x18, 0x80, 0x2A, 0xB9,
 0xA6, 0x2E, 0x22, 0x20, 0xD9, 0xC1, 0x1D, 0x36, 0x63, 0x99, 0xCE, 0xD4,
 0x46, 0x04, 0x22, 0x33, 0xBA, 0xC7, 0x6A, 0xB6, 0xCE, 0xC9, 0xEF, 0xD7,
 0x0B, 0x24, 0x58, 0x44, 0xA7, 0xA1, 0x9D, 0xFA, 0x4D, 0x44, 0x12, 0x47,
 0x20, 0x5D, 0x9C, 0x32, 0x2F, 0x54, 0xC9, 0x0A, 0x13, 0xFA, 0x27, 0x3C,
 0xE9, 0x34, 0xE4, 0x02, 0xB0, 0x26, 0x52, 0x40, 0x98, 0x93, 0x58, 0x00,
 0xC5, 0x64, 0x8E, 0x86, 0x7B, 0x91, 0x07, 0x00, 0x93, 0x38, 0xD0, 0xF1,
 0x1F, 0xE2, 0x01, 0x58, 0xF3, 0x39, 0x70, 0x9E, 0x6B, 0xEC, 0x9E, 0x80,
 0x92, 0x1D, 0xFE, 0x6D, 0xF5, 0x9C, 0x67, 0x65, 0x09, 0xE0, 0x00, 0x00,
 0x00, 0xF1, 0xD0, 0xDC, 0x3C, 0x06, 0x1C, 0x4C, 0x6E, 0x07, 0xFC, 0xB1,
 0x54, 0x9A, 0xDA, 0xA7, 0x60, 0x41, 0xA4, 0xEB, 0x7D, 0xA1, 0x95, 0x2A,
 0xC3, 0x16, 0x11, 0x14, 0xD0, 0x6C, 0x0D, 0x1F, 0xA6, 0x50, 0x6B, 0x38,
 0x27, 0x82, 0x82, 0x99, 0x9D, 0xFF, 0xC7, 0x1C, 0xA3, 0x4C, 0x97, 0x34,
 0x50, 0x53, 0x95, 0x00, 0xAA, 0xE6, 0x91, 0x2D, 0x19, 0x00, 0x10, 0xF2,
 0x04, 0x2F, 0xDB, 0xD0, 0x06, 0xF1, 0x00, 0x10, 0x33, 0x66, 0xA6, 0x67,
 0x79, 0x85, 0x22, 0xA9, 0x87, 0xE6, 0x55, 0xB5, 0x6E, 0x00, 0x50, 0x24,
 0xF5, 0xCC, 0xBC, 0x67, 0x9E, 0xED, 0x0D, 0x8A, 0xA4, 0x9E, 0x51, 0x9B,
 0x6B, 0xF6, 0x5F, 0xBA, 0x97, 0xD1, 0xEE, 0x45, 0xCF, 0xBF, 0xB9, 0x3B,
 0x04, 0x8D, 0x39, 0xF9, 0xF9, 0x7C, 0xAE, 0x48, 0xEA, 0x11, 0x7D, 0x7B,
 0x69, 0xEE, 0xA5, 0xA6, 0x31, 0xBD, 0x3F, 0x1E, 0x00, 0x10, 0x33, 0x56,
 0x22, 0x47, 0x4D, 0x81, 0xAE, 0x92, 0x58, 0xC6, 0x85, 0x53, 0x68, 0xD1,
 0x6F, 0x95, 0xEE, 0xD7, 0xD8, 0x67, 0x1C, 0x35, 0xF4, 0xCE, 0x12, 0xF2,
 0x9A, 0xFB, 0x8D, 0xD8, 0x98, 0x20, 0x11, 0x86, 0x22, 0x7A, 0x3F, 0x5E,
 0xFD, 0x47, 0x5B, 0x57, 0xBB, 0xFF, 0x28, 0x4B, 0x6B, 0xF9, 0x1F, 0x2D,
 0x8F, 0xED, 0xFE, 0xF1, 0x00, 0xD0, 0x56, 0x10, 0x33, 0xEE, 0xD4, 0xE5,
 0xF9, 0xBF, 0x23, 0x2D, 0x67, 0xB4, 0xD5, 0x92, 0xDB, 0x97, 0xB6, 0x68,
 0x52, 0xFB, 0xD1, 0xF2, 0x4F, 0x62, 0x4F, 0xFA, 0x71, 0xCA, 0xEB, 0x47,
 0x39, 0x5F, 0x69, 0xFD, 0xE8, 0x83, 0x2D, 0xAB, 0x8F, 0x07, 0x00, 0xD0,
 0x3E, 0x18, 0x33, 0xED, 0x5E, 0xF9, 0x82, 0x8A, 0xD2, 0x03, 0x03, 0xEB,
 0x14, 0xC2, 0xA6, 0x5D, 0x33, 0xB5, 0x26, 0xD7, 0xE2, 0xC2, 0x90, 0xD6,
 0x86, 0xB4, 0xFB, 0xD1, 0x96, 0x76, 0xFA, 0x4F, 0x67, 0x3A, 0x63, 0xC8,
 0x90, 0xDA, 0xF6, 0x1E, 0x35, 0xB2, 0x07, 0x90, 0xAF, 0xCC, 0x78, 0x00,
 0xD0, 0x61, 0xD0, 0x19, 0xD0, 0x55, 0xF1, 0x00, 0xD0, 0x61, 0x10, 0x37,
 0x76, 0x99, 0xAD, 0xB3, 0x7F, 0x1E, 0xA2, 0xA7, 0x74, 0x8F, 0xB3, 0x1A,
 0xCC, 0xED, 0x8D, 0xA4, 0x37, 0xA8, 0xDD, 0x9F, 0xEE, 0x9E, 0x1D, 0x75,
 0x71, 0x29, 0xF7, 0xA2, 0x66, 0x30, 0xDD, 0x7E, 0xE5, 0x00, 0x98, 0x23,
 0xC2, 0xC7, 0x03, 0x00, 0xD0, 0x06, 0xD0, 0x06, 0xD0, 0x53, 0xD0, 0x06,
 0xF1, 0x00, 0xD0, 0x06, 0xD0, 0x06, 0xD0, 0xA7, 0xF1, 0x00, 0x10, 0x32,
 0xF6, 0x9F, 0xA9, 0xBD, 0x3F, 0x22, 0x11, 0x86, 0x6E, 0xCF, 0xA3, 0xBB,
 0xFB, 0x46, 0xEB, 0xC8, 0xE9, 0xFF, 0x3D, 0xB4, 0x15, 0xF1, 0x00, 0xD8,
 0xB0, 0xD8, 0xB4, 0xF1, 0x00, 0xD0, 0x56, 0x10, 0x34, 0x76, 0x9B, 0xAB,
 0xB9, 0xBD, 0x15, 0x1F, 0x87, 0xEE, 0xC6, 0x1B, 0xB5, 0x3B, 0xEB, 0xFE,
 0xA3, 0xA5, 0xED, 0xDC, 0x9F, 0x8E, 0xBC, 0x9D, 0xEB, 0x96, 0xE3, 0x01,
 0x00, 0x10, 0x32, 0x6D, 0xA0, 0xA7, 0xBF, 0x81, 0x15, 0x1F, 0xCA, 0xB4,
 0xB6, 0x9B, 0x1E, 0x88, 0x96, 0x7D, 0x53, 0xFF, 0xD3, 0x77, 0x8E, 0x6A,
 0x00, 0x7D, 0x0A, 0xF1, 0x00, 0xD0, 0x56, 0x10, 0x32, 0x9C, 0xA0, 0xA9,
 0x2D, 0xBF, 0x22, 0x1F, 0x68, 0xF4, 0xF4, 0xA3, 0xF8, 0x93, 0xDE, 0x80,
 0x55, 0x7F, 0xD3, 0xDA, 0xAF, 0xE6, 0x4F, 0x4A, 0x03, 0x56, 0x1C, 0x4A,
 0xCD, 0x3C, 0x7A, 0x43, 0x9C, 0x99, 0x77, 0x4A, 0xF9, 0xCD, 0x0B, 0x4A,
 0x06, 0x00, 0x53, 0x26, 0x78, 0x3C, 0x00, 0xD0, 0x3E, 0xD8, 0xD2, 0xFE,
 0xD0, 0x56, 0xD8, 0xBA, 0xF1, 0x00, 0xD0, 0x61, 0xD0, 0x55, 0xF3, 0xD0,
 0x56, 0xD8, 0xBA, 0xF1, 0x00, 0xD0, 0x61, 0xD8, 0x9E, 0xD0, 0x61, 0xF5,
 0xD0, 0x56, 0xD8, 0xBA, 0xF1, 0x00, 0xD0, 0x06, 0xD0, 0x06, 0xD0, 0x53,
 0xD0, 0x06, 0xD0, 0x06, 0xF4, 0xD0, 0x56, 0xD8, 0xBA, 0xF1, 0x00, 0xD0,
 0x06, 0xD0, 0x06, 0xD8, 0xD1, 0xD0, 0x56, 0xD8, 0xBA, 0xF1, 0x00, 0xD8,
 0xCD, 0xFE, 0xD0, 0x56, 0xD8, 0xBA, 0xF1, 0x00, 0xD8, 0xB0, 0xD8, 0xB4,
 0xD0, 0x56, 0xD8, 0xBA, 0xF1, 0x00, 0xD0, 0x56, 0x10, 0x32, 0x6D, 0x93,
 0xAB, 0xB1, 0xBF, 0x1A, 0x1F, 0x46, 0xEE, 0xED, 0x1A, 0xAD, 0xC7, 0x6A,
 0xF6, 0xA2, 0x35, 0x5B, 0xDD, 0x9F, 0xF4, 0xA4, 0x9B, 0xFC, 0xDB, 0x8B,
 0x3C, 0x00, 0x87, 0x60, 0xF6, 0x7A, 0x68, 0x2B, 0xD8, 0x13, 0xF1, 0x00,
 0xD0, 0x3E, 0xD8, 0xD2, 0xD0, 0x56, 0xD8, 0x13, 0xF1, 0x00, 0xD0, 0x61,
 0xD0, 0x55, 0xF3, 0xD0, 0x56, 0xD8, 0x13, 0xF1, 0x00, 0xD0, 0x61, 0xD8,
 0x9E, 0xD0, 0x61, 0xD0, 0x56, 0xD8, 0x13, 0xF1, 0x00, 0xD0, 0x06, 0xD0,
 0x06, 0xD0, 0x53, 0xD0, 0x06, 0xD0, 0x06, 0xF4, 0xD0, 0x56, 0xD8, 0x13,
 0xF1, 0x00, 0xD0, 0x06, 0xD0, 0x06, 0xD8, 0xD1, 0xD0, 0x56, 0xD8, 0x13,
 0xF1, 0x00, 0xD8, 0xCD, 0xF7, 0xD0, 0x56, 0xD8, 0x13, 0xF1, 0x00, 0xD8,
 0xB0, 0xD8, 0xB4, 0xD0, 0x56, 0xD8, 0x13, 0xF1, 0x00, 0x10, 0x25, 0x02,
 0xC0, 0x10, 0x97, 0xBC, 0xA4, 0x01, 0xA8, 0x02, 0x93, 0xCF, 0xD8, 0x7D,
 0xB6, 0xD6, 0xFE, 0x6A, 0x7C, 0x1C, 0xD2, 0x1D, 0xD0, 0xEE, 0x3F, 0x5A,
 0xFE, 0x4D, 0xFD, 0x47, 0x4B, 0xC6, 0xB9, 0xFF, 0x88, 0x03, 0x20, 0x43,
 0x27, 0x97, 0xE9, 0x40, 0x3D, 0xBD, 0xED, 0xD5, 0xF8, 0x38, 0xA3, 0x2E,
 0x24, 0xDD, 0x5D, 0xF4, 0xCD, 0xA4, 0xDB, 0x8F, 0xBA, 0x95, 0x74, 0xFF,
 0xD1, 0x8E, 0x72, 0xEE, 0x1F, 0x0F, 0x00, 0xD0, 0x3E, 0x10, 0x35, 0x37,
 0x9A, 0xAB, 0xB5, 0xBF, 0x1A, 0x1F, 0xC7, 0x74, 0x4F, 0xB3, 0xFA, 0x97,
 0xBE, 0x7E, 0x15, 0x03, 0x52, 0x33, 0x93, 0x66, 0x60, 0x52, 0x00, 0xAC,
 0xF1, 0x06, 0x4E, 0x1A, 0x80, 0x3B, 0x06, 0xC5, 0x0C, 0xF7, 0xEA, 0x69,
 0xED, 0xAF, 0xC6, 0xC7, 0x21, 0xED, 0x90, 0xE7, 0x06, 0xA2, 0x15, 0xF6,
 0xD4, 0x7F, 0x3E, 0xA4, 0x00, 0x48, 0xE3, 0x91, 0xC7, 0x03, 0x00, 0xD0,
 0x56, 0xD8, 0xBA, 0xF1, 0x00, 0xD0, 0x56, 0xD8, 0x13, 0xF1, 0x00, 0x10,
 0x28, 0x1D, 0xC0, 0x18, 0x1D, 0x7C, 0x86, 0xDC, 0x33, 0xB5, 0x2E, 0x4F,
 0xE3, 0xD2, 0x8C, 0xD6, 0x7F, 0x75, 0xF7, 0x51, 0x1B, 0xB1, 0x6E, 0x3F,
 0x7A, 0xFB, 0xD5, 0xFD, 0xA1, 0x0D, 0x00, 0xD0, 0x06, 0xF1, 0x00, 0x10,
 0x34, 0x76, 0x9C, 0xA9, 0xBB, 0x7F, 0x1D, 0x22, 0x68, 0x74, 0x7F, 0xAB,
 0xFC, 0x8F, 0xB2, 0x77, 0x73, 0xFF, 0x99, 0xCB, 0x30, 0x62, 0xC7, 0x5F,
 0x53, 0x82, 0x9E, 0x4F, 0xE2, 0x01, 0x58, 0xF2, 0xF1, 0x67, 0x4C, 0x44,
 0x53, 0x6F, 0xFB, 0x3A, 0x44, 0x90, 0xA8, 0xE9, 0x4B, 0x77, 0x97, 0x2B,
 0xD1, 0xE3, 0x01, 0x00, 0xD0, 0x19, 0xD0, 0x55, 0xF1, 0x00, 0x10, 0x32,
 0xB4, 0xA9, 0xA9, 0xBB, 0x7F, 0x1D, 0x22, 0x48, 0xEE, 0x96, 0x0D, 0xDD,
 0x8F, 0x6B, 0xFF, 0x72, 0xBB, 0x73, 0xE8, 0x1E, 0x6D, 0xF9, 0x17, 0x7D,
 0x69, 0xEB, 0xFE, 0xA1, 0x2C, 0xE3, 0xDC, 0x60, 0xF4, 0xB4, 0x9B, 0x1A,
 0xC4, 0x9D, 0x69, 0x73, 0x56, 0x9B, 0xA8, 0x4B, 0x45, 0x37, 0x88, 0x63,
 0xAB, 0xE2, 0x01, 0x00, 0xF1, 0x00, 0xF1, 0x00, 0xF1, 0x00, 0xF1, 0x00,
 0xF1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* ======================================================================== */
/*  SP0256_DATAFMT   -- Data format table for the SP0256's microcontroller  */
/*                                                                          */
/*  len     4 bits      Length of field to extract                          */
/*  lshift  4 bits      Left-shift amount on field                          */
/*  param   4 bits      Parameter number being updated                      */
/*  delta   1 bit       This is a delta-update.  (Implies sign-extend)      */
/*  field   1 bit       This is a field replace.                            */
/*  clr5    1 bit       Clear F5, B5.                                       */
/*  clrall  1 bit       Clear all before doing this update                  */
/* ======================================================================== */

#define CR(l,s,p,d,f,c5,ca)         \
        (                           \
            (((l)  & 15) <<  0) |   \
            (((s)  & 15) <<  4) |   \
            (((p)  & 15) <<  8) |   \
            (((d)  &  1) << 12) |   \
            (((f)  &  1) << 13) |   \
            (((c5) &  1) << 14) |   \
            (((ca) &  1) << 15)     \
        )

#define CR_DELTA  CR(0,0,0,1,0,0,0)
#define CR_FIELD  CR(0,0,0,0,1,0,0)
#define CR_CLR5   CR(0,0,0,0,0,1,0)
#define CR_CLRA   CR(0,0,0,0,0,0,1)
#define CR_LEN(x) ((x) & 15)
#define CR_SHF(x) (((x) >> 4) & 15)
#define CR_PRM(x) (((x) >> 8) & 15)

enum { AM = 0, PR, B0, F0, B1, F1, B2, F2, B3, F3, B4, F4, B5, F5, IA, IP };

static const uint16_t sp0256_datafmt[] =
{
 /* -------------------------------------------------------------------- */
 /*  OPCODE 1111: PAUSE                                                  */
 /* -------------------------------------------------------------------- */
 /*    0 */  CR( 0,  0,  0,  0,  0,  0,  1),     /*  Clear all   */

 /* -------------------------------------------------------------------- */
 /*  Opcode 0001: LOADALL                                                */
 /* -------------------------------------------------------------------- */
 /* All modes                */
 /*    1 */  CR( 8,  0,  AM, 0,  0,  0,  1),     /*  Amplitude   */
 /*    2 */  CR( 8,  0,  PR, 0,  0,  0,  0),     /*  Period      */
 /*    3 */  CR( 8,  0,  B0, 0,  0,  0,  0),     /*  B0          */
 /*    4 */  CR( 8,  0,  F0, 0,  0,  0,  0),     /*  F0          */
 /*    5 */  CR( 8,  0,  B1, 0,  0,  0,  0),     /*  B1          */
 /*    6 */  CR( 8,  0,  F1, 0,  0,  0,  0),     /*  F1          */
 /*    7 */  CR( 8,  0,  B2, 0,  0,  0,  0),     /*  B2          */
 /*    8 */  CR( 8,  0,  F2, 0,  0,  0,  0),     /*  F2          */
 /*    9 */  CR( 8,  0,  B3, 0,  0,  0,  0),     /*  B3          */
 /*   10 */  CR( 8,  0,  F3, 0,  0,  0,  0),     /*  F3          */
 /*   11 */  CR( 8,  0,  B4, 0,  0,  0,  0),     /*  B4          */
 /*   12 */  CR( 8,  0,  F4, 0,  0,  0,  0),     /*  F4          */
 /*   13 */  CR( 8,  0,  B5, 0,  0,  0,  0),     /*  B5          */
 /*   14 */  CR( 8,  0,  F5, 0,  0,  0,  0),     /*  F5          */
 /* Mode 01 and 11 only      */
 /*   15 */  CR( 8,  0,  IA, 0,  0,  0,  0),     /*  Amp Interp  */
 /*   16 */  CR( 8,  0,  IP, 0,  0,  0,  0),     /*  Pit Interp  */

 /* -------------------------------------------------------------------- */
 /*  Opcode 0100: LOAD_4                                                 */
 /* -------------------------------------------------------------------- */
 /* Mode 00 and 01           */
 /*   17 */  CR( 6,  2,  AM, 0,  0,  0,  1),     /*  Amplitude   */
 /*   18 */  CR( 8,  0,  PR, 0,  0,  0,  0),     /*  Period      */
 /*   19 */  CR( 4,  3,  B3, 0,  0,  0,  0),     /*  B3 (S=0)    */
 /*   20 */  CR( 6,  2,  F3, 0,  0,  0,  0),     /*  F3          */
 /*   21 */  CR( 7,  1,  B4, 0,  0,  0,  0),     /*  B4          */
 /*   22 */  CR( 6,  2,  F4, 0,  0,  0,  0),     /*  F4          */
 /* Mode 01 only             */
 /*   23 */  CR( 8,  0,  B5, 0,  0,  0,  0),     /*  B5          */
 /*   24 */  CR( 8,  0,  F5, 0,  0,  0,  0),     /*  F5          */

 /* Mode 10 and 11           */
 /*   25 */  CR( 6,  2,  AM, 0,  0,  0,  1),     /*  Amplitude   */
 /*   26 */  CR( 8,  0,  PR, 0,  0,  0,  0),     /*  Period      */
 /*   27 */  CR( 6,  1,  B3, 0,  0,  0,  0),     /*  B3 (S=0)    */
 /*   28 */  CR( 7,  1,  F3, 0,  0,  0,  0),     /*  F3          */
 /*   29 */  CR( 8,  0,  B4, 0,  0,  0,  0),     /*  B4          */
 /*   30 */  CR( 8,  0,  F4, 0,  0,  0,  0),     /*  F4          */
 /* Mode 11 only             */
 /*   31 */  CR( 8,  0,  B5, 0,  0,  0,  0),     /*  B5          */
 /*   32 */  CR( 8,  0,  F5, 0,  0,  0,  0),     /*  F5          */

 /* -------------------------------------------------------------------- */
 /*  Opcode 0110: SETMSB_6                                               */
 /* -------------------------------------------------------------------- */
 /* Mode 00 only             */
 /*   33 */  CR( 0,  0,  0,  0,  0,  1,  0),     /*  Clear 5     */
 /* Mode 00 and 01           */
 /*   34 */  CR( 6,  2,  AM, 0,  0,  0,  0),     /*  Amplitude   */
 /*   35 */  CR( 6,  2,  F3, 0,  1,  0,  0),     /*  F3 (5 MSBs) */
 /*   36 */  CR( 6,  2,  F4, 0,  1,  0,  0),     /*  F4 (5 MSBs) */
 /* Mode 01 only             */
 /*   37 */  CR( 8,  0,  F5, 0,  1,  0,  0),     /*  F5 (5 MSBs) */

 /* Mode 10 only             */
 /*   38 */  CR( 0,  0,  0,  0,  0,  1,  0),     /*  Clear 5     */
 /* Mode 10 and 11           */
 /*   39 */  CR( 6,  2,  AM, 0,  0,  0,  0),     /*  Amplitude   */
 /*   40 */  CR( 7,  1,  F3, 0,  1,  0,  0),     /*  F3 (6 MSBs) */
 /*   41 */  CR( 8,  0,  F4, 0,  1,  0,  0),     /*  F4 (6 MSBs) */
 /* Mode 11 only             */
 /*   42 */  CR( 8,  0,  F5, 0,  1,  0,  0),     /*  F5 (6 MSBs) */

 /*   43 */  0,  /* unused */
 /*   44 */  0,  /* unused */

 /* -------------------------------------------------------------------- */
 /*  Opcode 1001: DELTA_9                                                */
 /* -------------------------------------------------------------------- */
 /* Mode 00 and 01           */
 /*   45 */  CR( 4,  2,  AM, 1,  0,  0,  0),     /*  Amplitude   */
 /*   46 */  CR( 5,  0,  PR, 1,  0,  0,  0),     /*  Period      */
 /*   47 */  CR( 3,  4,  B0, 1,  0,  0,  0),     /*  B0 4 MSBs   */
 /*   48 */  CR( 3,  3,  F0, 1,  0,  0,  0),     /*  F0 5 MSBs   */
 /*   49 */  CR( 3,  4,  B1, 1,  0,  0,  0),     /*  B1 4 MSBs   */
 /*   50 */  CR( 3,  3,  F1, 1,  0,  0,  0),     /*  F1 5 MSBs   */
 /*   51 */  CR( 3,  4,  B2, 1,  0,  0,  0),     /*  B2 4 MSBs   */
 /*   52 */  CR( 3,  3,  F2, 1,  0,  0,  0),     /*  F2 5 MSBs   */
 /*   53 */  CR( 3,  3,  B3, 1,  0,  0,  0),     /*  B3 5 MSBs   */
 /*   54 */  CR( 4,  2,  F3, 1,  0,  0,  0),     /*  F3 6 MSBs   */
 /*   55 */  CR( 4,  1,  B4, 1,  0,  0,  0),     /*  B4 7 MSBs   */
 /*   56 */  CR( 4,  2,  F4, 1,  0,  0,  0),     /*  F4 6 MSBs   */
 /* Mode 01 only             */
 /*   57 */  CR( 5,  0,  B5, 1,  0,  0,  0),     /*  B5 8 MSBs   */
 /*   58 */  CR( 5,  0,  F5, 1,  0,  0,  0),     /*  F5 8 MSBs   */

 /* Mode 10 and 11           */
 /*   59 */  CR( 4,  2,  AM, 1,  0,  0,  0),     /*  Amplitude   */
 /*   60 */  CR( 5,  0,  PR, 1,  0,  0,  0),     /*  Period      */
 /*   61 */  CR( 4,  1,  B0, 1,  0,  0,  0),     /*  B0 7 MSBs   */
 /*   62 */  CR( 4,  2,  F0, 1,  0,  0,  0),     /*  F0 6 MSBs   */
 /*   63 */  CR( 4,  1,  B1, 1,  0,  0,  0),     /*  B1 7 MSBs   */
 /*   64 */  CR( 4,  2,  F1, 1,  0,  0,  0),     /*  F1 6 MSBs   */
 /*   65 */  CR( 4,  1,  B2, 1,  0,  0,  0),     /*  B2 7 MSBs   */
 /*   66 */  CR( 4,  2,  F2, 1,  0,  0,  0),     /*  F2 6 MSBs   */
 /*   67 */  CR( 4,  1,  B3, 1,  0,  0,  0),     /*  B3 7 MSBs   */
 /*   68 */  CR( 5,  1,  F3, 1,  0,  0,  0),     /*  F3 7 MSBs   */
 /*   69 */  CR( 5,  0,  B4, 1,  0,  0,  0),     /*  B4 8 MSBs   */
 /*   70 */  CR( 5,  0,  F4, 1,  0,  0,  0),     /*  F4 8 MSBs   */
 /* Mode 11 only             */
 /*   71 */  CR( 5,  0,  B5, 1,  0,  0,  0),     /*  B5 8 MSBs   */
 /*   72 */  CR( 5,  0,  F5, 1,  0,  0,  0),     /*  F5 8 MSBs   */

 /* -------------------------------------------------------------------- */
 /*  Opcode 1010: SETMSB_A                                               */
 /* -------------------------------------------------------------------- */
 /* Mode 00 only             */
 /*   73 */  CR( 0,  0,  0,  0,  0,  1,  0),     /*  Clear 5     */
 /* Mode 00 and 01           */
 /*   74 */  CR( 6,  2,  AM, 0,  0,  0,  0),     /*  Amplitude   */
 /*   75 */  CR( 5,  3,  F0, 0,  1,  0,  0),     /*  F0 (5 MSBs) */
 /*   76 */  CR( 5,  3,  F1, 0,  1,  0,  0),     /*  F1 (5 MSBs) */
 /*   77 */  CR( 5,  3,  F2, 0,  1,  0,  0),     /*  F2 (5 MSBs) */

 /* Mode 10 only             */
 /*   78 */  CR( 0,  0,  0,  0,  0,  1,  0),     /*  Clear 5     */
 /* Mode 10 and 11           */
 /*   79 */  CR( 6,  2,  AM, 0,  0,  0,  0),     /*  Amplitude   */
 /*   80 */  CR( 6,  2,  F0, 0,  1,  0,  0),     /*  F0 (6 MSBs) */
 /*   81 */  CR( 6,  2,  F1, 0,  1,  0,  0),     /*  F1 (6 MSBs) */
 /*   82 */  CR( 6,  2,  F2, 0,  1,  0,  0),     /*  F2 (6 MSBs) */

 /* -------------------------------------------------------------------- */
 /*  Opcode 0010: LOAD_2  Mode 00 and 10                                 */
 /*  Opcode 1100: LOAD_C  Mode 00 and 10                                 */
 /* -------------------------------------------------------------------- */
 /* LOAD_2, LOAD_C  Mode 00  */
 /*   83 */  CR( 6,  2,  AM, 0,  0,  0,  1),     /*  Amplitude   */
 /*   84 */  CR( 8,  0,  PR, 0,  0,  0,  0),     /*  Period      */
 /*   85 */  CR( 3,  4,  B0, 0,  0,  0,  0),     /*  B0 (S=0)    */
 /*   86 */  CR( 5,  3,  F0, 0,  0,  0,  0),     /*  F0          */
 /*   87 */  CR( 3,  4,  B1, 0,  0,  0,  0),     /*  B1 (S=0)    */
 /*   88 */  CR( 5,  3,  F1, 0,  0,  0,  0),     /*  F1          */
 /*   89 */  CR( 3,  4,  B2, 0,  0,  0,  0),     /*  B2 (S=0)    */
 /*   90 */  CR( 5,  3,  F2, 0,  0,  0,  0),     /*  F2          */
 /*   91 */  CR( 4,  3,  B3, 0,  0,  0,  0),     /*  B3 (S=0)    */
 /*   92 */  CR( 6,  2,  F3, 0,  0,  0,  0),     /*  F3          */
 /*   93 */  CR( 7,  1,  B4, 0,  0,  0,  0),     /*  B4          */
 /*   94 */  CR( 6,  2,  F4, 0,  0,  0,  0),     /*  F4          */
 /* LOAD_2 only              */
 /*   95 */  CR( 5,  0,  IA, 0,  0,  0,  0),     /*  Ampl. Intr. */
 /*   96 */  CR( 5,  0,  IP, 0,  0,  0,  0),     /*  Per. Intr.  */

 /* LOAD_2, LOAD_C  Mode 10  */
 /*   97 */  CR( 6,  2,  AM, 0,  0,  0,  1),     /*  Amplitude   */
 /*   98 */  CR( 8,  0,  PR, 0,  0,  0,  0),     /*  Period      */
 /*   99 */  CR( 6,  1,  B0, 0,  0,  0,  0),     /*  B0 (S=0)    */
 /*  100 */  CR( 6,  2,  F0, 0,  0,  0,  0),     /*  F0          */
 /*  101 */  CR( 6,  1,  B1, 0,  0,  0,  0),     /*  B1 (S=0)    */
 /*  102 */  CR( 6,  2,  F1, 0,  0,  0,  0),     /*  F1          */
 /*  103 */  CR( 6,  1,  B2, 0,  0,  0,  0),     /*  B2 (S=0)    */
 /*  104 */  CR( 6,  2,  F2, 0,  0,  0,  0),     /*  F2          */
 /*  105 */  CR( 6,  1,  B3, 0,  0,  0,  0),     /*  B3 (S=0)    */
 /*  106 */  CR( 7,  1,  F3, 0,  0,  0,  0),     /*  F3          */
 /*  107 */  CR( 8,  0,  B4, 0,  0,  0,  0),     /*  B4          */
 /*  108 */  CR( 8,  0,  F4, 0,  0,  0,  0),     /*  F4          */
 /* LOAD_2 only              */
 /*  109 */  CR( 5,  0,  IA, 0,  0,  0,  0),     /*  Ampl. Intr. */
 /*  110 */  CR( 5,  0,  IP, 0,  0,  0,  0),     /*  Per. Intr.  */

 /* -------------------------------------------------------------------- */
 /*  OPCODE 1101: DELTA_D                                                */
 /* -------------------------------------------------------------------- */
 /* Mode 00 and 01           */
 /*  111 */  CR( 4,  2,  AM, 1,  0,  0,  0),     /*  Amplitude   */
 /*  112 */  CR( 5,  0,  PR, 1,  0,  0,  0),     /*  Period      */
 /*  113 */  CR( 3,  3,  B3, 1,  0,  0,  0),     /*  B3 5 MSBs   */
 /*  114 */  CR( 4,  2,  F3, 1,  0,  0,  0),     /*  F3 6 MSBs   */
 /*  115 */  CR( 4,  1,  B4, 1,  0,  0,  0),     /*  B4 7 MSBs   */
 /*  116 */  CR( 4,  2,  F4, 1,  0,  0,  0),     /*  F4 6 MSBs   */
 /* Mode 01 only             */
 /*  117 */  CR( 5,  0,  B5, 1,  0,  0,  0),     /*  B5 8 MSBs   */
 /*  118 */  CR( 5,  0,  F5, 1,  0,  0,  0),     /*  F5 8 MSBs   */

 /* Mode 10 and 11           */
 /*  119 */  CR( 4,  2,  AM, 1,  0,  0,  0),     /*  Amplitude   */
 /*  120 */  CR( 5,  0,  PR, 1,  0,  0,  0),     /*  Period      */
 /*  121 */  CR( 4,  1,  B3, 1,  0,  0,  0),     /*  B3 7 MSBs   */
 /*  122 */  CR( 5,  1,  F3, 1,  0,  0,  0),     /*  F3 7 MSBs   */
 /*  123 */  CR( 5,  0,  B4, 1,  0,  0,  0),     /*  B4 8 MSBs   */
 /*  124 */  CR( 5,  0,  F4, 1,  0,  0,  0),     /*  F4 8 MSBs   */
 /* Mode 11 only             */
 /*  125 */  CR( 5,  0,  B5, 1,  0,  0,  0),     /*  B5 8 MSBs   */
 /*  126 */  CR( 5,  0,  F5, 1,  0,  0,  0),     /*  F5 8 MSBs   */

 /* -------------------------------------------------------------------- */
 /*  OPCODE 1110: LOAD_E                                                 */
 /* -------------------------------------------------------------------- */
 /*  127 */  CR( 6,  2,  AM, 0,  0,  0,  0),     /*  Amplitude   */
 /*  128 */  CR( 8,  0,  PR, 0,  0,  0,  0),     /*  Period      */

 /* -------------------------------------------------------------------- */
 /*  Opcode 0010: LOAD_2  Mode 01 and 11                                 */
 /*  Opcode 1100: LOAD_C  Mode 01 and 11                                 */
 /* -------------------------------------------------------------------- */
 /* LOAD_2, LOAD_C  Mode 01  */
 /*  129 */  CR( 6,  2,  AM, 0,  0,  0,  1),     /*  Amplitude   */
 /*  130 */  CR( 8,  0,  PR, 0,  0,  0,  0),     /*  Period      */
 /*  131 */  CR( 3,  4,  B0, 0,  0,  0,  0),     /*  B0 (S=0)    */
 /*  132 */  CR( 5,  3,  F0, 0,  0,  0,  0),     /*  F0          */
 /*  133 */  CR( 3,  4,  B1, 0,  0,  0,  0),     /*  B1 (S=0)    */
 /*  134 */  CR( 5,  3,  F1, 0,  0,  0,  0),     /*  F1          */
 /*  135 */  CR( 3,  4,  B2, 0,  0,  0,  0),     /*  B2 (S=0)    */
 /*  136 */  CR( 5,  3,  F2, 0,  0,  0,  0),     /*  F2          */
 /*  137 */  CR( 4,  3,  B3, 0,  0,  0,  0),     /*  B3 (S=0)    */
 /*  138 */  CR( 6,  2,  F3, 0,  0,  0,  0),     /*  F3          */
 /*  139 */  CR( 7,  1,  B4, 0,  0,  0,  0),     /*  B4          */
 /*  140 */  CR( 6,  2,  F4, 0,  0,  0,  0),     /*  F4          */
 /*  141 */  CR( 8,  0,  B5, 0,  0,  0,  0),     /*  B5          */
 /*  142 */  CR( 8,  0,  F5, 0,  0,  0,  0),     /*  F5          */
 /* LOAD_2 only              */
 /*  143 */  CR( 5,  0,  IA, 0,  0,  0,  0),     /*  Ampl. Intr. */
 /*  144 */  CR( 5,  0,  IP, 0,  0,  0,  0),     /*  Per. Intr.  */

 /* LOAD_2, LOAD_C  Mode 11  */
 /*  145 */  CR( 6,  2,  AM, 0,  0,  0,  1),     /*  Amplitude   */
 /*  146 */  CR( 8,  0,  PR, 0,  0,  0,  0),     /*  Period      */
 /*  147 */  CR( 6,  1,  B0, 0,  0,  0,  0),     /*  B0 (S=0)    */
 /*  148 */  CR( 6,  2,  F0, 0,  0,  0,  0),     /*  F0          */
 /*  149 */  CR( 6,  1,  B1, 0,  0,  0,  0),     /*  B1 (S=0)    */
 /*  150 */  CR( 6,  2,  F1, 0,  0,  0,  0),     /*  F1          */
 /*  151 */  CR( 6,  1,  B2, 0,  0,  0,  0),     /*  B2 (S=0)    */
 /*  152 */  CR( 6,  2,  F2, 0,  0,  0,  0),     /*  F2          */
 /*  153 */  CR( 6,  1,  B3, 0,  0,  0,  0),     /*  B3 (S=0)    */
 /*  154 */  CR( 7,  1,  F3, 0,  0,  0,  0),     /*  F3          */
 /*  155 */  CR( 8,  0,  B4, 0,  0,  0,  0),     /*  B4          */
 /*  156 */  CR( 8,  0,  F4, 0,  0,  0,  0),     /*  F4          */
 /*  157 */  CR( 8,  0,  B5, 0,  0,  0,  0),     /*  B5          */
 /*  158 */  CR( 8,  0,  F5, 0,  0,  0,  0),     /*  F5          */
 /* LOAD_2 only              */
 /*  159 */  CR( 5,  0,  IA, 0,  0,  0,  0),     /*  Ampl. Intr. */
 /*  160 */  CR( 5,  0,  IP, 0,  0,  0,  0),     /*  Per. Intr.  */

 /* -------------------------------------------------------------------- */
 /*  Opcode 0011: SETMSB_3                                               */
 /*  Opcode 0101: SETMSB_5                                               */
 /* -------------------------------------------------------------------- */
 /* Mode 00 only             */
 /*  161 */  CR( 0,  0,  0,  0,  0,  1,  0),     /*  Clear 5     */
 /* Mode 00 and 01           */
 /*  162 */  CR( 6,  2,  AM, 0,  0,  0,  0),     /*  Amplitude   */
 /*  163 */  CR( 8,  0,  PR, 0,  0,  0,  0),     /*  Period      */
 /*  164 */  CR( 5,  3,  F0, 0,  1,  0,  0),     /*  F0 (5 MSBs) */
 /*  165 */  CR( 5,  3,  F1, 0,  1,  0,  0),     /*  F1 (5 MSBs) */
 /*  166 */  CR( 5,  3,  F2, 0,  1,  0,  0),     /*  F2 (5 MSBs) */
 /* SETMSB_3 only            */
 /*  167 */  CR( 5,  0,  IA, 0,  0,  0,  0),     /*  Ampl. Intr. */
 /*  168 */  CR( 5,  0,  IP, 0,  0,  0,  0),     /*  Per. Intr.  */

 /* Mode 10 only             */
 /*  169 */  CR( 0,  0,  0,  0,  0,  1,  0),     /*  Clear 5     */
 /* Mode 10 and 11           */
 /*  170 */  CR( 6,  2,  AM, 0,  0,  0,  0),     /*  Amplitude   */
 /*  171 */  CR( 8,  0,  PR, 0,  0,  0,  0),     /*  Period      */
 /*  172 */  CR( 6,  2,  F0, 0,  1,  0,  0),     /*  F0 (6 MSBs) */
 /*  173 */  CR( 6,  2,  F1, 0,  1,  0,  0),     /*  F1 (6 MSBs) */
 /*  174 */  CR( 6,  2,  F2, 0,  1,  0,  0),     /*  F2 (6 MSBs) */
 /* SETMSB_3 only            */
 /*  175 */  CR( 5,  0,  IA, 0,  0,  0,  0),     /*  Ampl. Intr. */
 /*  176 */  CR( 5,  0,  IP, 0,  0,  0,  0),     /*  Per. Intr.  */
};



static const int16_t sp0256_df_idx[16 * 8] =
{
 /*  OPCODE 0000 */      -1, -1,     -1, -1,     -1, -1,     -1, -1,
 /*  OPCODE 1000 */      -1, -1,     -1, -1,     -1, -1,     -1, -1,
 /*  OPCODE 0100 */      17, 22,     17, 24,     25, 30,     25, 32,
 /*  OPCODE 1100 */      83, 94,     129,142,    97, 108,    145,158,
 /*  OPCODE 0010 */      83, 96,     129,144,    97, 110,    145,160,
 /*  OPCODE 1010 */      73, 77,     74, 77,     78, 82,     79, 82,
 /*  OPCODE 0110 */      33, 36,     34, 37,     38, 41,     39, 42,
 /*  OPCODE 1110 */      127,128,    127,128,    127,128,    127,128,
 /*  OPCODE 0001 */      1,  14,     1,  16,     1,  14,     1,  16,
 /*  OPCODE 1001 */      45, 56,     45, 58,     59, 70,     59, 72,
 /*  OPCODE 0101 */      161,166,    162,166,    169,174,    170,174,
 /*  OPCODE 1101 */      111,116,    111,118,    119,124,    119,126,
 /*  OPCODE 0011 */      161,168,    162,168,    169,176,    170,176,
 /*  OPCODE 1011 */      -1, -1,     -1, -1,     -1, -1,     -1, -1,
 /*  OPCODE 0111 */      -1, -1,     -1, -1,     -1, -1,     -1, -1,
 /*  OPCODE 1111 */      0,  0,      0,  0,      0,  0,      0,  0
};

/* ======================================================================== */
/*  BITREV       -- Bit-reverse a 32-bit number.                            */
/* ======================================================================== */
static inline uint32_t bitrev(uint32_t val)
{
 val = ((val & 0xFFFF0000) >> 16) | ((val & 0x0000FFFF) << 16);
 val = ((val & 0xFF00FF00) >>  8) | ((val & 0x00FF00FF) <<  8);
 val = ((val & 0xF0F0F0F0) >>  4) | ((val & 0x0F0F0F0F) <<  4);
 val = ((val & 0xCCCCCCCC) >>  2) | ((val & 0x33333333) <<  2);
 val = ((val & 0xAAAAAAAA) >>  1) | ((val & 0x55555555) <<  1);

 return val;
}

/* ======================================================================== */
/*  SP0256_GETB  -- Get up to 8 bits at the current PC.                     */
/* ======================================================================== */
static uint32_t sp0256_getb(sp0256_t *sp0256, int len)
{
 uint32_t data = 0;
 uint32_t d0, d1;

 /* -------------------------------------------------------------------- */
 /*  Fetch data from the FIFO or from the MASK                           */
 /* -------------------------------------------------------------------- */
#if 0                           /* We don't support the FIFO in this application */
 if (sp0256->fifo_sel)
    {
     d0 = sp0256->fifo[(sp0256->fifo_tail    ) & 63];
     d1 = sp0256->fifo[(sp0256->fifo_tail + 1) & 63];

     data = ((d1 << 10) | d0) >> sp0256->fifo_bitp;

#ifdef DEBUG_FIFO
     DPRINTF(("sp0256: " "IV: RD_FIFO %.3X %d.%d %d\n", data & ((1 << len) - 1),
              sp0256->fifo_tail, sp0256->fifo_bitp, sp0256->fifo_head));
#endif

     /* ---------------------------------------------------------------- */
     /*  Note the PC doesn't advance when we execute from FIFO.          */
     /*  Just the FIFO's bit-pointer advances.   (That's not REALLY      */
     /*  what happens, but that's roughly how it behaves.)               */
     /* ---------------------------------------------------------------- */
     sp0256->fifo_bitp += len;
     if (sp0256->fifo_bitp >= 10)
        {
         sp0256->fifo_tail++;
         sp0256->fifo_bitp -= 10;
        }
    } else
#endif
       {
        /* ---------------------------------------------------------------- */
        /*  Figure out which ROMs are being fetched into, and grab two      */
        /*  adjacent bytes.  The byte we're interested in is extracted      */
        /*  from the appropriate bit-boundary between them.                 */
        /* ---------------------------------------------------------------- */
        int idx0 = (sp0256->pc    ) >> 3, page0 = idx0 >> 12;
        int idx1 = (sp0256->pc + 8) >> 3, page1 = idx1 >> 12;

        idx0 &= 0xFFF;
        idx1 &= 0xFFF;

        d0 = d1 = 0;

        if (sp0256->rom[page0]) d0 = sp0256->rom[page0][idx0];
        if (sp0256->rom[page1]) d1 = sp0256->rom[page1][idx1];

        data = ((d1 << 8) | d0) >> (sp0256->pc & 7);

        sp0256->pc += len;
       }

 /* -------------------------------------------------------------------- */
 /*  Mask data to the requested length.                                  */
 /* -------------------------------------------------------------------- */
 data &= ((1 << len) - 1);

 return data;
}

/* ======================================================================== */
/*  SP0256_MICRO -- Emulate the microcontroller in the SP0256.  Executes    */
/*                  instructions either until the repeat count != 0 or      */
/*                  the controller gets halted by a RTS to 0.               */
/* ======================================================================== */
void sp0256_micro(sp0256_t *sp)
{
 uint8_t  immed4;
 uint8_t  opcode;
 uint16_t cr;
 int     ctrl_xfer = 0;
 int     repeat    = 0;
 int     i, idx0, idx1;

 /* -------------------------------------------------------------------- */
 /*  Only execute instructions while the filter is not busy.             */
 /* -------------------------------------------------------------------- */
 while (sp->filt.rpt <= 0)
    {
     /* ---------------------------------------------------------------- */
     /*  If the CPU is halted, see if we have a new command pending      */
     /*  in the Address LoaD buffer.                                     */
     /* ---------------------------------------------------------------- */
     if (sp->halted && !sp->lrq)
        {
         sp->pc     = sp->ald | (0x1000 << 3);
         sp->halted = 0;
         sp->lrq    = 0x8000;
         sp->ald    = 0;
        }

     /* ---------------------------------------------------------------- */
     /*  If we're still halted, do nothing.                              */
     /* ---------------------------------------------------------------- */
     if (sp->halted)
        {
         sp->filt.rpt = 1;
         sp->lrq      = 0x8000;
         sp->ald      = 0;
         return;
        }

     /* ---------------------------------------------------------------- */
     /*  Fetch the first 8 bits of the opcode, which are always in the   */
     /*  same approximate format -- immed4 followed by opcode.           */
     /* ---------------------------------------------------------------- */
     immed4 = sp0256_getb(sp, 4);
     opcode = sp0256_getb(sp, 4);
     repeat = 0;
     ctrl_xfer = 0;

     DPRINTF(("sp0256: " "$%.4X.%.1X: OPCODE %d%d%d%d.%d%d\n",
              (sp->pc >> 3) - 1, sp->pc & 7,
              !!(opcode & 1), !!(opcode & 2),
              !!(opcode & 4), !!(opcode & 8),
              !!(sp->mode&4), !!(sp->mode&2)));

     /* ---------------------------------------------------------------- */
     /*  Handle the special cases for specific opcodes.                  */
     /* ---------------------------------------------------------------- */
     switch (opcode)
        {
         /* ------------------------------------------------------------ */
         /*  OPCODE 0000:  RTS / SETPAGE                                 */
         /* ------------------------------------------------------------ */
         case 0x0:
            {
             /* -------------------------------------------------------- */
             /*  If immed4 != 0, then this is a SETPAGE instruction.     */
             /* -------------------------------------------------------- */
             if (immed4)     /* SETPAGE */
                {
                 sp->page = bitrev(immed4) >> 13;
                } else
                   /* -------------------------------------------------------- */
                   /*  Otherwise, this is an RTS / HLT.                        */
                   /* -------------------------------------------------------- */
                   {
                    uint32_t btrg;

                    /* ---------------------------------------------------- */
                    /*  Figure out our branch target.                       */
                    /* ---------------------------------------------------- */
                    btrg = sp->stack;

                    sp->stack = 0;

                    /* ---------------------------------------------------- */
                    /*  If the branch target is zero, this is a HLT.        */
                    /*  Otherwise, it's an RTS, so set the PC.              */
                    /* ---------------------------------------------------- */
                    if (!btrg)
                       {
                        sp->halted = 1;
                        sp->pc     = 0;
                        ctrl_xfer  = 1;
                       } else
                          {
                           sp->pc    = btrg;
                           ctrl_xfer = 1;
                          }
                   }

             break;
            }

            /* ------------------------------------------------------------ */
            /*  OPCODE 0111:  JMP          Jump to 12-bit/16-bit Abs Addr   */
            /*  OPCODE 1011:  JSR          Jump to Subroutine               */
            /* ------------------------------------------------------------ */
         case 0xE:
         case 0xD:
            {
             int btrg;

             /* -------------------------------------------------------- */
             /*  Figure out our branch target.                           */
             /* -------------------------------------------------------- */
             btrg = sp->page                           |
                (bitrev(immed4)             >> 17) |
                (bitrev(sp0256_getb(sp, 8)) >> 21);
             ctrl_xfer = 1;

             /* -------------------------------------------------------- */
             /*  If this is a JSR, push our return address on the        */
             /*  stack.  Make sure it's byte aligned.                    */
             /* -------------------------------------------------------- */
             if (opcode == 0xD)
                sp->stack = (sp->pc + 7) & ~7;

             /* -------------------------------------------------------- */
             /*  Jump to the new location!                               */
             /* -------------------------------------------------------- */
             sp->pc = btrg;
             break;
            }

            /* ------------------------------------------------------------ */
            /*  OPCODE 1000:  SETMODE      Set the Mode and Repeat MSBs     */
            /* ------------------------------------------------------------ */
         case 0x1:
            {
             sp->mode = ((immed4 & 8) >> 2) | (immed4 & 4) |
                ((immed4 & 3) << 4);
             break;
            }

            /* ------------------------------------------------------------ */
            /*  OPCODE 0001:  LOADALL      Load All Parameters              */
            /*  OPCODE 0010:  LOAD_2       Load Per, Ampl, Coefs, Interp.   */
            /*  OPCODE 0011:  SETMSB_3     Load Pitch, Ampl, MSBs, & Intrp  */
            /*  OPCODE 0100:  LOAD_4       Load Pitch, Ampl, Coeffs         */
            /*  OPCODE 0101:  SETMSB_5     Load Pitch, Ampl, and Coeff MSBs */
            /*  OPCODE 0110:  SETMSB_6     Load Ampl, and Coeff MSBs.       */
            /*  OPCODE 1001:  DELTA_9      Delta update Ampl, Pitch, Coeffs */
            /*  OPCODE 1010:  SETMSB_A     Load Ampl and MSBs of 3 Coeffs   */
            /*  OPCODE 1100:  LOAD_C       Load Pitch, Ampl, Coeffs         */
            /*  OPCODE 1101:  DELTA_D      Delta update Ampl, Pitch, Coeffs */
            /*  OPCODE 1110:  LOAD_E       Load Pitch, Amplitude            */
            /*  OPCODE 1111:  PAUSE        Silent pause                     */
            /* ------------------------------------------------------------ */
         default:
         {
          repeat    = immed4 | (sp->mode & 0x30);
          break;
         }
        }
     if (opcode != 1) sp->mode &= 0xF;

     /* ---------------------------------------------------------------- */
     /*  If this was a control transfer, handle setting "fifo_sel"       */
     /*  and all that ugliness.                                          */
     /* ---------------------------------------------------------------- */
     if (ctrl_xfer)
        {
         DPRINTF(("sp0256: " "jumping to $%.4X.%.1X: ", sp->pc >> 3, sp->pc & 7));

         /* ------------------------------------------------------------ */
         /*  Set our "FIFO Selected" flag based on whether we're going   */
         /*  to the FIFO's address.                                      */
         /* ------------------------------------------------------------ */
#if 0                           /* FIFO is not supported */
         sp->fifo_sel = sp->pc == FIFO_ADDR;

         DPRINTF(("sp0256: " "%s ", sp->fifo_sel ? "FIFO" : "ROM"));

         /* ------------------------------------------------------------ */
         /*  Control transfers to the FIFO cause it to discard the       */
         /*  partial decle that's at the front of the FIFO.              */
         /* ------------------------------------------------------------ */
         if (sp->fifo_sel && sp->fifo_bitp)
            {
             DPRINTF(("sp0256: " "bitp = %d -> Flush", sp->fifo_bitp));

             /* Discard partially-read decle. */
             if (sp->fifo_tail < sp->fifo_head) sp->fifo_tail++;
             sp->fifo_bitp = 0;
            }
#endif

         DPRINTF(("sp0256: " "\n"));

         continue;
        }

     /* ---------------------------------------------------------------- */
     /*  Otherwise, if we have a repeat count, then go grab the data     */
     /*  block and feed it to the filter.                                */
     /* ---------------------------------------------------------------- */
     if (!repeat) continue;


#ifdef SINGLE_STEP
     DPRINTF(("sp0256: " "NEXT:\n"));
     {
      char buf[1024];
      gets(buf); if (opcode != 0xF) repeat <<= 3;
     }
#endif

     sp->filt.rpt = repeat + 1;
     DPRINTF(("sp0256: " "repeat = %d\n", repeat));

     i = (opcode << 3) | (sp->mode & 6);
     idx0 = sp0256_df_idx[i++];
     idx1 = sp0256_df_idx[i  ];

     assert(idx0 >= 0 && idx1 >= 0 && idx1 >= idx0);

     /* ---------------------------------------------------------------- */
     /*  Step through control words in the description for data block.   */
     /* ---------------------------------------------------------------- */
     for (i = idx0; i <= idx1; i++)
        {
         int len, shf, delta, field, prm, clra, clr5;
         int8_t value;

         /* ------------------------------------------------------------ */
         /*  Get the control word and pull out some important fields.    */
         /* ------------------------------------------------------------ */
         cr = sp0256_datafmt[i];

         len = CR_LEN(cr);
         shf = CR_SHF(cr);
         prm = CR_PRM(cr);
         clra  = cr & CR_CLRA;
         clr5  = cr & CR_CLR5;
         delta = cr & CR_DELTA;
         field = cr & CR_FIELD;
         value = 0;

         DPRINTF(("sp0256: " "$%.4X.%.1X: len=%2d shf=%2d prm=%2d d=%d f=%d ",
                  sp->pc >> 3, sp->pc & 7, len, shf, prm, !!delta, !!field));
         /* ------------------------------------------------------------ */
         /*  Clear any registers that were requested to be cleared.      */
         /* ------------------------------------------------------------ */
         if (clra)
            {
             int j;

             for (j = 0; j < 16; j++)
                sp->filt.r[j] = 0;
            }

         if (clr5)
            sp->filt.r[B5] = sp->filt.r[F5] = 0;

         /* ------------------------------------------------------------ */
         /*  If this entry has a bitfield with it, grab the bitfield.    */
         /* ------------------------------------------------------------ */
         if (len)
            value = sp0256_getb(sp, len);
         else
            {
             DPRINTF(("sp0256: " " (no update)\n"));
             continue;
            }

         /* ------------------------------------------------------------ */
         /*  Sign extend if this is a delta update.                      */
         /* ------------------------------------------------------------ */
         if (delta)  /* Sign extend */
            {
             if (value & (1 << (len - 1))) value |= -1 << len;
            }

         /* ------------------------------------------------------------ */
         /*  Shift the value to the appropriate precision.               */
         /* ------------------------------------------------------------ */
         if (shf)
            value <<= shf;

         DPRINTF(("sp0256: " "v=%.2X (%c%.2X)  ", value & 0xFF,
                  value & 0x80 ? '-' : '+',
                  0xFF & (value & 0x80 ? -value : value)));

         /* ------------------------------------------------------------ */
         /*  If this is a field-replace, insert the field.               */
         /* ------------------------------------------------------------ */
         if (field)
            {
             DPRINTF(("sp0256: " "--field-> r[%2d] = %.2X -> ", prm, sp->filt.r[prm]));

             sp->filt.r[prm] &= ~(~0 << shf); /* Clear the old bits.     */
             sp->filt.r[prm] |= value;        /* Merge in the new bits.  */

             DPRINTF(("sp0256: " "%.2X\n", sp->filt.r[prm]));

             continue;
            }

         /* ------------------------------------------------------------ */
         /*  If this is a delta update, add to the appropriate field.    */
         /* ------------------------------------------------------------ */
         if (delta)
            {
             DPRINTF(("sp0256: " "--delta-> r[%2d] = %.2X -> ", prm, sp->filt.r[prm]));

             sp->filt.r[prm] += value;

             DPRINTF(("sp0256: " "%.2X\n", sp->filt.r[prm]));

             continue;
            }

         /* ------------------------------------------------------------ */
         /*  Otherwise, just write the new value.                        */
         /* ------------------------------------------------------------ */

         sp->filt.r[prm] = value;
         DPRINTF(("sp0256: " "--value-> r[%2d] = %.2X\n", prm, sp->filt.r[prm]));
        }

     /* ---------------------------------------------------------------- */
     /*  Special case:  Set PAUSE's equivalent period.                   */
     /* ---------------------------------------------------------------- */
     if (opcode == 0xF)
        sp->filt.r[1] = PER_PAUSE;

     /* ---------------------------------------------------------------- */
     /*  Now that we've updated the registers, go decode them.           */
     /* ---------------------------------------------------------------- */
     lpc12_regdec(&sp->filt);

     /* ---------------------------------------------------------------- */
     /*  Break out since we now have a repeat count.                     */
     /* ---------------------------------------------------------------- */
     break;
    }
}

/* ======================================================================== */
/*  SP0256_RDROM -- Read a ROM file in the ubee512 ROMS directory.          */
/* ======================================================================== */
int sp0256_rdrom(sp0256_t *sp, int page)
{
    uint8_t *rom;
    char romname[SSIZE1];
    char filepath[PATH_MAX];
    int  i;

    /* -------------------------------------------------------------------- */
    /*  Generate the file name for this page's data.                        */
    /* -------------------------------------------------------------------- */
    snprintf(romname, sizeof(romname) - 1, "sp0256_%.1X.bin", page);

    /* -------------------------------------------------------------------- */
    /*  Allocate 4K worth of space to the ROM image.                        */
    /* -------------------------------------------------------------------- */
    rom = calloc(PAGESIZE, 1);
    if (!rom)
    {
        DPRINTF(("sp0256: " "Out of memory in rdrom\n"));
        return -1;
    }

    /* -------------------------------------------------------------------- */
    /*  Read in the ROM image and then bit-reverse it.                      */
    /* -------------------------------------------------------------------- */
    if (!roms_loadrom(romname, rom, PAGESIZE, filepath))
    {
        free(rom);                      /* couldn't load this rom. */
        sp->rom[page] = NULL;
        return 1;                       /* failure code */
    }
    else
    {
        for (i = 0; i < PAGESIZE; i++)
            rom[i] = bitrev(rom[i]) >> 24;
        if (modio.beetalker)
        {
            xprintf("sp0256: "
                    "added %s at SP0256 address $%.4X.0\n", filepath, page << 12);
        }
        /* ---------------------------------------------------------------- */
        /*  Set this as our ROM page, and we're all set.                    */
        /* ---------------------------------------------------------------- */
        sp->rom[page] = rom;
        return 0;
    }
}

/* ======================================================================== */
/*  SP0256_INIT -- Initialises the SP0256 scratch.                          */
/* ======================================================================== */
int sp0256_init(sp0256_t *sp)
{
 int i;
 int romloaded = 0;

 memset(sp, 0, sizeof(*sp));
 /* -------------------------------------------------------------------- */
 /*  Set up the microcontroller's initial state.                         */
 /* -------------------------------------------------------------------- */
 sp->halted   = 1;
 sp->filt.rng = 1;
 sp->filt.rpt = -1;
 sp->lrq      = 0x8000;
 sp->page     = 0x1000 << 3;

 /* -------------------------------------------------------------------- */
 /*  Attempt to read SP0256 ROM files.  This needs re-architecting if    */
 /*  you're going to have multiple SP0256's in a system, or use ROMs     */
 /*  from various places, but it'll do for the moment.                   */
 /* -------------------------------------------------------------------- */
 for (i = 0; i < 16; i++)
    if (!sp0256_rdrom(sp, i))
        romloaded = 1;

 if (!romloaded)
 {
     /* Having no ROM data isn't a catastrophic failure, it just means
        that every opcode fetch will halt the microsequencer
        immediately */
     xprintf("sp0256: no ROM data loaded\n");
     return -1;
 }

 /* -------------------------------------------------------------------- */
 /*  Allocate a circular buffer for samples.                             */
 /* -------------------------------------------------------------------- */
 if (!audio_circularbuf_init(&sp->scratch))
    return -1;

 return 0;
}

int sp0256_deinit(sp0256_t *sp)
{
 int i;

 for (i = 0; i < (sizeof(sp->rom)/sizeof(sp->rom[0])); ++i)
    {
     if (sp->rom[i])
        free((void *)sp->rom[i]);
     sp->rom[i] = NULL;
    }
 return audio_circularbuf_deinit(&sp->scratch);
}

void sp0256_ald(sp0256_t *sp, uint8_t data)
{
 sp->lrq = 0;
 sp->ald = (uint16_t)data << 4;
}

/* ======================================================================== */
/*  SP0256_ITERATE -- generate samples.  Returns the actual number of
 *  samples generated, or -1 if the sample buffer is full, or -2 if
 *  the microcontroller needs more data.*/
/* ======================================================================== */
int sp0256_iterate(sp0256_t *sp, int samples)
{
 int do_samp;

 if (samples <= 0)
    return 0;

 /* ------------------------------------------------------------ */
 /*  If the repeat count expired, emulate the microcontroller.   */
 /* ------------------------------------------------------------ */
 if (sp->filt.rpt <= 0)
    {
     int oldlrq = sp->lrq;
     sp0256_micro(sp);
     if (sp->lrq != oldlrq && !oldlrq)
        return -2;              /* Signal that the microcontroller
                                 * needs more data. */
    }

 do_samp = audio_circularbuf_samples_remaining(&sp->scratch,
                                               AUDIO_CIRCULARBUF_SIZE);
 /* the circular buffer is deemed to be full when only 1 sample is
    left - i.e. head is just behind tail */
 if (do_samp <= 1)
    return -1;          /* sample buffer is full */
 if (do_samp > samples)
    do_samp = samples;  /* generate no more samples than
                         * requested (but can generate
                         * fewer!) */
 assert(do_samp > 0 && do_samp <= samples && do_samp < AUDIO_CIRCULARBUF_SIZE);
 return lpc12_update(&sp->filt, do_samp, &sp->scratch);
}
