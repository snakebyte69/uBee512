/* support Header */

#ifndef HEADER_SUPPORT_H
#define HEADER_SUPPORT_H

#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include "z80.h"

#define le16_to_host little_endian_16
#define host_to_le16 little_endian_16
#define leu16_to_host little_endian_u16
#define host_to_leu16 little_endian_u16
#define le32_to_host little_endian_32
#define host_to_le32 little_endian_32
#define leu32_to_host little_endian_u32
#define host_to_leu32 little_endian_u32
#define le64_to_host little_endian_64
#define host_to_le64 little_endian_64
#define leu64_to_host little_endian_u64
#define host_to_leu64 little_endian_u64

typedef struct sup_args_t
{
 const char *name;
 int value;
}sup_args_t;

typedef union sup_fp_t
{
 uint64_t i;                 // forces 8 bytes of storage
 FILE *p;                    // store a 4 or 8 byte pointer
 DIR *d;
}sup_fp_t;

typedef struct sup_file_t
{
 int16_t res;                // result
 sup_fp_t fp;
 int16_t val1;               // value 1
 int16_t val2;               // value 2
 char *dpn;                  // pass directory path/name
 char *fnwc;                 // return file name or wildcards
 char *mfp;                  // return modified file path here
 char *fpfnm;                // return full path and file name match
 char filepath[256];
 char filename[256];
}sup_file_t;

typedef struct basicver_t
{
 char id[5];
 int addr;
}basicver_t;

char *sup_strncpy (char *d, const char *s, int size);
int time_get_secs (void);
uint64_t time_get_ms (void);
void time_delay_ms (int ms);
void time_wait_ms (int ms);
void get_date_and_time (char *s);
uint8_t *get_z80mem_ptr_and_addr (int *addr);
int get_integer_value (char *s);
float get_float_value (char *s);
int get_parameter_count (char *s, int delimiter);
char *get_next_parameter (char *s, int delimiter, char *sps, int *spi, int maxlen);
int get_psh (int value);
char *i2bx (int value, int bits, char *s);
char *i2b (int value, char *s);
void toupper_string (char *dest, char *src);
void tolower_string (char *dest, char *src);
int file_readline (FILE *fp, char *s, int size);
int copy_file (char *dest, char *src);
void convert_slash (char *path);
FILE *open_file (const char *path1, const char *path2, char *path3, const char *mode);
FILE *test_file (const char *path1, const char *path2, char *path3);
int string_search (char *strg_array[], char *strg_find);
int string_case_search (char *strg_array[], char *strg_find);
int string_struct_search (sup_args_t *args, char *strg_find);
int array_search (uint8_t *s1, uint8_t *s2, int start, int finish, int size, int any);
int string_prefix_get (char *strg_scan, char *strg, int position, int maxlen);
int find_file_entry (char *filename, char *strg_search, char *strg_value);
int find_md5_file_entry (char *filename, char *strg_search, char *strg_value);
int find_file_alias (char *alias_filename, char *strg_search, char *strg_value);
void create_md5 (char *filename, char *md5);
int get_mwb_version (int msg, char *vers);

#ifdef MINGW
char *strcasestr (char *haystack, char *needle);
int strcasecmp (const char *s1, const char *s2);
#endif

int xstrverscmp (const char *s1, const char *s2);
int IsBigEndian (void);
void swap_endian (unsigned char *na, int size);

int16_t little_endian_16 (int16_t n);
uint16_t little_endian_u16 (uint16_t n);
int32_t little_endian_32 (int32_t n);
uint32_t little_endian_u32 (uint32_t n);
int64_t little_endian_64 (int64_t n);
uint64_t little_endian_u64 (uint64_t n);

int16_t big_endian_16 (int16_t n);
uint16_t big_endian_u16 (uint16_t n);
int32_t big_endian_32 (int32_t n);
uint32_t big_endian_u32 (uint32_t n);
int64_t big_endian_64 (int64_t n);
uint64_t big_endian_u64 (uint64_t n);

int asterisk (char **wildcard, char **test);
int wildcardfit (char *wildcard, char *test);
int set (char **wildcard, char **test);

void sup_opendir (sup_file_t *f);
void sup_readdir (sup_file_t *f);
   
#endif     /* HEADER_SUPPORT_H */
