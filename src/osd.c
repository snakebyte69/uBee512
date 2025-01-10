//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                                OSD module                                  *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Provides on screen display functions (OSD)
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
// v6.0.0 - 1 January 2017, K Duckmanton
// - crtc.yscale is now video.yscale; video_renderer() is now video_render()
//   to match video.c
//
// v5.5.0 - 26 August 2012, uBee
// - Code cleanup of put_xpm() to remove 'data_type' variable and set code
//   as was unused.
// - Code in osd_keyup_event() is now disabled as it does nothing.
// - GCC sequence-point warning fixed in osd_proc_osd_args(). Code was
//   'osd.flags &= ~OSD_FLAG_ANIMATE' and should have been 'osd.flags &
//   ~OSD_FLAG_ANIMATE'
//
// v5.3.1 - 8 June 2011, uBee
// - Fixed a problem where OSD dialogues failed to be updated correctly when
//   the CRTC changes resolution.
// - Fixed the SDL rendering warning message to appear at the correct size
//   in the centre of the display instead of down at the bottom where
//   buttons were overlaying the text.
//
// v5.3.0 - 12 April 2011, uBee
// - Added OSD scheme feature structure 'osdsch_t' with built in schemes
//   that are selectable using --osd-scheme option.
// - All colours used in an OSD scheme can now be modified with options.
// - The OSD console size and location can now be configured with options.
// - The cursor rate is now settable, a value of 0 is a solid cursor.
// - Added new osd functions osd_list_schemes(), osd_set_cursor(),
//   osd_set_console_position(), osd_set_console_size(), osd_set_scheme(),
//   and osd_set_colour() and many other changes.
// - Removed map_rgb_to_int() function.
// - Added tapfile_command(EMU_CMD_TAPEREW) to dialogue_action().
//
// v5.1.0 - 20 November 2010, uBee
// - Added official distribution information to the about box.
// - mbox_t dialogues now calculate the .depth value at compile time to suit
//   the font depth in used.
// - Changed all create_*_box() functions to use a set size for title,
//   minimise, and close boxes.  (no longer determined by font depth)
// - Changed create_title_box() to calulate the position for the text start
//   y variable mbox->title.text_posy_s.
// - A new 8x8 font has replaced the 8x11 font allowing extra lines to be
//   displayed in the console.
//
// v5.0.0 - 4 August 2010, uBee
// - Added a Microbee 'Power Cycle' confirmation dialogue.
// - Replaced constant reset action numbers with EMU_RST_* defines.
// v5.0.0 - 13 July 2010, K Duckmanton
// - Removed all references to the 'sound' global variable and replaced them
//   with references to the 'audio' global instead.
//
// v4.6.0 - 21 April 2010, uBee
// - Added osd_proc_osd_args() function to process --osd arguments.
// - Changed CONSOLE_NONE to ~CONSOLE_ALL in dialogue_action() function.
//
// v4.2.0 - 14 July 2009, uBee
// - Prevent mouse cursor enabling if emulating the Microbee mouse.
// - Upadated the About message.
//
// v4.1.0 - 19 June 2009, uBee
// - Removed the minimize button from the Menu dialogue.
//
// v4.0.0 - 7 June 2009, uBee
// - Added fullscreen, sound, volume+, volume- and tape buttons to OSD menu
//   and removed Cancel.
// - Added a new attribute BOX_ATTR_NOEXIT to buttons.
// - GUI status line is now updated after an OSD console command is entered.
// - Moved convert_mouse_to_crtc_xy() over to video.c module.
//
// v3.1.0 - 14 December 2008, uBee
// - Create new OSD file.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#ifdef MINGW
#include <windows.h>
#else
#endif

#include "ubee512.h"
#include "options.h"
#include "crtc.h"
#include "z80debug.h"
#include "keyb.h"
#include "video.h"
#include "gui.h"
#include "mouse.h"
#include "osd.h"
#include "audio.h"
#include "support.h"
#include "tape.h"
#include "tapfile.h"

//==============================================================================
// structures and variables
//==============================================================================
 char *osd_scheme_names[] =
 {
  "black",
  "green",
  "blue",
  "old",
  "user",
  ""
 };

 char *osd_posx_names[] =
 {
  "center",
  "left",
  "right",
  ""
 };

 char *osd_posy_names[] =
 {
  "center",
  "top",
  "bottom",
  ""
 };

osd_t osd =
{
 .flags = OSD_FLAG_ANIMATE,
 .scheme = -1
};

osdsch_t osdsch_schemes[] =
{
 // OSD "black" new look (default) scheme
 {
  .dialogue_main_bcol = 0x000000,
  .dialogue_main_fcol = 0x808080,

  .dialogue_text_bcol = 0x000000,
  .dialogue_text_fcol = 0x808080,

  .widget_main_bcol_hl = 0x606060,
  .widget_main_bcol_ll = 0x404040,
  .widget_main_fcol_hl = 0x808080,
  .widget_main_fcol_ll = 0x505050,

  .widget_text_bcol_hl = 0x000000,
  .widget_text_bcol_ll = 0x404040,
  .widget_text_fcol_hl = 0xc0c0c0,
  .widget_text_fcol_ll = 0x808080,

  .widget_xpm_hl = 0xe84f19,
  .widget_xpm_ll = 0x808080,

  .button_main_bcol_hl = 0x606060,
  .button_main_bcol_ll = 0x303030,
  .button_main_fcol_hl = 0x808080,
  .button_main_fcol_ll = 0x808080,

  .button_text_bcol_hl = 0x606060,
  .button_text_bcol_ll = 0x303030,
  .button_text_fcol_hl = 0xc0c0c0,
  .button_text_fcol_ll = 0x808080,

  .console_cursor_rate = 250,
  .console_width = OSD_CON_PERCENT_050,
  .console_depth = 5,
  .console_pos_x = OSD_CON_CENTER,
  .console_pos_y = OSD_CON_BOTTOM,
 },

 // OSD "green" scheme
 {
  .dialogue_main_bcol = 0xadecb4,
  .dialogue_main_fcol = 0x2d813b,

  .dialogue_text_bcol = 0xadecb4,
  .dialogue_text_fcol = 0x000000,

  .widget_main_bcol_hl = 0x267433,
  .widget_main_bcol_ll = 0x21ad32,
  .widget_main_fcol_hl = 0x2d813b,
  .widget_main_fcol_ll = 0x9dde81,

  .widget_text_bcol_hl = 0x000000,
  .widget_text_bcol_ll = 0x21ad32,
  .widget_text_fcol_hl = 0xadecb4,
  .widget_text_fcol_ll = 0x2d813b,

  .widget_xpm_hl = 0xadecb4,
  .widget_xpm_ll = 0x267433,

  .button_main_bcol_hl = 0x226c2b,
  .button_main_bcol_ll = 0x21ad32,
  .button_main_fcol_hl = 0x000000,
  .button_main_fcol_ll = 0x000000,

  .button_text_bcol_hl = 0x226c2b,
  .button_text_bcol_ll = 0x21ad32,
  .button_text_fcol_hl = 0x70f936,
  .button_text_fcol_ll = 0x70f936,

  .console_cursor_rate = 250,
  .console_width = OSD_CON_PERCENT_050,
  .console_depth = 5,
  .console_pos_x = OSD_CON_CENTER,
  .console_pos_y = OSD_CON_BOTTOM,
 },

 // OSD "blue" scheme
 {
  .dialogue_main_bcol = 0x9fb8e8,
  .dialogue_main_fcol = 0x3a5b9b,

  .dialogue_text_bcol = 0x9fb8e8,
  .dialogue_text_fcol = 0x000000,

  .widget_main_bcol_hl = 0x1659ab,
  .widget_main_bcol_ll = 0x1c74e0,
  .widget_main_fcol_hl = 0x3a5b9b,
  .widget_main_fcol_ll = 0xb3c4e4,

  .widget_text_bcol_hl = 0x000000,
  .widget_text_bcol_ll = 0x1c74e0,
  .widget_text_fcol_hl = 0xb3c4e4,
  .widget_text_fcol_ll = 0x3a5b9b,

  .widget_xpm_hl = 0x9fb8e8,
  .widget_xpm_ll = 0x3a5b9b,

  .button_main_bcol_hl = 0x1659ab,
  .button_main_bcol_ll = 0x1c74e0,
  .button_main_fcol_hl = 0x000000,
  .button_main_fcol_ll = 0x000000,

  .button_text_bcol_hl = 0x1659ab,
  .button_text_bcol_ll = 0x1c74e0,
  .button_text_fcol_hl = 0x98afda,
  .button_text_fcol_ll = 0x98afda,

  .console_cursor_rate = 250,
  .console_width = OSD_CON_PERCENT_050,
  .console_depth = 5,
  .console_pos_x = OSD_CON_CENTER,
  .console_pos_y = OSD_CON_BOTTOM,
 },

 // original "old" OSD scheme
 {
  .dialogue_main_bcol = 0xa0a0a0,
  .dialogue_main_fcol = 0x404040,

  .dialogue_text_bcol = 0xa0a0a0,
  .dialogue_text_fcol = 0x000000,

  .widget_main_bcol_hl = 0xe9e9e9,
  .widget_main_bcol_ll = 0xc0c0c0,
  .widget_main_fcol_hl = 0x000000,
  .widget_main_fcol_ll = 0x808080,

  .widget_text_bcol_hl = 0xe9e9e9,
  .widget_text_bcol_ll = 0xc0c0c0,
  .widget_text_fcol_hl = 0x000000,
  .widget_text_fcol_ll = 0x808080,

  .widget_xpm_hl = 0xC00000,
  .widget_xpm_ll = 0x808080,

  .button_main_bcol_hl = 0xe9e9e9,
  .button_main_bcol_ll = 0xc0c0c0,
  .button_main_fcol_hl = 0x000000,
  .button_main_fcol_ll = 0x000000,

  .button_text_bcol_hl = 0xe9e9e9,
  .button_text_bcol_ll = 0xc0c0c0,
  .button_text_fcol_hl = 0x000000,
  .button_text_fcol_ll = 0x000000,

  .console_cursor_rate = 250,
  .console_width = OSD_CON_MAX,
  .console_depth = 10,
  .console_pos_x = OSD_CON_CENTER,
  .console_pos_y = OSD_CON_CENTER,
 },

 // OSD "user" scheme
 {
  .dialogue_main_bcol = 0x000000,
  .dialogue_main_fcol = 0x808080,

  .dialogue_text_bcol = 0x000000,
  .dialogue_text_fcol = 0x808080,

  .widget_main_bcol_hl = 0x606060,
  .widget_main_bcol_ll = 0x404040,
  .widget_main_fcol_hl = 0x808080,
  .widget_main_fcol_ll = 0x505050,

  .widget_text_bcol_hl = 0x000000,
  .widget_text_bcol_ll = 0x404040,
  .widget_text_fcol_hl = 0xc0c0c0,
  .widget_text_fcol_ll = 0x808080,

  .widget_xpm_hl = 0xe84f19,
  .widget_xpm_ll = 0x808080,

  .button_main_bcol_hl = 0x606060,
  .button_main_bcol_ll = 0x303030,
  .button_main_fcol_hl = 0x808080,
  .button_main_fcol_ll = 0x808080,

  .button_text_bcol_hl = 0x606060,
  .button_text_bcol_ll = 0x303030,
  .button_text_fcol_hl = 0xc0c0c0,
  .button_text_fcol_ll = 0x808080,

  .console_cursor_rate = 250,
  .console_width = OSD_CON_PERCENT_050,
  .console_depth = 5,
  .console_pos_x = OSD_CON_CENTER,
  .console_pos_y = OSD_CON_BOTTOM,
 }
};

static osdsch_t *osdsch;

static mbox_t *mbox;
static mbox_t maximised_mbox;
static mbox_t minimised_mbox;
static mbox_t animated_mbox;

static int dialogue_pending[DIALOGUE_PENDING_SIZE];
static int pending_put;
static int pending_get;
static int pending_count;

static int last_posx_s;
static int last_posx_f;

static int mouse_x_last;
static int mouse_y_last;
static int crt_w_last;
static int crt_h_last;

static int animating;
static int animate_update;
static int animate_add_x_s;
static int animate_add_x_f;
static int animate_shrink_x;
static int animate_add_y_s;
static int animate_add_y_f;
static int animate_shrink_y;

static int drag_window;

static uint64_t msecs_before;
static font_t font;
static SDL_PixelFormat *spf;

static char command[1000];
static int cmd_length;
static int cmd_putpos;

extern char *c_argv[];
extern int c_argc;

extern SDL_Surface *screen;
extern emu_t emu;
extern gui_t gui;
extern crtc_t crtc;
extern video_t video;
extern mouse_t mouse;
extern messages_t messages;
extern help_t help;
extern console_t console;
extern uint8_t fontdata[];

static void osd_dialogue (int dialogue);
static int osd_get_pending (void);

//==============================================================================
// dialogues text
//==============================================================================
static char dialogue_exit[] =
{
 "All unsaved data will be lost!\n"
 "\n"
 "Exit uBee512 ?"
};

static char dialogue_reset[] =
{
 "All unsaved data will be lost!\n"
 "\n"
 "Reset uBee512 ?"
};

static char dialogue_powercyc[] =
{
 "All unsaved data will be lost!\n"
 "\n"
 "Power Cycle uBee512 ?"
};

static char dialogue_devver[] =
{
 "This is a development version\n"
 "only. It is not intended to\n"
 "be recirculated."
};

static char dialogue_opengl[] =
{
 "You're using SDL video rendering mode, better\n"
 "video rendering can be achieved by using\n"
 "OpenGL (use --video-type=gl option).\n"
 "\n"
 "To prevent seeing this message again select\n"
 "the 'OK' button.\n"
};

static char dialogue_about[] =
{
 "             Version "APPVER"\n"
 "uBee512 is an emulator for all Microbee Z80\n"
 "  ROM, Floppy and Hard drive based models.\n"
 "       (c) Copyright 2007-2016 uBee\n"
 "\n"
 "This software is released under the GNU GPL\n"
 "license, the license is part of the original\n"
 "distribution.\n"
 "\n"
 "Official uBee512 distribution site is here:\n"
 "         www.microbee-mspp.org.au\n"
};

static char dialogue_console[CONSOLE_SIZE+1];
static char dialogue_shared[SHARED_SIZE+1];

//==============================================================================
// Title text
//==============================================================================
static char title_ubee512[] = "uBee512";
static char title_powercyc[] = "uBee512 Power Cycle";
static char title_reset[] = "uBee512 Reset";
static char title_exit[] = "uBee512 Exit";
static char title_console[] = "uBee512 Console";
static char title_output[] = "uBee512 Output";
static char title_about[] = "About uBee512";
static char title_menu[] = "Menu";

//==============================================================================
// Buttons text
//==============================================================================
static char button_about[] = "About";
static char button_cancel[] = "Cancel";
static char button_console[] = "Console";
static char button_output[] = "Output";
static char button_exit[] = "Exit";
static char button_ok[] = "OK";
static char button_reset[] = "Reset";
static char button_powercyc[] = "Power Cycle";
static char button_none[] = "None";
static char button_osd[] = "OSD";
static char button_stdout[] = "Stdout";
static char button_both[] = "Both";
static char button_fullscreen[] = "Fullscreen";
static char button_sound[] = "Sound";
static char button_volumei[] = "Volume +";
static char button_volumed[] = "Volume -";
static char button_tape[] = "Tape (rew)";

//==============================================================================
// Icons/images in XPM format,  these designs were made using the icon
// editor IcoFX under wine.  The ICO files created were then exported as PNG
// and then converted using the ImageMagick convert program:
// convert filename.png filename.xpm
//==============================================================================
static char *minimise_xpm[] = {
/* columns rows colors chars-per-pixel */
"21 11 2 1",
"  c #C00000",
". c None",
/* pixels */
".....................",
".....................",
".....................",
".....................",
".....................",
".....................",
".....................",
".....................",
"...               ...",
".....................",
"....................."
};

static char *maximise_a_xpm[] = {
/* columns rows colors chars-per-pixel */
"21 11 2 1",
"  c #C00000",
". c None",
/* pixels */
".....................",
".....................",
"...               ...",
"...               ...",
"... ............. ...",
"... ............. ...",
"... ............. ...",
"... ............. ...",
"...               ...",
".....................",
"....................."
};

static char *maximise_b_xpm[] = {
/* columns rows colors chars-per-pixel */
"21 11 2 1",
"  c #C00000",
". c None",
/* pixels */
".....................",
".........         ...",
".........         ...",
"......... ....... ...",
"...         ..... ...",
"...               ...",
"... ....... .........",
"... ....... .........",
"...         .........",
".....................",
"....................."
};

static char *close_xpm[] = {
/* columns rows colors chars-per-pixel */
"21 11 2 1",
"  c #C00000",
". c None",
/* pixels */
".....................",
".....................",
"...  ..........  ....",
".....  ......  ......",
".......  ..  ........",
".........  ..........",
".......  ..  ........",
".....  ......  ......",
"...  ..........  ....",
".....................",
"....................."
};

static char colour_xpm_hl[15];
static char colour_xpm_ll[15];

static char *warning_xpm[] = {
/* columns rows colors chars-per-pixel */
"32 32 6 1",
"  c black",
". c #FFEE04",
"X c #FFFF04",
"o c #FFFF05",
"O c #FFFF06",
"+ c None",
/* pixels */
"+++++++++++++++  +++++++++++++++",
"++++++++++++++ OO ++++++++++++++",
"++++++++++++++ oo ++++++++++++++",
"+++++++++++++ OXXO +++++++++++++",
"+++++++++++++ o..o +++++++++++++",
"++++++++++++ OX..XO ++++++++++++",
"++++++++++++ o....o ++++++++++++",
"+++++++++++ OXXooXXO +++++++++++",
"+++++++++++ oXO  OXo +++++++++++",
"++++++++++ OXo    oXO ++++++++++",
"++++++++++ o.O    O.o ++++++++++",
"+++++++++ OX.O    O.XO +++++++++",
"+++++++++ o..O    O..o +++++++++",
"++++++++ OX..O    O..XO ++++++++",
"++++++++ o...O    O...o ++++++++",
"+++++++ OX...O    O...XO +++++++",
"+++++++ o....O    O....o +++++++",
"++++++ OX....O    O....XO ++++++",
"++++++ o.....O    O.....o ++++++",
"+++++ OX.....O    O.....XO +++++",
"+++++ o......O    O......o +++++",
"++++ OX......O    O......XO ++++",
"++++ o.......o    o.......o ++++",
"+++ OX.......XO  OX.......XO +++",
"+++ o.........oOOo.........o +++",
"++ OX........XO  OX........XO ++",
"++ o.........o    o.........o ++",
"+ OX.........o    o.........XO +",
"+ o..........XO  OX..........o +",
" OX...........XooX...........XO ",
" OOOOOOOOOOOOOOOOOOOOOOOOOOOOOO ",
"+                              +"
};

static char *information_xpm[] = {
/* columns rows colors chars-per-pixel */
"32 32 10 1",
"  c black",
". c #0080FF",
"X c #0090FF",
"o c #00A0FF",
"O c #00A3FF",
"+ c #00B0FF",
"@ c #00B5FF",
"# c #00B6FF",
"$ c #00C6FF",
"% c None",
/* pixels */
"%%%%%%%%%%%%%%     %%%%%%%%%%%%%",
"%%%%%%%%%%%%% @+++@ %%%%%%%%%%%%",
"%%%%%%%%%%%% #X...X# %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% #X...X# %%%%%%%%%%%",
"%%%%%%%%%%%%% #+++# %%%%%%%%%%%%",
"%%%%%%%%%%%%%%     %%%%%%%%%%%%%",
"%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%",
"%%%%%%%%%           %%%%%%%%%%%%",
"%%%%%%%% $+++++++++$ %%%%%%%%%%%",
"%%%%%%%% +.........+ %%%%%%%%%%%",
"%%%%%%%% +.........+ %%%%%%%%%%%",
"%%%%%%%% $+oX......+ %%%%%%%%%%%",
"%%%%%%%%%   OX.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% o.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% +.....+ %%%%%%%%%%%",
"%%%%%%%%%%%% o.....o %%%%%%%%%%%",
"%%%%%%%%%   OX.....XO   %%%%%%%%",
"%%%%%%%% $+oX.......Xo+$ %%%%%%%",
"%%%%%%%% +.............+ %%%%%%%",
"%%%%%%%% +.............+ %%%%%%%",
"%%%%%%%% $+++++++++++++$ %%%%%%%",
"%%%%%%%%%               %%%%%%%%"
};

static char *question_xpm[] = {
/* columns rows colors chars-per-pixel */
"32 32 14 1",
"  c black",
". c #66CC33",
"X c #73E639",
"o c #80FF40",
"O c #82FF41",
"+ c #83FF41",
"@ c #83FF42",
"# c #84FF42",
"$ c #8CFF46",
"% c #90FF48",
"& c #91FF48",
"* c #91FF49",
"= c #9EFF4F",
"- c None",
/* pixels */
"-------                   ------",
"------ %$$$$$$$$$$$$$$$$$% -----",
"----- @X.................X@ ----",
"---- OX...Xo$$$$$$$$oX....X@ ---",
"--- @X...XO          OX....X% --",
"-- %X...XO ---------- OX....$ --",
"-- $...XO ------------ OX...$ --",
"-- $...o -------------- o...$ --",
"-- $...o -------------- o...$ --",
"-- $...X+ ------------ OX..X% --",
"-- $....X% ---------- OX..X# ---",
"-- &X...X* --------- OX.Xo% ----",
"--- &$$$& --------- OX.XO  -----",
"----     --------- @X.X@ -------",
"----------------- @XXo% --------",
"---------------- @XX+  ---------",
"--------------- #XX# -----------",
"-------------- #oo% ------------",
"------------- #o@  -------------",
"------------ %Xo ---------------",
"------------ $.$ ---------------",
"------------ $.$ ---------------",
"------------ $.$ ---------------",
"------------ =$= ---------------",
"-------------   ----------------",
"--------------------------------",
"--------------------------------",
"------------     ---------------",
"----------- =$$$= --------------",
"----------- $...$ --------------",
"----------- =$$$= --------------",
"------------     ---------------"
};

static x11_rgb_col_t x11_rgb_col[] =
 {
  {"None",         -1},
  {"black",  0x000000},
  {"red",    0xff0000},
  {"green",  0x00ff00},
  {"blue",   0x0000ff},
  {"",       0x000000}
 };

//==============================================================================
// Dialogues.
//
// The order of these entries must match the DIALOGUE_*entries found in osd.h
//==============================================================================
static mbox_t dialogues[] =
{
 // DIALOGUE_NOTINUSE (dont remove, this is used to start others from 1)
 {
 },

 // DIALOGUE_EXIT
 {
  .title.text = title_exit,
  .main.text = dialogue_exit,
  .main.text_buf_count = sizeof(dialogue_exit)-1,
  .buttons = 2,
  .width = 300,
  .depth = OSD_FONT_DEPTH * 3 + 65,
  .bwidth = BUTTON_WIDTH,
  .bdepth = BUTTON_DEPTH,
  .text_posx_ofs = 48,
  .text_posy_ofs = 30,
  .icon = warning_xpm,
  .attr = 0,
  .btn[0].text = button_ok,
  .btn[1].text = button_cancel,
  .components = BOX_COMP_TITLE | BOX_COMP_MIN | BOX_COMP_CLOSE
 },

 // DIALOGUE_POWERCYC
 {
  .title.text = title_powercyc,
  .main.text = dialogue_powercyc,
  .main.text_buf_count = sizeof(dialogue_powercyc)-1,
  .buttons = 2,
  .width = 300,
  .depth = OSD_FONT_DEPTH * 3 + 65,
  .bwidth = BUTTON_WIDTH,
  .bdepth = BUTTON_DEPTH,
  .text_posx_ofs = 48,
  .text_posy_ofs = 30,
  .icon = warning_xpm,
  .attr = 0,
  .btn[0].text = button_ok,
  .btn[1].text = button_cancel,
  .components = BOX_COMP_TITLE | BOX_COMP_MIN | BOX_COMP_CLOSE
 },

 // DIALOGUE_RESET
 {
  .title.text = title_reset,
  .main.text = dialogue_reset,
  .main.text_buf_count = sizeof(dialogue_reset)-1,
  .buttons = 2,
  .width = 300,
  .depth = OSD_FONT_DEPTH * 3 + 65,
  .bwidth = BUTTON_WIDTH,
  .bdepth = BUTTON_DEPTH,
  .text_posx_ofs = 48,
  .text_posy_ofs = 30,
  .icon = warning_xpm,
  .attr = 0,
  .btn[0].text = button_ok,
  .btn[1].text = button_cancel,
  .components = BOX_COMP_TITLE | BOX_COMP_MIN | BOX_COMP_CLOSE
 },

 // DIALOGUE_DEVMESG
 {
  .title.text = title_ubee512,
  .main.text = dialogue_devver,
  .main.text_buf_count = sizeof(dialogue_devver)-1,
  .buttons = 1,
  .width = 300,
  .depth = OSD_FONT_DEPTH * 3 + 65,
  .bwidth = BUTTON_WIDTH,
  .bdepth = BUTTON_DEPTH,
  .text_posx_ofs = 48,
  .text_posy_ofs = 30,
  .icon = warning_xpm,
  .attr = 0,
  .btn[0].text = button_ok,
  .components = BOX_COMP_TITLE | BOX_COMP_CLOSE
 },

 // DIALOGUE_OPENGL
 {
  .title.text = title_ubee512,
  .main.text = dialogue_opengl,
  .main.text_buf_count = sizeof(dialogue_opengl)-1,
  .buttons = 2,
  .width = 420,
  .depth = OSD_FONT_DEPTH * 6 + 65,
  .bwidth = BUTTON_WIDTH,
  .bdepth = BUTTON_DEPTH,
  .text_posx_ofs = 48,
  .text_posy_ofs = 30,
  .icon = information_xpm,
  .attr = 0,
  .btn[0].text = button_ok,
  .btn[1].text = button_cancel,
  .components = BOX_COMP_TITLE | BOX_COMP_MIN | BOX_COMP_CLOSE
 },

 // DIALOGUE_CONSOLE
 {
  .title.text = title_console,
  .main.text = dialogue_console,
  .text_posx_ofs = 8,
  .text_posy_ofs = 20,
  .icon = NULL,
  .width = -1,
  .depth = -1,
  .main.posx_s = -1,
  .main.posx_f = -1,
  .main.posy_s = -1,
  .main.posy_f = -1,
  .attr = MBOX_ATTR_RESIZABLE,
  .components = BOX_COMP_TITLE | BOX_COMP_MAX | BOX_COMP_MIN | BOX_COMP_CLOSE
 },

 // DIALOGUE_ABOUT
 {
  .title.text = title_about,
  .main.text = dialogue_about,
  .main.text_buf_count = sizeof(dialogue_about)-1,
  .buttons = 1,
  .width = 415,
  .depth = OSD_FONT_DEPTH * 11 + 55,
  .bwidth = BUTTON_WIDTH,
  .bdepth = BUTTON_DEPTH,
  .text_posx_ofs = 48,
  .text_posy_ofs = 25,
  .icon = information_xpm,
  .attr = 0,
  .btn[0].text = button_ok,
  .components = BOX_COMP_TITLE | BOX_COMP_MIN | BOX_COMP_CLOSE
 },

 // DIALOGUE_OUTPUT
 {
  .title.text = title_output,
  .main.text = dialogue_shared,
  .buttons = 4,
  .width = 330,
  .depth = BUTTON_DEPTH * DIALOGUE_OUTPUT_BUTTONS + 28,
  .bwidth = 70,
  .bdepth = BUTTON_DEPTH,
  .text_posx_ofs = 60,
  .text_posy_ofs = 30,
  .icon = question_xpm,
  .attr = MBOX_ATTR_VBTNS_RJ,
  .btn[0].text = button_none,
  .btn[1].text = button_osd,
  .btn[2].text = button_stdout,
  .btn[3].text = button_both,
  .components = BOX_COMP_TITLE | BOX_COMP_MIN | BOX_COMP_CLOSE
 },

 // DIALOGUE_MENU
 {
  .title.text = title_menu,
  .main.text = NULL,
  .buttons = DIALOGUE_MENU_BUTTONS,
  .width = DIALOGUE_MENU_WIDTH,
  .depth = BUTTON_DEPTH * DIALOGUE_MENU_BUTTONS + 28,
  .bwidth = DIALOGUE_MENU_WIDTH - 17,
  .bdepth = BUTTON_DEPTH,
  .icon = NULL,
  .attr = MBOX_ATTR_VBTNS_LJ | MBOX_ATTR_MOUSEPOS,
  .btn[0].text = button_about,
  .btn[1].text = button_console,
  .btn[2].text = button_output,
  .btn[3].text = button_fullscreen,
  .btn[4].text = button_sound,
  .btn[5].text = button_volumei,
  .btn[6].text = button_volumed,
  .btn[7].text = button_tape,
  .btn[8].text = button_reset,
  .btn[9].text = button_powercyc,
  .btn[10].text = button_exit,
  .btn[3].attr = BOX_ATTR_NOEXIT,
  .btn[5].attr = BOX_ATTR_NOEXIT,
  .btn[6].attr = BOX_ATTR_NOEXIT,
//  .components = BOX_COMP_TITLE | BOX_COMP_MIN | BOX_COMP_CLOSE
  .components = BOX_COMP_TITLE | BOX_COMP_CLOSE
 }
};

//==============================================================================
// OSD initialise.
//
// Must not contain any dependencies of other initilisation calls.  The main
// job is to initialise the console dialogue strings storage area so that it
// may be written to and displayed later.
//
//   pass: void
// return: int                  0 if no error, -1 if error
//==============================================================================
int osd_init (void)
{
 // if no scheme selected then use the default one
 if (osd.scheme == -1)
    {
     if (osd_set_scheme("default") == -1)
        return -1;
    }

 osd.initialised = 1;

#if 0
 // place a message into the console buffer
 int streams;

 streams = console_get_devices();
 console_set_devices(CONSOLE_OSD);
 xprintf("This is the OSD output console device.\n");
 console_set_devices(streams);
#endif

 return 0;
}

//==============================================================================
// OSD de-initialise.
//
//   pass: void
// return: int                  0
//==============================================================================
int osd_deinit (void)
{
 return 0;
}

//==============================================================================
// OSD reset.
//
//   pass: void
// return: int                  0
//==============================================================================
int osd_reset (void)
{
 return 0;
}

//==============================================================================
// Draw a single pixel
//
//   pass: int x
//         int y
//         int col
// return: void
//==============================================================================
static void put_pixel (int x, int y, int col)
{
 if (video.yscale == 2)
    {
     video_putpixel(x, y*2,   col);
     video_putpixel(x, y*2+1, col);
    }
 else
    video_putpixel(x, y, col);
}

//==============================================================================
// Draw an XPM image.
//
// This implementation assumes single ASCII characters in the XPM file and so
// is limited to 126 unique colours.  All data is assumed to be of type 'c'.
//
// Additional colour constants (see rgb.txt on a Little Endian X11 system)
// may need to be added to the x11_rgb_col table.
//
//   pass: char *xpm
//         int x
//         int y
// return: void
//==============================================================================
static void put_xpm (char *xpm[], int x, int y)
{
 int col_table[128];
 int width = 0;
 int height = 0;
 int colours = 0;
 int chars_per_pix = 0;

 int col_char;
 int col;
 int xpm_x;
 int xpm_y;
 int put_x;
 int i;
 int z;

 sscanf(xpm[0], "%d %d %d %d", &width, &height, &colours, &chars_per_pix);

 // build up a single ASCII table of XPM colours
 for (i = 1; i <= colours; i++)
    {
     col_char = xpm[i][0];   // colour character
     if (xpm[i][4] == '#')
        sscanf(&xpm[i][5], "%x", &col);
     else
        {
         col = 0;
         for (z = 0; x11_rgb_col[z].colour[0]; z++)
            {
             if (strcmp(x11_rgb_col[z].colour, xpm[i]+4) == 0)
                {
                 col = x11_rgb_col[z].value;
                 break;
                }
            }
        }
     if (col == -1)
        col_table[col_char] = -1;
     else
        col_table[col_char] = SDL_MapRGB(spf, (col & 0x00ff0000) >> 16,
                                        (col & 0x0000ff00) >> 8,
                                        (col & 0x000000ff));
    }

 // draw the image
 for (xpm_y = 0; xpm_y < height; xpm_y++)
    {
     put_x = x;
     for (xpm_x = 0; xpm_x < width; xpm_x++)
        {
         if (col_table[(int)xpm[i][xpm_x]] != -1)
            put_pixel(put_x, y, col_table[(int)xpm[i][xpm_x]]);
         put_x++;
        }
     i++;
     y++;
    }
}

//==============================================================================
// Return the dialogue width value.
//
// The current value stored in the dialogue being used is returned if the
// dialogue is not maximised.  If the dialogue is maximised the current
// screen width value is returned.
//
//   pass: void
// return: int                          dialogue width
//==============================================================================
static int dialogue_width (void)
{
 if ((mbox->attr & MBOX_ATTR_RESIZABLE) && (mbox->attr & MBOX_ATTR_MAXIMISED))
    return crtc.hdisp * 8;
 return mbox->width;
}

//==============================================================================
// Return the dialogue depth value.
//
// The current value stored in the dialogue being used is returned if the
// dialogue is not maximised.  If the dialogue is maximised the current
// screen depth value is returned.
//
//   pass: void
// return: int                          dialogue depth
//==============================================================================
static int dialogue_depth (void)
{
 if ((mbox->attr & MBOX_ATTR_RESIZABLE) && (mbox->attr & MBOX_ATTR_MAXIMISED))
    return crtc.vdisp * crtc.scans_per_row;
 return mbox->depth;
}

//==============================================================================
// Write character to the OSD.
//
//   pass: int c                        character to display
// return: void
//==============================================================================
static void write_char_to_osd (int c)
{
 int x;
 int y;
 int i;
 int pixels;

 i = font.depth * c;

 for (y = 0; y < font.depth; y++)
    {
     pixels = font.data[i++];

     for (x = 0; x < font.width; x++)
        {
         if (pixels & 0x80)
            put_pixel(font.x_s + x, font.y_s + y, font.fgc);
         else
            put_pixel(font.x_s + x, font.y_s + y, font.bgc);
         pixels <<= 1;
        }
    }

 font.x_s += font.width;
}

//==============================================================================
// Update cursor in the OSD.
//
//   pass: box_t *box                   dialogue box
// return: void
//==============================================================================
static void update_cursor (box_t *box)
{
 int x;
 int y;

 int curs_x;
 int curs_y;

 // only want cursor for console text
 if (box->text != dialogue_console)
    return;

 if (font.x_s + (font.width-1) > box->text_posx_f)
    {
     curs_y = font.y_s + font.depth;
     curs_x = font.xorig;
    }
 else
    {
     curs_x = font.x_s;
     curs_y = font.y_s;
    }

 if (curs_y + (font.depth-1) > box->text_posy_f)
    return;

 for (y = 0; y < font.depth; y++)
    {
     if ((! box->cursor_rate) || ((time_get_ms() / box->cursor_rate) & 0x01))
        {
         for (x = 0; x < font.width; x++)
            put_pixel(curs_x + x, curs_y + y, font.fgc);
        }
    }
}

//==============================================================================
// Write text buffer to the OSD.
//
// This function is used to update the OSD text on a regular basis. the
// number of lines displayed will be box->text_depth or less.  The current
// window size determines the box->text_depth value.
//
//   pass: box_t *box                   dialogue box
//         int bufpos                   buffer starting point
// return: void
//==============================================================================
static void write_buffer_to_osd (box_t *box, int bufpos)
{
 int y = 0;

 // exit if no text buffer
 if (box->text == NULL)
    return;

 // keep writing until a NULL is encountered or depth is used up
 while ((box->text[bufpos]) && (y < box->text_depth))
    {
     switch (box->text[bufpos])
        {
         case '\n' : font.x_s = font.xorig;
                     font.y_s += font.depth;
                     y++;
                     break;
#if 0
         case '\r' : font.x_s = font.xorig;
                     break;
#endif
           default : if ((font.x_s + (font.width - 1)) > font.x_f)
                        {
                         font.x_s = font.xorig;
                         font.y_s += font.depth;
                         y++;
                        }
                     if (y < box->text_depth)
                        write_char_to_osd(box->text[bufpos]);
                     break;
        }
     if (++bufpos >= CONSOLE_SIZE)
        bufpos = 0;
    }

 update_cursor(box);
}

//==============================================================================
// Find buffer display starting position.
//
// Work back from the ending buffer position to find the buffer position to
// start displaying from.
//
//   pass: box_t *box                   dialogue box
// return: int                          buffer starting point
//==============================================================================
static int find_buffer_position (box_t *box)
{
 int bufpos;
 int xbufpos;
 int count;
 int y = 1;

 int font_x_s = box->text_posx_s;
 int font_x_f = box->text_posx_f;

 count = box->text_buf_count;
 bufpos = box->text_buf_put - (count != 0);
 xbufpos = bufpos;

// printf("\nA: xbufpos=%d\n", xbufpos);

 while ((count) && (y < box->text_depth))
    {
     count--;
     switch (box->text[bufpos])
        {
         case '\n' : xbufpos = bufpos + 1;
// printf("B: xbufpos=%d\n", xbufpos);
                     font_x_s = font.xorig;
                     y++;
                     break;
           default : if ((font_x_s + (font.width - 1)) > font_x_f)
                        {
                         xbufpos = bufpos;

// printf("C: xbufpos=%d\n", xbufpos);
                         font_x_s = font.xorig;
                         y++;
                        }
                     else
                        font_x_s += font.width;
                     break;
        }
     if (--bufpos < 0)
        bufpos = CONSOLE_SIZE-1;

    }

 if (count == 0)
    xbufpos = box->text_buf_start;

// printf("D: xbufpos=%d\n", xbufpos);
// printf("Y: Y=%d  Depth=%d Count=%d\n", y, box->text_depth, count);

 return xbufpos;
}

//==============================================================================
// Write a character to a dialogue text buffer.
//
// Writes a character to the text buffer for the passed dialogue.  This can
// be written to at any time.
//
// This uses a circular buffer.  Data is not moved,  variables text_buf_put
// is where the next character is placed, text_buf_start is the current start
// position and text_buf_count is the number of bytes in the buffer.
//
// When text_buf_count + 1 is greater than the buffer size then the
// text_buf_start value is incremented and will wrap around to 0 if past the
// last buffer position.
//
//   pass: mbox_t *mbox                 dialogue
//         int c                        printable or control character
// return: void
//==============================================================================
static void osd_write_char_to_buffer (mbox_t *mbox, int c)
{
 switch (c)
    {
     case '\b' : // destructive backspace
        if (mbox->main.text_buf_count)
           {
            mbox->main.text_buf_count--;
            if (mbox->main.text_buf_put)
               mbox->main.text_buf_put--;
            else
               mbox->main.text_buf_put = CONSOLE_SIZE-1;
           }
        break;

     default : // all other characters
        if ((c < ' ') && (c != '\n'))
           break;

        mbox->main.text[mbox->main.text_buf_put++] = c;

        if (mbox->main.text_buf_put >= CONSOLE_SIZE)
           mbox->main.text_buf_put = 0;

        if ((mbox->main.text_buf_count + 1) > CONSOLE_SIZE)
           {
            mbox->main.text_buf_start++;
            if (mbox->main.text_buf_start >= CONSOLE_SIZE)
               mbox->main.text_buf_start = 0;
           }
        else
           mbox->main.text_buf_count++;
        break;
    }

 // keep the buffer NULL terminated
 mbox->main.text[mbox->main.text_buf_put] = 0;

 // if this dialogue is currently displayed then make it get updated
 if ((emu.display_context == EMU_OSD_CONTEXT) &&
     (mbox->dialogue) && (! mbox->minimised))
    crtc_set_redraw();
}

//==============================================================================
// Check if X, Y are within the box co-ordinates.
//
//   pass: box_t *box                   dialogue box
//         int x
//         int y
// return: int                          1 if within else 0
//==============================================================================
static int check_xy_in (box_t *box, int x, int y)
{
 return (x >= box->posx_s) && (x <= box->posx_f) &&
        (y >= box->posy_s) && (y <= box->posy_f);
}

//==============================================================================
// Check if X, Y are inside the box co-ordinates.
//
// A match requires X, Y to be inside the border limits.
//
//   pass: box_t *box                   dialogue box
//         int x
//         int y
// return: int                          1 if within else 0
//==============================================================================
static int check_xy_inside (box_t *box, int x, int y)
{
 return (x >= (box->posx_s + (box->attr & 0x07))) &&
        (x <= (box->posx_f - (box->attr & 0x07))) &&
        (y >= (box->posy_s + (box->attr & 0x07))) &&
        (y <= (box->posy_f - (box->attr & 0x07)));
}

//==============================================================================
// Check if X, Y is within a box or if a box side or corner on a resizable
// window is in context.
//
// Maximised windows return a value of 0.
//
// A value of 0 is returned if X, Y does match any sides or corners, the 9
// values returned determines what side, corner or if the window is in
// context, corners take precedence over sides and window:
//
// 1 : top left corner
// 2 : top right corner
// 3 : bottom right corner
// 4 : bottom left corner
// 5 : left side
// 6 : top side
// 7 : right side
// 8 : bottom side
// 9 : window
//
//   pass: box_t *box                   dialogue box
//         int x
//         int y
// return: int                          side, corner value else 0
//==============================================================================
static int check_window_xy_in (box_t *box, int x, int y)
{
 int res;

 // no dragging of dialogues permitted if currently maximised
 if (mbox->attr & MBOX_ATTR_MAXIMISED)
    return 0;

 // check if the dialogue has been grabbed
 res = check_xy_in(box, x, y) * 9;

 // if no maximising component then return the result value
 if (! (mbox->components & BOX_COMP_MAX))
    return res;

 // top left corner
 if (
     (((x >= box->posx_s) && (x <= (box->posx_s + (box->attr & 0x07)))) &&
     ((y >= box->posy_s) && (y <= (box->posy_s + 10)))) ||
     (((y >= box->posy_s) && (y <= (box->posy_s + (box->attr & 0x07)))) &&
     ((x >= box->posx_s) && (x <= (box->posx_s + 20))))
    )
    return 1;

 // top right corner
 if (
     (((x <= box->posx_f) && (x >= (box->posx_f - (box->attr & 0x07)))) &&
     ((y >= box->posy_s) && (y <= (box->posy_s + 10)))) ||
     (((y >= box->posy_s) && (y <= (box->posy_s + (box->attr & 0x07)))) &&
     ((x <= box->posx_f) && (x >= (box->posx_f - 20))))
    )
    return 2;

 // bottom right corner
 if (
     (((x <= box->posx_f) && (x >= (box->posx_f - (box->attr & 0x07)))) &&
     ((y <= box->posy_f) && (y >= (box->posy_f - 10)))) ||
     (((y <= box->posy_f) && (y >= (box->posy_f - (box->attr & 0x07)))) &&
     ((x <= box->posx_f) && (x >= (box->posx_f - 20))))
    )
    return 3;

 // bottom left corner
 if (
     (((x >= box->posx_s) && (x <= (box->posx_s + (box->attr & 0x07)))) &&
     ((y <= box->posy_f) && (y >= (box->posy_f - 10)))) ||
     (((y <= box->posy_f) && (y >= (box->posy_f - (box->attr & 0x07)))) &&
     ((x >= box->posx_s) && (x <= (box->posx_s + 20))))
    )
    return 4;

 // left side
 if ((x >= box->posx_s) && (x <= (box->posx_s + (box->attr & 0x07))))
    return 5;

 // top side
 if ((y >= box->posy_s) && (y <= (box->posy_s + (box->attr & 0x07))))
    return 6;

 // right side
 if ((x <= box->posx_f) && (x >= (box->posx_f - (box->attr & 0x07))))
    return 7;

 // bottom side
 if ((y <= box->posy_f) && (y >= (box->posy_f - (box->attr & 0x07))))
    return 8;

 return res;
}

//==============================================================================
// Set graphics colours
//
//   pass: box_t *box                   dialogue box
//         int *bgc
//         int *fgc
// return: void
//==============================================================================
static void set_box_col (box_t *box, int *bgc, int *fgc)
{
 *bgc = SDL_MapRGB(spf, (box->bcol & 0x00ff0000) >> 16,
 (box->bcol & 0x0000ff00) >> 8, (box->bcol & 0x000000ff));
 *fgc = SDL_MapRGB(spf, (box->fcol & 0x00ff0000) >> 16,
 (box->fcol & 0x0000ff00) >> 8, (box->fcol & 0x000000ff));
}

//==============================================================================
// Set text colours
//
//   pass: box_t *box                   dialogue box
//         int *bgc
//         int *fgc
// return: void
//==============================================================================
static void set_text_col (box_t *box, int *bgc, int *fgc)
{
 *bgc = SDL_MapRGB(spf, (box->text_bcol & 0x00ff0000) >> 16,
 (box->text_bcol & 0x0000ff00) >> 8, (box->text_bcol & 0x000000ff));
 *fgc = SDL_MapRGB(spf, (box->text_fcol & 0x00ff0000) >> 16,
 (box->text_fcol & 0x0000ff00) >> 8, (box->text_fcol & 0x000000ff));
}

//==============================================================================
// Fill a box with a background colour.
//
//   pass: box_t *box                   dialogue box
// return: void
//==============================================================================
static void fill_box (box_t *box)
{
 int bgc;
 int fgc;
 int x;
 int y;

 set_box_col(box, &bgc, &fgc);

 for (y = box->posy_s; y <= box->posy_f; y++)
    {
     for (x = box->posx_s; x <= box->posx_f; x++)
        put_pixel(x, y, bgc);
    }
}

//==============================================================================
// Draw a box outline with the required attributes.
//
//   pass: box_t *box                   dialogue box
// return: void
//==============================================================================
static void draw_box (box_t *box)
{
 int bgc;
 int fgc;
 int x;
 int y;

 int dash_count = 0;

 set_box_col(box, &bgc, &fgc);

 for (y = box->posy_s; y <= box->posy_f; y++)
    {
     for (x = 0; x < (box->attr & 0x07); x++)
        put_pixel(box->posx_s + x, y, fgc);
     for (x = 0; x < (box->attr & 0x07); x++)
        put_pixel(box->posx_f - x, y, fgc);
    }

 for (x = box->posx_s; x <= box->posx_f; x++)
    {
     for (y = 0; y < (box->attr & 0x07); y++)
        put_pixel(x, box->posy_s + y, fgc);
     for (y = 0; y < (box->attr & 0x07); y++)
        put_pixel(x, box->posy_f - y, fgc);
    }

 // draw a dashed text outline if the attribute bit is set
 for (y = box->posy_s + (box->attr & 0x07) + 1;
      y <= ((box->posy_f) - ((box->attr & 0x07) + 1)); y++)
    {
     if (box->attr & BOX_ATTR_DASHED)
        {
         if ((++dash_count % 3) == 0)
            {
             put_pixel(box->posx_s + ((box->attr & 0x07) + 1), y, fgc);
             put_pixel(box->posx_f - ((box->attr & 0x07) + 1), y, fgc);
            }
        }
    }

 dash_count = 0;

 for (x = box->posx_s + (box->attr & 0x07) + 1;
      x <= (box->posx_f - ((box->attr & 0x07) + 1)); x++)
    {
     if (box->attr & BOX_ATTR_DASHED)
        {
         if ((++dash_count % 3) == 0)
            {
             put_pixel(x, box->posy_s + ((box->attr & 0x07) + 1), fgc);
             put_pixel(x, box->posy_f - ((box->attr & 0x07) + 1), fgc);
            }
        }
    }
}

//==============================================================================
// Write text to box.
//
//   pass: box_t *box                   dialogue box
//         int find_buf_pos             find buffer position if not 0
// return: void
//==============================================================================
static void text_box (box_t *box, int find_buf_pos)
{
 int bgc;
 int fgc;
 int bufpos;

 set_text_col(box, &bgc, &fgc);

 font.data = fontdata;
 font.depth = OSD_FONT_DEPTH;
 font.width = OSD_FONT_WIDTH;
 font.x_s = box->text_posx_s;
 font.x_f = box->text_posx_f;
 font.y_s = box->text_posy_s;
 font.y_f = box->text_posy_f;
 font.xorig = font.x_s;
 font.yorig = font.y_s;
 font.bgc = bgc;
 font.fgc = fgc;

 box->text_width = ((box->text_posx_f - box->text_posx_s) + 1) / OSD_FONT_WIDTH;
 box->text_depth = ((box->text_posy_f - box->text_posy_s) + 1) / OSD_FONT_DEPTH;

// determine how far back up in the buffer to start displaying from
 if (find_buf_pos)
    bufpos = find_buffer_position(box);
 else
    bufpos = 0;

 write_buffer_to_osd(box, bufpos);
}

//==============================================================================
// Create the main dialogue box.
//
//   pass: void
// return: void
//==============================================================================
static void create_dialogue_box (void)
{
 fill_box(&mbox->main);

 // save processor time if dialogue is minimised
 if (mbox->minimised)
    return;

 mbox->main.bcol = osdsch->dialogue_main_bcol;
 mbox->main.fcol = osdsch->dialogue_main_fcol;

 if (! (mbox->attr & MBOX_ATTR_MAXIMISED))
    {
     mbox->main.text_posx_s = mbox->main.posx_s + mbox->text_posx_ofs;
     mbox->main.text_posx_f = mbox->main.posx_f -
                              ((mbox->main.attr & 0x07) + 2);
     mbox->main.text_posy_s = mbox->main.posy_s + mbox->text_posy_ofs;
     mbox->main.text_posy_f = mbox->main.posy_f -
                              ((mbox->main.attr & 0x07) + 2);
    }
 else
    {
     mbox->main.text_posx_s = mbox->main.posx_s;
     mbox->main.text_posx_f = mbox->main.posx_f;
     mbox->main.text_posy_s = mbox->main.posy_s + 16;
     mbox->main.text_posy_f = mbox->main.posy_f;
    }

 mbox->main.text_bcol = osdsch->dialogue_text_bcol;
 mbox->main.text_fcol = osdsch->dialogue_text_fcol;

 if (mbox->icon)
    put_xpm(mbox->icon, mbox->main.posx_s + ((mbox->main.attr & 0x07) + 4),
    mbox->main.posy_s + ((mbox->main.attr & 0x07) + 26));

 if (! (mbox->attr & MBOX_ATTR_MAXIMISED))
    {
     last_posx_s = mbox->main.posx_f - ((mbox->main.attr & 0x07) + 26);
     last_posx_f = mbox->main.posx_f - ((mbox->main.attr & 0x07) + 4);
     draw_box(&mbox->main);
    }
 else
    {
     last_posx_s = mbox->main.posx_f - ((mbox->main.attr & 0x07) + 23);
     last_posx_f = mbox->main.posx_f - ((mbox->main.attr & 0x07) + 1);
    }

 text_box(&mbox->main, mbox->components & BOX_COMP_MAX);
}

//==============================================================================
// Create the close box.
//
//   pass: void
// return: void
//==============================================================================
static void create_close_box (void)
{
 if (! (mbox->components & BOX_COMP_CLOSE))
    return;

 if (last_posx_s < (mbox->main.posx_s + (mbox->main.attr & 0x07) + 3))
    return;

 mbox->close.posx_s = last_posx_s;
 mbox->close.posx_f = last_posx_f;

 if (! (mbox->attr & MBOX_ATTR_MAXIMISED))
    mbox->close.posy_s = mbox->main.posy_s + ((mbox->main.attr & 0x07) + 2);
 else
    mbox->close.posy_s = mbox->main.posy_s + ((mbox->main.attr & 0x07) + 1);
 mbox->close.posy_f = mbox->close.posy_s + 11 + 1;

 if (mbox->close.attr & BOX_ATTR_HIGH)
    mbox->close.bcol = osdsch->widget_main_bcol_hl;
 else
    mbox->close.bcol = osdsch->widget_main_bcol_ll;

 if ((emu.osd_focus) || (mbox->close.attr & BOX_ATTR_HIGH))
    {
     mbox->close.fcol = osdsch->widget_main_fcol_hl;
     close_xpm[1] = colour_xpm_hl;
    }
 else
    {
     mbox->close.fcol = osdsch->widget_main_fcol_ll;
     close_xpm[1] = colour_xpm_ll;
    }

 fill_box(&mbox->close);
 draw_box(&mbox->close);
 put_xpm(close_xpm, mbox->close.posx_s + 1, mbox->close.posy_s + 1);
}

//==============================================================================
// Create the maximising box.
//
//   pass: void
// return: void
//==============================================================================
static void create_maximising_box (void)
{
 if (! (mbox->components & BOX_COMP_MAX))
    return;

 last_posx_s -= 26;
 last_posx_f -= 26;

 if (last_posx_s < (mbox->main.posx_s + (mbox->main.attr & 0x07) + 3))
    return;

 mbox->max.posx_s = last_posx_s;
 mbox->max.posx_f = last_posx_f;

 if (! (mbox->attr & MBOX_ATTR_MAXIMISED))
    mbox->max.posy_s = mbox->main.posy_s + ((mbox->main.attr & 0x07) + 2);
 else
    mbox->max.posy_s = mbox->main.posy_s + ((mbox->main.attr & 0x07) + 1);
 mbox->max.posy_f = mbox->max.posy_s + 11 + 1;

 if (mbox->max.attr & BOX_ATTR_HIGH)
    mbox->max.bcol = osdsch->widget_main_bcol_hl;
 else
    mbox->max.bcol = osdsch->widget_main_bcol_ll;

 if ((emu.osd_focus) || (mbox->max.attr & BOX_ATTR_HIGH))
    {
     mbox->max.fcol = osdsch->widget_main_fcol_hl;
     maximise_a_xpm[1] = colour_xpm_hl;
     maximise_b_xpm[1] = colour_xpm_hl;
    }
 else
    {
     mbox->max.bcol = osdsch->widget_main_bcol_ll;
     mbox->max.fcol = osdsch->widget_main_fcol_ll;
     maximise_a_xpm[1] = colour_xpm_ll;
     maximise_b_xpm[1] = colour_xpm_ll;
    }

 fill_box(&mbox->max);
 draw_box(&mbox->max);

 if (mbox->attr & MBOX_ATTR_MAXIMISED)
    put_xpm(maximise_b_xpm, mbox->max.posx_s + 1, mbox->max.posy_s + 1);
 else
    put_xpm(maximise_a_xpm, mbox->max.posx_s + 1, mbox->max.posy_s + 1);
}

//==============================================================================
// Create the minimise box.
//
//   pass: void
// return: void
//==============================================================================
static void create_minimise_box (void)
{
 if (! (mbox->components & BOX_COMP_MIN))
    return;

 last_posx_s -= 26;
 last_posx_f -= 26;

 if (last_posx_s < (mbox->main.posx_s + (mbox->main.attr & 0x07) + 3))
    return;

 mbox->min.posx_s = last_posx_s;
 mbox->min.posx_f = last_posx_f;

 if (! (mbox->attr & MBOX_ATTR_MAXIMISED))
    mbox->min.posy_s = mbox->main.posy_s + ((mbox->main.attr & 0x07) + 2);
 else
    mbox->min.posy_s = mbox->main.posy_s + ((mbox->main.attr & 0x07) + 1);
 mbox->min.posy_f = mbox->min.posy_s + 11 + 1;

 if (mbox->min.attr & BOX_ATTR_HIGH)
    mbox->min.bcol = osdsch->widget_main_bcol_hl;
 else   
    mbox->min.bcol = osdsch->widget_main_bcol_ll;    

 if ((emu.osd_focus) || (mbox->min.attr & BOX_ATTR_HIGH))
    {
     mbox->min.fcol = osdsch->widget_main_fcol_hl;
     minimise_xpm[1] = colour_xpm_hl;
    }
 else
    {
     mbox->min.fcol = osdsch->widget_main_fcol_ll;     
     minimise_xpm[1] = colour_xpm_ll;
    }

 fill_box(&mbox->min);
 draw_box(&mbox->min);
 put_xpm(minimise_xpm, mbox->min.posx_s + 1, mbox->min.posy_s + 1);
}

//==============================================================================
// Create the title box.
//
//   pass: void
// return: void
//==============================================================================
static void create_title_box (void)
{
 int x;

 if (! (mbox->components & BOX_COMP_TITLE))
    return;

 last_posx_s -= 4;

 if (last_posx_s < (mbox->main.posx_s + (mbox->main.attr & 0x07) + 6))
    return;

 if (! (mbox->attr & MBOX_ATTR_MAXIMISED))
    {
     mbox->title.posx_s = mbox->main.posx_s + (mbox->main.attr & 0x07) + 4;
     mbox->title.posy_s = mbox->main.posy_s + ((mbox->main.attr & 0x07) + 2);
    }
 else
    {
     mbox->title.posx_s = mbox->main.posx_s + (mbox->main.attr & 0x07) + 1;
     mbox->title.posy_s = mbox->main.posy_s + ((mbox->main.attr & 0x07) + 1);
    }
 mbox->title.posx_f = last_posx_s;
 mbox->title.posy_f = mbox->title.posy_s + 11 + 1;

 if (emu.osd_focus)
    {
     mbox->title.fcol = osdsch->widget_main_fcol_hl;
     mbox->title.text_fcol = osdsch->widget_text_fcol_hl;
     // we use the low lighting value for background as there is no
     // higlighting used for the title boxes
     mbox->title.text_bcol = osdsch->widget_text_bcol_ll;
     mbox->title.bcol = osdsch->widget_main_bcol_ll;
    }
 else
    {
     mbox->title.bcol = osdsch->widget_main_bcol_ll;
     mbox->title.fcol = osdsch->widget_main_fcol_ll;
     mbox->title.text_bcol = osdsch->widget_text_bcol_ll;
     mbox->title.text_fcol = osdsch->widget_text_fcol_ll;
    }

 x = ((mbox->title.posx_f - mbox->title.posx_s) -
     (strlen(mbox->title.text) * OSD_FONT_WIDTH));
 if (x < 0)
    mbox->title.text_posx_s = mbox->title.posx_s + 1;
 else
    mbox->title.text_posx_s = mbox->title.posx_s + (x / 2);
 mbox->title.text_posx_f = mbox->title.posx_f;
 mbox->title.text_posy_s = mbox->title.posy_s +
 ((11 - OSD_FONT_DEPTH) / 2) + 2;
 
 mbox->title.text_posy_f = mbox->title.posy_f;

 fill_box(&mbox->title);
 draw_box(&mbox->title);
 text_box(&mbox->title, 0);
}

//==============================================================================
// Create a button box.
//
//   pass: int btn
// return: void
//==============================================================================
static void create_button_box (int btn)
{
 int button_gap;

 if ((mbox->attr & MBOX_ATTR_VBTNS_LJ) || (mbox->attr & MBOX_ATTR_VBTNS_RJ))
    {
     if (mbox->attr & MBOX_ATTR_VBTNS_LJ)
        {
         mbox->btn[btn].posx_s = mbox->main.posx_s +
                                 (mbox->main.attr & 0x07) + 4;
         mbox->btn[btn].posx_f = mbox->btn[btn].posx_s + mbox->bwidth;
        }
     else
        {
         mbox->btn[btn].posx_f = mbox->main.posx_f -
                                 ((mbox->main.attr & 0x07) + 4);
         mbox->btn[btn].posx_s = mbox->btn[btn].posx_f - mbox->bwidth;
        }
     mbox->btn[btn].posy_s = mbox->title.posy_f + 3 + (mbox->bdepth) * btn;
     mbox->btn[btn].posy_f = mbox->btn[btn].posy_s + mbox->bdepth;
    }
 else
    {
     button_gap = ((mbox->main.posx_f - mbox->main.posx_s) -
                  (mbox->bwidth * mbox->buttons)) / (mbox->buttons + 1);
     mbox->btn[btn].posx_s = mbox->main.posx_s + (mbox->bwidth * (btn + 0)) +
                             (button_gap * (btn + 1));
     mbox->btn[btn].posx_f = mbox->main.posx_s + (mbox->bwidth * (btn + 1)) +
                             (button_gap * (btn + 1));
     mbox->btn[btn].posy_s = mbox->main.posy_f - ((mbox->main.attr & 0x07) +
                             1 + mbox->bdepth);
     mbox->btn[btn].posy_f = mbox->main.posy_f -
                             ((mbox->main.attr & 0x07) + 2);
    }

 mbox->btn[btn].text_posx_s = mbox->btn[btn].posx_s +
         (mbox->bwidth - (strlen(mbox->btn[btn].text) * OSD_FONT_WIDTH)) / 2;
 mbox->btn[btn].text_posx_f = mbox->btn[btn].posx_f;
 mbox->btn[btn].text_posy_s = mbox->btn[btn].posy_s +
         ((mbox->bdepth - OSD_FONT_DEPTH) / 2) + 1;
 mbox->btn[btn].text_posy_f = mbox->btn[btn].posy_f;

 if (mbox->btn[btn].attr & BOX_ATTR_HIGH)
    {
     mbox->btn[btn].bcol = osdsch->button_main_bcol_hl;
     mbox->btn[btn].fcol = osdsch->button_main_fcol_hl;
     mbox->btn[btn].text_bcol = osdsch->button_text_bcol_hl;
     mbox->btn[btn].text_fcol = osdsch->button_text_fcol_hl;
    }
 else
    {
     mbox->btn[btn].bcol = osdsch->button_main_bcol_ll;
     mbox->btn[btn].fcol = osdsch->button_main_fcol_ll;
     mbox->btn[btn].text_bcol = osdsch->button_text_bcol_ll;
     mbox->btn[btn].text_fcol = osdsch->button_text_fcol_ll;
    }   

 fill_box(&mbox->btn[btn]);
 draw_box(&mbox->btn[btn]);
 text_box(&mbox->btn[btn], 0);
}

//==============================================================================
// Set minimised window values.
//
//   pass: void
// return: void
//==============================================================================
static void set_minimised_values (void)
{
 int crt_w;
 int crt_h;

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;

 mbox->main.posx_s = (crt_w / 2) - MINIMISED_BOX_WIDTH / 2;
 mbox->main.posx_f = (crt_w / 2) + MINIMISED_BOX_WIDTH / 2;
 mbox->main.posy_s = crt_h - 1;
 mbox->main.posy_f = crt_h - 1;
}

//==============================================================================
// Draw the current dialogue.
//
//   pass: void
// return: void
//==============================================================================
static void draw_dialogue (void)
{
 int i;
 spf = screen->format;

 SDL_LockSurface(screen);

 create_dialogue_box();

 // save processor time if dialogue is minimised
 if (! mbox->minimised)
    {
     create_close_box();
     create_maximising_box();
     create_minimise_box();
     create_title_box();
     for (i = 0; i < mbox->buttons; i++)
       create_button_box(i);
    }

 SDL_UnlockSurface(screen);
}

//==============================================================================
// Animate the minimising of a dialogue.
//
//   pass: void
// return: void
//==============================================================================
static void animate_minimising (void)
{
 int x;
 int y;
 int crt_w;
 int crt_h;

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;

 if (animating == -1)
    {
     animating = OSD_ANIMATED_FRAMES + 1;
     animated_mbox.main.fcol = osdsch->dialogue_main_fcol;
     animated_mbox.main.attr = BOX_ATTR_PIXEL1;

     // calculate the number of +/- X pixels to move
     x = ((crt_w / 2) - MINIMISED_BOX_WIDTH / 2) - animated_mbox.main.posx_s;

     // calculate the number of Y pixels to move
     y = (crt_h - 1) - animated_mbox.main.posy_s;

     // calculate the shrinkage values
     animate_shrink_x = ((animated_mbox.main.posx_f -
                          animated_mbox.main.posx_s) - MINIMISED_BOX_WIDTH) /
                          OSD_ANIMATED_FRAMES;
     animate_add_x_f = (animated_mbox.main.posx_f - animate_shrink_x) -
                        animated_mbox.main.posx_s;
     animate_add_x_s = x / OSD_ANIMATED_FRAMES;

     animate_shrink_y = (animated_mbox.main.posy_f -
                         animated_mbox.main.posy_s) / OSD_ANIMATED_FRAMES;
     animate_add_y_f = animated_mbox.main.posy_f - animated_mbox.main.posy_s;
     animate_add_y_s = y / OSD_ANIMATED_FRAMES;
    }
 else
    // if time to redraw the outline in a new screen position
    if (animate_update)
       {
        animate_update = 0;

        animated_mbox.main.posx_s += animate_add_x_s;
        animated_mbox.main.posx_f = animated_mbox.main.posx_s +
                                    animate_add_x_f;
        animate_add_x_f -= animate_shrink_x;

        animated_mbox.main.posy_s += animate_add_y_s;
        animated_mbox.main.posy_f = animated_mbox.main.posy_s +
                                    animate_add_y_f;
        animate_add_y_f -= animate_shrink_y;
        animating--;
       }

 if (animating)
    draw_box(&animated_mbox.main);
 else
    {
     memcpy(mbox, &minimised_mbox, sizeof(mbox_t));
     set_minimised_values();
     draw_box(&mbox->main);
     crtc_set_redraw();
     crtc.update = 1;
    }
}

//==============================================================================
// Handle keys while the console dialogue has the focus.
//
// A command line string is edited and will be processed when the ENTER key
// is pressed.  As the string is being edited the OSD console will show the
// input.
//
//   pass: void
// return: void
//==============================================================================
static void console_key_handler (void)
{
 char s[1024];

 int c = osd_getkey();

 switch (c)
    {
     case 0 :
        // test for special keys, left, right, etc
        break;
     case 8 :
        if (cmd_length)
           {
            cmd_length--;
            command[--cmd_putpos] = 0;
            xputchar(c);
           }
        break;
     case 13 :
        xputchar('\n');
        // prepend "ubee512 " as argv[0]
        snprintf(s, sizeof(s), "ubee512 %s", command);
        command[0] = 0;
        cmd_putpos = 0;
        cmd_length = 0;
        options_make_pointers(s);
        console.xstdin = 0;
        console_set_keydevice(1);
        options_process(c_argc, c_argv);
        console_set_keydevice(0);
        gui_status_update();
        break;
     case 127 :
        if (cmd_length)
           {
            cmd_length--;
            command[--cmd_putpos] = 0;
            xprintf("\b \b");
           }
        break;
     default :
        if ((c) > 31)
           {
            if (cmd_putpos < (sizeof(command)-1))
               {
                cmd_length++;
                command[cmd_putpos++] = c;
                command[cmd_putpos] = 0;
                xputchar(c);
               }
            }
        break;
    }
 xflush();
}

//==============================================================================
// Handle dialogue action for the dialogue that has the focus.
//
//   pass: void
// return: void
//==============================================================================
static void dialogue_action (void)
{
 switch (mbox->dialogue)
    {
     case DIALOGUE_MENU :
        switch (mbox->result)
           {
            case MENU_BTN_ABOUT :
               osd_set_dialogue(DIALOGUE_ABOUT);
               break;
            case MENU_BTN_CONSOLE :
               osd_set_dialogue(DIALOGUE_CONSOLE);
               break;
            case MENU_BTN_OUTPUT :
               osd_set_dialogue(DIALOGUE_OUTPUT);
               break;
            case MENU_BTN_FULLSCREEN :
               osd_dialogue_exit();    // exit OSD before changing screen
               video_command(EMU_CMD_FULLSCR, 0);
               break;
            case MENU_BTN_SOUND :
               audio_command(EMU_CMD_MUTE);
               break;
            case MENU_BTN_VOLUMEI :
               audio_command(EMU_CMD_VOLUMEI);
               break;
            case MENU_BTN_VOLUMED :
               audio_command(EMU_CMD_VOLUMED);
               break;
            case MENU_BTN_TAPE :
               tape_command(EMU_CMD_TAPEREW);
               tapfile_command(EMU_CMD_TAPEREW);               
               break;
            case MENU_BTN_RESET :
               osd_set_dialogue(DIALOGUE_RESET);
               break;
            case MENU_BTN_POWERCYC :
               osd_set_dialogue(DIALOGUE_POWERCYC);
               break;
            case MENU_BTN_EXIT :
               osd_set_dialogue(DIALOGUE_EXIT);
               break;
            default :
               break;
           }
        break;

     case DIALOGUE_CONSOLE :
        console_key_handler();
        break;

     case DIALOGUE_EXIT :
        break;

     case DIALOGUE_RESET :
        break;

     case DIALOGUE_DEVMESG :
        break;

     case DIALOGUE_OPENGL : // OpenGL video rendering mode information dialogue
        if (mbox->result == OKCANCEL_BTN_OK)
           {
            messages.opengl_no = 1;
            write_id_file();
           }
        break;

     case DIALOGUE_OUTPUT :
        switch (mbox->result)
           {
            case OUTPUT_BTN_NONE :
               console_set_devices(CONSOLE_NONE);
               break;
            case OUTPUT_BTN_OSD :
               console_set_devices(CONSOLE_OSD);
               break;
            case OUTPUT_BTN_STDOUT :
               console_set_devices(CONSOLE_STDOUT);
               break;
            case OUTPUT_BTN_BOTH :
               console_set_devices(CONSOLE_OSD | CONSOLE_STDOUT);
               break;
           }
        break;
    }

 gui_status_update();
}

//==============================================================================
// Update dialogue highlight attributes.
//
//   pass: int mouse_x
//         int mouse_y
// return: void
//==============================================================================
void update_dialogue_highlights (int mouse_x, int mouse_y)
{
 int x;
 int y;
 int i;

 // convert the mouse X, Y values to CRTC scaled values
 video_convert_mouse_to_crtc_xy(mouse_x, mouse_y, &x, &y);

 // check and set/clear the close box attribute
 if (check_xy_inside(&mbox->close, x, y))
    mbox->close.attr |= BOX_ATTR_HIGH;
 else
    mbox->close.attr &= ~BOX_ATTR_HIGH;

 // check and set/clear the maximising box attribute
 if (check_xy_inside(&mbox->max, x, y))
    mbox->max.attr |= BOX_ATTR_HIGH;
 else
    mbox->max.attr &= ~BOX_ATTR_HIGH;

 // check and set/clear the minimising box attribute
 if (check_xy_inside(&mbox->min, x, y))
    mbox->min.attr |= BOX_ATTR_HIGH;
 else
    mbox->min.attr &= ~BOX_ATTR_HIGH;

 // check and set/clear the button attributes
 for (i = 0; i < mbox->buttons; i++)
    {
     if (check_xy_inside(&mbox->btn[i], x, y))
        mbox->btn[i].attr |= BOX_ATTR_HIGH;
     else
        mbox->btn[i].attr &= ~BOX_ATTR_HIGH;
    }

 // need to force a redraw to show attribute and drag changes otherwise may
 // be slow to display if CRTC not doing anything
 crtc_set_redraw();
}

//==============================================================================
// Exit dialogue
//
//   pass: void
// return: void
//==============================================================================
void osd_dialogue_exit (void)
{
 int dialogue;

 mbox->dialogue = DIALOGUE_NOTINUSE;

 dialogue = osd_get_pending();

 if (dialogue != -1)
    osd_dialogue(dialogue);
 else
    {
     // restore any minimised dialogue data for next time it's needed
     if (mbox->minimised)
        memcpy(mbox, &minimised_mbox, sizeof(mbox_t));

     emu.display_context = EMU_EMU_CONTEXT;
     emu.osd_focus = 0;
     osd.dialogue = 0;
     keyb_set_unicode(emu.osd_focus);
    }

 crtc_set_redraw();
}

//==============================================================================
// Get a dialogue result.
//
//   pass: int dialogue
// return: int                          dialogue result
//==============================================================================
int osd_dialogue_result (int dialogue)
{
 return dialogues[dialogue].result;
}

//==============================================================================
// Get the last OSD key saved by the console dialogue handler.
//
//   pass: void
// return: int                          OSD key
//==============================================================================
int osd_getkey (void)
{
 int k = osd.key;
 osd.key = 0;
 return k;
}

//==============================================================================
// Key down event handler.
//
//   pass: void
// return: void
//==============================================================================
void osd_keydown_event (void)
{
 SDLKey key;
 int exit_dialogue = 0;

 key = emu.event.key.keysym.sym;
 mbox->result = 0;

// if console dialogue is active then check keys
 if ((mbox->dialogue == DIALOGUE_CONSOLE) &&
    (console_get_devices() & CONSOLE_OSD))
    {
     int c = emu.event.key.keysym.unicode & 0x7F;
     osd.key = c;

     // if the 'help option' is currently active
     if ((help.state) && (help.state != -1))
        {
         options_usage_state(&help);
         return;
        }
    }

 switch (key)
    {
     case SDLK_ESCAPE :
        mbox->result = 0;
        exit_dialogue = 1;
        break;
     case SDLK_RETURN :
        if (! mbox->buttons)
           break;
        mbox->result = mbox->button_focus + 1;
        exit_dialogue = 1;
        break;
     case SDLK_LEFT :
     case SDLK_RIGHT :
     case SDLK_UP :
     case SDLK_DOWN :
     case SDLK_TAB :
        if (! mbox->buttons)
           break;
        mbox->btn[mbox->button_focus].attr &= ~BOX_ATTR_DASHED;
        if ((key == SDLK_LEFT) || (key == SDLK_UP))
           {
            if (--mbox->button_focus < 0)
               mbox->button_focus = mbox->buttons-1;
           }
        else
           {
            if (++mbox->button_focus >= mbox->buttons)
               mbox->button_focus = 0;
            }
        mbox->btn[mbox->button_focus].attr |= BOX_ATTR_DASHED;
        draw_dialogue();
        video_render();
        break;
     default :
        break;
    }

 // handle dialogue action
 dialogue_action();

 if (exit_dialogue)
    osd_dialogue_exit();
}

//==============================================================================
// Key up event handler.
//
//   pass: void
// return: void
//==============================================================================
void osd_keyup_event (void)
{
// code looks like it should be removed
#if 0
 SDLKey key;

 key = emu.event.key.keysym.sym;
#endif
}

//==============================================================================
// Determine if the current dialogue or emulation has the focus.  This is
// intended to be called when the left mouse mutton has been clicked from
// the GUI module.
//
//   pass: void
// return: void
//==============================================================================
void osd_set_focus (void)
{
 int x;
 int y;

 if (emu.display_context == EMU_EMU_CONTEXT)
    emu.osd_focus = 0;
 else
    {
     // convert the mouse X, Y values to CRTC scaled values
     video_convert_mouse_to_crtc_xy(emu.event.motion.x,
                                    emu.event.motion.y, &x, &y);
     if (! mbox->minimised)
        emu.osd_focus = check_xy_in(&mbox->main, x, y);
    }
 keyb_set_unicode(emu.osd_focus);

 if (emu.display_context == EMU_OSD_CONTEXT)
    {
     draw_dialogue();
     video_render();
    }
}

//==============================================================================
// Mouse button down event handler.
//
//   pass: void
// return: void
//==============================================================================
void osd_mousebuttondown_event (void)
{
 int mouse_x;
 int mouse_y;
 int crt_w;
 int crt_h;

 int i;
 int exit_dialogue = 0;

 if (! gui.button_l)
    return;

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;

 // close dialogue if the close box clicked on
 if (mbox->close.attr & BOX_ATTR_HIGH)
    {
     mbox->result = 0;
     osd_dialogue_exit();
     return;
    }

 // toggle maximise dialogue if maximise box clicked on
 if (mbox->max.attr & BOX_ATTR_HIGH)
    {
     if (! (mbox->attr & MBOX_ATTR_MAXIMISED))
        {
         mbox->attr |= MBOX_ATTR_MAXIMISED;
         memcpy(&maximised_mbox, mbox, sizeof(mbox_t));
         mbox->main.posx_s = 0;
         mbox->main.posx_f = crt_w - 1;
         mbox->main.posy_s = 0;
         mbox->main.posy_f = crt_h - 1;
         mbox->width = (mbox->main.posx_f - mbox->main.posx_s) + 1;
         mbox->depth = (mbox->main.posy_f - mbox->main.posy_s) + 1;
         mbox->main.attr = (mbox->main.attr & ~0x07) | BOX_ATTR_PIXEL0;
        }
     else
        {
         memcpy(mbox, &maximised_mbox, sizeof(mbox_t));
         mbox->attr &= ~MBOX_ATTR_MAXIMISED;
        }
     draw_dialogue();
     video_render();
     // get the current mouse X, Y values and update dialogue status
     SDL_GetMouseState(&mouse_x, &mouse_y);
     update_dialogue_highlights(mouse_x, mouse_y);
     return;
    }

 // minimise dialogue if minimised box clicked on
 if ((mbox->min.attr & BOX_ATTR_HIGH) && (! mbox->minimised))
    {
     mbox->minimised = 1;
     memcpy(&minimised_mbox, mbox, sizeof(mbox_t));
     memcpy(&animated_mbox, mbox, sizeof(mbox_t));

     if (osd.flags && OSD_FLAG_ANIMATE)
        animating = -1;  // start the minimising animation
     else
        {
         memcpy(mbox, &minimised_mbox, sizeof(mbox_t));
         set_minimised_values();
         draw_box(&mbox->main);
         crtc_set_redraw();
         crtc.update = 1;
        }

     emu.osd_focus = 0;
     keyb_set_unicode(emu.osd_focus);
     msecs_before = time_get_ms();
     draw_dialogue();
     video_render();
     // get the current mouse X, Y values and update dialogue status
     SDL_GetMouseState(&mouse_x, &mouse_y);
     update_dialogue_highlights(mouse_x, mouse_y);
     return;
    }

 // get the button result if one clicked on
 mbox->result = 0;

 for (i = 0; i < mbox->buttons; i++)
    {
     if (mbox->btn[i].attr & BOX_ATTR_HIGH)
        {
         mbox->result = i + 1;
         exit_dialogue = ((mbox->btn[i].attr & BOX_ATTR_NOEXIT) == 0);
         break;
        }
    }

 // handle dialogue action
 dialogue_action();

 if (exit_dialogue)
    osd_dialogue_exit();
}

//==============================================================================
// Mouse button up event handler.
//
//   pass: void
// return: void
//==============================================================================
void osd_mousebuttonup_event (void)
{
}

//==============================================================================
// Mouse motion event handler.
//
// When using OpenGL mode the X, Y motion values returned are true screen
// co-ordinates and not the resized values.  This requires the values to be
// converted back to CRTC like values before they can be used.
//
//   pass: void
// return: void
//==============================================================================
void osd_mousemotion_event (void)
{
 int x;
 int y;

 int crt_w;
 int crt_h;

 int drag_x_s_ok = 0;
 int drag_x_f_ok = 0;
 int drag_y_s_ok = 0;
 int drag_y_f_ok = 0;

 int border;

 box_t temp_box;

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;

 // convert the mouse X, Y values to CRTC scaled values
 video_convert_mouse_to_crtc_xy(emu.event.motion.x,
                                emu.event.motion.y, &x, &y);

 memcpy(&temp_box, &mbox->main, sizeof(box_t));

 // if minimised then check if mouse is pointing at the minimised dialogue
 if ((mbox->minimised) && (animating == 0))
    {
     temp_box.posy_s -= 3; // make it easier to gain focus
     emu.osd_focus = check_xy_in(&temp_box, x, y);
     keyb_set_unicode(emu.osd_focus);

     // if have the focus of the minimised box restore it
     if (emu.osd_focus)
        {
         memcpy(mbox, &minimised_mbox, sizeof(mbox_t));
         crtc_set_redraw();
         mouse_x_last = x;
         mouse_y_last = y;
         mbox->minimised = 0;
         return;
        }
     crtc_set_redraw();
     mouse_x_last = x;
     mouse_y_last = y;
     return;
    }

 // update dialogue highlighting attributes for buttons, etc
 update_dialogue_highlights(emu.event.motion.x, emu.event.motion.y);

 // dragging not reliable so keep drag context if button kept down
 if (gui.button_l)
    {
     if (! drag_window)
        drag_window = check_window_xy_in(&mbox->main, x, y);
    }
 else
    drag_window = 0;

 if (drag_window)
    {
     mbox->reset = 1;
     if (mbox->main.attr & 0x07)
        border = (mbox->main.attr & 0x07) - 1;
     else
        border = 0;

     switch (drag_window)
        {
         case 0 : // nothing
            break;
         case 1 : // left top corner
            temp_box.posx_s += (x - mouse_x_last);
            temp_box.posy_s += (y - mouse_y_last);
            break;
         case 2 : // top right corner
            temp_box.posx_f += (x - mouse_x_last);
            temp_box.posy_s += (y - mouse_y_last);
            break;
         case 3 : // bottom right corner
            temp_box.posx_f += (x - mouse_x_last);
            temp_box.posy_f += (y - mouse_y_last);
            break;
         case 4 : // bottom left corner
            temp_box.posx_s += (x - mouse_x_last);
            temp_box.posy_f += (y - mouse_y_last);
            break;
         case 5 : // left side
            temp_box.posx_s += (x - mouse_x_last);
            break;
         case 6 : // top side
            temp_box.posy_s += (y - mouse_y_last);
            break;
         case 7 : // right side
            temp_box.posx_f += (x - mouse_x_last);
            break;
         case 8 : // bottom side
            temp_box.posy_f += (y - mouse_y_last);
            break;
         case 9 : // whole window
            temp_box.posx_s += (x - mouse_x_last);
            temp_box.posx_f += (x - mouse_x_last);
            temp_box.posy_s += (y - mouse_y_last);
            temp_box.posy_f += (y - mouse_y_last);
            break;
        }

     // only resize/drag dialogue if still large enough to contain a close box
     if ((((temp_box.posx_f - temp_box.posx_s) > 37) &&
        ((temp_box.posy_f - temp_box.posy_s) > 23)) && (drag_window))
        {
         if (drag_window != 9)
            {
             drag_x_s_ok = (temp_box.posx_s >= 0) &&
                           ((temp_box.posx_s + border) < crt_w);
             drag_x_f_ok = (temp_box.posx_f >= border) &&
                           (temp_box.posx_f < crt_w);
             drag_y_s_ok = (temp_box.posy_s >= 0) &&
                           ((temp_box.posy_s + border) < crt_h);
             drag_y_f_ok = (temp_box.posy_f >= border) &&
                           (temp_box.posy_f < crt_h);
            }
         else
            {
             drag_x_s_ok = 1;
             drag_x_f_ok = 1;
             drag_y_s_ok = 1;
             drag_y_f_ok = 1;
            }

         if (drag_x_s_ok)
             mbox->main.posx_s = temp_box.posx_s;
         if (drag_x_f_ok)
             mbox->main.posx_f = temp_box.posx_f;
         if (drag_y_s_ok)
             mbox->main.posy_s = temp_box.posy_s;
         if (drag_y_f_ok)
             mbox->main.posy_f = temp_box.posy_f;

         mbox->width = (mbox->main.posx_f - mbox->main.posx_s) + 1;
         mbox->depth = (mbox->main.posy_f - mbox->main.posy_s) + 1;
        }
    }

 // need to force a redraw to show attribute and drag changes otherwise may
 // be slow to display if CRTC not doing anything
 crtc_set_redraw();

 mouse_x_last = x;
 mouse_y_last = y;
}

//==============================================================================
// Set the dialogue console size and location variables.
//
// This must only be called when 'crtc' and 'osdsch' have values assigned.
//
//   pass: int action                   if OSD_POS_UPDATE then force all values
//                                      to be updated.
// return: void
//==============================================================================
static void set_console_sizepos (int action)
{
 int crt_w;
 int crt_h;

 int x;
 int y;

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;
 
 if (action == OSD_POS_UPDATE)
    {
     mbox->width = -1;
     mbox->depth = -1;
     mbox->main.posx_s = -1;
     mbox->main.posy_s = -1;
    }   

 if (mbox->width == -1)
    {
     // set the console width
     if (osdsch->console_width == OSD_CON_MAX)
        mbox->width = crt_w;
     if ((osdsch->console_width >= OSD_CON_PERCENT_001) &&
        (osdsch->console_width <= OSD_CON_PERCENT_100))
        mbox->width = (int)(crt_w *
        (0.01 * (osdsch->console_width - OSD_CON_PERCENT_000)));
     if (osdsch->console_width < OSD_CON_PERCENT_000)
        mbox->width = 8 + 8 + osdsch->console_width * 8;
    }

 if (mbox->depth == -1)
    {
     // set the console depth
     if (osdsch->console_depth == OSD_CON_MAX)
        mbox->depth = crt_h;
     if ((osdsch->console_depth >= OSD_CON_PERCENT_001) &&
        (osdsch->console_depth <= OSD_CON_PERCENT_100))
        mbox->depth = (int)(crt_h *
        (0.01 * (osdsch->console_depth - OSD_CON_PERCENT_000)));
     if (osdsch->console_depth < OSD_CON_PERCENT_000)
        mbox->depth = 17 + 10 + OSD_FONT_DEPTH * osdsch->console_depth;
    }

 if (mbox->main.posx_s == -1)
    {
     // set the console X location
     switch (osdsch->console_pos_x)
        {
         case OSD_CON_CENTER :
            mbox->main.posx_s = (crt_w / 2) - (mbox->width / 2);
            mbox->main.posx_f = (crt_w / 2) + (mbox->width / 2) - 1;
            break;
         case OSD_CON_LEFT :
            mbox->main.posx_s = 0;
            mbox->main.posx_f = mbox->width - 1;
            break;
         case OSD_CON_RIGHT :
            mbox->main.posx_s = crt_w - mbox->width;
            mbox->main.posx_f = crt_w - 1;
            break;
         default :
            if ((osdsch->console_pos_x >= OSD_CON_PERCENT_001) &&
               (osdsch->console_pos_x <= OSD_CON_PERCENT_100))
               x = (int)(crt_w *
               (0.01 * (osdsch->console_pos_x - OSD_CON_PERCENT_000)));
            else
               x = OSD_FONT_WIDTH * osdsch->console_pos_x;
            mbox->main.posx_s = x;
            mbox->main.posx_f = x + mbox->width;
            break;
        }
    }   

 if (mbox->main.posy_s == -1)
    {
     // set the console Y location
     switch (osdsch->console_pos_y)
        {
         case OSD_CON_CENTER :
            mbox->main.posy_s = (crt_h / 2) - (mbox->depth / 2);
            mbox->main.posy_f = (crt_h / 2) + (mbox->depth / 2) - 1;
            break;
         case OSD_CON_TOP :
            mbox->main.posy_s = 0;
            mbox->main.posy_f = mbox->depth - 1;
            break;
         case OSD_CON_BOTTOM :
            mbox->main.posy_s = crt_h - mbox->depth;
            mbox->main.posy_f = crt_h - 1;
            break;
         default :
            if ((osdsch->console_pos_y >= OSD_CON_PERCENT_001) &&
               (osdsch->console_pos_y <= OSD_CON_PERCENT_100))
               y = (int)(crt_h *
               (0.01 * (osdsch->console_pos_y - OSD_CON_PERCENT_000)));
            else
               y = crtc.scans_per_row * osdsch->console_pos_y;
            mbox->main.posy_s = y;
            mbox->main.posy_f = y + mbox->depth;
            break;
        }
    }
}

//==============================================================================
// Set the dialogue box width, co-ordinates depend on the value passed to
// the action value.
//
// Save the current mouse location.
//
//   pass: int action
// return: void
//==============================================================================
void osd_set_dialogue_pos (int action)
{
 int mouse_x;
 int mouse_y;

 int crt_w;
 int crt_h;

 // get the current mouse X, Y values
 SDL_GetMouseState(&mouse_x, &mouse_y);

 // convert the mouse X, Y values to CRTC scaled values
 video_convert_mouse_to_crtc_xy(mouse_x, mouse_y,
                                &mouse_x_last, &mouse_y_last);

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;

 if ((mbox->attr & MBOX_ATTR_RESIZABLE) || (action == OSD_POS_UPDATE))
   {
    set_console_sizepos(action);
    return;
   } 

 if (mbox->minimised)
    {
     // center the dialogue window when it's restored
     minimised_mbox.main.posx_s = (crt_w / 2) - (dialogue_width() / 2);
     minimised_mbox.main.posx_f = ((crt_w / 2) + (dialogue_width() / 2)) - 1;
     minimised_mbox.main.posy_s = (crt_h / 2) - (dialogue_depth() / 2);
     minimised_mbox.main.posy_f = ((crt_h / 2) + (dialogue_depth() / 2)) - 1;

     // place the dialogue window in the minimised location
     set_minimised_values();
     return;
    }

 // set dialogue to mouse cursor position, if off display then locate to
 // maximum limits.
 if ((mbox->attr & MBOX_ATTR_MOUSEPOS) && (action != OSD_POS_UPDATE))
    {
     if ((mouse_x_last + dialogue_width()) > crt_w)
        {
         mbox->main.posx_s = crt_w - dialogue_width();
         mbox->main.posx_f = crt_w - 1;
        }
     else
        {
         mbox->main.posx_s = mouse_x_last;
         mbox->main.posx_f = mouse_x_last + (dialogue_width()-1);
        }
     if ((mouse_y_last + dialogue_depth()) > crt_h)
        {
         mbox->main.posy_s = crt_h - dialogue_depth();
         mbox->main.posy_f = crt_h - 1;
        }
     else
        {
         mbox->main.posy_s = mouse_y_last;
         mbox->main.posy_f = mouse_y_last + (dialogue_depth()-1);
        }
     return;
    }

 // center the dialogue window
 mbox->main.posx_s = (crt_w / 2) - (dialogue_width() / 2);
 mbox->main.posx_f = ((crt_w / 2) + (dialogue_width() / 2)) - 1;
 mbox->main.posy_s = (crt_h / 2) - (dialogue_depth() / 2);
 mbox->main.posy_f = ((crt_h / 2) + (dialogue_depth() / 2)) - 1;
}

//==============================================================================
// Create the initial dialogue
//
//   pass: int dialogue                 dialogue ID number
// return: void
//==============================================================================
static void osd_dialogue (int dialogue)
{
 int crt_w;
 int crt_h;

 int mouse_x;
 int mouse_y;
 int i;

 int devices;
 char devices_name[20];

 osd.dialogue = dialogue;
 mbox = &dialogues[dialogue];

 mbox->dialogue = dialogue;
 mbox->button_focus = 0;
 mbox->result = 0;
 mbox->minimised = 0;

 // set box attributes
 if (mbox->attr & MBOX_ATTR_MAXIMISED)
    mbox->main.attr = (mbox->main.attr & ~0x07) | BOX_ATTR_PIXEL0;
 else
    mbox->main.attr = (mbox->main.attr & ~0x07) | BOX_ATTR_PIXEL4;

 mbox->close.attr = BOX_ATTR_PIXEL1;
 mbox->max.attr = BOX_ATTR_PIXEL1;
 mbox->min.attr = BOX_ATTR_PIXEL1;
 mbox->title.attr = BOX_ATTR_PIXEL1;

 mbox->title.text_buf_count = strlen(mbox->title.text);

 for (i = 0; i < mbox->buttons; i++)
    {
     mbox->btn[i].attr = (mbox->btn[i].attr & BOX_ATTR_NOEXIT) |
                          BOX_ATTR_PIXEL1;
     mbox->btn[i].text_buf_count = strlen(mbox->btn[i].text);
     if (i == 0)
        mbox->btn[i].attr |= BOX_ATTR_DASHED;
    }

 switch (dialogue)
    {
     case DIALOGUE_RESET :
        emu.reset = EMU_RST_RESET_CON;
        break;
     case DIALOGUE_POWERCYC :
        emu.reset = EMU_RST_POWERCYC_CON;
        break;
     case DIALOGUE_EXIT :
        emu.quit = 1;
        break;
     case DIALOGUE_OUTPUT :
        devices = console_get_devices();
        console_get_devices_name(devices_name);
        mbox->main.text_buf_put = 0;
        mbox->main.text_buf_start = 0;
        mbox->main.text_buf_count = 0;
        osd_printf("Select output device.\n\n"
                   "Device is currently\nset to '%s'.", devices_name);
        mbox->btn[0].attr &= ~BOX_ATTR_DASHED;
        mbox->button_focus = devices;
        mbox->btn[devices].attr |= BOX_ATTR_DASHED;
        break;
    }

 // set the dialogue width, co-ordinates to the required screen location
 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;
 crt_w_last = crt_w;
 crt_h_last = crt_h;

 // if the dialogue was resized or moved then we need to reset to
 // the default location
 if (mbox->reset)
    {
     mbox->reset = 0;
     if (mbox->dialogue == DIALOGUE_CONSOLE)
        osd_set_dialogue_pos(OSD_POS_UPDATE);
     else
        osd_set_dialogue_pos(OSD_POS_MOUSEORCENTER);
    }
 else
    osd_set_dialogue_pos(OSD_POS_MOUSEORCENTER);

 // draw the dialogue box
 draw_dialogue();
 crtc.update = 1;

 // get the current mouse X, Y values and update dialogue status
 SDL_GetMouseState(&mouse_x, &mouse_y);

 update_dialogue_highlights(mouse_x, mouse_y);

 emu.display_context = EMU_OSD_CONTEXT;

 // make the dialogue the current focus
 emu.osd_focus = 1;
 keyb_set_unicode(emu.osd_focus);

 // show the mouse cursor (needed when a key causes a dailogue)
 if (! mouse.host_in_use)
    SDL_ShowCursor(SDL_ENABLE);
}

//==============================================================================
// Get a pending dialogue.
//
//   pass: void
// return: int                          dialogue or -1 if none pending
//==============================================================================
static int osd_get_pending (void)
{
 int dialogue;

 if (! pending_count)
    return -1;

 dialogue = dialogue_pending[pending_get++];
 if (pending_get >= DIALOGUE_PENDING_SIZE)
    pending_get = 0;
 pending_count--;

 return dialogue;
}

//==============================================================================
// Write a character direct to the console dialogue text buffer.
//
// Writes a character direct to the console dialogue text buffer.  This can
// be written to at any time.  The console dialogue does not need to be
// active. The buffer's X, Y values and buffer contents are updated after
// each call.
//
//   pass: int c                        printable or control character
// return: void
//==============================================================================
void osd_console_putchar (int c)
{
 osd_write_char_to_buffer(&dialogues[DIALOGUE_CONSOLE], c);
}

//==============================================================================
// osd_printf
//
// Output formatted text to the current mbox dialogue box.
//
//   pass: char * fmt,...
// return: int                  value from vsnprintf() function
//==============================================================================
int osd_printf (char * fmt, ...)
{
 int i;
 int ret;
 char buffer[5000+1];
 va_list ap;

 va_start(ap,fmt);
 ret = vsnprintf(buffer, 5000+1, fmt, ap);

 for (i = 0; buffer[i]; i++)
    osd_write_char_to_buffer(mbox, buffer[i]);

 return ret;
}

//==============================================================================
// Show a dialogue or set as pending if one is already active.
//
//   pass: int dialogue
// return: void
//==============================================================================
void osd_set_dialogue (int dialogue)
{
 if ((emu.display_context != EMU_OSD_CONTEXT) && (pending_count == 0))
    {
     osd_dialogue(dialogue);
     return;
    }

 if (pending_count >= DIALOGUE_PENDING_SIZE)
    return;

 dialogue_pending[pending_put++] = dialogue;
 if (pending_put >= DIALOGUE_PENDING_SIZE)
    pending_put = 0;
 pending_count++;
}

//==============================================================================
// Process --osd arguments.
//
//   pass: int arg              argument number (0=all)
//         int pf               prefix used 0='-', 1='+'
// return: void
//==============================================================================
void osd_proc_osd_args (int arg, int pf)
{
 switch (arg)
    {
     case  0 :
        osd.flags = OSD_FLAG_ALL * pf;
        break;
     case  1 :
        osd.flags = (osd.flags & ~OSD_FLAG_ANIMATE) | (OSD_FLAG_ANIMATE * pf);
        break;
    }
}

//==============================================================================
// Redraw the current OSD display if any is active.
//
// If the screen geometry has changed the OSD is re-positioned.
//
//   pass: void
// return: void
//==============================================================================
void osd_redraw (void)
{
 int crt_w;
 int crt_h;

 if (emu.display_context != EMU_OSD_CONTEXT)
    return;

 if (! animating)
    {
     crt_w = crtc.hdisp * 8;
     crt_h = crtc.vdisp * crtc.scans_per_row;
     if ((crt_w != crt_w_last) || (crt_h != crt_h_last))
        {
         crt_w_last = crt_w;
         crt_h_last = crt_h;
         if (mbox->dialogue == DIALOGUE_CONSOLE)
            osd_set_dialogue_pos(OSD_POS_UPDATE);
         else   
            osd_set_dialogue_pos(OSD_POS_MOUSEORCENTER);
        }
     draw_dialogue();
    }
 else
    animate_minimising();
}

//==============================================================================
// Update the OSD where required. This is called after each Z80 code frame
// has completed from the video_update() function.
//
// Sets the crtc.update flag if the OSD animated minimising function needs
// to update the display.
//
//   pass: void
// return: void
//==============================================================================
void osd_update (void)
{
 uint64_t msecs_now;

 if (animating)
    {
     msecs_now = time_get_ms();
     if ((msecs_now - msecs_before) < OSD_ANIMATED_TIME_FRAME)
        return;
     animate_update = 1;
     msecs_before = msecs_now;
     crtc_set_redraw();
     crtc.update = 1;
    }
 else
    // need to update if DIALOGUE_CONSOLE is in context for cursor flashing
    if ((emu.display_context == EMU_OSD_CONTEXT) &&
       (mbox->dialogue == DIALOGUE_CONSOLE) && (! mbox->minimised))
       crtc.update = 1;
}

//==============================================================================
// Configure the initial OSD scheme.
//
// Configures values mainly intended for the console dialogue,  no colours
// are configured here except to move 'widget_xpm_xl' values into strings.
//
//   pass: void
// return: void
//==============================================================================
static void osd_configure_scheme (void)
{
 dialogues[DIALOGUE_CONSOLE].width = -1;
 dialogues[DIALOGUE_CONSOLE].depth = -1;
 dialogues[DIALOGUE_CONSOLE].main.posx_s = -1;
 dialogues[DIALOGUE_CONSOLE].main.posy_s = -1;
 
 // set the cursor flashing rate
 dialogues[DIALOGUE_CONSOLE].main.cursor_rate = osdsch->console_cursor_rate;

 // set the widget's xpm icon colours 
 sprintf(colour_xpm_hl, "  c #%6X", osdsch->widget_xpm_hl);
 sprintf(colour_xpm_ll, "  c #%6X", osdsch->widget_xpm_ll);
}

//==============================================================================
// List OSD schemes.
//
//   pass: void
// return: void
//==============================================================================
void osd_list_schemes (void)
{
 int scheme = 0;
 
 while (osd_scheme_names[scheme][0])
    xprintf("%s\n", osd_scheme_names[scheme++]);
}

//==============================================================================
// Set the OSD cursor rate.
//
//   pass: int rate
// return: void
//==============================================================================
void osd_set_cursor (int rate)
{
 // if no scheme selected then use the default one
 if (osd.scheme == -1)
    osd_set_scheme("default");

 osdsch->console_cursor_rate = rate;
 dialogues[DIALOGUE_CONSOLE].main.cursor_rate = osdsch->console_cursor_rate;
}

//==============================================================================
// Set the OSD console position.
//
// The position of the console may be set using a combination of different
// parameter types, the types supported are:
//
// 1. Emulated character positioning. (n)
// 2. Location, (X = left, right, center) (Y = top, bottom, center)
// 3. Percentage of the emulated CRTC resolution. (n%)
//
// --osd-conpos x,y
//
// Examples:
// --osd-conpos 20,10
// --osd-conpos center,center
// --osd-conpos center,bottom
// --osd-conpos 30%,10%
//
//   pass: char *p              position of console
// return: int                  0 if no error else -1
//==============================================================================
int osd_set_console_position (char *p)
{
 int x;
 int y;
 int pos;

 char *c;
 char sp[512];

 // get the x value
 c = get_next_parameter(p, ',', sp, &x, sizeof(sp)-1);
 if (x == -1)
    {
     pos = string_search(osd_posx_names, sp);
     if (pos == -1)
        return -1;
     switch (pos)
        {
         case 0 :
            x = OSD_CON_CENTER;
            break;
         case 1 :
            x = OSD_CON_LEFT;
            break;
         case 2 :
            x = OSD_CON_RIGHT;
            break;
        }
    }
 else
    if (x < 1 || x > 100)
       return -1;
    else
       if (strstr(sp, "%") == (sp + strlen(sp) - 1))
          x += OSD_CON_PERCENT_000;

 // get the y value
 c = get_next_parameter(c, ',', sp, &y, sizeof(sp)-1);
 if (y == -1)
    {
     pos = string_search(osd_posy_names, sp);
     if (pos == -1)
        return -1;
     switch (pos)
        {
         case 0 :
            y = OSD_CON_CENTER;
            break;
         case 1 :
            y = OSD_CON_TOP;
            break;
         case 2 :
            y = OSD_CON_BOTTOM;
            break;
        }
    }
 else
    if (y < 1 || y > 100)
       return -1;
    else
       if (strstr(sp, "%") == (sp + strlen(sp) - 1))
          y += OSD_CON_PERCENT_000;

 // check that there are no more parameters
 if (c != NULL)   
    return -1;

 // if no scheme selected then use the default one
 if (osd.scheme == -1)
    osd_set_scheme("default");

 // set the new X, Y position values
 osdsch->console_pos_x = x;
 osdsch->console_pos_y = y;

 // force the new values to be updated
 dialogues[DIALOGUE_CONSOLE].width = -1;
 dialogues[DIALOGUE_CONSOLE].depth = -1;
 dialogues[DIALOGUE_CONSOLE].main.posx_s = -1;
 dialogues[DIALOGUE_CONSOLE].main.posy_s = -1;

 // only update the values once running
 if (emu.runmode)
    {
     set_console_sizepos(OSD_POS_UPDATE);
     draw_dialogue();
     crtc.update = 1;
    } 
 
 return 0;
}

//==============================================================================
// Set the OSD console size.
//
// The size of the console may be set using a combination of different
// parameter types, the types supported are:
//
// 1. Emulated character size. (n)
// 2. Maximised (max)
// 3. Percentage of the emulated CRTC resolution. (n%)
//
// --osd-consize x,y
//
// Examples:
// --osd-consize 20%,5
// --osd-consize max,5
// --osd-consize 50%,25%
//
//   pass: char *p              size of console
// return: int                  0 if no error else -1
//==============================================================================
int osd_set_console_size (char *p)
{
 int width;
 int depth;

 char *c;
 char sp[512];

 // get the width value
 c = get_next_parameter(p, ',', sp, &width, sizeof(sp)-1);
 if ((width < 1 || width > 100) && (strcasecmp(sp, "max") != 0))
    return -1;
 if (width == -1)
    width = OSD_CON_MAX;
 else
    if (strstr(sp, "%") == (sp + strlen(sp) - 1))
       width += OSD_CON_PERCENT_000;

 // get the depth value
 c = get_next_parameter(c, ',', sp, &depth, sizeof(sp)-1);
 if ((depth < 1 || depth > 100) && (strcasecmp(sp, "max") != 0))
    return -1;
 if (depth == -1)
    depth = OSD_CON_MAX;
 else
    if (strstr(sp, "%") == (sp + strlen(sp) - 1))
       depth += OSD_CON_PERCENT_000;

 // check that there are no more parameters
 if (c != NULL)   
    return -1;

 // if no scheme selected then use the default one
 if (osd.scheme == -1)
    osd_set_scheme("default");

 // set the new width and depth values
 osdsch->console_width = width;
 osdsch->console_depth = depth;

 // force the new values to be updated
 dialogues[DIALOGUE_CONSOLE].width = -1;
 dialogues[DIALOGUE_CONSOLE].depth = -1;
 dialogues[DIALOGUE_CONSOLE].main.posx_s = -1;
 dialogues[DIALOGUE_CONSOLE].main.posy_s = -1;

 // only update the values once running
 if (emu.runmode)
    {
     set_console_sizepos(OSD_POS_UPDATE);
     draw_dialogue();
     crtc.update = 1;
    } 
 
 return 0;
}

//==============================================================================
// Set the OSD scheme.
//
// Set a new OSD scheme or reset the currently selected scheme to use the
// original console size and positioning values.
//
//   pass: char *p              name of scheme
// return: int                  0 if no error else -1
//==============================================================================
int osd_set_scheme (char *p)
{
 int scheme;

 osd.schemes = sizeof(osdsch_schemes) / sizeof(osdsch_t);
 osd.scheme_user = osd.schemes - 1;

 if ((strcasecmp(p, "reset") == 0) && osdsch)
    {
     // force the new values to be updated
     dialogues[DIALOGUE_CONSOLE].width = -1;
     dialogues[DIALOGUE_CONSOLE].depth = -1;
     dialogues[DIALOGUE_CONSOLE].main.posx_s = -1;
     dialogues[DIALOGUE_CONSOLE].main.posy_s = -1;

     // only update the values once running
     if (emu.runmode)
        {
         set_console_sizepos(OSD_POS_UPDATE);
         draw_dialogue();
         crtc.update = 1;
        } 
     return 0;
    }

 if (strcasecmp(p, "default") == 0)
    scheme = 0;
 else
    if (strcasecmp(p, "user") == 0)
       scheme = osd.scheme_user;
    else
       {
        scheme = string_search(osd_scheme_names, p);
        if (scheme == -1)
           return -1;
       }

 // if a scheme is already in use don't configure new settings as we want
 // the new scheme to inherit the same size and position values
 if (osdsch)
    {
     osd.scheme = scheme;
     osdsch = &osdsch_schemes[scheme];

     // set the widget's xpm icon colours 
     sprintf(colour_xpm_hl, "  c #%6X", osdsch->widget_xpm_hl);
     sprintf(colour_xpm_ll, "  c #%6X", osdsch->widget_xpm_ll);

     return 0;
    }
    
 // set pointer to the scheme requested and configure
 osd.scheme = scheme;
 osdsch = &osdsch_schemes[scheme];
 osd_configure_scheme();
 return 0;
}

//==============================================================================
// Process options for setting colours in the currently selected scheme.
//
// --osd-x c1,c2,c3,c4
//
// Up to 4 arguments will be processed,  the amount to be processed depends on
// the option.  A value of 'x' will cause a value to be left unchanged.
//
// The ordering of c1,c2,c3,c4 and c1,c2 is as follows:
//
// c1,c2,c3,c4 : BGHL, BGLL, FGHL, FGLL
// c1,c2       : BG,   FG
//
// BG=background colour, FG=foreground colour
// HL=high light,        LL=low light
//
//   pass: char *p              parameter
//         int option
// return: int                  0 if no error else -1
//==============================================================================
int osd_set_colour (char *p, int option)
{
 int col1;
 int col2;
 int col3 = -1;
 int col4 = -1;
 int wanted = 0;

 char *c;
 char sp[512];

 osdsch_col_t *osdsch_col;

 // if no scheme selected then use the default one
 if (osd.scheme == -1)
    {
     if (osd_set_scheme("default") == -1)
        return -1;
    }

 switch (option)
    {
     case OPT_OSD_SET_BTN_MAIN :
        osdsch_col = (osdsch_col_t*)&osdsch->button_main_bcol_hl;
        wanted = 4;
        break;
     case OPT_OSD_SET_BTN_TEXT :
        osdsch_col = (osdsch_col_t*)&osdsch->button_text_bcol_hl;
        wanted = 4;
        break;
     case OPT_OSD_SET_DIA_MAIN :
        osdsch_col = (osdsch_col_t*)&osdsch->dialogue_main_bcol;
        wanted = 2;
        break;
     case OPT_OSD_SET_DIA_TEXT :
        osdsch_col = (osdsch_col_t*)&osdsch->dialogue_text_bcol;
        wanted = 2;
        break;
     case OPT_OSD_SET_WID_ICON :
        osdsch_col = (osdsch_col_t*)&osdsch->widget_xpm_hl;
        wanted = 2;
        break;
     case OPT_OSD_SET_WID_MAIN :
        osdsch_col = (osdsch_col_t*)&osdsch->widget_main_bcol_hl;
        wanted = 4;
        break;
     case OPT_OSD_SET_WID_TEXT :
        osdsch_col = (osdsch_col_t*)&osdsch->widget_text_bcol_hl;
        wanted = 4;
        break;
     default :
        return -1;   
    }
    
 // get the 1st colour value
 c = get_next_parameter(p, ',', sp, &col1, sizeof(sp)-1);
 if (((col1 < 0) || (col1 > 0xffffff)) && (strcasecmp(sp, "x") != 0))
    return -1;

 // get the 2nd colour value
 c = get_next_parameter(c, ',', sp, &col2, sizeof(sp)-1);
 if (((col2 < 0) || (col2 > 0xffffff)) && (strcasecmp(sp, "x") != 0))
    return -1;

 if (wanted >= 3)
    {
     // get the 3rd colour value
     c = get_next_parameter(c, ',', sp, &col3, sizeof(sp)-1);
     if (((col3 < 0) || (col3 > 0xffffff)) && (strcasecmp(sp, "x") != 0))
        return -1;
    }
 else
    col3 = -1;   

 if (wanted == 4)
    {
     // get the 4th colour value
     c = get_next_parameter(c, ',', sp, &col4, sizeof(sp)-1);
     if (((col4 < 0) || (col4 > 0xffffff)) && (strcasecmp(sp, "x") != 0))
        return -1;
    }
 else
    col4 = -1;

 // check that there are no more parameters
 if (c != NULL)   
    return -1;

 // set the colours, a -1 value will skip
 if (col1 != -1)
    osdsch_col->col1 = col1;
 if (col2 != -1)
    osdsch_col->col2 = col2;
 if (col3 != -1)
    osdsch_col->col3 = col3;
 if (col4 != -1)
    osdsch_col->col4 = col4;

 // set the widget's xpm icon colours   
 if (option != OPT_OSD_SET_WID_ICON)
    return 0;

 sprintf(colour_xpm_hl, "  c #%6X", osdsch->widget_xpm_hl);
 sprintf(colour_xpm_ll, "  c #%6X", osdsch->widget_xpm_ll);

 return 0;
}
