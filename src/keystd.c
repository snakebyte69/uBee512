//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                          Standard Keyboard module                          *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulate the keys for the Standard Microbees (scan codes 0-63)
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
// v6.0.0 - 13 February 2017, uBee
// - Changes to getkeystate() for the 'stopshift' and 'makeshift' values. 
//   See notes in the function overhead.
//
// v4.7.0 - 4 June 2010, uBee
// - Lock keys up/down bug is fixed in SDL-1.2.14 when using the environment
//   variable SDL_DISABLE_LOCK_KEYS="1"
//
// v4.6.0 - 19 April 2010, uBee
// - Added keystd_proc_mod_args() function to process --keystd-mod arguments.
//
// v4.1.0 - 22 June 2009, uBee
// - Made improvements to the logging process.
//
// v4.0.0 - 13 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
//
// v3.1.0 - 8 January 2009, uBee
// - Moved reset and exit key detection over to one common area in keyb.c
// - Added code to handle CTRL+SHIFT+KEY combinations. If enabled this will
//   not use the shift/invert modification code for these keys.  This is
//   required by some systems for extended function keys to work.
// - Changed all printf() calls to use a local xprintf() function.
//
// v2.8.0 - 21 August 2008, uBee
// - Added emu.win32_lock_key_fix and emu.x11_lock_key_fix as found that the
//   LOCK key work around under my ubuntu 8.04.1 x11 no longer allows this
//   to work correctly as this key now works normally.  It appears as if X11
//   can send a command to set the behaviour of this key.  For now a
//   variable for win32 and x11 can be set using options. The x11 set up used
//   may be different from one system implementation to another.
//
// v2.7.0 - 22 May 2008, uBee
// - Added structure emu_t and moved variables into it.
//
// v2.6.0 - 6 May 2008, uBee
// - Added keystd_scan_set() and keystd_scan_clear() functions.  Some minor
//   changes made to getkeystate() function conerning the lock key.
//
// v2.3.0 - 9 January 2008, uBee
// - HOME or ALT keys now act as a control keys allowing more emulator
//   functionality to to be included, Tape rewind is now HOME+T.  Full screen
//   toggle is now HOME+ENTER.  All emulator key commands are now in keyb.c
// - Added modio_t structure.
//
// v2.1.0 - 29 October 2007, uBee
// - Added missing key combination of 'SHIFT 0'. On a Microbee no key was above
//   the '0' but is a ')' on a PC key board.  The 'Insert' key returns a code
//   of 0x20 but only when the 'SHIFT + Insert' is pressed.
// - Added 'M' key pressed detection flag.
//
// v2.0.0 - 15 October 2007, uBee
// - Created a new keyboard module specific to the standard model Microbees
//   as key emulation for the 256TC is to be implemented and will be contained
//   in it's own module.  The keyb.c module will call the appropriate module
//   functions at run time.
// - Microbee LINE FEED moved from KP_ENTER to PAGEUP, BREAK from KP_PLUS to
//   PAUSE/BREAK and F1 (tape) to HOME keys.  This is for consistency with
//   the 256TC keys.
// - Revision history specific to this module has been imported from the
//   keyb.c module.
//
// v1.4.0 - 2 October 2007, uBee
// - The main event handler has been moved to the ubee512.c module and will
//   call the required key handlers.
//
// v1.2.0 - 22 August 2007, uBee
// - Added F1 emulator action key to enable data to be read from a tape file.
// - Changed labels PCK_F11, PCK_F12 to PCK_PAGEDOWN and PCK_END
//
// v1.0.0 - 2 August 2007, uBee
// Combined emulator specific keys into the main key event loop.  Emulator
// fullscreen toggle keys added (ALT+ENTER).
//
// A major rewrite of this module to allow all keys to be emulated correctly
// and to incorporate the Premium cursor keys.  Key repeating at the correct
// speed is now working but this was related to the CRTC vertical blanking
// status code improvement in crtc.c.
//
// The key release code is not ideal at present. It should only release the
// key that was generated with the shift status at the time, so this code will
// incorrectly reset some Microbee key locations and may be an issue for multi
// keypresses (i.e in games).
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "keyb.h"
#include "keystd.h"
#include "crtc.h"
#include "vdu.h"
#include "ubee512.h"
#include "support.h"

//==============================================================================
// structures and variables
//==============================================================================
#define MB_KEYS 64
#define PC_KEYS 72

// This must be the same as the pc_keys table below it.
enum
   {
    PCK_a,
    PCK_b,
    PCK_c,
    PCK_d,
    PCK_e,
    PCK_f,
    PCK_g,
    PCK_h,
    PCK_i,
    PCK_j,
    PCK_k,
    PCK_l,
    PCK_m,
    PCK_n,
    PCK_o,
    PCK_p,
    PCK_q,
    PCK_r,
    PCK_s,
    PCK_t,
    PCK_u,
    PCK_v,
    PCK_w,
    PCK_x,
    PCK_y,
    PCK_z,
    PCK_LEFTBRACKET,
    PCK_BACKSLASH,
    PCK_RIGHTBRACKET,

    PCK_DELETE,

    PCK_INSERT,

    PCK_1,

    PCK_3,
    PCK_4,
    PCK_5,

    PCK_COMMA,

    PCK_PERIOD,
    PCK_SLASH,

    PCK_ESCAPE,
    PCK_BACKSPACE,
    PCK_TAB,
    PCK_PAGEUP,
    PCK_RETURN,
    PCK_CAPSLOCK,
    PCK_PAUSE,
    PCK_SPACE,
    PCK_UP,
    PCK_LCTRL,
    PCK_RCTRL,
    PCK_DOWN,
    PCK_LEFT,
    PCK_F4,
    PCK_F5,
    PCK_RIGHT,
    PCK_LSHIFT,
    PCK_RSHIFT,

    PCK_0,
    PCK_2,
    PCK_6,
    PCK_7,
    PCK_8,
    PCK_9,
    PCK_BACKQUOTE,
    PCK_QUOTE,
    PCK_SEMICOLON,
    PCK_MINUS,
    PCK_EQUALS,

    PCK_PAGEDOWN,
    PCK_END,
    PCK_LALT,
    PCK_RALT,
    PCK_HOME
   };

// PC Keys to be checked for events
static SDLKey pc_keys [PC_KEYS] =
   {
    SDLK_a,             // a          01
    SDLK_b,             // b          02
    SDLK_c,             // c          03
    SDLK_d,             // d          04
    SDLK_e,             // e          05
    SDLK_f,             // f          06
    SDLK_g,             // g          07
    SDLK_h,             // h          08
    SDLK_i,             // i          09
    SDLK_j,             // j          0A
    SDLK_k,             // k          0B
    SDLK_l,             // l          0C
    SDLK_m,             // m          0D
    SDLK_n,             // n          0E
    SDLK_o,             // o          0F
    SDLK_p,             // p          10
    SDLK_q,             // q          11
    SDLK_r,             // r          12
    SDLK_s,             // s          13
    SDLK_t,             // t          14
    SDLK_u,             // u          15
    SDLK_v,             // v          16
    SDLK_w,             // w          17
    SDLK_x,             // x          18
    SDLK_y,             // y          19
    SDLK_z,             // z          1A
    SDLK_LEFTBRACKET,   // [{         1B
    SDLK_BACKSLASH,     // \|         1C
    SDLK_RIGHTBRACKET,  // ]}         1D

    SDLK_DELETE,        // DEL        1F

    SDLK_INSERT,        // (SHIFT)0   20

    SDLK_1,             // 1!         21

    SDLK_3,             // 3#         23
    SDLK_4,             // 4$         24
    SDLK_5,             // 5%         25

    SDLK_COMMA,         // ,<         2C

    SDLK_PERIOD,        // .>         2E
    SDLK_SLASH,         // /?         2F

    SDLK_ESCAPE,        // ESC        30
    SDLK_BACKSPACE,     // BS         31
    SDLK_TAB,           // TAB        32
    SDLK_PAGEUP,        // PAGEUP     33  LINE FEED
    SDLK_RETURN,        // ENT        34
    SDLK_CAPSLOCK,      // CAPL       35
    SDLK_PAUSE,         // PAUSE      36  BREAK
    SDLK_SPACE,         // SP         37
    SDLK_UP,            // UP         38
    SDLK_LCTRL,         // LCTRL      39
    SDLK_RCTRL,         // RCTRL      39
    SDLK_DOWN,          // DOWN       3A
    SDLK_LEFT,          // LEFT       3B
    SDLK_F4,            // F4         3C
    SDLK_F5,            // F5         3D
    SDLK_RIGHT,         // RIGHT      3E
    SDLK_LSHIFT,        // LSHIFT     3F
    SDLK_RSHIFT,        // RSHIFT     3F

// The following keys change for upper and lower case
//                                L   INV    U    INV
    SDLK_0,             // 0)   0-20   -    )-29   -
    SDLK_2,             // 2@   2-22   -    @-00   Y
    SDLK_6,             // 6^   6-26   -    ^-1E   Y
    SDLK_7,             // 7&   7-27   -    &-26   -
    SDLK_8,             // 8*   8-28   -    *-2A   -
    SDLK_9,             // 9(   9-29   -    (-28   -
    SDLK_BACKQUOTE,     // `~   `-00   Y    ~-1E   -
    SDLK_QUOTE,         // '"   '-27   Y    "-22   -
    SDLK_SEMICOLON,     // ;:   ;-2B   -    :-2A   Y
    SDLK_MINUS,         // -_   --2D   -    _-1F   -
    SDLK_EQUALS,        // =+   =-2D   Y    +-2B   -

// The following keys are only for emulator usage
    SDLK_PAGEDOWN,      // PAGEDOWN   n/a
    SDLK_END,           // END        n/a
    SDLK_LALT,          // LALT       n/a
    SDLK_RALT,          // RALT       n/a
    SDLK_HOME           // HOME       n/a
   };

// Microbee codes for lower PC keys.  If a key is not implemented for
// the Microbee key use 0xff.
static uint8_t mb_scan_pclower [PC_KEYS] =
{
 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
 0x1F,
 0xFF,
 0x21,
 0x23, 0x24, 0x25,
 0x2C,
 0x2E, 0x2F,
 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
 0x38, 0x39, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E,
 0x3F, 0x3F,
 0x20, 0x22, 0x26, 0x27, 0x28, 0x29, 0x00, 0x27,
 0x2B, 0x2D, 0x2D,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// Microbee codes for shift PC keys.  If a key is not implemented for
// the Microbee key use 0xff.
static uint8_t mb_scan_pcshift [PC_KEYS] =
{
 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
 0x1F,
 0x20,
 0x21,
 0x23, 0x24, 0x25,
 0x2C,
 0x2E, 0x2F,
 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
 0x38, 0x39, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E,
 0x3F, 0x3F,
 0x29, 0x00, 0x1E, 0x26, 0x2A, 0x28, 0x1E, 0x22,
 0x2A, 0x1F, 0x2B,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// PC status of all keys required for emulation. 0 if key up or 1 if key down.
static uint8_t pc_keystate[PC_KEYS];

// MB status of all keys required for emulation. 0 if key up or 1 if key down.
static uint8_t mb_keystate[MB_KEYS];

// PC shift used for all Microbee keys
static uint8_t mb_invert[MB_KEYS];

static int stopshift;
static int makeshift;
static int scan_check;
static int forcescans;
static int forcenone;
static int havekeys;
static int skip_lock_test;

keystd_t keystd =
{
 .key_mod = KEYSTD_MOD_CTRL_SHIFT
};

extern emu_t emu;
extern modio_t modio;
extern crtc_t crtc;

//==============================================================================
// Keyboard initialise
//
//   pass: void
// return: int                          0
//==============================================================================
int keystd_init (void)
{
 return 0;
}

//==============================================================================
// Keyboard de-initialise
//
//   pass: void
// return: int                          0
//==============================================================================
int keystd_deinit (void)
{
 return 0;
}

//==============================================================================
// Keyboard reset
//
//   pass: void
// return: int                          0
//==============================================================================
int keystd_reset (void)
{
 return 0;
}

//==============================================================================
// Get the down status of a Microbee key.
//
// v6.0.0 has lowered the 'stopshift' and 'makeshift' values from 25 down to
// 8 in both cases.  These changes were made to allow an version of Early
// Word (EW) to work correctly.  'makeshift' values of 2,5,6,7 causes EW to
// fail, min 8 is required for EW.
//
//   pass: int scan                     scan code 0-63
// return: int                          1=key is down, else 0
//==============================================================================
static int getkeystate (int scan)
{
 static int last_capslock = 0;
 static int capslock_count = 3;

 int shift;

 // lock keys up/down bug is fixed in SDL v1.2.14 when using the environment
 // variable SDL_DISABLE_LOCK_KEYS="1" or "2" but the user can override the
 // value and older versions of SDL do not support the fix so we check to
 // see if it's in affect, if not we use the old semi-fix.
 if (! keystd.lockkey_fix)
    {
     // if CapsLock scan code and the old semi-fix is enabled
#ifdef MINGW
     if ((scan == 0x35) && (! skip_lock_test) && (emu.win32_lock_key_fix))
#else
     if ((scan == 0x35) && (! skip_lock_test) && (emu.x11_lock_key_fix))
#endif
        {
         if (mb_keystate[0x35] != last_capslock)
            {
             if (--capslock_count == 0)
                {
                 last_capslock = mb_keystate[0x35];
                 capslock_count = 3;
                }
             return 1;
            }
         return 0;
        }
    }

 // if force none then return state as 0
 if (forcenone)
    {
     forcenone--;
     return 0;
    }

 // if force scans and scan match then return state as 1
 if ((forcescans) && (scan_check == scan))
    {
     forcescans--;
     return 1;
    }

 // return 0 if stopping shift key actions
 if ((stopshift) && (scan == MB_KEYS-1))
    {
     stopshift--;
     return 0;
    }

 // return 1 if making shift key actions
 if ((makeshift) && (scan == MB_KEYS-1))
    {
     makeshift--;
     return 1;
    }

 shift = mb_keystate[0x3F];

 if (mb_keystate[scan])
    {
     if (shift)
        {
#ifdef use_debug_getkeystate_shift
         xprintf("SHIFT - mb scan=%d  invert=%d\n", scan, mb_invert[scan]);
#endif
         if (! mb_invert[scan])
            return 1;                   // return a normal key
         else
            {
             stopshift = 8;            // wer'e inverting to lower !             
             return 1;
            }
        }
     else
        {
#ifdef use_debug_getkeystate_lower
         xprintf("LOWER - mb scan=%d  invert=%d\n", scan, mb_invert[scan]);
#endif
         if (! mb_invert[scan])
            return 1;                   // return a normal key
         else
            {
             makeshift = 8;            // wer'e inverting to SHIFT !
             return 1;
            }
        }
    }
 else
    return 0;
}

//==============================================================================
// key down event handler.
//
// This creates the Microbee scan keys and determines what inverting action
// may be required for each Microbee key generated.
//
//   pass: void
// return: void
//==============================================================================
void keystd_keydown_event (void)
{
 SDLKey key;
 int i;
 int scan;

 key = emu.event.key.keysym.sym;

 for (i = 0; i < PC_KEYS; i++)
    {
     if (key == pc_keys[i])
        {
         pc_keystate[i] = 1;
         // if a SHIFT key is currently down
         if (pc_keystate[PCK_LSHIFT] || pc_keystate[PCK_RSHIFT])
            {
             scan = mb_scan_pcshift[i];
             if (scan != 255)
                {
                 havekeys++;
                 // if a CTRL key is currently down and modified handling is on
                 if ((pc_keystate[PCK_LCTRL] || pc_keystate[PCK_RCTRL]) && (keystd.key_mod & KEYSTD_MOD_CTRL_SHIFT))
                    {
                     scan = mb_scan_pclower[i];
                     mb_invert[scan] = 0;
                    }
                 else
                    {
                     if ((key == SDLK_2) || (key == SDLK_6) || (key == SDLK_SEMICOLON))
                        mb_invert[scan] = 1;
                     else
                        mb_invert[scan] = 0;
                    }
                }
            }
         else
            {
             scan = mb_scan_pclower[i];
             if (scan != 255)
                {
                 havekeys++;
                 if ((key == SDLK_BACKQUOTE) || (key == SDLK_QUOTE) || (key == SDLK_EQUALS))
                    mb_invert[scan] = 1;
                 else
                    mb_invert[scan] = 0;
                }
            }
         if (scan != 255)
            mb_keystate[scan] = 1;
         break;
        }
    }

 emu.keyesc = mb_keystate[mb_scan_pclower[PCK_ESCAPE]];
 emu.keym = mb_keystate[mb_scan_pclower[PCK_m]];
}

//==============================================================================
// key up event handler.
//
// This creates the Microbee scan keys and determines what inverting action
// may be required for each Microbee key generated.
//
//   pass: void
// return: void
//==============================================================================
void keystd_keyup_event (void)
{
 SDLKey key;
 int i;

 key = emu.event.key.keysym.sym;

 for (i = 0; i < PC_KEYS; i++)
    {
     if (key == pc_keys[i])
        {
         pc_keystate[i] = 0;
         // shift = (pc_keystate[PCK_LSHIFT] | pc_keystate[PCK_RSHIFT]);
         // The following is not ideal as some multikey presses may be stopped.
         if (mb_scan_pcshift[i] != 255)
            mb_keystate[mb_scan_pcshift[i]] = 0;
         if (mb_scan_pclower[i] != 255)
            mb_keystate[mb_scan_pclower[i]] = 0;
         break;
        }
    }

 emu.keyesc = mb_keystate[mb_scan_pclower[PCK_ESCAPE]];
 emu.keym = mb_keystate[mb_scan_pclower[PCK_m]];
}

//==============================================================================
// Scan 1 key at the address passed.  If key is detected then the light pen
// register strobe is set.  This is called when the crtc lpen address has been
// set.
//
//   pass: int addr                     address of light pen
// return: void
//==============================================================================
void keystd_handler (int addr)
{
 int scan;

 scan = (addr >> 4) & 0x3F;

 if (modio.keystd)
    log_data_2("keystd_handler", "addr", "scan", addr, scan);

 event_handler();

 if (getkeystate(scan))
    crtc_lpen(addr);
}

//==============================================================================
// Scan all keys and setting the light pen registers to first found depressed
// key. This is called from the crtc.c for the 6545 status register.
//
//   pass: void
// return: void
//==============================================================================
void keystd_checkall (void)
{
 int i;

 event_handler();

 if (! crtc.latchrom)
    {
     for (i = 0; i < MB_KEYS; i++)
        if (getkeystate(i))
           {
            crtc_lpen(i << 4);
            break;
           }
    }
}

//==============================================================================
// Force a character to be returned.
//
// This is useful for placing the boot code into monitor mode by forcing the 'M'
// key (0x0d) to be returned or for pasting.
//
//   pass: int scan                     scan code 0-63
//   pass: int counts                   number of times to be forced
// return: void
//==============================================================================
void keystd_force (int scan, int counts)
{
 scan_check = scan;
 forcescans = counts;
}

//==============================================================================
// Force none.
//
// Forces no scan matches for counts
//
//   pass: int counts                   number of times to be forced
// return: void
//==============================================================================
void keystd_force_none (int counts)
{
 forcenone = counts;
}

//==============================================================================
// Set a scan code (key down)
//
//   pass: int scan                     scan code 0-63
// return: void
//==============================================================================
void keystd_scan_set (int scan)
{
 mb_keystate[scan] = 1;
 mb_invert[scan] = 0;
 if (scan == 0x35)
    skip_lock_test = 1;
}

//==============================================================================
// Clear a scan code (key up)
//
//   pass: int scan                     scan code 0-63
// return: void
//==============================================================================
void keystd_scan_clear (int scan)
{
 mb_keystate[scan] = 0;
 if (scan == 0x35)
    skip_lock_test = 0;
}

//==============================================================================
// Process --keystd-mod arguments.
//
//   pass: int arg              argument number (0=all)
//         int pf               prefix used 0='-', 1='+'
// return: void
//==============================================================================
void keystd_proc_mod_args (int arg, int pf)
{
 switch (arg)
    {
     case  0 :
        keystd.key_mod = KEYSTD_MOD_ALL * pf;
        break;
     case  1 :
        keystd.key_mod = (keystd.key_mod & ~KEYSTD_MOD_CTRL_SHIFT) |
        (KEYSTD_MOD_CTRL_SHIFT * pf);
        break;
    }
}
