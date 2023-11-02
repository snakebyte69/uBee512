/* quickload Header */

#ifndef HEADER_QUICKLOAD_H
#define HEADER_QUICKLOAD_H

int quickload_init (void);
int quickload_deinit (void);
int quickload_reset (void);

int quickload_load (char *p);
int quickload_list (char *p);
int quickload_execute (void);
#ifdef USE_ZZLIB
int quickload_load_arc (char *p);
int quickload_list_arc (char *p);
int quickload_dir_arc (char *p);
int quickload_open_arc (char *p);
#endif

#define QUICKLOAD_MAX_DESC_SIZE 1024

// Quickload file structure:
// First 7 bytes are part of the original quickload format when used with
// QBASIC, these are not used here.  The first 5 bytes never change.

#pragma pack(push, 1)  // push current alignment, alignment to 1 byte boundary

typedef struct quickload_qb_t
{
 uint8_t flag;       // 0xfd - qbasic flag
 uint16_t load_seg;  // 0x00 0x80 - segment to load to
 uint16_t load_off;  // 0x00 0x00 - offset to load to
 uint16_t file_size; // 0x12 0x34 - file size, this is (total size-7)
}quickload_qb_t;

// A Quickload header follows, firstly, a variable-size string containing
// the program name and any notes. 0x00 bytes should be skipped, the string
// is terminated with a 0x1a byte.  All values are in Little-endian format.

typedef struct quickload_hd_t
{
 uint16_t exec_addr; // exec address of program
 uint16_t load_addr; // load address
 uint16_t end_addr;  // end address
}quickload_hd_t;

#pragma pack(pop)       // restore original alignment from stack

typedef struct quickload_t
{
 char filename[512]; // file name string
 char desc[QUICKLOAD_MAX_DESC_SIZE];  // description string
 int prog_seek; // offset to the program data seek position
 int prog_size; // size of the program
 int exec_addr; // exec address of program
 int load_addr; // load address
 int end_addr;  // end address
}quickload_t;

#endif  /* HEADER_QUICKLOAD_H */
