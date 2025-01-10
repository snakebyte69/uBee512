/* Floppy Disk Support Header */

#ifndef HEADER_DISK_H
#define HEADER_DISK_H

#include <inttypes.h>

#ifdef USE_LIBDSK
#include <libdsk.h>
#endif

#include "ubee512.h"

// disk image types, do not change the order of these!
enum
{
 // variable sized disks and formats
 DISK_DIP = 1,
 DISK_DSK,

 // Microbee Nanowasp 400kb disks
 DISK_IMG,
 DISK_NW,

 // Microbee early disk formats (single density)
 DISK_SS40S,
 DISK_S4S,
 DISK_DS40S,
 DISK_D4S,

 // Microbee 400kb disks
 DISK_DS40,
 DISK_D40,
 DISK_SS80,
 DISK_S80,

 // Microbee 800kb disks
 DISK_DS80,
 DISK_D80,
 DISK_DS82,
 DISK_D82,
 DISK_DS84,
 DISK_D84,
 DISK_DS8B,
 DISK_D8B,

 // Microbee 10Mb HDD disks
 DISK_HD0,
 DISK_HD1,

 // GB's CF8 HDD disks
 DISK_HD2,
 DISK_HD3,
 
 // Dynamic HDD and FDD named disks
 DISK_HDD,
 DISK_FDD, 
 
 DISK_LIBDSK
};

// disk errors
enum
{
 DISK_ERR_NONE,
 DISK_ERR_NOTFOUND,
 DISK_ERR_READONLY
};

// disk attributes (data rate, density)
#define DISK_DENSITY_SINGLE     0
#define DISK_DENSITY_DOUBLE     1
#define DISK_RATE_250KBPS       0
#define DISK_RATE_500KBPS       1

// Disk image record information (adjust filler for 512 bytes)
// The data shall be in Little Endian format when stored as a file.
typedef struct diski_t
{
 char id[32];           // disk image ID
 char ver[16];          // disk image version
 char type[32];         // disk image type

 uint16_t wrprot;       // write protected
 uint16_t tracks;       // number of tracks
 uint16_t heads;        // number of heads (sides)
 uint16_t secsize;      // bytes per sector
 uint16_t sectrack;     // sectors per track
 uint16_t datatrack;    // start of data tracks (after system)
 uint16_t systsecofs;   // offset to apply to system sectors
 uint16_t datasecofs;   // offset to apply to data sectors
 uint8_t skewsa[3];     // skew algorithm info for system tracks
 uint8_t skewda[3];     // skew algorithm info for data tracks
 uint8_t skewsd[128];   // skew data for system tracks
 uint8_t skewdd[128];   // skew data for data tracks
 uint16_t secterrs;     // size of sector error table (0 if not used)
 char fill[152];        // filler to make structure 512 bytes long
}diski_t;

typedef struct disk_t
{
 FILE *fdisk;
 int wrprot;
 int itype;             // image type
 int drive;             // drive number (0-3)
 int error;
 int secsize;           // sector size
 diski_t imagerec;
 char filename[512];    // requested file
 char filepath[512];    // full path to requested file
 char image_name[64];   // image name type
 char density;          // single or double density
 char datarate;         // 250 or 500kbps.
#ifdef USE_LIBDSK
 int side1as0;
 int dstep;
 int dstep_hd;
 int have_rcpmfs;       // recognised format and 'rcpmfs' in use
 int cpm3;              // CP/M 3 flag for use by 'rcpmfs'
 char libdsk_type[40];
 char libdsk_format[40];
 DSK_PDRIVER self;
 DSK_GEOMETRY dg;
#endif
}disk_t;

// dsk disk structure, first 0x100 bytes of image is the disk header
// information block.
typedef struct dski_t
{
 char cpcemu[0x22];             // 0x00 - 0x21
 char unused1[0x0e];            // 0x22 - 0x2f
 uint8_t tracks;                // 0x30
 uint8_t heads;                 // 0x31
 uint8_t size_one_ta;           // 0x32
 uint8_t size_one_tb;           // 0x33
 char unused2[0x100-0x34];      // 0x34 - 0xff
}dski_t;

// dsk disk structure, track information block. We use this to figure out
// the SPT and BPS for all other tracks on the disk.
typedef struct dskt_t
{
 char track_info[0x0d];         // 0x00 - 0x0c
 char unused3[0x03];            // 0x0d - 0x0f
 uint8_t tracks_n;              // 0x10
 uint8_t heads_n;               // 0x11
 char unused4[0x02];            // 0x12 - 0x13
 uint8_t bps;                   // 0x14
 uint8_t spt;                   // 0x15
 uint8_t gap3;                  // 0x16
 uint8_t filling_byte;          // 0x17

 uint8_t track_numb;            // 0x18
 uint8_t hnumb_sectid;          // 0x19
 uint8_t sect_numb;             // 0x1a
 uint8_t bps_x;                 // 0x1b
 uint8_t state1_errcode;        // 0x1c
 uint8_t state2_errcode;        // 0x1d
 char unused5[0x02];            // 0x1e - 0x1f

 char unused6[0x100-0x20];      // 0x20 - 0xff
}dskt_t;

typedef struct read_addr_t
{
 uint8_t track;
 uint8_t side;
 uint8_t secaddr;
 uint8_t seclen;
 uint8_t crc1;
 uint8_t crc2;
}read_addr_t;

// reverse CP/M file system values for various disk formats needed by the
// libdsk 'rcpmfs' type
typedef struct rcpmfs_args_t
{
 const char *name;
 int block_size;
 int dir_blocks;
 int total_blocks;
 int sys_tracks;
}rcpmfs_args_t;

int disk_init (void);
int disk_open (disk_t *disk);
void disk_close (disk_t *disk);
int disk_create (disk_t *disk, int temp_only);
int disk_read (disk_t *disk, char *buf, int side, int idside, int track,
               int sect, char rtype);
int disk_write (disk_t *disk, char *buf, int side, int idside, int track,
               int sect, char wtype);
int disk_read_idfield (disk_t *disk, read_addr_t *idfield, int side, int track);
int disk_format_track (disk_t *disk, char *buf, int ddense, int track,
                       int side, int sectors);
int disk_iswrprot (disk_t *disk);

#endif  /* HEADER_DISK_H */
