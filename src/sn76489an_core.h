#ifndef _sn76489an_core_h
#define _sn76489an_core_h
/* $Id: sn76489an_core.h,v 1.1.1.1 2011/03/27 06:04:42 krd Exp $ */

/*
 * Data structures used by the SN76489 emulation core
 */
typedef struct sn_update_le_t {
   uint64_t when;               /* time at which to update register
                                 * (z80 clock ticks) */
   uint8_t address, data;       /* register to update and its new
                                 * value */
   struct sn_update_le_t *next;
} sn_update_le_t;
typedef sn_update_le_t *sn_update_list_t;

struct sn76489an_t {
   audio_scratch_t snd_buf;
   /* device registers */
   uint16_t regs[8];
   int current_register;

   /* working copies of the period registers */
   uint16_t period_current[4];
   // noise generator shift register
   uint32_t noise;
   // output state
   int state;

   // buffer for samples
   audio_circularbuf_t scratch;
   uint64_t cycles_remainder;
   sn_update_list_t update_head, update_tail;

   // the current input clock rate
   int clock_frequency;
};
typedef struct sn76489an_t sn76489an_t;

/*
 * Initialise, deinitialise and reset
 */
int sn76489an_core_init (sn76489an_t *s, char *name,
                         audio_clock_fn_t clock_fn,
                         int clock_frequency,
                         int silence);
int sn76489an_core_deinit (sn76489an_t *s);
int sn76489an_core_reset (sn76489an_t *s);
/*
 * Read and write
 */
uint16_t sn76489an_core_r (sn76489an_t *s, uint16_t port);
void sn76489an_core_w (sn76489an_t *s, uint16_t port, uint8_t data);
/*
 * Update the sample rate conversion factor should the SN76489 clock
 * change (e.g. when clocked from the CPU clock).
 */
void sn76489an_core_clock (sn76489an_t *s, int clock_frequency);

#endif /* _sn76489an_core_h */
