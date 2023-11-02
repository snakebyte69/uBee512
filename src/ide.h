/* IDE Header */

#ifndef HEADER_IDE_H
#define HEADER_IDE_H

#include "disk.h"
#include "z80.h"
#include "macros.h"

#define IDE_NUMDRIVES    4              // primary and secondary each with master/slave
#define IDE_MAXTRACK     10000
#define IDE_BUFSIZE      1024*128       // maximum data per track

#define IDE_BASE         0x60

// IDE harddisk controller commands
#define IDE_RESET_CMD    0x10
#define IDE_SEEK_CMD     0x70
#define IDE_READ_R_CMD   0x20
#define IDE_READ_CMD     0x21
#define IDE_WRITE_R_CMD  0x30
#define IDE_WRITE_CMD    0x31
#define IDE_IDENTIFY_CMD 0xec

// IDE registers
#define IDE_DATA         0
#define IDE_ERROR        1
#define IDE_SECTORCOUNT  2
#define IDE_SECTOR       3
#define IDE_CYL_LOW      4
#define IDE_CYL_HIGH     5
#define IDE_DRV_HEAD     6
#define IDE_CMD          7
#define IDE_STATUS       7

// interface control register and bits in it
#define IDE_DSR          IDE_BASE+16

// bits in the drive selection register
#define IDE_DSR_B_PWR    B8(10000000)
#define IDE_DSR_B_RESET  B8(01000000)
#define IDE_DSR_B_DSEL   B8(00000001)

// bits in the status register
#define IDE_D_BUSY       B8(10000000)   // drive busy
#define IDE_D_RDY        B8(01000000)   // drive ready
#define IDE_D_WF         B8(00100000)   // write fault
#define IDE_D_SC         B8(00010000)   // seek complete
#define IDE_D_DRQ        B8(00001000)   // data request bit - drive ready to transfer data
#define IDE_D_CORR       B8(00000100)   // soft error detected
#define IDE_D_INDEX      B8(00000010)   // index bit - set once every revolution.
#define IDE_D_ERROR      B8(00000001)   // set when there is a drive error.

// bits in the device/head register
#define IDE_DEVHD_B_DEV  B8(00010000)
#define IDE_DEVHD_B_LBA  B8(01000000)
#define IDE_DEVHD_B_CHS  B8(00001111)

#define IDE_SPINUP_COUNT 10             // Try waiting for the HD 10 times to let it spin up
#define IDE_WAIT_TIME    3000           // Wait for up to 3s.

typedef struct ide_id_t
{
 uint16_t confbit;                      // General configuration bit-significant information
 uint16_t log_cylinders;                // Number of logical cylinders
 uint16_t res1;                         // Reserved
 uint16_t log_heads;                    // Number of logical heads
 uint16_t obsolete1;                    // Obsolete
 uint16_t obsolete2;                    // Obsolete
 uint16_t log_sectrk;                   // Number of logical sectors per logical track
 uint8_t filler[498];                   // fill structure to 512 bytes
}ide_id_t;

// 4 of these are used, 1 for each IDE drive.
typedef struct ide_drive_t
{
 ide_id_t id;                           // information about this drive
 disk_t disk;
}ide_drive_t;

// 2 of these are used, 1 for the Primary and 1 for the Secondary drives
typedef struct ide_x_t
{
 int poweron;                           // powered on (1)
 int poweron_last;                      // last powered on state
 int reset;                             // in reset state (1)
 int reset_last;                        // last reset state
 int byte_count;                        // byte count remaining
 int error;
 int cf8;                               // cf8 data xfer mode
 char buffer[1024];                     // sector buffer for this drive
 void *bufptr;                          // buffer pointer
}ide_x_t;

int ide_init (void);
int ide_deinit (void);
int ide_reset (void);
int ide_set_drive (int drive, ide_drive_t *ide_d);

uint16_t ide_data_r (uint16_t port, struct z80_port_read *port_s);
uint16_t ide_error_r (uint16_t port, struct z80_port_read *port_s);
uint16_t ide_sectorcount_r (uint16_t port, struct z80_port_read *port_s);
uint16_t ide_sector_r (uint16_t port, struct z80_port_read *port_s);
uint16_t ide_cyl_low_r (uint16_t port, struct z80_port_read *port_s);
uint16_t ide_cyl_high_r (uint16_t port, struct z80_port_read *port_s);
uint16_t ide_drv_head_r (uint16_t port, struct z80_port_read *port_s);
uint16_t ide_status_r (uint16_t port, struct z80_port_read *port_s);

void ide_data_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void ide_error_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void ide_sectorcount_w (uint16_t port, uint8_t data,
                        struct z80_port_write *port_s);
void ide_sector_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void ide_cyl_low_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void ide_cyl_high_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void ide_drv_head_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void ide_cmd_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void ide_dsr_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

#endif  /* HEADER_IDE_H */
