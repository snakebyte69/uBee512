//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              uBee512 module                                *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Provides init, application loop, and deinit.
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
// v6.0.0 - 5 February 2017, uBee
// - Added in main() a new test for 'emu.exit_warning'.
// v6.0.0 - 1 January 2017, K Duckmanton
// - Minor changes to explicitly set the aspect ratio and Y-scale factor
//   when the emulation is reset.  video_renderer() is now video_render() to
//   match video.c.  The semantics of the .hwflash member in model_t have
//   changed.  Rather than being a flag, it now records the version of the
//   flashing circuit being emulated.
//
// v5.8.0 - 12 December 2016, uBee
// - Added an userhome_srampath for new batter backup of SRAM support.
//
// v5.5.0 - 27 June 2013, uBee
// - Fixed p1024k model PCG banks value in model_data[] to be 16 not 32.
// v5.5.0 - 21 June 2013, B.Robinson
// - Update debug_execution_loop() to resume console input after debug
//   operation.
//
// v5.4.0 - 2 Jan 2012, uBee
// - Added Microbee Technology's p1024k/1024k (pplus) model emulation.
//
// v5.3.0 - 3 April 2011, uBee
// - Added tapfile_* functions to init_func[] table.
//
// v5.2.0 - 7 August 2010, K Duckmanton
// - Emulate the SN76489AN sound generator, which could be fitted to
//   Premium series motherboards as an option.
//
// v5.1.0 - 20 November 2010, uBee
// - Added turbo_reset() function to disable turbo speed correctly.
//
// v5.0.0 - 4 August 2010, uBee
// - Added an audio DAC parallel port device 'dac_ops'
// - Added code to handle a power cycle.  This should act as if the power
//   was removed and re-applied so de-inits then inits all the hardware.
// - Moved module initilisation code to init_modules() function.
// - Moved module de-initilisation code to deinit_modules() function.
// - Moved module reset code to reset_modules() function.
// - Replaced constant reset action numbers with EMU_RST_* defines.
// - Added a flag member to structure 'init_func_t'.
// v5.0.0 - 13 July 2010, K Duckmanton
// - Removed all references to the 'sound' global variable and replaced them
//   with references to the 'audio' global instead.
// - Remove all code that selects one of two speaker drivers.
// - Changes to the emulated CPU speed are now communicated to all sound
//   sources via the audio_clock() function.
//
// v4.7.0 - 29 June 2010, uBee
// - Lock keys up/down bug is fixed in SDL-1.2.14 when using the SDL
//   environment variable "SDL_DISABLE_LOCK_KEYS=1" which is set here in the
//   main() function.
// - Added emu.system variable initialising code in main().
// - Changes made to symlink() function to use the result as some compilers
//   report warning: declared with attribute warn_unused_result.
// v4.7.0 - 17 June 2010, K Duckmanton
// - Changes to allow several different devices to be connected to the
//   emulated parallel port.  Added code to emulate the BeeThoven and
//   BeeTalker devices.
//
// v4.6.0 - 16 May 2010, uBee
// - Changes to Teleterm 'tterm' model data in model_data[] to use RTC.
// - Changes made to application_loop() when emulator is paused to make
//   the console much more responsive.
// - application_loop() function has been split up into new functions of
//   debug_execution_loop(), normal_execution_loop() and emulation_delay().
// - Improvements to debug and pause modes by untangling code.
//
// v4.5.0 - 2 April 2010, uBee
// - Removed code that copies 'rom.md5' from the config directory as MD5s
//   for ROMs are now auto generated by uBee512.
// - Fixed account creation to report that a configuration file has been
//   created when 'no_roms_alias' is detected. (was using 'no_disks_alias')
//
// v4.4.0 - 14 August 2009, uBee
// - Added MOD_PC85B model for Standard PC85 models that use 16K Pak ROMs.
//
// v4.3.0 - 31 July 2009, uBee
// - Added '2mhzdd' and 'dd' Dreamdisk models (workerbee).
//
// v4.2.0 - 13 July 2009, uBee
// - Added WD1002-5 Winchester/Floppy drive controller card emulation (hdd).
// - Added Microbee mouse peripheral emulation.
//
// v4.1.0 - 21 June 2009, uBee
// - Added models SCF and PCF (Compact Flash CB Standard/Premium)
// - Added time and date uBee512 environment variables.
// - Added emu.proc_delay_type to determine the method used for delays in
//   the application_loop() function, controlled by --cpu-delay option.
// - Removed the divider persist code and variables as better methods are
//   in place. 256TC and BN56 ROMs no longer have a problem booting at higher
//   clock rates.
// - Fixed an account creation problem where the directories failed to be
//   created on Unix like platforms. This bug was introduced in v3.1.0.
// - init/deinit/reset function table added which will now report any deinit
//   or reset functions failing.
//
// v4.0.0 - 13 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
//
// v3.1.0 - 22 April 2009, uBee
// - Added conditional message reporting based on --verbose option.
// - Removed all occurrences of console_output() function calls.
// - Added 'roms.md5' to the create_account() function.
// - Changes to the debugging section makes the display more responsive and
//   allows emulator commands to now work without delay while debugging.
// - Removed calls to gui_ask_exit_emulator() and gui_ask_reset_emulator()
//   functions and recoded to use new OSD dialogues.
// - Added SDL_MOUSEMOTION event to the event_handler() function.
// - Added z80debug_init(), z80debug_deinit() and z80debug_reset() calls.
// - Changed function_update() to keyb_update() as code was moved.
// - Changed standard PC and PC85 models to default to monochrome models.
// - Added code to exit after a --runsecs number of seconds has elapsed.
// - moved console_output() function over to the console.c module!
// - Changed all printf() calls to use a local xprintf() function.
// - Changed create_account() function to install libdskrc to the 'share'
//   directory on Unix systems and create a '.libdskrc' link to it from the
//   user's home account making the Unix layout the same as Windows.
// - Added UBEE_USERHOME local variable containing the user's home path on
//   Unices, or the directory containing the executable on Windows systems.
// - Added emu.prefix_path variable that can be set using --prefix option
//   to determine where the installation files are located.
// - Added OpenGL conditional code compilation using USE_OPENGL.
// - Modified set_account_paths() function and now only called from options.c
//   module. Default home account code moved to main().
// - userhome_acc variable renamed to userhome_path.
//
// v3.0.0 - 7 October 2008, uBee
// - Added code to correctly handle double quoted command line arguments
//   when using win32 before being processed by getopts_long().
// - Simplified the timer and delay processes by introducing a time_get_ms()
//   and time_delay_ms() functions.
// - Added SDL_VIDEOEXPOSE event to redraw display when using OpenGL.
// - Added SDL_VIDEORESIZE event for OpenGL texture mode resizing.
// - Added video_update() function as now have a video.c module.
// - Changed version checking to skip a new version install if a later version
//   ID string is found. This allows older version binaries to run without
//   modifying the version ID. This will only work with emulator versions
//   3.0.0 and onwards.
// - Added returning the exitstatus to the end of the main() function.
//
// v2.8.0 - 31 August 2008, uBee
// - Changes to the way gui_status_update() is called.
// - Initialise new emu_t members win32_lock_key_fix and x11_lock_key_fix.
// - Changes to icon BMP format allows 3 or 4 bytes storage per pixel.
// - Added rtc_clock() function call.
// - Increased the emu.divider_persist values. See application_setup() notes.
//
// v2.7.0 - 24 June May 2008, uBee
// - Clear the current block counter (emu.z80_blocks) to allow faster speed
//   changes when setting clock speed.
// - 512k and 256k models now default to monochrome mode.
// - Windows and Unices account creation is now share similar methods.
// - libdskrc is now libdskrc.sample and is installed as libdskrc as required.
// - ubee512rc will now be created from the ubee512rc.sample file if it does
//   not exist.
// - Added roms.alias and disk.alias to the create_account() function.
// - SDL_EventState() call moved and changed values, SDL_IGNORE is replaced
//   with SDL_ENABLE.  This function does not seem to do anything actually!
// - Added userhome_confpath.
// - Added a default 'files' directory for holding Microbee programs.
// - Changes to the exec_z80_ratio() function to take a persist parameter.
// - Changes to the application_setup function to handle differnt boot ROMs.
// - Added divider_persist code to the application_loop() function. This was
//   previously fast_poll_persist code moved over from the pio.c module.
// - Added two extra parameters to the set_clock_speed() function.
// - Changes to prevent emulator trying to catch up large lost time values
//   with an option to report the time lost.
// - Removed Z80 CPU execution speed fine adjustment code.
// - Added structures emu_t, crtc_t, sound_t and moved variables into it.
// - Moved crtc code in the application_loop() function over to the crtc.c
//   module function crtc_update().
// - Moved sound code in the application_loop() function over to the sound.c
//   module function sound_update().
// - Added gui_update() GUI function call to handle all GUI updates.
// - Added icon transparency masking code.
// - Changed console_output() function so that parent icon is always found
//   when AllocConsole() is called.
//
// v2.6.0 - 13 May 2008, uBee
// - Z80 CPU speed improvements to better regulate the execution rate rather
//   than just per each Z80 code frame executed.  This improves sound quality
//   but may vary between installations.
// - Added Joystick initilisation and events.
// - Many tape, printer and serial variables have been placed into structures
//   and declared in the appropriate modules.
//
// v2.5.0 - 20 March 2008, uBee
// - Added Teleterm model emulation.
// - Added 'tckeys' member to modelx structure.
// - Moved ApplicationUsage(), param_error_mesg() and getopt_long() coding
//   over to the new options.c module.
// - Change the directory location where the libdskrc file is located.
// - Renamed functions containing upper case letters.
// - Implement the modelc structure.
//
// v2.4.0 - 19 February 2008, uBee
// - Added options for use with libdsk.  --ltype lists the driver types.
//   --lformat lists the available format types.  --type sets the driver
//   type, and --format sets the disk format.
// - Added --side1as0 option to allow disks in LibDsk to read disks that have
//   the corrected side information in the sector headers.
// - Added sector size probing option --psec for access to protected disks.
// - Changes associated with -a to -d disk options.
// - Added +fdc_wtd and +fdc_wth debug output options.
// - Re-instated the --vdu option that was removed in v2.3.0.
// - Re-instated the paging of the text output for the --help option in
//   Windows as default buffers in the console are not large enough.
// - Added gui_status_update() and gui_status_timer() function calls and
//   removed the SDL_WM_SetCaption(TITLESTRING, ICONSTRING) call.
//
// v2.3.0 - 23 January 2008, uBee
// - Removed the vdu_configure() function and changed the way the --pcg
//   option is interpreted.
// - The --vdu option no longer does anything. It remains but does nothing.
// - Added --hint half intensity monochrome option for alpha+ models.
// - Removed the "invalid Z80 code" workaround during initial start up as
//   the problem causing it has now been fixed (see z80.c)
// - Removed the --standard (-s) option and replaced with --col and --mono
//   options that enables/disables colour for standard models.
// - Removed the 'standard' member from the model_t structure.
// - Added new members 'alphap' and 'pcg' to the model_t structure.
// - Added dissasembly options to debug commands (+on+trace+step+alt+count)
// - Added --dump option to set the dump address.
// - Added --bp and --bpc options to set break points for debugging.
// - Added --regs options and regdump_t structure.
// - Added modio_t and debug_t structures,  debugging methods have been
//   completely overhauled with additional functionality and flexbility.
//
// v2.2.0 - 13 December 2007, uBee
// - Added 'ubee512_ver.id' file to home account to allow later versions to
//   upgrade all the required account files for Unix systems.
// - Changed the default model values to use old colour circuitry where needed.
// - Added a --hwflash option for Premium (alpha+) models and changed the
//   default setting for hardware flashing for the standard Premium model.
// - Create directories in users home accounts for the placement of tools.
// - Changes to the reset section to optionally confirm the reset action. This
//   was required for the reset in the function module.
//
// v2.1.0 - 29 October 2007, uBee
// - M+RESET keys will not request a confirmation (for jumping into Monitor)
// - Setting of CPU clock speed now handled by set_clock_speed() function. this
//   can be called any time the CPU speed needs changing.
// - Added --lpen option to enable 6545 light pen key emulation for 256tc.
// - Added CPU clock speed functions and --speedsel option.
// - Added PIO port B bit 7 source option --piob7.
// - Added mouse button down and up event detection.
// - Added SDL version to the --version output option.
// - xtal is now cpuclock,  added new option name of --clock, options -x and
//   --xtal will be still be available for now.
// - model not found exits with segmentation fault fixed.
// - *model_args had incorrect ordering of p256k, 256k, and p128k, 128k models,
//   moved the 256k models above the 128k models.
// - The help option output is now organised into sections.
// - Better account creation for Unices when upgrading to later versions. Any
//   missing directories will be created.
// - Start some code re-organisation.
//
// v2.0.0 - 13 October 2007, uBee
// - Confirmation windows added for Unices builds.
// - Added --nodisk option.
// - Added --rtc option and rtc related functions.
// - Rearranged ordering from most recent to oldest for FDD models and enabled
//   the PJB P256K, 256K and 256TC models.
// - APC model name changed to 56K.
//
// v1.4.0 - 6 October 2007, uBee
// - Added --mmode option to force the FDD ROM's monitor mode to start.
// - Added --pcg option to select amount of PCG RAM for emulation (Premium).
// - Added --vdu option to select amount of VDU RAM for emulation (Premium).
// - Added --aspect option to select 1:1 or 2:1 Y scaling.
// - Changes for console input/output for the windows port, adding -mwindows
//   to the linking stopped the background console prompt appearing.
// - Added (Windows) --conio option to allow console to be output at all
//   times, else only outputs for --help and --version options or when an
//   error causes the emulator to exit.  If any console output was made a
//   message box appears requiring confirmation to allow the console output
//   to be read before exiting.  By default all stdout and stderr output
//   will redirected to files stdout.txt and stderr.txt files (see SDL).
// - Close window mouse click on [X] is now functional with a confirmation
//   request before exiting in Windows build.
// - Confirmation request for exit and reset keys added for Windows build,
//   ESC+RESET will not request a confirmation.
// - Added --model option.  This allows other model Microbees to be emulated.
// - Changes for win32 build to allow installation to any directory.  This now
//   allows an installer to be used instead of a ZIP file.
// - Main event handler moved out of keyb.c module an used here to dispatch to
//   each event handler as needed.
// - string_search() function now returns matching index value and -1 if no
//   match found.
// - option --sound=off now does not init sound in SDL.
// - Start some code re-organisation.
//
// v1.3.0 - 1 September 2007, uBee
// - Added PIO polling and PIO configure (for interrupts) with execution
//   ratio change feature when interrupts are made active.
// - Added RS232 serial TX and RX options to set communication port,  baud
//   rates for each direction, data and stop bits: --coms, --baud, --baudtx,
//   --baudrx, --datab and --stopb.
// - Added printer output options --print and --printa.
// - Additional directory 'printer' created in the user's home account in the
//   .ubee512 directory.
// - Added --tapevol option for setting volume level of tape wave file.
// - Changed some error messages to put out function name first.
//
// v1.2.0 - 21 August 2007, uBee
// - Added --tapeo, --tapei, and --tapesamp options to support tape functions.
// - Additional directory 'tapes' created in the user's home account in the
//   .ubee512 directory.
// - Added function module calls for init, deinit and reset.
// - disk_and_roms() function name changed to set_account_paths()
//
// v1.1.0 - 18 August 2007, uBee
//   Code has been modified to allow the XTAL frequency to be specified.  In
//   theory if the host PC can emulate the Z80 code at that speed then that
//   is what you will get with a frame rate of (n) FPS.
//
//   See the README file for an explanation of how the speed emulation works.
//
// - stdout and stderr on windows now directed back to the console instead
//   of stdout.txt and stderr.txt files.
// - Modified the cursor blinking and Premium hardware inverse/flashing
//   video timer methods depending on if turbo mode is used.  This keeps the
//   flash rate correct.
// - Added frate command line option, mainly intended for hacking.
// - Added vblank command line option, mainly intended for use with the
//   turbo option to make the host timer determine the 50Hz VBLANK rate.
// - Added XTAL command line option (-x) where the xtal rate in MHz can be
//   specified in floating point format.
// - Added sound command line options: off, prop and normal.
// - Added volume setting command line option.
// - Changed sound hold-off to use a timer instead of Z80 cycle counter.
// - Changed from getopt to getopt_long command arguments. Uses same single
//   options as before.
// - Drive B not responding if a drive D was specified.  Problem caused by
//   floppyd[0] incorrectly labeled as floppyb[0] in the disk_and_roms()
//   function clearing any values already set for floppyb.
//
// v1.0.1 - 11 August 2007, uBee
// - Wrong default disk image type set for boot. Was boot.dip,  changed to
//   boot.dsk
//
// v1.0.0 - 8 August 2007, uBee
// - A major recoding of most of the original source to improve speed
//   regulation and for the addition of new features.
// - Create directories in users home accounts for the placement of disk and
//   ROM images for first time use.
// - Z80_cycles and associated variables now use unsigned 64 bit variables.
// - Added signal call back handler to allow control from other applications.
// - Added video timers and function calls in application loop for blinking
//   cursor and alpha+ hardware video flashing.
// - Added some code in the application loop to support the new sound emulation
//   feature.
// - Recoded the Z80 CPU speed regulation for the clock target.  The code now
//   uses the gettimeofday() function for unices and clock() function in mingw
//   builds to determine delay periods between mz80exec(n) function calls.
// - Added command line option for standard and standard+colour emulation.
// - Added command line monitor type options.
// - Added command line parameters for disk images for drives A-D.
// - Added turbo command line option.
// - Added fullscreen command line option.
// - Report init failures, not successes.
//
// v0.0.0 - 16 June 2007, uBee
// Start with "nanowasp" source distribution version 0.22. An emulator for the
// microbee 128k. Copyright (C) 2000-2003  David G. Churchill.
//==============================================================================

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <SDL.h>

#ifdef USE_LIBDSK
#include <libdsk.h>
#endif

#ifdef MINGW
#include <windows.h>
#include <conio.h>
#else
#include <sys/stat.h>
#include <signal.h>             // signal name macros, and the signal() prototype
#include <errno.h>
#endif

#include <stdint.h>
#include <assert.h>

#include "ubee512.h"
#include "z80api.h"
#include "options.h"
#include "z80.h"
#include "vdu.h"
#include "crtc.h"
#include "memmap.h"
#include "keyb.h"
#include "fdc.h"
#include "hdd.h"
#include "ide.h"
#include "gui.h"
#include "video.h"
#include "osd.h"
#include "roms.h"
#include "pio.h"
#include "audio.h"
#include "sound.h"
#include "dac.h"
#include "tape.h"
#include "tapfile.h"
#include "serial.h"
#include "rtc.h"
#include "mouse.h"
#include "clock.h"
#include "support.h"
#include "function.h"
#include "z80debug.h"
#include "parint.h"
#include "joystick.h"
#include "keystd.h"
#include "sn76489an.h"
#include "console.h"

#include "macros.h"

//==============================================================================
// constants
//==============================================================================
#define DEBUG_DELAY 0           /* set to 1 to print timing
                                 * information for each part of the
                                 * application loop */
#define DEBUG_TSTATES 0         /* set to 1 to measure the number of
                                 * z80 tstates per iteration of the
                                 * application loop */

//==============================================================================
// structures and variables
//==============================================================================
emu_t emu =
{
 .model = MOD_DEFAULT,
 .framerate = FRAMERATE,
 .maxcpulag = EMU_MAXLAG_MS,
 .z80_divider = EMU_Z80_DIVIDER,
 .slashconv = EMU_SLASHCONV,
 .new_pc = -1,
 .alias_disks = 1,
 .alias_roms = 1,
 .win32_lock_key_fix = 1,
 .x11_lock_key_fix = 0,
 .exit_check = 1,
 .cpuclock_def = CPU_CLOCK_FREQ,
 .cmd_repeat1 = EMU_REPEAT1,
 .cmd_repeat2 = EMU_REPEAT2,
 .hardware=0xffffffff
};

regdump_t regdump;
model_t modelx;
model_custom_t modelc;
modio_t modio;
messages_t messages;

char userfile[SSIZE1];
char destfile[SSIZE1];

static char userhome_path[SSIZE1];
char userhome[SSIZE1];
char userhome_confpath[SSIZE1];
char userhome_docspath[SSIZE1];
char userhome_diskpath[SSIZE1];
char userhome_romspath[SSIZE1];
char userhome_srampath[SSIZE1];
char userhome_tapepath[SSIZE1];
char userhome_prntpath[SSIZE1];
char userhome_imagepath[SSIZE1];
char userhome_rtcpath[SSIZE1];
char userhome_toolspath[SSIZE1];
char userhome_filespath[SSIZE1];
char userhome_sharepath[SSIZE1];

volatile char gui_signal;

char *model_args[] =
{
 "256tc",
 "p1024k",
 "1024k",
 "p512k",
 "512k",
 "p256k",
 "256k",
 "p128k",
 "128k",
 "p64k",
 "64k",
 "56k",
 "tterm",
 "ppc85",
 "pc85b",
 "pc85",
 "pc",
 "ic",
 "2mhz",
 "2mhzdd",                      /* Dreamdisk system attached to a 2MHz microbee */
 "dd",                          /* Dreamdisk system attached to a 3.375MHz microbee */
 "scf",
 "pcf",
 ""
 };

char *wdays_c[] =
{
 "Sun",
 "Mon",
 "Tue",
 "Wed",
 "Thu",
 "Fri",
 "Sat"
};

char *wdays_l[] =
{
 "sun",
 "mon",
 "tue",
 "wed",
 "thu",
 "fri",
 "sat"
};

char *wdays_u[] =
{
 "SUN",
 "MON",
 "TUE",
 "WED",
 "THU",
 "FRI",
 "SAT"
};

const model_t model_data[MOD_TOTAL] =
{
//           ALPHAP  TCKEYS     ROM  IDE  HDD   boot   FDC         RAM  PCG  VDU  COLOUR   HWF  MHI LPEN    SPEED       PIOB7     RTC  CLOCK
    {/* 256tc */  1,      1,      0,   0,   0, 0x8000, MODFDC_AT,  256,  8,  0, MODCOL2, HFV4,   1,   0,  MODSPD, MODPB7_RTC, MODRTC, 3.375},
    {/*p1024k */  1,      0,      0,   0,   0, 0x8000, MODFDC_AT, 1024, 16,  0, MODCOL2, HFV4,   1,   1,       0, MODPB7_RTC, MODRTC, 3.375},
    {/* 1024k */  1,      0,      0,   0,   0, 0x8000, MODFDC_AT, 1024,  1,  0,       0, HFNO,   0,   1,       0, MODPB7_RTC, MODRTC, 3.375},
    {/* p512k */  1,      0,      0,   0,   0, 0x8000, MODFDC_AT,  512, 16,  0, MODCOL2, HFV3,   1,   1,  MODSPD, MODPB7_RTC, MODRTC, 3.375},
    {/*  512k */  0,      0,      0,   0,   0, 0x8000, MODFDC_AT,  512,  1,  0,       0, HFNO,   0,   1,  MODSPD, MODPB7_RTC, MODRTC, 3.375},
    {/* p256k */  1,      0,      0,   0,   0, 0x8000, MODFDC_AT,  256, 16,  0, MODCOL2, HFV3,   1,   1,  MODSPD, MODPB7_RTC, MODRTC, 3.375},
    {/*  256k */  0,      0,      0,   0,   0, 0x8000, MODFDC_AT,  256,  1,  0,       0, HFNO,   0,   1,  MODSPD, MODPB7_RTC, MODRTC, 3.375},
    {/* p128k */  1,      0,      0,   0,   0, 0x8000, MODFDC_AT,  128,  8,  0, MODCOL2, HFNO,   0,   1,       0, MODPB7_PUP,      0, 3.375},
    {/*  128k */  0,      0,      0,   0,   0, 0x8000, MODFDC_AT,  128,  1,  0,       0, HFNO,   0,   1,       0, MODPB7_PUP,      0, 3.375},
    {/*  p64k */  1,      0,      0,   0,   0, 0x8000, MODFDC_AT,   64,  8,  0, MODCOL2, HFNO,   0,   1,       0, MODPB7_PUP,      0, 3.375},
    {/*   64k */  0,      0,      0,   0,   0, 0x8000, MODFDC_AT,   64,  1,  0,       0, HFNO,   0,   1,       0, MODPB7_PUP,      0, 3.375},
    {/*   56k */  0,      0,      0,   0,   0, 0xE000, MODFDC_AT,   56,  1,  0,       0, HFNO,   0,   1,       0, MODPB7_PUP,      0, 3.375},
    {/* tterm */  1,      1, MODROM,   0,   0, 0x8000,         0,   32, 16,  0, MODCOL2, HFV4,   1,   0,       0, MODPB7_RTC, MODRTC, 3.375},
    {/* ppc85 */  1,      0, MODROM,   0,   0, 0x8000,         0,   32,  8,  0, MODCOL2, HFNO,   0,   1,       0,  MODPB7_VS,      0, 3.375},
    {/* pc85b */  0,      0, MODROM,   0,   0, 0x8000,         0,   32,  1,  0,       0, HFNO,   0,   1,       0,  MODPB7_VS,      0, 3.375},
    {/*  pc85 */  0,      0, MODROM,   0,   0, 0x8000,         0,   32,  1,  0,       0, HFNO,   0,   1,       0,  MODPB7_VS,      0, 3.375},
    {/*    pc */  0,      0, MODROM,   0,   0, 0x8000,         0,   32,  1,  0,       0, HFNO,   0,   1,       0,  MODPB7_VS,      0, 3.375},
    {/*    ic */  0,      0, MODROM,   0,   0, 0x8000,         0,   32,  1,  0,       0, HFNO,   0,   1,       0, MODPB7_PUP,      0, 3.375},
    {/*  2mhz */  0,      0, MODROM,   0,   0, 0x8000,         0,   32,  1,  0,       0, HFNO,   0,   1,       0, MODPB7_PUP,      0, 2.000},
    {/*2mhzdd */  0,      0,      0,   0,   0, 0xe000, MODFDC_DD,   56,  1,  0,       0, HFNO,   0,   1,       0, MODPB7_PUP,      0, 2.000},
    {/*    dd */  0,      0,      0,   0,   0, 0xe000, MODFDC_DD,   56,  1,  0,       0, HFNO,   0,   1,       0, MODPB7_PUP,      0, 3.375},
    {/*   scf */  0,      0,      0,   1,   0, 0x8000, MODFDC_AT, 2048,  1,  0,       0, HFNO,   0,   1,  MODSPD, MODPB7_RTC, MODRTC, 3.375},
    {/*   pcf */  1,      0,      0,   1,   0, 0x8000, MODFDC_AT, 2048, 16,  0, MODCOL2, HFV3,   1,   1,  MODSPD, MODPB7_RTC, MODRTC, 3.375}
};

#ifdef MINGW
#else
static char *shared_images[] =
{
 "ubee512-logo.bmp",
 NULL
};

static char *shared_disks[] =
{
 "ubee512_cpm_tools.ss80_",
 "ubee512_cpm_tools.ds40_",
 "ubee512_cpm_tools.ds80_",
 "ubee512_cpm_tools.ds82_",
 "ubee512_cpm_tools.ds84_",
 NULL
};

static char *acc_dir_paths[] =
{
 userhome_romspath,
 userhome_tapepath,
 userhome_prntpath,
 userhome_rtcpath,
 userhome_srampath,
 userhome_diskpath,
 userhome_docspath,
 userhome_imagepath,
 userhome_toolspath,
 userhome_filespath,
 NULL
};
#endif

// EMU_RST2 is not included for 'gui_reset' as when OpenGL full screen is current and after doing a reset the
// first mouse click (any button) afterwards does not generate a mouse button event.
static init_func_t init_func[] =
{
 {z80_init,      z80_deinit,      z80_reset,      EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,      "z80"},
 {vdu_init,      vdu_deinit,      vdu_reset,      EMU_INIT                     + EMU_RST1 + EMU_RST2,      "vdu"},
 {clock_init,    clock_deinit,    clock_reset,    EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,    "clock"},
 {gui_init,      gui_deinit,      gui_reset,      EMU_INIT                     + EMU_RST1,                 "gui"},
 {memmap_init,   memmap_deinit,   memmap_reset,   EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,   "memmap"},
 {roms_init,     roms_deinit,     roms_reset,     EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,     "roms"},
 {crtc_init,     crtc_deinit,     crtc_reset,     EMU_INIT                     + EMU_RST1 + EMU_RST2,     "crtc"},
 {keyb_init,     keyb_deinit,     keyb_reset,     EMU_INIT                     + EMU_RST1 + EMU_RST2,     "keyb"},
 {fdc_init,      fdc_deinit,      fdc_reset,      EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,      "fdc"},
 {ide_init,      ide_deinit,      ide_reset,      EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,      "ide"},
 {hdd_init,      hdd_deinit,      hdd_reset,      EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,      "hdd"},
 {pio_init,      pio_deinit,      pio_reset,      EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,      "pio"},
 {rtc_init,      rtc_deinit,      rtc_reset,      EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,      "rtc"},
 {tapfile_init,  tapfile_deinit,  tapfile_reset,  EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,  "tapfile"},
 {joystick_init, joystick_deinit, joystick_reset, EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2, "joystick"},
 {mouse_init,    mouse_deinit,    mouse_reset,    EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,    "mouse"},
 {sn76489an_init,sn76489an_deinit,sn76489an_reset,EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2,"sn76489an"},
 {function_init, function_deinit, function_reset, EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2, "function"},
 {z80debug_init, z80debug_deinit, z80debug_reset, EMU_INIT + EMU_INIT_POWERCYC + EMU_RST1 + EMU_RST2, "z80debug"},
 {NULL,          NULL,            NULL,           0,                                                          ""}
};

static int z80_block_cycles;    // working number of Z80 cycles in 1 block
static int z80_block_cycles_def; // default number of Z80 cycles in 1 block
static int z80_blocks_def;      // default number of Z80 blocks

static int z80_block_cycles_cur; // current number of Z80 cycles in 1 block
static int z80_blocks_cur;      // current number of Z80 blocks

static int z80ms;

static int delay;
static int delay_adj;
static uint64_t ticks1;
static uint64_t ticks2;

extern char *c_argv[];
extern int c_argc;

extern crtc_t crtc;
extern video_t video;
extern mouse_t mouse;
extern audio_t audio;
extern serial_t serial;
extern joystick_t joystick;
extern keystd_t keystd;
extern debug_t debug;

//==============================================================================
// External GUI signal handler.
//
// This allows an optional external GUI program to change values during the
// running of the emulator.  The code here is kept as simple as possible and
// simply just sets a flag to indicate that a signal was received.  The main
// loop checks the flag value to see if any action is required.
//
// Under Unix
// ----------
// Can be tested in unices by using the following to cause a reset():
// $kill -s 30 pid
//
// To find the pid use:
// $ps ax | grep ubee512
//
//   pass: int sig_num
// return: void
//==============================================================================
#ifdef MINGW
#else
void signal_handler (int sig_num)
{
 gui_signal = 1;
}
#endif

//==============================================================================
// Set account directory paths for disks, ROMs, tapes, printers, etc.
// Create uBee512 environment variables.
//
// The Windows version uses the installed location as the userhome area, this
// may require administration rights to use this area.  It does not rely on
// any registry settings.
//
// The Unices version uses the HOME location for the user's files.
//
//   pass: void
// return: int                          0 if no errors, else -1
//==============================================================================
int set_account_paths (void)
{
 char temp_str[SSIZE1];

 typedef struct tm tm_t;

 time_t result;
 tm_t resultp;

 snprintf(userhome_confpath, sizeof(userhome_confpath), "%s%s", userhome, DIR_CONF);
 snprintf(userhome_docspath, sizeof(userhome_docspath), "%s%s", userhome, DIR_DOCS);
 snprintf(userhome_diskpath, sizeof(userhome_diskpath), "%s%s", userhome, DIR_DISKS);
 snprintf(userhome_romspath, sizeof(userhome_romspath), "%s%s", userhome, DIR_ROMS);
 snprintf(userhome_tapepath, sizeof(userhome_tapepath), "%s%s", userhome, DIR_TAPES);
 snprintf(userhome_prntpath, sizeof(userhome_prntpath), "%s%s", userhome, DIR_PRINTER);
 snprintf(userhome_imagepath, sizeof(userhome_imagepath), "%s%s", userhome, DIR_IMAGES);
 snprintf(userhome_rtcpath, sizeof(userhome_rtcpath), "%s%s", userhome, DIR_RTC);
 snprintf(userhome_srampath, sizeof(userhome_srampath), "%s%s", userhome, DIR_SRAM);
 snprintf(userhome_toolspath, sizeof(userhome_toolspath), "%s%s", userhome, DIR_TOOLS);
 snprintf(userhome_filespath, sizeof(userhome_filespath), "%s%s", userhome, DIR_FILES);
 snprintf(userhome_sharepath, sizeof(userhome_sharepath), "%s%s", userhome, DIR_SHARE);

 // set some pre-configured local environment variables
 snprintf(temp_str, sizeof(temp_str), "UBEE512=%s", userhome);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "ubee512=%s", userhome);
 options_ubee512_envvar_set(temp_str);

 // set some time and date local environment variables
 time(&result);
#ifdef MINGW
 memcpy(&resultp, localtime(&result), sizeof(resultp));
#else
 localtime_r(&result, &resultp);
#endif
 snprintf(temp_str, sizeof(temp_str), "SS=%02d", resultp.tm_sec);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "MM=%02d", resultp.tm_min);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "HH=%02d", resultp.tm_hour);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "DD=%02d", resultp.tm_mday);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "mm=%02d", resultp.tm_mon+1);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "YYYY=%04d", resultp.tm_year + 1900);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "YY=%02d", resultp.tm_year % 100);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "ww=%d", resultp.tm_wday);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "ac=%s", wdays_c[resultp.tm_wday]);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "al=%s", wdays_l[resultp.tm_wday]);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "au=%s", wdays_u[resultp.tm_wday]);
 options_ubee512_envvar_set(temp_str);

 return 0;
}

//==============================================================================
// Read the settings from the user's ID file.
//
//   pass: void
// return: void
//==============================================================================
static void read_id_file (void)
{
 FILE *textfp;

 char temp_str[80];
 int i;

 snprintf(userfile, sizeof(userfile), "%s"SLASHCHAR_STR"ubee512_ver.id", userhome);

 textfp = fopen(userfile, "r");
 if (textfp == NULL)
    emu.install_files_req = 1;
 else
    {
     for (i = 0; i < 100; i++)
        {
         file_readline(textfp, temp_str, sizeof(temp_str)-1);
         if (i == 0)
            emu.install_files_req = (xstrverscmp(temp_str, APPVER) < 0);
         else
            {
             if (strcmp(temp_str, "messages_opengl_no") == 0)
                messages.opengl_no = 1;
            }
         if (temp_str[0] == 0)
            break;
        }
     fclose(textfp);
    }
}

//==============================================================================
// Write settings to the user's ID file.
//
//   pass: void
// return: void
//==============================================================================
void write_id_file (void)
{
 FILE *textfp;

 snprintf(userfile, sizeof(userfile), "%s"SLASHCHAR_STR"ubee512_ver.id", userhome);

 textfp = fopen(userfile, "w");
 if (textfp != NULL)
    {
     fprintf(textfp, APPVER"\n");
     if (messages.opengl_no)
        fprintf(textfp, "messages_opengl_no\n");
     fclose(textfp);
    }
}

#ifdef MINGW
#else
//==============================================================================
// Create the directories in the user's uBee512 home account.
//
// Directories may already exist and are not regarded as errors.
//
//   pass: char **dir
// return: int                          -1 if failure to create a dir, else 0
//==============================================================================
static int create_unix_dirs (char **dir)
{
 mode_t mode = S_IRWXG | S_IRWXU | S_IROTH | S_IXOTH;
 int res = 0;
 int i = 0;

 while ((dir[i] != NULL) && (res != -1))
    {
printf("creating %s\n", dir[i]);

     res = mkdir(dir[i], mode);
     if ((res == -1) && (errno == EEXIST))
        res = 0;
     if (res != -1)
        i++;
    }

 if (res == -1)
    {
     xprintf("create_unix_dirs: Error creating %s\n", dir[i]);
     xprintf("create_unix_dirs: Error no: %d\n", errno);
     return -1;
    }

 return 0;
}
#endif

//==============================================================================
// Create a uBee512 account in the user's home directory.
//
//   pass: void
// return: int                          1 if user account/dirs created, else 0
//==============================================================================
static int create_account (void)
{
#ifdef USE_LIBDSK
 int no_libdskrc = 0;
#endif
 int no_ubee512rc = 0;
 int no_roms_alias = 0;
 int no_disks_alias = 0;

 int dir_created = 0;
 FILE *textfp;

#ifdef MINGW
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Windows specific account setup
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // read the settings from the user's ID file
 read_id_file();

 // test if ubee512rc is already in the user's account.  Copy it there if not
 snprintf(destfile, sizeof(destfile), "%s\\ubee512rc", userhome);
 textfp = fopen(destfile, "r");
 no_ubee512rc = (textfp == NULL);
 if (no_ubee512rc)
    {
     snprintf(userfile, sizeof(userfile), "%s\\configs\\ubee512rc.sample", userhome);
     copy_file(destfile, userfile);
    }
 else
    fclose(textfp);

#ifdef USE_LIBDSK
 // test if libdskrc is already in the share directory.  Copy it there if not
 snprintf(destfile, sizeof(destfile), "%s\\share\\libdskrc", userhome);
 textfp = fopen(destfile, "r");
 no_libdskrc = (textfp == NULL);
 if (no_libdskrc)
    {
     snprintf(userfile, sizeof(userfile), "%s\\configs\\libdskrc.sample", userhome);
     copy_file(destfile, userfile);
    }
 else
    fclose(textfp);
#endif

 // test if roms.alias is already in the user's account.  Copy it there if not
 snprintf(destfile, sizeof(destfile), "%s\\"ALIASES_ROMS, userhome);
 textfp = fopen(destfile, "r");
 no_roms_alias = (textfp == NULL);
 if (no_roms_alias)
    {
     snprintf(userfile, sizeof(userfile), "%s\\configs\\"ALIASES_ROMS".sample", userhome);
     copy_file(destfile, userfile);
    }
 else
    fclose(textfp);

 // test if disks.alias is already in the user's account.  Copy it there if not
 snprintf(destfile, sizeof(destfile), "%s\\"ALIASES_DISKS, userhome);
 textfp = fopen(destfile, "r");
 no_disks_alias = (textfp == NULL);
 if (no_disks_alias)
    {
     snprintf(userfile, sizeof(userfile), "%s\\configs\\"ALIASES_DISKS".sample", userhome);
     copy_file(destfile, userfile);
    }
 else
    fclose(textfp);

#else
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Unices specific account setup
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 mode_t mode = S_IRWXG | S_IRWXU | S_IROTH | S_IXOTH;
 int i;

 // create the ubee512 account directory if none exists.
 dir_created = mkdir(userhome, mode) == 0;

 // read the settings from the user's ID file
 read_id_file();

 // test if ubee512rc is already in the user's account.  Copy it there if not
 snprintf(destfile, sizeof(destfile), "%s/ubee512rc", userhome);
 textfp = fopen(destfile, "r");
 no_ubee512rc = (textfp == NULL);
 if (no_ubee512rc)
    {
     snprintf(userfile, sizeof(userfile), "%s"PATH_SHARED_CONFIG"ubee512rc.sample", emu.prefix_path);
     copy_file(destfile, userfile);
    }
 else
    fclose(textfp);

#ifdef USE_LIBDSK
 // create a symbolic link to the ..../share/libdskrc file from the user's
 // home account, this will not overwrite any existing link.
 snprintf(destfile, sizeof(destfile), "%s/share/libdskrc", userhome);
 snprintf(userfile, sizeof(userfile), "%s/.libdskrc", userhome_path);
 if (symlink(destfile, userfile))
    ; // ignore result

 // test if libdskrc is already in the share directory.  Copy it there if not
 textfp = fopen(destfile, "r");
 no_libdskrc = (textfp == NULL);
 if (no_libdskrc)
    {
     mkdir(userhome_sharepath, mode);
     snprintf(userfile, sizeof(userfile), "%s"PATH_SHARED_CONFIG"libdskrc.sample", emu.prefix_path);
     copy_file(destfile, userfile);
    }
 else
    fclose(textfp);
#endif

 // test if roms.alias is already in the user's account.  Copy it there if not
 snprintf(destfile, sizeof(destfile), "%s/"ALIASES_ROMS, userhome);
 textfp = fopen(destfile, "r");
 no_roms_alias = (textfp == NULL);
 if (no_roms_alias)
    {
     snprintf(userfile, sizeof(userfile), "%s"PATH_SHARED_CONFIG""ALIASES_ROMS".sample", emu.prefix_path);
     copy_file(destfile, userfile);
    }
 else
    fclose(textfp);

 // test if disks.alias is already in the user's account.  Copy it there if not
 snprintf(destfile, sizeof(destfile), "%s/"ALIASES_DISKS, userhome);
 textfp = fopen(destfile, "r");
 no_disks_alias = (textfp == NULL);
 if (no_disks_alias)
    {
     snprintf(userfile, sizeof(userfile), "%s"PATH_SHARED_CONFIG""ALIASES_DISKS".sample", emu.prefix_path);
     copy_file(destfile, userfile);
    }
 else
    fclose(textfp);

#if 0
 // if a standard '.ubee512' account then create a 'no-dot' symbolic link to it
 if (! emu.home_account_set)
    {
     snprintf(userfile, sizeof(userfile), "%s/ubee512", userhome_path);
     symlink(userhome, userfile);
    }
#endif

 if (emu.install_files_req)
    {
     // create the sub-directories if not already present
     if (create_unix_dirs(acc_dir_paths) == -1)
        {
         xprintf("main: Failed to create a uBee512 home account.\n");
         return 1;
        }

     // copy files from the shared images directory to the user's home account
     for (i = 0; shared_images[i] != NULL; i++)
        {
         snprintf(userfile, sizeof(userfile), "%s"PATH_SHARED_IMAGES"%s", emu.prefix_path, shared_images[i]);
         snprintf(destfile, sizeof(destfile), "%s%s", userhome_imagepath, shared_images[i]);
         copy_file(destfile, userfile);
        }

     // copy files from the shared disks directory to the user's home account
     for (i = 0; shared_disks[i] != NULL; i++)
        {
         snprintf(userfile, sizeof(userfile), "%s"PATH_SHARED_DISKS"%s", emu.prefix_path, shared_disks[i]);
         snprintf(destfile, sizeof(destfile), "%s%s", userhome_diskpath, shared_disks[i]);
         copy_file(destfile, userfile);
        }

     // create readme.txt file telling the user where to find documentation
     snprintf(userfile, sizeof(userfile), "%sreadme.txt", userhome_docspath);
     textfp = fopen(userfile, "w");
     if (textfp != NULL)
        {
         fprintf(textfp, "Documentation including licensing information is located in:\n");
         fprintf(textfp, "%s"PATH_SHARED_DOCS"\n", emu.prefix_path);
         fclose(textfp);
        }

     // create readme.txt file telling the user where to find tools
     snprintf(userfile, sizeof(userfile), "%sreadme.txt", userhome_toolspath);
     textfp = fopen(userfile, "w");
     if (textfp != NULL)
        {
         fprintf(textfp, "CP/M Z80 emulator tools sources are located in:\n");
         fprintf(textfp, "%s"PATH_SHARED_TOOLS"\n", emu.prefix_path);
         fclose(textfp);
        }
    }
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Inform the user of a new account created or new version installed
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 if (emu.install_files_req)
    {
     write_id_file();

     emu.roms_create_md5 = 1;  // force 'roms.md5.auto' to be regenerated

     xprintf("\n");
     xprintf("==========================================================================\n");
     xprintf("                 ~~~ uBee512 "APPVER" Microbee emulator ~~~\n\n");
     if (dir_created)
        xprintf("This is the first time you have run the ubee512 emulator in this account.\n");
     else
        {
         xprintf("A newer version of the emulator has been detected. Additional directories\n");
         xprintf("and files may have been created in your account.\n");
        }
     xprintf("\n");
     xprintf("You now need to copy ROMs: charrom.bin, and either disk and/or BASIC ROMs\n");
     xprintf("to the ROMs directory if these don't already exist:\n");
     xprintf("%s\n", userhome_romspath);
     xprintf("\n");
     xprintf("Copy any required disk image(s) and make sure these are writable to:\n");
     xprintf("%s\n", userhome_diskpath);
     xprintf("\n");

#ifdef USE_LIBDSK
     if (no_libdskrc)
        {
         xprintf("A new %s"SLASHCHAR_STR"share"SLASHCHAR_STR"libdskrc configuration file has been\n", userhome);
         xprintf("created containing common Microbee disk formats for use by LibDsk.\n");
#ifdef MINGW
#else
         xprintf("A %s/.libdskrc symbolic link should now reference this file.\n", userhome_path);
#endif
         xprintf("\n");
        }
     else
        {
         xprintf("You already have a %s"SLASHCHAR_STR"share"SLASHCHAR_STR"libdskrc file.\n", userhome);
         xprintf("libdsk disk definitions contained in this release can be added to the\n");
         xprintf("above file from %s"SLASHCHAR_STR"share"SLASHCHAR_STR"libdskrc.sample.\n", userhome);
         xprintf("\n");
        }
#endif

     if (no_ubee512rc)
        {
         xprintf("A new %s"SLASHCHAR_STR"ubee512rc configuration file has been\n", userhome);
         xprintf("created containing some common customised sections.\n");
         xprintf("\n");
        }
     else
        {
         xprintf("You already have a %s"SLASHCHAR_STR"ubee512rc configuration file.\n", userhome);
#ifdef MINGW
         xprintf("The %s\\configs\\ubee512rc.sample file may contain\n", userhome);
#else
         xprintf("The %s"PATH_SHARED_CONFIG"ubee512rc.sample file may contain\n", emu.prefix_path);
#endif
         xprintf("some new or amended configurations that may be of interest.\n");
         xprintf("\n");
        }

     if (no_roms_alias)
        {
         xprintf("A new %s"SLASHCHAR_STR""ALIASES_ROMS" configuration file has been\n", userhome);
         xprintf("created. This file eliminates any need to use symbolic links for ROMs\n");
         xprintf("and is platform independent.\n");
         xprintf("\n");
        }

     if (no_disks_alias)
        {
         xprintf("A new %s"SLASHCHAR_STR""ALIASES_DISKS" configuration file has been\n", userhome);
         xprintf("created. This file eliminates any need to use symbolic links for disks\n");
         xprintf("and is platform independent.\n");
         xprintf("\n");
        }

     xprintf("Make any changes (if needed) and run the uBee512 emulator again.\n");
     xprintf("==========================================================================\n");
     return 1;          // exit so user can copy the required files
    }

 // create the 'roms.md5.auto' file if needed and determine what md5 shall be used
 roms_create_md5();

 return 0;
}

//==============================================================================
// Icon init.
//
//   pass: void
// return: int                          0 if no errors, else -1
//==============================================================================
static int icon_init (void)
{
#define ICON_PIXELS 128
#define ICON_TRAN 0xffffff

 SDL_Surface *icon;
 uint8_t *pixel_data;
 uint8_t icon_mask[ICON_PIXELS * ICON_PIXELS / 8];

 int pixel_count;
 int pixel_bytes;
 int icon_rgb;
 int mask_count;
 uint8_t mask;
 int i;
 int x;
 int z;

 // create the program icon transparency mask and set the icon, can't get
 // this to work under SDL 2.1.11/13 using SetColorKey so using mask method!
 snprintf(userfile, sizeof(userfile), "%subee512-logo.bmp", userhome_imagepath);

 if ((icon = SDL_LoadBMP(userfile)) != NULL)
    {
     if (icon->w > ICON_PIXELS || icon->h > ICON_PIXELS)
        {
         xprintf("init: ICON file %s is wrong size\n", userfile);
         return -1;
        }
     pixel_data = icon->pixels;
     pixel_count = icon->w * icon->h;
     pixel_bytes = icon->format->BytesPerPixel;

    // xprintf("pixel_bytes=%d\n", pixel_bytes);

     mask_count = 0;
     mask = 0;
     x = 0;
     z = 0;

     for (i=0; i < pixel_count; i++)
        {
         icon_rgb = (pixel_data[x] << 16) | (pixel_data[x+1] << 8) | (pixel_data[x+2]);
         if (icon_rgb != ICON_TRAN)
            mask |= 1;
         x += pixel_bytes;
         mask_count++;

         if (mask_count != 8)
            mask = mask << 1;
         else
            {
             icon_mask[z++] = mask;
             mask_count = 0;
             mask = 0;
            }
        }
     SDL_WM_SetIcon(icon, icon_mask);
     SDL_FreeSurface(icon);

#if 0
     // print out the icon transparency bit mask as ASCII characters for debugging purposes.
     xprintf("icon->w=%d, icon->h=%d,  pitch=%d, pixel_count=%d\n", icon->w, icon->h, icon->pitch, pixel_count);
     mask_count = 0;
     z = 0;
     for (i=0; i < pixel_count; i++)
        {
         if (mask_count == 0)
            {
             mask = icon_mask[z++];
             mask_count = 8;
            }

         if (mask > 127)
            xprintf("X");
         else
            xprintf(" ");
         if (((i+1) % icon->w) == 0)
            xprintf("\n");
         mask = mask << 1;
         mask_count--;
        }
     xprintf("\n");
#endif
    }
 else
    xprintf("init: Unable to load icon image: %s\n", userfile);

 return 0;
}

//==============================================================================
// Initialise modules.
//
//   pass: int flags                    flags indicating what to init
// return: int                          0 if no errors, else failed mod index
//==============================================================================
static int init_modules (int flags)
{
 int i;

 for (i = 0; init_func[i].memory_call_init != NULL; i++)
    {
     if (init_func[i].flags & flags)
        if ((*init_func[i].memory_call_init)() == -1)
           return i;
    }

 // only report if we are up and running or verbose reporting
 if (emu.runmode || emu.verbose)
    xprintf("ubee512: emulation power cycle\n");

 return 0;
}

//==============================================================================
// Initialise.
//
//   pass: void
// return: int                          0 if no errors, else -1
//==============================================================================
static int init (void)
{
 uint32_t sdl_init_properties;
 int i;
 if (osd_init() != 0)
    {
     xprintf("init: Failed osd_init\n");
     return -1;
    }

 if (emu.verbose)
    xprintf(TITLESTRING"\n");

#ifdef MINGW
#else
 // set the GUI signal handler
 signal(30, signal_handler);
#endif

 if (log_init() == -1)
    return -1;

 sdl_init_properties =
    SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO;
 if (joystick.used >= 0)
    sdl_init_properties |= SDL_INIT_JOYSTICK;

#ifndef MINGW
 // set the X window class name, necessary to avoid an SDL crash on
 // Debian with SDL 1.2
 putenv("SDL_VIDEO_X11_WMCLASS=ubee512");
#endif

 if (SDL_Init(sdl_init_properties) != 0)
    {
     xprintf("init: Failed SDL_Init - %s\n", SDL_GetError());
     return -1;
    }

 if (icon_init() != 0)
    return -1;

 if (video_init() != 0)
    {
     xprintf("init: Failed video_init\n");
     return -1;
    }
 else
    {
     SDL_EventState(
        SDL_ACTIVEEVENT | SDL_SYSWMEVENT | SDL_QUIT |
        SDL_KEYUP | SDL_KEYDOWN |
        SDL_MOUSEBUTTONDOWN | SDL_MOUSEBUTTONUP |
        SDL_JOYAXISMOTION | SDL_JOYBALLMOTION | SDL_JOYHATMOTION | SDL_JOYBUTTONUP | SDL_JOYBUTTONDOWN,
        SDL_ENABLE);
    }

 if (audio_init() != 0)
     return -1;
 // set the sound volume
 audio_set_master_volume(audio.vol_percent);

 if ((i = init_modules(EMU_INIT)))
    {
     xprintf("init: Failed %s_init\n", init_func[i].func_name);
     return -1;
    }

 return 0;
}

//==============================================================================
// De-initialise modules.
//
//   pass: int flags                    flags indicating what to deinit
// return: int                          0 if no errors, else failed mod index
//==============================================================================
static int deinit_modules (int flags)
{
 int i;

 for (i = 0; init_func[i].memory_call_deinit != NULL; i++)
    {
     if (init_func[i].flags & flags)
        if ((*init_func[i].memory_call_deinit)() == -1)
           return i;
    }

 return 0;
}

//==============================================================================
// De-initialise.
//
//   pass: void
// return: int                  result of de-init functions
//==============================================================================
static int deinit (void)
{
 int res = 0;
 int i = 0;

 log_deinit();

 if ((i = deinit_modules(EMU_INIT)))
    {
     xprintf("init: Failed %s_deinit\n", init_func[i].func_name);
     res = -1;
    }

 audio_deinit();

 SDL_Quit();

 return res;
}

//==============================================================================
// Reset modules.
//
//   pass: int flags                    flags indicating what to reset
// return: int                          0 if no errors, else failed mod index
//==============================================================================
static int reset_modules (int flags)
{
 int i;

 for (i = 0; init_func[i].memory_call_reset != NULL; i++)
    {
     if (init_func[i].flags & flags)
        if ((*init_func[i].memory_call_reset)() == -1)
           return i;
    }

 return 0;
}

//==============================================================================
// Reset the virtual Microbee.
//
// Make sure all reset functions clear variables that are constantly active
// values.
//
//   pass: int flags            flags indicating what to reset
// return: int                  result of reset functions
//==============================================================================
static int reset (int flags)
{
 int res = 0;
 int i = 0;

 emu.z80_cycles = 0;
 emu.done = 0;

 // only report if we are up and running or verbose reporting
 if (emu.runmode || emu.verbose)
    xprintf("ubee512: emulation reset\n");

 audio_reset();

 if ((i = reset_modules(flags)))
    {
     xprintf("init: Failed %s_reset\n", init_func[i].func_name);
     res = -1;
    }

 return res;
}

//==============================================================================
// Set clock speed setting
//
// All modules dependent on the CPU clock speed are set up from here.  This
// function may be called at any time to change the execution speed.
//
//   pass: float clock          CPU clock speed in MHz.
//         int divider          0 if default value is to be used.
//         int frate            0 if default value is to be used.
// return: void
//==============================================================================
void set_clock_speed (float clock, int divider, int frate)
{
 float z80ms_r;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CPU clock configuration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // convert clock to an integer in Hz
 emu.cpuclock = (int)(clock * 1E6);

 // calculate the number of Z80 cycles executed in 1 frame
 if (frate == 0)
    frate = emu.framerate;
 z80_block_cycles_def = emu.cpuclock / frate;

 // execution time in mS needed for one lot of z80_block_cycles_def
 z80ms_r = ((float)z80_block_cycles_def / emu.cpuclock) * 1000.0;

 z80ms = (int)(z80ms_r);
 if (z80ms < 0)
    z80ms = 0;

 if (divider == 0)
    divider = emu.z80_divider;
 z80_blocks_def = divider;      // use new divider

 // clear the current block counter to allow faster speed changes
 emu.z80_blocks = 0;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Z80 PIO configuration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 pio_configure(emu.cpuclock);

 z80_block_cycles_cur = z80_block_cycles_def / divider;
 z80_blocks_cur = divider;
 emu.z80_ratio = divider;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Tape configuration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 tape_config_out(emu.cpuclock);
 tape_config_in(emu.cpuclock);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// RS232 Serial configuration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 serial_config(emu.cpuclock);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Sound configuration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 audio_clock(emu.cpuclock);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CRTC configuration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 crtc_clock(emu.cpuclock);  // set VBLANK method
 gui_status_update();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// RTC configuration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 rtc_clock(emu.cpuclock);
}

//==============================================================================
// Turbo mode reset.
//
// This needs to be called when --turbo mode is disabled so that it can
// bring the emulation speed back to normal immediately.
//
//   pass: void
// return: void
//==============================================================================
void turbo_reset (void)
{
 ticks1 = time_get_ms();
 delay_adj = 0;     // if we're that late, start again.
 delay = 0;         // and release the CPU (for fairness)
}

//==============================================================================
// Application setup.
//
//   pass: void
// return: void
//==============================================================================
static void application_setup (void)
{
 set_clock_speed(modelx.cpuclock, emu.z80_divider, 0);
 video_configure(video.aspect);
 vdu_configure(video.yscale);
 reset(EMU_RST1);                       // resets the virtual Microbee

 // emulator is now up and running!
 emu.runmode = 1;

 // update the screen now before we start
 crtc_set_redraw();
 video_update();

 emu.secs_init = time_get_secs();
}

//==============================================================================
// Event checking.
//
//   pass: void
// return: void
//==============================================================================
void event_handler (void)
{
 while (SDL_PollEvent(&emu.event))
    {
     switch (emu.event.type)
        {
         case SDL_KEYDOWN:
            keyb_keydown_event();
            break;
         case SDL_KEYUP:
            keyb_keyup_event();
            break;

         case SDL_JOYBUTTONDOWN:
            joystick_buttondown_event();
            break;
         case SDL_JOYBUTTONUP:
            joystick_buttonup_event();
            break;
         case SDL_JOYHATMOTION:
            joystick_hatmotion_event();
            break;
         case SDL_JOYAXISMOTION:
            joystick_axismotion_event();
            break;

         case SDL_MOUSEBUTTONDOWN:
            if (mouse.host_in_use)
               mouse_mousebuttondown_event();
            else
               gui_mousebuttondown_event();
            break;
         case SDL_MOUSEBUTTONUP:
            if (mouse.host_in_use)
               mouse_mousebuttonup_event();
            else
               gui_mousebuttonup_event();
            break;
         case SDL_MOUSEMOTION:
            if (mouse.host_in_use)
               mouse_mousemotion_event();
            else
               gui_mousemotion_event();
            break;

#ifdef USE_OPENGL
         case SDL_VIDEOEXPOSE:
            if (video.type >= VIDEO_GL)
               {
                crtc_redraw();
                if (emu.display_context == EMU_OSD_CONTEXT)
                   osd_redraw();
                video_render();
               }
            break;
         case SDL_VIDEORESIZE:
            video_gl_resize_event();
         break;
#endif
         case SDL_QUIT:
            if (emu.display_context == EMU_OSD_CONTEXT)
               break;
            osd_set_dialogue(DIALOGUE_EXIT);
            break;
        }
    }
}

//==============================================================================
// Debug execution loop.
//
//   pass: void
// return: void
//==============================================================================
static void debug_execution_loop (void)
{
 int tstates;

 emu.z80_blocks = 5000;

 while (emu.z80_blocks--)
    {
     // disassemble and check for break points, etc
     tstates = z80debug_before();

     // execute one opcode instruction
     if (tstates != -1)
        {
         z80api_execute_complete();
         z80debug_after(); // show the state of the Z80 registers, etc
        }

     // if in a stepping mode (even if no steps are currently set)
     if (debug.mode != Z80DEBUG_MODE_RUN)
        {
         keyb_update();   // keyboard updating
         event_handler(); // check and handle any pending events
         crtc_update();   // CRTC updating for cursor and flashing atrributes
         gui_update();    // GUI updating of the status line values
         video_update();  // video updating of the display
        }

     // set a new PC if pending
     if (emu.new_pc != -1)
        {
         emu.z80_blocks = 0;  // make debug finish ASAP
         z80api_set_pc(emu.new_pc);
         emu.new_pc = -1;
         if (emu.paused)
            {
             z80debug_command_exec(EMU_CMD_PAUSE, 0);
             gui_status_update();
            }
        }

     if (console.resume_by_debugger)
        {
         console.resume_by_debugger = 0;
         if (console.end_by_debugger)
            console_command(EMU_CMD_CONSOLE);
        }
    }

 // this is needed when debug.mode is 0 or we won't see the events
 event_handler(); // check and handle any pending events
}

//==============================================================================
// Normal execution loop.
//
// Looping feature for speeding up PIO interrupt responses by having a
// smaller z80_block_cycles value.  Normally z80_blocks is set to 1 with
// z80_block_cycles equal to the default calculated rate.
//
//   pass: void
// return: void
//==============================================================================
static void normal_execution_loop (void)
{
 emu.z80_blocks = z80_blocks_cur;
 z80_block_cycles = z80_block_cycles_cur;
#if 0
 xprintf("emu.z80_blocks=%d  z80_block_cycles=%d\n", emu.z80_blocks, z80_block_cycles);
#endif

 while (emu.z80_blocks--)
    {
     static int64_t block_tstates_delta = 0;
     uint64_t block_tstates_start, block_tstates_end;
     // Execute a block (Z80CYCLES) of Z80 instructions and return the result.
     // The Z80CYCLES value used has been calculated for timing purposes.
     block_tstates_start = z80api_get_tstates();
     z80api_execute(z80_block_cycles + block_tstates_delta);
     block_tstates_end = z80api_get_tstates();
     // compute the number of tstates that the previous block
     // of instructions went over the target
     block_tstates_delta += z80_block_cycles -
        block_tstates_end + block_tstates_start;

     pio_polling();   // poll the PIO for interrupt events
     keyb_update();   // keyboard updating
     event_handler(); // check and handle any pending events
    }

 // set a new PC if pending
 if (emu.new_pc != -1)
    {
     z80api_set_pc(emu.new_pc);
     emu.new_pc = -1;
     if (emu.paused)
        {
         z80debug_command_exec(EMU_CMD_PAUSE, 0);
         gui_status_update();
        }
    }
}

//==============================================================================
// Emulation delay.
//
//   pass: void
// return: void
//==============================================================================
static void emulation_delay (void)
{
 if (emu.turbo)
    {
     time_delay_ms(0);
     return;
    }

 // The required delay is calculated from the last block (Z80CYCLES) of Z80
 // instructions.
 ticks2 = time_get_ms() - ticks1;
 if (ticks2 >= 0)               // if clock() has not wrapped around
    delay = z80ms - ticks2;     // else use the last delay value

 // Keep track of the adjustment value and adjust the delay value
//  delay_x = delay;
 delay += delay_adj; // adjustment value from the last frame
//  delay_adj -= delay_x;
// this may not make much of a difference
 if (delay_adj < -5 * z80ms)
    {
     delay_adj = 0;     // if we're that late, start again.
     delay = 0;         // and release the CPU (for fairness)
    }

 // Execute the delay period.  If a delay is not required then a delay of 0
 // is used to give up the CPU for other processes.
 switch (emu.proc_delay_type)
    {
     case 0:
        if (delay < 0)
//                 time_delay_ms(0);
           ;
        else
           time_delay_ms(delay);  // delay with release of CPU
        break;
     case 1:
        if (delay > 0)
           time_wait_ms(delay);  // delay with NO release of CPU!
        break;
     case 2:
        if (delay < 0)
           time_delay_ms(0);
        else
           time_delay_ms(delay);  // delay with release of CPU
        break;
    }

 // If a big adjustment is required then don't try and keep up.  This can be
 // a big problem under Win32 when the emulator window is dragged and released.
 if (delay_adj > emu.maxcpulag)
    {
     if (modio.ubee512)
        {
         xprintf("emulation_delay: excessive time loss detected:"
                 " %d mS (cleared)\n", delay_adj);
         if (modio.level)
            fprintf(modio.log, "emulation_delay: excessive time loss detected:"
                    " %d mS (cleared)\n", delay_adj);
        }
     delay_adj = 0;
    }
}

//==============================================================================
// Application loop.
//
// This code exits if emu.done becomes true or when the Z80 CPU emulation is
// interrupted with a window box query. This function can then be re-entered
// and the lost time will be cleared.
//
//   pass: void
// return: void
//==============================================================================
static void application_loop (void)
{
 int i;
 
#if DEBUG_DELAY
 uint64_t Tstart, Tcpu, Tsound, Tvideo, Tend;
#endif
#if DEBUG_TSTATES
 uint64_t tstates_start, tstates_end;
#endif

 delay = 0;
 delay_adj = 0;
 ticks1 = time_get_ms();

 while (! emu.done)
    {
     ticks2 = ticks1;
     ticks1 = time_get_ms();
     delay_adj += z80ms - (ticks1 - ticks2);

#if DEBUG_DELAY
     Tstart = ticks1;
#endif
#if DEBUG_TSTATES
     tstates_start = z80api_get_tstates();
#endif

     // if emulator is in a paused state
     if (emu.paused)
        {
         keyb_update();
         event_handler();
        }
     else
        // if Z80 debugging is active
        if (debug.mode != Z80DEBUG_MODE_OFF)
           debug_execution_loop();
        else
           normal_execution_loop();


#if DEBUG_DELAY
     Tcpu = time_get_ms();
#endif
#if DEBUG_TSTATES
     tstates_end = z80api_get_tstates();
     {
      int64_t dtstates =
         z80_blocks_cur * z80_block_cycles_cur -
         tstates_end + tstates_start;
      xprintf("ubee512: %llu tstates, expected %u, delta %lld\n",
              tstates_end - tstates_start,
              z80_blocks_cur * z80_block_cycles_cur,
              dtstates);
     }
#endif

     // update synchronous sound sources
     audio_sources_update();

#if DEBUG_DELAY
     Tsound = time_get_ms();
#endif

     crtc_update();   // CRTC updating for cursor and flashing atrributes
     gui_update();    // GUI updating of the status line values
     video_update();  // video updating of the display

#if DEBUG_DELAY
     Tvideo = time_get_ms();
#endif

     // handle external GUI signals
     if (gui_signal)
        {
         reset(EMU_RST2);
         gui_signal = 0;
        }

     // check for an immediate exit or if an OSD exit dialogue has OK'ed
     if (emu.quit)
        {
         if (! emu.exit_check)
            {
             emu.done = 1;
             emu.quit = 0;
             return;
            }
         if (osd_dialogue_result(DIALOGUE_EXIT) != 0)
            {
             if (osd_dialogue_result(DIALOGUE_EXIT) == OKCANCEL_BTN_OK)
                emu.done = 1;
             emu.quit = 0;
             return;
            }
        }

     // check for an immediate reset or if an OSD reset dialogue has OK'ed
     if (emu.reset)
        {
         switch (emu.reset)
            {
             case EMU_RST_RESET_CON : // if a confirmed reset is required
                if (osd_dialogue_result(DIALOGUE_RESET) == 0)
                   break;
                emu.reset = 0;
                if (osd_dialogue_result(DIALOGUE_RESET) != OKCANCEL_BTN_OK)
                   break;
                reset(EMU_RST2);
                return;
             case EMU_RST_RESET_NOW : // if a non-confirmed reset is required
                emu.reset = 0;
                reset(EMU_RST2);
                return;
             case EMU_RST_POWERCYC_CON : // if a power cycle is required
             case EMU_RST_POWERCYC_NOW :
                if (emu.reset == EMU_RST_POWERCYC_CON)
                   {
                    if (osd_dialogue_result(DIALOGUE_POWERCYC) == 0)
                       break;
                    emu.reset = 0;
                    if (osd_dialogue_result(DIALOGUE_POWERCYC) != OKCANCEL_BTN_OK)
                       break;
                   }
                emu.reset = 0;
                if ((i = deinit_modules(EMU_INIT_POWERCYC)))
                   xprintf("init: Failed %s_deinit\n", init_func[i].func_name);
                if ((i = init_modules(EMU_INIT_POWERCYC)))
                   xprintf("init: Failed %s_init\n", init_func[i].func_name);
                return;
            }
        }

     // insert a delay to get the emulation speed correct
     emulation_delay();

#if DEBUG_DELAY
     Tend = time_get_ms();
     {
      uint64_t cpu_time_ms, sound_time_ms, video_time_ms, delay_time_ms;

      cpu_time_ms = Tcpu - Tstart;
      sound_time_ms = Tsound - Tcpu;
      video_time_ms = Tvideo - Tsound;
      delay_time_ms = Tend - Tvideo;

      xprintf("ubee512: "
              "start %9llums "
              "b %3ums "
              "cpu %3lldms "
              "snd %3lldms "
              "vid %3lldms "
              "delay rq %3dms, got %3lldms "
              "tot %3lldms "
              "\n",
              Tstart, z80ms,
              cpu_time_ms,
              sound_time_ms,
              video_time_ms,
              delay, delay_time_ms,
              Tend - Tstart);
     }
#endif

    }
}

//==============================================================================
// Main program start point.
//
//   pass: int argc, char *argv[]       Command line arguments
// return: int                          0 if no error, 1 if error
//==============================================================================
int main (int argc, char *argv[])
{
 const SDL_version *sdlv;
 char temp_str[SSIZE1];
 const char *env;

 int exitstatus = 0;

 // get a copy of the default model data to work with
 memcpy(&modelx, &model_data[emu.model], sizeof(model_t));

 // options initilisation
 options_init();

 // get the default home account directory.  This will typically be something
 // like c:\Program files\ubee512 under windows and /home/username/.ubee512
 // under Unices if running from standard locations.
#ifdef MINGW
 int i;

 emu.system = EMU_SYSTEM_WINDOWS;

 // get the path and executable filename
 if (GetModuleFileName(NULL, userhome, sizeof(userhome)) == 0)
    {
     xprintf("ubee512: Unable to find the executable path\n");
     return -1;
    }

 // delete the executable name part and last '\' character
 i = strlen(userhome);
 while (userhome[--i] != SLASHCHAR)
    {}
 userhome[i] = 0;
 strcpy(userhome_path, userhome);
#else
#ifdef DARWIN
 emu.system = EMU_SYSTEM_DARWIN;
#else
 emu.system = EMU_SYSTEM_UNIX;
#endif

 char *s = getenv("HOME");
 if (s == NULL)
    {
     xprintf("ubee512: Unable to find the user's home path\n");
     return -1;
    }
 else
    {
     strcpy(userhome_path, s);
     snprintf(userhome, sizeof(userhome), "%s/.ubee512", s);
    }
#endif

 snprintf(temp_str, sizeof(temp_str), "UBEE_USERHOME=%s", userhome_path);
 options_ubee512_envvar_set(temp_str);

#ifdef USE_LIBDSK
 options_ubee512_envvar_set("UBEE_LIBDSK=1");
#endif

#ifdef USE_OPENGL
 options_ubee512_envvar_set("UBEE_OPENGL=1");
#endif

 // obtain the SDL version number for use in other modules
 sdlv = SDL_Linked_Version();
 emu.sdl_version = sdlv->major * 1000000 + sdlv->minor * 10000 + sdlv->patch;

 // Fix for the LOCK_KEYs toggle bug on SDL versions prior to 1.2.14.  This
 // only works for SDL version 1.2.14 and later? and must be used before
 // SDL_Init().  See SDL_keyboard.c, recognised values are "1", "2" and "3".
 // "1"=uses both "2" and "3", "2"=CapsLock, "3"=NumLock).  An --sdl-putenv
 // option allows this to be disabled or modified if required.
 if (emu.sdl_version >= 1020014)
    SDL_putenv("SDL_DISABLE_LOCK_KEYS=1");

 // process command line and initilisation options
 if (! exitstatus)
#ifdef MINGW
    exitstatus = options_process(c_argc, c_argv);
#else
    exitstatus = options_process(argc, argv);
#endif

 // if SDL-1.2.14 or later in use get back the SDL_DISABLE_LOCK_KEYS value
 // to see if the user disabled the LOCK key fix with an option.
 if (emu.sdl_version >= 1020014)
    {
     env = SDL_getenv("SDL_DISABLE_LOCK_KEYS");
     if (env)
        keystd.lockkey_fix = (SDL_atoi(env) == 1) || (SDL_atoi(env) == 2);
    }

 // create the user's account or parts of it if needed
 if (! exitstatus)
    if (create_account())
        exitstatus = 1;

 // if no option errors then initialise the emulator
 if (! exitstatus)
    {
     if (init() != 0)
        {
         exitstatus = 1;
         xprintf("main: Fatal error during initialisation.\n");
         SDL_Quit();
        }
    }

 // set some initial dialogues to show or set as pending
 if (! exitstatus)
    {
#if 1
     if (strstr(APPVER, "dev"))
        osd_set_dialogue(DIALOGUE_DEVMESG);
#endif
     if ((video.type != VIDEO_GL) && (messages.opengl_no == 0))
        osd_set_dialogue(DIALOGUE_OPENGL);
    }

 // if no errors then start the emulator
 if (! exitstatus)
    {
     application_setup();

     while (! emu.done)
        application_loop();
    }

 // if no errors then de-initialise the emulator
 if (! exitstatus)
    {
     if (deinit() != 0)
        {
         emu.runmode = 0;
         exitstatus = 1;
         xprintf("main: Error while de-initialising.\n");
        }
    }

 // if running on Windows then get a confirmation before the console
 // output window is closed.
 if ((exitstatus && (exitstatus != -2)) || emu.exit_warning)
    {
#ifdef MINGW
     gui_message_box(BUTTON_OK, "Read message(s) in console output window before closing.");
#endif
     if (exitstatus == -1)
        return 0;
     else
        return exitstatus;
    }

 return exitstatus;
}
