//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              console module                                *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Provides output stream redirection and an interface for entering options
// during the running of the emulator using an the stdout interface.
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
// - Added supporting functions console_exit_while_debugger_runs and
//   console_resume_after_debugger_run to support new debugger
//   functionality.
// v5.5.0 - 26 August 2012, uBee
// - Disabled xgetch() function from compilation as currently not used and
//   general code tidy up.
//
// v5.4.0 - 14 May 2012, uBee
// - Fixed a potential problem in xprintf() concerning XPRINT_BUFSIZE and
//   NULL termination.
//
// v4.7.0 - 29 June 2010, uBee
// - Changes made to fgets() function to use the result as some compilers
//   report warning: declared with attribute warn_unused_result.
//
// v4.6.0 - 2 May 2010, uBee
// - Added and console_debug_stream() function.
// - Added code to xputchar(), xprintf() and xfflush() functions to support
//   a console debug stream device.
// - Added console_proc_output_args() function to process --output arguments.
//
// v3.1.0 - 7 February 2009, uBee
// - Created a new file for the console module.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <SDL.h>

#ifdef MINGW
#include <windows.h>
#include <conio.h>
#else
#include <fcntl.h>      // setting keyboard flags
#include <sys/ioctl.h>
#include <termios.h>
#include <X11/Xlib.h>
#endif

#include "ubee512.h"
#include "console.h"
#include "options.h"
#include "crtc.h"
#include "video.h"
#include "gui.h"
#include "osd.h"
#include "z80debug.h"

//==============================================================================
// structures and variables
//==============================================================================
console_t console =
{
#ifdef MINGW
 .streams = CONSOLE_OSD,
#else
 .streams = CONSOLE_OSD | CONSOLE_STDOUT,
#endif
 .xstdin = 1,
 .end_by_debugger = 0,
 .resume_by_debugger = 0,
};

char *device_names[] =
{
 "none",
 "OSD",
 "stdout",
 "both"
};

static int console_mode_active;

#ifdef MINGW
static int consoleoutput;
#else
// two global variables for tty and keyboard control
static struct TERMIO_S term_orig;
static int kbdflgs;
#endif

extern char *c_argv[];
extern int c_argc;

extern char userhome[];
extern emu_t emu;

extern video_t video;
extern osd_t osd;

//==============================================================================
// Console initialise.
//
//   pass: void
// return: int                  0 if no error, -1 if error
//==============================================================================
int console_init (void)
{
 return 0;
}

//==============================================================================
// Console de-initialise.
//
//   pass: void
// return: int                  0
//==============================================================================
int console_deinit (void)
{
 return 0;
}

//==============================================================================
// Console reset.
//
//   pass: void
// return: int                  0
//==============================================================================
int console_reset (void)
{
 return 0;
}

//==============================================================================
// Console input/output.
//
// Windows:
// Turns on console input/output so that messages can be seen on a text
// console window.  If this is not called then console out will be
// redirected to stdout.txt and stderr.txt files This is set in the SDL main
// section.
//
// Unices:
// Does nothing.
//
//   pass: void
// return: void
//==============================================================================
static void console_output (void)
{
#ifdef MINGW
 char save_dir[5000];

 if (! consoleoutput)
    {
     consoleoutput = 1;

     getcwd(save_dir, sizeof(save_dir));
     chdir(userhome);
     AllocConsole();
     chdir(save_dir);

     freopen("conin$", "r", stdin);
     freopen("conout$", "w", stdout);
     freopen("conout$", "w", stderr);
    }
#else
#endif
}

//==============================================================================
// xputchar
//
// Output character to destinations determined by console.streams
//
//   pass: int c                        character
// return: int                          same as for putchar()
//==============================================================================
int xputchar (int c)
{
 int res = 0;

 if ((console.streams & CONSOLE_DEBUG) && console.debug)
    {
     fputc(c, console.debug);
     res = c;
     if (console.debug_only)
        return res;
    }

 if ((console.streams & CONSOLE_STDOUT) || (! emu.runmode) ||
 console.console_stdout || console.force_stdout)
    {
     console_output();
     res = putchar(c);
    }

 if ((console.streams & CONSOLE_OSD) && (osd.initialised) &&
 (! console.console_stdout))
    {
     osd_console_putchar(c);
     res = c;
    }

 return res;
}

//==============================================================================
// xprintf
//
// Output formatted text to destinations determined by console.streams
//
//   pass: char * fmt,...
// return: int                  value from vsnprintf() function
//==============================================================================
int xprintf (char * fmt, ...)
{
 int i;
 int ret;
 char buffer[XPRINT_BUFSIZE];
 va_list ap;

 va_start(ap,fmt);
 ret = vsnprintf(buffer, XPRINT_BUFSIZE-1, fmt, ap);
 buffer[XPRINT_BUFSIZE-1] = 0;

 // if writing debug commands to a file
 if ((console.streams & CONSOLE_DEBUG) && console.debug)
    {
     for (i = 0; buffer[i]; i++)
        fputc(buffer[i], console.debug);
     if (console.debug_only)
        return ret;
    }

 if ((console.streams & CONSOLE_STDOUT) || (! emu.runmode) ||
 console.console_stdout || console.force_stdout)
    {
     console_output();
     for (i = 0; buffer[i]; i++)
        putchar(buffer[i]);
    }

 // if the console mode is active we don't want output going to the OSD
 if (console_mode_active)
    return ret;

 if ((console.streams & CONSOLE_OSD) && (osd.initialised) &&
 (! console.console_stdout))
    {
     for (i = 0; buffer[i]; i++)
        osd_console_putchar(buffer[i]);
    }

 return ret;
}

//==============================================================================
// xflush
//
// flush output data destinations determined by console.streams
//
//   pass: void
// return: void
//==============================================================================
void xflush (void)
{
 if (console.streams & CONSOLE_DEBUG)
    fflush(console.debug);
 if (console.streams & CONSOLE_STDOUT)
    fflush(stdout);
 if (console.streams & CONSOLE_OSD)
    osd_redraw();
}

#ifndef MINGW
//==============================================================================
// getch() function for statndard Unix like systems.
//
// This code was extracted from some example found here:
// http://c-faq.com/osdep/kbhit.txt
//
//   pass: void
// return: int                  1 if key pressed, else 0
//==============================================================================

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// function : system_mode
// purpose  : reset the system to what it was before input_mode was
//            called
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static void system_mode (void)
{
 if (ioctl(0, TCSETA, &term_orig) == -1)
    return;
 fcntl(0, F_SETFL, kbdflgs);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// function : input_mode
// purpose  : set the system into raw mode for keyboard i/o
// returns  : 0 - error
//            1 - no error
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static int input_mode (void)
{
 struct TERMIO_S term;    // to avoid ^S ^Q processing

 // get rid of XON/XOFF handling, echo, and other input processing
 if (ioctl(0, TCGETA, &term) == -1)
    return (0);

 (void) ioctl(0, TCGETA, &term_orig);
 term.c_iflag = 0;
 term.c_oflag = 0;
 term.c_lflag = 0;
 term.c_cc[VMIN] = 1;
 term.c_cc[VTIME] = 0;
 if (ioctl(0, TCSETA, &term) == -1)
    return (0);
 kbdflgs = fcntl(0, F_GETFL, 0);

 // no delay on reading stdin
 int flags = fcntl(0, F_GETFL);
 flags &= ~O_NDELAY;

 fcntl(0, F_SETFL, flags);
 return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// function : getch
// purpose  : read a single character from the keyboard without echo
// returns  : the keypress character
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int getch (void)
{
 // no delays on reading stdin
 input_mode();

 // do a simple loop and get the response
 unsigned char ch;
 while (read(0, &ch, 1) != 1)
    ;

 system_mode();
 return ch;
}
#endif

#if 0
//==============================================================================
// xgetch
//
// Get a key from STDIN (getch) or SDL depending on the current key_device
// source setting. This call blocks until a key is pressed.
//
// Unicode mode is assumed to be already enabled when requesting an SDL key.
//
//   pass: void
// return: int                          key pressed
//==============================================================================
int xgetch (void)
{
 int ch = 0;

 switch (console.key_device)
    {
     case 0 :
        ch = getch();
        break;
     case 1 :
        while (ch == 0)
           {
            while (SDL_PollEvent(&emu.event))
               {
                switch (emu.event.type)
                   {
                    case SDL_KEYDOWN:
                       ch = emu.event.key.keysym.unicode & 0x7F;
                       break;
                   }
               }
           }
        break;
    }
 return ch;
}
#endif

//==============================================================================
// Set console key device.
//
//   pass: int d                        input device
// return: void
//==============================================================================
void console_set_keydevice (int d)
{
 console.key_device = d;
}

//==============================================================================
// Set console stream devices
//
//   pass: int d                        output devices
// return: void
//==============================================================================
void console_set_devices (int d)
{
 console.streams = d;
}

//==============================================================================
// Add a console stream device
//
//   pass: int d                        output device
// return: void
//==============================================================================
void console_add_device (int d)
{
 console.streams |= d;
}

//==============================================================================
// Get console stream devices
//
//   pass: void
// return: int                          device bits
//==============================================================================
int console_get_devices (void)
{
 return console.streams;
}

//==============================================================================
// Get console stream devices name
//
//   pass: char *devices                returns a device name
// return: void
//==============================================================================
void console_get_devices_name (char *devices)
{
 strcpy(devices, device_names[console.streams]);
}

//==============================================================================
// Console debugging message output and wait
//
//   pass: char *mesg
// return: void
//==============================================================================
void console_debug_message (char *mesg)
{
 int streams;

 streams = console.streams;
 console_set_devices(console.streams | CONSOLE_STDOUT);

 xprintf("Debugging: '%s' -- press a key (in this window) to continue\n", mesg);
 xflush();
 getch();

 console.streams = streams;
}

//==============================================================================
// Console exit while debugger runs
//
//   pass: void
// return: void
//==============================================================================
void console_exit_while_debugger_runs (void)
{
 console.end_by_debugger = 1;
}

//==============================================================================
// Console resume after debugger run
//
//   pass: void
// return: void
//==============================================================================
void console_resume_after_debugger_run (void)
{
 console.resume_by_debugger = 1;
}

//==============================================================================
// Console mode using stdout
//
//   pass: void
// return: void
//==============================================================================
static void console_mode (void)
{
 char si[1000];
 char s[1024];

 console.console_stdout = 1;

 if (! z80debug_print_console_prompt())
    {
     xprintf(
"================================= Console mode ================================\n"
"Make this window the current focus to enter options.  Enter options or an\n"
"empty line to exit console mode.\n"
"\n");
    }

 for (;;)
    {
     xprintf("ubee512>");
     xflush();

     console.end_by_debugger = 0;

     if (fgets(si, sizeof(si), stdin))
        ; // no error

     if (si[0] == '\n')
        {
         xprintf(
         "\n"
         "Console mode has been terminated, select ubee512 window now.\n");
         break;
        }

     si[strlen(si)-1] = 0;

     // prepend "ubee512 " as argv[0]
     snprintf(s, sizeof(s), "ubee512 %s", si);
     console.xstdin = 1;
     options_make_pointers(s);
     options_process(c_argc, c_argv);

     if (console.end_by_debugger)
        break;

     // redraw the microbee screen to reflect any related option changes
     gui_status_update();
     crtc_set_redraw();
     video_update();
    }

 console.console_stdout = 0;
}

//==============================================================================
// Activate/de-activate the debug logging stream.
//
//   pass: int activate                 1 to activate, 0 to de-activate
// return: void
//==============================================================================
void console_debug_stream (int activate)
{
 console_set_devices((console_get_devices() & ~CONSOLE_DEBUG) |
 (CONSOLE_DEBUG * activate));
}

//==============================================================================
// Process --output arguments.
//
//   pass: int arg                      argument number (0=all)
//         int pf                       prefix used 0='-', 1='+'
// return: void
//==============================================================================
void console_proc_output_args (int arg, int pf)
{
 switch (arg)
    {
     case  0 :
        console_set_devices(CONSOLE_ALL * pf);
        break;
     case  1 :
        console_set_devices((console_get_devices() & ~CONSOLE_OSD) |
        (CONSOLE_OSD * pf));
        break;
     case  2 :
        console_set_devices((console_get_devices() & ~CONSOLE_STDOUT) |
        (CONSOLE_STDOUT * pf));
        break;
    }
}

//==============================================================================
// Console commands
//
//   pass: int cmd                      console command
// return: void
//==============================================================================
void console_command (int cmd)
{
 switch (cmd)
    {
     case EMU_CMD_CONSOLE :
        if (! video.fullscreen)
           console_mode();
        break;
    }
}
