/*
 * ============================================================================
 *  Title:    GI SP0256 Emulation for the BeeTalker
 *
 *  (based on the Intellivoice Emulator by Joseph Zbiciak)
 *  $Id: sp0256.h,v 1.1.1.3 2010/08/07 04:17:12 krd Exp $
 * ============================================================================
 */
#ifndef _SP0256_H_
#define _SP0256_H_

#define FIFO_ADDR    (0x1800 << 3)      /* SP0256 address of speech FIFO.   */

#define SP0256_CLOCK_DIVISOR    312

typedef struct lpc12_t
{
   int     rpt, cnt;       /* Repeat counter, Period down-counter.         */
   uint32_t per, rng;      /* Period, Amplitude, Random Number Generator   */
   int     amp;
   int16_t f_coef[6];      /* F0 through F5.                               */
   int16_t b_coef[6];      /* B0 through B5.                               */
   int16_t z_data[6][2];   /* Time-delay data for the filter stages.       */
   uint8_t r[16];          /* The encoded register set.                    */
   int     interp;
} lpc12_t;


typedef struct sp0256_t
{
   audio_circularbuf_t scratch;
   lpc12_t     filt;       /* 12-pole filter                               */
   int         lrq;        /* Load ReQuest.  == 0 if we can accept a load  */
   int         ald;        /* Address LoaD.  < 0 if no command pending.    */
   int         pc;         /* Microcontroller's PC value.                  */
   int         stack;      /* Microcontroller's PC stack.                  */
   int         fifo_sel;   /* True when executing from FIFO.               */
   int         halted;     /* True when CPU is halted.                     */
   uint32_t    mode;       /* Mode register.                               */
   uint32_t    page;       /* Page set by SETPAGE                          */

   const uint8_t *rom[16]; /* 4K ROM pages.                                */
} sp0256_t;

/* ======================================================================== */
/*  SP0256_INIT -- Initialises the SP0256 scratch.                          */
/* ======================================================================== */
int sp0256_init(sp0256_t *sp);

/* ======================================================================== */
/*  SP0256_DEINIT -- cleans up the SP0256 scratch.                          */
/* ======================================================================== */
int sp0256_deinit(sp0256_t *sp);

/* ======================================================================== */
/*  SP0256_MICRO -- Emulate the microcontroller in the SP0256.  Executes    */
/*                  instructions either until the repeat count != 0 or      */
/*                  the controller is halted by a RTS to 0.                 */
/* ======================================================================== */
void sp0256_micro(sp0256_t *sp);

/* ======================================================================== */
/*  SP0256_ITERATE -- generate samples and return 1 if the
 *  microcontroller can accept more data                                    */
/* ======================================================================== */
int sp0256_iterate(sp0256_t *sp, int samples);

/* ======================================================================== */
/*  LPC12_UPDATE -- Update the 12-pole filter, outputting samples.          */
/* ======================================================================== */
int lpc12_update(lpc12_t *f, int num_samp, audio_circularbuf_t *cb);

/* ======================================================================== */
/*  SP0256_ALD -- load the address register.                                */
/* ======================================================================== */
void sp0256_ald(sp0256_t *sp, uint8_t data);

#endif /* _SP0256_H */
/* ======================================================================== */
/*  This program is free software; you can redistribute it and/or modify    */
/*  it under the terms of the GNU General Public License as published by    */
/*  the Free Software Foundation; either version 2 of the License, or       */
/*  (at your option) any later version.                                     */
/*                                                                          */
/*  This program is distributed in the hope that it will be useful,         */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       */
/*  General Public License for more details.                                */
/*                                                                          */
/*  You should have received a copy of the GNU General Public License       */
/*  along with this program; if not, write to the Free Software             */
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.               */
/* ======================================================================== */
/*                 Copyright (c) 1998-2000, Joseph Zbiciak                  */
/*                 Copyright (c) 2009-2010, Kalvis Duckmanton               */
/* ======================================================================== */
