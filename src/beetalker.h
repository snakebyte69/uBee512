#ifndef _BEETALKER_H
#define _BEETALKER_H

typedef struct {
   uint8_t data;
   SDL_Thread *workerthread;
   int terminate;               /* flag set if the worker thread is to terminate */
   uint32_t workerthreadid;     /* thread ID of the worker thread. */

   audio_scratch_t snd_buf;        /* Sound circular buffer.                        */

   sp0256_t     sp0256;
   SDL_mutex    *sp0256_mutex;
} beetalker_t;
#endif /* _BEETALKER_H */
