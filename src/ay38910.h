#ifndef _ay38910_h
#define _ay38910_h
/* $Id: ay38910.h,v 1.1.1.4 2011/03/27 06:04:44 krd Exp $ */

/*----------------------------------------------------------------*/
/* AY constants                                                   */

/* The PSG clock is divided down by this much first before clocking
 * the tone or noise generators */
#define PSG_CLOCK_DIVISOR 16

/* The data sheet defines these register numbers in octal, grr */

#define PSG_TONE_A_FINE         000
#define PSG_TONE_A_COARSE       001
#define PSG_TONE_B_FINE         002
#define PSG_TONE_B_COARSE       003
#define PSG_TONE_C_FINE         004
#define PSG_TONE_C_COARSE       005
#define PSG_NOISE               006
#define PSG_MIXER_IO_EN         007

#define PSG_AMPLITUDE_A         010
#define PSG_AMPLITUDE_B         011
#define PSG_AMPLITUDE_C         012
#define PSG_ENVELOPE_FINE       013
#define PSG_ENVELOPE_COARSE     014
#define PSG_ENVELOPE_SHAPE      015
#define PSG_IO_A                016
#define PSG_IO_B                017

/* masks */
#define PSG_TONE_MASK           ((1 << 12) - 1) /* tone period is 12 bits */
#define PSG_NOISE_MASK          ((1 << 5) - 1) /* noise period is 5 bits */
#define PSG_ENVELOPE_MASK       ((1U << 16) - 1)        /* envelope period is 16 bits */
#define PSG_AMPLITUDE_MASK      ((1 << 4) - 1)
#define PSG_AMPLITUDE_MODE_MASK (1 << 4)
#define PSG_IO_A_EN_MASK        (1 << 6)
#define PSG_IO_B_EN_MASK        (1 << 7)
#define PSG_NOISE_SELECT_MASK   (((1 << 3) - 1) << 3)
#define PSG_TONE_SELECT_MASK    ((1 << 3) - 1)
#define PSG_CHANNEL_A           (1 << 0)
#define PSG_CHANNEL_B           (1 << 1)
#define PSG_CHANNEL_C           (1 << 2)
#define PSG_NOISE_CHANNEL_A     (1 << (0+3))
#define PSG_NOISE_CHANNEL_B     (1 << (1+3))
#define PSG_NOISE_CHANNEL_C     (1 << (2+3))
#define PSG_NOISE_BIT           (1 << 3)
#define PSG_ENVELOPE_CYCLE_HOLD         (1 << 0)
#define PSG_ENVELOPE_CYCLE_ALTERNATE    (1 << 1)
#define PSG_ENVELOPE_CYCLE_ATTACK       (1 << 2)
#define PSG_ENVELOPE_CYCLE_CONTINUE     (1 << 3)

/* envelope generator state bits */
/* whether the envelope generator is in the attacking or decaying
 * cycle */
#define PSG_ENVELOPE_STATE_DECAY        (1 << 5)
/* envelope generator output mask */
#define PSG_ENVELOPE_OUTPUT_MASK        ((1 << 5) - 1)
/* the envelope generator output */
#define PSG_ENVELOPE_CYCLE_VALUE(x)     (((x) & PSG_ENVELOPE_OUTPUT_MASK) >> 1)

#define PSG_SELECT_MASK         (((1 << 4) - 1) << 4)
#define PSG_REGISTER_MASK       ((1 << 4) - 1)
#define PSG_SELECT_VALUE         (0 << 4)

typedef struct ay_3_8910_t {
    uint8_t reg[16];            /* device registers */

    // Working copies of the tone, noise and envelope period registers
    uint16_t tone_current[3];
    uint8_t noise_current;
    uint16_t envelope_current;

    // Current tone, noise and envelope periods
    uint16_t tone_per[3];
    uint8_t noise_per;
    uint16_t envelope_per;

    // current state of the tone and noise generator outputs.
    uint8_t state;
    // current state of the envelope generator
    uint8_t envelope_state;
    // current envelope generator amplitude
    uint8_t envelope_amplitude;
    // noise generator register
    uint32_t noise;

    // buffer for samples
    audio_circularbuf_t scratch;
} ay_3_8910_t;

int psg_init(ay_3_8910_t *psg);
int psg_deinit(ay_3_8910_t *psg);

uint8_t psg_r(ay_3_8910_t *psg, uint8_t reg);
void psg_w(ay_3_8910_t *psg, uint8_t reg, uint8_t data);

/* ======================================================================== */
/*  PSG_ITERATE -- generate the requested number of samples into the
 *  sample buffer  */
/* ======================================================================== */
int psg_iterate(ay_3_8910_t *psg, int samples);

#endif /* _ay38910_h */
