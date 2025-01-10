//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                         Floppy disk support module                         *
//*                                                                            *
//*                        Copyright (C) 2007-2016 uBee                        *
//******************************************************************************
//
// This module implements functions to access floppy disks and/or disk images
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
// v5.8.0 - 15 November 2016, uBee
// - Added detection for LibDsk's 'rcpmfs' type in disk_open() for use by
//   modified disk_read() and disk_write() functions.  If detected and a
//   common Microbee format is also in use then we use reverse skewing.
// - Added string_struct_search_4i(), rcpmfs_setup() and rcpmfs_revskew()
//   functions.
// - Added disk_t structure members have_rcpmfs and cpm3 to the LibDsk
//   section.
//
// v5.7.0 - 14 December 2015, uBee
// - Added FDD and HDD Dynamic CHS naming convention for RAW files. i.e.
//   filename.hdd-490-4-32-512 and filename.fdd-80-2-10-512
// - Added HD2 and HD3 hard disk formats.
// - Added new RAW disk formats SS40S (S4S) and DS40S (D4S). These are
//   single density formats, 40T, 128 b/s, 18 s/t.
// - Changes made to disk_open() (DISK_DSK) to set the density to single
//   where the format is known to be.
// - Fixed a mojor bug that calculated the wrong physical shift value for
//   1024 sector sizes (and above).  The code "idfield->seclen = secsize >>
//   8" has been replaced with "idfield->seclen = get_psh(secsize)".  Note:
//   'secsize' uses different variable names.
// - Added report_dg() to report LibDsk geometry values.
// - Fixed a segmentation fault in disk_create() when searching for a '.'
//   which is now ignored if next to a slash character on either side.
//
// v5.4.0 - 26 April 2012, uBee
// - Added DS8B (and D8B) Beeboard raw file format definition support.
// - Added disk_modify() function to change values for special LibDsk
//   formats such as hs350, hs525 Honeysoft and DS80 formats.  This fixes a
//   slowness problem that was noticable on remote floppy disks where
//   buffering is used and also fixes access to DS80 remote format floppy
//   disks.
// - Removed all the *no_dsk_*[] and *no_drive_status[] code and rely on
//   DSK_ERR_NOTIMPL instead to determine any alternative action.
// - Fixed disk_create() leading '/.' problem causing code to crash.
//   "filename[l+1] != SLASHCHAR" changed to "filename[l-1] != SLASHCHAR"
//
// v5.2.0 - 21 March 2011, uBee
// - Implemented the disk_create() function so that LibDsk can create
//   formatted disk images if built in or by using the internal RAW disk
//   image support.
// - Fixed creation of '.temp' file images by writing out the full size as
//   determined by the raw format extension as the files were not growing as
//   expected.  LibDsk if built in will be used instead of internal RAW file
//   support. This now calls disk_create().
// - Fixed call to dsk_xwrite() by passing '0' instead of '(int)NULL' for
//   the last parameter.
//
// v5.0.0 - 7 September 2010, uBee
// - Fixed hard disk image type '.hd0' to have 306 cylinders, not 305.
//
// v4.7.0 - 29 June 2010, uBee
// - Changes made to fread() function to use the result as some compilers
//   report warning: declared with attribute warn_unused_result.
//
// v4.6.0 - 2 May 2010, uBee
// - Change made in disk_open() as the find_file_alias() function now
//   returns a -1 error if an MD5 can't be located.
//
// v4.5.0 - 31 March 2010, uBee
// - Fixed problems when using LibDsk and dsk_*() functions not supported by
//   the driver by making use of the DSK_ERR_NOTIMPL return value. This
//   value when returned will cause a more standard function call to be used
//   instead. The 'raw' type should now work when used as an input.  Other
//   types may also be working now if they didn't before.
// - Fixed disk_open() LibDsk code to allow 'remote' and 'rcpmfs' types to
//   be used. (rcpmfs requires testing)
// - Changes made to disk_iswrprot() to improve speed when using LibDsk's
//   'remote' driver.
//
// v4.3.0 - 6 August 2009, uBee
// - Added disk_t structure members density and datarate. (workerbee)
// - Removed 'side1as0' when used for normal read/writes with LibDsk as these
//   now use xread/xwrite exclusively. (workerbee)
// - 'side1as0' is still valid as it determines how a disk will be formatted.
//   This is done from the fdc.c module.
// - Added Dreamdisk's double sided head flag (0x80) to RAW/DIP image types
//   in disk_read_idfield() function.
// - Added conditional libdsk reporting depending on the --verbose option.
//
// v4.2.0 - 9 July 2009, uBee
// - Added hard disk image type '.hd0' (305 cylinders, 4 heads, 17 sect/track)
//
// v4.1.0 - 4 July 2009, uBee
// - Made Endian changes to DIP image format.
// - Added hard disk image type '.hd1' (80 cylinders, 4 heads, 63 sect/track)
//
// v4.0.0 - 8 June 2009, uBee
// - Fixed segmentation error in disk_open() function for '.temp' files when
//   a create file fails.  Typically caused by a non-existent path.  Writing
//   to the file is now skipped and a file error is reported later when not
//   found.
//
// v3.1.0 - 22 April 2009, uBee
// - Added conditional message reporting based on --verbose option.
// - Removed all occurrences of console_output() function calls.
// - Changed function name find_file_entry() to find_file_alias().
// - Changes made to disk_read() and disk_write() functions for DISK_DSK type
//   so that mixed 512x10 and 1024x5 tracks will work. This required obtaining
//   information from the (already read) track header each time.
// - Changes made disk_read_idfield() function for DISK_DSK and LIBDSK_DSK
//   to update some disk.imagerec values.  In DISK_DSK the idfield->track,
//   idfield->side, idfield->crc1 and idfield->crc2 values are filled in with
//   values from the dskt structure.
// - Added code to disk_write() and disk_format_track() to adjust the timer
//   exit value 'emu.secs_exit', this is done to prevent exiting while disk
//   write activity may be currently active.
// - Changed all printf() calls to use a local xprintf() function.
//
// v2.7.0 - 18 June 2008, uBee
// - Added creation of disk images when '.temp' is appended to a raw disk
//   file name. Will be over written next time started with same file name.
// - Added find_file_entry() function call to the disk_open() function to
//   find what file to use for file name aliases defined in a configuration
//   file.
// - Changes to disk_open() function, now uses file name in disk_t structure.
// - Added members, 'drive', 'error' and 'image_name' to the disk_t structure.
// - Disk reporting changed.
// - Added double stepping option to support 48tpi disks in 96tpi DD drives.
// - Added double stepping high density option to support 48tpi disks in
//   96tpi HD drives.
//
// v2.6.0 - 8 May 2008, uBee
// - Use an external upper case string copy function.
//
// v2.4.0 - 18 February 2008, uBee
// - Added LibDsk providing disk image, floppy access, allowing other
//   drivers to be employed. The existing image and basic Unices floppy
//   access methods remains.
// - Added a side read method to allow disks in LibDsk to correctly read
//   side 1 when the sector header contains side 0. Microbee DSx0 formats
//   can have this problem depending on the format program used.
// - Added disk_format_track() function.
// - Sector range checking is now carried out in this module.
// - Changed itype numbers, 0 is now 'no type selected'.
// - Removed obsolete and unimplemented disk_create() function.
// - Added disk_init function.
//
// v2.3.0 - 21 January 2008, uBee
// - Added preliminary support for direct floppy access for Unices.
// - Added modio_t structure.
//
// v2.2.0 - 12 December 2007, uBee
// - added image write protect feature.  A trailing underscore '_' character
//   makes the image write protected.
// - disk->imagerec.datatrack value changed to 1 for non DS80 formats. Not
//   an issue as the system and data tracks have the same sector offsets for
//   either tracks anyway.
//
// v2.1.0 - 24 October 2007, uBee
// - Added new function disk_read_idfield() to obtain the ID field from the
//   track, side.
// - Undid the changes made in v2.0.0 to the imagerec.datatrack back to how it
//   is in v1.4.0.
// - Microbee DS80 format fixed,  this format has 4 reserved cylinders not 2.
//   disk->imagerec.datatrack = 4
//
// v2.0.0 - 18 October 2007, uBee
// - DSK format should now be fully complete.
// - Changes to imagerec.datatrack used by all raw and DIP formats now holds
//   the byte offset instead of number of tracks for first data track.
//
// v1.4.0 - 6 October 2007, uBee
// - Added alternative shortened 3 character filename extentions for raw images.
// - The '.dsk' format now includes 3.5" Modular and should work with other
//   unknown disk formats.  Sector offset information is now read per track.
// - Fixed bug preventing '.DSnn' and '.SSnn' raw images from being opened.
//   Changed image_types[][5] to be image_types[][6].
//
// v1.1.0 - 14 August 2007, uBee
// - Added Microbee ds40, ss80, ds80, ds82, ds84 raw disk image types.
// - Modified DSK code to detect all Microbee formats.  This needs testing for
//   some formats. (warning in README)
//
// v1.0.0 - 19 June 2007, uBee
// The following changes/additions have been made:
// - Added new disk image structures,  this now allows for different floppy
//   images to be used instead of only the 40T DS (SBC) images.
// - disk_open function now uses open with write enabled (r+b). This is
//   required to allow disk_write to work.
// - Added an assert to test max_size variable in disk_read function.
// - Implement the disk_write function.
//
// v0.0.0 - 7 June 2007, uBee
// Start with "nanowasp" source distribution version 0.22. An emulator for the
// microbee 128k. Copyright (C) 2000-2003  David G. Churchill.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#ifdef USE_LIBDSK
#include <libdsk.h>
#endif

#include "ubee512.h"
#include "support.h"
#include "disk.h"

//==============================================================================
// structures and variables
//==============================================================================
rcpmfs_args_t rcpmfs_args[] =
 {
  // format BlockSize DirBlocks TotalBlocks SysTracks
  {"ds40",  2048,     2,        195,        2}, // 128 dir entries
  {"ds40s", 2048,     2,        195,        2}, // 128 dir entries
  {"ss80",  2048,     2,        195,        2}, // 128 dir entries
  {"ds80",  4096,     1,        195,        4}, // 128 dir entries
  {"ds82",  2048,     4,        395,        2}, // 256 dir entries
  {"ds84",  4096,     1,        195,        2}, // 128 dir entries (TB=195.5)
  {"ds8b",  2048,     2,        390,        4}, // 128 dir entries
  {"",         0,     0,          0,        0}
 };

extern char userhome_diskpath[];

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;

// these formats are for the built in RAW and DSK driver (not LibDsk)
// The order must match the enumeration for the labels. FIXME
char const image_types[][10] =
{
 ".DIP",                       // Disk Image Plus (in-house format)
 ".DSK",                       // standard CPC DSK format
 ".IMG",                       // Nanowasp v0.22 format
 ".NW",                        // Nanowasp v0.22 format
 ".SS40S",                     // raw images
 ".S4S",
 ".DS40S",
 ".D4S",
 ".DS40",
 ".D40",
 ".SS80",
 ".S80",
 ".DS80",
 ".D80",
 ".DS82",
 ".D82",
 ".DS84",
 ".D84",
 ".DS8B",
 ".D8B",
 ".HD0",
 ".HD1",
 ".HD2",
 ".HD3",
 ".HDD-",
 ".FDD-", 
 ""
};

#ifdef USE_LIBDSK
char *not_image_types[] =
{
 "floppy",
 "ntwdm",
 "remote",
 "rcpmfs",
 ""
};

// comment this out to disable LibDsk geometry debug report
//#define LIBDSK_REPORT_GEOM

#ifdef LIBDSK_REPORT_GEOM
//==============================================================================
// Report DSK_GEOMETRY values (debugging)
//
//   pass: DSK_GEOMETRY *dg
// return: void
//==============================================================================
static void report_dg (DSK_GEOMETRY *dg)
{
 xprintf(
 "\nreport_dg():\n"
 "sidedness=%d\n"
 "cylinders=%d\n"
 "    heads=%d\n"
 "  sectors=%d\n"
 "  secbase=%d\n"
 "  secsize=%d\n"
 " datarate=%d\n"
 "    rwgap=%d\n"
 "   fmtgap=%d\n"
 "       fm=%d\n"
 "  nomulti=%d\n"
 "   noskip=%d\n",
 
 dg->dg_sidedness,
 dg->dg_cylinders,
 dg->dg_heads,
 dg->dg_sectors,
 dg->dg_secbase,
 dg->dg_secsize,
 dg->dg_datarate,
 dg->dg_rwgap,
 dg->dg_fmtgap,
 dg->dg_fm,
 dg->dg_nomulti,
 dg->dg_noskip);
}
#endif

//==============================================================================
// Callback function for LibDsk to report messages.
//
//   pass: void
// return: DSK_REPORTFUNC
//==============================================================================
static void disk_libdsk_report (const char *s)
{
 if (emu.verbose)
    xprintf("LibDsk: %s\n", s);
}

//==============================================================================
// Callback function for LibDsk report end messages.
//
//   pass: void
// return: DSK_REPORTFUNC
//==============================================================================
static void disk_libdsk_report_end (void)
{
}

//==============================================================================
// Search an array of struture strings containing 4 integer values in any
// case for the first occurence of the passed search string.  The string
// array must be terminated by an empty string.
//
//   pass: rcpmfs_args_t *args  pointer to array structure
//         char *strg_find      pointer to a string to search for
// return: int                  index if found, else -1
//==============================================================================
static int string_struct_search_4i (rcpmfs_args_t *args, char *strg_find)
{
 int x = 0;

 while (*args[x].name && strcasecmp(args[x].name, strg_find) != 0)
    x++;

 if (! *args[x].name)
    return -1;

 return x;
}

//==============================================================================
// Create an automated  RCPMFS '.libdsk.ini' file if the disk type is
// 'rcpmfs' and a matching disk format is in use.
//
// If no matching disk format then 'rcpmfs' type may still be used but the
// user needs to create their own '.libdsk.ini' file in the directory
// concerned and only disk formats that have a skew of 1 will work.
//
// The file is only created if it does not exist or if it does then only if
// the 2nd line starts with "AutoCreated=".
//
// If a CP/M 3 file system is in use the --cpm3 option should be specified
// before each instance of --type=rcpmfs.  This sets Version=3, default is
// Version=2 in the '.libdsk.ini' file.
//
// LibDsk appears to happily ignore the "AutoCreated=" line. It is only used
// by uBee512 to determine if the file was originally auto created.
//
// The LibDsk documentation should be consulted for more information
// concerning the '.libdsk.ini' file if required.
//
// The '.libdsk.ini' file created will look similar to this DS40 example:
//
// [RCPMFS]
// # This is an auto generated file and will be overwritten each time
// AutoCreated=uBee512
// BlockSize=2048
// DirBlocks=2
// TotalBlocks=195
// SysTracks=2
// Version=2
// Format=ds40
// secbase=1
//
// TotalBlocks = ((C * H - SysTracks) * 512 * sectrk) / BlockSize
//
//   pass: disk_t *disk
// return: int                          0 if no error, else -1
//==============================================================================
static int rcpmfs_setup (disk_t *disk)
{
 FILE *textfp;
 char temp_str[80];
 char libdsk_file[SSIZE1];
 int have_libdsk_ini;
 int create_libdsk_ini = 0;
 int x;

 // flag a common Microbee format and if 'rcpmfs' type is in use
 x = string_struct_search_4i(rcpmfs_args, disk->libdsk_format);
 if (strcasecmp(disk->libdsk_type, "rcpmfs") == 0 && x != -1)
    disk->have_rcpmfs = x + 1;
 else
    disk->have_rcpmfs = 0;

 // auto create the '.libdsk.ini' file if it does not exist or if it
 // does then only if it was auto created previously
 if (! disk->have_rcpmfs)
    return 0;
    
 snprintf(libdsk_file, sizeof(libdsk_file), "%s"SLASHCHAR_STR".libdsk.ini",
 disk->filename);
 // printf("ini file is: %s\n", libdsk_file);
 textfp = fopen(libdsk_file, "r");
 have_libdsk_ini = textfp != NULL;

 if (have_libdsk_ini)
    {
     // all lines read using file_readline() starting with a '#' or
     // empty lines are ignored

     // does file contain "[RCPMFS]" as the first line?
     file_readline(textfp, temp_str, sizeof(temp_str)-1);
     if (strcmp(temp_str, "[RCPMFS]") == 0)
        {
         // does file contain sub-string "AutoCreated=" on the next line?
         file_readline(textfp, temp_str, sizeof(temp_str)-1);
         create_libdsk_ini =
         strcasestr(temp_str, "AutoCreated=") == temp_str;
        }
    }

 if (textfp)
    fclose(textfp);

 // create the auto '.libdsk.ini' file ?
 if (have_libdsk_ini && ! create_libdsk_ini)
    return 0;

 // if not a supported format return (not an error)
 if ((x = string_struct_search_4i(rcpmfs_args, disk->libdsk_format)) == -1)
    return 0;

#if 0
 printf("Found format: %s bs=%d dblks=%d tb=%d st=%d\n",
 rcpmfs_args[x].name, rcpmfs_args[x].block_size,
 rcpmfs_args[x].dir_blocks, rcpmfs_args[x].total_blocks,
 rcpmfs_args[x].sys_tracks);
 printf("Creating new '%s' file.\n", libdsk_file);
#endif

 // create a new '.libdsk.ini' file
 textfp = fopen(libdsk_file, "w");
 if (! textfp)
    {
     xprintf("rcpmfs_setup: Could not create ini file '%s'\n", libdsk_file);
     return -1;
    }
    
 fprintf(textfp, "[RCPMFS]\n");
 fprintf(textfp, "# This is an auto generated file and will"
                 " be overwritten each time\n");
 fprintf(textfp, "AutoCreated=uBee512\n");
 fprintf(textfp,
 "BlockSize=%d\nDirBlocks=%d\nTotalBlocks=%d\nSysTracks=%d\n",
 rcpmfs_args[x].block_size, rcpmfs_args[x].dir_blocks,
 rcpmfs_args[x].total_blocks, rcpmfs_args[x].sys_tracks);
 fprintf(textfp, "Version=%d\nFormat=%s\nsecbase=1\n",
 disk->cpm3 ? 3:2, disk->libdsk_format);

 fclose(textfp);
 return 0;
}

//==============================================================================
// Reverse skewing for data tracks for known formats when using RCPMFS.
//
//   pass: disk_t *disk
//         const char *op               pointer to read/write string
//         int side                     physical side number
//         int track
//         int sect
// return: int                          sector value, else -1 if error
//==============================================================================
static int rcpmfs_revskew (disk_t *disk, const char *op, int side, int track,
                           int sect)
{
 // microbee sectors       1, 2, 3, 4, 5, 6, 7, 8, 9, 10
 int microbee_revskew[] = {4, 1, 8, 5, 2, 9, 6, 3, 10, 7};

 int i = 0;
 
 // if not using rcpmfs return with sector
 if (! disk->have_rcpmfs)
    return sect;
    
 // if a Microbee format and 'rcpmfs' then do reverse skewing
 // if the track is not a system track
 if (track >= (rcpmfs_args[disk->have_rcpmfs - 1].sys_tracks) / disk->dg.dg_heads)
    {
     // Microbee DS80 data sectors (>= 21)?
     if (sect >= 21)
        i = sect - 20;
     else
        i = sect;
       
     // make sure sector is within table range
     if (i < 0 || i > 10)
        {
         xprintf("disk_%s() rcpmfs Sector BAD reverse skew value: "
         "track=%2d side=%d Skew index=%d\n", op, track, side, i);     
         return -1;
        }
#if 0
     xprintf("disk_%s() rcpmfs Sector reverse skew: "
     "track=%2d side=%d skew=%2d -> %2d\n",
     op, track, side, sect, microbee_revskew[i-1]);
#endif
     sect = microbee_revskew[i-1];
    }
 else
    {
#if 0
     xprintf("disk_%s() rcpmfs Sector: "
     "track=%2d side=%d sect=%2d\n", op, track, side, sect);
#endif
    }

 return sect;
}

//==============================================================================
// Format a disk track.
//
//   pass: disk_t *disk
//         dsk_pcyl_t cyl               cylinder number
//         dsk_phead_t head             physical side of disk
//         int side                     side ID value
// return: dsk_err_t                    DSK_ERR_OK if successful
//==============================================================================
static dsk_err_t format_track (disk_t *disk, dsk_pcyl_t cyl,
       dsk_phead_t head, int side)
{
 dsk_err_t dsk_err = DSK_ERR_OK;

 DSK_FORMAT format[256];
 int i;
    
 for (i = 0; i < disk->dg.dg_sectors; i++)
    {
     format[i].fmt_cylinder = cyl;
     format[i].fmt_head = side;
     format[i].fmt_sector = disk->dg.dg_secbase + i;
     format[i].fmt_secsize = disk->dg.dg_secsize;
    }

 dsk_err = dsk_pformat(disk->self, &disk->dg, cyl, head, format, 0xE5);

 // ignore formatting if the driver does not support the format function. 
 if (dsk_err == DSK_ERR_NOTIMPL)
    return DSK_ERR_OK;
 
 if (dsk_err != DSK_ERR_OK)
    xprintf("\nformat_track: %s\n", dsk_strerror(dsk_err));

 return dsk_err;   
}

//==============================================================================
// Format a disk using LibDsk.
//
//   pass: disk_t *disk
//         char *filename
//         char *disk_format
//         char *disk_type
// return: int                          0 if no errors,  else -1
//==============================================================================
static int format_using_libdsk (disk_t *disk, char *filename, char *disk_format,
                                char *disk_type)
{
 dsk_err_t dsk_err = DSK_ERR_OK;
 dsk_format_t fmt;
 dsk_cchar_t fdesc;
 dsk_cchar_t fname;
 dsk_pcyl_t cyl;
 dsk_phead_t head;

 int secbase1s;
 int secbase2c;
 int secbase2s;

 // find the format name
 strcpy(disk->libdsk_format, disk_format);
 strcpy(disk->libdsk_type, disk_type);

#if 0
 xprintf("disk->libdsk_format=%s\n", disk->libdsk_format);
 xprintf("disk->libdsk_type=%s\n", disk->libdsk_type);
#endif
 
 int found = 0;
 fmt = FMT_180K;
 while ((dg_stdformat(NULL, fmt, &fname, NULL) == DSK_ERR_OK) && (! found))
    {
     if (!strcmp(disk->libdsk_format, fname))
        found = 1;
     else
        ++fmt;
    }

 if (! found)
    {
     xprintf("disk_create: LibDsk format name '%s' not recognised.\n",
     disk->libdsk_format);
     return -1;
    }

  snprintf(disk->image_name, sizeof(disk->image_name), "LIBDSK-%s",
  disk->libdsk_format);

  // initialise disk geometry
  dsk_err = dg_stdformat(&disk->dg, fmt, NULL, &fdesc);
  if (dsk_err != DSK_ERR_OK)
     {
      xprintf("disk_create: LibDsk format name '%s' not recognised.\n", fdesc);
      return -1;
     }

  // use side1 as 0 if native formatted disks are being used
  disk->side1as0 = 0;
  secbase1s = 1;
  secbase2c = -1;
  secbase2s = 1;

  if (strcmp(disk->libdsk_format, "ds80") == 0)
     {
      disk->side1as0 = 1;
      secbase2c = 2;
      secbase2s = 21;
     } 
  if (strcmp(disk->libdsk_format, "ds40") == 0 ||
     strcmp(disk->libdsk_format, "ds40s") == 0)
     disk->side1as0 = 1;

  // create the disk image     
  disk->fdisk = open_file(filename, userhome_diskpath, disk->filepath, "wb");
  if (disk->fdisk)
     fclose(disk->fdisk);
  dsk_err = dsk_creat(&disk->self, disk->filepath, disk->libdsk_type, NULL);
  if (dsk_err != DSK_ERR_OK)
     {
      xprintf("disk_create: dsk_creat() - %s\n", dsk_strerror(dsk_err));
      return -1;
     }

 // format the disk image
 for (cyl = 0; cyl < disk->dg.dg_cylinders; cyl++)
    {
     for (head = 0; head < disk->dg.dg_heads; head++)
        {
         if ((secbase2c != -1) && (cyl >= secbase2c))
            disk->dg.dg_secbase = secbase2s;
         else   
            disk->dg.dg_secbase = secbase1s;
            
         if (emu.verbose)         
            {
             xprintf("\rCylinder: %02d/%02d Head: %d/%d", cyl,
             disk->dg.dg_cylinders-1, head, disk->dg.dg_heads-1);
             fflush(stdout);
            } 
            
         dsk_err = format_track(disk, cyl, head, head & (1 ^ disk->side1as0));
         if (dsk_err != DSK_ERR_OK)
            {
             if (emu.verbose)
                xprintf("\n\nFormat failed.\n");
             return -1;
            }
        }
    }

 if (emu.verbose)
    xprintf("\nFormat completed.\n");
     
 return 0;
}

//==============================================================================
// Disk modify.
//
// On the go disk format modifications used on special formats.  This is
// needed by LibDsk remote and local floppy access.
//
//   pass: disk_t *disk
//         int track
// return: void
//==============================================================================
static void disk_modify (disk_t *disk, int track)
{
 // if format is hs350 or hs525 then change values as needed
 if (! strcmp(disk->libdsk_format, "hs350") ||
    ! strcmp(disk->libdsk_format, "hs525"))
    {
     if (track < 5)
        {
         disk->dg.dg_sectors = 10;
         disk->dg.dg_secsize = 512;
         disk->secsize = 512;
        }
     else
        {
         disk->dg.dg_sectors = 5;
         disk->dg.dg_secsize = 1024;
         disk->secsize = 1024;
        }

     disk->imagerec.sectrack = disk->dg.dg_sectors;
     disk->imagerec.secsize = disk->dg.dg_secsize;
     return;
    }

 // if format is ds80 then change values as needed
 if (! strcmp(disk->libdsk_format, "ds80"))
    {
     if (track < 2)
        disk->dg.dg_secbase = 1;
     else
        disk->dg.dg_secbase = 21;
           
     disk->imagerec.datasecofs = disk->dg.dg_secbase;
     disk->imagerec.systsecofs = disk->dg.dg_secbase;
     return;
    }
}
#endif

//==============================================================================
// Disk create.
//
// This function is used for creating disk images, this will be achieved by
// using the LibDsk library or the simple built in RAW image support.
//
// If 'temp_only' is set then a disk image will only be created if '.temp'
// is appended to the file name.  This method is only used when called from
// disk_open() and is provided for backward compatibilty with earlier
// versions.
//
// When using '.temp' as the last part of the file name the last 3 dots used
// in the name will be used to define the disk format and type.  A file name
// of 'a.b.c.ss80.temp' will fail as 'c' won't be recognised so something
// like 'a.b.c.ss80.raw.temp' would be required.
//
// The filename determines the disk format and type.  The built in support
// is only able to create RAW disk images.  LibDisk is able to create any
// disk images that it supports except for floppy format and remote file
// systems due to limitations in the interfacing being implemented here. 
// Copy protected formats like Honeysoft disks are not supported.
//
// Examples of what may be used:
//
// filename.format.type.temp
// filename.format.type
// filename.format              assumes RAW image
// filename.format.temp         assumes RAW image
//
// It should be noted that the built in RAW image support won't recognise
// the format of a disk if '.raw' is used as the last part of the filename
//
//   pass: disk_t *disk_x
//         int temp_only
// return: int                          0 if no errors,  else -1
//==============================================================================
int disk_create (disk_t *disk_x, int temp_only)
{
#define DISK_SSIZE 20
 disk_t disk;

 char filename[SSIZE1];

 int dot_pos[3];
 
 char s[3][DISK_SSIZE];
 char disk_type[DISK_SSIZE];
 char disk_format[DISK_SSIZE];
 char disk_format_dot[DISK_SSIZE+1];

 int disk_temp = 0;
 int type_start = 0;
 int dot_count = 0;

 int i;
 int l;
 int amount;

 // use a copy of the disk structure only!
 memcpy(&disk, disk_x, sizeof(disk_t));
 
 strcpy(filename, disk.filename);

 // find the last 3 '.' characters in the file name
 for (i = 0; i < 3; i++)
    {
     dot_pos[i] = 0;
     s[i][0] = 0;
    } 

 i = 0;
 l = strlen(filename);
 while (l && i < 3)
    {
     if (filename[--l] == '.' && filename[l-1] != SLASHCHAR &&
     filename[l+1] != SLASHCHAR)
        {
         dot_pos[i++] = l+1;
         dot_count++;
        }
    }

 // extract 3 strings from the file name that have been delimited with '.'
 // string index:               2    1    0
 // example string: filename.format.type.temp
 if (dot_pos[0])
    strncpy(s[0], filename + dot_pos[0], DISK_SSIZE-1);
 s[0][dot_pos[0] - 1] = 0;

 if (dot_pos[1])
    strncpy(s[1], filename + dot_pos[1], dot_pos[0] - dot_pos[1] - 1);
 s[1][dot_pos[0] - dot_pos[1] - 1] = 0;

 if (dot_pos[2])
    strncpy(s[2], filename + dot_pos[2], dot_pos[1] - dot_pos[2] - 1);
 s[2][dot_pos[1] - dot_pos[2] - 1] = 0;

 // get the 'format' and 'type' values
 switch (dot_count)
    {
     case 0 : // can't do anything without any information!, so just return
        return 0;
     case 1 : // filename.format
        strcpy(disk_format, s[0]);
        strcpy(disk_type, "raw");
        break;
     case 2 : // filename.format.temp or filename.format.type
        strcpy(disk_format, s[1]);
        if (strcasecmp(s[0], "temp") == 0)
           {
            strcpy(disk_type, "raw");
            disk_temp = 1;
           }
        else
           strcpy(disk_type, s[0]);
        break;   
     case 3 : // filename.format.type.temp or filename.other.format.type
        if (strcasecmp(s[0], "temp") == 0)
           {
            strcpy(disk_format, s[2]);
            strcpy(disk_type, s[1]);
            disk_temp = 1;
           }
        else
           {
            strcpy(disk_format, s[1]);
            strcpy(disk_type, s[0]);
           }
        break;
    }

 // check if we only want to create a file when it has '.temp' in the name
 if (temp_only && ! disk_temp)
    return 0;

#if 0
 xprintf("dot_count=%d\n", dot_count);
 xprintf("dot positions: s0=%d s1=%d s2=%d\n",
         dot_pos[0], dot_pos[1], dot_pos[2]);
 xprintf("strings:       s0=%s s1=%s s2=%s\n",
         s[0], s[1], s[2]);
#endif

 // if LibDsk is compiled in we try using it first and if it fails we try
 // with the built in RAW file support.
#ifdef USE_LIBDSK
 if (format_using_libdsk(&disk, filename, disk_format, disk_type) != -1)
    return 0;
#endif 

 xprintf("disk_create: Attempt image creation using built in support...\n");
 
 sprintf(disk_format_dot, ".%s", disk_format); // add a leading '.'

 for (i = type_start; image_types[i][0] != 0; i++)
    {
     if (strcasecmp(disk_format_dot, image_types[i]) == 0)
        break;
    }
 if (image_types[i][0] == 0)
    {
     xprintf("Can't create disk image for format '%s'.\n",
     disk_format);
     return -1;
    } 
 i++;

 // set the amount value based on the disk type for RAW image types only
 if (i == DISK_DS40S || i == DISK_D4S)
    amount = 184320;
 else if (i == DISK_SS40S || i == DISK_S4S)
    amount = 92160;
 else if ((i >= DISK_IMG && i <= DISK_NW) || (i >= DISK_DS40 && i <= DISK_S80))
    amount = 409600;
 else if (i >= DISK_DS80 && i <= DISK_D8B)
    amount = 819200;
 else if (i == DISK_HD0)
    amount = 10653696;
 else if (i == DISK_HD1)
    amount = 10321920;
 else if (i == DISK_HD2)
    amount = 8060928;
 else if (i == DISK_HD3)
    amount = 32112640;
 else
    {
     xprintf("Can't create disk image for format '%s'.\n",
     disk_format);
     return -1;
    }

 // create the RAW disk image filling it with 0xe5
 disk.fdisk = open_file(filename, userhome_diskpath, disk.filepath, "wb");
 if (disk.fdisk)
    {
     while (amount--)
        fputc(0xe5, disk.fdisk);
     fclose(disk.fdisk);
     return 0;
    }

 xprintf("Can't create disk image '%s'.\n", disk.filepath);        
 return -1;
}

//==============================================================================
// Initialise.
//
// Called from fdc.c fdc_init function
//
//   pass: void
// return: int                  0 if no errors, else -1
//==============================================================================
int disk_init (void)
{
#ifdef USE_LIBDSK
 dsk_reportfunc_set(disk_libdsk_report, disk_libdsk_report_end);
#endif
 return 0;
}

//==============================================================================
// Disk open.
//
// Open a disk format for the disk structure requested.  This can be one of
// the following:
//
// * original v0.22 raw disk format (40T DS DD) Side0=1st 200K, Side1=2nd 200K
// * CPC-EMU dsk format (ubee512 built-in support)
// * dip format (disk image plus) This is an in-house spec not finalised.
// * ds40 - Microbee 40T DS DD (SBC) S0-T0, S1-T0, S0-T1, S1-T1 ...
// * ss80 - Microbee 80T SS DD (CIAB) S0-T0, S0-T1, S0-T2 ...
// * ds80 - Microbee 80T DS DD (modular) S0-T0, S1-T0, S0-T1, S1-T1 ...
// * ds82 - Microbee 80T DS DD (dream) S0-T0, S1-T0, S0-T1, S1-T1 ...
// * ds84 - Microbee 80T DS DD (pjb) S0-T0, S1-T0, S0-T1, S1-T1 ...
// * hdx  - Various preset HDD images with x set to a format number.
// * HDD- - HDD Dynamic CHS naming convention for RAW files.
// * FDD- - FDD Dynamic CHS naming convention for RAW files.
// * LibDsk - Many types
//
//   pass: disk_t *disk
// return: int                          0 if no error, else -1
//==============================================================================
int disk_open (disk_t *disk)
{
 diski_t temp_imagerec;

 char filename[SSIZE1];
 char *p;
 char *c;
 char sp[100];

 int i;
 int type_start = 0;
 int itype_temp;

 int dummy;
 int tracks;
 int heads;
 int sectrack;
 int secsize;

 dski_t dski;                   // dsk information block
 dskt_t dskt;                   // dsk track information block

#ifdef USE_LIBDSK
 dsk_err_t dsk_err;
 dsk_format_t fmt;
 dsk_cchar_t fdesc;
 dsk_cchar_t fname;
#endif

 char uname[SSIZE1];

 // no type selected
 disk->itype = 0;

 disk->error = 0;

 // see if the name has an alias file name entry
 if (emu.alias_disks)
    {
     if (find_file_alias(ALIASES_DISKS, disk->filename, filename) == -1)
        {
         xprintf("disk_open: Drive %c: can't find md5 entry for '%s'\n",
         disk->drive+'A', disk->filename);
         return -1;
        }
    }
 else
    strcpy(filename, disk->filename);

  // make an upper case copy of the file name
  toupper_string(uname, filename);

  // check and create a temporary disk image if '.temp' is found
  if (disk_create(disk, 1) == -1)
     return -1;

#ifdef USE_LIBDSK
 if (! strlen(disk->libdsk_format)) // if not using LibDsk
    {
#endif
     // direct floppy access. ie /dev/fd0u800/.ds84
#ifdef MINGW
#else
     if (strstr(filename, "/dev/") == filename)
        {
         char *ch = strstr(filename, "/.");
         if (ch)
            *ch = 0;
         type_start = 4;
        }
#endif

     // open the image or floppy
     i = (test_file(filename, userhome_diskpath, disk->filepath) == NULL);
     if (emu.verbose)
        xprintf("disk_open: Drive %c: %s\n",  disk->drive+'A', disk->filepath);

     if (i)
        {
         disk->error = DISK_ERR_NOTFOUND;
         return -1;
        }

     disk->fdisk = open_file(filename, userhome_diskpath, disk->filepath,
     "r+b");
     if (disk->fdisk == NULL)
        {
         disk->error = DISK_ERR_READONLY;
         return -1;
        }

     // make an upper case copy of the file name
     toupper_string(uname, disk->filepath);

     // image is write protected if a trailing '_' is found
     disk->wrprot = uname[strlen(uname)-1] == '_';
     if (disk->wrprot)
        uname[strlen(uname)-1] = 0;             // remove the '_'

     // find the disk type
     for (i = type_start; image_types[i][0] != 0; i++)
        {
         if (strstr(uname, image_types[i]) != NULL)
            break;
        }
     if (image_types[i][0] == 0)
        return -1;

     itype_temp = i + 1;

#ifdef USE_LIBDSK
    }
 else
    {
     disk->wrprot = 0;
     itype_temp = DISK_LIBDSK;
    }
#endif

 // configure parameters for the disk type
 switch (itype_temp)
    {
     case DISK_DIP : // DIP (in house spec)
        fseek(disk->fdisk, -sizeof(disk->imagerec), SEEK_END);

        // read into temporary location as endianness needs addressing
        if (fread(&temp_imagerec, sizeof(temp_imagerec), 1, disk->fdisk) != 1)
           return -1;
        memcpy(&disk->imagerec, &temp_imagerec, sizeof(disk->imagerec));
        disk->imagerec.wrprot = leu16_to_host(temp_imagerec.wrprot);
        disk->imagerec.tracks = leu16_to_host(temp_imagerec.tracks);
        disk->imagerec.heads = leu16_to_host(temp_imagerec.heads);
        disk->imagerec.secsize = leu16_to_host(temp_imagerec.secsize);
        disk->imagerec.sectrack = leu16_to_host(temp_imagerec.sectrack);
        disk->imagerec.datatrack = leu16_to_host(temp_imagerec.datatrack);
        disk->imagerec.systsecofs = leu16_to_host(temp_imagerec.systsecofs);
        disk->imagerec.datasecofs = leu16_to_host(temp_imagerec.datasecofs);
        disk->imagerec.secterrs = leu16_to_host(temp_imagerec.secterrs);

        if (strcmp(disk->imagerec.id, "DISK-IMAGE-PLUS") != 0)
           return -1;
        if ((disk->imagerec.secsize != 128) &&
           (disk->imagerec.secsize != 256) &&
           (disk->imagerec.secsize != 512) &&
           (disk->imagerec.secsize != 1024))
           return -1;
        if (! disk->wrprot)   // if not already '_' write protected
           disk->wrprot = disk->imagerec.wrprot;
        // default image to double density, 250kbps data rate
        disk->density = DISK_DENSITY_DOUBLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "DIP");
        break;
     case DISK_DSK : // DSK
        // get the disk information block
        if (fread(&dski, sizeof(dski), 1, disk->fdisk) != 1)
           return -1;
        disk->imagerec.tracks = dski.tracks;
        disk->imagerec.heads = dski.heads;

        // get the first track information block to get some initial values
        if (fread(&dskt, sizeof(dskt), 1, disk->fdisk) != 1)
           return -1;
        disk->imagerec.sectrack = dskt.spt;
        disk->imagerec.secsize = 128 << dskt.bps; // 0=128, 1=256, 2=512, 3=1024
        disk->imagerec.systsecofs = dskt.sect_numb;

        // the DSK format does not appear to support a density value? so the
        // disk density will be assumed to be double unless the disk is
        // something we know to be single (could force with an option)
        if (disk->imagerec.secsize == 128 && disk->imagerec.sectrack <= 18)
           disk->density = DISK_DENSITY_SINGLE;
        else
           disk->density = DISK_DENSITY_DOUBLE;

        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "DSK");
        break;
     case DISK_IMG : // IMG (nanowasp v0.22 format)
     case DISK_NW : // NW (nanowasp v0.22 format)
        disk->imagerec.tracks = 40;
        disk->imagerec.heads = 2;
        disk->imagerec.sectrack = 10;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        disk->density = DISK_DENSITY_DOUBLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "RAW (IMG/NW)");
        break;
     case DISK_SS40S : // SS40S - Early formats SS SD 40T 128 b/s 18 s/t
     case DISK_S4S : // S4S
        disk->imagerec.tracks = 40;
        disk->imagerec.heads = 1;
        disk->imagerec.sectrack = 18;
        disk->imagerec.secsize = 128;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        disk->density = DISK_DENSITY_SINGLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "RAW");
        break;
     case DISK_DS40S : // DS40S - Early formats DS SD 40T 128 b/s 18 s/t
     case DISK_D4S : // D4S
        disk->imagerec.tracks = 40;
        disk->imagerec.heads = 2;
        disk->imagerec.sectrack = 18;
        disk->imagerec.secsize = 128;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        disk->density = DISK_DENSITY_SINGLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "RAW");
        break;
     case DISK_DS40 : // DS40
     case DISK_D40 : // D40
        disk->imagerec.tracks = 40;
        disk->imagerec.heads = 2;
        disk->imagerec.sectrack = 10;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        disk->density = DISK_DENSITY_DOUBLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "RAW");
        break;
     case DISK_SS80 : // SS80
     case DISK_S80 : // S80
        disk->imagerec.tracks = 80;
        disk->imagerec.heads = 1;
        disk->imagerec.sectrack = 10;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        disk->density = DISK_DENSITY_DOUBLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "RAW SS80");
        break;
     case DISK_DS80 : // DS80 (Microbee modular)
     case DISK_D80 : // D80 (Microbee modular)
        disk->imagerec.tracks = 80;
        disk->imagerec.heads = 2;
        disk->imagerec.sectrack = 10;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 2;         // 4 reserved tracks
        disk->imagerec.datasecofs = 21;       // sectors are 21 to 30
        disk->imagerec.systsecofs = 1;
        disk->density = DISK_DENSITY_DOUBLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "RAW DS80");
        break;
     case DISK_DS82 : // DS82
     case DISK_D82 : // D82
        disk->imagerec.tracks = 80;
        disk->imagerec.heads = 2;
        disk->imagerec.sectrack = 10;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        disk->density = DISK_DENSITY_DOUBLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "RAW DS82");
        break;
     case DISK_DS84 : // DS84
     case DISK_D84 : // D84
        disk->imagerec.tracks = 80;
        disk->imagerec.heads = 2;
        disk->imagerec.sectrack = 10;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        disk->density = DISK_DENSITY_DOUBLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "RAW DS84");
        break;
     case DISK_DS8B : // DS8B (Microbee Beeboard)
     case DISK_D8B : // D8B (Microbee Beeboard)
        disk->imagerec.tracks = 80;
        disk->imagerec.heads = 2;
        disk->imagerec.sectrack = 10;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 2;        // 4 reserved tracks
        disk->imagerec.datasecofs = 1;       // sectors are 1 to 10
        disk->imagerec.systsecofs = 1;
        disk->density = DISK_DENSITY_DOUBLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "RAW DS8B");
        break;
     case DISK_HD0 : // HDD (10MB) 306 cylinders, 4 heads, 17 sect/track
        disk->imagerec.tracks = 306;
        disk->imagerec.heads = 4;
        disk->imagerec.sectrack = 17;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        strcpy(disk->image_name, "RAW HD0");
        break;
     case DISK_HD1 : // HDD (10MB) 80 cylinders, 4 heads, 63 sect/track
        disk->imagerec.tracks = 80;
        disk->imagerec.heads = 4;
        disk->imagerec.sectrack = 63;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        // Density and data rate aren't used for a harddisk image
        disk->density = DISK_DENSITY_DOUBLE;
        disk->datarate = DISK_RATE_250KBPS;
        strcpy(disk->image_name, "RAW HD1");
        break;
     case DISK_HD2 : // HDD (8MB) 246 cylinders, 2 heads, 32 sect/track (i.e GB's CF8)
        disk->imagerec.tracks = 246;
        disk->imagerec.heads = 2;
        disk->imagerec.sectrack = 32;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        strcpy(disk->image_name, "RAW HD2");
        break;
     case DISK_HD3 : // HDD (32MB) 490 cylinders, 4 heads, 32 sect/track (i.e GB's CF8)
        disk->imagerec.tracks = 490;
        disk->imagerec.heads = 4;
        disk->imagerec.sectrack = 32;
        disk->imagerec.secsize = 512;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;
        strcpy(disk->image_name, "RAW HD3");
        break;
     case DISK_HDD : // HDD Dynamic CHS naming. i.e. filename.hdd-490-4-32[-512]
     case DISK_FDD : // FDD Dynamic CHS naming. i.e. filename.fdd-80-2-10[-512]
        p = strrchr(uname, '.');
        c = get_next_parameter(p, '-', sp, &dummy, sizeof(sp)-1);
        c = get_next_parameter(c, '-', sp, &tracks, sizeof(sp)-1);
        c = get_next_parameter(c, '-', sp, &heads, sizeof(sp)-1);
        c = get_next_parameter(c, '-', sp, &sectrack, sizeof(sp)-1);
        if (c)
           c = get_next_parameter(c, '-', sp, &secsize, sizeof(sp)-1);
        else
           secsize = 512;

        if (tracks == -1 || heads == -1 || sectrack == -1 || secsize == -1)
            {
             xprintf("disk_open: error in dynamic CHS value\n");
             disk->itype = 0;
             return -1;
            }
            
        disk->imagerec.tracks = tracks;
        disk->imagerec.heads = heads;
        disk->imagerec.sectrack = sectrack;
        disk->imagerec.secsize = secsize;
        disk->imagerec.datatrack = 1;
        disk->imagerec.datasecofs = 1;
        disk->imagerec.systsecofs = 1;

        if (itype_temp == DISK_HDD)
           sprintf(disk->image_name, "RAW HDD-%d-%d-%d-%d", tracks, heads, sectrack, secsize);
        else
           {
            disk->density = DISK_DENSITY_DOUBLE;
            disk->datarate = DISK_RATE_250KBPS;
            sprintf(disk->image_name, "RAW FDD-%d-%d-%d-%d", tracks, heads, sectrack, secsize);
           }
        break;

#ifdef USE_LIBDSK
     case DISK_LIBDSK : // LibDsk
        disk->itype = 0;

        if (emu.verbose)
           xprintf("disk_open: Drive %c: %s\n",  disk->drive+'A', filename);

        // Find the format name
        {
        int found = 0;
        fmt = FMT_180K;
        while ((dg_stdformat(NULL, fmt, &fname, NULL) == DSK_ERR_OK) &&
        (! found))
           {
            if (!strcmp(disk->libdsk_format, fname))
               found = 1;
            else
               ++fmt;
           }

        if (! found)
           {
            xprintf("disk_open: LibDsk format name '%s' not recognised.\n",
            disk->libdsk_format);
            return -1;
           }
         }
         snprintf(disk->image_name, sizeof(disk->image_name), "LIBDSK-%s",
         disk->libdsk_format);

         // Initialise disk geometry
         dsk_err = dg_stdformat(&disk->dg, fmt, NULL, &fdesc);
         if (dsk_err != DSK_ERR_OK)
            {
             xprintf("disk_open: LibDsk format name '%s' not recognised.\n",
                     fdesc);
             return -1;
            }

         // Allow the Data rate to be changed for DS40 DD media in HD 80T
         // 1.2MB drives.
         if (disk->dstep_hd)
            disk->dg.dg_datarate = RATE_DD;

         disk->imagerec.tracks = disk->dg.dg_cylinders;
         disk->imagerec.heads = disk->dg.dg_heads;
         disk->imagerec.sectrack = disk->dg.dg_sectors;
         disk->imagerec.secsize = disk->dg.dg_secsize;
         disk->imagerec.datatrack = 1;
         disk->imagerec.datasecofs = disk->dg.dg_secbase;
         disk->imagerec.systsecofs = disk->dg.dg_secbase;
#ifdef LIBDSK_REPORT_GEOM
         report_dg(&disk->dg);
#endif
         switch (disk->dg.dg_datarate)
            {
             case RATE_HD : // Data rate for 1.4Mb 3.5"  in 3.5"  drive
                disk->datarate = DISK_RATE_500KBPS;
                break;
             case RATE_DD : // Data rate for 360k  5.25" in 1.2Mb drive
             case RATE_SD : // Data rate for 720k  3.5"  in 3.5"  drive
                disk->datarate = DISK_RATE_250KBPS;
                break;
             case RATE_ED :
                // Data rate for 2.8Mb 3.5"  in 3.5"  drive
                // we don't support this.
             default :
                assert(0);
                break;
            }
         disk->density = disk->dg.dg_fm ? \
         DISK_DENSITY_SINGLE : DISK_DENSITY_DOUBLE;

         // use side1 as 0 if native formatted disks are being used
         if ((strcmp(disk->libdsk_format, "ds80") == 0) ||
            (strcmp(disk->libdsk_format, "ds40") == 0) ||
            (strcmp(disk->libdsk_format, "ds40s") == 0))
            disk->side1as0 = 1;

         // check if the RCPMFS type is in use and set-up accordingly
         if (rcpmfs_setup(disk) == -1)
            return -1;

         // open the disk image, floppy, rcpmfs or remote device
         if (string_search(not_image_types, disk->libdsk_type) != -1)
            dsk_err = dsk_open(&disk->self, filename, disk->libdsk_type, NULL);
         else
            {
             test_file(filename, userhome_diskpath, disk->filepath);
             if (strlen(disk->libdsk_type))
                dsk_err = dsk_open(&disk->self, disk->filepath,
                disk->libdsk_type, NULL);
             else
                dsk_err = dsk_open(&disk->self, disk->filepath,
                NULL, NULL);
            }

         // use double stepping if --dstep or --dstep-hd was specified
         if (disk->dstep)
            dsk_set_option(disk->self, "DOUBLESTEP", 1);

         if (disk->self)
            dsk_set_retry(disk->self, 5);
         else
            {
             xprintf("disk_open: dsk_open error: dsk_err=%d\n", dsk_err);
             disk->itype = 0;
             return -1;
            }
         break;
#endif
    }

 disk->itype = itype_temp;

#ifdef use_info_disk_open
 if (emu.verbose)
    xprintf("disk_open: %s   Tracks: %d   Heads: %d   S/T: %d   B/S: %d\n",
    disk->image_name,
    disk->imagerec.tracks, disk->imagerec.heads,
    disk->imagerec.sectrack, disk->imagerec.secsize);
#endif

 return 0;
}

//==============================================================================
// Disk close.
//
//   pass: disk_t *disk
// return: void
//==============================================================================
void disk_close (disk_t *disk)
{
#ifdef USE_LIBDSK
 if (disk->itype == DISK_LIBDSK)
    dsk_close(&disk->self);
 else
#endif
    fclose(disk->fdisk);

 disk->fdisk = NULL;
 disk->itype = 0;
}

//==============================================================================
// Disk read.
//
//   pass: disk_t *disk
//         char *buf
//         int side                     physical side number
//         int idside                   side number in the ID field
//         int track
//         int sect
//         char rtype                   m if a multi sector read operation
// return: int                          0 if no errors, else error number
//==============================================================================
int disk_read (disk_t *disk, char *buf, int side, int idside, int track,
               int sect, char rtype)
{
 dskt_t dskt;   // dsk track information block

 int res = 0;

#ifdef USE_LIBDSK
 dsk_err_t dsk_err = 0;
#endif

 int trackofs;
 int sectofs;
 int sectuse;
 int dskofs;

#ifdef use_debug_disk_read_abort
 xprintf("disk_read: forcing an abort here\n");
 assert(1 != 1);
#endif

//#define use_info_disk_read

#ifdef use_info_disk_read
 if (emu.verbose)
    {
     xprintf("Tracks: %d   Heads: %d   S/T: %d   B/S: %d\n",
     disk->imagerec.tracks, disk->imagerec.heads,
     disk->imagerec.sectrack, disk->imagerec.secsize);
     xprintf("Track: %3d   Side: %3d   Sector: %3d\n",  track, side, sect);
    }
#endif

 switch (disk->itype)
    {
     case DISK_DIP : // DIP (in house spec)
        trackofs = track * disk->imagerec.heads * disk->imagerec.sectrack *
        disk->imagerec.secsize;
        if (track >= disk->imagerec.datatrack)
           sectuse = sect - disk->imagerec.datasecofs;
        else
           sectuse = sect - disk->imagerec.systsecofs;
        sectofs = side * disk->imagerec.sectrack * disk->imagerec.secsize +
        (sectuse * disk->imagerec.secsize);
#ifdef use_info_disk_read
        if (emu.verbose)
           xprintf("Image: %s   Vers: %s   Type: %s\n", disk->imagerec.id,
           disk->imagerec.ver, disk->imagerec.type);
#endif
        if ((sectuse >= 0) && (sectuse < disk->imagerec.sectrack))
           {
            fseek(disk->fdisk, trackofs + sectofs, SEEK_SET);
            if (fread(buf, disk->imagerec.secsize, 1, disk->fdisk) != 1)
               res = -1;
           }
        else
           res = -1;
        break;
     case DISK_DSK : // DSK
        trackofs = track * disk->imagerec.heads * disk->imagerec.sectrack *
        disk->imagerec.secsize;
        dskofs = (((track * disk->imagerec.heads) + side + 1) * 0x100) + 0x100;

        // read the track information header for the track
        sectofs = side * disk->imagerec.sectrack * disk->imagerec.secsize;
        fseek(disk->fdisk, trackofs + sectofs + dskofs - 0x100, SEEK_SET);

        if (fread(&dskt, sizeof(dskt), 1, disk->fdisk) != 1)
           {
            res = -1;
            break;
           }
        disk->imagerec.datasecofs = dskt.sect_numb;
        disk->imagerec.systsecofs = dskt.sect_numb;
        sectuse = sect - disk->imagerec.datasecofs;

        // the sectors per track and size may change on some DSK images
        disk->imagerec.sectrack = dskt.spt;
        disk->imagerec.secsize = 128 << dskt.bps; // 0=128, 1=256, 2=512, 3=1024

        // seek and read the sector
        if ((sectuse >= 0) && (sectuse < disk->imagerec.sectrack))
           {
            sectofs = side * disk->imagerec.sectrack * disk->imagerec.secsize +
            (sectuse * disk->imagerec.secsize);
            fseek(disk->fdisk, trackofs + sectofs + dskofs, SEEK_SET);
            if (fread(buf, disk->imagerec.secsize, 1, disk->fdisk) != 1)
               res = -1;
           }
        else
           res = -1;
        break;
     case DISK_IMG : // IMG (nanowasp v0.22 format)
     case DISK_NW : // NW (nanowasp v0.22 format)
        if ((sect > 0) && (sect <= disk->imagerec.sectrack))
           {
            fseek(disk->fdisk, ((side * 40 + track) * 10 + (sect - 1)) * 512,
            SEEK_SET);
            if (fread(buf, 512, 1, disk->fdisk) != 1)
               res = -1;
           }
        else
           res = -1;
        break;
     case DISK_SS40S : // SS40S
     case DISK_S4S : // S4S
     case DISK_DS40S : // DS40S
     case DISK_D4S : // D4S
     case DISK_DS40 : // DS40
     case DISK_D40 : // D40
     case DISK_SS80 : // SS80
     case DISK_S80 : // S80
     case DISK_DS80 : // DS80 (Microbee modular)
     case DISK_D80 : // D80 (Microbee modular)
     case DISK_DS82 : // DS82
     case DISK_D82 : // D82
     case DISK_DS84 : // DS84
     case DISK_D84 : // D84
     case DISK_DS8B : // DS8B (Microbee Beeboard)
     case DISK_D8B : // D8B (Microbee Beeboard)
     case DISK_HD0 : // HDD (10MB) 306 cylinders, 4 heads, 17 sect/track
     case DISK_HD1 : // HDD (10MB) 80 cylinders, 4 heads, 63 sect/track
     case DISK_HD2 : // HDD (8MB) 246 cylinders, 2 heads, 32 sect/track (i.e GB's CF8)
     case DISK_HD3 : // HDD (32MB) 490 cylinders, 4 heads, 32 sect/track (i.e GB's CF8)
     case DISK_HDD : // HDD Dynamic CHS naming. i.e. filename.hdd-490-4-32[-512]
     case DISK_FDD : // FDD Dynamic CHS naming. i.e. filename.fdd-80-2-10[-512]
        trackofs = track * disk->imagerec.heads * disk->imagerec.sectrack *
        disk->imagerec.secsize;
        sectofs = side * disk->imagerec.sectrack * disk->imagerec.secsize;

        if (track >= disk->imagerec.datatrack)
           sectuse = sect - disk->imagerec.datasecofs;
        else
           sectuse = sect - disk->imagerec.systsecofs;

        sectofs += sectuse * disk->imagerec.secsize;

        if ((sectuse >= 0) && (sectuse < disk->imagerec.sectrack))
           {
            fseek(disk->fdisk, trackofs + sectofs, SEEK_SET);
            if (fread(buf, disk->imagerec.secsize, 1, disk->fdisk) != 1)
               res = -1;
           }
        else
           res = -1;
        break;

        trackofs = track * disk->imagerec.heads * disk->imagerec.sectrack *
        disk->imagerec.secsize;
        sectofs = side * disk->imagerec.sectrack * disk->imagerec.secsize;

        if (track >= disk->imagerec.datatrack)
           sectuse = sect - disk->imagerec.datasecofs;
        else
           sectuse = sect - disk->imagerec.systsecofs;

        sectofs += sectuse * disk->imagerec.secsize;

        if ((sectuse >= 0) && (sectuse < disk->imagerec.sectrack))
           {
            fseek(disk->fdisk, trackofs + sectofs, SEEK_SET);
            if (fread(buf, disk->imagerec.secsize, 1, disk->fdisk) != 1)
               res = -1;
           }
        else
           res = -1;
        break;

#ifdef USE_LIBDSK
     case DISK_LIBDSK : // LibDsk
        // handle special formats while on the go where required
        disk_modify(disk, track);
        
        // handle rcpmfs type if active or return same sector
        if ((sect = rcpmfs_revskew(disk, "read", side, track, sect)) == -1)
           return -1;
        
        dsk_err = dsk_xread(disk->self, &disk->dg, buf, track, side, track,
        idside, sect, disk->secsize, NULL);

        if (dsk_err == DSK_ERR_NOTIMPL)
           dsk_err = dsk_pread(disk->self, &disk->dg, buf, track, side, sect);

        if (dsk_err != DSK_ERR_OK)
           res = -1;
        break;
#endif
    }

 // check if there were any errors, if so report and exit
 if (res)
    {
#ifdef USE_LIBDSK
     if (disk->itype == DISK_LIBDSK)
        {
         // don't report multi sector error
         if ((dsk_err != DSK_ERR_NODATA) && (rtype != 'm')) 
            {
             xprintf("disk_read: dsk_xread");
             xprintf(" error: file=%s dsk_err=%d %s\n", 
                     disk->filepath, dsk_err, dsk_strerror(dsk_err));
             xprintf("Track: %3d   Side: %3d   Sector: %3d   IDside: %3d\n",
                     track, side, sect, idside);
            }
        }
     else
#endif
        {
         if (rtype != 'm') // don't report multi sector error
            {
             xprintf("disk_read: (inbuilt) file=%s res=%d\n",
                     disk->filepath, res);
             xprintf("Track: %3d   Side: %3d   Sector: %3d\n",
                     track, side, sect);
            }
        }

     return res;
    }

 return 0;
}

//==============================================================================
// Disk write.
//
// A flush call is used on each sector write.
//
//   pass: disk_t *disk
//         char *buf
//         int side
//         int track
//         int sect
//         char wtype                   m if a multi sector read operation
// return: int                          0 if no errors, else error number
//==============================================================================
int disk_write (disk_t *disk, char *buf, int side, int idside, int track,
                int sect, char wtype)
{
 dskt_t dskt;                   // dsk track information block

 int res = 0;

#ifdef USE_LIBDSK
 dsk_err_t dsk_err = 0;
#endif

 int trackofs;
 int sectofs;
 int sectuse;
 int dskofs;

 // reset the exit seconds counter to a new minimum value every time we write
 // to disk and emu.secs_exit is not zero.
 if ((emu.secs_exit) && ((emu.secs_run + 3) >= emu.secs_exit))
    emu.secs_exit = emu.secs_run + 3;

#ifdef use_info_disk_read
 if (emu.verbose)
    {
     xprintf("Tracks: %d   Heads: %d   S/T: %d   B/S: %d\n",
     disk->imagerec.tracks, disk->imagerec.heads,
     disk->imagerec.sectrack, disk->imagerec.secsize);
     xprintf("Track: %3d   Side: %3d   Sector: %3d\n",  track, side, sect);
    }
#endif

 switch (disk->itype)
    {
     case DISK_DIP : // DIP (in house spec)
        trackofs = track * disk->imagerec.heads * disk->imagerec.sectrack *
        disk->imagerec.secsize;
        if (track >= disk->imagerec.datatrack)
           sectuse = sect - disk->imagerec.datasecofs;
        else
           sectuse = sect - disk->imagerec.systsecofs;
        sectofs = side * disk->imagerec.sectrack * disk->imagerec.secsize +
        (sectuse * disk->imagerec.secsize);
#ifdef use_info_disk_read
        if (emu.verbose)
           xprintf("Image: %s   Vers: %s   Type: %s\n", disk->imagerec.id,
           disk->imagerec.ver, disk->imagerec.type);
#endif
        if ((sectuse >= 0) && (sectuse < disk->imagerec.sectrack))
           {
            fseek(disk->fdisk, trackofs + sectofs, SEEK_SET);
            fwrite(buf, disk->imagerec.secsize, 1, disk->fdisk);
            fflush(disk->fdisk);
           }
        else
           res = -1;
        break;
     case DISK_DSK : // DSK
        trackofs = track * disk->imagerec.heads * disk->imagerec.sectrack *
        disk->imagerec.secsize;
        dskofs = (((track * disk->imagerec.heads) + side + 1) * 0x100) + 0x100;

        // read the track information header for the track
        sectofs = side * disk->imagerec.sectrack * disk->imagerec.secsize;
        fseek(disk->fdisk, trackofs + sectofs + dskofs - 0x100, SEEK_SET);

        if (fread(&dskt, sizeof(dskt), 1, disk->fdisk) != 1)
           {
            res = -1;
            break;
           }
           
        disk->imagerec.datasecofs = dskt.sect_numb;
        disk->imagerec.systsecofs = dskt.sect_numb;
        sectuse = sect - disk->imagerec.datasecofs;

        // the sectors per track and size may change on some DSK images
        disk->imagerec.sectrack = dskt.spt;
        disk->imagerec.secsize = 128 << dskt.bps; // 0=128, 1=256, 2=512, 3=1024

        // seek and write the sector
        sectofs = side * disk->imagerec.sectrack * disk->imagerec.secsize +
        (sectuse * disk->imagerec.secsize);
        if ((sectuse >= 0) && (sectuse < disk->imagerec.sectrack))
           {
            fseek(disk->fdisk, trackofs + sectofs + dskofs, SEEK_SET);
            fwrite(buf, disk->imagerec.secsize, 1, disk->fdisk);
            fflush(disk->fdisk);
           }
        else
           res = -1;
        break;
     case DISK_IMG : // IMG (nanowasp v0.22 format)
     case DISK_NW : // NW (nanowasp v0.22 format)
        if ((sect > 0) && (sect <= disk->imagerec.sectrack))
           {
            fseek(disk->fdisk, ((side * 40 + track) * 10 + (sect - 1)) * 512,
            SEEK_SET);
            fwrite(buf, 512, 1, disk->fdisk);
            fflush(disk->fdisk);
           }
        else
           res = -1;
        break;
     case DISK_SS40S : // SS40S
     case DISK_S4S : // S4S
     case DISK_DS40S : // DS40S
     case DISK_D4S : // D4S
     case DISK_DS40 : // DS40
     case DISK_D40 : // D40
     case DISK_SS80 : // SS80
     case DISK_S80 : // S80
     case DISK_DS80 : // DS80 (Microbee modular)
     case DISK_D80 : // D80 (Microbee modular)
     case DISK_DS82 : // DS82
     case DISK_D82 : // D82
     case DISK_DS84 : // DS84
     case DISK_D84 : // D84
     case DISK_DS8B : // DS8B (Microbee Beeboard)
     case DISK_D8B : // D8B (Microbee Beeboard)
     case DISK_HD0 : // HDD (10MB) 306 cylinders, 4 heads, 17 sect/track
     case DISK_HD1 : // HDD (10MB) 80 cylinders, 4 heads, 63 sect/track
     case DISK_HD2 : // HDD (8MB) 246 cylinders, 2 heads, 32 sect/track (i.e GB's CF8)
     case DISK_HD3 : // HDD (32MB) 490 cylinders, 4 heads, 32 sect/track (i.e GB's CF8)
     case DISK_HDD : // HDD Dynamic CHS naming. i.e. filename.hdd-490-4-32[-512]
     case DISK_FDD : // FDD Dynamic CHS naming. i.e. filename.fdd-80-2-10[-512]
        trackofs = track * disk->imagerec.heads * disk->imagerec.sectrack *
        disk->imagerec.secsize;
        sectofs = side * disk->imagerec.sectrack * disk->imagerec.secsize;

        if (track >= disk->imagerec.datatrack)
           sectuse = sect - disk->imagerec.datasecofs;
        else
           sectuse = sect - disk->imagerec.systsecofs;

        sectofs += sectuse * disk->imagerec.secsize;

        if ((sectuse >= 0) && (sectuse < disk->imagerec.sectrack))
           {
            fseek(disk->fdisk, trackofs + sectofs, SEEK_SET);
            fwrite(buf, disk->imagerec.secsize, 1, disk->fdisk);
            fflush(disk->fdisk);
           }
        else
           res = -1;
        break;
#ifdef USE_LIBDSK
     case DISK_LIBDSK : // LibDsk
        // handle special formats while on the go where required
        disk_modify(disk, track);

        // handle rcpmfs type if active or return same sector
        if ((sect = rcpmfs_revskew(disk, "write", side, track, sect)) == -1)
           return -1;

        dsk_err = dsk_xwrite(disk->self, &disk->dg, buf, track, side, track,
        idside, sect, disk->secsize, 0);

        if (dsk_err == DSK_ERR_NOTIMPL)
           dsk_err = dsk_pwrite(disk->self, &disk->dg, buf, track, side, sect);

        if (dsk_err != DSK_ERR_OK)
           res = -1;
        break;
#endif
    }

 // check if there were any errors, if so report and exit
 if (res)
    {
#ifdef USE_LIBDSK
     if (disk->itype == DISK_LIBDSK)
        {
         // don't report multi sector error
         if ((dsk_err != DSK_ERR_NODATA) && (wtype != 'm'))
            {
             xprintf("disk_write: dsk_xwrite");
             xprintf(" error: file=%s dsk_err=%d %s\n",
                     disk->filepath, dsk_err, dsk_strerror(dsk_err));
             xprintf("Track: %3d   Side: %3d   Sector: %3d   IDside: %3d\n",
                     track, side, sect, idside);
            }
        }
     else
#endif
        {
         if (wtype != 'm') // don't report multi sector error
            {
             xprintf("disk_write: (inbuilt) file=%s res=%d\n",
                     disk->filepath, res);
             xprintf("Track: %3d   Side: %3d   Sector: %3d\n",
                     track, side, sect);
            }
        }

     return res;
    }

 return 0;
}

//==============================================================================
// Disk read ID field.
//
// Fills the read_addr_t structure id with the ID field.  For internal non
// DSK drivers the structure is set to the passed side and track values and
// values from the disk->imagerec structure.
//
// CPCEMU DSK formats can use different size sectors but must be the same for
// the entire track and the total size of the track must be the same for all
// tracks on the disk.  This means 512x10 and 1024x5 tracks work ok.
//
//   pass: disk_t *disk
//         read_addr_t *idfield
//         int side
//         int track
// return: int                          0 if no errors, else error number
//==============================================================================
int disk_read_idfield (disk_t *disk, read_addr_t *idfield, int side, int track)
{
 dskt_t dskt;                   // dsk track information block

 int res = 0;

#ifdef USE_LIBDSK
 dsk_err_t dsk_err;
 DSK_FORMAT result;
#endif

 int trackofs;
 int sectofs;
 int dskofs;

 switch (disk->itype)
    {
     case DISK_DSK : // if DSK format (not LibDsk)
        trackofs = track * disk->imagerec.heads * disk->imagerec.sectrack *
        disk->imagerec.secsize;
        dskofs = (((track * disk->imagerec.heads) + side + 1) * 0x100) + 0x100;

        // offset calculation when side 1 of disk accessed
        sectofs = side * disk->imagerec.sectrack * disk->imagerec.secsize;

        // read the track information header for the track
        fseek(disk->fdisk, trackofs + sectofs + dskofs - 0x100, SEEK_SET);

        if (fread(&dskt, sizeof(dskt), 1, disk->fdisk) != 1)
           {
            res = -1;
            break;
           }

        disk->imagerec.sectrack = dskt.spt;
        disk->imagerec.secsize = 128 << dskt.bps; // 0=128, 1=256, 2=512, 3=1024

        idfield->track = dskt.track_numb;
        idfield->side = dskt.hnumb_sectid;
        idfield->secaddr = dskt.sect_numb;
        idfield->seclen = get_psh(disk->imagerec.secsize);

        idfield->crc1 = dskt.state1_errcode;
        idfield->crc2 = dskt.state2_errcode;

#if 0
        printf("side=%d track=%d idfield->track=%d idfield->side=%d idfield->seclen=%d"
        " idfield->secaddr=%d\n",
        side, track,
        idfield->track, idfield->side, idfield->seclen,
        idfield->secaddr);
#endif
        break;
#ifdef USE_LIBDSK
     case DISK_LIBDSK : // LibDsk

        // get the side information from first available sector header ID
        dsk_err = dsk_psecid(disk->self, &disk->dg, track, side, &result);

        // handle special formats while on the go where required
        disk_modify(disk, track);

        if (dsk_err == DSK_ERR_OK)
           {
            idfield->track = result.fmt_cylinder;
            idfield->side = result.fmt_head;
            idfield->seclen = get_psh(result.fmt_secsize);
            idfield->secaddr = result.fmt_sector;
            // should update disk->imagerec.sectrack too, but how (but don't
            // need to ??)
            disk->imagerec.secsize = result.fmt_secsize;
           }
        else  // else if DSK_ERR_NOTIMPL or read error
           {
            idfield->track = track;
            idfield->side = side;
            idfield->seclen = get_psh(disk->imagerec.secsize);

            if (track >= disk->imagerec.datatrack)
               idfield->secaddr = disk->imagerec.datasecofs;
            else
               idfield->secaddr = disk->imagerec.systsecofs;
           }

        idfield->crc1 = 0xff;
        idfield->crc2 = 0xff;
        res = DSK_ERR_OK;

#if 0        
        printf("side=%d track=%d idfield->track=%d idfield->side=%d idfield->seclen=%d"
        " idfield->secaddr=%d\n",
        side, track,
        idfield->track, idfield->side, idfield->seclen,
        idfield->secaddr);
#endif
        break;
#endif
     default : // internal raw and DIP formats
        idfield->track = track;

        idfield->side = side;

        // if a Dreamdisk double sided disk then set the 'double sided' flag
        // FIXME - this should only apply to real Dreamdisk images.
        if ((modelx.fdc == MODFDC_DD) && (disk->imagerec.heads == 2))
           idfield->side |= 0x80;

        idfield->seclen = get_psh(disk->imagerec.secsize);
        if (track >= disk->imagerec.datatrack)
           idfield->secaddr = disk->imagerec.datasecofs;
        else
           idfield->secaddr = disk->imagerec.systsecofs;
        idfield->crc1 = 0xff;
        idfield->crc2 = 0xff;

        break;
    }

 return res;
}

//==============================================================================
// Disk write protected query.
//
//   pass: disk_t *disk
// return: int                          write protect status
//==============================================================================
int disk_iswrprot (disk_t *disk)
{
#ifdef USE_LIBDSK
 dsk_err_t dsk_err;
 unsigned char result;
#endif

 switch (disk->itype)
    {
#ifdef USE_LIBDSK
     case DISK_LIBDSK : // LibDsk
        dsk_err = dsk_drive_status(disk->self, &disk->dg, 0, &result);

        if (dsk_err == DSK_ERR_NOTIMPL)
           break;  // will return the current state of disk->wrprot

        disk->wrprot = (result & DSK_ST3_RO);

        if (dsk_err)
           xprintf("disk_iswrprot: dsk_drive_status error: "
           "file=%s dsk_err=%d %s\n",
           disk->filepath, dsk_err, dsk_strerror(dsk_err));
        break;
#endif
     default : // internal formats
        break;
    }

 return disk->wrprot;
}

//==============================================================================
// Disk set geometry member.
//
//   pass: disk_t *disk
// return: void
//==============================================================================
int disk_setgeom_member (disk_t *disk)
{
 return 0;
}

//==============================================================================
// Disk format track.
//
//   pass: disk_t *disk
//         int side
//         char *buf
//         int ddense                   MFM=1, FM=0
//         int track
//         int side
//         int sectors
// return: int                          0 if no error
//==============================================================================
int disk_format_track (disk_t *disk, char *buf, int ddense, int track,
    int side, int sectors)
{
 int res;

#ifdef USE_LIBDSK
 dsk_err_t dsk_err;
 static DSK_GEOMETRY geom;
 static DSK_FORMAT format[128]; // up to 128 sector headers
 int i;
 int s;
#endif

 // reset the exit seconds counter to a new minimum value every time we write
 // to disk and emu.secs_exit is not zero.
 if ((emu.secs_exit) && ((emu.secs_run + 3) >= emu.secs_exit))
    emu.secs_exit = emu.secs_run + 3;

 switch (disk->itype)
    {
#ifdef USE_LIBDSK
     case DISK_LIBDSK : // LibDsk
        memcpy(&geom, &disk->dg, sizeof(geom));
        geom.dg_fm = (ddense == 0);
        i = 0;
        for (s = 0; s < sectors; s++)
           {
            format[s].fmt_cylinder = buf[i++];
            format[s].fmt_head = buf[i++];
            format[s].fmt_sector = buf[i++];
            format[s].fmt_secsize = buf[i++] << 8;
           }
        dsk_err = dsk_pformat(disk->self, &geom, track, side, format, 0xe5);

        if (dsk_err)
           xprintf("disk_format_track: dsk_pformat error: "
           "file=%s dsk_err=%d %s\n",
           disk->filepath, dsk_err, dsk_strerror(dsk_err));
        res = dsk_err;
        break;
#endif
     default : // internal formats
        res = 0;
        break;
    }

 return res;
}
