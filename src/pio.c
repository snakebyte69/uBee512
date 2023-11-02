//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              Z80 PIO module                                *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulate the Z80 PIO functionality and interrupts.
//
// All PIO peripheral devices are maintained in separate source files:
//
// tape.c               Tape in/out port
// serial.c             Serial in/out port
// sound.c              Speaker sound port
// rtc.c                RTC (interrupt)
// keytc.c              256TC keyboard (interrupt)
//
// printer.c            Parallel printer peripheral
// joystick.c           Joystick peripheral
// mouse.c              Mouse peripheral
// beethoven.c          BeeThoven sound synthesiser peripheral
// beetalker.c          Voice synthesiser peripheral
// dac.c                Digital to analogue converter (DAC) peripheral
// compumuse.c          Compumuse peripheral
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
// v5.0.0 - 13 July 2010, K Duckmanton
// - Removed all references to the 'sound' global variable and replaced them
//   with references to the 'audio' global instead.
// - Removed code to select one of two speaker modules
// v5.0.0 - 3 July 2010, uBee
// - Added an audio DAC parallel port device 'dac_ops'
//
// v4.7.0 - 17 June 2010, K Duckmanton
// - Changes to allow several different devices to be connected to the
//   emulated parallel port.  Added code to select alternate speaker
//   emulation code when the beethoven or beetalker devices are in use.
// - Minor changes to the way the speaker function is called.
// - Update the I/O direction register when setting the port mode, also
//   make port mode changes take effect immediately (see also comments
//   for version 2.6.0).  This is important for some games (e.g. Colour
//   Robotman) as they do not configure the PIO correctly, relying
//   instead on implementation-defined behaviour.
//
// v4.6.0 - 5 May 2010, uBee
// - Changes in pio_update() to act on TC key interrupts as quickly as
//   possible by using z80api_set_poll_tstates(0, 500000)
// - Changes to pio_regdump() to add binary output.
// - Renamed log_data() calls to log_data_1() and log_port() to log_port_1().
// - Fixed a regression bug introduced in v4.0.0 that had originally been
//   fixed earlier in v2.8.0 concerning calling the rtc_poll() function. The
//   256TC boot ROMs now updates the time again.
//
// v4.2.0 - 18 July 2009, uBee
// - Added Microbee mouse peripheral emulation.
// - Added pio_regdump() function to allow dumping PIO register values.
// - Added additional reporting of interrupt vector table contents.
// - Fixes to PIO to keep the port input and output data bits in their own
//   variables and various other improvements.
//
// v4.1.0 - 22 June 2009, uBee
// - Made improvements to the logging process.
//
// v4.0.0 - 28 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Many changes made to interrupt emulation and coding in general.
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
//
// v3.1.0 - 6 November 2008, uBee
// - Changed test of tape.tape_i to tape.tapei[0] and tape.tape_o to
//   tape.tapeo[0].
// - Changed all printf() calls to use a local xprintf() function.
//
// v2.8.0 - 30 August 2008, uBee
// - rtc_poll() was not being called in pio_polling() function unless
//   modelx.piob7 == MODPB7_RTC. The RTC is now polled if RTC emulation is
//   enabled so that the RTC registers are kept updated.
// - Assign some values to piob7_irq and key_irq to keep the compiler from
//   generating compile warnings.
//
// v2.7.0 - 30 May 2008, uBee
// - Changes to the pio_configure() function.
// - fast_poll_persist changed to emu.divider_persist and is now handled in
//   the ubee512.c module.  This is now set when any execution ratio is changed.
// - Made changes to the z80_exec_ratio() function timing values as the duty
//   cycle on the CRTC 6545 Vertical blanking was being missed and also the
//   execution ratio is now standard so other settings have been increased to
//   at least match the default divider value.
// - Added structure emu_t and moved variables into it.
//
// v2.6.0 - 11 May 2008, uBee
// - Many tape, printer and serial variables have been placed into structures
//   and declared in the appropriate modules.
// - Buffer pio->mode using pio->mode_buf.  See the notes in the pio_control()
//   function for an explanation.
// - Added Joystick support on PIO port A.
//
// v2.5.0 - 9 March 2008, uBee
// - Use new member 'tckeys' in modelx structure as Teleterm model emulation
//   has been added.
//
// v2.3.0 - 24 December 2007, uBee
// - Added modio_t structure.
//
// v2.2.0 - 15 November 2007, uBee
// - The printer_w() function is now always called even if no --print options
//   are specified.
//
// v2.1.0 - 23 October 2007, uBee
// - Added PIO port B bit 7 code to handle alternative signal sources in the
//   pio_polling() and pio_r() functions.
// - Added a fast poll persist counter,  this makes the vsync clock keep time
//   correctly by not missing interrupts.
// - Implement the modelx information structure.
//
// v2.0.0 - 15 October 2007, uBee
// - Added RTC polling and interrupt code, and PIO RTC IRQ bit 7.
// - Added 256TC polling and interrupt code, and PIO port B bit 1 (key ready)
// - PIO port B in/out now uses the direction bits.
// - PIO port B interrupt mask word now used.
//
// v1.4.0 - 20 September 2007, uBee
// - Added sound on/off conditional testing.
//
// v1.3.0 - 1 September 2007, uBee
// - Emulate PIO internal registers and interrupts.
// - Added RS232 serial TX, RX, DTR and CTS.
// - Added printer (parallel) out on port A.
//
// v1.2.0 - 22 August 2007, uBee
// - Added tape out and tape in.
//
// v1.0.0 - 23 June 2007, uBee
// - Created a new file to implement the Z80 PIO emulation.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <SDL.h>

#include "pio.h"
#include "z80api.h"
#include "ubee512.h"
#include "sound.h"
#include "tape.h"
#include "serial.h"
#include "mouse.h"
#include "keytc.h"
#include "rtc.h"
#include "crtc.h"
#include "z80api.h"
#include "support.h"
#include "parint.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
extern parint_ops_t printer_ops;

pio_t pio_a;
parint_ops_t *pio_a_peripheral = &printer_ops;
pio_t pio_b;

static z80_device_interrupt_t pio_int_scratch;

static int polling;

extern int coms1;

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;
extern serial_t serial;
extern tape_t tape;
extern mouse_t mouse;

//==============================================================================
// function prototypes
//==============================================================================
void pio_reti(void);
int pio_ieo(void);

//==============================================================================
// PIO - Control register
//
// Experiments with a real PIO suggest that setting modes 0 and 1
// clear and set all bits in the I/O direction register; mode 2 clears and sets
// the register based on the STB* input, and mode 3 sets the register based on
// the next byte written.  In all cases the mode change takes effect immediately;
// for mode 3 this means that whatever is in the I/O direction register when
// mode 3 is set is used until the register is updated on the next write.
//
// Since the actual values of the strobe inputs aren't available, we assume
// that they are tied high here (thus the I/O direction register sets the I/O
// lines to inputs)
//
//   pass: pio_t *pio
//         uint8_t data
//         char port
// return: void
//==============================================================================
static void pio_control (pio_t *pio, uint8_t data, char port)
{
 pio->cont = data;

 if (pio->action)
    {
     if (pio->action == 1)
        {
         pio->direction = data; // set data direction (1=input, 0=output)
         if (modio.piocont)
            log_data_1("pio_control", "set data direction", data);
        }
     if (pio->action == 2)
        {
         pio->maskword = data;
         if (modio.piocont)
            log_data_1("pio_control", "mask word", data);
        }
     pio->action = 0;
    }
 else
    {
     if ((data & B8(00000001)) == 0) // if interrupt vector
        {
         pio->vector = data;
         if (modio.piocont)
            log_data_1("pio_control", "interrupt vector", data);
        }
     else
        if ((data & B8(00001111)) == B8(00000111)) // if interrupt control word
           {
            pio->ienable = data & B8(10000000);
            if (pio->ienable)
               pio->ienableff = B8(10000000);
            pio->andor = data & B8(01000000);
            if (data & B8(00100000))
               pio->hilo = 0xff;
            else
               pio->hilo = 0x00;
            if (data & B8(00010000)) // if mask word follows
               {
                pio->action = 2;
                pio->pending = 0;
               }
            if (modio.piocont)
               log_data_1("pio_control", "interrupt control word", data);
           }
        else
           if ((data & B8(00001111)) == B8(00001111)) // if mode select
              {
               pio->mode = (data & B8(11000000)) >> 6;
               switch (pio->mode)
                  {
                   case 0 :
                      pio->direction = 0x00; // all bits outputs
                      break;
                   case 1 :
                   case 2 :
                      pio->direction = 0xff; // all bits inputs
                      break;
                   case 3 :
                      pio->action = 1; // additional control byte needed
                      break;
                  }
               if (modio.piocont)
                  log_data_1("pio_control", "mode select", data);
              }
           else
              if ((data & B8(00001111)) == B8(00000011)) // if interrupt flipflop set
                 {
                  pio->ienableff = data & B8(10000000);
                  if (modio.piocont)
                  log_data_1("pio_control", "interrupt flipflop set", data);
                 }
    }
}

//==============================================================================
// PIO Initialise
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int pio_init (void)
{
 int x=0;

 x = speaker_init();

 // create mutexes to protect the interrupt pending flag on both
 // channels.
 pio_a.pending_mutex = SDL_CreateMutex();
 pio_b.pending_mutex = SDL_CreateMutex();

 // register interrupt-possible and reti handlers
 z80api_register_interrupting_device(&pio_int_scratch,
                                     &pio_ieo,
                                     &pio_reti);

 if (pio_a_peripheral && pio_a_peripheral->init)
    x |= (*pio_a_peripheral->init)();

 return (x | tape_init() | serial_init());
}

//==============================================================================
// PIO de-initialise
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int pio_deinit (void)
{
 int x=0;

 x = speaker_deinit();
 if (pio_a_peripheral && pio_a_peripheral->deinit)
    x |= (*pio_a_peripheral->deinit)();
 x |= tape_deinit() | serial_deinit();
 SDL_DestroyMutex(pio_a.pending_mutex);
 SDL_DestroyMutex(pio_b.pending_mutex);

 return x;
}

//==============================================================================
// PIO reset
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int pio_reset (void)
{
 int x = 0;

 pio_a.mode = 1;
 pio_a.maskword = 0;
 pio_a.ienableff = 0;
 pio_a.data = 0;
 pio_a.data_in = 0;
 pio_a.data_out = 0;
 pio_a.action = 0;
 pio_a.change = 0;
 pio_a.last = 0;

 pio_b.mode = 1;
 pio_b.maskword = 0;
 pio_b.ienableff = 0;
 pio_b.data = 0;
 pio_b.data_in = 0;
 pio_b.data_out = 0;
 pio_b.action = 0;
 pio_b.change = 0;
 pio_b.last = 0;

 x = speaker_reset();

 if (pio_a_peripheral && pio_a_peripheral->reset)
    x |= (*pio_a_peripheral->reset)();

 return (x | tape_reset() | serial_reset() | mouse_reset());
}

//==============================================================================
// PIO - connect a device to parallel port A
//
//   pass: void
// return: int
//==============================================================================
int pio_porta_connect(parint_ops_t *device)
{
 int x;

 // Don't call the initialisation and deinitialisation functions if the
 // emulator isn't running yet.
 if (emu.runmode && pio_a_peripheral && pio_a_peripheral->deinit())
    {
     x = (*pio_a_peripheral->deinit)();
     if (x)
        return x;
    }
 pio_a_peripheral = device;
 if (emu.runmode && pio_a_peripheral && pio_a_peripheral->init())
    {
     x = (*pio_a_peripheral->init)();
     if (x)
        return x;
    }
 return 0;                      /* success */
}

//==============================================================================
// PIO - poll for interrupt events.
//
//   pass: void
// return: void
//==============================================================================
void pio_polling (void)
{
 polling = 1;
 pio_r(0x00, NULL);
 pio_r(0x02, NULL);
 polling = 0;
}

//==============================================================================
// PIO - acknowledge the service of an interrupt by the CPU
//
//   pass: void
// return: void
//==============================================================================
void pio_reti(void)
{
 // Call up to higher priority devices first.
 (*pio_int_scratch.intack)();

 // This device is (now) allowed to raise interrupts?
 if (!(*pio_int_scratch.iei)())
    return;

 // Within the PIO, port A interrupts have higher priority than port B
 // interrupts.
 SDL_LockMutex(pio_a.pending_mutex);
 if (pio_a.pending == 1)
    {
     pio_a.pending = 0;
     SDL_UnlockMutex(pio_a.pending_mutex);
     return;
    }
 SDL_UnlockMutex(pio_a.pending_mutex);

 SDL_LockMutex(pio_b.pending_mutex);
 if (pio_b.pending == 1)
    {
     pio_b.pending = 0;         /* clear port A interrupt pending
                                 * flag */
     SDL_UnlockMutex(pio_b.pending_mutex);
     return;
    }
 SDL_UnlockMutex(pio_b.pending_mutex);
}

//==============================================================================
// PIO - check to see if lower priority devices may interrupt the CPU.
//
//   pass: void
// return: void
//==============================================================================
int pio_ieo(void)
{
 int res;
 // The CPU may be interrupted only if no higher priority devices have
 // interrupts pending.
 res = (*pio_int_scratch.iei)();
 if (!res)
    return res;
 SDL_LockMutex(pio_a.pending_mutex);
 if (pio_a.pending == 1)
    {
     SDL_UnlockMutex(pio_a.pending_mutex);
     return 0;                  // can't interrupt, port A has a
                                // pending interrupt
    }
 SDL_UnlockMutex(pio_a.pending_mutex);

 SDL_LockMutex(pio_b.pending_mutex);
 if (pio_b.pending == 1)
    {
     SDL_UnlockMutex(pio_b.pending_mutex);
     return 0;                  // ditto port B
    }
 SDL_UnlockMutex(pio_b.pending_mutex);

 return 1;                      // no pending interrupts, other
                                // devices may interrupt.
}

//==============================================================================
// PIO - configure.
//
//   pass: int cpuclock
// return: void
//==============================================================================
void pio_configure (int cpuclock)
{
}

//==============================================================================
// PIO register dump
//
// Dump the contents of the PIO registers.
//
//   pass: void
// return: void
//==============================================================================
void pio_regdump (void)
{
 char s[17];

 xprintf("\n");
 xprintf("Z80 PIO Registers      Hex  Dec    Binary\n");
 xprintf("------------------------------------------\n");
 xprintf("%-22s %02x %5d %10s\n", "PIO A control",
 pio_a.cont, pio_a.cont, i2b(pio_a.cont, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO A vector",
 pio_a.vector, pio_a.vector, i2b(pio_a.vector, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO A mode",
 pio_a.mode, pio_a.mode, i2b(pio_a.mode, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO A maskword",
 pio_a.maskword, pio_a.maskword, i2b(pio_a.maskword, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO A ienableff",
 pio_a.ienableff, pio_a.ienableff, i2b(pio_a.ienableff, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO A data",
 pio_a.data, pio_a.data, i2b(pio_a.data, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO A data_in",
 pio_a.data_in, pio_a.data_in, i2b(pio_a.data_in, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO A data_out",
 pio_a.data_out, pio_a.data_out, i2b(pio_a.data_out, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO A direction",
 pio_a.direction, pio_a.direction, i2b(pio_a.direction, s));
 xprintf("\n");
 xprintf("%-22s %02x %5d %10s\n", "PIO B control",
 pio_b.cont, pio_b.cont, i2b(pio_b.cont, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO B vector",
 pio_b.vector, pio_b.vector, i2b(pio_b.vector, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO B mode",
 pio_b.mode, pio_b.mode, i2b(pio_b.mode, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO B maskword",
 pio_b.maskword, pio_b.maskword, i2b(pio_b.maskword, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO B ienableff",
 pio_b.ienableff, pio_b.ienableff, i2b(pio_b.ienableff, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO B data",
 pio_b.data, pio_b.data, i2b(pio_b.data, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO B data_in",
 pio_b.data_in, pio_b.data_in, i2b(pio_b.data_in, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO B data_out",
 pio_b.data_out, pio_b.data_out, i2b(pio_b.data_out, s));
 xprintf("%-22s %02x %5d %10s\n", "PIO B direction",
 pio_b.direction, pio_b.direction, i2b(pio_b.direction, s));
}

//==============================================================================
// PIO - update.
//
// Checks to see if a maskable interrupt should be generated,  if so, set the
// interrupt pending.
//
// Interrupts supported:
//
// RS232 Serial interrupts
// -----------------------
// Emulated and working much better since version v4.0.0.
//
// Mouse interrupts
// ----------------
// Emulates Mouse interface, data is on CTS and sync interrupt on DTR.
//
// 256TC/Teleterm Key interrupts
// -----------------------------
// Used when emulating the Telecomputer 256TC and Telecomputer keyboard.
// The 256TC boot ROM tested used only polling methods.  The Teleterm
// model uses a combination of polling and interrupts.
//
// Clock interrupts
// ----------------
// Used when emulating a source interrupt on bit port B bit 7.  the signal
// source can be one of the following:
//
//       RTC : Used if the RTC is being emulated and the source has been
//             selected to be taken from the RTC.
//     VSYNC : Used when emulating the CRTC VSYNC into port B bit 7.
//       NET : Not implemented.
//             D15 parallel port connector (X2 pin 14) used as a network
//             direction bit.
//       PUP : Pullup resistor,  no interrupt is generated for this.
//
// Port A LPT
// ----------
// Generates an interrupt after each write to port A.
//
//   pass: void
// return: void
//==============================================================================
static void pio_update (void)
{
 int p;
 int addr;

 z80regs_t z80regs;

 // check and save changed bits on PIO port B
 p = pio_b.data_in;

 if (p ^ pio_b.last)
    {
     pio_b.change |= (p ^ pio_b.last);
     pio_b.last = p;
    }

 // if the Z80 can't be interrupted or if a higher priority device is blocking
 // interrupts, return
 if (!(*pio_int_scratch.iei)() || !z80api_intr_possible())
    return;

 if (pio_a.ienableff)
    {
     if (pio_a_peripheral && pio_a_peripheral->poll)
        (*pio_a_peripheral->poll)();
     SDL_LockMutex(pio_a.pending_mutex);
     if (pio_a.pending == 1)
        {
         // the pending interrupt flag is reset in the RETI handler
         SDL_UnlockMutex(pio_a.pending_mutex);
         z80api_set_poll_tstates(100, 10);
         z80api_maskable_intr(pio_a.vector);

         if (modio.piocont)
            {
             z80api_get_regs(&z80regs);
             addr = z80api_read_mem((z80regs.i << 8) | pio_a.vector) |
                (z80api_read_mem((z80regs.i << 8) | (pio_a.vector + 1)) << 8);
             log_data_2("pio_interrupt (A)", "vector", "contents", pio_a.vector, addr);
            }
         return;
        }
     SDL_UnlockMutex(pio_a.pending_mutex);
    }

 if (! pio_b.ienableff)
    return;

 // all PIO interrupts assume the use of OR bits, ANDing bits is not currently supported.
 if (! pio_b.change)
    return;

 // standard serial port interrupt
 if ((pio_b.change & PIO_B_RS232_RX) &&
     ((p & PIO_B_RS232_RX) == (pio_b.hilo & PIO_B_RS232_RX)) &&
     ((pio_b.maskword & PIO_B_RS232_RX) == 0))
    {
     pio_b.change &= ~PIO_B_RS232_RX;
     z80api_set_poll_tstates(100, 1000);
     serial_interrupt_adjust();
     z80api_maskable_intr(pio_b.vector);

     if (modio.piocont)
        {
         z80api_get_regs(&z80regs);
         addr = z80api_read_mem((z80regs.i << 8) | pio_b.vector) |
            (z80api_read_mem((z80regs.i << 8) | (pio_b.vector + 1)) << 8);
         log_data_2("pio_interrupt (B) Serial", "vector", "contents", pio_b.vector, addr);
        }
     return;
    }

 // mouse port interrupt
 if ((pio_b.change & PIO_B_RS232_DTR) &&
     ((p & PIO_B_RS232_DTR) == (pio_b.hilo & PIO_B_RS232_DTR)) &&
     ((pio_b.maskword & PIO_B_RS232_DTR) == 0))
    {
     mouse_sync_clear();
     pio_b.change &= ~PIO_B_RS232_DTR;
     z80api_set_poll_tstates(100, 1000);
     z80api_maskable_intr(pio_b.vector);

     if (modio.piocont)
        {
         z80api_get_regs(&z80regs);
         addr = z80api_read_mem((z80regs.i << 8) | pio_b.vector) |
            (z80api_read_mem((z80regs.i << 8) | (pio_b.vector + 1)) << 8);
         log_data_2("pio_interrupt (B) Mouse", "vector", "contents", pio_b.vector, addr);
        }
     return;
    }

 // 256TC/Teleterm key board port interrupt
 if ((modelx.tckeys) &&
     (pio_b.change & PIO_B_KEY256TC) &&
     ((p & PIO_B_KEY256TC) == (pio_b.hilo & PIO_B_KEY256TC)) &&
     ((pio_b.maskword & PIO_B_KEY256TC) == 0))
    {
     pio_b.change &= ~PIO_B_KEY256TC;

     // we need immediate PIO polling (0) to occur with a generous persist
     // repeat counter, 500000 may seem overly generous, 10000 is not enough
     // and 100000 will lose keys when the key is held down for a lengthy
     // period before being released.
     z80api_set_poll_tstates(0, 500000);

     z80api_maskable_intr(pio_b.vector);

     if (modio.piocont)
        {
         z80api_get_regs(&z80regs);
         addr = z80api_read_mem((z80regs.i << 8) | pio_b.vector) |
            (z80api_read_mem((z80regs.i << 8) | (pio_b.vector + 1)) << 8);
         log_data_2("pio_interrupt (B) 256TC/Teleterm KBD", "vector", "contents", pio_b.vector, addr);
        }
     return;
    }

 // clock or network direction port interrupt
 if ((pio_b.change & PIO_B_CLOCK) &&
     ((p & PIO_B_CLOCK) == (pio_b.hilo & PIO_B_CLOCK)) &&
     ((pio_b.maskword & PIO_B_CLOCK) == 0))
    {
     pio_b.change &= ~PIO_B_CLOCK;
     z80api_set_poll_tstates(100, 10);
     switch (modelx.piob7)
        {
         case MODPB7_PUP :
            break;
         case MODPB7_VS :
            z80api_maskable_intr(pio_b.vector);
            if (modio.piocont)
               {
                z80api_get_regs(&z80regs);
                addr = z80api_read_mem((z80regs.i << 8) | pio_b.vector) |
                (z80api_read_mem((z80regs.i << 8) | (pio_b.vector + 1)) << 8);
                log_data_2("pio_interrupt (B) VSYNC", "vector", "contents", pio_b.vector, addr);
               }
            break;
         case MODPB7_RTC :
            z80api_maskable_intr(pio_b.vector);
            if (modio.piocont)
               {
                z80api_get_regs(&z80regs);
                addr = z80api_read_mem((z80regs.i << 8) | pio_b.vector) |
                (z80api_read_mem((z80regs.i << 8) | (pio_b.vector + 1)) << 8);
                log_data_2("pio_interrupt (B) RTC", "vector", "contents", pio_b.vector, addr);
               }
            break;
         case MODPB7_NET :
            break;
        }
     return;
    }
}

//==============================================================================
// PIO strobe - Port function
//
// Sets the interrupt pending bit on PIO port A.  Used by peripheral devices
// to signal that a received byte has been processed.
//
//   pass: void
// return: void
//==============================================================================
void pio_porta_strobe(void)
{
 SDL_LockMutex(pio_a.pending_mutex);
 pio_a.pending = 1;
 SDL_UnlockMutex(pio_a.pending_mutex);
}

//==============================================================================
// PIO read - Port function
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t             register data
//==============================================================================
uint16_t pio_r (uint16_t port, struct z80_port_read *port_s)
{
 int p = port & 0xff;
 int rtc_irq;

 switch (p & 0x03)
    {
     case 0x00 : // PIO port A data port
        if (!polling)
           {
            switch (pio_a.mode)
               {
                case 0 : /* output mode */
                   /* Input from a port in output mode returns the
                    * last data that was written */
                   break;
                case 1 : /* input mode */
                case 2 : /* bidirectional mode */
                   /* In input and bidirectional mode a read will
                    * return the data currently on the data bus and
                    * assert ready */
                   if (pio_a_peripheral)
                      {
                       if (pio_a_peripheral->read)
                          pio_a.data = (*pio_a_peripheral->read)();
                       if (pio_a_peripheral->ready)
                          (*pio_a_peripheral->ready)();
                      }
                   break;
                case 3 : /* control mode */
                   /* In control mode, of course, we need to poll
                    * the peripheral device directly and update the
                    * data register that way */
                   pio_a.data =
                   (pio_a.data & ~pio_a.direction) |
                   (((pio_a_peripheral && pio_a_peripheral->read) ?
                   (*pio_a_peripheral->read)() : 0xff) & pio_a.direction);
                   break;
               }
            if (modio.pioa)
               log_port_1("pio_r (A)", "data", port, pio_a.data);
           }
        // update the PIO interrupts, etc
        pio_update();   /* FIXME */
        if ((modio.pioa) && (! polling))
           log_port_1("pio_r (A)", "data", port, pio_a.data);
        return pio_a.data;
     case 0x01 : // PIO port A control port
        if ((modio.piocont) && (! polling))
           log_port_1("pio_r (A)", "control", port, pio_a.cont);
        return pio_a.cont;
     case 0x02 : // PIO port B data port
        pio_b.data_in = 0;

        if ((tape.in_status) && (tape.tapei[0]) && (pio_b.direction & PIO_B_CASIN))
           pio_b.data_in |= tape_r();

        if ((modelx.tckeys) && (pio_b.direction & PIO_B_KEY256TC))
           pio_b.data_in |= keytc_poll();

        if (mouse.active)
           {
            if (pio_b.direction & PIO_B_RS232_DTR)
               pio_b.data_in |= mouse_r();
           }
        else
           {
            if (pio_b.direction & PIO_B_RS232_RX)
               pio_b.data_in |= serial_r();
           }

        if (modelx.rtc)
           rtc_irq = rtc_poll();
        else
           rtc_irq = 0;

        switch (modelx.piob7)
           {
            case MODPB7_PUP :
               pio_b.data_in |= PIO_B_PUP;  // pull up resistor
               break;
            case  MODPB7_VS :
               if (pio_b.direction & PIO_B_CLOCK)
                  pio_b.data_in |= crtc_vblank();
               break;
            case MODPB7_RTC :
               if (pio_b.direction & PIO_B_RTC)
                  pio_b.data_in |= rtc_irq;
               break;
            case MODPB7_NET :
               break;
           }

        // keep only bits designated as inputs
        pio_b.data_in &= pio_b.direction;

        // combine the inputs with the outputs
        pio_b.data = pio_b.data_in | pio_b.data_out;

        if ((modio.piob) && (! polling))
           log_port_1("pio_r (B)", "data", port, pio_b.data);

        // update the PIO interrupts, etc
        pio_update();
        return pio_b.data;

     case 0x03 : // PIO port B control port
        if (modio.piocont)
           log_port_1("pio_r (B)", "control", port, pio_b.cont);
        return pio_b.cont;
    }
 return 0xFFFF;
}

//==============================================================================
// PIO write - Port function
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void pio_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 int p = port & 0xff;

 switch (p & 0x03)
    {
     case 0x00 : // PIO port A data port
        if (modio.pioa)
           log_port_1("pio_w (A)", "data", port, data);
        switch (pio_a.mode)
           {
            case 0 :
            case 2 :
               /* writes go through in bidirectional and output modes */
               pio_a.data = data;
               if (pio_a_peripheral)
                  {
                   if (pio_a_peripheral->write)
                      (*pio_a_peripheral->write)(pio_a.data);
                   if (pio_a_peripheral->ready)
                      (*pio_a_peripheral->ready)();
                  }
               break;
            case 1 :
               /* writes are ignored in input mode */
               break;
            case 3 :
               pio_a.data = data & ~pio_a.direction;
               if (pio_a_peripheral && pio_a_peripheral->write)
                  (*pio_a_peripheral->write)(pio_a.data);
               break;
           }
        break;
     case 0x01 : // PIO port A control port
        if (modio.piocont)
           log_port_1("pio_w (A)", "control", port, data);
        pio_control(&pio_a, data, 'A');
        break;
     case 0x02 : // PIO port B data port
        if (modio.piob)
           log_port_1("pio_w (B)", "data", port, data);

        if ((pio_b.direction & PIO_B_SPEAKER) == 0)
           speaker_w(data & PIO_B_SPEAKER);
        if ((tape.tapeo[0]) && ((pio_b.direction & PIO_B_CASOUT) == 0))
           tape_w(data);
        if ((pio_b.direction & PIO_B_RS232_TX) == 0)
           serial_w(data);

        pio_b.data_out = data & ~pio_b.direction;

        // combine the inputs with the outputs
        pio_b.data = pio_b.data_in | pio_b.data_out;
        break;
     case 0x03 : // PIO port B control port
        if (modio.piocont)
           log_port_1("pio_w (B)", "control", port, data);
        pio_control(&pio_b, data, 'B');
        break;
    }

 // update the PIO interrupts, etc
 pio_update();
}
