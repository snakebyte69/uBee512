//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              WD2793 FDC module                             *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulates Applied Technology's and Dreamdisk WD2793 floppy disk controller.
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
// v5.7.0 - 1 February 2014, uBee
// - Fixed a major bug that prevents correct operation of 128 and 1024 byte
//   size sectors in FDC_READSECT and FDC_WRITESECT.  The idfield->seclen
//   will hold values of 0, 1, 2, 3 sector size physical shift values for
//   128, 256, 512, and 1024 byte sectors respectively.  The code
//   "fdc_drive[ctrl_drive].disk.secsize = idfield->seclen << 8" is wrong
//   and has been replaced with" fdc_drive[ctrl_drive].disk.secsize = 128 <<
//   idfield->seclen".
//
// v4.6.0 - 4 May 2010, uBee
// - Renamed log_data() calls to log_data_1() and log_port() to log_port_1().
//
// v4.3.0 - 4 August July 2009, uBee
// - Extensive changes to support Dreamdisk emulation and to emulate actual
//   FDC timings. (workerbee)
// - Code reformatting and breaking down the fdc_data_w_ready() function into
//   several easier to maintain functions. (ubee)
//
// v4.2.0 - 9 July 2009, uBee
// - Prevent loading the fallback disk if any hard drives used.
//
// v4.1.0 - 21 June 2009, uBee
// - Removed all modelx.rom tests from port handlers as these are now
//   configured for model types.
// - Made improvements to the logging process.
//
// v4.0.0 - 19 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
//
// v3.1.0 - 22 April 2009, uBee
// - Added conditional message reporting based on --verbose option.
// - Removed all occurrences of console_output() function calls.
// - FDC commands FDC_WRITESECT and FDC_WRITETRACK fixed to handle a write
//   protected disk correctly, O/S system will no longer hang now and should
//   report an error message. Fix submitted by workerbee.
// - Sector probing is no longer optional in FDC commands FDC_READSECT and
//   FDC_WRITESECT, probing always takes place now.
// - Changed all printf() calls to use a local xprintf() function.
// - Changed fdc_unloaddisk() to public so options can be used to close disks.
// - Added fdc_set_drive() function to set/change the drive disk/image.
// - Fixed FDC head positioning commands (type-I) to be acted upon even if
//   no disk/image. This needed to be fixed to allow disk/image changing to
//   work with the new live options support.
//
// v2.8.0 - 31 August 2008, uBee
// - Changes to the way GUI status persist works for the drive persist, now
//   uses a gui_status_set_persist() function call.
//
// v2.7.0 - 11 June 2008, uBee
// - Added drive member to disk_t structure.
// - Removed drive 'Image X' reporting, now handled in disk.c module.
// - fdc_loaddisk() and disk_open() functions no longer uses the file name
//   parameter.  The filename(s) members in disk_t are now used.
// - Better error reporting for when a file is read only or is not found.
// - Added Index pulse emulation, correct emulation of this is required for
//   Premium BN60 and 256TC v1.2 Boot/Net ROMs.
// - The --nodisk option now stays in affect until any key is pressed. It now
//   also prevents the Index pulse being returned.
// - Added structure emu_t and moved variables into it.
//
// v2.6.0 - 4 May 2008, uBee
// - Added two missing compile conditionals '#ifdef USE_LIBDSK' around where
//   disk.dg.dg_secsize is used.
//
// v2.4.0 - 13 February 2008, uBee
// - Make changes to incorporate LibDsk and multio reads.  Sector range
//   checking is now hanlded by the disk.c module.
// - Added emulation of the FDC write track command.  Format and Init
//   programs now work.
// - Changes to the fdc_data_w() function to handle multi sector writes.
// - Added sector size probing option for read/write sectors.
// - Added error checking and setting of status registers has been revised.
// - Sector size probing option to allow protected disks to work.
// - Added a disk drive activity status function call.
// - Removed malloc and use static intances of fdc_drive[drive].disk
// - Restructured the disk_t to inlude file names, etc
//
// v2.3.0 - 21 January 2008, uBee
// - Print references to images now includes floppy disk.
// - Added modio_t structure.
//
// v2.2.0 - 12 December 2007, uBee
// - Added disk write protection to FDC_WRITESECT command.
// - The FDC_READSECT and FDC_WRITESECT commands did not have the correct
//   sectoffs value for DS80 DSK format images when the data track sector
//   offset was obtained.  The problem only affected DSK images where the
//   first sector number is bigger than 1. Now use the disk_read_idfield()
//   function to obtain the sectoffs value.
// - No loading of the fallback boot disk if emulating a machine of less than
//   128K memory.
//
// v2.1.0 - 24 October 2007, uBee
// - The FDC_READADDR command now calls a new disk_read_idfield() function to
//   obtain the track ID field.  The previous code had assumed all sectors were
//   in the 1 to 10 range and would not work on the 256TC and the DS80 Modular
//   disk images,  the sector size also returns the size for the disk selected.
// - Changes made to allow disk formats that have sectors that start from
//   greater than 1 to be read. i.e DS80 Microbee disks are 10 sectors, with
//   sector numbers 21-30 on the data tracks.
// - Implement the modelx information structure.
//
// v2.0.0 - 7 October 2007, uBee
// - Each model emulated now has a preferred boot image.  If that image can not
//   be opened then the fallback image is used.
// - Added fdc_status0 variable for a --nodisk option.  This returns a status
//   value of 0 from the fdc_ext_r() function, and is used to simulate a no
//   disk in drive condition.
//
// v1.4.0 - 26 September 2007, uBee
// - Changes to ignore functions if a ROM model is being emulated.
// - Added changes to error reporting, replaced exit(1) with return -1 in
//   fdc_loaddisk() function.
//
// v1.3.0 - 1 September 2007, uBee
// - Changes some error messages and redirection.
//
// v1.0.0 - 2 July 2007, uBee
// - Implement DSK disk image format instead of raw images.
// - All references to '10' sectors per track have been replaced by
//   drive->disk->imagerec.sectrack and references to '512' have been replaced
//   by drive->disk->imagerec.secsize.
// - Disk images are no longer assumed to MAXTRACK=39,  for now change to
//   255 (largest value allowed for WD2793 ?)
// - BUFSSZE increased from 10x512 to 10x1024 byte sectors.
// - Added 2 more drives (now is A-D).  The drive images can be specified on
//   the command line. If drive 'A' is not specified then "boot.dsk" will be
//   used by default.  Drives 'B-D' must be specified if required.
// - implemented case FDC_WRITESECT: in fdc_cmd function.
// - implemented fdc_data_w function.
// - in fdc_data_r changed the ctrl_status value at end sector read.  The
//   value of FDC_WRPROT did not look correct here.
// - Added a second drive.
//
// v0.0.0 - 5 June 2007, uBee
// Start with "nanowasp" source distribution version 0.22. An emulator for the
// microbee 128k. Copyright (C) 2000-2003  David G. Churchill.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "fdc.h"
#include "z80api.h"
#include "disk.h"
#include "ubee512.h"
#include "gui.h"
#include "options.h"
#include "z80.h"

static int fdc_loaddisk (int drive, int report);
static int fdc_bootimage (void);
static void fdc_nmi(void);
static void fdc_update_drv_status(void);
static int fdc_data_r_ready(void);
static int fdc_data_w_ready(void);
static void fdc_schedule_data(int buflen, char *buf, uint64_t start_cycles);
static void fdc_update_data_interval(void);

//==============================================================================
// structures and variables
//==============================================================================
fdc_t fdc;

fdc_drive_t fdc_drive[FDC_NUMDRIVES];

static int ctrl_side;
static int ctrl_drive;
static int ctrl_ddense;
static int ctrl_rate;
static int ctrl_motoron;
static uint64_t ctrl_motoroff_time, ctrl_motoron_time;
static int ctrl_rdata;
static int ctrl_rtrack;
static int ctrl_rsect;
static int ctrl_status;
static int ctrl_stepdir;

static int fdc_error;

static char buf[FDC_BUFSIZE];

static int sidex;
static int cmdx;
static int lastcmd;

//static int fdc_cmd_delay;
static uint64_t cycles_last;

static int bytes_left;
static int buf_index;
static int buf_len;
static uint64_t starting_cycles, every_cycles, window_start, window_end;
static int sector_header_pos;
static int sector_count;

extern char *model_args[];

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;

//==============================================================================
// Initialise
//
//   pass: void
// return: int                          0 if no errors, else -1
//==============================================================================
int fdc_init (void)
{
 int i;
 int res;

 if (modelx.fdc == 0)
    return 0;

 if (modelx.fdc == MODFDC_DD)
     z80api_register_action(Z80_HALT, fdc_nmi);

 disk_init();

 for (i = 0; i < FDC_NUMDRIVES; i++)
    {
     fdc_drive[i].disk.fdisk = NULL;
     fdc_drive[i].disk.itype = 0;
     fdc_drive[i].disk.drive = i;
    }

 res = fdc_bootimage();

 if (strlen(fdc_drive[1].disk.filename) && (res == 0))
    res = (fdc_loaddisk(1, 1));

 if (strlen(fdc_drive[2].disk.filename) && (res == 0))
    res = (fdc_loaddisk(2, 1));

 if (strlen(fdc_drive[3].disk.filename) && (res == 0))
    res = (fdc_loaddisk(3, 1));

 lastcmd = cmdx = -1;

 if (res < 0)
    return -1;
 return 0;
}

//==============================================================================
// De-initialise
//
//   pass: void
// return: int                         0
//==============================================================================
int fdc_deinit (void)
{
 int i;

 if (modelx.fdc == 0)
    return 0;

 for (i = 0; i < FDC_NUMDRIVES; i++)
    fdc_unloaddisk(i);

 if (modelx.fdc == MODFDC_DD)
     z80api_deregister_action(Z80_HALT, fdc_nmi);

 return 0;
}

//==============================================================================
// cause an NMI when data is available/required, or when an interrupt is
// required.  The dreamdisk floppy controller wires (DRQ+INTRQ) from the
// 2793 to NMI, but NMI is only asserted when the CPU is halted.
//
//   pass: void
// return: void
//==============================================================================
void fdc_nmi (void)
{
 /* Update FDC flags and cause an interrupt if */
 fdc_data_r_ready();                /* a read is pending */
 fdc_data_w_ready();                /* or a write is pending */
 if (ctrl_status & (FDC_INTRQ | FDC_DRQ))
    z80api_nonmaskable_intr();
}

//==============================================================================
// reset the controller
//
//   pass: void
// return: int                         0
//==============================================================================
int fdc_reset (void)
{
 int i;

 if (modelx.fdc == 0)
    return 0;

 if (modelx.fdc == MODFDC_DD)
    {
     // motor on time is set to 5S; this is 5 * emu.cpuclock tstates :)
     ctrl_motoron_time = (uint64_t)emu.cpuclock * 5ULL;
     if (modio.fdc)
        xprintf("fdc_init: cpuclock %ul, motorontime %llu\n",
        emu.cpuclock, ctrl_motoron_time);
    }

 ctrl_side = ctrl_drive = 0;
 ctrl_ddense = FDC_DENSITY_SINGLE;
 ctrl_rate = FDC_RATE_250KBPS;
 fdc_update_data_interval();
 ctrl_motoron = 0;                /* FIXME: for now. */
 ctrl_rtrack = 0;
 ctrl_stepdir = +1;

 ctrl_status = FDC_TRACK0;        /* also clears intrq and drq bits<< */

 for (i = 0; i < FDC_NUMDRIVES; i++)
     fdc_drive[i].track = 0;

// if (disk_iswrprot(&fdc_drive[ctrl_drive].disk))
//    ctrl_status |= FDC_WRPROT;

 bytes_left = 0;
 lastcmd = cmdx = -1;

 return 0;
}

//==============================================================================
// Update data interval.
//
// This function must be called after the controller's density or data
// rate are changed.  It recomputes the rate at which bytes read from
// the emulated disk are made available, and the rate at which bytes
// to be written are expected to be presented.
//
//   pass: void
// return: void
//==============================================================================
void fdc_update_data_interval (void)
{
 float cpuclock = modelx.cpuclock > 3.375 ? 3.375 : modelx.cpuclock;

 /* every_cycles is a function of the data rate.  With a
  * 500kHz clock, a new data byte is ready every 16us (32us
  * for a 250kHz clock).
  *
  * For emulated CPU speeds greater than 3.375MHz, we switch
  * behaviour, presenting data bytes every 108 t-states for double
  * density, and every 54 t-states for high density, regardless of
  * the emulated CPU speed.
 */

 switch (ctrl_rate)
    {
     case FDC_RATE_250KBPS:
        every_cycles = (uint64_t)(32.0 * cpuclock);
        break;
     case FDC_RATE_500KBPS:
        every_cycles = (uint64_t)(16.0 * cpuclock);
        break;
     default:
        assert(0);
        break;
   }
}

//==============================================================================
// Set drive.
//
// This function is called to set drive 0-3 on startup and when the emulator
// is running.
//
// It is important that the current track number associated with the drive
// is preserved in this function when changing images as this would happen
// on real hardware.
//
// It is probably not necessary to call this function when using LibDsk and
// physical floppy disks as sectors are not being buffered, LibDsk images
// WILL require calling this function first.
//
// CP/M systems prior to v3.0 (CP/M Plus) require a disk reset operation
// using ^C in the CCP or some call from a CP/M application after changing
// the media in a drive.
//
// The potential to corrupt media data on a real Microbee caused by changing
// media at the incorrect time, wrong format, or forgetting to reset the
// system (^C) holds 100% true for emulation as well.
//
//   pass: int drive                  drive number 0-3
//         fdc_drive_t *fdc_d         pointer to drive data parameters
// return: int                        0 if no errors,  else -1
//==============================================================================
int fdc_set_drive (int drive, fdc_drive_t *fdc_d)
{
 if ((drive < 0) || (drive > 3))
    return -1;

 // we must keep the current track setting for the drive when changing disks
 fdc_d->track = fdc_drive[drive].track;

 // unload (close) an existing open image/drive
 fdc_unloaddisk(drive);

 // set the drive number
 fdc_d->disk.drive = drive;

 memcpy(&fdc_drive[drive], fdc_d, sizeof(fdc_drive_t));

 if (emu.runmode)
    return fdc_loaddisk(drive, 1);
 else
    return 0;
}

//==============================================================================
// load disk
//
//   pass: int drive
//         int report                 1 if require error message if file not
//                                    opened
// return: int                        0 if no errors, else -1
//==============================================================================
static int fdc_loaddisk (int drive, int report)
{
 fdc_unloaddisk(drive);

 if (disk_open(&fdc_drive[drive].disk) != 0)
    {
     fdc_unloaddisk(drive);
     if (report)
        {
         switch (fdc_drive[drive].disk.error)
            {
             case DISK_ERR_NOTFOUND:
                xprintf("fdc_loaddisk: File not found: %s\n",
                fdc_drive[drive].disk.filepath);
                break;
             case DISK_ERR_READONLY:
                xprintf("fdc_loaddisk: File is read only access: %s\n",
                fdc_drive[drive].disk.filepath);
                break;
             default:
                xprintf("fdc_loaddisk: Unknown disk error: %s\n",
                fdc_drive[drive].disk.filepath);
                break;
            }
        }
     return -1;
    }
 return 0;
}

//==============================================================================
// un-load disk
//
//   pass: int drive                    drive number
// return: void
//==============================================================================
void fdc_unloaddisk (int drive)
{
 if (fdc_drive[drive].disk.itype)
    {
     disk_close(&fdc_drive[drive].disk);
     fdc_drive[drive].disk.fdisk = NULL;
     fdc_drive[drive].disk.itype = 0;
    }
}

//==============================================================================
// Load boot image.
//
// Load the default boot disk for the model being emulated.  if the default
// boot image is not found then the fall-back image will be booted.
//
//   pass: void
// return: int                          0 if no errors,  else -1
//==============================================================================
static int fdc_bootimage (void)
{
 int res;

 // if an image for drive A: was specified then load that one
 if (strlen(fdc_drive[0].disk.filename))
    res = (fdc_loaddisk(0, 1));
 else
    // load the default image for the model being emulated
    {
     snprintf(fdc_drive[0].disk.filename, SSIZE1, "%s.dsk",
     model_args[emu.model]);
     res = (fdc_loaddisk(0, 0));

     // if the default image was not found then use the fallback boot image
     if (res < 0)
        {
         // don't load the fallback disk if any hard drives are being used
         if (modelx.ide || modelx.hdd)
            return 0;

         if (modelx.ram < 128)
            xprintf("Image not able to be opened, aborting...\n");
         else
            {
             strcpy(fdc_drive[0].disk.filename, BOOT_IMAGE);
             if (emu.verbose)
                xprintf("Image not able to be opened, "
                "trying fall-back boot image\n");
             res = (fdc_loaddisk(0, 1));
            }
        }
    }

 return res;
}

//==============================================================================
// FDC command
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void fdc_cmd_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 int cmd;
 int prevcmd = lastcmd;                /* previous value of the last command,
                                        * for forced interrupts */

 read_addr_t *idfield = (read_addr_t *)buf;

 if (modelx.fdc == 0)
    return;

 if (modio.fdc)
    log_data_1("fdc_cmd_w", "data", data);

 if (modelx.fdc == MODFDC_DD)
    {
     // any FDC access switches on the floppy motor!
     ctrl_motoroff_time = z80api_get_tstates() + ctrl_motoron_time;
     ctrl_motoron = 1;
    }

 ctrl_status &= ~(FDC_INTRQ | FDC_DRQ);  /* new command clears
                                         * interrupt and DRQ.  FIXME:
                                         * Other bits should be
                                         * cleared too - see data
                                         * sheet. */

 cycles_last = z80api_get_tstates();

 cmd = (data >> 4) & 0x0F;  // get command bits
 if ( ((cmd >> 1) >= 1) && ((cmd >> 1) <= 5) )
    cmd &= 0x0E;  // get rid of data bits in the command field
 lastcmd = cmd;

 // the head positioning commands must be allowed even if no disk/image
 if (fdc_drive[ctrl_drive].disk.itype == 0)
    {
     switch (cmd)
        {
         case FDC_RESTORE:
         case FDC_SEEK:
         case FDC_STEP:
         case FDC_STEPIN:
         case FDC_STEPOUT:
            break;
         default:
            ctrl_status = FDC_RECNOTFOUND | FDC_INTRQ; /* clears drq */
            return;
        }
    }

 switch (cmd)
    {
//------------------------------------------------------------------------------
// Restore - Seek track 0.
//------------------------------------------------------------------------------
     case FDC_RESTORE:
        if (modio.fdc)
           log_mesg("fdc_cmd_w: restore");

        fdc_drive[ctrl_drive].track = ctrl_rtrack = 0;

        ctrl_status = FDC_INTRQ;
        if (data & (FDC_LOADHEAD | FDC_VERIFY))
           ctrl_status |= FDC_HEADLOADED;
        if ((data & FDC_VERIFY) && ctrl_rtrack != fdc_drive[ctrl_drive].track)
            ctrl_status |= FDC_SEEKERROR;
        break;

//------------------------------------------------------------------------------
// Seek.
//------------------------------------------------------------------------------
    case FDC_SEEK:
        if (modio.fdc)
            xprintf("fdc_cmd_w: seek to track %d from track %d%s\n",
                    ctrl_rdata, ctrl_rtrack,
                    (data & FDC_VERIFY) ? " with verify" : "");

        // the actual track sought to might not match the FDC's idea of
        // where the head is.
        fdc_drive[ctrl_drive].track += ctrl_rdata - ctrl_rtrack;
        if (fdc_drive[ctrl_drive].track < 0)
           fdc_drive[ctrl_drive].track = 0;
        else
           if (fdc_drive[ctrl_drive].track > FDC_MAXTRACK)
              fdc_drive[ctrl_drive].track = FDC_MAXTRACK;

        if (modio.fdc)
           xprintf("fdc_cmd_w: drive %d is at track %d\n",
           ctrl_drive, fdc_drive[ctrl_drive].track);

        if (ctrl_rdata < ctrl_rtrack)
           ctrl_stepdir = -1;
        else
           ctrl_stepdir = +1;
        ctrl_rtrack = ctrl_rdata;

        ctrl_status = FDC_INTRQ;
        if (data & (FDC_LOADHEAD | FDC_VERIFY))
           ctrl_status |= FDC_HEADLOADED;
        if ((data & FDC_VERIFY) && ctrl_rtrack != fdc_drive[ctrl_drive].track)
            ctrl_status |= FDC_SEEKERROR;
        break;

//------------------------------------------------------------------------------
// Step - Steps the same direction as the last step-x command.
//------------------------------------------------------------------------------
     case FDC_STEP:
        if (modio.fdc)
           log_mesg("fdc_cmd_w: step");

        fdc_drive[ctrl_drive].track += ctrl_stepdir;
        if (fdc_drive[ctrl_drive].track < 0)
           fdc_drive[ctrl_drive].track = 0;
        else
           if (fdc_drive[ctrl_drive].track > FDC_MAXTRACK)
              fdc_drive[ctrl_drive].track = FDC_MAXTRACK;

        if (data & FDC_UPDATETRACK)
           ctrl_rtrack += ctrl_stepdir;

        ctrl_status = FDC_INTRQ;
        if (data & (FDC_LOADHEAD | FDC_VERIFY))
           ctrl_status |= FDC_HEADLOADED;
        if ((data & FDC_VERIFY) && ctrl_rtrack != fdc_drive[ctrl_drive].track)
            ctrl_status |= FDC_SEEKERROR;
        break;

//------------------------------------------------------------------------------
// Step-in - Step in towards centre of disk.
//------------------------------------------------------------------------------
     case FDC_STEPIN:
        if (modio.fdc)
           log_mesg("fdc_cmd_w: stepin");

        fdc_drive[ctrl_drive].track++;
        if (fdc_drive[ctrl_drive].track > FDC_MAXTRACK)
           fdc_drive[ctrl_drive].track = FDC_MAXTRACK;

        if (data & FDC_UPDATETRACK)
           ctrl_rtrack++;

        ctrl_stepdir = +1;

        ctrl_status = FDC_INTRQ;
        if (data & (FDC_LOADHEAD | FDC_VERIFY))
           ctrl_status |= FDC_HEADLOADED;
        if ((data & FDC_VERIFY) && ctrl_rtrack != fdc_drive[ctrl_drive].track)
            ctrl_status |= FDC_SEEKERROR;
        break;

//------------------------------------------------------------------------------
// Step-out - Step out towards track 0.
//------------------------------------------------------------------------------
     case FDC_STEPOUT:
        if (modio.fdc)
           log_mesg("fdc_cmd_w: stepout");

        fdc_drive[ctrl_drive].track--;
        if (fdc_drive[ctrl_drive].track < 0)
           fdc_drive[ctrl_drive].track = 0;

        if (data & FDC_UPDATETRACK)
           ctrl_rtrack--;

        ctrl_stepdir = -1;

        ctrl_status = FDC_INTRQ;
        if (data & (FDC_LOADHEAD | FDC_VERIFY))
           ctrl_status |= FDC_HEADLOADED;
        if ((data & FDC_VERIFY) && ctrl_rtrack != fdc_drive[ctrl_drive].track)
            ctrl_status |= FDC_SEEKERROR;
        break;

//------------------------------------------------------------------------------
// Read sector - Read single and multi sectors.
//------------------------------------------------------------------------------
     case FDC_READSECT:
        if (modio.fdc)
            xprintf("fdc_cmd_w: readsect drive %d track %d side %d(%d) "
            "sector %d%s\n",
            ctrl_drive, fdc_drive[ctrl_drive].track,
            ctrl_side,
            (data & FDC_SIDE) ? 1 : 0,
            ctrl_rsect,
            (data & FDC_MULTISECT) ? " (multisector)" : "");

        /* The data rate and density selected for the controller must
         * match the drive and the disk in it. If they do not match, a
         * disk error needs to be scheduled...  */
        if ((ctrl_rate == FDC_RATE_250KBPS &&
        fdc_drive[ctrl_drive].disk.datarate != DISK_RATE_250KBPS) ||
        (ctrl_rate == FDC_RATE_500KBPS &&
        fdc_drive[ctrl_drive].disk.datarate != DISK_RATE_500KBPS) ||
        (ctrl_ddense == FDC_DENSITY_SINGLE &&
        fdc_drive[ctrl_drive].disk.density != DISK_DENSITY_SINGLE) ||
        (ctrl_ddense == FDC_DENSITY_DOUBLE &&
        fdc_drive[ctrl_drive].disk.density != DISK_DENSITY_DOUBLE))
           {
            ctrl_status = FDC_RECNOTFOUND | FDC_INTRQ;
            cmd = -1;
            break;
           }

        fdc_error = 0;
        // When executing the read sector command, a real 2793
        // verifies the track, sector and side numbers, as well as the
        // CRC for the ID field.  The track number can be trivially
        // verified here.  The CRC, sector and side numbers are
        // implicitly verified in disk_read().
        if (ctrl_rtrack != fdc_drive[ctrl_drive].track)
           {
            ctrl_status = FDC_RECNOTFOUND | FDC_INTRQ;
            cmd = -1;
            break;
           }

        // probe for the sector size as it might have changed
        if (disk_read_idfield(&fdc_drive[ctrl_drive].disk, idfield, ctrl_side,
           fdc_drive[ctrl_drive].track) != 0)
           {
            ctrl_status = FDC_RECNOTFOUND | FDC_INTRQ;
            cmd = -1;
            break;
           }

        // verify the side number in the idfield against the side
        // number in the command.  Note - this isn't the physical side
        // number in ctrl_side!
        if ((data & FDC_CMPSIDE) != 0 &&
            (((data & FDC_SIDE) != 0 && (idfield->side & 1) == 0) ||
             ((data & FDC_SIDE) == 0 && (idfield->side & 1) != 0)))
           {
            ctrl_status = FDC_RECNOTFOUND | FDC_INTRQ;
            cmd = -1;
            break;
           }

        sidex = idfield->side;
        if (modio.fdc)
           xprintf("fdc_cmd_w: readsect idfield side %d\n", sidex);
        fdc_drive[ctrl_drive].disk.secsize = 128 << idfield->seclen;
#ifdef USE_LIBDSK
        fdc_drive[ctrl_drive].disk.dg.dg_secsize = fdc_drive[ctrl_drive].disk.secsize;
#endif
        // read 1 sector
        buf_index = 0;
        ctrl_status &= ~(FDC_NOTREADY | FDC_RECNOTFOUND | FDC_CRCERROR);
        fdc_error = disk_read(&fdc_drive[ctrl_drive].disk, buf,
        ctrl_side, sidex, fdc_drive[ctrl_drive].track, ctrl_rsect, 0);
        // check errors and set status register
        if (fdc_error)
           {
#ifdef USE_LIBDSK
            if (fdc_drive[ctrl_drive].disk.itype == DISK_LIBDSK)
               {
                switch (fdc_error)
                   {
                    case DSK_ERR_NOTRDY:
                       ctrl_status |= FDC_NOTREADY;
                       break;
                    case DSK_ERR_NODATA:
                       ctrl_status |= FDC_RECNOTFOUND;
                       break;
                    case DSK_ERR_DATAERR:
                       ctrl_status |= FDC_CRCERROR;
                       break;
                    case DSK_ERR_NOADDR:
                    case DSK_ERR_SYSERR:
                    default:
                       ctrl_status |= FDC_RECNOTFOUND;
                       break;
                   }
               }
            else
#endif
               ctrl_status |= FDC_RECNOTFOUND;

            ctrl_status |= FDC_INTRQ;
            ctrl_status &= ~(FDC_BUSY | FDC_DRQ);
            cmd = -1; /* no command executing */
            break;
           }

        if (data & FDC_MULTISECT)
           ctrl_status |= FDC_CMULTISECT;
        ctrl_status |= FDC_BUSY;
        fdc_schedule_data(fdc_drive[ctrl_drive].disk.secsize, buf,
                          z80api_get_tstates() + every_cycles * 20);
        /* assume data ready in 20 byte times. */
        break;
//------------------------------------------------------------------------------
// Write sector - Write single and multi sectors.
//------------------------------------------------------------------------------
     case FDC_WRITESECT:
        if (modio.fdc)
           xprintf("fdc_cmd_w: writesect drive %d track %d side %d(%d) "
           "sector %d%s\n", ctrl_drive, fdc_drive[ctrl_drive].track,
           ctrl_side, (data & FDC_SIDE) ? 1 : 0, ctrl_rsect,
           (data & FDC_MULTISECT) ? " (multisector)" : "");

        // The data rate and density selected for the controller must
        // match the drive and the disk in it. If they do not match, a
        // disk error needs to be scheduled...
        if ((ctrl_rate == FDC_RATE_250KBPS &&
        fdc_drive[ctrl_drive].disk.datarate != DISK_RATE_250KBPS) ||
        (ctrl_rate == FDC_RATE_500KBPS &&
        fdc_drive[ctrl_drive].disk.datarate != DISK_RATE_500KBPS) ||
        (ctrl_ddense == FDC_DENSITY_SINGLE &&
        fdc_drive[ctrl_drive].disk.density != DISK_DENSITY_SINGLE) ||
        (ctrl_ddense == FDC_DENSITY_DOUBLE &&
        fdc_drive[ctrl_drive].disk.density != DISK_DENSITY_DOUBLE))
           {
            ctrl_status = FDC_RECNOTFOUND | FDC_INTRQ; // set record not
                                                       // found bit now.
            cmd = -1;
            break;
           }

        // When executing the write sector command, a real 2793 verifies the
        // track, sector and side numbers, as well as the CRC for the ID
        // field.  The track number can be trivially verified here.  The
        // CRC, sector and side numbers are implicitly verified in
        // disk_write().  This is different from a real 2793 - a write to a
        // non-existent sector will fail AFTER the data for that sector is
        // written, rather than before data is transferred.  This may not be
        // a problem.

        if (ctrl_rtrack != fdc_drive[ctrl_drive].track)
           {
            ctrl_status = FDC_RECNOTFOUND | FDC_INTRQ;
            cmd = -1;
            break;
           }

        // probe for the sector size as it might have changed.
        if (disk_read_idfield(&fdc_drive[ctrl_drive].disk, idfield,
        ctrl_side, fdc_drive[ctrl_drive].track) != 0)
           {
            // failure reading the next ID field, abort the command.
            ctrl_status = FDC_RECNOTFOUND | FDC_INTRQ;
            cmd = -1;
            break;
           }

        // verify the side number in the idfield against the side number in
        // the command.  Note - this isn't the physical side number in
        // ctrl_side!
        if ((data & FDC_CMPSIDE) != 0 &&
            (((data & FDC_SIDE) != 0 && (idfield->side & 1) == 0) ||
             ((data & FDC_SIDE) == 0 && (idfield->side & 1) != 0)))
           {
            ctrl_status = FDC_RECNOTFOUND | FDC_INTRQ;
            cmd = -1;
            break;
           }

        // Check that the disk isn't write protected.
        if (disk_iswrprot(&fdc_drive[ctrl_drive].disk))
           {
            ctrl_status = FDC_INTRQ;
            cmd = -1;
            break;
           }

        sidex = idfield->side;
        fdc_drive[ctrl_drive].disk.secsize = 128 << idfield->seclen;
#ifdef USE_LIBDSK
        fdc_drive[ctrl_drive].disk.dg.dg_secsize =
        fdc_drive[ctrl_drive].disk.secsize;
#endif
        ctrl_status = FDC_BUSY;
        if (data & FDC_MULTISECT)
           ctrl_status |= FDC_CMULTISECT; /* flag that this is a
                                           * multisector operation */
        /* The first data byte must be supplied after 22 bytes
         * (double density) according to the 2793 data sheet */
        fdc_schedule_data(fdc_drive[ctrl_drive].disk.secsize, buf,
                          z80api_get_tstates() + every_cycles * 22);
        buf_index = -1;                /* special state */
        break;

//------------------------------------------------------------------------------
// Read address - Read next ID field.
//------------------------------------------------------------------------------
     case FDC_READADDR:
        if (modio.fdc)
           log_mesg("fdc_cmd_w: readaddr");

        // The data rate and density selected for the controller must
        // match the drive and the disk in it. If they do not match, a
        // disk error needs to be scheduled...
        if ((ctrl_rate == FDC_RATE_250KBPS &&
        fdc_drive[ctrl_drive].disk.datarate != DISK_RATE_250KBPS) ||
        (ctrl_rate == FDC_RATE_500KBPS &&
        fdc_drive[ctrl_drive].disk.datarate != DISK_RATE_500KBPS) ||
        (ctrl_ddense == FDC_DENSITY_SINGLE &&
        fdc_drive[ctrl_drive].disk.density != DISK_DENSITY_SINGLE) ||
        (ctrl_ddense == FDC_DENSITY_DOUBLE &&
        fdc_drive[ctrl_drive].disk.density != DISK_DENSITY_DOUBLE))
           {
            ctrl_status = FDC_RECNOTFOUND | FDC_INTRQ; // set record not
                                                       // found bit now.
            cmd = -1;
            break;
           }

        fdc_error = disk_read_idfield(&fdc_drive[ctrl_drive].disk,
        idfield, ctrl_side, fdc_drive[ctrl_drive].track);
        ctrl_rsect = idfield->track;

        // check errors and set status register
        ctrl_status = 0;

        if (fdc_error)
           {
#ifdef USE_LIBDSK
            if (fdc_drive[ctrl_drive].disk.itype == DISK_LIBDSK)
               {
                switch (fdc_error)
                   {
                    case DSK_ERR_NOTRDY:
                       ctrl_status = FDC_NOTREADY;
                       break;
                    case DSK_ERR_NODATA:
                       ctrl_status = FDC_RECNOTFOUND;
                       break;
                    case DSK_ERR_DATAERR:
                       ctrl_status = FDC_CRCERROR;
                       break;
                    case DSK_ERR_NOADDR:
                       ctrl_status = FDC_RECNOTFOUND;
                       break;
                    case DSK_ERR_SYSERR:
                       ctrl_status = FDC_RECNOTFOUND;
                       break;
                    default:
                       ctrl_status = FDC_RECNOTFOUND;
                       break;
                   }
               }
            else
#endif
               ctrl_status = FDC_RECNOTFOUND;
            // busy is reset by this point
            ctrl_status |= FDC_INTRQ;
            cmd = -1;
            break;
           }
        ctrl_status |= FDC_BUSY;
        fdc_schedule_data(6, buf, z80api_get_tstates() + every_cycles * 20);
        /* assume data ready in 20 byte times. */
        break;

//------------------------------------------------------------------------------
// Read track.
//------------------------------------------------------------------------------
     case FDC_READTRACK:
        if (modio.fdc)
           log_mesg("fdc_cmd_w: readtrack (not supported)");
        // we don't do that.
        ctrl_status = FDC_NOTREADY | FDC_INTRQ;
        cmd = -1;
        break;

//------------------------------------------------------------------------------
// Write track - format a disk track.
//
// based on 250KB/S and 5 disk rotations per second the number of bytes that
// can be placed on a track is:
//
// Double Density (DD) bytes is 6250 = 250000 / 5 / 8
// Single Density (SD) bytes is 3125 = 250000 / 5 / 8 / 2
//
// Data rates of 500kbps are typically used with 8" drives, which spin at
// 360rpm, or 6 rps.  In that case, the number of bytes that will fit
// on a track are:
//
// Double Density (DD) bytes is 10416 = 500000 / 6 / 8
// Single Density (SD) bytes is 5208  = 500000 / 6 / 8 / 2
//
// FIXME: strictly speaking, of course, the properties of the drive
// are completely independent of the data rate!
//------------------------------------------------------------------------------
     case FDC_WRITETRACK:
        if (modio.fdc)
           log_mesg("fdc_cmd_w: writetrack");

        // write command is immediately terminated should the disk be
        // write protected.
        if (disk_iswrprot(&fdc_drive[ctrl_drive].disk))
           {
            ctrl_status = FDC_INTRQ;
            cmd = -1;                /* no command executing */
            break;
           }

        int bytes_expected;

        switch (ctrl_rate)
           {
            case FDC_RATE_250KBPS:
               bytes_expected = ctrl_ddense ? 6250 : 3125;
               break;
            case FDC_RATE_500KBPS:
               bytes_expected = ctrl_ddense ? 10416 : 5208; //FIXME: CHECK THESE
               break;
            default:
               assert(0);                /* should never happen */
               break;
           }

        // The 2793 data sheet says that DRQ is asserted
        // immediately but the writing of data doesn't begin until
        // an index pulse is encountered.
        ctrl_status |= FDC_BUSY | FDC_DRQ;
        fdc_schedule_data(bytes_expected, buf, z80api_get_tstates() +
        every_cycles * 100);
        // assume data ready in 100 byte times.
        break;

//------------------------------------------------------------------------------
// Force interrupt.
//
// Used to terminate a multi sector read/write command and to ensure Type I
// status in the status register.
//------------------------------------------------------------------------------
     case FDC_INTERRUPT:
        if (modio.fdc)
           log_mesg("fdc_cmd_w: interrupt");

        // Allow a multisector read or write to terminate before
        // interrupting it.

        // This odd bit of code is here to work around a timing
        // problem in the DreamDisk FASTCOPY program, where, on a
        // 3.375MHz Microbee, it doesn't wait quite long enough before
        // interrupting a multisector write command when writing a
        // track to the destination disk.  On real hardware this
        // probably works because the force interrupt command will
        // wait until the currently executing 2793 uop finishes.
        //
        // Under emulation, we bring the data window forward a bit
        // before calling fdc_data_w_ready(), in the hopes that this
        // will allow the write to complete before being interrupted.
        if ((cmdx == FDC_WRITESECT ||
           cmdx == FDC_WRITETRACK) && window_start - cycles_last < 10)
           {
            window_start = cycles_last - 1;
            fdc_data_w_ready();
           }

        ctrl_status = 0;
        bytes_left = 0;
        // Forced interrupt with no condition bits terminates the
        // current command and clears the interrupt bit.
        if ((data & 0xf) != 0)
           ctrl_status |= FDC_INTRQ;
        else
           {
            if (modio.fdc)
               log_mesg("fdc_cmd_w: command terminated without interrupt");
            lastcmd = prevcmd;  // restore command so status
                                // is reported properly.
           }
        cmd = -1;               // no command executing
        break;
    }

   cmdx = cmd;
   fdc_update_drv_status();
}

//==============================================================================
// Internal function to update the drive-related FDC type 1 status
// bits on completion of an FDC command OR if a new drive is selected.
//
//   pass: nothing
// return: nothing
// effect: updates bits in ctrl_status
//==============================================================================
static void fdc_update_drv_status (void)
{
 ctrl_status &= ~FDC_TRACK0;
 if (fdc_drive[ctrl_drive].track == 0)
    ctrl_status |= FDC_TRACK0;
 ctrl_status &= ~FDC_WRPROT;
 if (disk_iswrprot(&fdc_drive[ctrl_drive].disk))
    ctrl_status |= FDC_WRPROT;

 // Index pulse bits are updated in fdc_status_r() (most effective to do it
 // there)
}

//==============================================================================
// Read FDC status.
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t                     ctrl_status
//==============================================================================
uint16_t fdc_status_r (uint16_t port, struct z80_port_read *port_s)
{
 uint64_t cycles_now;
 int status = ctrl_status;

 if (modelx.fdc == 0)
    return 0;

 if (modelx.fdc == MODFDC_DD)
    {
     // any FDC access switches on the floppy motor!
     ctrl_motoroff_time = z80api_get_tstates() + ctrl_motoron_time;
     ctrl_motoron = 1;
    }

 if (!fdc.nodisk)
    {
     // update status here if a command is executing.
     fdc_data_r_ready();
     fdc_data_w_ready();
    }

 ctrl_status &= ~FDC_INTRQ;     // read status clears interrupt

 cycles_now = z80api_get_tstates();

 // create the index pulse at the correct rate.  A 6 rev/second disk
 // requires 166mS per revolution (540000 Z80 CPU cycles).  The index pulse
 // is 3.33mS wide and is 11239 Z80 CPU cycles.

 if (! fdc.nodisk)
    {
     ctrl_status &= ~FDC_INDEXPULSE;
     if ((cycles_now % 540000) < 11239)
        ctrl_status |= FDC_INDEXPULSE;
    }

 switch (lastcmd)
    {
     // type II/III read commands
     case FDC_READSECT:
     case FDC_READADDR:
     case FDC_READTRACK:
        // write protect bit must be zero on reads
        status = FDC_TYPEII_MASK & (ctrl_status >> 8);
        status |= ctrl_status & ~(FDC_TYPEII_MASK | FDC_WRPROT);
        break;
     case FDC_WRITESECT:
     case FDC_WRITETRACK:
        status = FDC_TYPEII_MASK & (ctrl_status >> 8);
        status |= ctrl_status & ~FDC_TYPEII_MASK;
        break;
     // type I commands
     case FDC_RESTORE:
     case FDC_SEEK:
     case FDC_STEP:
     case FDC_STEPIN:
     case FDC_STEPOUT:
     case FDC_INTERRUPT:
     default:
        status = ctrl_status;
        break;
    }

 status &= 0xff;

 if (modio.fdc)
    log_port_1("fdc_status_r", "ctrl_status", port, status);

 return status;
}

//==============================================================================
// Schedule a block of data for reading or writing.
//
// When reading, buf is expected to point to a buffer of buflen bytes
// of data; these are made available for reading at start_cycles cpu
// cycles, with subsequent bytes available after every delta cycles.
// Data bytes need to be read promptly.
//
// When writing, buf is expected to point to an (empty) buffer of
// buflen bytes, which will start being filled at start_cycles cpu
// cycles, with subsequent bytes being provided every delta cycles.
// Data bytes need to be written in time.
//
//   pass: int buflen
//           uint8_t *buf
//           uint64_t start_cycles
// return: void
//==============================================================================
static void fdc_schedule_data (int buflen, char *buf, uint64_t start_cycles)
{
 if (modio.fdc)
    xprintf("fdc_schedule_data: %d bytes starting at %llu every %llu cycles\n",
 buflen, start_cycles, every_cycles);

 buf_index = 0;              /* index of next data byte to return */
 bytes_left = buflen;        /* number of data bytes to return */
 buf_len = buflen;           /* total number of data bytes */
 window_start = starting_cycles = start_cycles;
 window_end = start_cycles + every_cycles;
}

//==============================================================================
// Test whether the next data byte is ready to be read.
//
// Returns the value of the FDC DRQ bit, updates the FDC status register,
// updates the FDC data register.
//
//   pass: void
// return: int
//==============================================================================
static int fdc_data_r_ready (void)
{
 uint64_t cycles_now;

 if (cmdx == -1)
    return 0;                /* no command executing - no data! */

 if (!(cmdx == FDC_READTRACK || cmdx == FDC_READADDR || cmdx == FDC_READSECT))
    return 0;                /* not a read data command */

 cycles_now = z80api_get_tstates();
 if (cycles_now < window_start)
    {
     /* do nothing, no data is ready yet! */
    }
 else
    if (bytes_left != 0 && cycles_now < window_end)
       {
        if (ctrl_status & FDC_DRQ)
           {
            if (modio.fdc)
               xprintf("fdc_cmd_w: lost data\n");
            ctrl_status |= FDC_LOSTDATA; /* oops, previous byte hasn't
                                          * been serviced in time! */
           }
        ctrl_status |= FDC_DRQ; /* assert drq */
        window_start += every_cycles;
                                       /* update time at which the
                                        * next data byte is
                                        * presented */
        window_end += every_cycles;    /* and the end of the
                                        * presentation window too */
        ctrl_rdata = buf[buf_index++]; /* load the next data byte into
                                        * the data register */
        bytes_left--;
       }
    else
       if (bytes_left == 0 && (ctrl_status & FDC_CMULTISECT))
          {
           /* The last byte of the sector has been returned.  Try to
           * read the next sector on the track */
           buf_index = 0;
           ctrl_rsect++;
           ctrl_status &= ~FDC_DRQ; /* clear DRQ */
           fdc_error = disk_read(&fdc_drive[ctrl_drive].disk,
                                 &buf[buf_index],
                                 ctrl_side, sidex,
                                 fdc_drive[ctrl_drive].track,
                                 ctrl_rsect, 'm');
           if (!fdc_error)
              {
               // schedule the next sector read to happen in say 106 byte times
               fdc_schedule_data(fdc_drive[ctrl_drive].disk.secsize, buf,
                                 z80api_get_tstates() + every_cycles * 106);
              }
           else
              {
               /* leave bytes_left at zero, change the window start to be
                * 1s from now rather than one byte time, to model the
                * 279x's search for 5 index pulses (1s for a 300rpm
                * drive) before giving up.  The Applied Technology
                * systems only really require that the FDC stay busy for
                * about 10ms afer the last data byte is returned, 1s is a
                * bit excessive. */
               window_start = window_end =
               z80api_get_tstates() + 1000000UL * (uint64_t)modelx.cpuclock;
               ctrl_status |= FDC_RECNOTFOUND; //set the record not found bit now
               ctrl_status &= ~FDC_CMULTISECT; // clear the multisector bit
              }
          }
       else
          if (bytes_left == 0)
             {
              /* command complete. */
              cmdx = -1;
              ctrl_status &= ~FDC_BUSY;
              /* DRQ already clear, multisector bit clear. */
              ctrl_status |= FDC_INTRQ;
             }
          else
             {
              int nextbyte_index;

              /* data lost, work out which data byte should be presented now. */
              if (modio.fdc)
                 xprintf("fdc_cmd_w: lost data, was %d ", buf_index);
              ctrl_status |= FDC_LOSTDATA;
              nextbyte_index = (cycles_now - starting_cycles + every_cycles - 1)
              / every_cycles;
              if (nextbyte_index >= buf_len)
                 {
                  bytes_left = 0;        /* no more bytes to return */
                  nextbyte_index = buf_len;
                  window_start = window_end = cycles_now - 1; /* hack */
                 }
              else
                 {
                  buf_index = nextbyte_index;
                  window_start = starting_cycles + buf_index * every_cycles;
                  window_end = window_start + every_cycles;
                  bytes_left = buf_len - nextbyte_index;
                 }
              if (modio.fdc)
                 xprintf("is %d\n", nextbyte_index);
              return fdc_data_r_ready(); // recursive call to set status bits
             }

 return (ctrl_status & FDC_DRQ);
}

//==============================================================================
// Write sector.
//
// The sector(s) is written after a complete sector has been filled.
//
//   pass: void
// return: void
//==============================================================================
static void fdc_writesect_cmd (void)
{
 // the right number of bytes is assumed to be in buf
 if (modio.fdc)
    xprintf("fdc_data_w_ready: writing to drive %d track %d "
    "side %d sector %d\n", ctrl_drive, fdc_drive[ctrl_drive].track,
    ctrl_side, ctrl_rsect);

 fdc_error = disk_write(&fdc_drive[ctrl_drive].disk, buf, ctrl_side, sidex,
                  fdc_drive[ctrl_drive].track, ctrl_rsect, 0);
 if (fdc_error)
    {
#ifdef USE_LIBDSK
     if (fdc_drive[ctrl_drive].disk.itype == DISK_LIBDSK)
        {
         switch (fdc_error)
            {
             case DSK_ERR_RDONLY:
                ctrl_status = FDC_WRPROT;
                break;
             case DSK_ERR_NOTRDY:
                ctrl_status = FDC_NOTREADY;
                break;
             case DSK_ERR_NODATA:
                ctrl_status = FDC_RECNOTFOUND;
                break;
             case DSK_ERR_DATAERR:
                ctrl_status = FDC_CRCERROR;
                break;
             case DSK_ERR_NOADDR:
                ctrl_status = FDC_RECNOTFOUND;
                break;
             default:
                ctrl_status = FDC_LOSTDATA;
                break;
            }
        }
     else
 #endif
        ctrl_status = FDC_RECNOTFOUND;

     cmdx = -1;
     ctrl_status &= ~FDC_DRQ;
     ctrl_status |= FDC_INTRQ;
    }
 else
    if (ctrl_status & FDC_CMULTISECT)
       {
        ctrl_rsect++;
        fdc_schedule_data(fdc_drive[ctrl_drive].disk.secsize, buf,
        z80api_get_tstates() + every_cycles * 106); // inter sector gap
        buf_index = -1; // special condition to force the initial DRQ
       }
    else
       {
        ctrl_status = 0; // successful write, no errors to report
        cmdx = -1;
        ctrl_status &= ~FDC_DRQ;
        ctrl_status |= FDC_INTRQ;
       }
}

//==============================================================================
// Write track.
//
// Gathers sector header information,  when the count of bytes written is
// greater than the capacity of one track (SS or DD) a format track command
// function formats the track using ctrl_side, ctrl_rtrack, and ctrl_ddense
// values.
//
// LibDsk: The side used in the sector header comes directly from the track
// data.
//
//   pass: void
// return: void
//==============================================================================
static void fdc_writetrack_cmd (void)
{
 int parser_state;
 int sector_headers_index;
 int data_columns, data, data_last, data_same;

 // show the track write data
 if (modio.fdc_wtd)
    {
     xprintf("\nfdc_data_w: Write track data: Drive=%c: DD=%d "
     "Track=%d Side=%d\n", ctrl_drive + 'A', ctrl_ddense,
     ctrl_rtrack, ctrl_side);
     if (modio.level)
        fprintf(modio.log, "\nfdc_data_w: Write track data: "
        "Drive=%c: DD=%d Track=%d Side=%d\n",
        ctrl_drive + 'A', ctrl_ddense, ctrl_rtrack, ctrl_side);

     data_columns = 0;
     data_same = 0;
     data_last = (unsigned char)buf[0];

     for (buf_index = 0; buf_index < buf_len; buf_index++)
        {
         data = (unsigned char)buf[buf_index];
         if (data_last == data)
            data_same++;
         else
            {
             xprintf("0x%02xx%-5d", data_last, data_same);
             if (modio.level)
                fprintf(modio.log, "0x%02xx%-5d", data_last, data_same);
             if ((++data_columns % 8) == 0)
                {
                 xprintf("\n");
                 if (modio.level)
                     fprintf(modio.log, "\n");
                }
             data_last = data;
             data_same = 1;
            }
        }

     if ((data_columns % 8) != 0)
        {
         xprintf("\n");
         if (modio.level)
             fprintf(modio.log, "\n");
        }
    }

 buf_index = 0;        /* start at the beginning of the buffer */
 data_last = -1;
 parser_state = 0;        /* parsing gap 1 */
 /*
  * The list of sector headers extracted from the track
  * data is built up at the beginning of the data buffer,
  * overwriting the track leadin data.
  */
 sector_headers_index = 0;
 sector_count = 0;

 while (buf_index < buf_len && parser_state != -1)
    {
     data = (unsigned char)buf[buf_index++];
     switch (parser_state)
        {
         case 0:  /* looking for the id field header
                   * byte.  The leadin bytes are
                   * discarded, it would be a bad idea
                   * to use the emulator to develop a
                   * real disk formatter program! */
            switch (data)
               {
                case 0xfe:
                   parser_state = 1; // reading sector header
                   sector_header_pos = 0; // number of sector header bytes read.
                   sector_count++;        // number of sectors seen
                   break;
                default:
                   break;   /* discard everything else */
               }
            break;
         case 1:   /* have seen the id field header start byte. */
            switch (data)
               {
                case 0xf7:
                   // write sector header CRC byte, sector header done
                   sector_headers_index += 4; // sector header has 4 bytes
                                              //  of data
                   sector_header_pos = 0;
                   parser_state = 0; // look for the next sector header
                   break;
                default:
#ifdef USE_LIBDSK
                   if (sector_header_pos == 1)
                      {
                       if (!fdc_drive[ctrl_drive].disk.side1as0)
                          data = ctrl_side;  // force to current physical side
                      }
#endif
                   buf[sector_headers_index + sector_header_pos] = data;
                   break;
               }
            sector_header_pos++;
            break;
         default:
            assert(0);        /* bad state, should never happen */
            break;
        }
    }

 // show the sector header information
 if (modio.fdc_wth)
    {
     int i;
     xprintf("\nfdc_data_w: Write track header: Drive=%c: "
     "DD=%d Track=%d Side=%d\n", ctrl_drive + 'A', ctrl_ddense,
     ctrl_rtrack, ctrl_side); 
     xprintf("Track  Head  Sect  Size\n");
     for (i = 0; i < sector_count; i++)
        xprintf("%5d%6d%6d%6d\n",
        buf[i*4+0], buf[i*4+1], buf[i*4+2], buf[i*4+3] << 8);

     if (modio.level)
        {
         fprintf(modio.log, "\nfdc_data_w: Write track header: Drive=%c: "
         "DD=%d Track=%d Side=%d\n", ctrl_drive + 'A', ctrl_ddense, 
         ctrl_rtrack, ctrl_side);
         fprintf(modio.log, "Track  Head  Sect  Size\n");
         for (i = 0; i < sector_count; i++)
            fprintf(modio.log, "%5d%6d%6d%6d\n",
            buf[i*4+0], buf[i*4+1], buf[i*4+2], buf[i*4+3] << 8);
        }
    }

 fdc_error = disk_format_track(&fdc_drive[ctrl_drive].disk, buf, ctrl_ddense,
             ctrl_rtrack, ctrl_side, sector_count);
 if (fdc_error)
    {
#ifdef USE_LIBDSK
     if (fdc_drive[ctrl_drive].disk.itype == DISK_LIBDSK)
        {
         switch (fdc_error)
            {
             case DSK_ERR_RDONLY:
                ctrl_status = FDC_WRPROT;
                break;
             case DSK_ERR_NOTRDY:
                ctrl_status = FDC_NOTREADY;
                break;
             default:
                ctrl_status = FDC_LOSTDATA;
                break;
            }
        }
     else
#endif
        ctrl_status = FDC_LOSTDATA;
    }
 else
    ctrl_status = 0;

 cmdx = -1;
 ctrl_status |= FDC_INTRQ;
}

//==============================================================================
// Test whether the next data byte to write needs to be supplied.
//
// Updates the FDC status register and returns the value of the FDC
// DRQ bit.  If new data has been written to the FDC data register in
// time, adds the data byte to the sector/track under construction.
//
// Handles Write sector and Write track FDC commands.
//
// Write sector:
//  The sector(s) is written after a complete sector has been filled.
//
// Write track:
//  Gathers sector header information,  when the count of bytes written is
//  greater than the capacity of one track (SS or DD) a format track command
//  function formats the track using ctrl_side, ctrl_rtrack, and ctrl_ddense
//  values.
//
//   pass: void
// return: int
//==============================================================================
static int fdc_data_w_ready (void)
{
 uint64_t cycles_now;
 unsigned char data;

 if (cmdx == -1)
    return 0;                /* no command executing - no data! */

 if (!(cmdx == FDC_WRITETRACK || cmdx == FDC_WRITESECT))
    return 0;                /* not a write data command */

 cycles_now = z80api_get_tstates();
 if (cycles_now < window_start)
    {
     /* do nothing, the next data byte is not required to be ready yet */
    }
 else
    if (bytes_left != 0 && (cycles_now < window_end ||
    // On the last byte of the sector the check for new data
    // is allowed to be a little late, as the Dreamdisk code
    // doesn't quite get there in time at 2MHz.
    (bytes_left == 1 && cycles_now - window_end < 5)))
       {
        if (buf_index < 0)
           {
            /* This is a special state to get the intial DRQ to happen
             * at the right time before writing a sector or a
             * track. */
            buf_index = 0;
           }
        else
           {
            if (ctrl_status & FDC_DRQ)
               {
                if (modio.fdc)
                   xprintf("fdc_cmd_w: lost data\n");
                ctrl_status |= FDC_LOSTDATA; /* oops, the next data byte
                                              * hasn't been supplied in
                                              * time! */
                data = 0;                    /* substitute a zero byte */
               }
            else
               data = ctrl_rdata;      /* fetch next data byte */

            buf[buf_index++] = data;   /* add the data byte to the
                                        * track/sector buffer. */
            bytes_left--;
           }

        if (bytes_left)
           {
            ctrl_status |= FDC_DRQ; /* assert drq to get next byte */
            window_start += every_cycles;  /* update time by which the
                                            * next data byte is due to
                                            * be presented */
            window_end += every_cycles;    /* and the end of the
                                            * service window too */
           }
        else
           {
            window_start = window_end = cycles_now - 1;  /* hack */
            return fdc_data_w_ready(); /* recursive call to force the
                                        * immediate write of the
                                        * sector */
           }
       }
    else
       if (bytes_left == 0)
          {
           switch (cmdx)
              {
               case FDC_WRITESECT:
                  fdc_writesect_cmd();
                  break;
               case FDC_WRITETRACK:
                  fdc_writetrack_cmd();
                  break;
               default:
                  assert(0);                /* should never happen! */
                  break;
              }
          }
       else
          {
           int bytes_lost;

           /* lots of data lost, work out how many extra zero bytes need
            * to be added to the sector buffer.  This is a little
            * different to the read data lost case. */
           if (modio.fdc)
              xprintf("fdc_cmd_w: lost data, was %d ", buf_index);
           ctrl_status |= FDC_LOSTDATA;
           bytes_lost = (cycles_now - window_start + every_cycles - 1) /
           every_cycles;

           if (buf_index + bytes_lost >= buf_len)
              bytes_lost = buf_len - buf_index; /* fill the entire buffer */

           while (bytes_lost)
              {
               buf[buf_index++] = 0;
               bytes_left--;
               bytes_lost--;
              }

           if (bytes_left == 0)
              window_start = window_end = cycles_now - 1;        /* hack */
           else
              {
               window_start += every_cycles - (cycles_now - window_start) %
               every_cycles;
               window_end = window_start + every_cycles;
              }

           return fdc_data_w_ready(); // recursive call to force the write
                                      // of the track or sector
          }

 return (ctrl_status & FDC_DRQ);
}

//==============================================================================
// Read 1 data byte from sector
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: uint16_t                     ctrl_rdata
//==============================================================================
uint16_t fdc_data_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modelx.fdc == 0)
    return 0;

 if (modelx.fdc == MODFDC_DD)
    {
     // any FDC access switches on the floppy motor!
     ctrl_motoroff_time = z80api_get_tstates() + ctrl_motoron_time;
     ctrl_motoron = 1;
    }

 ctrl_status &= ~FDC_DRQ;

 if (modio.fdc)
    log_port_1("fdc_data_r", "ctrl_rdata", port, ctrl_rdata & 0xFF);

 return ctrl_rdata & 0xFF;
}

//==============================================================================
// Write 1 data byte.  The actual writing of sectors and tracks happens
// in fdc_data_w_ready(), so it's important to periodically poll the
// FDC for its status.
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void fdc_data_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modelx.fdc == 0)
    return;

 if (modelx.fdc == MODFDC_DD)
    {
     // any FDC access switches on the floppy motor!
     ctrl_motoroff_time = z80api_get_tstates() + ctrl_motoron_time;
     ctrl_motoron = 1;
    }

 if (modio.fdc)
    log_port_2("fdc_data_w", "ctrl_rdata", "bytes_left",
    port, data, bytes_left);

 // A write to the data register resets the DRQ bit
 ctrl_rdata = data;
 ctrl_status &= ~FDC_DRQ;
}

//==============================================================================
// Set track number
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void fdc_track_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modelx.fdc == 0)
    return;

 if (modelx.fdc == MODFDC_DD)
    {
     // any FDC access switches on the floppy motor!
     ctrl_motoroff_time = z80api_get_tstates() + ctrl_motoron_time;
     ctrl_motoron = 1;
    }

 if (modio.fdc)
    log_port_1("fdc_track_w", "ctrl_rtrack", port, data);

 ctrl_rtrack = data;
}

//==============================================================================
// Get track number
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: uint16_t                     ctrl_rtrack
//==============================================================================
uint16_t fdc_track_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modelx.fdc == 0)
    return 0;

 if (modelx.fdc == MODFDC_DD)
    {
     // any FDC access switches on the floppy motor!
     ctrl_motoroff_time = z80api_get_tstates() + ctrl_motoron_time;
     ctrl_motoron = 1;
    }

 if (modio.fdc)
    log_port_1("fdc_track_r", "ctrl_rtrack", port, ctrl_rtrack);

 return ctrl_rtrack;
}

//==============================================================================
// Set sector number
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void fdc_sect_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 if (modelx.fdc == 0)
    return;

 if (modelx.fdc == MODFDC_DD)
    {
     // any FDC access switches on the floppy motor!
     ctrl_motoroff_time = z80api_get_tstates() + ctrl_motoron_time;
     ctrl_motoron = 1;
    }

 if (modio.fdc)
    log_port_1("fdc_sect_w", "ctrl_rsect", port, data);

 ctrl_rsect = data;
}

//==============================================================================
// Get sector number
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: uint16_t                     ctrl_rsect
//==============================================================================
uint16_t fdc_sect_r (uint16_t port, struct z80_port_read *port_s)
{
 if (modelx.fdc == 0)
    return 0;

 if (modelx.fdc == MODFDC_DD)
    {
     // any FDC access switches on the floppy motor!
     ctrl_motoroff_time = z80api_get_tstates() + ctrl_motoron_time;
     ctrl_motoron = 1;
    }

 if (modio.fdc)
    log_port_1("fdc_sect_r", "ctrl_rsect", port, ctrl_rsect);

 return ctrl_rsect;
}

//==============================================================================
// Set drive, side and density
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void fdc_ext_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 int i;

 if (modelx.fdc == 0)
    return;

 if (modio.fdc)
    log_port_1("fdc_ext_w", "ctrl_drive/side/ddense", port, data);

 switch (modelx.fdc)
    {
     case MODFDC_AT:
        ctrl_drive = data & FDC_AT_DRIVE_SELECT_MASK;
        ctrl_side = (data & FDC_AT_SIDE_SELECT_MASK) ? 1 : 0;
        ctrl_ddense = (data & FDC_AT_DENSITY_SELECT_MASK) ?
           FDC_DENSITY_DOUBLE : FDC_DENSITY_SINGLE;
        ctrl_rate = FDC_RATE_250KBPS;
        break;
     case MODFDC_DD:
        ctrl_drive = data & FDC_DD_DRIVE_SELECT_MASK;
        // It's possible to select several drives at once on a dreamdisk system,
        // we don't emulate that, instead we select the lowest one...
        for (i = 0; i < 4; ++i)
           {
            if ((ctrl_drive & 1) == 0)
               break;
            else
               ctrl_drive >>= 1;
           }
        // It's also possible to have NO drive selected on a dreamdisk system,
        // so in that case force drive 0 on.  FIXME
        if (i == 4)
           i = 0;
        ctrl_drive = i;
        ctrl_side = (data & FDC_DD_SIDE_SELECT_MASK) ? 0 : 1;
        // Need to check these magic numbers against the dreamdisk board
        ctrl_ddense = (data & FDC_DD_DENSITY_SELECT_MASK) ?
           FDC_DENSITY_SINGLE : FDC_DENSITY_DOUBLE;
        ctrl_rate = (data & FDC_DD_RATE_SELECT_MASK) ?
           FDC_RATE_500KBPS : FDC_RATE_250KBPS;
        break;
     default:
        assert(0);                        /* should never happen! */
        break;
    }

 fdc_update_data_interval();
 if (modio.fdc)
    {
     struct disk_t *disk = &fdc_drive[ctrl_drive].disk;
     xprintf("fdc_ext_w: drive %d side %d %s density %s\n",
             ctrl_drive, ctrl_side,
             ctrl_ddense == FDC_DENSITY_DOUBLE ? "double" : "single",
             ctrl_rate == FDC_RATE_250KBPS ? "250kb/s" : "500kb/s");
     xprintf("fdc_ext_w: disk is %s density %s\n",
             disk->density == DISK_DENSITY_DOUBLE ? "double" : "single",
             disk->datarate == DISK_RATE_250KBPS ? "250kb/s" : "500kb/s");
     xprintf("fdc_ext_w: data presented every %llu t-states\n", every_cycles);
    }
 fdc_update_drv_status(); // update track0, wp bits in FDC status register
 gui_status_set_persist(GUI_PERSIST_DRIVE, ctrl_drive + 'A');
}

//==============================================================================
// Get intrq and drq
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t                     0x80 if intrq or drq set, else 0
//==============================================================================
uint16_t fdc_ext_r (uint16_t port, struct z80_port_read *port_s)
{
 int status;
 uint64_t cycles_now;

 if (modelx.fdc == 0)
    return 0;

 fdc_data_r_ready();                /* update FDC status */
 fdc_data_w_ready();

 switch (modelx.fdc)
    {
     case MODFDC_AT:
        status = (ctrl_status & (FDC_INTRQ | FDC_DRQ)) ? 0x80 : 0x00;
        // force returning a status value of 0,  used to simulate no disk in
        // drive for v1.15 of 256TC Boot/Net ROM.
        if (fdc.nodisk)
           status = 0x00;
        if (modio.fdc)
           log_port_1("fdc_ext_r", "(ctrl_intrq | ctrl_drq)", port, status);
        break;
     case MODFDC_DD:
        /* The Dreamdisk controller returns the motor status on a read of
         * port 0x48.  DRQ and INTRQ are signalled via NMI# */
        cycles_now = z80api_get_tstates();
        if (cycles_now > ctrl_motoroff_time)
           ctrl_motoron = 0;        /* floppy motor is now OFF */
        status = ctrl_motoron ? 0x80 : 0x00;

        switch (ctrl_side)
           {
            case 0:
               status |= FDC_DD_SIDE_SELECT_MASK;
               break;
            case 1:
               status &= ~FDC_DD_SIDE_SELECT_MASK;
            default:
               break;
           }

        switch (ctrl_ddense)
           {
            case FDC_DENSITY_DOUBLE:
               status &= ~FDC_DD_DENSITY_SELECT_MASK;
               break;
            case FDC_DENSITY_SINGLE:
               status |= FDC_DD_DENSITY_SELECT_MASK;
            default:
               break;
           }

        switch (ctrl_rate)
           {
            case FDC_RATE_250KBPS:
               status &= ~FDC_DD_RATE_SELECT_MASK;
               break;
            case FDC_RATE_500KBPS:
               status |= FDC_DD_RATE_SELECT_MASK;
            default:
               break;
           }

        status |= ~(1 << ctrl_drive) & FDC_DD_DRIVE_SELECT_MASK;
        if (modio.fdc)
           log_port_1("fdc_ext_r", "(ctrl_motoron)", port, status);

        break;

     default:
        status = 0x00;
    }

 return status;
}
