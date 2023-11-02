/* FDC Header */

#ifndef HEADER_FDC_H
#define HEADER_FDC_H

#include "disk.h"
#include "z80.h"

#define FDC_NUMDRIVES    4

#define FDC_RESTORE      0       /* type I */
#define FDC_SEEK         1
#define FDC_STEP         2
#define FDC_STEPIN       4
#define FDC_STEPOUT      6

#define FDC_READSECT     8       /* type II */
#define FDC_WRITESECT    10

#define FDC_READADDR     12      /* type III */
#define FDC_READTRACK    14
#define FDC_WRITETRACK   15

#define FDC_INTERRUPT    13      /* type IV */


                                 /* status bits - internal controller */
#define FDC_CMULTISECT   0x20000
#define FDC_INTRQ        0x10000
                                 /* status bits - type II commands */
#define FDC_RECTYPE      0x2000  /* stored in the higher byte of the */
#define FDC_RECNOTFOUND  0x1000  /* controller status var */
#define FDC_LOSTDATA     0x0400
#define FDC_DRQ          0x0200
                                 /* status bits - type I commands */
#define FDC_HEADLOADED   0x20
#define FDC_SEEKERROR    0x10
#define FDC_TRACK0       0x04
#define FDC_INDEXPULSE   0x02
                                 /* status bits - common */
#define FDC_NOTREADY     0x80
#define FDC_WRPROT       0x40
#define FDC_CRCERROR     0x08
#define FDC_BUSY         0x01

#define FDC_TYPEII_MASK  0x36

#define FDC_STEPRATE     0x03    /* fdc command data bits */
#define FDC_VERIFY       0x04
#define FDC_LOADHEAD     0x08
#define FDC_UPDATETRACK  0x10
#define FDC_MULTISECT    0x10
#define FDC_SIDE         0x08
#define FDC_DELAY        0x04
#define FDC_CMPSIDE      0x02
#define FDC_DATAMARK     0x01
#define FDC_INTREADY     0x01
#define FDC_INTNOTREADY  0x02
#define FDC_INTINDEX     0x04
#define FDC_INTIMMED     0x08

#define FDC_MAXTRACK     255

#define FDC_BUFSIZE      1024*128       // maximum data per track

                                /* Applied Technology drive/side/density bits */
#define FDC_AT_DRIVE_SELECT_MASK   0x03
#define FDC_AT_SIDE_SELECT_MASK    (1 << 2)
#define FDC_AT_DENSITY_SELECT_MASK (1 << 3)
                                /* Dreamdisk drive/side/density bits */
#define FDC_DD_DRIVE_SELECT_MASK   0x0f
#define FDC_DD_SIDE_SELECT_MASK    (1 << 4)
#define FDC_DD_DENSITY_SELECT_MASK (1 << 6)
#define FDC_DD_RATE_SELECT_MASK    (1 << 5) /* 250kbps/500kbps for 5.25" and 8" drives */

#define FDC_DENSITY_SINGLE         0
#define FDC_DENSITY_DOUBLE         1
#define FDC_RATE_250KBPS           0
#define FDC_RATE_500KBPS           1

typedef struct fdc_drive_t
{
 int track;                             // current track position of the head
 disk_t disk;
}fdc_drive_t;

typedef struct fdc_t
{
 int nodisk;
}fdc_t;

int fdc_init (void);
int fdc_deinit (void);
int fdc_reset (void);
int fdc_set_drive (int drive, fdc_drive_t *fdc_d);
void fdc_unloaddisk (int drive);

uint16_t fdc_status_r (uint16_t port, struct z80_port_read *port_s);
void fdc_cmd_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

uint16_t fdc_data_r (uint16_t port, struct z80_port_read *port_s);
void fdc_data_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
uint16_t fdc_track_r (uint16_t port, struct z80_port_read *port_s);
void fdc_track_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
uint16_t fdc_sect_r (uint16_t port, struct z80_port_read *port_s);
void fdc_sect_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

uint16_t fdc_ext_r (uint16_t port, struct z80_port_read *port_s);
void fdc_ext_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

#endif  /* HEADER_FDC_H */
