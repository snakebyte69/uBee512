//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                       Copyright (C) 2007-2016 uBee                         *
//*                                                                            *
//*                           Audio support module                             *
//*                Copyright (C) 2009-2010 Kalvis Duckmanton                   *
//*                                                                            *
//******************************************************************************
//
// This module provides functions to mix the output from several
// emulated audio sources
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
// v5.7.0 - 13 July 2013, uBee
// - Changed code in audio_command() for EMU_CMD_MUTE to remove call to
//   audio_set_master_volume() as muting is now handled by changes to
//   audio_source_play() for the mute setting.
//
// v5.0.0 - 9 July 2010, K Duckmanton
// - Moved the sound_command() function from sound.c to this file and renamed
//   it to audio_command().  Introduced a new global variable 'audio' storing
//   the audio sampling frequency.  Removed all referenced to the 'sound'
//   global variable.
// - audio_register() now requires a new callback function which is called
//   whenever the emulated CPU speed changes
//
// v4.7.0 - 17 June 2010, K Duckmanton
// - Initial implementation
//==============================================================================

#ifdef MINGW
#include <windows.h>
#else
#include <sys/stat.h>
#include <signal.h>    // signal name macros, and the signal() prototype
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <SDL.h>
#include <SDL_thread.h>

#include "ubee512.h"
#include "gui.h"
#include "audio.h"
#include "z80api.h"
#include "function.h"

//==============================================================================
// structures and variables
//==============================================================================

static SDL_AudioSpec wanted, obtained;

static void audio_fill(void *udata, Uint8 *stream, int len);

extern emu_t emu;

//==============================================================================
// constants
//==============================================================================
#define MAX_AUDIO_BUFFERS 10    /* 20480 bytes, or about 1/2 a second */
#define DEBUG_MIXER 0           /* set to 1 to debug the operation of
                                 * the mixer function */
#define DEBUG_AUDIO 0           /* set to 1 to debug the operation of
                                 * the audio thread */
#define DEBUG_AUDIO_MUTEXES 0
#define DEBUG_DISCARD_SAMPLES 0 /* set to 1 to throw samples away
                                 * instead of adding them to the audio
                                 * buffers. */
#define NUM_AUDIO_SOURCES 5

//==============================================================================
// macros
//==============================================================================
#if DEBUG_AUDIO_MUTEXES
#define DEBUG_MUTEX(name, what, x) \
do { \
 uint64_t now, then = time_get_ms(); \
 (x); \
 now = time_get_ms(); \
 if (now - then >= 2) \
    { \
     xprintf(name ": %s: waited %llu ms for lock (line %d: %s)\n", \
             what, now - then, __LINE__, #x); \
     assert(now - then <= 5); \
    } \
} while(0)
#else
#  define DEBUG_MUTEX(name, what, x) (x)
#endif

//==============================================================================
// global variables
//==============================================================================
audio_t audio =
{
 .mode = AUDIO_PROPORTIONAL,          // default audio mode
 .vol_percent = AUDIO_VOLUME_PERCENT, // default audio volume
 .samples = AUDIO_SAMPLES,            // number of samples in each audio frame
 .frequency = AUDIO_FREQUENCY,        // audio sampling rate
};
static audio_source_t audio_sources[NUM_AUDIO_SOURCES];
int audio_samples = AUDIO_SAMPLES; /* size of the SDL audio buffer.
                                    * Should not be smaller than the
                                    * size of the audio buffers */
static uint64_t audio_tstates_last = 0; /* Z80 tstate count at the
                                         * start of each frame */
#if DEBUG_AUDIO
static uint64_t audio_fill_last;
static int audio_fill_expected_delay = 0;
#endif
static int audio_master_volume = SDL_MIX_MAXVOLUME;

extern gui_t gui;
extern gui_status_t gui_status;

//==============================================================================
// internal function prototypes
//==============================================================================
int audio_allocate_buffers(audio_scratch_t *a, int len);
int audio_deallocate_buffers(audio_scratch_t *a);
int audio_source_play(audio_scratch_t *a, void *udata, Uint8 *stream, int len);
void audio_next_dirty_buffer(audio_scratch_t *a);
void audio_recycle_buffer(audio_scratch_t *a, audio_buffer_t *buf);
#if DEBUG_MIXER
void audio_dumpstream(const char *what, AUDIO_BUFTYP *data, Uint32 len);
#endif

//==============================================================================
// audio subsystem commands
//
//   pass: int cmd                      audio command
// return: void
//==============================================================================
void audio_command (int cmd)
{
 switch (cmd)
    {
     case EMU_CMD_MUTE :
        audio.mute = ! audio.mute;
        break;
     case EMU_CMD_VOLUMEI :
        if ((gui_status.vol) || (gui.persist_flags & GUI_PERSIST_VOL))
           {
            audio.vol_percent += EMU_VOLUME_CHANGE;
            if (audio.vol_percent > 100)
               audio.vol_percent = 100;
            audio_set_master_volume(audio.vol_percent);
           }
        if (! gui_status.vol)
           gui_status_set_persist(GUI_PERSIST_VOL, 0);
        break;
     case EMU_CMD_VOLUMED :
        if ((gui_status.vol) || (gui.persist_flags & GUI_PERSIST_VOL))
           {
            audio.vol_percent -= EMU_VOLUME_CHANGE;
            if (audio.vol_percent < 0)
               audio.vol_percent = 0;
            audio_set_master_volume(audio.vol_percent);
           }
        if (! gui_status.vol)
           gui_status_set_persist(GUI_PERSIST_VOL, 0);
        break;
    }
}

//==============================================================================
// audio init
//
//   pass: void
// return: int                          0 if no errors,  else -1
//==============================================================================
int audio_init (void)
{
 // set the audio format desired
 wanted.format = AUDIO_FORMAT;
 wanted.channels = AUDIO_CHANNELS;
 wanted.freq = audio.frequency;
 wanted.samples = audio.samples;
 wanted.callback = audio_fill;
 wanted.userdata = NULL;
#if DEBUG_AUDIO
 audio_fill_expected_delay = wanted.samples * 1000 / wanted.freq;
 xprintf("audio_init: wanted "
         "format %d, channels %d, "
         "freq %d, samples %d, "
         "expected delay %d ms\n",
         wanted.format, wanted.channels,
         wanted.freq, wanted.samples,
         audio_fill_expected_delay
         );
#endif
 if (SDL_OpenAudio(&wanted, &obtained) < 0)
    {
     xprintf("audio_init: Couldn't open audio: %s\n", SDL_GetError());
     return -1;
    }
#if DEBUG_AUDIO
 audio_fill_expected_delay = wanted.samples * 1000 / wanted.freq;
 xprintf("audio_init: wanted "
         "format %d, channels %d, "
         "freq %d, samples %d, "
         "expected delay %d ms\n",
         wanted.format, wanted.channels,
         wanted.freq, wanted.samples,
         audio_fill_expected_delay
         );
#endif
#if DEBUG_AUDIO
 audio_fill_expected_delay = obtained.samples * 1000 / obtained.freq;
 xprintf("audio_init: obtained "
         "format %d, channels %d, "
         "freq %d, samples %d, "
         "expected delay %d ms\n",
         obtained.format, obtained.channels,
         obtained.freq, obtained.samples,
         audio_fill_expected_delay
         );
 audio_fill_last = time_get_ms();
#endif
 audio_set_master_volume(100);
 SDL_PauseAudio(0);
 return 0;
}

//==============================================================================
// audio de-init
//
//   pass: void
// return: int                          0 if no errors,  else -1
//==============================================================================
int audio_deinit (void)
{
 SDL_CloseAudio();

 return 0;
}

//==============================================================================
// audio reset
//
//   pass: void
// return: int                          0 if no errors,  else -1
//==============================================================================
int audio_reset (void)
{
 audio_tstates_last = z80api_get_tstates();
 return 0;
}

//==============================================================================
// audio set master volume
//
//   pass: int percent                 volume percent 0 - 100
// return: void
//==============================================================================
void audio_set_master_volume (int percent)
{
 SDL_LockAudio();
 audio_master_volume = SDL_MIX_MAXVOLUME * percent / 100;
 SDL_UnlockAudio();
}

//==============================================================================
// Mix the buffered data from all of the registered audio sources into
// the output buffer.
//
//   pass: void *udata
//         Uint8 *stream        A pointer to the audio buffer to be filled
//         int len              The length (in bytes) of the audio buffer
// return: void
//
// When this function runs, SDL's internal audio mutex is locked.
//==============================================================================
static void audio_fill (void *udata, Uint8 *stream, int len)
{
 audio_source_t *p;

#if DEBUG_AUDIO
 {
  uint64_t now = time_get_ms();
  xprintf("audio_fill: start %llums filling %d, "
          "%llums since last update%s\n",
          now, len,
          now - audio_fill_last,
                                // tolerate 2ms of error before
                                // declaring an audio update late
          (now - audio_fill_last > audio_fill_expected_delay + 2) ?
          " [late]" : "");
  audio_fill_last = now;
 }
#endif

 //
 // Mix the audio for the active sources.
 //
 for (p = &audio_sources[0];
      p < &audio_sources[sizeof(audio_sources)/sizeof(audio_sources[0])];
      ++p)
    {
     if (p->buf)
        {
         DEBUG_MUTEX("audio_fill",
                     p->name,
                     SDL_LockMutex(p->buf->mutex));
         switch (p->state)
            {
             case AUDIO_SOURCE_QUIESCENT:
                {
                 int new_samples;
                 new_samples = p->buf->new_samples;
                 if (new_samples)
                    {
                     p->count = p->holdoff_count;
                     p->state = AUDIO_SOURCE_BUFFERING;
                    }
                }
                break;
             case AUDIO_SOURCE_BUFFERING:
                if (p->count < len)
                   p->state = AUDIO_SOURCE_PLAYING;
                else
                   p->count -= len;
                break;
             case AUDIO_SOURCE_PLAYING:
                {
                 int result;
#if DEBUG_AUDIO
                 xprintf("audio_fill: %s: "
                         "new %d length %d\n",
                         p->name,
                         p->buf->new_samples, p->buf->len);
                 xprintf("audio_fill: %s: %d samples to drain\n",
                         p->name, p->buf->drain_len);
#endif
                 result = audio_source_play(p->buf, udata, stream, len);
                 p->buf->new_samples = 0;
                 /* An audio source that has stopped generating new
                    samples will cause audio_source_play to return 0
                    once all of the outstanding samples have been
                    drained . */
                 if (result == 0 && p->sync)
                    p->state = AUDIO_SOURCE_QUIESCENT;
                 break;
                }
             default:
                assert(0);      // should never happen
            }
         //
         // Signal producer thread if it is blocked waiting for
         // buffers to become available.
         //
         {
          int signal_producer;
          signal_producer = (p->buf->num_clean > 0);
#if DEBUG_AUDIO
          xprintf("audio_fill: %s: "
                  "clean %d dirty %d new %d state %d\n",
                  p->name,
                  p->buf->num_clean, p->buf->num_dirty,
                  p->buf->new_samples, (int)p->state);
#endif
          DEBUG_MUTEX("audio_fill", p->name, SDL_UnlockMutex(p->buf->mutex));
          if (signal_producer)
             SDL_CondSignal(p->buf->cond);
         }
        }
    }

#if DEBUG_MIXER
 audio_dumpstream("result", stream, len);
#endif
#if DEBUG_AUDIO
 {
  uint64_t now;
  now = time_get_ms();
  xprintf("audio_fill: end %llu %llu\n", now, now - audio_fill_last);
 }
#endif
}

//==============================================================================
// Audio source play function.  Mixes the data for the specified audio
// source accumulated since the last call into the supplied sound
// buffer.
//
//   pass: audio_scratch_t *a   A pointer to the audio source structure
//         void *udata
//         Uint8 *stream        A pointer to the audio buffer to be filled
//         int len              The length (in bytes) of the audio buffer
// return: int                  0 if there were no samples to play, non 0
//                              otherwise.
//
// This function assumes that the mutex for the specified audio
// source, a->mutex, has already been locked.
//==============================================================================
int audio_source_play (audio_scratch_t *a, void *udata, Uint8 *stream, int len)
{
 if (a->num_dirty == 0)
    return 0;
    
 int volume = audio.mute ? 0 : audio_master_volume;

 while (len && a->num_dirty)
    {
     if (a->dirty[0]->drain_count)
        {
         int i = a->dirty[0]->drain_count;
         if (i > len)
            i = len;
         SDL_MixAudio(stream,
         a->dirty[0]->samples + a->dirty[0]->count - a->dirty[0]->drain_count,
         i, volume);
         len -= i;
         a->dirty[0]->drain_count -= i;
         stream += i;
         if (!a->dirty[0]->drain_count)
            {
             audio_recycle_buffer(a, a->dirty[0]);
             audio_next_dirty_buffer(a);
            }
        }
     else
        break;
    }
 return 1;
}

//==============================================================================
// Register an audio source
//
//   pass: audio_scratch_t *    pointer to sound buffer type
//         const char *         the name of the audio source, for debugging
//                              and progress messages
//         audio_gen_fn_t audio_func
//                              function that will generate samples
//         audio_clock_fn_t clock_func
//                              function called when the CPU clock changes
//         const void *data     data to pass to the sample function
//         int synchronous      whether samples generation is to be synchronised
//                              with the Z80 emulation
//         int holdoff_time_ms  time to delay before playing accumulated samples
//      
// return: int                  
//==============================================================================
int audio_register(audio_scratch_t *a, const char *name,
                   audio_gen_fn_t audio_func, const void *data,
                   audio_clock_fn_t clock_func,
                   int synchronous,
                   int holdoff_time_ms)
{
 int res;
 audio_source_t *p;

 res = audio_allocate_buffers(a, audio.frequency / emu.framerate);
 a->mutex = SDL_CreateMutex();
 a->cond = SDL_CreateCond();
 assert(res == 0);
 SDL_LockAudio();               /* lock out audio thread, the audio
                                 * sources array is changing */
 for (p = &audio_sources[0];
      p < &audio_sources[sizeof(audio_sources)/sizeof(audio_sources[0])];
      ++p)
    {
     if (!p->buf)
        {
         p->buf = a;
         p->name = name;
         p->audio_func = audio_func;
         p->clock_func = clock_func;
         p->data = data;
         p->sync = synchronous;
         // The holdoff time needs to be converted to a minimum number
         // of samples that need to be played before audio from this
         // audio source can be mixed into the output stream
         p->holdoff_count = holdoff_time_ms * wanted.freq / 1000;
         p->count = 0;
         p->state =
            p->sync && p->holdoff_count > 0 ?
            AUDIO_SOURCE_QUIESCENT : AUDIO_SOURCE_PLAYING;
         if (p->clock_func && emu.cpuclock != 0)
             (*p->clock_func)(emu.cpuclock);
#if DEBUG_AUDIO
         xprintf("audio_register: %s\n", p->name);
#endif
         break;
        }
    }
 SDL_UnlockAudio();
 if (p == &audio_sources[sizeof(audio_sources)/sizeof(audio_sources[0])])
    return 1;                   /* failure */
 else
    return 0;                   /* success */
}

int audio_deregister(audio_scratch_t *a)
{
 audio_source_t *p;

 assert(a);
 SDL_LockAudio();
 for (p = &audio_sources[0];
      p < &audio_sources[sizeof(audio_sources)/sizeof(audio_sources[0])];
      ++p)
    {
     if (p->buf == a)
        {
#if DEBUG_AUDIO
         xprintf("audio_deregister: %s\n", p->name);
#endif
         p->buf = NULL;
         p->name = NULL;
         p->audio_func = NULL;
         p->clock_func = NULL;
         p->data = NULL;
         p->sync = 0;
         p->holdoff_count = 0;
         p->count = 0;
         p->state = AUDIO_SOURCE_QUIESCENT;
        }
    }
 SDL_UnlockAudio();
 SDL_DestroyMutex(a->mutex);
 SDL_DestroyCond(a->cond);
 audio_deallocate_buffers(a);
 return 0;                      /* success */
}

//==============================================================================
// Update an audio source's sample conversion factors.
//
//   pass: int cpuclock         the new CPU clock frequency, Hertz
// return: void
//==============================================================================
void audio_clock(int cpuclock)
{
 audio_source_t *p;

 audio_sources_update();        /* force an update of the audio
                                 * sources before changing the CPU
                                 * clock */

 for (p = &audio_sources[0];
      p < &audio_sources[sizeof(audio_sources)/sizeof(audio_sources[0])];
      ++p)
    {
     if (!p->buf)
        continue;
     if (!p->clock_func)
        continue;
     SDL_LockMutex(p->buf->mutex); // lock out the source
     (*p->clock_func)(cpuclock);
     SDL_UnlockMutex(p->buf->mutex);
    }
}


int audio_allocate_buffers(audio_scratch_t *a, int len)
{
 int i;

 a->len = len ? len : AUDIO_SAMPLES;
 a->num_clean = MAX_AUDIO_BUFFERS;      /* number of clean buffers available */
 a->clean = calloc(a->num_clean, sizeof(a->clean[0]));
 /* assume that the number of dirty buffers will never be more than
  * the number of clean buffers we started with */
 a->dirty = calloc(a->num_clean, sizeof(a->dirty[0]));
 for (i = 0; i < a->num_clean; ++i)
    a->clean[i] = malloc(sizeof(*a->clean[0]) +
                         a->len * sizeof(a->clean[0]->samples[0]));
 a->num_dirty = 0;
 a->cur_buf = NULL;
 return 0;
}

int audio_deallocate_buffers(audio_scratch_t *a)
{
 int i;

 for (i = 0; i < a->num_clean; ++i)
    free(a->clean[i]);
 free(a->clean);
 a->num_clean = 0;
 a->clean = NULL;
 for (i = 0; i < a->num_dirty; ++i)
    free(a->dirty[i]);
 free(a->dirty);
 a->num_dirty = 0;
 a->dirty = NULL;
 if (a->cur_buf)                /* a working buffer has been allocated? */
    {
     free(a->cur_buf);
     a->cur_buf = NULL;
    }
 return 0;
}

//==============================================================================
// Get a fresh buffer to write samples into
//
//   pass: audio_scratch_t *    pointer to audio buffer structure
//      
// return: AUDIO_BUFTYP *       pointer to a buffer of length a->len
//
// This function locks the audio buffer mutex.  If there are no clean buffers
// available, this function will pause until one becomes available.
//==============================================================================
void audio_get_work_buffer(audio_scratch_t *a)
{
 SDL_LockMutex(a->mutex);
 assert(a->num_clean + a->num_dirty >= MAX_AUDIO_BUFFERS - 1);
 assert(!a->cur_buf);
 /* If there are no clean buffers, wait for one to become
  * available */
 while (!a->num_clean)
    {
#if DEBUG_AUDIO
     uint64_t then, delay;

     then = time_get_ms();
#endif
     SDL_CondWait(a->cond, a->mutex);
#if DEBUG_AUDIO
     delay = time_get_ms() - then;
     if (delay > 2)             /* conservative */
        xprintf("audio_get_buffer: "
                "waited from %llu to %llu (%llu ms) for a clean buffer\n",
                then, then + delay, delay);
#endif
    }
 a->cur_buf = a->clean[--a->num_clean];
 a->cur_buf->count = a->cur_buf->drain_count = 0;
 assert(a->num_clean >= 0 &&
        a->num_clean <= MAX_AUDIO_BUFFERS);
 SDL_UnlockMutex(a->mutex);
}

//==============================================================================
// Put a full buffer on the dirty buffers queue
//
//   pass: audio_scratch_t *    pointer to audio buffer structure
//         AUDIO_BUFTYP *       pointer to a buffer of length a->len
//      
// return: void
//
// This function locks the audio buffer mutex.
//==============================================================================
void audio_put_work_buffer(audio_scratch_t *a)
{
 SDL_LockMutex(a->mutex);
 a->cur_buf->drain_count = a->cur_buf->count;
 a->dirty[a->num_dirty++] = a->cur_buf;
 a->cur_buf = NULL;
 assert(a->num_clean + a->num_dirty >= MAX_AUDIO_BUFFERS - 1);
 assert(a->num_dirty >= 0 &&
        a->num_dirty <= MAX_AUDIO_BUFFERS);
 SDL_UnlockMutex(a->mutex);
}

//==============================================================================
// This function updates the dirty buffers queue.  It assumes that the dirty
// buffer at the head of the queue has already been added to the clean buffers
// queue.
//
//   pass: audio_scratch_t *    pointer to audio buffer structure
//      
// return: void
//
// This function is intended to be called from the SDL audio thread.
// It assumes that the audio buffer mutex a->mutex has already been
// locked.
//==============================================================================
void audio_next_dirty_buffer(audio_scratch_t *a)
{
 if (a->num_dirty)
    {
     // shuffle the dirty buffer pointers down.
     memcpy(&a->dirty[0],
            &a->dirty[1],
            (MAX_AUDIO_BUFFERS - 1) * sizeof(a->dirty[0]));
     a->num_dirty--;
     assert(a->num_dirty >= 0 &&
            a->num_dirty <= MAX_AUDIO_BUFFERS);
    }
}

//==============================================================================
// This function adds a drained, previously dirty buffer, to the tail of the
// clean buffers queue.
//
//   pass: audio_scratch_t *    pointer to audio buffer structure
//         AUDIO_BUFTYP *       pointer to a buffer of length a->len
//      
// return: void
//
// This function is intended to be called from the SDL audio thread.
// It assumes that the audio buffer mutex a->mutex has already been
// locked.
//
//==============================================================================
void audio_recycle_buffer(audio_scratch_t *a, audio_buffer_t *buf)
{
 a->clean[a->num_clean++] = buf;
 assert(a->num_clean >= 0 &&
        a->num_clean <= MAX_AUDIO_BUFFERS);
}

#if DEBUG_MIXER
void audio_dumpstream(const char *what, AUDIO_BUFTYP *data, Uint32 len)
{
 AUDIO_BUFTYP lastsample;
 int c = 0;

 xprintf("audio: %s: ", what);
 if (len)
    {
     lastsample = *data++;
     len--;
     c = 1;
     while (len--)
        {
         if (*data != lastsample)
            {
             xprintf("%5d x %02x ", c, lastsample);
             c = 1;
             lastsample = *data;
            }
         else
            c++;
         data++;
        }
     xprintf("%5d x %02x ", c, lastsample);
    }
 else
    xprintf("(empty)");
 xprintf("\n");
}
#endif


//==============================================================================
// This function calls the audio sources' generation function to
// generate the audio samples for the.last frame interval
//
//   pass: void
//      
// return: void
//
// This function is intended to be called periodically from the CPU
// thread.  The audio sources array is assumed to remain unchanged.
//
//==============================================================================
void audio_sources_update(void)
{
 uint64_t tstates_cur = z80api_get_tstates();
 const audio_source_t *p;

#if DEBUG_AUDIO
 xprintf("audio_sources_update: start %lld\n", time_get_ms());
#endif
 for (p = &audio_sources[0];
      p < &audio_sources[sizeof(audio_sources)/sizeof(audio_sources[0])];
      ++p)
    {
     if (p->buf && p->sync)
        {
         (*p->audio_func)(p->buf, p->data,
                          audio_tstates_last,
                          tstates_cur - audio_tstates_last);
        }
#if DEBUG_AUDIO
     if (p->buf)
        {
         // The audio thread will change the audio buffer structure,
         // so the buffer mutex needs to be locked before we touch it
         SDL_LockMutex(p->buf->mutex);
         xprintf("audio_sources_update: %s: "
                 "clean %d dirty %d new %d state %d\n",
                 p->name,
                 p->buf->num_clean, p->buf->num_dirty,
                 p->buf->new_samples, (int)p->state);
//           assert(p->buf->new_samples <= p->buf->len);
         SDL_UnlockMutex(p->buf->mutex);
        }
#endif

    }
 audio_tstates_last = tstates_cur;
#if DEBUG_AUDIO
 xprintf("audio_sources_update: end %lld\n", time_get_ms());
#endif
}

// Each sound generator module has a circular buffer into which samples
// are written at a convenient "native" sampling rate.  When this
// buffer is filled it is drained into the sound buffers (at which
// point sample rate conversion is done).
//
// Returns non-zero if initialisation was successful.
int audio_circularbuf_init(audio_circularbuf_t *cb)
{
 cb->buf = calloc(AUDIO_CIRCULARBUF_SIZE, sizeof(cb->buf[0]));
 cb->head = cb->tail = 0;
 cb->this_sample = cb->next_sample = audio_limit(0);
 cb->index = cb->increment = 0;
 cb->tau = cb->decay = 0;
 return (cb->buf != NULL);
}

int audio_circularbuf_deinit(audio_circularbuf_t *cb)
{
 free(cb->buf);
 cb->buf = NULL;
 return 0;
}

// Sample rate conversion initialisation
//
// Initialise sample rate conversion variables
//
void audio_circularbuf_set_rate_conversion(audio_circularbuf_t *cb,
                                           int dst_rate,
                                           int src_rate)
{
 cb->src_rate = src_rate;
 cb->dst_rate = dst_rate;
 /* -------------------------------------------------------------------- */
 /*  Compute the ratio of the destinaton sampling rate to the source     */
 /*  source sampling rate as a fixed point fraction                      */
 /* -------------------------------------------------------------------- */
 {
  int a = dst_rate, b = src_rate, t;

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
  src_rate /= a;
  dst_rate /= a;
 }
 cb->increment = dst_rate;      /* index is incremented by this much
                                 * when a new sample is processed */
 cb->limit = src_rate;          /* and a sample is output when the
                                 * index goes over the limit */
 cb->index = cb->increment;     /* pretend that one partial sample has
                                 * already been generated */
 cb->rate_num = src_rate;       /* store rate conversion fraction */
 cb->rate_denom = dst_rate;
 cb->this_sample = cb->next_sample =    audio_limit(0); /* start with silence. */

#if DEBUG_AUDIO
 xprintf("audio_circularbuf_set_rate_conversion: "
         "src rate %d, dst_rate %d, "
         "increment %d, index %d\n",
         cb->rate_num, cb->rate_denom,
         cb->index, cb->increment);
#endif
}

void audio_circularbuf_set_decay_constant(audio_circularbuf_t *cb,
                                          int tau)
{
 /* try to normalise the time constant - it's nominally in
  * milliseconds, need to scale it to something appropriate for the
  * source sampling rate. */
 cb->tau = cb->src_rate * tau / 1000;
}

// get the next sample from the circular buffer
static inline AUDIO_BUFTYP audio_circularbuf_get_sample(audio_circularbuf_t *cb, int mask)
{
 return cb->buf[cb->tail++ & mask];
}

//
// Drain all of the accumulated samples into the sound buffers,
// performing sample rate conversion
//
void audio_drain_samples(audio_scratch_t *a, audio_circularbuf_t *cb)
{
 int n;
 int sample_delta;
 AUDIO_BUFTYP sample_output;

 /* ----------------------------------------------------------------- */
 /*  Make sure we have a clean buffer to write in.                    */
 /* ----------------------------------------------------------------- */
 if (!audio_has_work_buffer(a))
    audio_get_work_buffer(a);

 /* ---------------------------------------------------------------- */
 /*  Renormalize our sc_head and sc_tail.                            */
 /* ---------------------------------------------------------------- */
 audio_circularbuf_normalise(cb, AUDIO_CIRCULARBUF_MASK);

 /* ---------------------------------------------------------------- */
 /*  First, drain as much of our circular buffer as we can into the  */
 /*  sound buffers.                                                  */
 /* ---------------------------------------------------------------- */
 n = audio_circularbuf_samples(cb, AUDIO_CIRCULARBUF_SIZE);

#if DEBUG_DISCARD_SAMPLES
 cb->tail = cb->head;
#else

 while (n)
    {
     /* Convert to the destination sample rate by linear
      * interpolation. */
     if (cb->index >= cb->limit)
        {
         cb->this_sample = cb->next_sample;
         cb->next_sample = audio_circularbuf_get_sample(cb,
         AUDIO_CIRCULARBUF_MASK);
         cb->index -= cb->increment;
         n--;
         continue;
        }
     sample_delta = (int)cb->next_sample - (int)cb->this_sample;
        
     while (cb->index < cb->limit)
        {
         if (cb->rate_num > cb->rate_denom)
            sample_output =
               cb->this_sample +
               sample_delta *
               cb->rate_denom / cb->rate_num;
         else
            sample_output =
               cb->this_sample +
               sample_delta *
               cb->rate_num / cb->rate_denom;
         cb->index += cb->limit;
        
         /* -------------------------------------------------------- */
         /*  Store out the current sample.                           */
         /* -------------------------------------------------------- */
         audio_put_sample(a, sample_output);
        
         /* -------------------------------------------------------- */
         /*  Commit the buffer when it's full.                       */
         /* -------------------------------------------------------- */
         if (audio_space_remaining(a) <= 0)
            {
             /* ---------------------------------------------------- */
             /*  Put it on the dirty list.                           */
             /* ---------------------------------------------------- */
             audio_put_work_buffer(a);
                
             /* ---------------------------------------------------- */
             /*  Get a clean buffer.                                 */
             /* ---------------------------------------------------- */
             audio_get_work_buffer(a);
            }
        }
    }
#endif /* DEBUG_DISCARD_SAMPLES */
}
