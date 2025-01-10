//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                          hdd (WD1002-5) module                             *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulates a Western Digital WD1002-5 Winchester/floppy disk controller.
//
// The WD1002-5 provides 3 hard and 4 floppy disk drives capability.
//
// Reference:
// WD1002-5/HDO
// Winchester/Floppy Disk
// Controller
// OEM Manual
//
// Document No: 61-031050-0030
//
// http://maben.homeip.net/static/S100/western%20digital/cards/
// WD%201002-05%20Winchester%20Controller.pdf
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
// v5.5.0 - 8 July 2013, uBee
// - Changes required to disable port 0x58 by default as this was a 3rd
//   party modification and to boot a standard Microbee HDD ROM it must be
//   disabled.  To use the 3rd party port 58h emulation requires a --port58h
//   option to be specified.
//   Changes made to hdd_init(), hdd_reset() and hdd_fdc_select_w().
// v5.5.0 - 6 June 2013, B.Robinson
// - Fixed a potential bug in HDD_FORMAT_CMD as byte_count was not zeroed.
//
// v4.6.0 - 4 May 2010, uBee
// - Renamed log_port() calls to log_port_1().
//
// v4.2.0 - 9 July 2010, uBee
// - Created new hdd.c module.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>

#include "hdd.h"
#include "z80api.h"
#include "disk.h"
#include "ubee512.h"
#include "gui.h"
#include "options.h"
#include "z80.h"
#include "support.h"

#include "macros.h"

static int hdd_loaddisk (int drive, int report);

//==============================================================================
// structures and variables
//==============================================================================
static hdd_drive_t hdd_drive[HDD_NUMDRIVES];

static int drive;
static int error;
static int byte_count;
static int sector_count;
static int head;
static int use_head;
static int sector_size;
static char format_buffer_00h[1024];
static char format_buffer_e5h[1024];
static char buffer[1024];
static void *bufptr;
static uint8_t regs[8];  // registers
static int port48h;

static int cmd;
static int cmd_readintr;
static int cmd_longbit;
static int cmd_multisect;

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;

//==============================================================================
// Initialise.
//
//   pass: void
// return: int                  0 if no errors, else -1
//==============================================================================
int hdd_init (void)
{
 int i;
 int res = 0;

 if (! modelx.hdd)
    return 0;

 // set port 0x58 in ports map to select FDC or WD1002-5 card if emulating
 // port 0x58 to associate ports 0x40-0x47 with the WD1002-5 or WD2793.
 if (emu.port58h_use)
    {
     z80_set_port_58h();
     emu.port58h = 0;
    }
    
 z80_hdd_ports();

 memset(format_buffer_e5h, 0xe5, sizeof(format_buffer_e5h));

 for (i = 0; i < HDD_NUMDRIVES; i++)
    {
     hdd_drive[i].disk.fdisk = NULL;
     hdd_drive[i].disk.itype = 0;
     hdd_drive[i].disk.drive = i;

     if (hdd_drive[i].disk.filename[0])
        {
         res = (hdd_loaddisk(i, 1));
         if (res == -1)
            return -1;
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
int hdd_deinit (void)
{
 int i;

 if (! modelx.hdd)
    return 0;

 for (i = 0; i < HDD_NUMDRIVES; i++)
    hdd_unloaddisk(i);

 return 0;
}

//==============================================================================
// Reset the controller.
//
//   pass: void
// return: int                  0
//==============================================================================
int hdd_reset (void)
{
 if (! modelx.hdd)
    return 0;

 drive = 0;
 error = 0;
 byte_count = 0;
 sector_count = 0;

 memset(regs, 0, sizeof(regs));

 // set port 0x58 in ports map to select FDC or WD1002-5 card if emulating
 // port 0x58 to associate ports 0x40-0x47 with the WD1002-5 or WD2793.
 if (emu.port58h_use)
    {
     z80_set_port_58h();
     emu.port58h = 0;
    }
     
 z80_hdd_ports();

 return 0;
}

//==============================================================================
// Set drive.
//
// This function is called to set the HDD drives on startup.
//
//   pass: int d                drive number
//         hdd_drive_t *hdd_d   pointer to drive data parameters
// return: int                  0 if no errors,  else -1
//==============================================================================
int hdd_set_drive (int d, hdd_drive_t *hdd_d)
{
 if ((d < 0) || (d > HDD_NUMDRIVES))
    return -1;

 modelx.hdd = 1;

 // unload (close) an existing open image/drive
 hdd_unloaddisk(d);

 // set the drive number
 hdd_d->disk.drive = d;

 memcpy(&hdd_drive[d], hdd_d, sizeof(hdd_drive_t));

 if (emu.runmode)
    return hdd_loaddisk(d, 1);
 else
    return 0;
}

//==============================================================================
// Load HDD disk.
//
//   pass: int d                drive number
//         int report           1 if require error message if file not opened
// return: int                  0 if no errors, else -1
//==============================================================================
static int hdd_loaddisk (int d, int report)
{
 hdd_unloaddisk(d);

 if (disk_open(&hdd_drive[d].disk) != 0)
    {
     hdd_unloaddisk(d);
     if (report)
        {
         switch (hdd_drive[d].disk.error)
            {
             case DISK_ERR_NOTFOUND:
                xprintf("hdd_loaddisk: File not found: %s\n",
                hdd_drive[d].disk.filepath);
                break;
             case DISK_ERR_READONLY:
                xprintf("hdd_loaddisk: File is read only access: %s\n",
                hdd_drive[d].disk.filepath);
                break;
             default:
                xprintf("hdd_loaddisk: Unknown disk error: %s\n",
                hdd_drive[d].disk.filepath);
                break;
            }
        }
     return -1;
    }
 return 0;
}

//==============================================================================
// Un-load HDD disk.
//
//   pass: int d                        drive number
// return: void
//==============================================================================
void hdd_unloaddisk (int d)
{
 if (hdd_drive[d].disk.itype)
    {
     disk_close(&hdd_drive[d].disk);
     hdd_drive[d].disk.fdisk = NULL;
     hdd_drive[d].disk.itype = 0;
    }
}

//==============================================================================
// Get use-head value.
//
// Forces side selection if floppy access depending on the port 0x48 value.
//
//   pass: void
// return: void
//==============================================================================
static void hdd_get_use_head (void)
{
 if (drive > 2)  // if a floppy drive
    {
     if (port48h & 0x01)
        use_head = 1;
     else
        use_head = head;
    }
 else
    use_head = head;
}

//==============================================================================
// Get data.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t hdd_data_r (uint16_t port, struct z80_port_read *port_s)
{
 int res;
 int cylinder;
 uint8_t *buf;

 if (! sector_count)
    {
     regs[HDD_STATUS] &= ~HDD_STA_DRQ;
     return 0;
    }

 if (! byte_count)
    {
     if (modio.hdd)
        log_port_1("hdd_data_r", "sector", port, regs[HDD_SECTOR]);

     bufptr = buffer;
     byte_count = sector_size;

     hdd_get_use_head();
     cylinder = ((regs[HDD_CYL_HIGH] << 8) | regs[HDD_CYL_LOW]) & 0x03FF;

     res = disk_read(&hdd_drive[drive].disk, buffer,
                     use_head, use_head,
     cylinder, regs[HDD_SECTOR], 0);

     if (res)
        {
         regs[HDD_STATUS] |= HDD_STA_ERROR;
         error = HDD_ERR_DAM_NFOUND | HDD_ERR_ID_NFOUND;
        }
    }

 if (! --byte_count)
    {
     if (! --sector_count)
        regs[HDD_STATUS] &= ~HDD_STA_DRQ;

     if (cmd_multisect)
        {
         if (! sector_count)
            regs[HDD_SECTORCOUNT] = 0;
         else
            regs[HDD_SECTORCOUNT]--;
         regs[HDD_SECTOR]++;
        }
    }

 buf = bufptr++;

 return *buf;
}

//==============================================================================
// Get error.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t hdd_error_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_error_r", "error", port, error);

 return error;
}

//==============================================================================
// Get sector count.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t hdd_sectorcount_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_sectorcount_r", "sectorcount", port, regs[HDD_SECTORCOUNT]);

 return regs[HDD_SECTORCOUNT];
}

//==============================================================================
// Get sector number.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t hdd_sector_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_sector_r", "sector", port, regs[HDD_SECTOR]);

 return regs[HDD_SECTOR];
}

//==============================================================================
// Get cylinder number low.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t hdd_cyl_low_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_cyl_low_r", "cyl_low", port, regs[HDD_CYL_LOW]);

 return regs[HDD_CYL_LOW];
}

//==============================================================================
// Get cylinder number high.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t hdd_cyl_high_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_cyl_high_r", "cyl_high", port, regs[HDD_CYL_HIGH]);

 return regs[HDD_CYL_HIGH];
}

//==============================================================================
// Get sector, drive and head.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t hdd_sdh_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_sdh_r", "drv_head", port, regs[HDD_SDH]);

 return regs[HDD_SDH];
}

//==============================================================================
// Get status.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t hdd_status_r (uint16_t port, struct z80_port_read *port_s)
{
 regs[HDD_STATUS] |= HDD_STA_RDY;

 if (modio.hdd)
    log_port_1("hdd_status_r", "status", port, regs[HDD_STATUS]);

 return regs[HDD_STATUS];
}

//==============================================================================
// Set side for WD1002-5 card floppy (modification to overide the WD2797 side)
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t
//==============================================================================
uint16_t hdd_fd_side_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_fd_side_r", "data", port, port48h);

 return port48h;
}

//==============================================================================
// Write data.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void hdd_data_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 int cylinder;
 int sector;
 int res;
 int i;

 uint8_t *buf;

 regs[port & 0x07] = data;

 if (! sector_count)
    return;

 if (! byte_count)
    {
     bufptr = buffer;
     byte_count = sector_size;
    }

 buf = (bufptr++);
 *buf = data;

 if (! --byte_count)
    {
     if (modio.hdd)
        log_port_1("hdd_data_w", "sector", port, regs[HDD_SECTOR]);

     hdd_get_use_head();
     cylinder = ((regs[HDD_CYL_HIGH] << 8) | regs[HDD_CYL_LOW]) & 0x03FF;

     if (cmd == HDD_FORMAT_CMD)
        {
         if (emu.verbose > 1)
            xprintf("D=%d C=%04d H=%d: ", drive, cylinder, use_head);

         i = 1;
         while (sector_count--)
            {
             sector = (uint8_t)buffer[i];
             i += 2;  // to the next sector number data (skip bad sector data)

             if (drive >= 2)
                res = disk_write(&hdd_drive[drive].disk, format_buffer_e5h,
                                 use_head, use_head,
                                 cylinder, sector, 0);
             else
                res = disk_write(&hdd_drive[drive].disk, format_buffer_00h,
                                 use_head, use_head,
                                 cylinder, sector, 0);

             if (res)
                {
                 regs[HDD_STATUS] |= HDD_STA_ERROR;
                 error = HDD_ERR_DAM_NFOUND | HDD_ERR_ID_NFOUND;
                }

             if (emu.verbose > 1)
                xprintf("%02x ", sector);

             if (! sector_count)
                regs[HDD_SECTORCOUNT] = 0;
             else
                regs[HDD_SECTORCOUNT]--;
            }

          if (emu.verbose > 1)
             xprintf("\n");

         regs[HDD_STATUS] &= ~HDD_STA_DRQ;
        }
     else
        {
         res = disk_write(&hdd_drive[drive].disk, buffer,
                          use_head, use_head,
         cylinder, regs[HDD_SECTOR], 0);

         if (res)
            {
             regs[HDD_STATUS] |= HDD_STA_ERROR;
             error = HDD_ERR_DAM_NFOUND | HDD_ERR_ID_NFOUND;
            }

         if (! --sector_count)
            regs[HDD_STATUS] &= ~HDD_STA_DRQ;

         if (cmd_multisect)
            {
             if (! sector_count)
                regs[HDD_SECTORCOUNT] = 0;
             else
                regs[HDD_SECTORCOUNT]--;
             regs[HDD_SECTOR]++;
            }
        }
    }
}

//==============================================================================
// Set write precompensation.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void hdd_precomp_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_precomp_w", "data", port, data);

 regs[port & 0x07] = data;
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
void hdd_sectorcount_w (uint16_t port, uint8_t data,
                        struct z80_port_write *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_sectorcount_w", "data", port, data);

 regs[port & 0x07] = data;
}

//==============================================================================
// Set sector number.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void hdd_sector_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_sector_w", "data", port, data);

 regs[port & 0x07] = data;
}

//==============================================================================
// Set cylinder number low byte.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void hdd_cyl_low_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_cyl_low_w", "data", port, data);

 regs[port & 0x07] = data;
}

//==============================================================================
// Set cylinder number high byte.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void hdd_cyl_high_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_cyl_high_w", "data", port, data);

 regs[port & 0x07] = data;
}

//==============================================================================
// Set sector, drive and head.
//
// The sector size value is used to determine the amount of data to transfer.
//
// The drive value is numbered from 0-7. This number is derived from the
// drive select and floppy drive select bits. Drives 0-2 are hard disk and
// 3-7 are floppy disk types.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void hdd_sdh_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 const int sec_table[4] = {256, 512, 1024, 128};

 if (modio.hdd)
    log_port_1("hdd_sdh_w", "data", port, data);

 regs[port & 0x07] = data;

 sector_size = sec_table[(regs[HDD_SDH] & HDD_SDH_SIZE) >> 5];

 drive = (regs[HDD_SDH] & HDD_SDH_DRIVE) >> 3;

 if (drive == 3)  // if a floppy drive
    {
     drive += ((regs[HDD_SDH] & HDD_SDH_FDSEL) >> 1);
     head = (regs[HDD_SDH] & HDD_SDH_FDHEAD);
     regs[HDD_STATUS] |= HDD_STA_SC;
    }
 else
    head = (regs[HDD_SDH] & HDD_SDH_HDHEAD);
}

//==============================================================================
// HDD command.
//
// The Microbee uses port 0x48 when the WD1002-5 card is in context to force
// the head selection for floppy access.  This is required as the WD2797 FDC
// DOES pay attention to the side value stored in the sector IDs whereas the
// WD2793 on the Microbee didn't.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void hdd_cmd_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_cmd_w", "data", port, data);

 regs[HDD_STATUS] &= ~HDD_STA_ERROR;

 cmd = data & B8(11110000);

 switch (cmd)
    {
     case HDD_TEST_CMD : // test command
        if (modio.hdd)
           log_mesg("hdd_cmd_w: test command");
        break;

     case HDD_RESTORE_CMD : // restore command
        if (modio.hdd)
           log_mesg("hdd_cmd_w: restore command");
        if (! hdd_drive[drive].disk.itype)
           {
            regs[HDD_STATUS] |= HDD_STA_ERROR;
            error = HDD_ERR_TR000 | HDD_ERR_ID_NFOUND;
           }
        break;

     case HDD_SEEK_CMD : // seek command
        regs[HDD_STATUS] |= HDD_STA_SC;
        break;

     case HDD_READ_CMD : // read sector command
        cmd_longbit = data & B8(00000010);
        cmd_multisect = data & B8(00000100);
        cmd_readintr = data & B8(00001000);
        byte_count = 0;

        if (! hdd_drive[drive].disk.itype)
           {
            regs[HDD_STATUS] |= HDD_STA_ERROR;
            error = HDD_ERR_DAM_NFOUND | HDD_ERR_ID_NFOUND;
            break;
           }

        if (cmd_multisect)
           {
            if (regs[HDD_SECTORCOUNT] == 0)
               sector_count = 256;
            else
               sector_count = regs[HDD_SECTORCOUNT];
           }
        else
           sector_count = 1;
        break;

     case HDD_WRITE_CMD : // write sector command
        cmd_longbit = data & B8(00000010);
        cmd_multisect = data & B8(00000100);
        byte_count = 0;

        if (! hdd_drive[drive].disk.itype)
           {
            regs[HDD_STATUS] |= HDD_STA_ERROR;
            error = HDD_ERR_DAM_NFOUND | HDD_ERR_ID_NFOUND;
            break;
           }

        if (cmd_multisect)
           {
            if (regs[HDD_SECTORCOUNT] == 0)
               sector_count = 256;
            else
               sector_count = regs[HDD_SECTORCOUNT];
           }
        else
           sector_count = 1;

        regs[HDD_STATUS] |= HDD_STA_DRQ;
        break;

     case HDD_FORMAT_CMD : // format command
        byte_count = 0;
        
        if (! hdd_drive[drive].disk.itype)
           {
            regs[HDD_STATUS] |= HDD_STA_ERROR;
            error = HDD_ERR_DAM_NFOUND | HDD_ERR_ID_NFOUND;
            break;
           }

        if (regs[HDD_SECTORCOUNT] == 0)
           sector_count = 256;
        else
           sector_count = regs[HDD_SECTORCOUNT];
        break;

     default :
        if (modio.hdd)
           log_mesg("hdd_cmd_w: Unknown command!");
    }

 gui_status_set_persist(GUI_PERSIST_DRIVE, drive + '0');
}

//==============================================================================
// Set side for WD1002-5 card floppy (modification to overide the WD2797 side)
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void hdd_fd_side_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modio.hdd)
    log_port_1("hdd_fd_side_w", "data", port, data);

 port48h = data;
}

//==============================================================================
// This is a 3rd party circuit modification (added port 0x58) that allows
// associating ports 0x40-0x47 to the WD1002-5 or the Coreboard WD2793 FDC
// controller.  A 3rd party boot ROM is used to select what is required.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void hdd_fdc_select_w (uint16_t port, uint8_t data,
                       struct z80_port_write *port_s)
{
 if (! emu.port58h_use)
    return;

 if (modio.hdd)
    log_port_1("hdd_fdc_select_w", "data", port, data);

 emu.port58h = data;
 z80_hdd_ports();
}
