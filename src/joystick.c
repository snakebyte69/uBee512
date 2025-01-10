//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              Joystick module                               *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// This module is used to provide joystick support.
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
// v5.0.0 - 2 August 2010, uBee
// - Changes to joystick commands to use an optional command SHIFT button.
//   This will allow using the same buttons for keys and commands.
// - Added the joystick_kbjoy_keybuttons() function.
// - Changed joystick_mbjoy_set_action() to take multiple button values.
// - Added code to joystick_hatmotion_event() to convert hat values to buttons.
// - Code to disable conversion of axis values to buttons was missing, this
//   has now been added and can be disabled with --js-axis=off.
// - Changes to modio.joystick to also report pseudo buttons.
// - New default joystick values are used and enabled by default.
//
// v4.7.0 - 17 June 2010, K Duckmanton
// - Changes to allow several different devices to be connected to the
//   emulated parallel port.
//
// v4.6.0 - 4 May 2010, uBee
// - Renamed log_data() calls to log_data_1().
// - Added C_DASML command for disassembler listing and changed emu_commands[]
//   ordering to match enumerated command order.
//
// v4.1.0 - 22 June 2009, uBee
// - Made improvements to the logging process.
//
// v4.0.0 - 13 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
//
// v3.1.0 - 7 November 2008, uBee
// - Changed functions function_*() to keyb_*() as code was moved.
// - Changed all printf() calls to use a local xprintf() function.
//
// v3.0.0 - 24 September 2008, uBee
// - Added C_WINI and C_WIND commands for increasing/decreasing window size.
// - Added C_GLFILT command for toggling of OpenGL filter mode, and C_VSIZE1
//   command for video resizing.
// - Added C_MWHEEL command to select mouse wheel association.
//
// v2.8.0 - 3 September 2008, uBee
// - JS key commands can now be held down and the commands repeated.
// - Added mapping of joystick buttons to emulator (EMUKEY style) commands and
//   joystick_kbjoy_listcommands() function to list the command names.
//
// v2.7.0 - 4 June 2008, uBee
// - Made Hat motion reportable if +joystick used in --modio option.
// - Fixed 12 button limitation, now is 128 buttons as originally intended.
// - Added structure emu_t and moved variables into it.
//
// v2.6.0 - 10 May 2008, uBee
// - Created a new file and implement the joystick emulation.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <SDL.h>

#include "ubee512.h"
#include "joystick.h"
#include "support.h"
#include "keyb.h"
#include "keystd.h"
#include "parint.h"
#include "pio.h"

#include "macros.h"

//==============================================================================
// Game controller layout and default configuration.
//
// The default values are defined in structures joystick_mbjoy_def[] and
// joystick_kbjoy_def[] and applied in joystick_init().
//==============================================================================
/*
//
//                  A (top)                      C (top)
//                  B (bot)                      D (bot)
//                 +-------+                    +-------+
//                /         +------------------+         \
//               /                                        \
//              /     E                              K     \
//             /      |                              |      \
//            /    H-- --F        I      J        N-- --L    \
//           /        |                              |        \
//          /         G                              M         \
//         /                  O    ANALOG    T                  \
//        /                   |              |                   \
//       /                 R--S--P        W--X--U                 \
//      /                     |              |                     \
//     /              +-+     Q    +----+    V     +-+              \
//    /              /   \        /      \        /   \              \
//   +              /     +------+        +------+     \              +
//   |             /                                    \             |
//   +            /        TOP VIEW OF A TYPICAL         \            +
//    \          /            GAME CONTROLLER             \          /
//      +------+                                            +------+
//
//
//      Joystick (PIO A)    Button Number    (Shift function)
//      ----------------    -------------    ----------------
//  A : Player 1            0x04             (MUTE)
//  B : SPARE               0x06             (FULLSCREEN)
//  C : Player 2            0x05             (PAUSE)
//  D :                     0x07             (SHIFT BUTTON)
//  E : Up                  0x80/0x90        (VOLUME+)
//  F : Right               0x81/0x91        (WINDOW+)
//  G : Down                0x82/0x92        (VOLUME-)
//  H : Left                0x83/0x93        (WINDOW-)
//  I : Player 1            0x08
//  J : Player 2            0x09
//  K : Fire                0x00
//  L : Fire                0x01
//  M : SPARE               0x02
//  N : SPARE               0x03
//  O : Up                  0x80/0x90        (VOLUME+)
//  P : Right               0x81/0x91        (WINDOW+)
//  Q : Down                0x82/0x92        (VOLUME-)
//  R : Left                0x83/0x93        (WINDOW-)
//  S :                     0x0a
//  T : Fire                0x00
//  U : Fire                0x01
//  V : SPARE               0x02
//  W : SPARE               0x03
//  X : Fire                0x0b
//
// Note: The values of 0x8x and 0x9x are Axis and Hat values after being
// converted to buttons numbers.  The values shown are using the default
// base values.  Values less than 0x80 are typical button numbers returned
// by a game controller.
*/

//==============================================================================
// prototypes
//==============================================================================
uint8_t joystick_r (void);

//==============================================================================
// structures and variables
//==============================================================================
typedef struct button_states_t
{
 int up;
 int down;
 int left;
 int right;
}button_states_t;

typedef struct joystick_actions_t
{
 int action;
 char buttons[50];
}joystick_actions_t;

joystick_t joystick =
{
 .joy = NULL,
 .used = -1,
 .mbee = 1,
 .shift_button = JOY_SHIFT_BTN,
 .axis_used = 1,
 .axis_buttons = 0x80,
 .axis_level = 3200,
 .hat_used = 1,
 .hat_buttons = 0x90
};

parint_ops_t joystick_ops =
{
 .init = NULL,    // Joystick initialisation and
 .deinit = NULL,  // deinitialisation is done elsewhere
 .reset = NULL,
 .poll = NULL,
 .ready = NULL,
 .strobe = NULL,  // never called
 .read = &joystick_r,
 .write = NULL,
};

static const button_states_t hat_values[] =
{
// U  D  L  R
  {1, 0, 0, 0}, // up
  {0, 1, 0, 0}, // down
  {0, 0, 1, 0}, // left
  {0, 0, 0, 1}, // right
  {0, 0, 0, 0}, // centered
  {1, 0, 1, 0}, // left up
  {1, 0, 0, 1}, // right up
  {0, 1, 1, 0}, // left down
  {0, 1, 0, 1}  // right down
};

static int kb_clear_buffer = 1;
static int kb_scan;
static int kb_no_init;
static int js_no_init;
static int button_shift_last;

static button_states_t axis_button;
static button_states_t hat_button;

static int cmd_scan;

extern emu_t emu;
extern modio_t modio;

//==============================================================================
// Structures and variables related to Joystick to 6545 key and emulator
// commands mappings.
//
// The code supports game controllers that can return up to 128 buttons.
// A further 128 pseudo buttons are allowed for axis and Hat values and 256
// shifted button states.
//
// 0 - 127 : Normal joystick buttons. Pseudo buttons may also use this range
//           but is not recommended.
//
// 128-255 : Pseudo buttons, these have been converted from Axis and Hat
//           values returned by the joystick and may be configured to a base
//           number anywhere in this area.
//
// 256-511 : Shifted Normal and Pseudo buttons.
//==============================================================================

// This array holds the joystick PIO A port value for every possible JS
// button.  Different buttons are allowed to affect the same PIO bit value.
// The array should cleared before doing any reconfiguring.
static uint8_t mb_button_data[512];

// joystick buttons mapped to keyboard keys sets, first 128 buttons are
// unshifted followed by 128 shifted values.
static uint8_t kb_button_data[JOY_KB_SETS][512];

// pointer to the current mapped buttons to keys set
static uint8_t *kb_button_data_use = *kb_button_data;

// buffer for building a mapped buttons to keys set
static uint8_t kb_button_data_buf[512];

// Search table of Microbee keys and corresponding matrix codes (6545 CRTC)
static char *key_to_6545[] =
{
 "@",     "A",     "B",     "C",     "D",     "E",     "F",     "G",
 "H",     "I",     "J",     "K",     "L",     "M",     "N",     "O",
 "P",     "Q",     "R",     "S",     "T",     "U",     "V",     "W",
 "X",     "Y",     "Z",     "[",     "\\",    "]",     "^",     "DEL",
 "0",     "1",     "2",     "3",     "4",     "5",     "6",     "7",
 "8",     "9",     ":",     ";",     ",",     "-",     ".",     "/",
 "ESC",   "BS",    "TAB",   "LF",    "CR",    "LOCK",  "BRK",   "SP",
 "UP",    "CTRL",  "DOWN",  "LEFT",  "60",    "61",    "RIGHT", "SHIFT",
 ""
};

// Search table of emulator commands.  This must match the EMU_CMD_NAME
// number ordering.  If any commands are not supported then padding is still
// required.
static char *emu_commands[] =
{
 "C_DMP",
 "C_DMPN1",
 "C_DMPN2",
 "C_DMPB1",
 "C_DMPB2",
 "C_DMPREP",
 "C_DMPREG",
 "C_DBOFF",
 "C_DBON",
 "C_DBTRA",
 "C_DBST1",
 "C_DBST10",
 "C_DBST20",
 "C_DASML",
 "C_PAUSE",

 "C_FSTOG",
 "C_TAPER",
 "C_JS",
 "C_MUTE",
 "C_VOLU",
 "C_VOLD",
 "C_WINI",
 "C_WIND",
 "C_VSIZE1",
 "C_GLFILT",
 "C_MWHEEL",
 ""
};

// default Microbee joystick action (PIO port A) values.
static joystick_actions_t joystick_mbjoy_def[] =
{
 {JOY_MB_UP,    "0x80, 0x90"},
 {JOY_MB_RIGHT, "0x81, 0x91"},
 {JOY_MB_DOWN,  "0x82, 0x92"},
 {JOY_MB_LEFT,  "0x83, 0x93"},
 {JOY_MB_FIRE,  "0x00, 0x01, 0x0b"},
 {JOY_MB_PLAY1, "0x04, 0x08"},
 {JOY_MB_PLAY2, "0x05, 0x09"},
 {JOY_MB_SPARE, "0x02, 0x03, 0x06"},
 {0, ""}
};

// default joystick mapped buttons to keys values.
static char *joystick_kbjoy_def[] =
{
 "C_VOLU,  0x180, 0x190",
 "C_VOLD,  0x182, 0x192",
 "C_WINI,  0x181, 0x191",
 "C_WIND,  0x183, 0x193",
 "C_MUTE,  0x104",
 "C_PAUSE, 0x105",
 "C_FSTOG, 0x106",
 NULL
};

//==============================================================================
// Joystick Initialise.
//
//   pass: void
// return: int                          0 if no error, else -1
//==============================================================================
int joystick_init (void)
{
 int i;

 joystick.data = B8(11111111);

 if (joystick.used < 0)
    return 0; // consider it a warning only

 SDL_JoystickEventState(SDL_ENABLE);

 joystick.joy = SDL_JoystickOpen(joystick.used);

 if (! joystick.joy)
    {
     xprintf("joystick_init: Failed to open joystick device: %d\n", joystick.used);
     return 0;  // consider it a warning only
    }

 if (modio.joystick)
    {
     xprintf("joystick_init:\n");
     xprintf("Joystick: %d\n", joystick.used);
     xprintf("Name: %s\n", SDL_JoystickName(joystick.used));
     xprintf("Number of Axes: %d\n", SDL_JoystickNumAxes(joystick.joy));
     xprintf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joystick.joy));
     xprintf("Number of Balls: %d\n", SDL_JoystickNumBalls(joystick.joy));

     if (modio.level)
        {
         fprintf(modio.log, "joystick_init:\n");
         fprintf(modio.log, "Joystick: %d\n", joystick.used);
         fprintf(modio.log, "Name: %s\n", SDL_JoystickName(joystick.used));
         fprintf(modio.log, "Number of Axes: %d\n", SDL_JoystickNumAxes(joystick.joy));
         fprintf(modio.log, "Number of Buttons: %d\n", SDL_JoystickNumButtons(joystick.joy));
         fprintf(modio.log, "Number of Balls: %d\n", SDL_JoystickNumBalls(joystick.joy));
        }
    }

 // set up the default Microbee joystick action (PIO port A) values
 if (! js_no_init)
    {
     joystick_mbjoy_clear();
     i = -1;
     while (joystick_mbjoy_def[++i].buttons[0])
        joystick_mbjoy_set_action(joystick_mbjoy_def[i].action, joystick_mbjoy_def[i].buttons);
    }

 // set up the default joystick mapped buttons to 6545 keys/commands
 if (! kb_no_init)
    {
     i = -1;
     while (joystick_kbjoy_def[++i])
        joystick_kbjoy_keybuttons(joystick_kbjoy_def[i]);
     joystick_kbjoy_set(0, "A");
     joystick_kbjoy_select(0, "A");
    }

 return 0;
}

//==============================================================================
// Joystick de-initialise.
//
//   pass: void
// return: int                          0
//==============================================================================
int joystick_deinit (void)
{
 if (joystick.joy)
    SDL_JoystickClose(joystick.joy);

 return 0;
}

//==============================================================================
// Joystick reset.
//
//   pass: void
// return: int                          0
//==============================================================================
int joystick_reset (void)
{
 if (joystick.joy)
    {
    }

 return 0;
}

//==============================================================================
// Joystick button down
//
//   pass: int button                   normal/pseudo button value (unshifted)
// return: void
//==============================================================================
static void joystick_buttondown (int button)
{
 int scan;
 int button_value;

 if (modio.joystick)
    log_data_1("joystick_button down", "button", button);

 if (button >= JOY_SHIFT_BASE)
    return;

 // if the button pressed was the shift button
 if (joystick.shift_button == button)
    {
     joystick.shift_inuse = 1;
     return;
    }

 // if the shift button is down change the shifted button's value
 if (joystick.shift_inuse)
    {
     button_value = button + JOY_SHIFT_BASE;
     button_shift_last = button_value;
    }
 else
    button_value = button;

 joystick.data ^= mb_button_data[button_value];

 if (! joystick.kbd)
    return;

 scan = kb_button_data_use[button_value];

 if (! scan)
    return;

 if (scan < 65)
    keystd_scan_set(scan-1);
 else
    if ((scan-65) != EMU_CMD_JOYSTICK)
       {
        cmd_scan = scan;
        keyb_repeat_start();
        keyb_emu_command(scan-65, 0);
       }
}

//==============================================================================
// Joystick button down event
//
//   pass: void
// return: void
//==============================================================================
void joystick_buttondown_event (void)
{
 joystick_buttondown(emu.event.jbutton.button);

#if 0
 if (modio.joystick)
    log_mesg("joystick_buttondown_event");
#endif
}

//==============================================================================
// Joystick button up
//
//   pass: int button                   normal/pseudo button value (unshifted)
// return: void
//==============================================================================
static void joystick_buttonup (int button)
{
 int scan;
 int button_value;

 if (modio.joystick)
    log_data_1("joystick_buttonup", "button", button);

 if (button >= JOY_SHIFT_BASE)
    return;

 // if the button released was the shift button
 if (joystick.shift_button == button)
    {
     joystick.shift_inuse = 0;
     return;
    }

 // if the shift button is down change the shifted button's value
 if (joystick.shift_inuse)
    {
     button_value = button + JOY_SHIFT_BASE;
     button_shift_last = 0;
    }
 else
    {
     // handle button where the shift button was released before the button
     if ((button + JOY_SHIFT_BASE) == button_shift_last)
        {
         button_value = button_shift_last;
         button_shift_last = 0;
        }
     else
        button_value = button;
    }

 joystick.data |= mb_button_data[button_value];

 if (! joystick.kbd)
    return;

 scan = kb_button_data_use[button_value];

 if ((scan) && (scan < 65))
    keystd_scan_clear(scan-1);
 else
    {
     if (cmd_scan == scan)
        keyb_repeat_stop();
    }
}

//==============================================================================
// Joystick button up event
//
//   pass: void
// return: void
//==============================================================================
void joystick_buttonup_event (void)
{
 joystick_buttonup(emu.event.jbutton.button);

#if 0
 if (modio.joystick)
    log_mesg("joystick_buttonup_event");
#endif
}

//==============================================================================
// Joystick hat motion event
//
//   pass: void
// return: void
//==============================================================================
void joystick_hatmotion_event (void)
{
 button_states_t *hat;
 int x;

 switch (emu.event.jhat.value)
    {
     case SDL_HAT_UP :
        x = 0;
        if (modio.joystick)
           log_mesg("joystick_hatmotion_event: Hat up");
        break;
     case SDL_HAT_DOWN :
        x = 1;
        if (modio.joystick)
           log_mesg("joystick_hatmotion_event: Hat down");
        break;
     case SDL_HAT_LEFT :
        x = 2;
        if (modio.joystick)
           log_mesg("joystick_hatmotion_event: Hat left");
        break;
     case SDL_HAT_RIGHT :
        x = 3;
        if (modio.joystick)
           log_mesg("joystick_hatmotion_event: Hat right");
        break;
     case SDL_HAT_CENTERED :
        x = 4;
        if (modio.joystick)
           log_mesg("joystick_hatmotion_event: Hat centered");
        break;
     case SDL_HAT_LEFTUP :
        x = 5;
        if (modio.joystick)
           log_mesg("joystick_hatmotion_event: Hat left up");
        break;
     case SDL_HAT_RIGHTUP :
        x = 6;
        if (modio.joystick)
           log_mesg("joystick_hatmotion_event: Hat right up");
        break;
     case SDL_HAT_LEFTDOWN :
        x = 7;
        if (modio.joystick)
           log_mesg("joystick_hatmotion_event: Hat left down");
        break;
     case SDL_HAT_RIGHTDOWN :
        x = 8;
        if (modio.joystick)
           log_mesg("joystick_hatmotion_event: Hat right down");
        break;
     default :
        x = 4;
        break;
    }

 // exit if hat to button conversion is disabled
 if (! joystick.hat_used)
    return;

 hat = (button_states_t*)&hat_values[x];

 // set joystick hat button states off
 if ((hat_button.up != hat->up) && (! hat->up))
    joystick_buttonup(joystick.hat_buttons + 0);
 if ((hat_button.right != hat->right) && (! hat->right))
    joystick_buttonup(joystick.hat_buttons + 1);
 if ((hat_button.down != hat->down) && (! hat->down))
    joystick_buttonup(joystick.hat_buttons + 2);
 if ((hat_button.left != hat->left) && (! hat->left))
    joystick_buttonup(joystick.hat_buttons + 3);

 // set joystick hat button states on
 if ((hat_button.up != hat->up) && (hat->up))
    joystick_buttondown(joystick.hat_buttons + 0);
 if ((hat_button.right != hat->right) && (hat->right))
    joystick_buttondown(joystick.hat_buttons + 1);
 if ((hat_button.down != hat->down) && (hat->down))
    joystick_buttondown(joystick.hat_buttons + 2);
 if ((hat_button.left != hat->left) && (hat->left))
    joystick_buttondown(joystick.hat_buttons + 3);

 // update the current hat button states
 memcpy(&hat_button, hat, sizeof(hat_button));
}

//==============================================================================
// Joystick Axis motion event
//
// The joystick may need to be calibrated to work correctly.
//
//   pass: void
// return: void
//==============================================================================
void joystick_axismotion_event (void)
{
 if (modio.joystick)
    {
     if (emu.event.jaxis.axis == 0)
        log_data_2("joystick_axismotion_event", "X axis", "X axis value",
        emu.event.jaxis.axis, emu.event.jaxis.value);

     if (emu.event.jaxis.axis == 1)
        log_data_2("joystick_axismotion_event", "Y axis", "Y axis value",
        emu.event.jaxis.axis, emu.event.jaxis.value);
    }

 // exit if axis to button conversion is disabled
 if (! joystick.axis_used)
    return;

 // X axis movement
 if (emu.event.jaxis.axis == 0)
    {
     if ((emu.event.jaxis.value >= -joystick.axis_level) && (axis_button.left))
        {
         axis_button.left = 0;
         joystick_buttonup(joystick.axis_buttons + 3);
        }
     if ((emu.event.jaxis.value <= joystick.axis_level) && (axis_button.right))
        {
         axis_button.right = 0;
         joystick_buttonup(joystick.axis_buttons + 1);
        }

     if ((emu.event.jaxis.value < -joystick.axis_level) && (! axis_button.left))
        {
         if (axis_button.right)
            {
             axis_button.right = 0;
             joystick_buttonup(joystick.axis_buttons + 1);
            }
         axis_button.left = 1;
         joystick_buttondown(joystick.axis_buttons + 3);
         return;
        }

    if ((emu.event.jaxis.value > joystick.axis_level) && (! axis_button.right))
       {
        if (axis_button.left)
           {
            axis_button.left = 0;
            joystick_buttonup(joystick.axis_buttons + 3);
           }
        axis_button.right = 1;
        joystick_buttondown(joystick.axis_buttons + 1);
        return;
       }
    }

 // Y axis movement
 if (emu.event.jaxis.axis == 1)
    {
     if ((emu.event.jaxis.value >= -joystick.axis_level) && (axis_button.up))
        {
         axis_button.up = 0;
         joystick_buttonup(joystick.axis_buttons + 0);
        }
     if ((emu.event.jaxis.value <= joystick.axis_level) && (axis_button.down))
        {
         axis_button.down = 0;
         joystick_buttonup(joystick.axis_buttons + 2);
        }

     if ((emu.event.jaxis.value < -joystick.axis_level) && (! axis_button.up))
        {
         if (axis_button.down)
            {
             axis_button.down = 0;
             joystick_buttonup(joystick.axis_buttons + 2);
            }
         axis_button.up = 1;
         joystick_buttondown(joystick.axis_buttons + 0);
         return;
        }

     if ((emu.event.jaxis.value > joystick.axis_level) && (! axis_button.down))
        {
         if (axis_button.up)
            {
             axis_button.up = 0;
             joystick_buttonup(joystick.axis_buttons + 0);
            }
         axis_button.down = 1;
         joystick_buttondown(joystick.axis_buttons + 2);
         return;
        }
    }
}

//==============================================================================
// Clear all the Microbee joystick button actions.
//
//   pass: void
// return: void
//==============================================================================
void joystick_mbjoy_clear (void)
{
 memset(mb_button_data, 0, sizeof(mb_button_data));
}

//==============================================================================
// Set a Microbee joystick action to be associated with joystick button(s).
//
// --js-ACTION=n[,n..]
//
// Associate ACTION with with joystick button(s) 'n'
//
// When this function is called the variable 'js_no_init' will be set to
// prevent joystick_init() function from overwriting the values.
//
//   pass: int action
//         char *p                      parameter
// return: int                          0 if no error, else -1
//==============================================================================
int joystick_mbjoy_set_action (int action, char *p)
{
 char *c;
 char sp[100];

 int button;

 js_no_init = 1;

 if ((action < 0) || (action > 255))
    return -1;

 c = get_next_parameter(p, ',', sp, &button, sizeof(sp)-1);

 if ((button < 0) || (button >= sizeof(mb_button_data)))
    return -1;

 mb_button_data[button] = action;

 while (c != NULL)
    {
     c = get_next_parameter(c, ',', sp, &button, sizeof(sp)-1); // get a button

     if ((button < 0) || (button >= sizeof(mb_button_data)))
        return -1;

     mb_button_data[button] = action;
    }

 return 0;
}

//==============================================================================
// Associate a joystick mapped button key to a 6545 scan code or emulator
// command value.  The value is saved in kb_scan for later use.
//
//   pass: char *key
// return: int                          0 if no error, else -1
//==============================================================================
int joystick_kbjoy_key (char *key)
{
 int scan;
 char keyx[10];

 strncpy(keyx, key, sizeof(keyx));
 keyx[sizeof(keyx)-1] = 0;

 toupper_string(keyx, keyx);

 scan = string_search(key_to_6545, keyx);
 if (scan == -1)
    {
     scan = string_search(emu_commands, keyx);
     if (scan == -1)
        return -1;
     scan += 64;
    }

 // scan code is 0-63 for 6545 keys, or 64 or greater if a joystick command
 kb_scan = scan;

 if (kb_clear_buffer)
    {
     memset(kb_button_data_buf, 0, sizeof(kb_button_data_buf));
     kb_clear_buffer = 0;
    }

 return 0;
}

//==============================================================================
// Save a button in the mapped buttons buffer.
//
// When this function is called the variable 'kb_no_init' will be set to
// prevent joystick_init() function from overwriting the values.
//
//   pass: int button                   0-255, shifted buttons are 256-511.
// return: int                          0 if no error, else -1
//==============================================================================
int joystick_kbjoy_button (int button)
{
 kb_no_init = 1;

 if ((button < 0) || (button >= sizeof(kb_button_data_buf)))
    return -1;

 kb_button_data_buf[button] = kb_scan+1;

 return 0;
}

//==============================================================================
// Process --js-kkb option.
//
// --js-kkb=k,n[,n..]
//
// Associate the key 'k' with joystick button(s) 'n'.  Shifted buttons are
// passed as values 128-255.
//
//   pass: char *p                      parameter
// return: int                          0 if no error else -1
//==============================================================================
int joystick_kbjoy_keybuttons (char *p)
{
 char *c;
 char sp[100];

 int temp;
 int value;

 c = get_next_parameter(p, ',', sp, &temp, sizeof(sp)-1);

 if (strcasecmp(sp, "COMMA") == 0)
    strcpy(sp, ",");

 if (joystick_kbjoy_key(sp) == -1)
    return -1;

 if (c == NULL) // must be at least one 'n' value
    return -1;

 while (c != NULL)
    {
     c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1); // get a value
     if (sp[0])
        {
         if (joystick_kbjoy_button(value) == -1)
            return -1;
        }
    }

 return 0;
}

//==============================================================================
// Move the joystick key mappings to storage
//
//   pass: int set
//         char *set_str
// return: int                          0 if no error, else -1
//==============================================================================
int joystick_kbjoy_set (int set, char *set_str)
{
 char set_chr;

 if (strlen(set_str) == 1)
    {
     set_chr = toupper(set_str[0]);
     if ((set_chr >= 'A') && (set_chr <= 'Z'))
        set = set_chr - 'A';
    }

 if ((set < 0) || (set >= JOY_KB_SETS))
    return -1;

 memcpy(&kb_button_data[set], kb_button_data_buf, sizeof(kb_button_data_buf));
 kb_clear_buffer = 1;

 return 0;
}

//==============================================================================
// List the key names available for the joystick key mappings.
//
//   pass: void
// return: void
//==============================================================================
void joystick_kbjoy_listkeys (void)
{
 int i = 0;

 while (i < 64)
    {
     xprintf("%-8s", key_to_6545[i++]);
     if ((i % 8) == 0)
        xprintf("\n");
    }
}

//==============================================================================
// List the command names available for the joystick command mappings.
//
//   pass: void
// return: void
//==============================================================================
void joystick_kbjoy_listcommands (void)
{
 int i = 0;

 while (emu_commands[i])
    {
     xprintf("%-16s", emu_commands[i++]);
     if ((i % 4) == 0)
        xprintf("\n");
    }
 if ((i % 4) != 0)
    xprintf("\n");
}

//==============================================================================
// Select a joystick key mappings set and enable joystick keys.
//
//   pass: int set
//         char *set_str
// return: int                          0 if no error, else -1
//==============================================================================
int joystick_kbjoy_select (int set, char *set_str)
{
 char set_chr;

 if (strlen(set_str) == 1)
    {
     set_chr = toupper(set_str[0]);
     if ((set_chr >= 'A') && (set_chr <= 'Z'))
        set = set_chr - 'A';
    }

 if ((set < 0) || (set >= JOY_KB_SETS))
    return -1;

 kb_button_data_use = &kb_button_data[set][0];
 joystick.kbd = 1;
 joystick.set = set;

 return 0;
}

//==============================================================================
// Joystick commands.
//
//   pass: int cmd                      joystick commands
//         int p                        parameter
// return: void
//==============================================================================
void joystick_command (int cmd, int p)
{
 switch (cmd)
    {
     case EMU_CMD_JOYSTICK :
        if ((p >= 0) && (p < 26))
           joystick_kbjoy_select(p, "");
        if (p == 26)
           {
            joystick.mbee = 0;
            joystick.kbd = 0;
           }
        if (p == 27)
           joystick.mbee = 1;
        break;
    }
}

//==============================================================================
// Read joystick values.
//
// These are the non-mapped joystick values and gets called if the joystick
// is enabled on PIO port A with a --parallel-port=joystick option.
//
//   pass: void
// return: uint8_t                      joystick values
//==============================================================================
uint8_t joystick_r (void)
{
 return joystick.data;
}
