#ifndef _beethoven_h
#define _beethoven_h

/* $Id: beethoven.h,v 1.1.1.4 2011/03/27 06:04:42 krd Exp $ */

typedef struct ay_update_le_t {
   uint64_t when;               /* time at which to update register
                                 * (z80 clock ticks) */
   uint8_t address, data;       /* register to update and its new
                                 * value */
   struct ay_update_le_t *next;
} ay_update_le_t;
typedef ay_update_le_t *ay_update_list_t;

typedef struct beethoven_t {
   uint8_t addrsel;
   uint8_t address;

   // Sound buffers and other whatnot
   audio_scratch_t  snd_buf;
   ay_3_8910_t      ay_3_8910;
   ay_update_list_t ay_update_head, ay_update_tail;
   uint64_t         cycles_remainder;
} beethoven_t;

#endif /* _beethoven_h */
