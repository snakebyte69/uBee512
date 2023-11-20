/* function Header */

#ifndef HEADER_FUNCTION_H
#define HEADER_FUNCTION_H

#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include "z80.h"

int function_init (void);
int function_deinit (void);
int function_reset (void);

uint16_t function_ubee_r (uint16_t port, struct z80_port_read *port_s);
void function_ubee_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

#define FILE_STR_SIZE      256
#define FILE_LIST_ENTRIES  128

#pragma pack(push, 1)  // push current alignment, alignment to 1 byte boundary

typedef union fp_t
   {
    uint64_t i;                         // forces 8 bytes of storage
    FILE *p;                            // store a 4 or 8 byte pointer
    DIR *d;
   }fp_t;

// version informnation response structure
typedef struct ub_version_t
   {
    uint16_t cmd;                       // sub command
    uint16_t ver1;                      // vers 1.x.x value
    uint16_t ver2;                      // vers x.1.x value
    uint16_t ver3;                      // vers x.x.1 value
   }ub_version_t;

// dump memory structure
typedef struct ub_dump_t
   {
    uint16_t cmd;                       // sub command
    uint16_t id;                        // 0xAA55 check ID
    uint16_t addr;                      // dump address
    uint16_t lines;                     // lines
    uint16_t htype;                     // header type
   }ub_dump_t;

// command and response structure
typedef struct ub_cmdres_t
   {
    uint16_t cmd;                       // sub command
    uint16_t id;                        // 0xAA55 check ID
    int16_t res;                        // result
    fp_t fp;
   }ub_cmdres_t;

// file structure
typedef struct ub_file_t
   {
    uint16_t cmd;                       // sub command
    uint16_t id;                        // 0xAA55 check ID
    int16_t res;                        // result
    fp_t fp;
    uint16_t addr1;                     // address 1 in Z80 map
    uint16_t addr2;                     // address 2 in Z80 map
    uint16_t addr3;                     // address 3 in Z80 map
    uint16_t addr4;                     // address 4 in Z80 map
    int16_t size;                       // size of object
    uint16_t num;                       // number of chars
    int16_t val1;                       // value 1
    int16_t val2;                       // value 2
   }ub_file_t;

// stdio input commands
typedef struct ub_stdio_input_t
   {
    uint16_t cmd;                       // sub command
    int16_t res;                        // result
   }ub_stdio_input_t;

// stdio character write commands
typedef struct ub_stdio_char_t
   {
    uint16_t cmd;                       // sub command
    int16_t res;                        // result
    uint16_t val;                       // value
   }ub_stdio_char_t;

// stdio string commands
typedef struct ub_stdio_str_t
   {
    uint16_t cmd;                       // sub command
    int16_t res;                        // result
    uint16_t addr;                      // address in Z80 map
   }ub_stdio_str_t;

typedef union ub_func_u
   {
    ub_version_t version;
    ub_dump_t dump;
    ub_stdio_input_t getchar;
    ub_stdio_char_t putchar;
    ub_stdio_str_t puts;
    ub_cmdres_t cmdres;
    ub_file_t file;
   }ub_func_u;

#pragma pack(pop)       // restore original alignment from stack

typedef struct func_t
   {
    int dump_addr;
    char file_list[FILE_LIST_ENTRIES][FILE_STR_SIZE];
    char file_run[FILE_STR_SIZE];
    char file_app[FILE_STR_SIZE];
    int file_load;
    int file_exec;
    int file_exit;
    int file_list_pos;
    int file_list_count;
   }func_t;

#endif     /* HEADER_FUNCTION_H */
