/* Audio Header */

#ifndef HEADER_AUDIO_H
#define HEADER_AUDIO_H

#include "z80.h"

#define AUDIO_VOLUME_PERCENT 45
#define AUDIO_FREQUENCY 44100
#define AUDIO_FORMAT AUDIO_U8
#define AUDIO_CHANNELS 1
#define AUDIO_BUFTYP uint8_t
#define AUDIO_MAXVAL 127

// The value used for AUDIO_SAMPLES has been revised with version 2.7.0 as
// other changes have been made that can affect the audio quality. A very
// SLOW distorted sound is produced by some Windows installations if the
// value is not high enough.  For Unix this does not appear to be as
// critical.
//
// A value of 2048 for Windows allows a sample rate of up to 88200KHz to be
// used, a value of 1024 was found to be suitable for 44100KHz.

#ifdef MINGW
#define AUDIO_SAMPLES 2048
#else
#define AUDIO_SAMPLES 2048
#endif

// Constants controlling the way that CPU-dependent audio sources,
// such as the Microbee speaker, or external DACs, are emulated.
//
// Such sources can be tied to the current emulated CPU frequency
// (which needn't be 2, 3.375 or 6.75MHz) or they can assume the
// CPU clock frequency is always 3.375MHz.  This is most useful
// for sources that produce sound effects of (resonably) short
// duration (e.g. the Microbee speaker)

#define AUDIO_DISABLED 0        /* deprecated */
#define AUDIO_PROPORTIONAL 1
#define AUDIO_FIXED_3375 2

typedef struct audio_t
{
 int mute;
 int vol_percent;
 int samples;
 int frequency;
 int mode;
}audio_t;

/*-------------------------------------------------------------------*/

void audio_command (int cmd);

/* ======================================================================== */
/*  LIMIT            -- Limiter function for digital sample output.         */
/* ======================================================================== */
static inline AUDIO_BUFTYP audio_limit(int16_t s)
{
 if (s >  127) return  128 + 127;
 if (s < -128) return  128 - 128;
 return 128 + s;
}

typedef struct audio_buffer_t
{
 int count;                   /* number of samples in this buffer */
 int drain_count;             /* number of samples to drain */
 AUDIO_BUFTYP samples[0];     /* the samples themselves */
}audio_buffer_t;

typedef struct audio_scratch_t
{
 int num_clean;
 audio_buffer_t **clean;      /* array of clean buffers */
 int num_dirty;
 audio_buffer_t **dirty;      /* array of dirty buffers */
 audio_buffer_t *cur_buf;     /* Current sound buffer. */
 int len;                     /* size of each buffer */
 SDL_mutex *mutex;            /* mutex protecting accesses to the
                               * clean and dirty buffer arrays */
 SDL_cond *cond;              /* condition variable for
                               * signalling */
 unsigned int new_samples;    /* debugging variable, recording the
                               * number of new samples added per
                               * frame */
}audio_scratch_t;

typedef enum
{
 AUDIO_SOURCE_QUIESCENT = 0,
 AUDIO_SOURCE_BUFFERING,
 AUDIO_SOURCE_PLAYING,
}audio_source_state_t;

typedef int (*audio_gen_fn_t)(audio_scratch_t *buf, const void *data,
                              uint64_t start, uint64_t cycles);
typedef void (*audio_clock_fn_t)(int cpuclock);

typedef struct audio_source_t
{
 audio_scratch_t *buf;
 const char *name;            /* name of the audio source */
 const void *data;            /* additional data to pass to the
                               * sound generation function */
 audio_gen_fn_t audio_func;   /* function to call to generate the
                               * specified number of CPU cycles'
                               * worth of sound */
 audio_clock_fn_t clock_func; /* function to call when the CPU clock
                               * changes */
 int sync:1;                  /* non-zero if this sound source is
                               * synchronised to the CPU thread
                               * (i.e. it's called from the CPU
                               * thread). */
 int holdoff_count;           /* number of samples which need to be
                               *  generated before this source
                               *  starts playing */
 /* -- members below here are changed by the audio thread -- */
 int count;                   /* sample count */
 audio_source_state_t state;  /* state of this audio source */
}audio_source_t;

// Audio circular buffer constants
#define AUDIO_CIRCULARBUF_SIZE  (1 << 12) /* must be a power of 2 */
#define AUDIO_CIRCULARBUF_MASK  (AUDIO_CIRCULARBUF_SIZE - 1)

typedef struct audio_circularbuf_t
{
 AUDIO_BUFTYP *buf;       /* the buffer */
 int          head;       /* Head/Tail pointer into circular buf  */
 int          tail;       /* Head/Tail pointer into circular buf  */
                          /* sample rate conversion variables */
 int          index, increment, limit;
 int          rate_num, rate_denom; /* numerator and denominator of
                                     * conversion fraction */
 int          src_rate, dst_rate;   /* actual source and destination
                                     * sampling rates */
 AUDIO_BUFTYP this_sample, next_sample;
 int          tau;        /* decay constant, in ms */
 int          decay;
}audio_circularbuf_t;

/* circular buffer initialisation and deinitialisation */
int audio_circularbuf_init(audio_circularbuf_t *cb);
int audio_circularbuf_deinit(audio_circularbuf_t *cb);
void audio_circularbuf_set_rate_conversion(audio_circularbuf_t *cb,
                                          int dst_rate, int src_rate);
void audio_circularbuf_set_decay_constant(audio_circularbuf_t *cb,
                                          int tau);

/* normalise head and tail pointers */
static inline void audio_circularbuf_normalise(audio_circularbuf_t *cb, int mask)
{
 cb->head &= mask;
 cb->tail &= mask;
}

/* Return the number of samples in the circular buffer */
static inline int audio_circularbuf_samples(audio_circularbuf_t *cb, int bufsize)
{
 int n = cb->head - cb->tail;
 if (n < 0)
    n += bufsize;
 return n;
}

/* Return the number of samples remaining in the circular buffer */
static inline int audio_circularbuf_samples_remaining(audio_circularbuf_t *cb, int bufsize)
{
 return bufsize - 1 - audio_circularbuf_samples(cb, bufsize);
}

/* put a sample into the circular buffer */
static inline void audio_circularbuf_put_sample(audio_circularbuf_t *cb, int mask, int s)
{
 if (cb->tau)
    {
     cb->decay -= (s * (1<<16) + cb->decay) / cb->tau;
     cb->buf[cb->head++ & mask] = audio_limit(s + (cb->decay / (1<<16)));
    }
 else
    cb->buf[cb->head++ & mask] = audio_limit(s);
}

int audio_init(void);
int audio_deinit(void);
int audio_reset(void);
void audio_clock(int cpuclock);
int audio_register(audio_scratch_t *a, const char *name,
                   audio_gen_fn_t audio_func, const void *data,
                   audio_clock_fn_t clock_func,
                   int synchronous,
                   int holdoff_time_ms);
int audio_deregister(audio_scratch_t *a);
void audio_set_master_volume(int percent);
void audio_get_work_buffer(audio_scratch_t *a);
void audio_put_work_buffer(audio_scratch_t *a);
void audio_sources_update(void);
void audio_drain_samples(audio_scratch_t *a, audio_circularbuf_t *sc);

static inline void audio_put_sample(audio_scratch_t *a, AUDIO_BUFTYP sample)
{
 a->cur_buf->samples[a->cur_buf->count++] = sample;
 a->new_samples++;
}

static inline void audio_put_samples(audio_scratch_t *a, AUDIO_BUFTYP sample, int n)
{
 while (n--)
    a->cur_buf->samples[a->cur_buf->count++] = sample;
 a->new_samples += n;
}

static inline int audio_space_remaining(audio_scratch_t *a)
{
 return a->len - a->cur_buf->count;
}

static inline int audio_has_work_buffer(audio_scratch_t *a)
{
 return a->cur_buf != NULL;
}
#endif     /* HEADER_AUDIO_H */
