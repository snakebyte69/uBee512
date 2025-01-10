//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                           256TC Keyboard module                            *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulates the keys for the 256TC Microbee keyboard.
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
// v4.6.0 - 4 May 2010, uBee
// - Fixed bug in keytc_r() function where a 256TC model was being tested
//   for instead of testing for models that use the TC keys (modelx.tckeys)
//   which inludes the Teleterm model.
// - Changes made to keytc_keydown_event() and keytc_keyup_event() which
//   now use new keytc_key_event() function for both.
// - Changes to make the code work much better when polling (i.e 256tc) and
//   when interrupts are used for TC keys.
// - Renamed log_port() calls to log_port_1().
//
// v4.1.0 - 22 June 2009, uBee
// - Made improvements to the logging process.
//
// v4.0.0 - 19 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
//
// v3.1.0 - 8 January 2009, uBee
// - Moved reset and exit key detection over to one common area in keyb.c
// - Changed all printf() calls to use a local xprintf() function.
// - Fixed modio.keytc reporting in keytc_r() function.
//
// v2.7.0 - 22 May 2008, uBee
// - Added structure emu_t and moved variables into it.
//
// v2.6.0 - 11 May 2008, uBee
// - Minor code clean up.
//
// v2.3.0 - 9 January 2008, uBee
// - HOME key now acts as a control key allowing more emulator functionality
//   to to be included, Tape rewind is now HOME+T.  Full screen toggle is now
//   HOME+ENTER. All emulator key commands are now in keyb.c.
// - Added keytc_w() function due to changes made in z80.c
// - Added modio_t structure.
//
// v2.0.0 - 15 October 2007, uBee
// - Created a new keyboard module specific to the 256TC model Microbee.
//   The keyb.c module will call the appropriate module functions at run time.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "keytc.h"
#include "crtc.h"
#include "vdu.h"
#include "ubee512.h"
#include "z80api.h"
#include "z80.h"
#include "pio.h"
#include "support.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
// 120 key codes
#define MB_KEYS 120

// 120 key codes + 3 emulator keys
#define PC_KEYS 120+3

#define SDLK_NOKEY -1

// This must be the same ordering as the pc_keys table below it.
enum
   {
    PCK_F1,             // F1             00    0
    PCK_ESCAPE,         // ESC            01    1
    PCK_TAB,            // TAB            02    2
    PCK_PAUSE,          // BREAK          03    3  don't use PCK_BREAK
    PCK_NOKEY_1,        // NOKEY          04    4
    PCK_KP0,            // KP 0           05    5
    PCK_KP_PERIOD,      // KP DEL         06    6
    PCK_SPACE,          // SP             07    7
    PCK_F2,             // F2             08    8
    PCK_1,              // 1!             09    9
    PCK_q,              // q              0A   10
    PCK_a,              // a              0B   11
    PCK_NOKEY_2,        // NOKEY          0C   12
    PCK_CAPSLOCK,       // CAPL           0D   13
    PCK_PAGEUP,         // LINEFEED       0E   14  to right of CAPS LOCK
    PCK_INSERT,         // INSERT         0F   15
    PCK_F3,             // F3             10   16
    PCK_2,              // 2@             11   17
    PCK_w,              // w              12   18
    PCK_s,              // s              13   19
    PCK_KP_PLUS,        // KP PLUS        14   20
    PCK_KP2,            // KP 2           15   21
    PCK_KP3,            // KP 3           16   22
    PCK_z,              // z              17   23
    PCK_F4,             // F4             18   24
    PCK_3,              // 3#             19   25
    PCK_e,              // e              1A   26
    PCK_d,              // d              1B   27
    PCK_KP_MINUS,       // KP MINUS       1C   28
    PCK_KP5,            // KP 5           1D   29
    PCK_KP6,            // KP 6           1E   30
    PCK_x,              // x              1F   31
    PCK_F5,             // F5             20   32
    PCK_4,              // 4$             21   33
    PCK_r,              // r              22   34
    PCK_f,              // f              23   35
    PCK_KP_MULTIPLY,    // KP MULTIPLY    24   36
    PCK_KP8,            // KP 8           25   37
    PCK_KP9,            // KP 9           26   38
    PCK_c,              // c              27   39
    PCK_F6,             // F6             28   40
    PCK_5,              // 5%             29   41
    PCK_t,              // t              2A   42
    PCK_g,              // g              2B   43
    PCK_KP7,            // KP 7           2C   44
    PCK_KP1,            // KP 1           2D   45
    PCK_KP4,            // KP 4           2E   46
    PCK_v,              // v              2F   47
    PCK_F7,             // F7             30   48
    PCK_6,              // 6^             31   49
    PCK_y,              // y              32   50
    PCK_h,              // h              33   51
    PCK_KP_DIVIDE,      // KP DIVIDE      34   52
    PCK_DOWN,           // DOWN           35   53
    PCK_RIGHT,          // RIGHT          36   54
    PCK_b,              // b              37   55
    PCK_F8,             // F8             38   56
    PCK_7,              // 7&             39   57
    PCK_u,              // u              3A   58
    PCK_j,              // j              3B   59
    PCK_NOKEY_3,        // NOKEY          3C   60
    PCK_LEFT,           // LEFT           3D   61
    PCK_NOKEY_4,        // NOKEY          3E   62
    PCK_n,              // n              3F   63
    PCK_F9,             // F9             40   64
    PCK_8,              // 8*             41   65
    PCK_i,              // i              42   66
    PCK_k,              // k              43   67
    PCK_NOKEY_5,        // NOKEY          44   68
    PCK_NOKEY_6,        // NOKEY          45   69
    PCK_UP,             // UP             46   70
    PCK_m,              // m              47   71
    PCK_F10,            // F10            48   72
    PCK_9,              // 9(             49   73
    PCK_o,              // o              4A   74
    PCK_l,              // l              4B   75
    PCK_NOKEY_7,        // NOKEY          4C   76
    PCK_BACKSPACE,      // BS             4D   77
    PCK_RETURN,         // ENT            4E   78
    PCK_COMMA,          // ,<             4F   79
    PCK_F11,            // F11            50   80
    PCK_0,              // 0)             51   81
    PCK_p,              // p              52   82
    PCK_SEMICOLON,      // ;:             53   83
    PCK_DELETE,         // DEL            54   84
    PCK_BACKQUOTE,      // `~             55   85
    PCK_BACKSLASH,      // \|             56   86
    PCK_PERIOD,         // .>             57   87
    PCK_F12,            // F12            58   88
    PCK_MINUS,          // -_             59   89
    PCK_LEFTBRACKET,    // [{             5A   90
    PCK_QUOTE,          // '"             5B   91
    PCK_NOKEY_8,        // NOKEY          5C   92
    PCK_EQUALS,         // =+             5D   93
    PCK_RIGHTBRACKET,   // ]}             5E   94
    PCK_SLASH,          // /?             5F   95
    PCK_NOKEY_9,        // NOKEY          60   96
    PCK_NOKEY_10,       // NOKEY          61   97
    PCK_NOKEY_11,       // NOKEY          62   98
    PCK_NOKEY_12,       // NOKEY          63   99
    PCK_NOKEY_13,       // NOKEY          64  100
    PCK_NOKEY_14,       // NOKEY          65  101
    PCK_NOKEY_15,       // NOKEY          66  102
    PCK_LSHIFT,         // LSHIFT         67  103*
//  PCK_RSHIFT,         // RSHIFT         67  103*
    PCK_NOKEY_16,       // NOKEY          68  104
    PCK_NOKEY_17,       // NOKEY          69  105
    PCK_NOKEY_18,       // NOKEY          6A  106
    PCK_NOKEY_19,       // NOKEY          6B  107
    PCK_NOKEY_20,       // NOKEY          6C  108
    PCK_NOKEY_21,       // NOKEY          6D  109
    PCK_NOKEY_22,       // NOKEY          6E  110
    PCK_LCTRL,          // LCTRL          6F  111*
//  PCK_RCTRL,          // RCTRL          6F  111*
    PCK_NOKEY_23,       // NOKEY          70  112
    PCK_NOKEY_24,       // NOKEY          71  113
    PCK_NOKEY_25,       // NOKEY          72  114
    PCK_NOKEY_26,       // NOKEY          73  115
    PCK_NOKEY_27,       // NOKEY          74  116
    PCK_NOKEY_28,       // NOKEY          75  117
    PCK_NOKEY_29,       // NOKEY          76  118
    PCK_LALT,           // LALT           77  119*
//  PCK_RALT,           // RALT           77  119*

    PCK_HOME,           // HOME           78  120
    PCK_END,            // END            79  121
    PCK_PAGEDOWN        // PAGE DOWN      7A  122
   };

// PC Keys to be checked for events
static SDLKey pc_keys [PC_KEYS] =
   {
    SDLK_F1,            // F1             00    0
    SDLK_ESCAPE,        // ESC            01    1
    SDLK_TAB,           // TAB            02    2
    SDLK_PAUSE,         // BREAK          03    3  don't use SDLK_BREAK
    SDLK_NOKEY,         // NOKEY          04    4
    SDLK_KP0,           // KP 0           05    5
    SDLK_KP_PERIOD,     // KP DEL         06    6
    SDLK_SPACE,         // SP             07    7
    SDLK_F2,            // F2             08    8
    SDLK_1,             // 1!             09    9
    SDLK_q,             // q              0A   10
    SDLK_a,             // a              0B   11
    SDLK_NOKEY,         // NOKEY          0C   12
    SDLK_CAPSLOCK,      // CAPL           0D   13
    SDLK_PAGEUP,        // LINEFEED       0E   14  to right of CAPS LOCK
    SDLK_INSERT,        // INSERT         0F   15
    SDLK_F3,            // F3             10   16
    SDLK_2,             // 2@             11   17
    SDLK_w,             // w              12   18
    SDLK_s,             // s              13   19
    SDLK_KP_PLUS,       // KP PLUS        14   20
    SDLK_KP2,           // KP 2           15   21
    SDLK_KP3,           // KP 3           16   22
    SDLK_z,             // z              17   23
    SDLK_F4,            // F4             18   24
    SDLK_3,             // 3#             19   25
    SDLK_e,             // e              1A   26
    SDLK_d,             // d              1B   27
    SDLK_KP_MINUS,      // KP MINUS       1C   28
    SDLK_KP5,           // KP 5           1D   29
    SDLK_KP6,           // KP 6           1E   30
    SDLK_x,             // x              1F   31
    SDLK_F5,            // F5             20   32
    SDLK_4,             // 4$             21   33
    SDLK_r,             // r              22   34
    SDLK_f,             // f              23   35
    SDLK_KP_MULTIPLY,   // KP MULTIPLY    24   36
    SDLK_KP8,           // KP 8           25   37
    SDLK_KP9,           // KP 9           26   38
    SDLK_c,             // c              27   39
    SDLK_F6,            // F6             28   40
    SDLK_5,             // 5%             29   41
    SDLK_t,             // t              2A   42
    SDLK_g,             // g              2B   43
    SDLK_KP7,           // KP 7           2C   44
    SDLK_KP1,           // KP 1           2D   45
    SDLK_KP4,           // KP 4           2E   46
    SDLK_v,             // v              2F   47
    SDLK_F7,            // F7             30   48
    SDLK_6,             // 6^             31   49
    SDLK_y,             // y              32   50
    SDLK_h,             // h              33   51
    SDLK_KP_DIVIDE,     // KP DIVIDE      34   52
    SDLK_DOWN,          // DOWN           35   53
    SDLK_RIGHT,         // RIGHT          36   54
    SDLK_b,             // b              37   55
    SDLK_F8,            // F8             38   56
    SDLK_7,             // 7&             39   57
    SDLK_u,             // u              3A   58
    SDLK_j,             // j              3B   59
    SDLK_NOKEY,         // NOKEY          3C   60
    SDLK_LEFT,          // LEFT           3D   61
    SDLK_NOKEY,         // NOKEY          3E   62
    SDLK_n,             // n              3F   63
    SDLK_F9,            // F9             40   64
    SDLK_8,             // 8*             41   65
    SDLK_i,             // i              42   66
    SDLK_k,             // k              43   67
    SDLK_NOKEY,         // NOKEY          44   68
    SDLK_NOKEY,         // NOKEY          45   69
    SDLK_UP,            // UP             46   70
    SDLK_m,             // m              47   71
    SDLK_F10,           // F10            48   72
    SDLK_9,             // 9(             49   73
    SDLK_o,             // o              4A   74
    SDLK_l,             // l              4B   75
    SDLK_NOKEY,         // NOKEY          4C   76
    SDLK_BACKSPACE,     // BS             4D   77
    SDLK_RETURN,        // ENT            4E   78
    SDLK_COMMA,         // ,<             4F   79
    SDLK_F11,           // F11            50   80
    SDLK_0,             // 0)             51   81
    SDLK_p,             // p              52   82
    SDLK_SEMICOLON,     // ;:             53   83
    SDLK_DELETE,        // DEL            54   84
    SDLK_BACKQUOTE,     // `~             55   85
    SDLK_BACKSLASH,     // \|             56   86
    SDLK_PERIOD,        // .>             57   87
    SDLK_F12,           // F12            58   88
    SDLK_MINUS,         // -_             59   89
    SDLK_LEFTBRACKET,   // [{             5A   90
    SDLK_QUOTE,         // '"             5B   91
    SDLK_NOKEY,         // NOKEY          5C   92
    SDLK_EQUALS,        // =+             5D   93
    SDLK_RIGHTBRACKET,  // ]}             5E   94
    SDLK_SLASH,         // /?             5F   95
    SDLK_NOKEY,         // NOKEY          60   96
    SDLK_NOKEY,         // NOKEY          61   97
    SDLK_NOKEY,         // NOKEY          62   98
    SDLK_NOKEY,         // NOKEY          63   99
    SDLK_NOKEY,         // NOKEY          64  100
    SDLK_NOKEY,         // NOKEY          65  101
    SDLK_NOKEY,         // NOKEY          66  102
    SDLK_LSHIFT,        // LSHIFT         67  103*
//  SDLK_RSHIFT,        // RSHIFT         67  103*
    SDLK_NOKEY,         // NOKEY          68  104
    SDLK_NOKEY,         // NOKEY          69  105
    SDLK_NOKEY,         // NOKEY          6A  106
    SDLK_NOKEY,         // NOKEY          6B  107
    SDLK_NOKEY,         // NOKEY          6C  108
    SDLK_NOKEY,         // NOKEY          6D  109
    SDLK_NOKEY,         // NOKEY          6E  110
    SDLK_LCTRL,         // LCTRL          6F  111*
//  SDLK_RCTRL,         // RCTRL          6F  111*
    SDLK_NOKEY,         // NOKEY          70  112
    SDLK_NOKEY,         // NOKEY          71  113
    SDLK_NOKEY,         // NOKEY          72  114
    SDLK_NOKEY,         // NOKEY          73  115
    SDLK_NOKEY,         // NOKEY          74  116
    SDLK_NOKEY,         // NOKEY          75  117
    SDLK_NOKEY,         // NOKEY          76  118
    SDLK_LALT,          // LALT           77  119*
//  SDLK_RALT,          // RALT           77  119*

    SDLK_HOME,          // HOME           78  120
    SDLK_END,           // END            79  121
    SDLK_PAGEDOWN       // PAGE DOWN      7A  122
   };

static int key_256tc;

// PC status of all keys required for emulation. 0 if key up or 1 if key down.
static uint8_t pc_keystate[PC_KEYS];

// Key buffer, 256TC can store 9 down keys
static uint8_t key_buffer[32];

static int key_count;
static int key_get;
static int key_put;

static uint16_t port_18h;

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;

//==============================================================================
// Keyboard initialise
//
//   pass: void
// return: int                          0
//==============================================================================
int keytc_init (void)
{
 key_count = 0;
 key_get = 0;
 key_put = 0;
 key_256tc = 0;
 return 0;
}

//==============================================================================
// Keyboard de-initialise
//
//   pass: void
// return: int                          0
//==============================================================================
int keytc_deinit (void)
{
 return 0;
}

//==============================================================================
// Keyboard reset
//
//   pass: void
// return: int                          0
//==============================================================================
int keytc_reset (void)
{
 return 0;
}

//==============================================================================
// Key event handler.
//
// If any keys are in the buffer then the PIO port B bit 1 will be set and is
// cleared when port 0x18 is read.
//
//   pass: int down                     1 if key down event, else 0
// return: void
//==============================================================================
void keytc_key_event (int down)
{
 SDLKey key;
 int i;
 int match;

 key = emu.event.key.keysym.sym;
 match = 0;

 for (i = 0; i < PC_KEYS; i++)
    {
     if ((key == pc_keys[i]) ||
        ((i == PCK_LCTRL) && (key == SDLK_RCTRL)) ||
        ((i == PCK_LSHIFT) && (key == SDLK_RSHIFT)) ||
        ((i == PCK_LALT) && (key == SDLK_RALT)))
           {
            // ignore any keys that are already flagged as up or down
            if (down == pc_keystate[i])
               break;

            pc_keystate[i] = down; // 1 if down, 0 if up event
            match = 1;
            break;
           }
    }

  if (match)
     {
      emu.keyesc = pc_keystate[PCK_ESCAPE];
      if (key_count < sizeof(key_buffer))
         {
          i |= (0x80 * down);
#if 0
          if (down)
             xprintf("keytc_key_event: DOWN key code=%03d (0x%02X)\n", i, i);
          else
             xprintf("keytc_key_event:   UP key code=%03d (0x%02X)\n", i, i);
#endif
          key_buffer[key_put++] = i;
          if (key_put == sizeof(key_buffer))
             key_put = 0;
          key_count++;

          key_256tc = B8(00000010); // a key is ready
          pio_polling();  // needed for TC key interrupts to work well
         }
     }
}

//==============================================================================
// Key down event handler.
//
// This sets the key pressed state for the event key.
//
//   pass: void
// return: void
//==============================================================================
void keytc_keydown_event (void)
{
 keytc_key_event(1);
}

//==============================================================================
// Key up event handler.
//
// This clears the key pressed state for the event key.
//
//   pass: void
// return: void
//==============================================================================
void keytc_keyup_event (void)
{
 keytc_key_event(0);
}

//==============================================================================
// Test if a TC key code is ready.
//
// This is normally called from the PIO read of port B function.
//
// If any keys are in the buffer then the PIO port B bit 1 will be set and is
// cleared when port 0x18 is read.
//
//   pass: void
// return: int                          0x02 if key ready, else 0x00
//==============================================================================
int keytc_poll (void)
{
 return key_256tc;
}

//==============================================================================
// Force a character to be returned.
//
// This is useful for placing the boot code into monitor mode by forcing the 'M'
// key to be returned or for pasting.
//
//   pass: int scan                     key code
//   pass: int counts                   number of times to be forced
// return: void
//==============================================================================
void keytc_force (int scan, int counts)
{
}

//==============================================================================
// Force none.
//
// Forces no key matches for counts
//
//   pass: int counts                   number of times to be forced
// return: void
//==============================================================================
void keytc_force_none (int counts)
{
}

//==============================================================================
// Teleterm and 256TC port 0x18-0x1B keyboard port
//
// Reading port 0x18 should return the last key state if no new key is ready.
// The key ready status is cleared on a read of this port.
//
// Teleterm problems
// -----------------
// The Teleterm model appears to poll PIO B bit 1 for a key ready bit
// outside of the ISR but then this value is also read by the TC key
// interrupt ISR.  This method requires accurate emulation of the
// PIO port B and interrupt so that it happens at the same time.
//
// The emulator sets bit 1 of PIO B when a key is ready (port 18h) then an
// immediate call to the PIO polling function will detect the interrupt
// edge.  The PIO polling is then set to check after each Z80 instruction
// is emulated for a generous repeat count.  See notes in pio.c
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t                     key code
//==============================================================================
uint16_t keytc_r (uint16_t port, struct z80_port_read *port_s)
{
 int p = port & 0xff;

 if (! modelx.tckeys)
    return 0;

 if (p != 0x18)
   {
    if (modio.keytc)
       log_port_1("keytc_r", "data", port, 0);
    return 0;
   }

 // get the current key if any are in the buffer, a TC key interrupt poll
 // will have already been made by the event handler.
 if (key_count)
    {
     key_count--;
     port_18h = key_buffer[key_get++];
     if (key_get == sizeof(key_buffer))
        key_get = 0;
    }

 // if there are more keys in the buffer we need to set the key ready bit
 // and do a TC key interrupt poll.
 if (key_count)
    {
     key_256tc = B8(00000010);
     pio_polling();
    }
 else
    key_256tc = 0;

 if (modio.keytc)
    log_port_1("keytc_r", "data", port, port_18h);

 return port_18h;
}

//==============================================================================
// Teleterm 256TC port 0x18-0x1B keyboard port (write)
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void keytc_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
}
