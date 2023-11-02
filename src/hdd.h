/* HDD Header */

#ifndef HEADER_HDD_H
#define HEADER_HDD_H

#include "disk.h"
#include "z80.h"
#include "macros.h"

#define HDD_NUMDRIVES    7              // 3 hard + 4 floppy drives supported by WD1002-5
#define HDD_MAXTRACK     10000
#define HDD_BUFSIZE      1024*128       // maximum data per track

#define HDD_BASE         0x40

// HDD harddisk controller commands
#define HDD_TEST_CMD     0x90
#define HDD_RESTORE_CMD  0x10
#define HDD_SEEK_CMD     0x70
#define HDD_READ_CMD     0x20
#define HDD_WRITE_CMD    0x30
#define HDD_FORMAT_CMD   0x50

// HDD registers
#define HDD_DATA         0
#define HDD_ERROR        1
#define HDD_PRECOMP      1
#define HDD_SECTORCOUNT  2
#define HDD_SECTOR       3
#define HDD_CYL_LOW      4
#define HDD_CYL_HIGH     5
#define HDD_SDH          6
#define HDD_CMD          7
#define HDD_STATUS       7

// bits in the status register
#define HDD_STA_BUSY       B8(10000000)   // drive busy
#define HDD_STA_RDY        B8(01000000)   // drive ready
#define HDD_STA_WF         B8(00100000)   // write fault
#define HDD_STA_SC         B8(00010000)   // seek complete
#define HDD_STA_DRQ        B8(00001000)   // data request bit - drive ready to transfer data
#define HDD_STA_CORR       B8(00000100)   // soft error detected
#define HDD_STA_NOTUSED    B8(00000010)   // not used
#define HDD_STA_ERROR      B8(00000001)   // set when there is a drive error.

// bits in the SDH register
#define HDD_SDH_CRCECC     B8(10000000)
#define HDD_SDH_SIZE       B8(01100000)
#define HDD_SDH_DRIVE      B8(00011000)
#define HDD_SDH_HDHEAD     B8(00000111)
#define HDD_SDH_FDSEL      B8(00000110)
#define HDD_SDH_FDHEAD     B8(00000001)

// bits in the error register
#define HDD_ERR_BAD_BLOCK  B8(10000000)   // bad block detect
#define HDD_ERR_UNREC      B8(01000000)   // unrecoverable error
#define HDD_ERR_CRC_ERR_ID B8(00100000)   // CRC error ID field
#define HDD_ERR_ID_NFOUND  B8(00010000)   // ID not found
#define HDD_ERR_NOTUSED    B8(00001000)   // -
#define HDD_ERR_ABORT_CMD  B8(00000100)   // aborted command
#define HDD_ERR_TR000      B8(00000010)   // TR000 error
#define HDD_ERR_DAM_NFOUND B8(00000001)   // DAM not found

// 1 of these are used for each HDD and floppy drive.
typedef struct hdd_drive_t
{
 disk_t disk;
}hdd_drive_t;

int hdd_init (void);
int hdd_deinit (void);
int hdd_reset (void);
int hdd_set_drive (int drive, hdd_drive_t *hdd_d);
void hdd_unloaddisk (int d);

uint16_t hdd_data_r (uint16_t port, struct z80_port_read *port_s);
uint16_t hdd_error_r (uint16_t port, struct z80_port_read *port_s);
uint16_t hdd_sectorcount_r (uint16_t port, struct z80_port_read *port_s);
uint16_t hdd_sector_r (uint16_t port, struct z80_port_read *port_s);
uint16_t hdd_cyl_low_r (uint16_t port, struct z80_port_read *port_s);
uint16_t hdd_cyl_high_r (uint16_t port, struct z80_port_read *port_s);
uint16_t hdd_sdh_r (uint16_t port, struct z80_port_read *port_s);
uint16_t hdd_status_r (uint16_t port, struct z80_port_read *port_s);
uint16_t hdd_fd_side_r (uint16_t port, struct z80_port_read *port_s);

void hdd_data_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void hdd_precomp_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void hdd_sectorcount_w (uint16_t port, uint8_t data,
                        struct z80_port_write *port_s);
void hdd_sector_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void hdd_cyl_low_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void hdd_cyl_high_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void hdd_sdh_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void hdd_cmd_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
void hdd_fd_side_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

void hdd_fdc_select_w (uint16_t port, uint8_t data,
                       struct z80_port_write *port_s);

#endif  /* HEADER_HDD_H */
