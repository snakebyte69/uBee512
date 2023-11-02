//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              Printer module                                *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// This module is used to emulate the parallel printer port,  it sends data to
// an ASCII decimal and/or direct without any changes to files.
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
// v4.7.0 - 17 June 2010, K Duckmanton
// - Changes to allow several different devices to be connected to the
//   emulated parallel port
//
// v4.1.0 - 20 June 2009, uBee
// - Improvements made to printer strobe timing.
// - Printer now emulates strobe signal only if a printer file is specified.
//
// v4.0.0 - 13 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
//
// v3.1.0 - 5 November 2008, uBee
// - Changes made to printer_init() and printer_deinit() functions.
// - Added functions printer_a_close() and printer_b_close()
// - Added functions printer_a_open() and printer_b_open()
// - Moved variables to printer_t structure.
//
// v2.7.0 - 2 June 2008, uBee
// - New open_file() function is now used to create printer files.
//
// v2.6.0 - 11 May 2008, uBee
// - printer variables have been placed into structure printer_t.
//
// v2.2.0 - 15 November 2007, uBee
// - Make changes so that printer_strobe is still cleared even if no --print
//   options are specified.
//
// v1.3.0 - 29 August 2007, uBee
// - Created a new file and implement the printer emulation.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "ubee512.h"
#include "printer.h"
#include "support.h"
#include "z80api.h"
#include "parint.h"
#include "pio.h"


//==============================================================================
// constants
//==============================================================================
// time required for the printer to do something
#define PRINTER_PROCESSING_TIME (int)(0.25 * 3375)

//==============================================================================
// prototypes
//==============================================================================
int printer_reset (void);
int printer_init (void);
int printer_deinit (void);
void printer_a_close (void);
void printer_b_close (void);
int printer_a_open (char *s, int action);
int printer_b_open (char *s, int action);
void printer_strobe (void);
void printer_w (uint8_t data);
uint8_t printer_r(void);
void printer_ready(void);
void printer_poll(void);

//==============================================================================
// structures and variables
//==============================================================================
printer_t printer;

extern char userhome_prntpath[];

parint_ops_t printer_ops = {
    .init = &printer_init,
    .deinit = &printer_deinit,
    .reset = &printer_reset,
    .poll = &printer_poll,
    .ready = &printer_ready,
    .strobe = &pio_porta_strobe,
    .read = NULL,
    .write = &printer_w,
};

//==============================================================================
// Printer reset.
//
//   pass: void
// return: int                          0
//==============================================================================
int printer_reset (void)
{
 printer.busy = 0;
 printer.strobe_due = 0;
 return 0;
}

//==============================================================================
// Printer Initialise.
//
// Open the text and binary printer files for writing
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int printer_init (void)
{
 if (printer.printa[0])
    {
     if (printer_a_open(printer.printa, 1) == -1)
        return -1;
    }
 if (printer.printb[0])
    {
     if (printer_b_open(printer.printb, 1) == -1)
        return -1;
    }
 printer.strobe_due = 0;
 return 0;
}

//==============================================================================
// Printer de-initialise.
//
// Close all printer files
//
//   pass: void
// return: int                          0
//==============================================================================
int printer_deinit (void)
{
 printer_a_close();
 printer_b_close();
 return 0;
}

//==============================================================================
// Printer ASCII file close.
//
// Close the printer text file if one is open.
//
//   pass: void
// return: void
//==============================================================================
void printer_a_close (void)
{
 if (printer.print_a_file != NULL)
    {
     if (printer.count != 0)
        fprintf(printer.print_a_file, "\n");
     fclose(printer.print_a_file);
     printer.print_a_file = NULL;
    }
}

//==============================================================================
// Printer binary file close.
//
// Close the printer binary file if one is open.
//
//   pass: void
// return: void
//==============================================================================
void printer_b_close (void)
{
 if (printer.print_b_file != NULL)
    {
     fclose(printer.print_b_file);
     printer.print_b_file = NULL;
    }
}

//==============================================================================
// Printer ASCII open.
//
// Open the text printer file for writing after closing any other open
// printer file if currently open.
//
//   pass: char *s                      printer file name path
//         int action                   0 saves file name only, 1 creates file
// return: int                          0 if success, -1 if error
//==============================================================================
int printer_a_open (char *s, int action)
{
 char filepath[SSIZE1];

 strcpy(printer.printa, s);
 if (action == 1)
    {
     if (printer.print_a_file != NULL)
        printer_a_close();

     printer.print_a_file = open_file(s, userhome_prntpath, filepath, "w");
     if (printer.print_a_file == NULL)
        {
         xprintf("printer_a_open: Unable to create printer ASCII file %s\n", filepath);
         return -1;
        }
     printer.count = 0;
    }
 return 0;
}

//==============================================================================
// Printer binary open.
//
// Open the binary printer file for writing after closing any other open
// printer file if currently open.
//
//   pass: char *s                      printer file name path
//         int action                   0 saves file name only, 1 creates file
// return: int                          0 if success, -1 if error
//==============================================================================
int printer_b_open (char *s, int action)
{
 char filepath[SSIZE1];

 strcpy(printer.printb, s);
 if (action == 1)
    {
     if (printer.print_b_file != NULL)
        printer_b_close();

     printer.print_b_file = open_file(s, userhome_prntpath, filepath, "wb");
     if (printer.print_b_file == NULL)
        {
         xprintf("printer_b_open: Unable to create printer binary file %s\n", filepath);
         return -1;
        }
    }
 return 0;
}

//==============================================================================
// Printer strobe.
//
//   pass: void
// return: void
//==============================================================================
void printer_strobe (void)
{
 printer.busy = 0;
 (*printer_ops.strobe)();
}

//==============================================================================
// Printer strobe.
//
//   pass: void
// return: void
//==============================================================================
void printer_poll(void)
{
 if (printer.strobe_due != 0 &&
     z80api_get_tstates() > printer.strobe_due)
    {
     printer_strobe();
     printer.strobe_due = 0;
    }
}

//==============================================================================
// Printer write.
//
// PRINTER_STROBE_MS should contain a typical strobe time value in ms.
//
//   pass: int data
// return: void
//==============================================================================
void printer_w (uint8_t data)
{
 printer.data = data;   /* not written yet. */
}

void printer_ready(void)
{
 if (printer.busy == 1)
    return;                     /* new data has been written before
                                 * the previous data was
                                 * acknowledged */
 if (printer.print_a_file != NULL)
    {
     fprintf(printer.print_a_file, "%3d ", printer.data);
     if (++printer.count == 16)
        {
         fprintf(printer.print_a_file, "\n");
         printer.count = 0;
        }
    }

 if (printer.print_b_file != NULL)
    fwrite(&printer.data, sizeof(printer.data), 1, printer.print_b_file);

 if ((printer.print_a_file != NULL) || (printer.print_b_file != NULL))
    {
     printer.strobe_due = z80api_get_tstates() + PRINTER_PROCESSING_TIME;
     printer.busy = 1;
    }
}
