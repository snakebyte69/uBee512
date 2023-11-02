//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                                 GUI module                                 *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Provides the graphical interface.
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
// v5.5.0 - 21 June 2013, B.Robinson
// - Updated title bar to show debug mode - running, tracing, stopped etc...
//
// v5.3.0 - 2 April 2011, uBee
// - Added tapfile to GUI tape updates as OR values with the tape status.
//
// v5.0.0 - 13 July 2010, K Duckmanton
// - Removed all references to the 'sound' global variable and replaced them
//   with references to the 'audio' global instead.
//
// v4.7.0 - 17 June 2010, uBee
// - When using SDL-1.2.14 and the mouse cursor is disabled in full screen
//   mode it causes spurious mouse motion events.  This is evident under
//   win32 with the cursor reappearing at the center of display before it
//   times out for a second time.  The fix will be used for all SDL versions
//   and has fixed some other problems too, one being starting up in full
//   screen mode now has the cursor disabled.  Changes were made to the
//   gui_mousemotion_event() and gui_update() functions.
// - Fixed mouse_cursor_time variable, was int is now uint64_t. (workerbee)
//
// v4.6.0 - 21 April 2010, uBee
// - Added gui_proc_status_args() function to process --status arguments.
//
// v4.2.0 - 19 July 2009, uBee
// - Added Microbee mouse emulated status 'm'.
// - Changed middle mouse button to switch Microbee mouse mode on.
//
// v4.0.0 - 7 June 2009, uBee
// - Added gui_changed_videostate() function which is called when toggling
//   between full and window screens. (see notes in function)
// - Double left button mouse click no longer toggles full screen mode when
//   the OSD has the focus.
//
// v3.1.0 - 17 March 2009, uBee
// - Fixed gui_status_update() for printer status by changing
//   gui_status.tape to gui_status.print
// - Added code to hide the mouse cursor in full screen mode if not moved
//   for a period of time.  Made visible whenever the mouse is moved.
// - Changed middle and right mouse down buttons to call osd_set_dialogue()
//   function that provides a menu of different options including reset and
//   exit sub dialogues.
// - Added gui_mousemotion_event() handler function.
// - Moved mouse button state variables into gui_t structure.
// - Removed gui_ask_exit_emulator() and gui_ask_reset_emulator() functions.
// - Removed mbox() code, gui_message_box() is only called in Windows when
//   output has gone to a console and needs to be read before exiting.
// - Reset and Exit confirmation messages now use the osd_set_dialogue() On
//   Screen Display (OSD) function and no longer needs to switch screen size.
// - Changed function_*() functions to keyb_*() as code was moved.
// - Changed Window size status to only show if using OpenGL video mode.
// - Added OpenGL conditional code compilation using USE_OPENGL.
//
// v3.0.0 - 13 October 2008, uBee
// - Changes to gui_message_box() to test is full screen mode is active.
// - Simplified the timer by introducing a time_get_ms() function.
// - Added GUI_MOUSE_WHEEL_WIN scroll wheel functions.
// - Increased status array from 200 to 300 now that OpenGL can make maximize.
// - Removed code directly associated with video and created a new video.c
//   module containing modified code.
//
// v2.8.0 - 2 Sepetember 2008, uBee
// - Added SDL_BUTTON_WHEELUP and SDL_BUTTON_WHEELDOWN events.
// - Added display of '[DEBUG]' to status line when emulator is debugging.
// - Added volume level display persist to GUI status line.
// - Improved GUI status line code so new persist values can be easily added
//   and changed persist timers to use system clock.
// - Added display of '[PAUSED]' to status line when emulator is in the
//   paused state.
// - SDL video rendering has seen some major changes, SW and HW surfaces are
//   now supported along with 8, 16 or 32 bits per pixel modes.
// - Added 'default : b = MB_OKCANCEL' to gui_message_box() function to
//   eliminate a compiler warning of b not assigned.
//
// v2.7.0 - 20 June 2008, uBee
// - Doubled the default VIDHEIGHT value from 275 to 550.
// - Added sound mute status 'M' when in muted state.
// - Added gui_update() GUI function to handle all GUI updates. The
//   crtc.blitted flag greatly reduces the host CPU time and may improve
//   sound quality.
// - Added structure emu_t and moved variables into it.
// - Added SDL_FreeSurface(screen) before creating a new surface in
// - gui_videochange() is now gui_create_surface() and added a
//   SDL_FreeSurface() call before creating a new surface.
//
// v2.6.0 - 12 May 2008, uBee
// - Status line is now configurable with many new properties.
// - Left mouse button now requires a double click to toggle full screen mode.
// - Grab mode if used is disabled before showing any windows then restored.
//
// v2.4.0 - 19 February 2008, uBee
// - Added a status line functions for the title bar.
// - Implement the modelx information structure.
//
// v2.2.0 - 30 November 2007, uBee
// - Mouse button click causing mbox to be brought up and a button is then
//   pressed/released caused unwanted action.  Now records the down button
//   state to be tested on release.
//
// v2.1.0 - 29 October 2007, uBee
// - Added gui_mousebuttondown_event() and gui_mousebuttonup_event() functions.
//   Left mouse button click toggles fullscreen mode.  Middle button is reset,
//   and right button for exit.
// - Added gui_ask_exit_emulator() and gui_ask_reset_emulator() functions.
//
// v2.0.0 - 10 October 2007, uBee
// - Added message box for Unices builds.
// - Fullscreen switched to a window if message box is displayed then switches
//   back to fullsceen.
//
// v1.4.0 - 29 September 2007, uBee
// - Changes to Y scaling to achieve better aspect ratio.
// - Added changes to error reporting.
//
// v1.0.0 - 21 June 2007, uBee
// - Added fullscreen toggle mode (ALT+ENTER), special code for win32 is used.
// - Added screen resizing capabilities.
//
// v0.0.0 - 5 June 2007, uBee
// Start with "nanowasp" source distribution version 0.22. An emulator for the
// microbee 128k. Copyright (C) 2000-2003  David G. Churchill.
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
#include "audio.h"
#include "z80debug.h"
#include "keyb.h"
#include "video.h"
#include "gui.h"
#include "mouse.h"
#include "support.h"
#include "osd.h"
#include "tape.h"
#include "tapfile.h"
#include "joystick.h"
#include "async.h"
#include "serial.h"
#include "printer.h"

//==============================================================================
// structures and variables
//==============================================================================
gui_t gui =
{
 .dclick_time=300,
 .mouse_wheel=GUI_MOUSE_WHEEL_VOL,
 .persist_time=GUI_PERSIST_TIME
};

gui_status_t gui_status =
{
 .left=0,
 .emuver=1,
 .emu=0,
 .ver=0,
 .model=1,
 .mouse=1,
 .mute=1,
 .title=0,
 .ram=0,
 .speed=1,
 .serial=1,
 .print=1,
 .tape=1,
 .joy=1,
 .longdrive=0,
 .shortdrive=1
};

static int mouse_motion_ignore;

static char padding[50];

static int drive;
static int drive_spinner_pos;

static uint64_t button_l_dclick;
static uint64_t mouse_cursor_time;

extern deschand_t coms1;
extern char *model_args[];

extern emu_t emu;
extern model_t modelx;
extern model_custom_t modelc;
extern debug_t debug;
extern joystick_t joystick;
extern tape_t tape;
extern tapfile_t tapfile;
extern serial_t serial;
extern audio_t audio;
extern printer_t printer;
extern video_t video;
extern mouse_t mouse;

//==============================================================================
// GUI initialise.
//
// This creates a default video mode of 640 x 275 (80x25) which is suitable for
// CP/M operating systems.  The video mode will be able to be changed later when
// the CRTC registers are changed,  typically this will occur when switching
// between CP/M and Microbee's 64x16 BASIC screen.
//
//   pass: void
// return: int                  0 if no error, -1 if error
//==============================================================================
int gui_init (void)
{
 mouse_cursor_time = time_get_ms() + GUI_CURSOR_TIME;
 gui_status_padding(GUI_SPADDING);

 if (video.flags & SDL_FULLSCREEN)
    {
     mouse_motion_ignore = 5;
     SDL_ShowCursor(SDL_DISABLE);
     mouse_cursor_time = 0;
    }

 return 0;
}

//==============================================================================
// GUI de-initialise.
//
//   pass: void
// return: int                  0
//==============================================================================
int gui_deinit (void)
{
 return 0;
}

//==============================================================================
// GUI reset.
//
//   pass: void
// return: int                  0
//==============================================================================
int gui_reset (void)
{
 if (video.flags & SDL_FULLSCREEN)
    {
     mouse_motion_ignore = 5;
     SDL_ShowCursor(SDL_DISABLE);
     mouse_cursor_time = 0;
    }

 return 0;
}

//==============================================================================
// Message Box.
//
// This will only be called in Windows programs so that the extra console
// window will not be closed before being read.
//
//   pass: int buttons                  button combination
//         char *s                      message
// return: int                          return value
//==============================================================================
int gui_message_box (int buttons, char *s)
{
 SDL_GrabMode grab_mode = SDL_WM_GrabInput(SDL_GRAB_QUERY);
 if (grab_mode == SDL_GRAB_ON)
    SDL_WM_GrabInput(SDL_GRAB_OFF);

 int fs = video.flags & SDL_FULLSCREEN;
 if (fs)
    video_toggledisplay();

#ifdef MINGW
 unsigned int b;
 int res;

 switch (buttons)
    {
     case BUTTON_OK :
        b = MB_OK;
        break;
     case BUTTON_OKCANCEL :
        b = MB_OKCANCEL;
        break;
     default :
        b = MB_OKCANCEL;
        break;
    }
 res = MessageBox(NULL, s, ICONSTRING, b | MB_ICONWARNING);

 if (fs)
    video_toggledisplay();

 switch (res)
    {
     case IDOK :
        return BUTTON_IDOK;
     case IDCANCEL :
        return BUTTON_IDCANCEL;
     default :
        return 0;
    }
#else
    return BUTTON_IDOK;
#endif
}

//==============================================================================
// GUI status line padding
//
//   pass: int n
// return: int                          0 if no error, else -1
//==============================================================================
int gui_status_padding (int n)
{
 if (n >= sizeof(padding))
    return -1;

 memset(&padding, ' ', n);
 padding[n] = 0;

 return 0;
}

//==============================================================================
// GUI emulator status line update
//
//   pass: void
// return: void
//==============================================================================
void gui_status_update (void)
{
 static char drive_spinner[5] = {"|/-\\"};

 int displayed = 0;
 int i;

 char status[300] = {""};
 char vstates[20] = {""};
 char convert[20];

 if (gui_status.emuver)
    {
     strcat(status, ICONSTRING"-"APPVER);
     displayed++;
    }

 if (gui_status.emu)
    {
     if (displayed)
        strcat(status, padding);
     displayed++;
     strcat(status, ICONSTRING);
    }

 if (gui_status.ver)
    {
     if (displayed)
        strcat(status, padding);
     displayed++;
     strcat(status, APPVER);
    }

 if (gui_status.title)
    {
     if (displayed)
        strcat(status, padding);
     displayed++;
     strcat(status, gui.title);
    }

 if (gui_status.sys)
    {
     if (displayed)
        strcat(status, padding);
     displayed++;
     strcat(status, modelc.systname);
    }

 if (gui_status.model)
    {
     if (displayed)
        strcat(status, padding);
     displayed++;
     toupper_string(convert, model_args[emu.model]);
     strcat(status, convert);
    }

 if (emu.paused)
    {
     if (displayed)
        strcat(status, padding);
     displayed++;
     strcat(status, "[PAUSED]");
    }
 else
    {
     if (debug.mode != Z80DEBUG_MODE_OFF)
        {
         if (displayed)
            strcat(status, padding);
         displayed++;

         switch (debug.mode)
            {
             case Z80DEBUG_MODE_RUN :
                strcat(status, "[RUNNING]");
                break;

             case Z80DEBUG_MODE_TRACE :
                strcat(status, "[TRACING]");
                break;

             case Z80DEBUG_MODE_STOP :
                strcat(status, "[STOPPED]");
                break;

             case Z80DEBUG_MODE_STEP_QUIET:
             case Z80DEBUG_MODE_STEP_VERBOSE :
                strcat(status, "[STEP]");
                break;
            }
        }

     if (gui_status.ram)
        {
         if (displayed)
            strcat(status, padding);
         displayed++;
         snprintf(convert, sizeof(convert)-1, "%dK", modelx.ram);
         strcat(status, convert);
        }

     if (gui_status.speed)
        {
         if (displayed)
            strcat(status, padding);
         displayed++;
         snprintf(convert, sizeof(convert)-1, "%.3fMHz", (float)emu.cpuclock / 1000000.0);
         strcat(status, convert);
        }

     if ((gui_status.serial) && (coms1 != (deschand_t)-1))
        {
         if (displayed)
            strcat(status, padding);
         displayed++;
         snprintf(convert, sizeof(convert)-1, "%dN%d:%d", serial.databits, serial.stopbits, serial.tx_baud);
         strcat(status, convert);
        }

     if (gui_status.mute && audio.mute)
        {
         strcat(vstates, "[M");
         displayed++;
        }

     if (gui_status.mouse && mouse.active)
        {
         if (vstates[0])
            strcat(vstates, ":m");
         else
            strcat(vstates, "[m");
         displayed++;
        }

     if (gui_status.print && (printer.print_a_file || printer.print_b_file))
        {
         if (vstates[0])
            strcat(vstates, ":P");
         else
            strcat(vstates, "[P");
         displayed++;
        }

     if (gui_status.tape && (tape.in_status | tapfile.in_status))
        {
         if (vstates[0])
            strcat(vstates, ":Ti");
         else
            strcat(vstates, "[Ti");
         displayed++;
        }

     if (gui_status.tape && (tape.tape_o_file || tapfile.tape_o_file))
        {
         if (vstates[0])
            strcat(vstates, ":To");
         else
            strcat(vstates, "[To");
         displayed++;
        }

     if (gui_status.joy && joystick.joy)
        {
         if (joystick.mbee)
            {
             if (vstates[0])
                strcat(vstates, ":JS");
             else
                strcat(vstates, "[JS");
            }
         if (joystick.kbd)
            {
             if (vstates[0])
                strcat(vstates, ":J");
             else
                strcat(vstates, "[J");
             snprintf(convert, sizeof(convert)-1, "%d", joystick.set);
             strcat(vstates, convert);
            }
         displayed++;
        }

     if (vstates[0])
        {
         strcat(vstates, "]");
         if (displayed)
            strcat(status, padding);
         strcat(status, vstates);
        }

     if ((gui_status.vol) || (gui.persist_flags & GUI_PERSIST_VOL))
        {
         if (displayed)
            strcat(status, padding);
         displayed++;
         snprintf(convert, sizeof(convert)-1, "[vol %d%%]", audio.vol_percent);
         strcat(status, convert);
        }

#ifdef USE_OPENGL
     if (((gui_status.win) && (video.type == VIDEO_GL)) || (gui.persist_flags & GUI_PERSIST_WIN))
        {
         if (displayed)
            strcat(status, padding);
         displayed++;
         snprintf(convert, sizeof(convert)-1, "[win %d%%]", video.percent_size);
         strcat(status, convert);
        }
#endif

     if ((gui_status.shortdrive || gui_status.longdrive) && (gui.persist_flags & GUI_PERSIST_DRIVE))
        {
         if (displayed)
            strcat(status, padding);
         displayed++;
         if (gui_status.shortdrive)
            snprintf(convert, sizeof(convert)-1, "%c: %c", drive, drive_spinner[drive_spinner_pos]);
         else
            snprintf(convert, sizeof(convert)-1, "Drive %c: %c", drive, drive_spinner[drive_spinner_pos]);
         strcat(status, convert);
        }
    }

 if (gui_status.left)
    {
     i = strlen(status);
     memset(&status[i], ' ', sizeof(status)-i);
     status[sizeof(status)-1] = 0;
    }

 SDL_WM_SetCaption(status, ICONSTRING);
}

//==============================================================================
// GUI emulator status line persist value set.  Only one persist value per
// call is allowed.  The GUI status line is also updated.
//
//   pass: int n                        flag value of the persist value to set
//         int p                        parameter associated with persist value
// return: void
//==============================================================================
void gui_status_set_persist (int f, int p)
{
 uint64_t ticks;

 ticks = time_get_ms();
 gui.persist_flags |= f;

 switch (f)
    {
     case GUI_PERSIST_DRIVE :
        drive = p;
        drive_spinner_pos = (drive_spinner_pos + 1) & 0x03;
        gui.drive_persist_timer = ticks + gui.persist_time;
        break;
     case GUI_PERSIST_VOL :
        gui.volume_persist_timer = ticks + gui.persist_time;
        break;
     case GUI_PERSIST_WIN :
        gui.window_persist_timer = ticks + gui.persist_time;
        break;
    }

 gui_status_update();
}

//==============================================================================
// Mouse button down event.
//
//   pass: void
// return: void
//==============================================================================
void gui_mousebuttondown_event (void)
{
 // show the mouse cursor
 SDL_ShowCursor(SDL_ENABLE);
 mouse_cursor_time = time_get_ms() + GUI_CURSOR_TIME;

 switch (emu.event.button.button)
    {
     case SDL_BUTTON_LEFT :
        gui.button_l = 1;
        osd_set_focus();
        break;
     case SDL_BUTTON_MIDDLE :
        mouse_configure(MOUSE_ON);
        break;
     case SDL_BUTTON_RIGHT :
        gui.button_r = 1;
        if (emu.display_context == EMU_EMU_CONTEXT)
           {
            osd_set_dialogue(DIALOGUE_MENU);
            return;
           }
        else
           {
            osd_dialogue_exit();
            return;
           }
        break;
     case SDL_BUTTON_WHEELUP :
        gui.button_wu = 1;
        break;
     case SDL_BUTTON_WHEELDOWN :
        gui.button_wd = 1;
        break;
    }
 if ((emu.display_context == EMU_OSD_CONTEXT) && (emu.osd_focus))
    osd_mousebuttondown_event();
}

//==============================================================================
// Mouse button up event.
//
// Left mouse button double click toggles full screen mode.
// Middle mouse button resets emulator.
// Right mouse button exit emulator.
// Wheel up increases application volume.
// Wheel down decreases application volume.
//
//   pass: void
// return: void
//==============================================================================
void gui_mousebuttonup_event (void)
{
 uint64_t ticks;

 switch (emu.event.button.button)
    {
     case SDL_BUTTON_LEFT :
        if (gui.button_l)
           {
            ticks = time_get_ms();

            if (button_l_dclick == 0)
               button_l_dclick = ticks;
            else
               {
                if ((ticks - button_l_dclick) <= gui.dclick_time)
                   {
                    if ((emu.display_context != EMU_OSD_CONTEXT) || (! emu.osd_focus))
                       video_toggledisplay();
                    button_l_dclick = 0;
                   }
                else
                   button_l_dclick = ticks;
               }
            gui.button_l = 0;
           }
        break;
     case SDL_BUTTON_MIDDLE :
        if (gui.button_m)
           gui.button_m = 0;
        break;
     case SDL_BUTTON_RIGHT :
        if (gui.button_r)
           gui.button_r = 0;
        break;
     case SDL_BUTTON_WHEELUP :
        gui.button_wu = 0;
        switch (gui.mouse_wheel)
           {
            case GUI_MOUSE_WHEEL_NONE :
               break;
            case GUI_MOUSE_WHEEL_VOL :
               keyb_emu_command(EMU_CMD_VOLUMEI, 0);
               keyb_repeat_stop();
               break;
            case GUI_MOUSE_WHEEL_WIN :
               keyb_emu_command(EMU_CMD_SCREENI, 0);
               keyb_repeat_stop();
               break;
           }
        break;
     case SDL_BUTTON_WHEELDOWN :
        gui.button_wd = 0;
        switch (gui.mouse_wheel)
           {
            case GUI_MOUSE_WHEEL_NONE :
               break;
            case GUI_MOUSE_WHEEL_VOL :
               keyb_emu_command(EMU_CMD_VOLUMED, 0);
               keyb_repeat_stop();
               break;
            case GUI_MOUSE_WHEEL_WIN :
               keyb_emu_command(EMU_CMD_SCREEND, 0);
               keyb_repeat_stop();
               break;
           }
        break;
    }
 if ((emu.display_context == EMU_OSD_CONTEXT) && (emu.osd_focus))
    osd_mousebuttonup_event();
}

//==============================================================================
// Mouse motion event.
//
// When using SDL-1.2.14 and the mouse cursor is disabled in full screen
// mode it causes spurious mouse motion events.  This is evident under win32
// with the cursor reappearing at the center of display before it times out
// for a second time.  We get around the problem by ignoring a handful of
// mouse motion events.  A counter gets set with a value whenever the mouse
// cursor is disabled.  A smaller value is used for non-win32 systems.
//
//   pass: void
// return: void
//==============================================================================
void gui_mousemotion_event (void)
{
 if (emu.display_context == EMU_OSD_CONTEXT)
    {
     SDL_ShowCursor(SDL_ENABLE);
     osd_mousemotion_event();
     return;
    }

 if (mouse_motion_ignore)
    {
     mouse_motion_ignore--;
     return;
    }

 if (video.flags & SDL_FULLSCREEN)
    {
#ifdef MINGW
     mouse_motion_ignore = 15; // more of an issue here
#else
     mouse_motion_ignore = 3;  // not an issue here but use a small value anyway
#endif
     SDL_ShowCursor(SDL_ENABLE);
     mouse_cursor_time = time_get_ms() + GUI_CURSOR_TIME;
    }
}

//==============================================================================
// GUI update. This is called after each Z80 code frame has completed.
//
// Hide the mouse cursor in full screen mode if mouse not moved for a
// specified time period.
//
// Updates GUI status line for persist values but only when the timer values
// expire and if any of these are currently active.
//
//   pass: void
// return: void
//==============================================================================
void gui_update (void)
{
 uint64_t ticks = time_get_ms();

 if ((! mouse.host_in_use) && (video.flags & SDL_FULLSCREEN) &&
 (emu.display_context != EMU_OSD_CONTEXT) && (ticks > mouse_cursor_time))
    {
     SDL_ShowCursor(SDL_DISABLE);  // disable the mouse cursor
     mouse_cursor_time = ticks + 1000; // reduces the cursor disable frequency
    }

 if (gui.persist_flags)
    {
     if ((gui.persist_flags & GUI_PERSIST_DRIVE) && (ticks >= gui.drive_persist_timer))
        {
         gui.persist_flags ^= GUI_PERSIST_DRIVE;
         gui_status_update();
        }
     if ((gui.persist_flags & GUI_PERSIST_VOL) && (ticks >= gui.volume_persist_timer))
        {
         gui.persist_flags ^= GUI_PERSIST_VOL;
         gui_status_update();
        }
     if ((gui.persist_flags & GUI_PERSIST_WIN) && (ticks >= gui.window_persist_timer))
        {
         gui.persist_flags ^= GUI_PERSIST_WIN;
         gui_status_update();
        }
    }
}

//==============================================================================
// This should be called when switching between a full screen and a window
// display. The state of SDL seems to get confused (reset) for keys and the
// mouse buttons.  i.e. keys and buttons are set to the released state.
//
//   pass: void
// return: void
//==============================================================================
void gui_changed_videostate (void)
{
 gui.button_l = 0;
 gui.button_m = 0;
 gui.button_r = 0;
 gui.button_wu = 0;
 gui.button_wd = 0;
 button_l_dclick = 0;
}

//==============================================================================
// Process --status arguments.
//
// Note: The table of value pointers does not contain an entry for 'all'.
// The first entry is therefore arg=1.
//
//   pass: int arg              argument number (0=all)
//         int pf               prefix used 0='-', 1='+'
// return: void
//==============================================================================
void gui_proc_status_args (int arg, int pf)
{
 int *status_values[] =
 {
  &gui_status.shortdrive,
  &gui_status.longdrive,
  &gui_status.emu,
  &gui_status.emuver,
  &gui_status.joy,
  &gui_status.left,
  &gui_status.model,
  &gui_status.mouse,
  &gui_status.mute,
  &gui_status.print,
  &gui_status.ram,
  &gui_status.speed,
  &gui_status.serial,
  &gui_status.sys,
  &gui_status.tape,
  &gui_status.title,
  &gui_status.ver,
  &gui_status.vol,
  &gui_status.win,
  NULL
 };

 if (arg)
    // one value
    *status_values[arg-1] = pf;
 else
    {
     // all values
     while (status_values[arg])
        *status_values[arg++] = pf;
    }
}

//==============================================================================
// GUI commands
//
//   pass: int cmd                      GUI command
// return: void
//==============================================================================
void gui_command (int cmd)
{
 switch (cmd)
    {
     case EMU_CMD_MWHEEL :
        gui.mouse_wheel++;
        if (gui.mouse_wheel > GUI_MOUSE_WHEEL_WIN)
           gui.mouse_wheel = 1;
        break;
    }
}
