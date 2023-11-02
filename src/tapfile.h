/* TAP Header */

#ifndef HEADER_TAP_H
#define HEADER_TAP_H

#include <inttypes.h>
#include "ubee512.h"
#include "z80.h"

int tapfile_init (void);
int tapfile_deinit (void);
int tapfile_reset (void);
int tapfile_check (char *s1, char *s2);
int tapfile_list (char *s);
int tapfile_i_open (char *s, int action);
void tapfile_i_close (void);
int tapfile_o_open (char *s, int action);
void tapfile_o_close (void);
void tapfile_read (void);
void tapfile_write (void);
void tapfile_command (int cmd);

typedef struct tapfile_t
{
 int in_status;
 int code_patch_status;    // patch flags
 FILE *tape_i_file;
 char tapei[SSIZE1];
 FILE *tape_o_file;
 char tapeo[SSIZE1];
}tapfile_t;

#pragma pack(push, 1)  // push current alignment, alignment to 1 byte boundary

typedef struct dgos_t
{
 char name[6];
 char type;
 uint16_t length;
 uint16_t load;
 uint16_t start;
 uint8_t speed;
 uint8_t autos;
 uint8_t spare;
 uint8_t crc;
}dgos_t;

#pragma pack(pop)       // restore original alignment from stack

#endif     /* HEADER_TAP_H */
