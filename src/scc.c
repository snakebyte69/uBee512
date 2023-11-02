//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*               Serial Communications Controller (SCC) module                *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulates the Serial Communications Controller (SCC) module
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
// v4.0.0 - 13 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
//
// v2.3.0 - 18 December 2007, uBee
// - Created a new file to implement the Z80 SCC emulation.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ubee512.h"
#include "scc.h"
#include "z80.h"

#include "macros.h"
//==============================================================================
// structures and variables
//==============================================================================

//==============================================================================
// SCC initialise
//
//   pass: void
// return: int                          0
//==============================================================================
int scc_init (void)
{
 return 0;
}

//==============================================================================
// SCC de-initialise
//
//   pass: void
// return: int                          0
//==============================================================================
int scc_deinit (void)
{
 return 0;
}

//==============================================================================
// SCC reset
//
//   pass: void
// return: int                          0
//==============================================================================
int scc_reset (void)
{
 return 0;
}

//==============================================================================
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t                     0
//==============================================================================
uint16_t scc_r (uint16_t port, struct z80_port_read *port_s)
{
 return 0;
}

//==============================================================================
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void scc_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
}
