/* z80 Header */

#ifndef HEADER_Z80_H
#define HEADER_Z80_H

#include <stdint.h>

#define Z80_PORTS_NONE      0x00000000
#define Z80_PORTS_SN76489AN (1 <<  0)
#define Z80_PORTS_TCKEYS    (1 <<  1)
#define Z80_PORTS_LVDAT     (1 <<  2)
#define Z80_PORTS_PIOA      (1 <<  3)
#define Z80_PORTS_PIOB      (1 <<  4)
#define Z80_PORTS_ROMLATCH  (1 <<  5)
#define Z80_PORTS_RTC       (1 <<  6)
#define Z80_PORTS_COLOUR    (1 <<  7)
#define Z80_PORTS_CRTC      (1 <<  8)
#define Z80_PORTS_CPUCLOCK  (1 <<  9)
#define Z80_PORTS_COLWOFF   (1 << 10)
#define Z80_PORTS_PAKNET    (1 << 11)
#define Z80_PORTS_FDC       (1 << 12)
#define Z80_PORTS_MEMMAP    (1 << 13)
#define Z80_PORTS_CFCB      (1 << 14)
#define Z80_PORTS_IDE       (1 << 15)
#define Z80_PORTS_FDCHDD    (1 << 16)
#define Z80_PORTS_SCC       (1 << 17)
#define Z80_PORTS_UBEE512   (1 << 18)
#define Z80_PORTS_ALL       0xffffffff

struct z80_memory_read_byte
{
 uint32_t low_addr;
 uint32_t high_addr;
 uint8_t (*memory_call)(uint32_t, struct z80_memory_read_byte *);
 void *p_user_area;
};

struct z80_memory_write_byte
{
 uint32_t low_addr;
 uint32_t high_addr;
 void (*memory_call)(uint32_t, uint8_t, struct z80_memory_write_byte *);
 void *p_user_area;
};

struct z80_port_read
{
 uint16_t low_addr;
 uint16_t high_addr;
 uint16_t (*io_call)(uint16_t, struct z80_port_read *);
 void *p_user_area;
};      

struct z80_port_write
{
 uint16_t low_addr;
 uint16_t high_addr;
 void (*io_call)(uint16_t, uint8_t, struct z80_port_write *);
 void *p_user_area;
};

void z80_port_rset (int port, void *handler);
void z80_port_wset (int port, void *handler);
void z80_ports_set (int flags);
int z80_init (void);
int z80_deinit (void);
int z80_reset (void);
void z80_cf_ports (void);
void z80_hdd_ports (void);
void z80_set_port_58h (void);
uint16_t z80_unhandled_r (uint16_t port, struct z80_port_read *port_s);
void z80_unhandled_w (uint16_t port, uint8_t data, 
                      struct z80_port_write *port_s);

#endif /* HEADER_Z80_H */
