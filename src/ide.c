//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                                IDE module                                  *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulates IDE hard disk controller.  This module at minimum only emulates
// what is strictly neccessary.
//
// A Primary and Secondary interface with Master/Slave emulation providing
// up to 4 IDE drives in total.
//
// Reference:
// Information Technology - AT Attachment-3 Interface (ATA-3)
// ANSI X3.298 - 1997
//
// http://www.t13.org/Documents/UploadedDocuments/project/d2008r7b-ATA-3.pdf
//
//==============================================================================
/*
 *  uBee512 - An emulator for the Microbee Z80 ROM, FDD and HDD based models.
 *  Copyright (C) 2007-2016 uBee   
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
//==============================================================================
// ChangeLog (most recent entries are at top)
//==============================================================================
// v5.7.0 - 13 December 2015, uBee
// - Added member 'cf8' to ide_x_t structure to enable 8 bit data transfer
//   mode for CF cards.  This is set when calling ide_error_w() with data = 1.
// - Added IDE_FEATURES_CMD to ide_cmd_w() for CF8 support but does nothing.
// - Changes to ide_data_w() to not use SWAP bytes for CF8 mode.
//
// v5.7.0 - 13 December 2015, K Duckmanton
// - Added code to ide_cmd_w() to return if no disk present.  Fixes for
//   IDE_IDENTIFY_CMD as was using the wrong emulated drive and IDE_READ_CMD
//   as wasn't setting IDE_D_DRQ.
//
// v5.5.0 - 26 August 2012, uBee
// - Removed unused 'error' variable in ide_data_w() and ide_cmd_w() for the
//   time being.
//
// v4.6.0 - 4 May 2010, uBee
// - Renamed log_data() calls to log_data_1() and log_port() to log_port_1().
//
// v4.1.0 - 12 June 2009, uBee
// - Created new ide.c module.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>

#include "ide.h"
#include "z80api.h"
#include "disk.h"
#include "ubee512.h"
#include "gui.h"
#include "options.h"
#include "z80.h"
#include "support.h"

static int ide_loaddisk (int drive, int report);
static void ide_unloaddisk (int d);

//==============================================================================
// structures and variables
//==============================================================================
static ide_drive_t ide_drive[IDE_NUMDRIVES];

static ide_x_t ide_x[2];
static uint8_t regs[2][8];  // Primary and Secondary unaltered registers
static uint8_t dsr_port;

static int drive;
static int iface;
static int swap_bytes;

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;

//==============================================================================
// Initialise.
//
//   pass: void
// return: int                  0 if no errors, else -1
//==============================================================================
int ide_init (void)
{
 int i;
 int res = 0;

 if (modelx.rom)
    return 0;

 for (i = 0; i < IDE_NUMDRIVES; i++)
    {
     ide_drive[i].disk.fdisk = NULL;
     ide_drive[i].disk.itype = 0;
     ide_drive[i].disk.drive = i;

     if (ide_drive[i].disk.filename[0])
        {
         res = (ide_loaddisk(i, 1));
         if (res == -1)
            return -1;
         ide_drive[i].id.log_cylinders =
         host_to_leu16(ide_drive[i].disk.imagerec.tracks);
         ide_drive[i].id.log_heads =
         host_to_leu16(ide_drive[i].disk.imagerec.heads);
         ide_drive[i].id.log_sectrk =
         host_to_leu16(ide_drive[i].disk.imagerec.sectrack);
        }
    }

 return 0;
}

//==============================================================================
// De-initialise.
//
//   pass: void
// return: int                  0
//==============================================================================
int ide_deinit (void)
{
 int i;

 if (modelx.rom)
    return 0;

 for (i = 0; i < IDE_NUMDRIVES; i++)
    ide_unloaddisk(i);

 return 0;
}

//==============================================================================
// Reset the controller.
//
//   pass: void
// return: int                  0
//==============================================================================
int ide_reset (void)
{
 memset(ide_x, 0, sizeof(ide_x));
 swap_bytes = -1;
 return 0;
}

//==============================================================================
// Set drive.
//
// This function is called to set the IDE drives on startup.
//
// It is probably not necessary to call this function when using LibDsk and
// physical floppy disks as sectors are not being buffered, LibDsk images
// WILL require calling this function first.
//
//   pass: int d                drive number
//         ide_drive_t *ide_d   pointer to drive data parameters
// return: int                  0 if no errors,  else -1
//==============================================================================
int ide_set_drive (int d, ide_drive_t *ide_d)
{
 if ((d < 0) || (d > IDE_NUMDRIVES))
    return -1;

 modelx.ide = 1;

 // unload (close) an existing open image/drive
 ide_unloaddisk(d);

 // set the drive number
 ide_d->disk.drive = d;

 memcpy(&ide_drive[d], ide_d, sizeof(ide_drive_t));
 return 0;
}

//==============================================================================
// Load IDE disk.
//
//   pass: int d                drive number
//         int report           1 if require error message if file not opened
// return: int                  0 if no errors, else -1
//==============================================================================
static int ide_loaddisk (int d, int report)
{
 ide_unloaddisk(d);

 if (disk_open(&ide_drive[d].disk) != 0)
    {
     ide_unloaddisk(d);
     if (report)
        {
         switch (ide_drive[d].disk.error)
            {
             case DISK_ERR_NOTFOUND:
                xprintf("ide_loaddisk: File not found: %s\n",
                ide_drive[d].disk.filepath);
                break;
             case DISK_ERR_READONLY:
                xprintf("ide_loaddisk: File is read only access: %s\n",
                ide_drive[d].disk.filepath);
                break;
             default:
                xprintf("ide_loaddisk: Unknown disk error: %s\n",
                ide_drive[d].disk.filepath);
                break;
            }
        }
     return -1;
    }

 return 0;
}

//==============================================================================
// Un-load IDE disk.
//
//   pass: int d                        drive number
// return: void
//==============================================================================
static void ide_unloaddisk (int d)
{
 if (ide_drive[d].disk.itype)
    {
     disk_close(&ide_drive[d].disk);
     ide_drive[d].disk.fdisk = NULL;
     ide_drive[d].disk.itype = 0;
    }
}

//==============================================================================
// Get data.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t ide_data_r (uint16_t port, struct z80_port_read *port_s)
{
 uint8_t *buf;

 buf = ide_x[iface].bufptr++;

 if (ide_x[iface].byte_count)
    {
     ide_x[iface].byte_count--;
     regs[iface][IDE_STATUS] |= IDE_D_DRQ;
    }
 else
     regs[iface][IDE_STATUS] &= ~IDE_D_DRQ;

 if (modio.ide)
    log_port_1("ide_data_r", "data", port, *buf);

 return *buf;
}

//==============================================================================
// Get error.
//
// Should never return any errors.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t ide_error_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.ide)
    log_port_1("ide_error_r", "error", port, ide_x[iface].error);

 return ide_x[iface].error;
}

//==============================================================================
// Get sector count.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t ide_sectorcount_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.ide)
    log_port_1("ide_sectorcount_r", "sectorcount",
    port, regs[iface][IDE_SECTORCOUNT]);

 return regs[iface][IDE_SECTORCOUNT];
}

//==============================================================================
// Get sector number.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t ide_sector_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.ide)
    log_port_1("ide_sector_r", "sector", port, regs[iface][IDE_SECTOR]);

 return regs[iface][IDE_SECTOR];
}

//==============================================================================
// Get cylinder number low.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t ide_cyl_low_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.ide)
    log_port_1("ide_cyl_low_r", "cyl_low", port, regs[iface][IDE_CYL_LOW]);

 return regs[iface][IDE_CYL_LOW];
}

//==============================================================================
// Get cylinder number high.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t ide_cyl_high_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.ide)
    log_port_1("ide_cyl_high_r", "cyl_high", port, regs[iface][IDE_CYL_HIGH]);

 return regs[iface][IDE_CYL_HIGH];
}

//==============================================================================
// Get head number.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t ide_drv_head_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.ide)
    log_port_1("ide_drv_head_r", "drv_head", port, regs[iface][IDE_DRV_HEAD]);

 return regs[iface][IDE_DRV_HEAD];
}

//==============================================================================
// Get status.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t ide_status_r (uint16_t port, struct z80_port_read *port_s)
{
 regs[iface][IDE_STATUS] |= IDE_D_RDY;

 if (modio.ide)
    log_port_1("ide_status_r", "status", port, regs[iface][IDE_STATUS]);

 return regs[iface][IDE_STATUS];
}

//==============================================================================
// Write data.
//
// For non CF8 mode the data word bytes need to be swapped to be in the
// correct order on a write operation.  This is required due to the
// interface HW.  A write command initially sets swap_bytes=-1.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void ide_data_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 int cylinder;
 uint8_t *buf;

 if (modio.ide)
    log_port_1("ide_data_w", "data", port, data);

 if (ide_x[iface].cf8 == 1)
    buf = ide_x[iface].bufptr++;
 else   
    {
     swap_bytes *= -1;
     buf = (ide_x[iface].bufptr++) + swap_bytes;
    }

 *buf = data;

 if (ide_x[iface].byte_count)
    {
     cylinder = (regs[iface][IDE_CYL_HIGH] << 8) | regs[iface][IDE_CYL_LOW];
     if (--ide_x[iface].byte_count == 0)
        {
         // FIX-ME - at some stage report the result of disk_write()
         // int error = 
         disk_write(&ide_drive[drive].disk, ide_x[iface].buffer,
                    regs[iface][IDE_DRV_HEAD] & IDE_DEVHD_B_CHS,
                    regs[iface][IDE_DRV_HEAD] & IDE_DEVHD_B_CHS,
                    cylinder, regs[drive&1][IDE_SECTOR], 0);

         regs[iface][IDE_SECTOR]++;
        }
    }

 regs[iface][port & 0x07] = data;
}

//==============================================================================
// ide_error_w
//
// This appears to be used for another purpose to set 8 bit data transfer
// mode in CF cards and is needed for Greybeard's CF8 support in KDR's
// PJB/CF system.  For this purpose we just save the data value.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void ide_error_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.ide)
    log_port_1("ide_error_w", "data", port, data);

 regs[iface][port & 0x07] = data;

 ide_x[iface].cf8 = data;
}

//==============================================================================
// Set sector count.
//
// 256 sectors are read/write if this value is set to 0.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void ide_sectorcount_w (uint16_t port, uint8_t data,
                        struct z80_port_write *port_s)
{
 if (modio.ide)
    log_port_1("ide_sectorcount_w", "data", port, data);

 regs[iface][port & 0x07] = data;

 if (data > 1)
    log_data_1(
 "ide_sectorcount_w", "No support for multi sector read/write! sectors", data);
}

//==============================================================================
// Set sector number.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void ide_sector_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.ide)
    log_port_1("ide_sector_w", "data", port, data);

 regs[iface][port & 0x07] = data;
}

//==============================================================================
// Set cylinder number low byte.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void ide_cyl_low_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.ide)
    log_port_1("ide_cyl_low_w", "data", port, data);

 regs[iface][port & 0x07] = data;
}

//==============================================================================
// Set cylinder number high byte.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void ide_cyl_high_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.ide)
    log_port_1("ide_cyl_high_w", "data", port, data);

 regs[iface][port & 0x07] = data;
}

//==============================================================================
// Set drive and head.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void ide_drv_head_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.ide)
    log_port_1("ide_drv_head_w", "data", port, data);

 regs[iface][port & 0x07] = data;

 iface = (dsr_port & IDE_DSR_B_DSEL) != 0;
 drive = (iface << 1) | ((regs[iface][IDE_DRV_HEAD] & IDE_DEVHD_B_DEV) >> 4);
}

//==============================================================================
// IDE command.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void ide_cmd_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 int cylinder;

 if (modio.ide)
    log_port_1("ide_cmd_w", "data", port, data);

 // return if there is no disk present.
 if (!ide_drive[drive].disk.filename[0])
    {
     regs[iface][IDE_STATUS] &= ~(IDE_D_RDY | IDE_D_DRQ);
     return;                    
    }

 // reset command
 if ((data >= IDE_RESET_CMD) && (data <= (IDE_RESET_CMD+0x0F)))
    {
     regs[iface][IDE_STATUS] |= IDE_D_RDY;
     return;
    }

 // seek commands 0x70 - 0x7F
 if ((data >= IDE_SEEK_CMD) && (data <= (IDE_SEEK_CMD+0x0F)))
    {
     regs[iface][IDE_STATUS] |= (IDE_D_RDY | IDE_D_SC);
     gui_status_set_persist(GUI_PERSIST_DRIVE, drive + '0');
     return;
    }

 switch (data)
    {
     case IDE_READ_R_CMD : // read sector with retry command
     case IDE_READ_CMD : // read sector, no retry command
        ide_x[iface].bufptr = ide_x[iface].buffer;
        ide_x[iface].byte_count = ide_drive[drive].disk.imagerec.secsize;
        cylinder = (regs[iface][IDE_CYL_HIGH] << 8) | regs[iface][IDE_CYL_LOW];

        // FIX-ME - at some stage report the result of disk_read()
        // int error = 
        disk_read(&ide_drive[drive].disk, ide_x[iface].buffer,
                  regs[iface][IDE_DRV_HEAD] & IDE_DEVHD_B_CHS,
                  regs[iface][IDE_DRV_HEAD] & IDE_DEVHD_B_CHS,
                  cylinder, regs[iface][IDE_SECTOR], 0);
                  
        regs[iface][IDE_SECTOR]++;
        regs[iface][IDE_STATUS] |= IDE_D_RDY | IDE_D_DRQ;
        break;

     case IDE_WRITE_R_CMD : // write sector with retry command
     case IDE_WRITE_CMD : // write sector, no retry command
        ide_x[iface].bufptr = ide_x[iface].buffer;
        ide_x[iface].byte_count = ide_drive[drive].disk.imagerec.secsize;
        swap_bytes = -1;
        regs[iface][IDE_STATUS] |= IDE_D_RDY;
        break;

     case IDE_IDENTIFY_CMD : // identify drive command
        ide_x[iface].bufptr = &ide_drive[drive].id;
        ide_x[iface].byte_count = 256*2;
        regs[iface][IDE_STATUS] |= IDE_D_DRQ;
        break;
    }

 gui_status_set_persist(GUI_PERSIST_DRIVE, drive + '0');
}

//==============================================================================
// Drive selection register.
//
// Combine the drive and device selection bits to select 1 of 4 drives.
// drive 0 (ide-a0) = drive sel (DSEL) = 0, device sel = 0
// drive 1 (ide-a1) = drive sel (DSEL) = 0, device sel = 1
// drive 2 (ide-b0) = drive sel (DSEL) = 1, device sel = 0
// drive 3 (ide-b1) = drive sel (DSEL) = 1, device sel = 1
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void ide_dsr_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.ide)
    log_port_1("ide_dsr_w", "data", port, data);

 dsr_port = data;

 iface = (dsr_port & IDE_DSR_B_DSEL) != 0;
 drive = (iface << 1) | ((regs[iface][IDE_DRV_HEAD] & IDE_DEVHD_B_DEV) >> 4);

 ide_x[iface].poweron = (data & IDE_DSR_B_PWR) == 0;
 ide_x[iface].reset = (data & IDE_DSR_B_RESET) == 0;

 if ((ide_x[iface].poweron) && (ide_x[iface].poweron_last == 0))
    {
     ide_x[iface].poweron_last = 1;
     regs[iface][IDE_STATUS] &= ~IDE_D_RDY;
    }

 if (! ide_x[iface].reset) // if not hold in reset state
    {
     if (ide_x[iface].reset_last) // if it was held in reset state
        {
         ide_x[iface].reset_last = ide_x[iface].reset;
         regs[iface][IDE_STATUS] = 0;
        }
    }
}
