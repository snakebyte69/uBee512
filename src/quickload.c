//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              quickload module                              *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Quickload mechanism for 8-bit systems.
//
// Quickload provides a method to quickly load programs using the quickload
// file format into memory on SRAM (ROM) based models instead of using the
// emulated tape loading method.
//
// zziplib library support must be part of the build for quickload archived
// file support.  Some sections of this code are based on unzip-mem.c from
// the zziplib package.
//
// Only systems that have v5.xx Microworld Basic currently in memory will be
// allowed to load and execute quickload files.
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
// v5.3.0 - 7 April 2011, uBee
// - Moved get_mwb_version() to support.c as is now also used by tapfile.c.
// - Calls to get_mwb_version() now includes a 2nd parameter to point to a
//   string array to return the Basic version, NULL used as is not required.
//
// v4.7.0 - 29 June 2010, uBee
// - Changes made to fread() function to use the result as some compilers
//   report warning: declared with attribute warn_unused_result.
//
// v4.6.0 - 22 May 2010, uBee
// - Created a new file to implement the quickload support.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#ifdef USE_ZZLIB
#include <zlib.h>
#include <zzip/memdisk.h>
#include <time.h>
#endif

#include "ubee512.h"
#include "quickload.h"
#include "z80api.h"
#include "z80.h"
#include "support.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
quickload_t quickload_exec;

char quickload_header[7 + QUICKLOAD_MAX_DESC_SIZE + 6];

#ifdef USE_ZZLIB
static ZZIP_MEM_DISK *disk;
static char archive[SSIZE1];
static char time_str[30];

static zzip_off_t sum_usize;
static zzip_off_t sum_csize;
static zzip_off_t sum_files;
#endif

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;

//==============================================================================
// quickload initialise.
//
//   pass: void
// return: int                          0
//==============================================================================
int quickload_init (void)
{
 return 0;
}

//==============================================================================
// quickload de-initialise.
//
//   pass: void
// return: int                          0
//==============================================================================
int quickload_deinit (void)
{
 // should close any open files here!
 return 0;
}

//==============================================================================
// quickload reset.
//
//   pass: void
// return: int                          0
//==============================================================================
int quickload_reset (void)
{
 return 0;
}

//==============================================================================
// Report the quickload header values.
//
//   pass: quickload_t *quickload
// return: void
//==============================================================================
static void report_header_values (quickload_t *quickload)
{
 xprintf("Quickload file: %s\n", quickload->filename);
 xprintf("Description: %s\n", quickload->desc);
 xprintf("Load address: %04x  End address: %04x  Exec address: %04x\n",
 quickload->load_addr, quickload->end_addr, quickload->exec_addr);
}

//==============================================================================
// Get the quickload header values.
//
//   pass: unsigned char *s             pointer to the quickload header data
//         quickload_t *quickload
// return: int                          0 if no error else -1
//==============================================================================
static int get_header_values (unsigned char *s, quickload_t *quickload)
{
 int i = 0;
 int x = 0;

 quickload_qb_t *quickload_qb = (void *)s;
 quickload_hd_t *quickload_hd;

 if ((quickload_qb->flag != 0xfd) || (leu16_to_host(quickload_qb->load_seg) != 0x8000)
 || (leu16_to_host(quickload_qb->load_off) != 0x0000))
    {
     strcpy(quickload->desc, "Not a recognised quickload file format.");
     return -1;
    }

 s += 7;  // get the file description (terminated with 0x1a)
 while ((*s != 0x1a) && (x < QUICKLOAD_MAX_DESC_SIZE-1))
    {
     if (*s != 0)
        quickload->desc[i++] = *s;
     s++;
     x++;
    }
 quickload->desc[i++] = 0;
 x++;

 if (*s != 0x1a)
    {
     strcpy(quickload->desc, "Quickload file description is too long.");
     return -1;
    }

 quickload_hd = (void *)s + 1;
 quickload->prog_seek = 7 + x + 6;  // offset to the program data seek position
 quickload->prog_size = leu16_to_host(quickload_qb->file_size) - x - 6;  // program size
 quickload->exec_addr = leu16_to_host(quickload_hd->exec_addr); // exec address of program
 quickload->load_addr = leu16_to_host(quickload_hd->load_addr); // load address
 quickload->end_addr = leu16_to_host(quickload_hd->end_addr);   // end address

 return 0;
}

//==============================================================================
// Prime a quickload file ready to be executed.
//
// Scratchpad 0xa2: Warm start jump address LSB+1.
// Scratchpad 0xa6: Machine language EXEC address.
//
// Scratchpad 0xa2 contains 0x8517 by default which is the 'Monitor to BASIC
// warm start' vector in Basic.
//
//   pass: int autorun
//         quickload_t *quickload
// return: int                          0 if no error else -1
//==============================================================================
static int prime_quickload (int autorun, quickload_t *quickload)
{
 if (! quickload->filename[0])
    {
     xprintf("No quickload file loaded into memory.\n");
     return -1;
    }

 z80api_write_mem(0x00a6, quickload->exec_addr & 0x00ff);
 z80api_write_mem(0x00a7, quickload->exec_addr >> 8);
 if (autorun)
    {
     z80api_write_mem(0x00a2, quickload->exec_addr & 0x00ff);
     z80api_write_mem(0x00a3, quickload->exec_addr >> 8);
     emu.new_pc = quickload->exec_addr;
    }
 else
    {
     // put back the normal default value
     z80api_write_mem(0x00a2, 0x8517 & 0x00ff);
     z80api_write_mem(0x00a3, 0x8517 >> 8);
    }

 return 0;
}

//==============================================================================
// Load a quickload file.
//
// --ql-load=file[,x]
//
// Load a quickload file, an optional 'x' will cause the code to be executed
// once loaded.
//
//   pass: char *p                      parameter
// return: int                          0 if no error, -1 if CL error else 1
//==============================================================================
int quickload_load (char *p)
{
 FILE *fp;

 char *c;
 char sp[512];
 int temp;
 int count;
 int addr;
 int execute = 0;
 uint8_t data;

 if (get_mwb_version(1, NULL) == -1)
    return 1;

 c = get_next_parameter(p, ',', quickload_exec.filename, &temp, sizeof(quickload_exec.filename)-1);
 fp = fopen(quickload_exec.filename, "rb");
 if (fp == NULL)
    {
     xprintf("Unable to open file: %s\n", quickload_exec.filename);
     quickload_exec.filename[0] = 0;
     return -1;
    }

 c = get_next_parameter(c, ',', sp, &temp, sizeof(sp)-1);

 if (sp[0])
    {
     if (strcasecmp(sp, "x") == 0)
        execute = 1;
     else
        {
         fclose(fp);
         return -1;
        }
    }

 if (fread(quickload_header, sizeof(quickload_header), 1, fp) != 1)
    {
     xprintf("Unable to read quickload header from %s\n", quickload_exec.filename);
     fclose(fp);
     return 1;
    }

 if (get_header_values((unsigned char *)quickload_header, &quickload_exec) == -1)
    {
     xprintf("%s\n", quickload_exec.desc);
     fclose(fp);
     return 1;
    }

 fseek(fp, quickload_exec.prog_seek, SEEK_SET); // seek to the program data

 count = quickload_exec.prog_size;
 addr = quickload_exec.load_addr;

 while (count--)
    {
     if (fread(&data, 1, 1, fp) == 1)
        z80api_write_mem(addr++, data);
     else
        break;
     addr &= 0xffff;
    }

 fclose(fp);

 if (prime_quickload(execute, &quickload_exec) == -1)
    return 1;

 report_header_values(&quickload_exec);

 return 0;
}

//==============================================================================
// List description contained in a quickload file.
//
// --ql-list=file
//
//   pass: char *p                      parameter
// return: int                          0 if no error, -1 if CL error else 1
//==============================================================================
int quickload_list (char *p)
{
 quickload_t quickload;

 FILE *fp;

 char *c;
 int temp;

 c = get_next_parameter(p, ',', quickload.filename, &temp, sizeof(quickload.filename)-1);
 fp = fopen(quickload.filename, "rb");
 if (fp == NULL)
    {
     xprintf("Unable to open file: %s\n", quickload.filename);
     quickload.filename[0] = 0;
     return -1;
    }

 if (c != NULL)
    return -1;

 if (fread(quickload_header, sizeof(quickload_header), 1, fp) != 1)
    {
     xprintf("Unable to read quickload header from %s\n", quickload_exec.filename);
     fclose(fp);
     return 1;
    }

 fclose(fp);

 if (get_header_values((unsigned char *)quickload_header, &quickload) == -1)
    {
     xprintf("%s\n", quickload.desc);
     return 1;
    }

 report_header_values(&quickload);

 return 0;
}

//==============================================================================
// Execute the quickload file in memory.
//
// --ql-x
//
//   pass: void
// return: int                          0 if no error, -1 if CL error else 1
//==============================================================================
int quickload_execute (void)
{
 if (get_mwb_version(1, NULL) == -1)
    return 1;

 if (prime_quickload(1, &quickload_exec) == -1)
    return 1;

 return 0;
}

#ifdef USE_ARC
//==============================================================================
// Display header for ZIP listing.
//
//   pass: int verbose
// return: void
//==============================================================================
static void dir_entry_header (int verbose)
{
 sum_usize = 0;
 sum_csize = 0;
 sum_files = 0;

 if (verbose)
    {
     xprintf("  Length  Method     Size Ratio   Date   Time    CRC-32   Name\n");
     xprintf("  ------  ------     ---- -----   ----   ----    ------   ----\n");
    }
 else
    {
     xprintf("  Length    Date   Time   Name\n");
     xprintf("  ------    ----   ----   ----\n");
    }
}

//==============================================================================
// Display footer for ZIP listing.
//
//   pass: int verbose
// return: void
//==============================================================================
static void dir_entry_footer (int verbose)
{
 char exp = ' ';

 if (sum_usize / 1024 > 1024*1024*1024)
    {
     exp = 'G';
     sum_usize /= 1024*1024*1024;
     sum_usize /= 1024*1024*1024;
    }

 if (sum_usize > 1024*1024*1024)
    {
     exp = 'M';
     sum_usize /= 1024*1024;
     sum_csize /= 1024*1024;
    }

 if (sum_usize > 1024*1024)
    {
     exp = 'K';
     sum_usize /= 1024;
     sum_csize /= 1024;
    }

 if (verbose)
    {
     xprintf("  ------             ---- -----                           ----\n");
     xprintf("%8li%c        %8li%c%3li%%                       %8li %s\n",
     (long)sum_usize, exp, (long)sum_csize, exp, (long)(100 - (sum_csize*100/sum_usize)),
     (long)sum_files, sum_files == 1 ? "file" : "files");
    }
 else
    {
     xprintf("  ------                  ----\n");
     xprintf("%8li%c            %8li %s\n", (long)sum_usize, exp, (long)sum_files,
     sum_files == 1 ? "file" : "files");
    }
}

//==============================================================================
// Display information for a file from the ZIP archive.
//
//   pass: ZZIP_MEM_ENTRY *entry
//         int verbose
// return: void
//==============================================================================
static void dir_entry (ZZIP_MEM_ENTRY *entry, int verbose)
{
 const char *comprlevel[] =
 {
  "stored",   "shrunk",   "redu:1",   "redu:2",   "redu:3",   "redu:4",
  "impl:N",   "toknze",   "defl:N",   "defl:B",   "impl:B"
 };

 char *name = zzip_mem_entry_to_name(entry);
 zzip_off_t usize = zzip_mem_entry_usize(entry);
 zzip_off_t csize = zzip_mem_entry_csize(entry);
 int compr = zzip_mem_entry_data_comprlevel(entry);
 time_t mtime = entry->zz_mktime;
 struct tm *date = localtime(&mtime);
 long crc32 = entry->zz_crc32;
 const char *comment = zzip_mem_entry_to_comment(entry);
 char exp = ' ';

 sum_usize += usize;
 sum_csize += csize;
 sum_files += 1;

 if (usize / 1024 > 1024*1024*1024)
    {
     exp = 'G';
     usize /= 1024*1024*1024;
     usize /= 1024*1024*1024;
    }

 if (usize > 1024*1024*1024)
    {
     exp = 'M';
     usize /= 1024*1024; csize /= 1024*1024;
    }

 if (usize > 1024*1024)
    {
     exp = 'K';
     usize /= 1024;
     csize /= 1024;
    }

 if (! comment)
    comment = "";
 if (*name == '\n')
    name++;

 sprintf(time_str, "%02i-%02i-%02i %02i:%02i", date->tm_mon, date->tm_mday,
 date->tm_year%100, date->tm_hour, date->tm_min);

 if (verbose)
    xprintf("%8li%c %s %8li%c%3li%%  %s  %08lx  %s %s\n", (long)usize, exp,
    comprlevel[compr], (long)csize, exp, (long)(100 - (csize*100/usize)),
    time_str, crc32, name, comment);
 else
    xprintf("%8li%c %s  %s %s\n", (long)usize, exp,
    time_str, name, comment);
}

//==============================================================================
// Display information for a quickload file from the ZIP archive.
//
//   pass: ZZIP_MEM_ENTRY *entry
// return: int
//==============================================================================
static int show_zip_entry (ZZIP_MEM_ENTRY *entry)
{
 ZZIP_DISK_FILE *file;

 quickload_t quickload;

 // open the file in the archive
 file = zzip_mem_entry_fopen(disk, entry);
 if (! file)
    {
     xprintf("Unable to open file: %s\n", zzip_mem_entry_to_name(entry));
     return -1;
    }

 zzip_mem_disk_fread(quickload_header, sizeof(quickload_header), 1, file);
 if (get_header_values((unsigned char *)quickload_header, &quickload) == -1)
    xprintf("%s: ERROR! %s\n", zzip_mem_entry_to_name(entry), quickload.desc);
 else
    xprintf("%s: %s\n", zzip_mem_entry_to_name(entry), quickload.desc);

 zzip_mem_disk_fclose(file);

 return 0;
}

//==============================================================================
// Load a quickload file from the current quickload archive.
//
// --qla-load=file[,x]
//
// Load file from the current quickload archive, an optional 'x' will cause
// the code to be executed once loaded.
//
//   pass: char *p                      parameter
// return: int                          0 if no error, -1 if CL error else 1
//==============================================================================
int quickload_load_arc (char *p)
{
 char *c;
 char sp[512];
 int temp;
 int count;
 int addr;
 int execute = 0;
 uint8_t data;

 char buffer[7 + QUICKLOAD_MAX_DESC_SIZE + 6];

 ZZIP_MEM_ENTRY *entry = 0;
 ZZIP_DISK_FILE *file;

 if (! disk)
    {
     xprintf("No ZIP archive is open.\n");
     return 1;
    }

 if (get_mwb_version(1, NULL) == -1)
    return 1;

 c = get_next_parameter(p, ',', quickload_exec.filename, &temp, sizeof(quickload_exec.filename)-1);

 if (! (entry = zzip_mem_disk_findmatch(disk, quickload_exec.filename, entry, 0, 0)))
    {
     xprintf("Unable to locate file: %s\n", quickload_exec.filename);
     quickload_exec.filename[0] = 0;
     return 1;
    }

 // display ZIP file information in short format
 dir_entry_header(0);
 dir_entry(entry, 0);

 // check for execution argument 'x' for the file
 c = get_next_parameter(c, ',', sp, &temp, sizeof(sp)-1);
 if (sp[0])
    {
     if (strcasecmp(sp, "x") == 0)
        execute = 1;
     else
        return -1;
    }

 // open the file in the archive
 file = zzip_mem_entry_fopen(disk, entry);
 if (! file)
    {
     xprintf("Unable to open file: %s\n", quickload_exec.filename);
     quickload_exec.filename[0] = 0;
     return 1;
    }

 zzip_mem_disk_fread(quickload_header, sizeof(quickload_header), 1, file);
 if (get_header_values((unsigned char *)quickload_header, &quickload_exec) == -1)
    {
     xprintf("%s\n", quickload_exec.desc);
     zzip_mem_disk_fclose(file);
     return 1;
    }

 // close then re-open and seek to the data position
 zzip_mem_disk_fclose(file);
 file = zzip_mem_entry_fopen(disk, entry);
 zzip_mem_disk_fread(buffer, quickload_exec.prog_seek, 1, file);

 count = quickload_exec.prog_size;
 addr = quickload_exec.load_addr;

 while (count--)
    {
     if (zzip_mem_disk_fread(&data, 1, 1, file) == 1)
        z80api_write_mem(addr++, data);
     else
        break;
     addr &= 0xffff;
    }

 zzip_mem_disk_fclose(file);

 if (prime_quickload(execute, &quickload_exec) == -1)
    return 1;

 xprintf("\n");
 report_header_values(&quickload_exec);

 return 0;
}

//==============================================================================
// List files and display descriptions for files contained in the archive.
//
// --qla-list=file|*
//
// The entire archive directory will be listed if '*' is specified for file
// or a single file within the archive matching 'file' may be specified.
//
//   pass: char *p                      parameter
// return: int                          0 if no error, -1 if CL error else 1
//==============================================================================
int quickload_list_arc (char *p)
{
 char *c;
 char filename[512];
 int temp;

 ZZIP_MEM_ENTRY *entry = 0;

 if (! disk)
    {
     xprintf("No ZIP archive is open.\n");
     return 1;
    }

 c = get_next_parameter(p, ',', filename, &temp, sizeof(filename)-1);

 if (c != NULL)
    return -1;

 if ((strcmp(filename, "*") == 0) || (strcmp(filename, "*.*") == 0))
    {
     entry = zzip_mem_disk_findfirst(disk);

     for (; entry ; entry = zzip_mem_disk_findnext(disk, entry))
        show_zip_entry(entry);
    }
 else
    {
     if ((entry = zzip_mem_disk_findmatch(disk, filename, entry, 0, 0)))
        show_zip_entry(entry);
    }

 return 0;
}

//==============================================================================
// Show a directory of file(s) with detailed archive file information.
//
// --qla-dir=file|*[,+v]
//
// The entire archive directory will be listed if '*' is specified for file
// or a single file within the archive matching 'file' may be specified.  An
// optional verbose argument of '+v' may be specified for more information.
//
//   pass: char *p                      parameter
// return: int                          0 if no error, -1 if CL error else 1
//==============================================================================
int quickload_dir_arc (char *p)
{
 char *c;
 char sp[512];
 char filename[512];
 int temp;
 int verbose = 0;

 ZZIP_MEM_ENTRY *entry = 0;

 if (! disk)
    {
     xprintf("No ZIP archive is open.\n");
     return 1;
    }

 c = get_next_parameter(p, ',', filename, &temp, sizeof(filename)-1);

 if (c != NULL)
    {
     c = get_next_parameter(c, ',', sp, &temp, sizeof(sp)-1);
     if (strcasecmp(sp, "+v") == 0)
        verbose++;
     else
        return -1;
    }

 if ((strcmp(filename, "*") == 0) || (strcmp(filename, "*.*") == 0))
    {
     dir_entry_header(verbose);
     entry = zzip_mem_disk_findfirst(disk);

     for (; entry ; entry = zzip_mem_disk_findnext(disk, entry))
         dir_entry(entry, verbose);

     dir_entry_footer(verbose);
    }
 else
    {
     if ((entry = zzip_mem_disk_findmatch(disk, filename, entry, 0, 0)))
        {
         dir_entry_header(verbose);
         dir_entry(entry, verbose);
        }
     else
        {
         xprintf("Unable to locate file: %s\n", filename);
         return 1;
        }
    }

 return 0;
}

//==============================================================================
// Open a quickload archive.
//
// --qla-arc=file
//
// Specify a quickload archive file to be used for further operations. Only
// ZIP archives are currently supported. Any archive currently open will be
// closed first.
//
//   pass: char *p                      parameter
// return: int                          0 if no error, -1 if CL error else 1
//==============================================================================
int quickload_open_arc (char *p)
{
 FILE *fp;
 uint8_t id[4];

 if (disk)
    {
     zzip_mem_disk_close(disk);
     disk = NULL;
    }

 strncpy(archive, p, sizeof(archive));
 archive[sizeof(archive)-1] = 0;

 fp = fopen(archive, "rb");
 if (! fp)
    return -1;

 if (fread(id, 4, 1, fp) < 1)
    {
     fclose(fp);
     return -1;
    }

 fclose(fp);

 if ((id[0] != 0x50) || (id[1] != 0x4b) || (id[2] != 0x03) || (id[3] != 0x04))
    return -1;

 disk = zzip_mem_disk_open(archive);
 if (! disk)
    return -1;

 return 0;
}
#endif
