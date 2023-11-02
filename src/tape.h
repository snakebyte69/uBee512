/* Tape Header */

#ifndef HEADER_TAPE_H
#define HEADER_TAPE_H

#include <inttypes.h>
#include "ubee512.h"
#include "z80.h"

#define TAPE_SAMPLE_FREQ 22050  // default tape out sample frequency (in Hz)
#define TAPE_VOLUME 19          // default tape volume (15% = 19/127)

int tape_init (void);
int tape_deinit (void);
int tape_reset (void);

int tape_check (char *s1, char *s2);
void tape_o_close (void);
void tape_i_close (void);
int tape_o_open (char *s, int action);
int tape_i_open (char *s, int action);

int tape_r (void);
void tape_w (int data);
void tape_config_out (int cpuclock);
void tape_config_in (int cpuclock);
void tape_command (int cmd);

typedef struct wav_t
   {
    char chunk_id[4];
    uint32_t chunk_size;
    char format[4];

    char sub_chunk1_id[4];
    uint32_t sub_chunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;

    char sub_chunk2_id[4];
    uint32_t sub_chunk2_size;
   }wav_t;

typedef struct tape_t
   {
    int in_status;
    FILE *tape_i_file;
    char tapei[SSIZE1];
    FILE *tape_o_file;
    char tapeo[SSIZE1];
    int orate;
    int olevel;
    float detect;
   }tape_t;

#endif     /* HEADER_TAPE_H */
