/* Memory mapper header */

#ifndef HEADER_MEMMAP_H
#define HEADER_MEMMAP_H

#include "z80.h"
#include "ubee512.h"

#define BLOCK_TOTAL 64
#define BLOCK_SIZE 0x8000
#define BLOCK_HIGH block00

#define BANK_NOROMS  B8(00000100)
#define BANK_ROM3    B8(00100000)
#define BANK_VRAM    B8(00001000)
#define BANK_VADD    B8(00010000)

#define BANK_CF_PC85 B8(10000000)

#if 0
#define MAXMEMHANDLERS 20
#define MEMMAP_BLOCKS 16
#define MEMMAP_MASK  0xF000
#define MEMMAP_SHIFT 12
#else
#define MAXMEMHANDLERS 64+4
#define MEMMAP_BLOCKS 64
#define MEMMAP_MASK   0xFC00
#define MEMMAP_SHIFT  10
#endif

typedef struct memmap_t
{
 int backup;
 int load;
 int save;
 char filepath[SSIZE1];
}memmap_t;

int memmap_init (void);
int memmap_deinit (void);
int memmap_reset (void);
void memmap_init6264 (uint8_t *mem, int banks);
void memmap_init6116 (uint8_t *mem, int banks);
void memmap_init6264 (uint8_t *mem, int banks);
void memmap_mode1_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void memmap_mode2_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
uint8_t *memmap_get_z80_ptr (int addr);
void memmap_configure (void);

#endif  /* HEADER_MEMMAP_H */
