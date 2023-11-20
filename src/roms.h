/* ROMS Header */

#ifndef HEADER_ROMS_H
#define HEADER_ROMS_H

#include "z80.h"

#define PAK_ADDR 0xc000
#define NET_ADDR 0xe000

#define ROM1_SIZE 0x4000
#define ROM2_SIZE 0x4000
#define ROM3_SIZE 0x2000

#define ROMS_MD5_USER 1
#define ROMS_MD5_AUTO 2

typedef struct boot_t
   {
    char romimage[6][20];
    uint8_t *dest;
    int offset;
    int size;
    char romimage2[20];
    char romimage3[20];
   }boot_t;

typedef struct romfix_t
   {
    int ofs;
    int data;
   }romfix_t;

int roms_init (void);
int roms_deinit (void);
int roms_reset (void);

uint16_t roms_nsel_r (uint16_t port, struct z80_port_read *port_s);
void roms_psel_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void roms_create_md5 (void);
int roms_proc_pak_argument (int pak, char *p);

int roms_loadrom (char *name, uint8_t *dest, int size, char *filepath);

#endif     /* HEADER_ROMS_H */
