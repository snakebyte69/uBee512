/* CLOCK Header */

#ifndef HEADER_CLOCK_H
#define HEADER_CLOCK_H

#include "z80.h"

int clock_init (void);
int clock_deinit (void);
int clock_reset (void);

uint16_t clock_r (uint16_t port, struct z80_port_read *port_s);

#endif     /* HEADER_CLOCK_H */
