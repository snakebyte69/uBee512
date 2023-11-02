//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                          Mouse peripheral module                           *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulates a Microsoft Mouse attached to a Microbee mouse interface circuit
// connected to the serial port.
//
// Reference:
// http://www.kryslix.com/nsfaq/Q.12.html
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
// v4.2.0 - 13 July 2009, uBee
// - Created new mouse.c module.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ubee512.h"
#include "mouse.h"
#include "z80api.h"
#include "pio.h"
#include "gui.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
mouse_t mouse =
{
 .protocol = MOUSE_HOST
};

static uint64_t tstates_data;

static char packet_buf_in[3];
static char packet_buf_out[3];

static int packet_active;
static int packet_pending;
static int packet_bytenumb;
static int packet_bitpos;
static int bitmask;

static int mouse_cts;
static int mouse_dtr;

extern emu_t emu;
extern pio_t pio_b;

//==============================================================================
// Mouse initialise
//
//   pass: void
// return: int                          0
//==============================================================================
int mouse_init (void)
{
 mouse_configure(mouse.active);

 return 0;
}

//==============================================================================
// Mouse de-initialise
//
//   pass: void
// return: int                          0
//==============================================================================
int mouse_deinit (void)
{
 return 0;
}

//==============================================================================
// Mouse reset
//
//   pass: void
// return: int                          0
//==============================================================================
int mouse_reset (void)
{
 mouse_configure(mouse.active);

 packet_pending = 0;
 packet_active = 0;
 mouse_cts = 0;
 mouse_dtr = 0;

 return 0;
}

//==============================================================================
// Configure the Microbee mouse.
//
//   pass: int x
// return: void
//==============================================================================
void mouse_configure (int x)
{
 mouse.active = x;

 mouse.host_in_use = ((mouse.protocol == MOUSE_HOST) && (mouse.active == MOUSE_ON));

 if (mouse.host_in_use)
    {
     SDL_WM_GrabInput(SDL_GRAB_ON);
     SDL_ShowCursor(SDL_DISABLE);
    }
 else
    {
     SDL_WM_GrabInput(SDL_GRAB_OFF);
     SDL_ShowCursor(SDL_ENABLE);
    }

 gui_status_update();
}

//==============================================================================
// Read data from the Microbee mouse interface.
//
// The Microbee uses an interface circuit that provides a clock bit for each
// serial bit received from the mouse.  The Start and Stop bits are passed
// through but these are not used for synchronising.
//
// A new data bit is ready on each clock transition.  This function must
// return the data and clock bits in the stated RS232 postions.
//
// Sets mouse_dtr to 1 if the start of a new data bit.  The bit is cleared
// by the PIO polling mode.
//
// A mouse packet is:
//    Protocol: Microsoft 2 button mouse
//   baud rate: 1200
//       bytes: 1 start bit, 7 data bits, 1 stop bit
// bits/packet: 27
//   clock bit: bit 3 (DTR input)
//    data bit: bit 2 (CTS input)
//
//   pass: void
// return: int                          CTS | DTR bits
//==============================================================================
int mouse_r (void)
{
 uint64_t tstates_now = z80api_get_tstates();

 if ((packet_pending) && (! packet_active))
    {
     memcpy(&packet_buf_out, &packet_buf_in, sizeof(packet_buf_out));
     packet_pending = 0;
     packet_active = 1;
     packet_bytenumb = 0;
     packet_bitpos = 0;
     tstates_data = tstates_now; // allow it to start without delay
     mouse_cts = 0;  // stop bit
     mouse_dtr = 0;  // sync low
    }

 if (! packet_active)
    {
     mouse_cts = 0;  // stop bit
     mouse_dtr = 0;  // sync low
     return mouse_cts | mouse_dtr;
    }

 if (tstates_now < tstates_data)
    return mouse_cts | mouse_dtr;

 tstates_data = tstates_now + 2812; // tstates for 1200 baud data

 switch (packet_bitpos)
    {
     case 0 : // start bit
        if (packet_bytenumb == 3)
           {
            packet_active = 0;
            break;
           }
        bitmask = B8(00000001);
        packet_bitpos++;
        mouse_cts = PIO_B_RS232_CTS;  // start bit
        mouse_dtr = PIO_B_RS232_DTR;
        break;
     case 8 : // stop bit
        packet_bitpos = 0;
        packet_bytenumb++;
        mouse_cts = 0;  // stop bit
        mouse_dtr = PIO_B_RS232_DTR;
        break;
     default : // data bits
        mouse_cts = (((uint8_t)packet_buf_out[packet_bytenumb] & bitmask) == 0) *
                    PIO_B_RS232_CTS;
        mouse_dtr = PIO_B_RS232_DTR;
        packet_bitpos++;
        bitmask <<= 1;
        break;
    }

 return mouse_cts | mouse_dtr;
}

//==============================================================================
// Clear the mouse sync.
//
// This is called after the PIO has processed a mouse interrupt.
//
//   pass: void
// return: void
//==============================================================================
void mouse_sync_clear (void)
{
 mouse_dtr = 0;
}

//==============================================================================
// Construct mouse packet.
//
// Simulate the Microsoft 3 byte data packet structure.
//
// Packets consist of 3 bytes. A packet is sent for each mouse event, button
// pressed, button released, and mouse moved.
//
//         D7      D6      D5      D4      D3      D2      D1      D0
// Byte 1   X       1      LB      RB      Y7      Y6      X7      X6
// Byte 2   X       0      X5      X4      X3      X2      X1      X0
// Byte 3   X       0      Y5      Y4      Y3      Y2      Y1      Y0
//
// LB is the state of the left button (1 means down)
// RB is the state of the right button (1 means down)
// X7-X0 movement in X direction since last packet (signed byte)
// Y7-Y0 movement in Y direction since last packet (signed byte)
//
// The high order bit of each byte (D7) is ignored. Bit D6 indicates the
// start of an event, which allows the software to synchronize with the
// mouse.
//
//   pass: int button
// return: void
//==============================================================================
static void mouse_construct_packet (int button)
{
 int8_t x = mouse.x;
 int8_t y = mouse.y;

 // if mouse is not programmed for interrupts ignore the mouse event
 if ((! (pio_b.hilo & PIO_B_RS232_DTR)) || (pio_b.maskword & PIO_B_RS232_DTR))
    return;

 if (button)
    {
     x = 0;
     y = 0;
    }
 else
    {
     x = mouse.x;
     y = mouse.y;
    }

 packet_buf_in[0] = B8(01000000);
 packet_buf_in[0] |= ((mouse.button_l << 5) | (mouse.button_r << 4));
 packet_buf_in[0] |= (((y & B8(11000000)) >> 4) | ((x & B8(11000000)) >> 6));
 packet_buf_in[1] = x & B8(00111111);
 packet_buf_in[2] = y & B8(00111111);
 packet_pending = 1;
}

//==============================================================================
// Mouse button down event.
//
//   pass: void
// return: void
//==============================================================================
void mouse_mousebuttondown_event (void)
{
 switch (emu.event.button.button)
    {
     case SDL_BUTTON_LEFT :
        mouse.button_l = 1;
        break;
     case SDL_BUTTON_MIDDLE :
        mouse_configure(MOUSE_OFF);
        return;
     case SDL_BUTTON_RIGHT :
        mouse.button_r = 1;
        break;
    }

 if (packet_pending || packet_active)
    return;

 mouse_construct_packet(1);
}

//==============================================================================
// Mouse button up event.
//
//   pass: void
// return: void
//==============================================================================
void mouse_mousebuttonup_event (void)
{
 switch (emu.event.button.button)
    {
     case SDL_BUTTON_LEFT :
        if (mouse.button_l)
           mouse.button_l = 0;
        break;
     case SDL_BUTTON_MIDDLE :
        return;
     case SDL_BUTTON_RIGHT :
        if (mouse.button_r)
           mouse.button_r = 0;
        break;
    }

 if (packet_pending || packet_active)
    return;

 mouse_construct_packet(1);
}

//==============================================================================
// Mouse motion event.
//
//   pass: void
// return: void
//==============================================================================
void mouse_mousemotion_event (void)
{
 if (packet_pending || packet_active)
    return;

 SDL_GetRelativeMouseState(&mouse.x, &mouse.y);

 mouse_construct_packet(0);
}

//==============================================================================
// Mouse commands.
//
//   pass: int cmd                      mouse command
// return: void
//==============================================================================
void mouse_command (int cmd)
{
 switch (cmd)
    {
     case EMU_CMD_MOUSE :
        mouse_configure(! mouse.active);
        break;
    }
}
