/* Asynchronous Header */

#ifndef HEADER_ASYNC_H
#define HEADER_ASYNC_H

#include <inttypes.h>
#include "z80.h"

#ifdef MINGW
typedef HANDLE deschand_t;
#define NIL 0
#define SYST_RX_BUFSIZE 0x4000
#define SYST_TX_BUFSIZE 0x1000
#else
typedef int deschand_t;
#endif

deschand_t async_open (char *device);
int async_close (deschand_t fd);
int async_configure (deschand_t fd, int baudtx, int baudrx, int data, int stop, int hw);
int async_read (deschand_t fd);
int async_write (deschand_t fd, uint8_t ch);
void async_write_break (deschand_t fd);

#endif     /* HEADER_ASYNC_H */
