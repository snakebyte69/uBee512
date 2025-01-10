//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              options module                                *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Provides command line, run time and file options processing including
// basic macro and processing conditionals support.
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
// - Moved functions relating to pixels and pixel colours to the vdu module.
//
// v5.8.0 - 14 November 2016, uBee
// - Added --cpm3 option for use with LibDsk's RCPMFS type.
// - Added CMOS RAM battery backup options --sram-backup, --sram-file,
//   --sram-load and --sram-save.
//
// v5.7.0 - 14 December 2015, uBee
// - Added --sram option to set SRAM size for ROM model emulation.   
// - Added '+regs' and '+memr' arguments to --debug option.
// - Added to help section about Dynamically named RAW FDD and HDD images.
// - Changed 19200 values to 38400 in options_serial_port() and help.
// - Fixed debug_args[] 'tstate' string to be 'tstates' as per help.
// - Changes to OPT_SOUND option as 'off' argument had been drepecated in
//   5.0.0 (can no longer disable sound system).  The 'off' argument will
//   now act as an alias for --snd-mute=on.
// - Removed audio_set_master_volume(0) from OPT_SND_MUTE option as better
//   handling method now in audio.c and fixes the --snd-mute=on option not
//   working.
// - Added Windows 8.1 (W8.1) and Windows 10 (W10) to options_init()
//   system detection.
// - Some minor changes to some functions to eliminate build warnings by
//   replacing "int x" with "int x = 0".
//
// v5.5.0 - 8 July 2013, uBee
// - Added --port58h option to enable 3rd party WD1002-5/WD2793 support
//   for dual drive interface selection using port 0x58.
// - Added --db-trace option for condtional tracing based on PC address range.
// - Added --db-trace-clr option to clear the value set with --db-trace.
// - Added --db-bpos option to break when the PC is outside an address range
//   's' and 'f' (inclusive).
// - Changes to --db-bpclr (--bpclr) to allow optional clearing of all PC
//   break points except inside break points (code moved to z80debug.c).
// - Changes to --db-bp (--bp) to now set 1 or more PC break points in the
//   one option declaration (code moved to z80debug.c).
// - Changes to --db-bpr (--bpr) to now set 1 or more PC break points in the
//   one option declaration (code moved to z80debug.c).
// - Added 256TC v1.31 ROM information in help for the --century option.
// - Added Windows 8 (W8) to options_init() system detection.
// - Fixed OPT_RGB_NN_X options to use the restructured table in crtc.c and
//   also the structure declaration used for it. (broken in 5.0.0)
// v5.5.0 - 21 June 2013, B.Robinson
// - Added --db-bp-mem, --db-bp-meml, --db-bpclr-mem and --db-bpclr-meml for
//   memory read/write breakpoints.
//
// v5.4.0 - 2 Jan 2012, uBee
// - Added Microbee Technology's p1024k/1024k (pplus) model emulation.
// - Changes made to --tapei-det option to take a percentage value to be
//   used for high and low threshold levels.
//
// v5.3.0 - 11 April 2011, uBee
// - Added --tapfile-list, -tapfilei, --tapfileo, --tapfilei-close,
//   --tapfileo-close options for TAP file support.
// - Added 'tapfile' argument to modio table.
// - Created a new OSD group and moved the --osd option to it.
// - Added --osd-consize to set the console dialogue size.
// - Added --osd-conpos to set the console dialogue position.
// - Added --osd-cursor to set the console cursor flash rate and type.
// - Added --osd-list to list all the built in OSD schemes supported.
// - Added --osd-scheme to allow a scheme to be selected.
// - Added --osd-setbtnm to set the colour values for OSD buttons.
// - Added --osd-setbtnt to set the colour values for OSD buttons text.
// - Added --osd-setdiam to set the colour values for OSD dialogue.
// - Added --osd-setdiat to set the colour values for OSD dialogue text.
// - Added --osd-setwidi to set the colour values for OSD widget icons.
// - Added --osd-setwidm to set the colour values for OSD widget.
// - Added --osd-setwidt to set the colour values for OSD widget.
//
// v5.2.0 - 29 March 2011, uBee
// - Added --disk-create option for the creation of disk images.
// v5.2.0 - 19 February 2011, K Duckmanton
// - Added an additional parallel port peripheral - the EA Compumuse
// - Add an option to enable the SN76489AN emulation.  The SN76489AN
//   emulation is only available when a Premium series Microbee is being
//   emulated.
//
// v5.1.0 - 20 November 2010, uBee
// - Added call to turbo_reset() when turbo mode is disabled.
//
// v5.0.0 - 4 August 2010, uBee
// - Added an audio DAC parallel port device argument of 'dac' to the
//   --parallel-port option.
// - Added --powercyc option (rto) to perform a 'power cycle'.
// - Added --js-hat and --js-hatb options to set joystick hat parameters.
// - Added --js-shift option to set the joystick SHIFT button.
// - Added --js-kkb option to make defining mapped joystick keys easier.
// - Changed --js-ACTION options to take multiple button values.
// - Changes made to options_ubee512_envvar_unset() function so that
//   'varname', 'varname=' and 'varname=value' all work.
// - Fixed options_ubee512_envvar_set() to add a trailing '=' character if
//   none is detected in the variable being set.  The
//   options_ubee512_envvar_unset() function is now called before setting
//   the variable to remove any existing definition.
// - Added 'dac' argument to --modio arguments.
// - Replaced constant reset action numbers with EMU_RST_* defines.
// v5.0.0 - 13 July 2010, K Duckmanton
// - Removed all references to the 'sound' global variable and replaced them
//   with references to the 'audio' global instead.
// - Parallel port devices can now be connected and disconnected at runtime
// - The --snd-alg, --snd-freqadj, --snd-freqlow and --snd-holdoff options
//   are now deprecated.  ubee512 will still recognise them but will do
//   nothing.
//
// v4.7.0 - 21 June 2010, uBee
// - Added --sdl-putenv option to allow SDL environment variables to be set.
// - Changes made to the CapsLock key code semi-fix options --lockfix-win32
//   and --lockfix-x11, and other changes made in the --help option.
// v4.7.0 - 17 June 2010, K Duckmanton
// - Added a new '--parallel-port' option, to allow one of several different
//   devices ('printer', 'joystick', 'beetalker', 'beethoven') to be connected
//   to the parallel port.
//
// v4.6.0 - 29 May 2010, uBee
// - Added run-time only option flag bit (OPT_RTO) for options that are only
//   usable after the emulator has been started (rto).
// - Added options for 'quickload mechanism for 8-bit systems' support.
// - Added many --db-* options to the debugging section.
// - Added --dump-header option to enable/disable the header when memory
//   dumping.
// - Added --dump-lines option to determine the default number of lines for
//   a memory dump, previously the value was set at 8 lines.
// - Added --dasm-lines option to determine the default number of lines for
//   disassembly.
// - Added --find-count option to determine the maximum number of matches
//   for the new --db-findb and --db-findm options.
// - Options --bp, --bpr, --bpclr, --bpc now have a corresponding prefixed
//   '--db-' option for consistency and may be removed at a later time.
// - Added 'off', 'debug10', 'debug20', 'piopoll' and 'tstate' arguments to
//   the --debug option.
// - Added --debug-open and --debug-close debug file capture options.
// - Changed all prefixed (i.e +abcd) type arguments to use '+' and '-'
//   prefixes.  The new '-' prefix has the opposite affect to '+' where this
//   is supported.  The arguments are now also checked for errors.
// - Added --reset option (rto).
// - Added Windows 7 (W7) to options_init() system detection.
// - Changed 'clear' and 'none' arguments to 'all' for --regs, --keystd-mod,
//   --osd, --status and --output options. These use the new +/- prefixes.
// - Added 'all' argument to --modio arguments and use prefixing.
// - Added a bunch of functions to allow the removal of a lot of repetative
//   coding.  These include: set_int_from_list(), set_int_from_arg() and
//   set_float_from_arg().
// - Changes made to --pak argument specification to allow 4K A and B EPROM
//   images to be loaded.
// - Added --edasm option which is an alias for the --pak0 option.
// - Moved --if-* conditional options in options_control() into new function
//   options_conditional().
// - Added new variables UBEE_MODEL and UBEE_RAM.
// - Fixed options_ubee512_envvar_unget(), element size and matching issues.
// - Fixed options_ubee512_compare() function to return a error number that
//   is unlikely to be a legal value.
// - Improvements made to string to integer and float conversions to detect
//   errors and able to auto detect other base types.
// - Improvements made to --help option to provide better information.
// - Removed a 'e_optarg != NULL' conditional, uses static pointer!
// - Various other code changes.
//
// v4.5.0 - 1 April 2010, uBee
// - Added --md5-create option to enable/disable creation of the
//   rom.md5.auto file.
// - Added --bpclr option to clear a debugging break point.
//
// v4.4.0 - 14 August 2009, uBee
// - Added 'pc85b' model to --model option in help section.
// - Fixed --pak and --pakx index bug introduced after enumeration of
//   options.
//
// v4.3.0 - 31 July 2009, uBee
// - Added '2mhzdd' and 'dd' Dreamdisk model parameters to help section.
// - Fixed --hddx-close options, incorrect index was being used.
//
// v4.2.0 - 19 July 2009, uBee
// - Added options --hdd0 to --hdd6 for WD1002-6 Winchester/floppy card.
// - Added options --hdd3-close, --hdd4-close, --hdd5-close, --hdd6-close.
// - Added '+hdd' argument to --modio option.
// - Added --mouse option for emulation of a Microbee mouse.
// - Added '+mouse' argument argument to --status option.
// - Added --bootkey option to pass a key (scan code) to boot ROMs.
// - Added --bpr option for repeatable break point action in debug mode.
// - Added '+pio' argument to --regs option.
// - Changed --verbose option to take optional level argument.
// - Enumerated all options for easier maintenance.
//
// v4.1.0 - 4 July 2009, uBee
// - Added --rom256k override option for 3rd party models.
// - Added --ide-a0, --ide-a1, --ide-b0 and --ide-b1 IDE drive options.
// - Added models 'scf' and 'pcf' (Compact Flash CB Standard/Premium)
// - Added --cfmode option to force start-up mode when emulation CF model.
// - Added --cpu-delay option to determine the method used for delays when
//   controlling emulation speed.
// - Added '+ide' argument to --modio option.
// - Changed +paknet argument to +roms.
// - Fixed options_ubee512_envvar_get() as was incorrectly matching
//   variables that had the same search name as the leading part of the
//   name.
//
// v4.0.0 - 18 May 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Changes made to --version option to also include Z80 emulator.
//
// v3.1.0 - 22 April 2009, uBee
// - Added --verbose option to switch on additional emulator reporting.
// - Removed all occurrences of console_output() function calls.
// - Added 'clear' argument to --regs option.
// - Added --hardware option to disable emulation of various hardware.
// - Changed --tapevol option to now use a percentage value.
// - Added --tapei-det option to set an optional input level detection value.
// - Added --osd option to configure OSD operation.
// - Added --output option to set text output devices for xprintf().
// - Added --century option to change century dates where appropriate.
// - Added --keystd-mod option for setting standard keyboard behaviour flags.
// - The --psec option no longer does anything and is ignored.
// - Added --hwflashr option to set the video hardware flashing rate.
// - Options --mon-fgl-b, --mon-fgl-g and --mon-fgl-r are now obsolete and
//   will be ignored.  A warning message will be reported if any of these
//   are used.
// - Added --mon-bgi-b, --mon-bgi-g, --mon-bgi-r, --mon-fgi-b, --mon-fgi-g,
//   and --mon-fgi-r options for setting the user dual monochrome intensity
//   RGB values.
// - Added --dint option (dual intensity) to replace --hint option.  The
//   latter option will remain but --dint should be used.
// - Changed --hwflash option, takes 'off', 'on', 'v3' and 'v4' as arguments.
// - Fixed debug option parameters for step and trace.
// - Added --runsecs option to set number of seconds to run before exiting.
// - Added colour, amber, green, black, white, and user to the single
//   character --monitor option parameters of c,a,g,b,w and u.
// - Added Windows OSVERSIONINFOEX dwMajorVersion and dwMinorVersion members
//   as variables UBEE_SYS_MAJOR_VAL and UBEE_SYS_MINOR_VAL to mingw build.
// - Changed -f, --fullscreen and -t, --turbo options to now have support
//   for optional on/off arguments.
// - Added --coms-close option to close open serial communications ports.
// - Added --a-close, --b-close, --c-close, and --d-close options.
// - Added --print-close and --printa-close options.
// - Added --tapei-close and --tapeo-close options.
// - Added --options-warn to issue a warning or produce an error when an
//   option is encountered that is not supported in run mode.
// - Fixed sound volume set option(s), was unable to set any level.
// - Removed the options_exit() function as closing open file handled on
//   entry to options_process() now.
// - Changed all printf() calls to use a local xprintf() function.
// - Changed all fflush() calls to use a local xflush() function.
// - Added ability to process options during the running of the emulator.
//   This has resulted in a large number of changes to some options. Options
//   now only set affected values if the parameter is legal.
// - Added optional administration sections 'global-start-runmode' and
//   'global-end-runmode' for configuration files when in run mode.
// - Added --prefix option to specify the installation path prefix.
// - Added --account option to specify an alternative home account location.
// - getopt_long() now uses local xgetopt_long() function.
// - Moved the 'UBEE512' and 'ubee512' home variable setting to ubee512.c
//   Added OpenGL conditional code compilation using USE_OPENGL.
// - Changed --config option to no longer cause the program to terminate if
//   the specified file does not exist.  A message will be output and no
//   config file will be used.
//
// v3.0.0 - 13 October 2008, uBee
// - Re-organised the hugely long options_getopt() function by moving each
//   option into their own respective functions. option_*name*()
// - Added code to correctly handle double quoted command line arguments
//   when using win32 before being processed by getopts_long().
// - Added options_init() function.
// - Added --varset and --varuset option to set/unset built in uBee512
//   variables.
// - Added conditional options --if-egt, --if-elt, --if-eq, --if-gt,
//   --if-lt, --if-negt, --if-nelt, --if-neq, --if-ngt, --if-nlt, --if-nset,
//   --if-set, --if-false, --if-true, --if-system, --if-else, --if-end and
//   --if-cmpmode.
// - Added --echo and --echoq options.
// - Added +video and +options to the --modio option arguments.
// - Added 'win' parameter to the --mouse-wheel and --status options.
// - Added 'gl' OpenGL parameter to the --video-type option.
// - Added gl-aspect-bee, gl-aspect-mon, gl-max, gl-vsync, gl-winpct,
//   gl-winpix, --gl-filter-fs, --gl-filter-max and --gl-filter-win
//   options.
// - Changed gui.fullscreen to video.fullscreen, crtc.video_depth to
//   video.depth, and crtc.video_type to video.type.
// - Changed --snd-freqlow help as is now 20Hz default.
// - Changed help information concerning the --cmd-repeat2 option.
// - Added UBEE_HOST and UBEE_VERSION internal pre-defined variables.
// - Added UBEE_SYS_MAJOR and UBEE_SYS_MINOR internal pre-defined variables.
// - Added options_ubee512_envvar_set() and options_ubee512_envvar_get()
//   functions for setting and retrieving uBee512 variables.
// - Changes to extract_environment_vars() function for uBee512 variables.
// - Added --exit option to allow exiting from start up in a script.
// - Renamed the options_free() function to options_exit() and removed the
//   code used to free the allocated memory as nothing really to be gained.
//
// v2.8.0 - 2 September 2008, uBee
// - Added --mouse-wheel option to associate actions for wheel scroll events.
// - Added --cmd-repeat1 and --cmd-repeat2 options to set command repeat delays.
// - Added +vol argument parameter to --status options.
// - Changed --snd-volume, --vol to now take a percentage integer value.
// - Added --gui-persist option to set persist times in milliseconds for
//   values that appear on the status line.
// - Changed --js-kset option to now also be the default joystick key set.
// - Added --lcons option to set the list start point for --lcon and --lconw.
// - Added --js-clist option to list the commands available for mapping to
//   a joystick.
// - Modified --file-list and --file-list-q options to allow more than one
//   file string to be passed to Z80 applications.
// - Added --lockfix-win32 and --lockfix-x11 options. These allow the LOCK
//   key work around code to be enabled/disbabled for each system type.
// - crtc.early_col_type is now crtc.std_col_type. Edited help documentation
//   to refer to 'standard colour' instead of 'early colour'.
// - Added --video-depth option to select 8, 8gs, 16 and 32 bit video depths.
// - Added --video-type option to select SW and HW video modes.
// - Fixed --col-type option to set the colour type only if emulating a
//   non-alpha+ model.
// - Changed the default colour monitor type to RGBrgb when --col is used.
// - Changed --frate help as Unices is now also 50 FPS.
//
// v2.7.0 - 2 July 2008, uBee
// - Changed rgb-nn-x ordering from bgr to rgb as table has changed.
// - Added --col-type option to select early colour circuit monitor types.
// - Added --lconw option to list section names in config file in wide format.
// - Added --clock-def option to be used by uBee512 API functions.
// - Added --args-error options to disable some argument option errors.
// - Added --exit-check option to allow quick exits with no checking.
// - Added application options --file-app, --file-exec, --file-exit,
//   --file-list, --file-list-q, --file-load, and --file-run.
// - Added +mute argument parameter to --status options.
// - Added --alias-disks and --alias-roms options.
// - options_readline() function moved over to functions.c module and is now
//   file_readline() with file pointer and string size also passed.
// - Added --slashes option to enable/disable conversion of path slashes.
// - Added support for environment variables in configuration files.
// - Added joystick options --js-axis, --js-axisb, and --js-axisl to allow
//   analogue axis movements to return joystick buttons.
// - Changed the way files names are handled. Determining the paths is now
//   handled by the open_file() function in the functions.c module.
// - Added --dstep option to support 48tpi disks in 96tpi DD drives.
// - Added --dstep-hd option to support 48tpi disks in 96tpi HD drives.
// - Added +ubee512 to the --modio option arguments.
// - Added option --z80div to set the number of Z80 blocks executed per Z80
//   frame.
// - Added option --maxcpulag that sets the maximium Z80 CPU lag time allowed.
// - Added options --snd-alg1, --snd-freq=f, --snd-freqadj, --snd-freqlow,
//   --snd-holdoff, --snd-samples, --snd-hq, --snd-mute, and --snd-volume.
// - Added option --config to allow alternative configuration files to be used.
// - Added structures emu_t, crtc_t, func_t, sound_t and moved the associated
//   variables into it.
// - Made changes for the --nodisk option.
// - Added --video option to disable/enable video output on startup.
// - Added function calls to set colours instead of doing direct.
//
// v2.6.0 - 13 May 2008, uBee
// - Many tape, printer and serial variables have been placed into structures
//   and declared in the appropriate modules.
// - Added --status option to cutomize the status line in the title bar.
// - Added --title option to set a title name.
// - Added --spad option to set padding between status information.
// - Added joystick select and enable option --js.
// - Added Microbee joystick options: --js-clear, --js-mbee, --js-up,
//   --js-right, --js-down, --js-left, --js-fire, --js-play1, --js-play2,
//   and --js-spare.
// - Added joystick to keys mapping options: --js-klist --js-kbd, --js-kk,
//   --js-kb, and --js-kset.
// - Added +joystick parameter to --modio options.
// - Added --dclick option to set double click rate for full screen toggle.
// - Added --basram option to use SRAM instead of ROM at 0xA000-0xBFFF.
// - Added --pakram option to allow a PAK location to use SRAM instead of ROM
//   at 0xC000-0xDFFF.
// - Added --netram option to use SRAM instead of ROM at 0xE000-0xEFFF.
// - Moved the string_search() function over to the function.c module.
//
// v2.5.0 - 30 March 2008, uBee
// - Restructured the help output and increased lineswanted to 24 from 23.
// - Added --lmodel option to list the available models.
// - Added Teleterm model emulation.
// - Added user's monochrome configuration type 'u' to the --monitor option.
// - Added --mon-fg-x, --mon-bg-x, and --mon-fgl-x options for a user
//   configurable monochrome monitor.
// - Added --basic, --basica, --basicb,--basicc, --basicd, options to override
//   default BASIC in ROM based models.
// - Added --pak, --pak0 to --pak7, and --netrom options to override PAK and
//   net ROMs in ROM based models.
// - Added --rom1, --rom2, --rom3 options to override ROMs in DRAM models.
// - Added --charrom option to override the default character ROM.
// - Added --sys option.  Used to allow some files to include the system name.
// - Added --lcon option to list section names in the configuration file.
// - Added --colprom option to override early colour internal PROM values.
// - Implement the modelc structure.
// - Added 48 --rgb-nn-x options to custom configure the alpha+ colours.
// - Removed un-necessary information output used by some options.
// - Modified --fullscreen option to now have a toggling action.
// - Moved the options usage help information into this module.
// - Created a new file containing the options processing code for command
//   line and initilisation options.
//==============================================================================

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <SDL.h>

#ifdef USE_LIBDSK
#include <libdsk.h>
#endif

#ifdef MINGW
#include <windows.h>
#include <conio.h>
#else
#include <sys/utsname.h>
#endif
#include <stdint.h>

#include "getopt.h"
#include "ubee512.h"
#include "audio.h"
#include "options.h"
#include "z80.h"
#include "z80api.h"
#include "crtc.h"
#include "vdu.h"
#include "crtc.h"
#include "memmap.h"
#include "keyb.h"
#include "keystd.h"
#include "fdc.h"
#include "hdd.h"
#include "ide.h"
#include "gui.h"
#include "osd.h"
#include "video.h"
#include "roms.h"
#include "pio.h"
#include "tape.h"
#include "tapfile.h"
#include "serial.h"
#include "mouse.h"
#include "printer.h"
#include "joystick.h"
#include "parint.h"
#include "rtc.h"
#include "clock.h"
#include "support.h"
#include "function.h"
#include "z80debug.h"
#include "console.h"
#include "keystd.h"
#include "quickload.h"
#include "sn76489an_core.h"
#include "compumuse.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
static struct option long_options[] =
{
 // Control related
 {"account",        required_argument, 0, OPT_ACCOUNT          + OPT_Z  },
 {"alias-disks",    required_argument, 0, OPT_ALIAS_DISKS      + OPT_RUN},
 {"alias-roms",     required_argument, 0, OPT_ALIAS_ROMS       + OPT_RUN},
 {"args-error",     required_argument, 0, OPT_ARGS_ERROR       + OPT_RUN},
 {"bootkey",        required_argument, 0, OPT_BOOTKEY          + OPT_RUN},
 {"cfmode",         required_argument, 0, OPT_CFMODE           + OPT_Z  },
 {"config",         required_argument, 0, OPT_CONFIG           + OPT_RUN},
 {"cmd-repeat1",    required_argument, 0, OPT_CMD_REPEAT1      + OPT_RUN},
 {"cmd-repeat2",    required_argument, 0, OPT_CMD_REPEAT2      + OPT_RUN},
 {"cpu-delay",      required_argument, 0, OPT_CPU_DELAY        + OPT_RUN},
 {"dclick",         required_argument, 0, OPT_DCLICK           + OPT_RUN},
 {"exit",           required_argument, 0, OPT_EXIT             + OPT_RUN},
 {"exit-check",     required_argument, 0, OPT_EXIT_CHECK       + OPT_RUN},
 {"gui-persist",    required_argument, 0, OPT_GUI_PERSIST      + OPT_RUN},
 {"keystd-mod",     required_argument, 0, OPT_KEYSTD_MOD       + OPT_RUN},
 {"lockfix-win32",  required_argument, 0, OPT_LOCKFIX_WIN32    + OPT_RUN},
 {"lockfix-x11",    required_argument, 0, OPT_LOCKFIX_X11      + OPT_RUN},
 {"md5-create",     required_argument, 0, OPT_MD5_CREATE       + OPT_Z  },
 {"mmode",          no_argument,       0, OPT_MMODE            + OPT_RUN},
 {"mouse-wheel",    required_argument, 0, OPT_MOUSE_WHEEL      + OPT_RUN},
 {"nodisk",         no_argument,       0, OPT_NODISK           + OPT_RUN},
 {"options-warn",   required_argument, 0, OPT_OPTIONS_WARN     + OPT_RUN},
 {"output",         required_argument, 0, OPT_OUTPUT           + OPT_RUN},
 {"powercyc",       no_argument,       0, OPT_POWERCYC         + OPT_RTO},
 {"prefix",         required_argument, 0, OPT_PREFIX           + OPT_Z  },
 {"reset",          no_argument,       0, OPT_RESET            + OPT_RTO},
 {"runsecs",        required_argument, 0, OPT_RUNSECS          + OPT_RUN},
 {"sdl-putenv",     required_argument, 0, OPT_SDL_PUTENV       + OPT_RUN},
 {"slashes",        required_argument, 0, OPT_SLASHES          + OPT_RUN},
 {"spad",           required_argument, 0, OPT_SPAD             + OPT_RUN},
 {"status",         required_argument, 0, OPT_STATUS           + OPT_RUN},
 {"title",          required_argument, 0, OPT_TITLE            + OPT_RUN},
 {"varset",         required_argument, 0, OPT_VARSET           + OPT_RUN},
 {"varuset",        required_argument, 0, OPT_VARUSET          + OPT_RUN},
 {"verbose",        optional_argument, 0, OPT_VERBOSE          + OPT_RUN},

 // Conditional option parsing
 {"if-egt",         required_argument, 0, OPT_IF_EGT           + OPT_RUN},
 {"if-elt",         required_argument, 0, OPT_IF_ELT           + OPT_RUN},
 {"if-eq",          required_argument, 0, OPT_IF_EQ            + OPT_RUN},
 {"if-gt",          required_argument, 0, OPT_IF_GT            + OPT_RUN},
 {"if-lt",          required_argument, 0, OPT_IF_LT            + OPT_RUN},
 {"if-negt",        required_argument, 0, OPT_IF_NEGT          + OPT_RUN},
 {"if-nelt",        required_argument, 0, OPT_IF_NELT          + OPT_RUN},
 {"if-neq",         required_argument, 0, OPT_IF_NEQ           + OPT_RUN},
 {"if-ngt",         required_argument, 0, OPT_IF_NGT           + OPT_RUN},
 {"if-nlt",         required_argument, 0, OPT_IF_NLT           + OPT_RUN},
 {"if-nset",        required_argument, 0, OPT_IF_NSET          + OPT_RUN},
 {"if-set",         required_argument, 0, OPT_IF_SET           + OPT_RUN},
 {"if-system",      required_argument, 0, OPT_IF_SYSTEM        + OPT_RUN},
 {"if-false",       no_argument,       0, OPT_IF_FALSE         + OPT_RUN},
 {"if-true",        no_argument,       0, OPT_IF_TRUE          + OPT_RUN},
 {"if-else",        no_argument,       0, OPT_IF_ELSE          + OPT_RUN},
 {"if-end",         no_argument,       0, OPT_IF_END           + OPT_RUN},
 {"if-cmpmode",     required_argument, 0, OPT_IF_CMPMODE       + OPT_RUN},

 // Debugging tools
 {"bp",             required_argument, 0, OPT_BP               + OPT_RUN},
 {"bpr",            required_argument, 0, OPT_BPR              + OPT_RUN},
 {"bpclr",          required_argument, 0, OPT_BPCLR            + OPT_RUN},
 {"bpc",            required_argument, 0, OPT_BPC              + OPT_RUN},
 {"break",          no_argument,       0, OPT_BREAK            + OPT_RUN},
 {"cont",           no_argument,       0, OPT_CONT             + OPT_RUN},
 {"dasm-lines",     required_argument, 0, OPT_DASM_LINES       + OPT_RUN},

 {"db-bp",          required_argument, 0, OPT_DB_BP            + OPT_RUN},
 {"db-bpr",         required_argument, 0, OPT_DB_BPR           + OPT_RUN},
 {"db-bpclr",       required_argument, 0, OPT_DB_BPCLR         + OPT_RUN},
 {"db-bpos",        required_argument, 0, OPT_DB_BPOS          + OPT_RUN},
 {"db-bpc",         required_argument, 0, OPT_DB_BPC           + OPT_RUN},
 {"db-bp-port",     required_argument, 0, OPT_DB_BP_PORT       + OPT_RUN},
 {"db-bpclr-port",  required_argument, 0, OPT_DB_BPCLR_PORT    + OPT_RUN},
 {"db-bpr-port",    required_argument, 0, OPT_DB_BPR_PORT      + OPT_RUN},
 {"db-bp-rst",      required_argument, 0, OPT_DB_BP_RST        + OPT_RUN},
 {"db-bpclr-rst",   required_argument, 0, OPT_DB_BPCLR_RST     + OPT_RUN},
 {"db-bpr-rst",     required_argument, 0, OPT_DB_BPR_RST       + OPT_RUN},
 {"db-break",       no_argument,       0, OPT_DB_BREAK         + OPT_RUN},

 {"db-bp-mem",      required_argument, 0, OPT_DB_BP_MEM        + OPT_RUN},
 {"db-bpclr-mem",   required_argument, 0, OPT_DB_BPCLR_MEM     + OPT_RUN},
 {"db-bp-meml",     required_argument, 0, OPT_DB_BP_MEML       + OPT_RUN},
 {"db-bpclr-meml",  required_argument, 0, OPT_DB_BPCLR_MEML    + OPT_RUN},

 {"db-cont",        no_argument,       0, OPT_DB_CONT          + OPT_RTO},
 {"db-dasm",        required_argument, 0, OPT_DB_DASM          + OPT_RTO},
 {"db-dasml",       optional_argument, 0, OPT_DB_DASML         + OPT_RTO},
 {"db-dump",        required_argument, 0, OPT_DB_DUMP          + OPT_RTO},
 {"db-dumpb",       required_argument, 0, OPT_DB_DUMPB         + OPT_RTO},
 {"db-dumpl",       optional_argument, 0, OPT_DB_DUMPL         + OPT_RTO},
 {"db-dumplb",      required_argument, 0, OPT_DB_DUMPLB        + OPT_RTO},
 {"db-dumpp",       required_argument, 0, OPT_DB_DUMPP         + OPT_RTO},
 {"db-dumpr",       no_argument,       0, OPT_DB_DUMPR         + OPT_RTO},

 {"db-fillm",       required_argument, 0, OPT_DB_FILLM         + OPT_RTO},
 {"db-fillb",       required_argument, 0, OPT_DB_FILLB         + OPT_RTO},
 {"db-findb",       required_argument, 0, OPT_DB_FINDB         + OPT_RTO},
 {"db-findm",       required_argument, 0, OPT_DB_FINDM         + OPT_RTO},
 {"db-go",          required_argument, 0, OPT_DB_GO            + OPT_RTO},
 {"db-loadb",       required_argument, 0, OPT_DB_LOADB         + OPT_RTO},
 {"db-loadm",       required_argument, 0, OPT_DB_LOADM         + OPT_RTO},
 {"db-move",        required_argument, 0, OPT_DB_MOVE          + OPT_RTO},
 {"db-popm",        no_argument,       0, OPT_DB_POPM          + OPT_RTO},
 {"db-popr",        no_argument,       0, OPT_DB_POPR          + OPT_RTO},
 {"db-portr",       required_argument, 0, OPT_DB_PORTR         + OPT_RTO},
 {"db-portw",       required_argument, 0, OPT_DB_PORTW         + OPT_RTO},
 {"db-pushm",       required_argument, 0, OPT_DB_PUSHM         + OPT_RTO},
 {"db-pushr",       no_argument,       0, OPT_DB_PUSHR         + OPT_RTO},
 {"db-saveb",       required_argument, 0, OPT_DB_SAVEB         + OPT_RTO},
 {"db-savem",       required_argument, 0, OPT_DB_SAVEM         + OPT_RTO},
 {"db-setb",        required_argument, 0, OPT_DB_SETB          + OPT_RTO},
 {"db-setr",        required_argument, 0, OPT_DB_SETR          + OPT_RTO},
 {"db-setm",        required_argument, 0, OPT_DB_SETM          + OPT_RTO},
 {"db-step",        required_argument, 0, OPT_DB_STEP          + OPT_RTO},

 {"db-trace",       required_argument, 0, OPT_DB_TRACE         + OPT_RUN},
 {"db-trace-clr",   no_argument,       0, OPT_DB_TRACE_CLR     + OPT_RUN},

 {"debug",          required_argument, 0, OPT_DEBUG            + OPT_RUN}, // option (-z)
 {"debug-close",    no_argument,       0, OPT_DEBUG_CLOSE      + OPT_RUN},
 {"debug-open",     required_argument, 0, OPT_DEBUG_OPEN       + OPT_RUN},
 {"dump",           required_argument, 0, OPT_DUMP             + OPT_RUN},
 {"dump-header",    required_argument, 0, OPT_DUMP_HEADER      + OPT_RUN},
 {"dump-lines",     required_argument, 0, OPT_DUMP_LINES       + OPT_RUN},
 {"echo",           required_argument, 0, OPT_ECHO             + OPT_RUN},
 {"echoq",          required_argument, 0, OPT_ECHOQ            + OPT_RUN},
 {"find-count",     required_argument, 0, OPT_FIND_COUNT       + OPT_RUN},
 {"modio",          required_argument, 0, OPT_MODIO            + OPT_RUN},
 {"regs",           required_argument, 0, OPT_REGS             + OPT_RUN},

 // Disk drive images
 {"disk-create",    required_argument, 0, OPT_DISK_CREATE      + OPT_RUN},

 {"hdd0",           required_argument, 0, OPT_HDD0             + OPT_Z  }, // 0-2 are HDDs
 {"hdd1",           required_argument, 0, OPT_HDD1             + OPT_Z  },
 {"hdd2",           required_argument, 0, OPT_HDD2             + OPT_Z  },
 {"hdd3",           required_argument, 0, OPT_HDD3             + OPT_RUN}, // 3-6 are floppies
 {"hdd4",           required_argument, 0, OPT_HDD4             + OPT_RUN},
 {"hdd5",           required_argument, 0, OPT_HDD5             + OPT_RUN},
 {"hdd6",           required_argument, 0, OPT_HDD6             + OPT_RUN},

 {"hdd3-close",     no_argument,       0, OPT_HDD3_CLOSE       + OPT_RUN},
 {"hdd4-close",     no_argument,       0, OPT_HDD4_CLOSE       + OPT_RUN},
 {"hdd5-close",     no_argument,       0, OPT_HDD5_CLOSE       + OPT_RUN},
 {"hdd6-close",     no_argument,       0, OPT_HDD6_CLOSE       + OPT_RUN},

 {"ide-a0",         required_argument, 0, OPT_IDE_A0           + OPT_Z  },
 {"ide-a1",         required_argument, 0, OPT_IDE_A1           + OPT_Z  },
 {"ide-b0",         required_argument, 0, OPT_IDE_B0           + OPT_Z  },
 {"ide-b1",         required_argument, 0, OPT_IDE_B1           + OPT_Z  },

 {"image_a",        required_argument, 0, OPT_IMAGE_A          + OPT_RUN}, // option (-a)
 {"image_b",        required_argument, 0, OPT_IMAGE_B          + OPT_RUN}, // option (-b)
 {"image_c",        required_argument, 0, OPT_IMAGE_C          + OPT_RUN}, // option (-c)
 {"image_d",        required_argument, 0, OPT_IMAGE_D          + OPT_RUN}, // option (-d)

 {"a-close",        no_argument,       0, OPT_A_CLOSE          + OPT_RUN},
 {"b-close",        no_argument,       0, OPT_B_CLOSE          + OPT_RUN},
 {"c-close",        no_argument,       0, OPT_C_CLOSE          + OPT_RUN},
 {"d-close",        no_argument,       0, OPT_D_CLOSE          + OPT_RUN},

#ifdef USE_LIBDSK
 {"cpm3",           no_argument,       0, OPT_CPM3             + OPT_RUN},
 {"dstep",          no_argument,       0, OPT_DSTEP            + OPT_RUN},
 {"dstep-hd",       no_argument,       0, OPT_DSTEP_HD         + OPT_RUN},
 {"format",         required_argument, 0, OPT_FORMAT           + OPT_RUN},
 {"lformat",        no_argument,       0, OPT_LFORMAT          + OPT_RUN},
 {"ltype",          no_argument,       0, OPT_LTYPE            + OPT_RUN},
 {"side1as0",       no_argument,       0, OPT_SIDE1AS0         + OPT_RUN},
 {"type",           required_argument, 0, OPT_TYPE             + OPT_RUN},
#endif
 {"psec",           no_argument,       0, OPT_PSEC             + OPT_RUN},

 // Display related
 {"aspect",         required_argument, 0, OPT_ASPECT           + OPT_Z  },
 {"fullscreen",     optional_argument, 0, OPT_FULLSCREEN       + OPT_Z  }, // option (-f)
 {"monitor",        required_argument, 0, OPT_MONITOR          + OPT_RUN}, // option (-m)

 {"mon-bg-b",       required_argument, 0, OPT_MON_BG_B         + OPT_RUN},
 {"mon-bg-g",       required_argument, 0, OPT_MON_BG_G         + OPT_RUN},
 {"mon-bg-r",       required_argument, 0, OPT_MON_BG_R         + OPT_RUN},
 {"mon-bgi-b",      required_argument, 0, OPT_MON_BGI_B        + OPT_RUN},
 {"mon-bgi-g",      required_argument, 0, OPT_MON_BGI_G        + OPT_RUN},
 {"mon-bgi-r",      required_argument, 0, OPT_MON_BGI_R        + OPT_RUN},
 {"mon-fg-b",       required_argument, 0, OPT_MON_FG_B         + OPT_RUN},
 {"mon-fg-g",       required_argument, 0, OPT_MON_FG_G         + OPT_RUN},
 {"mon-fg-r",       required_argument, 0, OPT_MON_FG_R         + OPT_RUN},
 {"mon-fgi-b",      required_argument, 0, OPT_MON_FGI_B        + OPT_RUN},
 {"mon-fgi-g",      required_argument, 0, OPT_MON_FGI_G        + OPT_RUN},
 {"mon-fgi-r",      required_argument, 0, OPT_MON_FGI_R        + OPT_RUN},

 {"mon-fgl-b",      required_argument, 0, OPT_MON_FGL_B        + OPT_RUN}, // these 3 report warnings
 {"mon-fgl-g",      required_argument, 0, OPT_MON_FGL_G        + OPT_RUN},
 {"mon-fgl-r",      required_argument, 0, OPT_MON_FGL_R        + OPT_RUN},

 {"rgb-00-r",       required_argument, 0, OPT_RGB_00_R         + OPT_RUN},
 {"rgb-00-g",       required_argument, 0, OPT_RGB_00_G         + OPT_RUN},
 {"rgb-00-b",       required_argument, 0, OPT_RGB_00_B         + OPT_RUN},
 {"rgb-01-r",       required_argument, 0, OPT_RGB_01_R         + OPT_RUN},
 {"rgb-01-g",       required_argument, 0, OPT_RGB_01_G         + OPT_RUN},
 {"rgb-01-b",       required_argument, 0, OPT_RGB_01_B         + OPT_RUN},
 {"rgb-02-r",       required_argument, 0, OPT_RGB_02_R         + OPT_RUN},
 {"rgb-02-g",       required_argument, 0, OPT_RGB_02_G         + OPT_RUN},
 {"rgb-02-b",       required_argument, 0, OPT_RGB_02_B         + OPT_RUN},
 {"rgb-03-r",       required_argument, 0, OPT_RGB_03_R         + OPT_RUN},
 {"rgb-03-g",       required_argument, 0, OPT_RGB_03_G         + OPT_RUN},
 {"rgb-03-b",       required_argument, 0, OPT_RGB_03_B         + OPT_RUN},
 {"rgb-04-r",       required_argument, 0, OPT_RGB_04_R         + OPT_RUN},
 {"rgb-04-g",       required_argument, 0, OPT_RGB_04_G         + OPT_RUN},
 {"rgb-04-b",       required_argument, 0, OPT_RGB_04_B         + OPT_RUN},
 {"rgb-05-r",       required_argument, 0, OPT_RGB_05_R         + OPT_RUN},
 {"rgb-05-g",       required_argument, 0, OPT_RGB_05_G         + OPT_RUN},
 {"rgb-05-b",       required_argument, 0, OPT_RGB_05_B         + OPT_RUN},
 {"rgb-06-r",       required_argument, 0, OPT_RGB_06_R         + OPT_RUN},
 {"rgb-06-g",       required_argument, 0, OPT_RGB_06_G         + OPT_RUN},
 {"rgb-06-b",       required_argument, 0, OPT_RGB_06_B         + OPT_RUN},
 {"rgb-07-r",       required_argument, 0, OPT_RGB_07_R         + OPT_RUN},
 {"rgb-07-g",       required_argument, 0, OPT_RGB_07_G         + OPT_RUN},
 {"rgb-07-b",       required_argument, 0, OPT_RGB_07_B         + OPT_RUN},
 {"rgb-08-r",       required_argument, 0, OPT_RGB_08_R         + OPT_RUN},
 {"rgb-08-g",       required_argument, 0, OPT_RGB_08_G         + OPT_RUN},
 {"rgb-08-b",       required_argument, 0, OPT_RGB_08_B         + OPT_RUN},
 {"rgb-09-r",       required_argument, 0, OPT_RGB_09_R         + OPT_RUN},
 {"rgb-09-g",       required_argument, 0, OPT_RGB_09_G         + OPT_RUN},
 {"rgb-09-b",       required_argument, 0, OPT_RGB_09_B         + OPT_RUN},
 {"rgb-10-r",       required_argument, 0, OPT_RGB_10_R         + OPT_RUN},
 {"rgb-10-g",       required_argument, 0, OPT_RGB_10_G         + OPT_RUN},
 {"rgb-10-b",       required_argument, 0, OPT_RGB_10_B         + OPT_RUN},
 {"rgb-11-r",       required_argument, 0, OPT_RGB_11_R         + OPT_RUN},
 {"rgb-11-g",       required_argument, 0, OPT_RGB_11_G         + OPT_RUN},
 {"rgb-11-b",       required_argument, 0, OPT_RGB_11_B         + OPT_RUN},
 {"rgb-12-r",       required_argument, 0, OPT_RGB_12_R         + OPT_RUN},
 {"rgb-12-g",       required_argument, 0, OPT_RGB_12_G         + OPT_RUN},
 {"rgb-12-b",       required_argument, 0, OPT_RGB_12_B         + OPT_RUN},
 {"rgb-13-r",       required_argument, 0, OPT_RGB_13_R         + OPT_RUN},
 {"rgb-13-g",       required_argument, 0, OPT_RGB_13_G         + OPT_RUN},
 {"rgb-13-b",       required_argument, 0, OPT_RGB_13_B         + OPT_RUN},
 {"rgb-14-r",       required_argument, 0, OPT_RGB_14_R         + OPT_RUN},
 {"rgb-14-g",       required_argument, 0, OPT_RGB_14_G         + OPT_RUN},
 {"rgb-14-b",       required_argument, 0, OPT_RGB_14_B         + OPT_RUN},
 {"rgb-15-r",       required_argument, 0, OPT_RGB_15_R         + OPT_RUN},
 {"rgb-15-g",       required_argument, 0, OPT_RGB_15_G         + OPT_RUN},
 {"rgb-15-b",       required_argument, 0, OPT_RGB_15_B         + OPT_RUN},

 {"video",          required_argument, 0, OPT_VIDEO            + OPT_RUN},
 {"video-depth",    required_argument, 0, OPT_VIDEO_DEPTH      + OPT_Z  },
 {"video-type",     required_argument, 0, OPT_VIDEO_TYPE       + OPT_Z  },

#ifdef USE_OPENGL
 // OpenGL rendering
 {"gl-aspect-bee",  required_argument, 0, OPT_GL_ASPECT_BEE    + OPT_RUN},
 {"gl-aspect-mon",  required_argument, 0, OPT_GL_ASPECT_MON    + OPT_RUN},
 {"gl-filter-fs",   required_argument, 0, OPT_GL_FILTER_FS     + OPT_RUN},
 {"gl-filter-max",  required_argument, 0, OPT_GL_FILTER_MAX    + OPT_RUN},
 {"gl-filter-win",  required_argument, 0, OPT_GL_FILTER_WIN    + OPT_RUN},
 {"gl-max",         required_argument, 0, OPT_GL_MAX           + OPT_Z  },
 {"gl-vsync",       required_argument, 0, OPT_GL_VSYNC         + OPT_Z  },
 {"gl-winpct",      required_argument, 0, OPT_GL_WINPCT        + OPT_Z  },
 {"gl-winpix",      required_argument, 0, OPT_GL_WINPIX        + OPT_Z  },
#endif

 // Model emulation
 {"basic",          required_argument, 0, OPT_BASIC            + OPT_Z  },
 {"basica",         required_argument, 0, OPT_BASICA           + OPT_Z  },
 {"basicb",         required_argument, 0, OPT_BASICB           + OPT_Z  },
 {"basicc",         required_argument, 0, OPT_BASICC           + OPT_Z  },
 {"basicd",         required_argument, 0, OPT_BASICD           + OPT_Z  },

 {"basram",         no_argument,       0, OPT_BASRAM           + OPT_Z  },
 {"charrom",        required_argument, 0, OPT_CHARROM          + OPT_Z  },
 {"col",            no_argument,       0, OPT_COL              + OPT_RUN},
 {"col-type",       required_argument, 0, OPT_COL_TYPE         + OPT_RUN},
 {"colprom",        required_argument, 0, OPT_COLPROM          + OPT_Z  },
 {"dint",           required_argument, 0, OPT_DINT             + OPT_RUN},
 {"edasm",          required_argument, 0, OPT_PAK0             + OPT_Z  },

 {"hint",           required_argument, 0, OPT_HINT             + OPT_RUN},
 {"hardware",       required_argument, 0, OPT_HARDWARE         + OPT_Z  },
 {"hwflash",        required_argument, 0, OPT_HWFLASH          + OPT_RUN},
 {"hwflashr",       required_argument, 0, OPT_HWFLASHR         + OPT_RUN},
 {"lmodel",         no_argument,       0, OPT_LMODEL           + OPT_RUN},
 {"lpen",           no_argument,       0, OPT_LPEN             + OPT_RUN},
 {"model",          required_argument, 0, OPT_MODEL            + OPT_Z  },
 {"mono",           no_argument,       0, OPT_MONO             + OPT_RUN},
 {"netram",         no_argument,       0, OPT_NETRAM           + OPT_Z  },
 {"netrom",         required_argument, 0, OPT_NETROM           + OPT_Z  },

 {"pak",            required_argument, 0, OPT_PAK0             + OPT_Z  },
 {"pak0",           required_argument, 0, OPT_PAK0             + OPT_Z  },
 {"pak1",           required_argument, 0, OPT_PAK1             + OPT_Z  },
 {"pak2",           required_argument, 0, OPT_PAK2             + OPT_Z  },
 {"pak3",           required_argument, 0, OPT_PAK3             + OPT_Z  },
 {"pak4",           required_argument, 0, OPT_PAK4             + OPT_Z  },
 {"pak5",           required_argument, 0, OPT_PAK5             + OPT_Z  },
 {"pak6",           required_argument, 0, OPT_PAK6             + OPT_Z  },
 {"pak7",           required_argument, 0, OPT_PAK7             + OPT_Z  },

 {"pakram",         required_argument, 0, OPT_PAKRAM           + OPT_Z  },

 {"pcg",            required_argument, 0, OPT_PCG              + OPT_RUN},
 {"piob7",          required_argument, 0, OPT_PIOB7            + OPT_RUN},
 {"port58h",        no_argument,       0, OPT_PORT58H          + OPT_Z  },

 {"rom1",           required_argument, 0, OPT_ROM1             + OPT_Z  },
 {"rom2",           required_argument, 0, OPT_ROM2             + OPT_Z  },
 {"rom3",           required_argument, 0, OPT_ROM3             + OPT_Z  },

 {"rom256k",        required_argument, 0, OPT_ROM256K          + OPT_Z  },
 
 {"sram",           required_argument, 0, OPT_SRAM             + OPT_Z  },
 {"sram-backup",    required_argument, 0, OPT_SRAM_BACKUP      + OPT_Z  },
 {"sram-file",      required_argument, 0, OPT_SRAM_FILE        + OPT_Z  },
 {"sram-load",      required_argument, 0, OPT_SRAM_LOAD        + OPT_Z  },
 {"sram-save",      required_argument, 0, OPT_SRAM_SAVE        + OPT_Z  },

 {"sys",            required_argument, 0, OPT_SYS              + OPT_Z  },
 {"vdu",            required_argument, 0, OPT_VDU              + OPT_RUN},

 // On Screen Display (OSD)
 {"osd",            required_argument, 0, OPT_OSD              + OPT_RUN},
 {"osd-consize",    required_argument, 0, OPT_OSD_CON_SIZE     + OPT_RUN},
 {"osd-conpos",     required_argument, 0, OPT_OSD_CON_POS      + OPT_RUN},
 {"osd-cursor",     required_argument, 0, OPT_OSD_CURSOR_RATE  + OPT_RUN},
 {"osd-list",       no_argument,       0, OPT_OSD_LIST         + OPT_RUN},
 {"osd-scheme",     required_argument, 0, OPT_OSD_SCHEME       + OPT_RUN},
 {"osd-setbtnm",    required_argument, 0, OPT_OSD_SET_BTN_MAIN + OPT_RUN},
 {"osd-setbtnt",    required_argument, 0, OPT_OSD_SET_BTN_TEXT + OPT_RUN},
 {"osd-setdiam",    required_argument, 0, OPT_OSD_SET_DIA_MAIN + OPT_RUN},
 {"osd-setdiat",    required_argument, 0, OPT_OSD_SET_DIA_TEXT + OPT_RUN},
 {"osd-setwidi",    required_argument, 0, OPT_OSD_SET_WID_ICON + OPT_RUN},
 {"osd-setwidm",    required_argument, 0, OPT_OSD_SET_WID_MAIN + OPT_RUN},
 {"osd-setwidt",    required_argument, 0, OPT_OSD_SET_WID_TEXT + OPT_RUN},
 
 // Information output
 {"conio",          no_argument,       0, OPT_CONIO            + OPT_Z  },
 {"help",           no_argument,       0, OPT_HELP             + OPT_RUN}, // option (-h)
 {"lcon",           no_argument,       0, OPT_LCON             + OPT_RUN},
 {"lconw",          no_argument,       0, OPT_LCONW            + OPT_RUN},
 {"lcons",          required_argument, 0, OPT_LCONS            + OPT_RUN},
 {"usage",          no_argument,       0, OPT_USAGE            + OPT_RUN}, // option (-h)
 {"version",        no_argument,       0, OPT_VERSION          + OPT_RUN},

 // Printer emulation
 {"print",          required_argument, 0, OPT_PRINT            + OPT_RUN},
 {"print-close",    no_argument,       0, OPT_PRINT_CLOSE      + OPT_RUN},
 {"printa",         required_argument, 0, OPT_PRINTA           + OPT_RUN},
 {"printa-close",   no_argument,       0, OPT_PRINTA_CLOSE     + OPT_RUN},

 // Parallel port device selection
 {"parallel-port",  required_argument, 0, OPT_PARALLEL_PORT    + OPT_RUN},

 // Parallel port device options
 {"compumuse-init", no_argument,       0, OPT_COMPUMUSE_INIT   + OPT_RUN},
 {"compumuse-clock",required_argument, 0, OPT_COMPUMUSE_CLOCK  + OPT_RUN},

 // Serial port emulation
 {"baud",           required_argument, 0, OPT_BAUD             + OPT_RUN},
 {"baudrx",         required_argument, 0, OPT_BAUDRX           + OPT_RUN},
 {"baudtx",         required_argument, 0, OPT_BAUDTX           + OPT_RUN},
 {"coms",           required_argument, 0, OPT_COMS             + OPT_RUN},
 {"coms-close",     no_argument,       0, OPT_COMS_CLOSE       + OPT_RUN},
 {"datab",          required_argument, 0, OPT_DATAB            + OPT_RUN},
 {"stopb",          required_argument, 0, OPT_STOPB            + OPT_RUN},

 // Sound emulation
 {"sound",          required_argument, 0, OPT_SOUND            + OPT_Z  },
 {"snd-alg1",       required_argument, 0, OPT_SND_ALG1         + OPT_RUN}, // deprecated
 {"snd-freq",       required_argument, 0, OPT_SND_FREQ         + OPT_Z  },
 {"snd-freqadj",    required_argument, 0, OPT_SND_FREQADJ      + OPT_Z  }, // deprecated
 {"snd-freqlow",    required_argument, 0, OPT_SND_FREQLOW      + OPT_Z  }, // deprecated
 {"snd-holdoff",    required_argument, 0, OPT_SND_HOLDOFF      + OPT_RUN}, // deprecated
 {"snd-hq",         no_argument,       0, OPT_SND_HQ           + OPT_Z  },
 {"snd-mute",       required_argument, 0, OPT_SND_MUTE         + OPT_RUN},
 {"snd-samples",    required_argument, 0, OPT_SND_SAMPLES      + OPT_Z  },
 {"snd-volume",     required_argument, 0, OPT_SND_VOLUME       + OPT_RUN}, // option (-v)
 {"vol",            required_argument, 0, OPT_VOL              + OPT_RUN}, // option (-v)

 // Speed related
 {"clock",          required_argument, 0, OPT_CLOCK            + OPT_RUN}, // same as 'xtal'
 {"clock-def",      required_argument, 0, OPT_CLOCK_DEF        + OPT_Z  },
 {"frate",          required_argument, 0, OPT_FRATE            + OPT_RUN},
 {"maxcpulag",      required_argument, 0, OPT_MAXCPULAG        + OPT_RUN},
 {"vblank",         required_argument, 0, OPT_VBLANK           + OPT_RUN},
 {"xtal",           required_argument, 0, OPT_XTAL             + OPT_RUN}, // option (-x)
 {"speedsel",       required_argument, 0, OPT_SPEEDSEL         + OPT_RUN},
 {"turbo",          optional_argument, 0, OPT_TURBO            + OPT_RUN}, // option (-t)
 {"z80div",         required_argument, 0, OPT_Z80DIV           + OPT_RUN},

 // Tape port emulation
 {"tapei",          required_argument, 0, OPT_TAPEI            + OPT_RUN},
 {"tapei-close",    no_argument,       0, OPT_TAPEI_CLOSE      + OPT_RUN},
 {"tapei-det",      required_argument, 0, OPT_TAPE_DET         + OPT_RUN},
 {"tapeo",          required_argument, 0, OPT_TAPEO            + OPT_RUN},
 {"tapeo-close",    no_argument,       0, OPT_TAPEO_CLOSE      + OPT_RUN},
 {"tapesamp",       required_argument, 0, OPT_TAPESAMP         + OPT_RUN},
 {"tapevol",        required_argument, 0, OPT_TAPEVOL          + OPT_RUN},
 {"tapfile-list",   required_argument, 0, OPT_TAPFILE_LIST     + OPT_RUN},
 {"tapfilei",       required_argument, 0, OPT_TAPFILEI         + OPT_RUN},
 {"tapfileo",       required_argument, 0, OPT_TAPFILEO         + OPT_RUN},
 {"tapfilei-close", no_argument,       0, OPT_TAPFILEI_CLOSE   + OPT_RUN},
 {"tapfileo-close", no_argument,       0, OPT_TAPFILEO_CLOSE   + OPT_RUN},

 // Real Time Clock (RTC) emulation and time
 {"century",        required_argument, 0, OPT_CENTURY          + OPT_Z  },
 {"rtc",            required_argument, 0, OPT_RTC              + OPT_Z  },

 // Joystick emulation
 {"js",             required_argument, 0, OPT_JS               + OPT_Z  },

 {"js-axis",        required_argument, 0, OPT_JS_AXIS          + OPT_RUN},
 {"js-axisb",       required_argument, 0, OPT_JS_AXISB         + OPT_RUN},
 {"js-axisl",       required_argument, 0, OPT_JS_AXISL         + OPT_RUN},

 {"js-hat",         required_argument, 0, OPT_JS_HAT           + OPT_RUN},
 {"js-hatb",        required_argument, 0, OPT_JS_HATB          + OPT_RUN},

 {"js-shift",       required_argument, 0, OPT_JS_SHIFT         + OPT_RUN},

 {"js-clear",       no_argument,       0, OPT_JS_CLEAR         + OPT_RUN},
 {"js-mbee",        required_argument, 0, OPT_JS_MBEE          + OPT_RUN},
 {"js-up",          required_argument, 0, OPT_JS_UP            + OPT_RUN},
 {"js-right",       required_argument, 0, OPT_JS_RIGHT         + OPT_RUN},
 {"js-down",        required_argument, 0, OPT_JS_DOWN          + OPT_RUN},
 {"js-left",        required_argument, 0, OPT_JS_LEFT          + OPT_RUN},
 {"js-fire",        required_argument, 0, OPT_JS_FIRE          + OPT_RUN},
 {"js-play1",       required_argument, 0, OPT_JS_PLAY1         + OPT_RUN},
 {"js-play2",       required_argument, 0, OPT_JS_PLAY2         + OPT_RUN},
 {"js-spare",       required_argument, 0, OPT_JS_SPARE         + OPT_RUN},

 {"js-clist",       no_argument,       0, OPT_JS_CLIST         + OPT_RUN},
 {"js-klist",       no_argument,       0, OPT_JS_KLIST         + OPT_RUN},
 {"js-kbd",         required_argument, 0, OPT_JS_KBD           + OPT_RUN},
 {"js-kk",          required_argument, 0, OPT_JS_KK            + OPT_RUN},
 {"js-kb",          required_argument, 0, OPT_JS_KB            + OPT_RUN},
 {"js-kkb",         required_argument, 0, OPT_JS_KKB           + OPT_RUN},
 {"js-kset",        required_argument, 0, OPT_JS_KSET          + OPT_RUN},
 {"js-ksel",        required_argument, 0, OPT_JS_KSEL          + OPT_RUN},

 // Mouse emulation
 {"mouse",          required_argument, 0, OPT_MOUSE            + OPT_RUN},

 // Application dependent
 {"file-app",       required_argument, 0, OPT_FILE_APP         + OPT_RUN},
 {"file-exec",      required_argument, 0, OPT_FILE_EXEC        + OPT_RUN},
 {"file-exit",      required_argument, 0, OPT_FILE_EXIT        + OPT_RUN},
 {"file-list",      required_argument, 0, OPT_FILE_LIST        + OPT_RUN},
 {"file-list-q",    required_argument, 0, OPT_FILE_LIST_Q      + OPT_RUN},
 {"file-load",      required_argument, 0, OPT_FILE_LOAD        + OPT_RUN},
 {"file-run",       required_argument, 0, OPT_FILE_RUN         + OPT_RUN},

 // Quickload support
 {"ql-list",        required_argument, 0, OPT_QL_LIST          + OPT_RUN},
 {"ql-load",        required_argument, 0, OPT_QL_LOAD          + OPT_RTO},
 {"ql-x",           no_argument,       0, OPT_QL_X             + OPT_RTO},
#ifdef USE_ARC
 {"qla-arc",        required_argument, 0, OPT_QLA_ARC          + OPT_RUN},
 {"qla-dir",        required_argument, 0, OPT_QLA_DIR          + OPT_RUN},
 {"qla-list",       required_argument, 0, OPT_QLA_LIST         + OPT_RUN},
 {"qla-load",       required_argument, 0, OPT_QLA_LOAD         + OPT_RTO},
#endif
 {0,                0,                 0,                              0}
};

// table to convert short options to a long option number >= 0x100
static short_options_trans_t short_options[] =
{
 {'a', OPT_IMAGE_A    + OPT_RUN},
 {'b', OPT_IMAGE_B    + OPT_RUN},
 {'c', OPT_IMAGE_C    + OPT_RUN},
 {'d', OPT_IMAGE_D    + OPT_RUN},
 {'f', OPT_FULLSCREEN + OPT_Z  },
 {'h', OPT_HELP       + OPT_RUN},
 {'m', OPT_MONITOR    + OPT_RUN},
 {'t', OPT_TURBO      + OPT_RUN},
 {'v', OPT_SND_VOLUME + OPT_RUN},
 {'x', OPT_CLOCK      + OPT_RUN},
 {'z', OPT_DEBUG      + OPT_RUN},
 {0,                     0x0000}
};

char *monitor_args[] =
{
 "c",
 "a",
 "g",
 "b",
 "w",
 "u",
 "colour",
 "amber",
 "green",
 "black",
 "white",
 "user",
 ""
};

char *offon_args[] =
{
 "off",
 "on",
 ""
};

// xgetopt_long stores the long option index here.
int long_index;

char *c_argv[OPTIONS_SIZE];
int c_argc;

help_t help;

#define TRY_MESG "%s: Try `%s --help' or `%s --usage' for more information.\n"

const SDL_version *sdlv;

static int int_arg;
static float float_arg;

static int if_state_prev;

static char temp_str[512];

static char e_optarg[OPTIONS_PARM_SIZE];
static char e_optarg_q[OPTIONS_PARM_SIZE];
static char e_optarg_x[OPTIONS_PARM_SIZE];

static fdc_drive_t fdc_d;
static hdd_drive_t hdd_d;
static ide_drive_t ide_d;

static int runmode_warn = 1;

#ifdef USE_LIBDSK
static dsk_format_t format;
static dsk_cchar_t fname, fdesc;
static char use_driver_type[40];
static char use_format_type[40];
static char *xstr;
static int side1as0;
static int cpm3;
static int dstep;
static int dstep_hd;
#endif

static int exitstatus;
static int args_err_flags = 0xffffffffL;
static int list_config_start;

static char config_file[SSIZE1];

static char *ndefsv[OPTIONS_SIZE];
static int ndefsc;

static char *xargv[OPTIONS_SIZE];
static int xargc;

static char *emuenv[OPTIONS_ENV_SIZE];
static int emuenvc;

static FILE *fp;

static int if_pos;
static int if_state[OPTIONS_MAXCOND];
static int if_cmp_mode;

#ifdef MINGW
static char win_major_ver[20];
static char win_minor_ver[20];
#else
static struct utsname buf;
#endif

extern char userhome_confpath[];
extern char userhome[];
extern uint8_t col_table_p[16][3];

extern char *model_args[];
extern fdc_drive_t fdc_drive[];
extern ide_drive_t ide_drive[];

extern emu_t emu;
extern memmap_t memmap;
extern model_t model_data[];
extern model_t modelx;
extern model_custom_t modelc;
extern crtc_t crtc;
extern fdc_t fdc;
extern gui_t gui;
extern osd_t osd;
extern video_t video;
extern gui_status_t gui_status;
extern mouse_t mouse;
extern debug_t debug;
extern modio_t modio;
extern regdump_t regdump;
extern func_t func;
extern joystick_t joystick;
extern serial_t serial;
extern audio_t audio;
extern tape_t tape;
extern tapfile_t tapfile;
extern keystd_t keystd;
extern console_t console;
extern compumuse_t compumuse;

extern parint_ops_t printer_ops;
extern parint_ops_t joystick_ops;
extern parint_ops_t beetalker_ops;
extern parint_ops_t beethoven_ops;
extern parint_ops_t dac_ops;
extern parint_ops_t compumuse_ops;

static char *options_malloc (int size);

//==============================================================================
// Options initilisation.
//
// Convert Windows command line arguments to be similar to Unix arguments
// by preserving double quoted arguments.
//
// Set the internal variable values for Unix and Windows systems and some
// other built in variables.
//
//   pass: void
// return: void
//==============================================================================
void options_init (void)
{
 char temp_str[SSIZE1];

#ifdef MINGW
 options_make_pointers(GetCommandLine());

 // get host system version information
 OSVERSIONINFO osvi;

 ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
 osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
 GetVersionEx(&osvi);

 switch (osvi.dwMajorVersion)
    {
     case 3 : strcpy(win_major_ver, "NT3");
              break;
     case 4 : if ((osvi.dwMinorVersion == 0) || (osvi.dwMinorVersion == 10) ||
              (osvi.dwMinorVersion == 90))
                 {
                  strcpy(win_major_ver, "WIN9X_ME");
                  switch (osvi.dwMinorVersion)
                     {
                      case  0 : strcpy(win_minor_ver, "W95");
                                break;
                      case 10 : strcpy(win_minor_ver, "W98");
                                break;
                      case 90 : strcpy(win_minor_ver, "ME");
                                break;
                     }
                  break;
                 }
              if ((osvi.dwMinorVersion == 1) || (osvi.dwMinorVersion == 3))
                 {
                  strcpy(win_major_ver, "NT4");
                  switch (osvi.dwMinorVersion)
                     {
                      case 1 : strcpy(win_minor_ver, "NT4_WS");
                               break;
                      case 3 : strcpy(win_minor_ver, "NT4_SERVER");
                               break;
                     }
                 }
              break;
     case 5 : strcpy(win_major_ver, "NT5");
              switch (osvi.dwMinorVersion)
                 {
                  case 0 : strcpy(win_minor_ver, "W2000");
                           break;
                  case 1 : strcpy(win_minor_ver, "XP");
                           break;
                  case 2 : strcpy(win_minor_ver, "SERVER_2003");
                           break;
                 }
              break;
     case 6 : strcpy(win_major_ver, "NT6");
              switch (osvi.dwMinorVersion)
                 {
                  case 0 : strcpy(win_minor_ver, "VISTA");
                           break;
                  case 1 : strcpy(win_minor_ver, "W7");
                           break;
                  case 2 : strcpy(win_minor_ver, "W8");
                           break;
                  case 3 : strcpy(win_minor_ver, "W8.1");
                           break;
                 }
     case 10 : strcpy(win_major_ver, "NT10");
               switch (osvi.dwMinorVersion)
                  {
                   case 0 : strcpy(win_minor_ver, "W10");
                            break;
                  }
               break;
    }

 options_ubee512_envvar_set("UBEE_HOST=WIN");
 snprintf(temp_str, sizeof(temp_str), "UBEE_SYS_MAJOR=%s", win_major_ver);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "UBEE_SYS_MAJOR_VAL=%d", (int)osvi.dwMajorVersion);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "UBEE_SYS_MINOR=%s", win_minor_ver);
 options_ubee512_envvar_set(temp_str);
 snprintf(temp_str, sizeof(temp_str), "UBEE_SYS_MINOR_VAL=%d", (int)osvi.dwMinorVersion);
 options_ubee512_envvar_set(temp_str);
#else
 uname(&buf);
 strcpy(emu.sysname, buf.sysname);
 toupper_string(emu.sysname, emu.sysname);

 options_ubee512_envvar_set("UBEE_HOST=UNIX");
 options_ubee512_envvar_set("UBEE_SYS_MAJOR=UNIX");
 snprintf(temp_str, sizeof(temp_str), "UBEE_SYS_MINOR=%s", emu.sysname);
 options_ubee512_envvar_set(temp_str);
#endif

 // set some pre-configured local environment variables
 options_ubee512_envvar_set("UBEE_VERSION="APPVER);

 snprintf(temp_str, sizeof(temp_str), "UBEE_MODEL=%s", model_args[emu.model]);
 options_ubee512_envvar_set(temp_str);

 snprintf(temp_str, sizeof(temp_str), "UBEE_RAM=%d", modelx.ram);
 options_ubee512_envvar_set(temp_str);
}

//==============================================================================
// Convert console arguments to pointers for each argument found in 's'.
// Memory is allocated with malloc for each argument found.  Doubled quoted
// arguments are preserved and treated as one argument.  The pointer array
// created will be used by the xgetopts_long() function.  A 'ubee512'
// argument is inserted for the first argument if no arguments were passed.
//
// This function is used for Windows command line as double quoted arguments
// have no special meaning.
//
// This function is also used when using an inbuilt console during the
// running of the emulator.
//
//   pass: char *s
// return: void
//==============================================================================
void options_make_pointers (char *s)
{
 char argx[SSIZE1];
 int i;

 // free existing pointers in use
 while (c_argc)
    free(c_argv[--c_argc]);

 while ((*s) && (c_argc < OPTIONS_SIZE))
    {
     argx[0] = 0;
     while ((*s) && (*s <= ' '))  // move past white space
        s++;

     i = 0;

     while ((*s) && (*s != ' ') && (*s != '"'))
        {
         if (*s)
            {
             argx[i++] = *s;
             s++;
            }
        }

     if (*s == '"')
        {
         s++;
         while ((*s) && (*s != '"'))
            {
             argx[i++] = *s;
             s++;
            }
         s++;
        }

     argx[i] = 0;

     if (argx[0])
        {
         c_argv[c_argc] = options_malloc(i+1);

         if (c_argv[c_argc])
            {
             strcpy(c_argv[c_argc], argx);
             c_argc++;
            }
        }
    }

 if (! c_argc)  // insert an argv[0] value of 'ubee512' if no args
    {
     strcpy(argx, "ubee512");
     c_argv[c_argc] = options_malloc(strlen(argx)+1);
     if (c_argv[c_argc])
        {
         strcpy(c_argv[c_argc], argx);
         c_argc++;
        }
    }

#if 0
 for (i = 0; i < c_argc; i++)
    xprintf("|%s| string len=%d  c_argc=%d\n", c_argv[i], strlen(c_argv[i]), c_argc);
 xprintf("END OF ARGS\n");
#endif
}

//==============================================================================
// Options modio information
//
//   pass: void
// return: void
//==============================================================================
void options_modio_info (void)
{
#ifdef MINGW
 if (modio.options)
    {
     xprintf("options_modio_info: win_major_ver=%s   win_minor_ver=%s\n",
     win_major_ver, win_minor_ver);
     if (modio.level)
        fprintf(modio.log, "options_modio_info: win_major_ver=%s   win_minor_ver=%s\n",
        win_major_ver, win_minor_ver);
    }
#else
 if (modio.options)
    {
     xprintf("options_modio_info: uname.sysname field=%s\n", buf.sysname);
     if (modio.level)
        fprintf(modio.log, "options_modio_info: uname.sysname field=%s\n", buf.sysname);
    }
#endif
}

//==============================================================================
// Options usage
//
// Provide help using a state machine or blocking mode.  The state machine
// will be used by the OSD console dialogue.  Blocking mode will be used when
// help is requested from the command line or when using the blocking ALT+C
// console command.
//
// The help information is arranged in sections, each section and members of
// each section are arranged in alphabetical order.
//
// The last printable character as it appears in this source should be less
// than column position 81.
//
//   pass: help_t help
// return: void
//==============================================================================
void options_usage_state (help_t *help)
{
 char ch;

 char const usage[] =
TITLESTRING"\n"
"\n"
"Usage: ubee512 [options]\n"
"\n"
// +++++++++++++++++++++++++++ Control related +++++++++++++++++++++++++++++++++
" Control related:\n\n"
"  --account=path          Specify an alternative account location. To create\n"
"                          other accounts in the home path on Unices or Windows\n"
"                          use path=@UBEE_USERHOME@\\name.  Accounts may also be\n"
"                          created on removable media. Use this option whenever\n"
"                          an alternative account should be used,  the default\n"
"                          account is @UBEE_USERHOME@/.ubee512 on Unices and\n"
"                          the location of the executed ubee512.exe on Windows\n"
"                          machines. This option if used must be the first\n"
"                          option declared on the command line.\n"
"\n"
"  --alias-disk=x          Enables/disables checking for disk aliases in the\n"
"                          disks.alias file. x=on to enable, x=off to disable.\n"
"                          Default is enabled.\n"
"\n"
"  --alias-roms=x          Enables/disables checking for ROM aliases in the\n"
"                          roms.alias file. x=on to enable, x=off to disable.\n"
"                          Default is enabled.\n"
"\n"
"  --args-error=args       Changes an option error detection flag.\n"
"\n"
"                          This option uses prefixed arguments. See the\n"
"                          'Prefixed arguments' section near the end of this\n"
"                          help for more information.\n"
"\n"
"                          The arguments supported are:\n"
"                          unknown (-+) non-recognised argument error.\n"
"\n"
"  --bootkey=key           Forces a light-pen key scan code on start-up. This\n"
"                          is needed by some ROMs to enter certain operating\n"
"                          modes. The ASCII key value is converted to a scan\n"
"                          code. a-z, and A-Z are converted to codes 1-26 and\n"
"                          ASCII 0-9 to codes 32-41.\n"
"\n"
"  --cfmode=x              Force emulation mode for CF model. x=pc85 for PC85\n"
"                          mode, x=boot for normal boot emulation. Default is\n"
"                          boot mode.\n"
"\n"
"  --config=file           Allows an alternative configuration file to be used\n"
"                          or if file='none' then no configuration file will be\n"
"                          used. This option if used must be the first or\n"
"                          second option declared on the command line. The\n"
"                          default file used for configuration is 'ubee512rc'\n"
"                          and must be located in the ubee512 directory.\n"
"\n"
"  --cmd-repeat1=n         Set the first delay period in milliseconds to be\n"
"                          used for repeated emulator commands. Normally these\n"
"                          are activated with EMUKEY and joystick buttons\n"
"                          mapped to commands. Default value is 500mS.\n"
"  --cmd-repeat2=n         Same as --cmd-repeat1 except this value determines\n"
"                          the delay period to be used after the first period.\n"
"                          Default value is 50mS.\n"
"\n"
"  --cpu-delay=n           Set the delay method used for controlling Z80 CPU\n"
"                          emulation speed. The default value is 0.\n"
"                          The arguments supported are:\n"
"\n"
"                          0 : delays give up processor time.\n"
"                          1 : delays do not give up processor time.\n"
"                          2 : if data is in the sound buffer then use method 1\n"
"                              otherwise method 0 applies.\n"
"\n"
"  --dclick=n              Set the double click speed for mouse button events.\n"
"                          n may be 100-3000 milliseconds, default is 300mS.\n"
"\n"
"  --exit=x                Forces the emulator to exit. This option is intended\n"
"                          to be used inside start up scripts when a condition\n"
"                          is not met. x is the exit status value.\n"
"\n"
"  --exit-check=x          Enables/disables exit checking. if enabled the user\n"
"                          must confirm before exiting the emulator. x=on to\n"
"                          enable, x=off to disable. Default is enabled.\n"
"\n"
"  --gui-persist=n         Set the persist time in milliseconds for values that\n"
"                          appear on the status line, default is 3000mS.\n"
"\n"
"  --keystd-mod=args       Set a standard keyboard behaviour modifier flag.\n"
"                          These flags provide workarounds when emulating the\n"
"                          6545 light pen keys.\n"
"\n"
"                          This option uses prefixed arguments. See the\n"
"                          'Prefixed arguments' section near the end of this\n"
"                          help for more information.\n"
"\n"
"                          The arguments supported are:\n"
"                          all        (+-) all selections.\n"
"                          ctrl_shift (+-) extended function keys emulation.\n"
"\n"
"  --lockfix-win32=x       Enables/disables CapsLock key semi-fix code for Win32.\n"
"                          This option has been provided in case it needs to be\n"
"                          disabled. x=on to enable, x=off to disable. Default\n"
"                          is enabled.\n"
"  --lockfix-x11=x         Enables/disables CapsLock key semi-fix code for x11.\n"
"                          This option has been provided in case it needs to be\n"
"                          enabled (possibly on some x11 set-ups). x=on to\n"
"                          enable, x=off to disable. Default is disabled.\n"
"\n"
"                          Note: The --lockfix-* options will have no affect if\n"
"                          SDL-1.2.14 or later is in use and the CapsLock fix is\n"
"                          enabled within SDL. The fix is enabled by default but\n"
"                          the behaviour may be modified or disabled with an\n"
"                          --sdl-putenv=SDL_DISABLE_LOCK_KEYS=0 option.\n"
"\n"
"  --md5-create=x          Forces the creation of the 'roms.md5.auto' file.\n"
"                          If enabled the ROMs directory is scanned and\n"
"                          MD5s are created for every file found. No directory\n"
"                          recursion is used. x=on to enable, x=off to\n"
"                          disable. Default is disabled.\n"
"\n"
"  --mmode                 Forces the return of the 'M' key once when the\n"
"                          emulator is started. This is used for jumping\n"
"                          directly into the ROM's Monitor mode.\n"
"\n"
"  --mouse-wheel=x         Set the action associated with mouse wheel scrolling\n"
"                          The default is 'vol', the association can also be\n"
"                          changed with the EMUKEY+W hot key.\n"
"\n"
"                          The arguments supported are:\n"
"                          none : no association (does nothing).\n"
"                           vol : adjust application volume level.\n"
"                           win : resize windows display when in OpenGL mode.\n"
"\n"
"  --nodisk                Set no disks flag,  use to start some boot ROMs in\n"
"                          menu mode. This flag will be cleared when any key\n"
"                          is pressed.\n"
"\n"
"  --options-warn=x        Turn warnings on/off for unsupported options\n"
"                          encountered during run mode. If warning is enabled\n"
"                          the offending option is not processed but the\n"
"                          processing of options continues. If warning is\n"
"                          disabled the offending and all remaining options\n"
"                          will not be processed. x=on to enable, x=off to\n"
"                          disable. Default is enabled.\n"
"\n"
"  --output=args           Set output devices for all text output. Default\n"
"                          output is set to 'osd' and 'stdout' on Unices and\n"
"                          'osd' on Windows systems.\n"
"\n"
"                          This option uses prefixed arguments. See the\n"
"                          'Prefixed arguments' section near the end of this\n"
"                          help for more information.\n"
"\n"
"                          The arguments supported are:\n"
"                          all    (+-) all selections.\n"
"                          osd    (+-) output to the emulator's OSD console.\n"
"                          stdout (+-) output to STDOUT or stdout.txt on win32.\n"
"                                      default is (-+) on win32 systems.\n"
"\n"
"  --powercyc              Microbee 'Power Cycle'. (no confirmation checking)\n"
"\n"
"  --prefix=path           Specify an alternative installation location to be\n"
"                          used when creating an account on a Unix system.\n"
"                          Installed files are normally located in /usr/local/\n"
"                          but may be prefixed with 'path'.\n"
"\n"
"  --reset                 Reset z80. (no confirmation checking)\n"
"\n"
"  --runsecs=n             Run the emulator for n seconds then exit. A minimum\n"
"                          value of 5 seconds is allowed. Any disk write\n"
"                          activity will increase the run value until several\n"
"                          seconds of disk write inactivity has passed,  this is\n"
"                          used to reduce the chances of exiting at a critical\n"
"                          moment. Use this option with care. Setting n=0 will\n"
"                          disable this feature and is the default.\n"
"\n"
"  --sdl-putenv=var=value  Sets an SDL environment variable. This can be used\n"
"                          to change the behaviour of SDL. The variables\n"
"                          supported depends on the SDL version and the SDL\n"
"                          documentation should be consulted.\n"
"\n"
"  --slashes=x             Conversion of path slashes to host format. x=on to\n"
"                          enable, x=off to disable. Default is enabled.\n"
"\n"
"  --spad=n                Sets the number of spaces to be placed between each\n"
"                          status entry on the title bar. The actual spacing\n"
"                          achieved will be dependent on the title font used.\n"
"                          Default value is 2 spaces.\n"
"\n"
"  --status=args           Status configuration for title bar.\n"
"\n"
"                          This option uses prefixed arguments. See the\n"
"                          'Prefixed arguments' section near the end of this\n"
"                          help for more information.\n"
"\n"
"                          The arguments supported are:\n"
"                          all    (-+) all selections.\n"
"                          d      (+-) show short drive access.\n"
"                          drive  (-+) show long drive access.\n"
"                          emu    (-+) show emulator name.\n"
"                          emuver (+-) show emulator name and version.\n"
"                          joy    (+-) show joystick status.\n"
"                          left   (-+) force left hand justification.\n"
"                          model  (+-) show base model emulated.\n"
"                          mouse  (+-) show Microbee mouse emulated.\n"
"                          mute   (+-) show the sound mute state.\n"
"                          print  (+-) show parallel printer enable.\n"
"                          ram    (-+) show amount of RAM emulated.\n"
"                          serial (+-) show serial port set up if enabled.\n"
"                          speed  (+-) show CPU clock speed.\n"
"                          sys    (-+) show system name.\n"
"                          tape   (+-) show tape input/output state.\n"
"                          title  (-+) show customised title.\n"
"                          ver    (-+) show emulator version.\n"
"                          vol    (-+) always show volume level.\n"
"                          win    (-+) always show window size.\n"
"\n"
"  --title=name            Define the customised title name to be used when\n"
"                          '+title' is used in the --status option.\n"
"\n"
"  --varset=var[=val]      Set a uBee512 built in environment variable.\n"
"                          var contains the variable name and val is an optional\n"
"                          value to assign to it. i.e. --varset=myvar=myvalue.\n"
"  --varuset=var           Un-set (remove) a uBee512 built in environment\n"
"                          variable. var is the variable name.\n"
"\n"
"  --verbose=level         Switch on additional emulator reporting. The default\n"
"                          setting does not report any messages during start-up\n"
"                          unless error(s) occur. The level value is optional\n"
"                          and defaults to 1 if omitted.\n"
"\n"
// +++++++++++++++++++++++ Conditional processing ++++++++++++++++++++++++++++++
" Conditional processing:\n\n"
"                          If any of the following conditionals returns a true\n"
"                          result then option processing is enabled, a false\n"
"                          result turns processing off until a true condition\n"
"                          is met.\n"
"\n"
"  --if-egt=str1,str2      If str1 is equal to or greater than str2.\n"
"  --if-elt=str1,str2      If str1 is equal to or less than str2.\n"
"  --if-eq=str1,str2       If str1 is equal to str2.\n"
"  --if-gt=str1,str2       If str1 is greater than str2.\n"
"  --if-lt=str1,str2       If str1 is less than str2.\n"
"  --if-negt=str1,str2     If str1 is not equal or greater than str2.\n"
"  --if-nelt=str1,str2     If str1 is not equal or less than str2.\n"
"  --if-neq=str1,str2      If str1 is not equal to str2.\n"
"  --if-ngt=str1,str2      If str1 is not greater than str2.\n"
"  --if-nlt=str1,str2      If str1 is not less than str2.\n"
"  --if-nset=var           If variable var has not been set.\n"
"  --if-set=var            If variable var has been set.\n"
"\n"
"  --if-false              If this is used then set false.\n"
"  --if-true               If this is used then set true.\n"
"\n"
"  --if-system=x           If the host system is equal to system x. On POSIX\n"
"                          systems (Unix) this value is tested against the value\n"
"                          returned by the uname function sysname field. Known\n"
"                          arguments supported are:\n"
"\n"
"                          bsd         : Unix BSD system.\n"
"                          freebsd     : Unix FreeBSD system.\n"
"                          linux       : Unix Linux system.\n"
"                          unix        : Unix system.\n"
"                          win         : Windows system.\n"
"                          win9x_me    : Windows 95, 98 or Me system\n"
"                          w95         : Windows 95 system.\n"
"                          w98         : Windows 98 system.\n"
"                          me          : Windows Millennium system.\n"
"                          nt4         : Windows NT4 systems.\n"
"                          nt4_ws      : Windows NT4 Work station system.\n"
"                          nt4_server  : Windows NT4 Server system.\n"
"                          nt5         : Windows NT5 systems.\n"
"                          w2000       : Windows 2000 system.\n"
"                          xp          : Windows XP system.\n"
"                          server_2003 : Windows NT5 Server system.\n"
"                          nt6         : Windows NT6 systems.\n"
"                          vista       : Windows Vista system.\n"
"                          w7          : Windows 7 system.\n"
"                          w8          : Windows 8 system.\n"
"                          w8.1        : Windows 8.1 system.\n"
"                          w10         : Windows 10 system.\n"
"\n"
"  --if-else               If last conditional resulted in false.\n"
"  --if-end                End of a conditional block.\n"
"  --if-cmpmode=x          Set the method used for comparing values, x=0 uses\n"
"                          'C' style strverscmp() and x=1 uses strcmp(). Default\n"
"                          method is 0.\n"
"\n"
// ++++++++++++++++++++++++++++ Debugging tools ++++++++++++++++++++++++++++++++
" Debugging tools:\n\n"
"  --bp=addr[,addr..]      Set a Z80 PC address break point(s). This option can\n"
"                          be used to set one or more break points separated by\n"
"                          comma characters. The break point is cleared after\n"
"                          detection.\n"
"  --bpclr=addr            Clear a Z80 address break point. 'a' or 'all' may\n"
"                          be specified for 'addr' to clear all break points.'\n"
"  --bpr=addr[,addr..]     Same action as --bp option except the break point is\n"
"                          not cleared after detection.\n"
"\n"
"  --bpc=count             Set a Z80 break point determined by the number of\n"
"                          instructions executed. Can only be specified once.\n"
"                          A break point can only be detected when in debug\n"
"                          mode.\n"
"\n"
"  --dasm-lines=n          Set the number of lines for disassembly. The default\n"
"                          value is 1.\n"
"\n"
"  --db-bp=addr            Alternative option name for --bp.\n"
"  --db-bpclr=addr         Alternative option name for --bpclr.\n"
"  --db-bpr=addr           Alternative option name for --bpr.\n"
"  --db-bpc=count          Alternative option name for --bpc.\n"
"\n"
"  --db-bpos=s,f           Set a break point when the PC is outside of the\n"
"                          address range 's' and 'f' (inclusive). This may be\n"
"                          cleared using a 'c' or 'clr' for 's'. The break\n"
"                          point once triggered must re-enter the address\n"
"                          range before another break can occur.\n"
"\n"
"  --db-bp-port=d,p,n      Set a breakpoint for a read/write on port 'p' with\n"
"                          a matching value 'n'. 'n=*' may be used to match any\n"
"                          value. The port direction 'd', may be 'w' for writes\n"
"                          and 'r' for reads.\n"
"  --db-bpclr-port=d,p     Clear a breakpoint for port 'p', for port direction\n"
"                          'd', where 'd' may be 'w' for writes and 'r' for\n"
"                          reads.\n"
"  --db-bpr-port=d,p,n     Same action as --db-bp-port option except the break\n"
"                          point is not cleared after detection.\n"
"\n"
"  --db-bp-rst=x           Set a Z80 RST n break point. This option can be\n"
"                          specified as many times as is required. A break\n"
"                          point can only be detected when in debug mode. The\n"
"                          break point is cleared after detection. n may be\n"
"                          any RST instruction: 00h, 08h, 10h, etc.\n"
"  --db-bpclr-rst=n        Clear an RST n break point.\n"
"  --db-bpr-rst=n          Same action as --db-bp-rst option except the break\n"
"                          point is not cleared after detection.\n"
"\n"
"  --db-bp-mem=d,s[,f]     Sets a memory read/write breakpoint for the memory\n"
"                          range 's' to 'f' (inclusive). The direction 'd', may\n"
"                          be 'w' for memory writes or 'r' for memory reads.\n"
"  --db-bpclr-mem=d,s[,f]  Clears a memory read/write breakpoint for the memory\n"
"                          range 's' to 'f' (inclusive). The direction 'd', may\n"
"                          be 'w' for memory writes or 'r' for memory reads.\n"
"  --db-bp-meml=d,s,l      Sets a memory read/write breakpoint for the memory\n"
"                          range 's' for 'l' bytes. The direction 'd', may\n"
"                          be 'w' for memory writes or 'r' for memory reads.\n"
"  --db-bpclr-meml=d,s,l   Clears a memory read/write breakpoint for the memory\n"
"                          range 's' for 'l' bytes. The direction 'd', may\n"
"                          be 'w' for memory writes or 'r' for memory reads.\n"
"\n"
"  --db-break, --break     Stop Z80 code execution (enters paused state).\n"
"\n"
"  --db-cont, --cont       Continue Z80 code execution (pause off).\n"
"\n"
"  --db-dasm=s,f           Disassemble Z80 code starting at address 's' and\n"
"                          finishing at 'f'. The code is only disassembled and\n"
"                          is not executed.\n"
"  --db-dasml=[s[,l]]      Disassemble Z80 code starting at address 's' for 'l'\n"
"                          number of lines. If the optional parameters are\n"
"                          omitted the disassembly continues on from the last\n"
"                          address for the current line value as set with the\n"
"                          --dasm-lines option. The code is only disassembled\n"
"                          and is not executed.\n"
"\n"
"  --db-dump=s,f[,h]       Dump memory starting at address 's' and finishing at\n"
"                          'f'. The optional 'h' value determines if a header is\n"
"                          used. A '+h' enables and a '-h' disables the header.\n"
"                          The default header setting is determined by the\n"
"                          --dump-header option if the 'h' value is omitted.\n"
"  --db-dumpb=t,b,s,f[,h]  Dump bank memory type 't', bank 'b', starting at\n"
"                          offset 's' and finishing at 'f'. The 'h' value is the\n"
"                          same as that described for the --db-dump option.\n"
"                          See 'Bank t arguments' section near the end of this\n"
"                          help for more information.\n"
"  --db-dumpl=[s[,l][,h]]  Dump memory starting at address 's' for 'l' number of\n"
"                          lines. If the optional parameters are omitted the\n"
"                          dump continues on from the last address for the\n"
"                          current line value as set with the --dump-lines\n"
"                          option. The 'h' value is the same as that described\n"
"                          for the --db-dump option.\n"
"  --db-dumplb=t,b,s,l[,h] Dump bank memory type 't', bank 'b', starting at\n"
"                          offset 's' for number of lines 'l'. The 'h' value is\n"
"                          the same as that described for the --db-dump option.\n"
"                          See 'Bank t arguments' section near the end of this\n"
"                          help for more information.\n"
"  --db-dumpp=d,p[,p..]    Dump the current Z80 8 bit port 'p' input/output\n"
"                          state values for direction 'd', where 'd=i' for\n"
"                          inputs and 'd=o' for outputs. All 256 ports will be\n"
"                          dumped if 'a' or 'all' is specified for 'p'. This\n"
"                          option will not read or write to the port.\n"
"  --db-dumpr              Dump current value of all Z80 registers using 'all'\n"
"                          output settings.\n"
"\n"
"  --db-fillb=t,b,v        Fill bank memory type 't', bank 'b' using value 'v'.\n"
"                          All banks belonging to type 't' may be filled by\n"
"                          specifying 'a' or 'all' for bank 'b'.\n"
"                          See 'Bank t arguments' section near the end of this\n"
"                          help for more information.\n"
"  --db-fillm=s,f,v        Fill memory with a value. Fill memory starting at\n"
"                          address 's' and finishing at 'f' with value 'v'. This\n"
"                          works on the current Z80 memory map configuration.\n"
"                          Memory destinations and locations will be dependent\n"
"                          on the current port 0x50 setting on DRAM models,\n"
"                          other things like character ROM may also be in the\n"
"                          memory map and needs to be taken into account.\n"
"\n"
"  --db-findb=t,s,f,o,d    Search banked memory type 't', starting with bank\n"
"                          's', finishing at bank 'f' with an initial starting\n"
"                          offset of 'o' in the first bank.  The 'f' value may\n"
"                          be 'a' or 'all' for all remaining banks. The\n"
"                          'bank:offset' values where matches are found will be\n"
"                          displayed. The search criteria is passed in 'd' and\n"
"                          is defined in the --findm option.\n"
"  --db-findm=s,f,d        Search memory starting at address 's' and finishing\n"
"                          at 'f' with the address displayed where a successful\n"
"                          search was located. The search criteria is passed in\n"
"                          'd' which may consist of any of the following values\n"
"                          with each one separated by a ',':\n"
"\n"
"                          a     : Following value is ASCII (next only).\n"
"                          b     : Following values are bytes (default).\n"
"                          c     : As for 'a' but matches any case for\n"
"                                  everything! Avoid searching for integer\n"
"                                  values in the same search if using this.\n"
"                          w     : Following values are words.\n"
"                          byte  : Byte value.\n"
"                          word  : Word value.\n"
"                          ASCII : ASCII characters.\n"
"\n"
"  --db-go=addr            Start executing code at address 'addr'. Emulation\n"
"                          will be switched on if currently in a paused state.\n"
"\n"
"  --db-loadb=t,b,file     Loads bank memory type 't', bank 'b', with data from\n"
"                          a file.  All banks that belong to type 't' will be\n"
"                          loaded if 'a' or 'all' is specified for 'b'.\n"
"                          See 'Bank t arguments' section near the end of this\n"
"                          help for more information.\n"
"  --db-loadm=a,file       Load memory address 'a' with data from a file. Up to\n"
"                          65536 bytes may be loaded, if the value is exceeded\n"
"                          the process terminates without error.\n"
"\n"
"  --db-move=s,d,a         Move (copy) memory from source address 's' to\n"
"                          destination 'd' for amount 'a'.\n"
"\n"
"  --db-popm               Restore state of memory from an earlier --db-pushm\n"
"                          option.\n"
"  --db-popr               Restore state of Z80 registers from an earlier\n"
"                          --db-pushr option.\n"
"\n"
"  --db-portr p[,m]        Read port 'p' and display the value. An optional 'm'\n"
"                          value if specified will be placed onto the MSB of the\n"
"                          port address, if 'm' is omitted 0 will be used.\n"
"  --db-portw=p,v[,v..]    Write value 'v' to port 'p'.\n"
"\n"
"  --db-pushm=s,f          Save state of memory starting from address 's' and\n"
"                          finishing at 'f'. Only one level is allowed.\n"
"  --db-pushr              Save state of Z80 registers. Only one level is\n"
"                          allowed.\n"
"\n"
"  --db-saveb=t,b,file     Saves bank memory type 't', bank 'b', to a file. All\n"
"                          banks that belong to type 't' will be saved if 'a' or\n"
"                          'all' is specified for 'b'.\n"
"                          See 'Bank t arguments' section near the end of this\n"
"                          help for more information.\n"
"  --db-savem=s,f,file     Save memory starting at address 's' and finishing at\n"
"                          'f' to a file.\n"
"\n"
"  --db-setb=t,b,o,v[,v..] Set memory in bank type 't', bank 'b' at offset 'o'\n"
"                          with value(s) 'v'.\n"
"                          See 'Bank t arguments' section near the end of this\n"
"                          help for more information.\n"
"  --db-setm=a,v[,v..]     Set memory locations starting at address 'a' with\n"
"                          value(s) 'v'. The number of 'v' arguments is limited\n"
"                          to the argument size allowed in this build and by the\n"
"                          host system. The address wraps around to 0 when\n"
"                          moving past 0xffff.\n"
"  --db-setr=r,v           Set a Z80 register 'r' with value 'v'. The register\n"
"                          register values supported are:\n"
"                          af, bc, de, hl, ix, iy, pc, sp, a, f, b, c, d, e, h,\n"
"                          l, i, r and alternate registers rr_p and r_p.\n"
"\n"
"  --db-step=lines         Step lines of instructions.  For continuous operation\n"
"                          pass 'c' or 'cont' and to stop pass 's', 'stop' or\n"
"                          '0' for lines.  To step over a CALL instruction, pass\n"
"                          'o' or 'over'.  To step out of the currently CALLed\n"
"                          function, pass 'x' or 'exit'.  Step out runs until\n"
"                          the instruction after the next RET instruction\n"
"                          (excluding nested CALLs).\n"
"\n"
"  --db-trace=s,f          Trace only if PC is between addresses 's' and 'f'\n"
"                          inclusively. Default is trace any PC value.\n"
"  --db-trace-clr          Clear the value set with the --db-trace option.\n"
"\n"
"  -z, --debug=args        Debugging mode options.\n"
"\n"
"                          This option uses prefixed arguments. See the\n"
"                          'Prefixed arguments' section near the end of this\n"
"                          help for more information. The 'off' and 'on'\n"
"                          arguments may also be used without a prefix if a\n"
"                          single argument is supplied.\n"
"\n"
"                          The arguments recognised are:\n"
"                          all     (-+) all output options.\n"
"                          alt     (-+) output the alternate and I, R registers.\n"
"                          count   (-+) use instruction counter in disassembly.\n"
"                          index   (-+) output the index registers.\n"
"                          memr    (+-) output memory pointed to by 16 bit reg.\n"
"                          regs    (+-) output the standard Z80 registers.\n"
"                          off     (+-) disables/enables debugging mode.\n"
"                          on      (-+) enables/disables debugging mode.\n"
"                          piopoll (+-) PIO polling when stepping.\n"
"                          step    (-+) start stepping.\n"
"                          step10  (-+) step * 10.\n"
"                          step20  (-+) step * 20.\n"
"                          trace   (-+) start tracing.\n"
"                          tstates (+-) output Z80 instruction t-states.\n"
"\n"
"  --debug-close           Closes a debugging capture file if open.\n"
"  --debug-open=file       Create a debugging capture file.  This file will\n"
"                          capture the output from all options belonging to the\n"
"                          debugging group when open.  This option will first\n"
"                          close any open file before creating a new file.\n"
"                          This open option will not append and will create a\n"
"                          new file overwriting any file by the same name.\n"
"\n"
"  --dump=addr             Set the initial dump address value when using the\n"
"                          dump commands. addr must be a valid Z80 address from\n"
"                          0 to 0xFFFF. The default address is 0.\n"
"  --dump-lines=n          Set the number of lines for a memory dump. The\n"
"                          default value is 8.\n"
"  --dump-header=x         Enables/disables the dump header. Default is enabled.\n"
"\n"
"  --echo=x                Echo a string to stdout. The string may also contain\n"
"                          an environment variable.\n"
"  --echoq=x               Same as --echo option but echoes a quoted version of\n"
"                          the environment variable if any spaces are found.\n"
"\n"
"  --find-count=n          Set the maximum number of matches possible when using\n"
"                          the --db-find* options. The default is 20.\n"
"\n"
"  --modio=args            Module I/O debugging output.\n"
"\n"
"                          This option uses prefixed arguments. See the\n"
"                          'Prefixed arguments' section near the end of this\n"
"                          help for more information.\n"
"\n"
"                          The arguments supported are:\n"
"                          log     (-+) logs to ubee512_log.txt\n"
"                          raminit (-+) use bank numbers as DRAM init values.\n"
"\n"
"                          These arguments turn on port debugging for modules:\n"
"                          all       (-+) all selections.\n"
"                          beetalker (-+) beetalker module.\n"
"                          beethoven (-+) beethoven module.\n"
"                          clock     (-+) clock speed change.\n"
"                          compumuse (-+) Compumuse module.\n"
"                          crtc      (-+) CRTC access.\n"
"                          dac       (-+) DAC module.\n"
"                          fdc       (-+) Floppy Disk Controller registers.\n"
"                          fdc_wtd   (-+) FDC show the track write data.\n"
"                          fdc_wth   (-+) FDC show the sector header info.\n"
"                          func      (-+) function module.\n"
"                          hdd       (-+) ST506 Hard Disk Drive registers.\n"
"                          ide       (-+) IDE Hard disk drive registers.\n"
"                          joystick  (-+) joystick module.\n"
"                          keystd    (-+) standard keys (6545) module.\n"
"                          keytc     (-+) TC keys (256TC/Teleterm) module.\n"
"                          mem       (-+) memory management module.\n"
"                          options   (-+) options module.\n"
"                          roms      (-+) ROMs module.\n"
"                          pioa      (-+) PIO A data.\n"
"                          piob      (-+) PIO B data.\n"
"                          piocont   (-+) PIO control and interrupts.\n"
"                          rtc       (-+) Real Time Clock.\n"
"                          tapfile   (-+) TAP file module.\n"
"                          ubee512   (-+) application loop.\n"
"                          vdu       (-+) Video Display Unit.\n"
"                          vdumem    (-+) access to VDU memory.\n"
"                          video     (-+) SDL and OpenGL video.\n"
"                          z80       (-+) unhandled Z80 port accesses.\n"
"\n"
"  --regs=args             Register dump. Determines what registers will be\n"
"                          dumped when the EMUKEY+R key is pressed.\n"
"\n"
"                          This option uses prefixed arguments. See the\n"
"                          'Prefixed arguments' section near the end of this\n"
"                          help for more information.\n"
"\n"
"                          The arguments supported are:\n"
"                          all  (-+) all selections.\n"
"                          crtc (-+) CRTC6545 registers.\n"
"                          pio  (-+) PIO registers.\n"
"                          rtc  (-+) RTC registers.\n"
"                          z80  (+-) Z80 registers.\n"
"\n"
// +++++++++++++++++++++++++++++ Disk drives +++++++++++++++++++++++++++++++++++
" Disk drives:\n\n"
"  --disk-create=file      This option will create a disk image using LibDsk\n"
"                          support as first preference or by using the built\n"
"                          in RAW disk image support. To keep the option\n"
"                          simple the last 2 '.ext' parts of the file name are\n"
"                          used to determine the disk format and type.\n"
"                          If the '.type' value is omitted then the disk is\n"
"                          assumed to be a RAW disk image.\n"
"\n"
"                          The file format required is: 'filename.format.type'\n"
"                          Some examples of different types using DS40 as the\n"
"                          format are shown below:\n"
"\n"
"                          raw  : filename.ds40\n"
"                          raw  : filename.ds40.raw\n"
"                          dsk  : filename.ds40.dsk\n"
"                          edsk : filename.ds40.edsk\n"
"\n"
"  --hdd(n)=file           The --hdd(n) options allow emulation of WD1002-5\n"
"                          Winchester and floppy disk controller drives. n=0-2\n"
"                          are hard disk drives and n=3-6 are floppy drives.\n"
"                          file=file path for drive (n).\n"
"\n"
"  --hdd3-close            close WD1002-5 floppy disk (1st)\n"
"  --hdd4-close            close WD1002-5 floppy disk (2nd)\n"
"  --hdd5-close            close WD1002-5 floppy disk (3rd)\n"
"  --hdd6-close            close WD1002-5 floppy disk (4th)\n"
"\n"
"  --ide-a0=file           file path for emulator IDE primary master drive.\n"
"  --ide-a1=file           file path for emulator IDE primary slave drive.\n"
"  --ide-b0=file           file path for emulator IDE secondary master drive.\n"
"  --ide-b1=file           file path for emulator IDE secondary slave drive.\n"
"\n"
"  -a, --image_a=file      file path for emulator floppy drive A\n"
"  -b, --image_b=file      file path for emulator floppy drive B\n"
"  -c, --image_c=file      file path for emulator floppy drive C\n"
"  -d, --image_d=file      file path for emulator floppy drive D\n"
"\n"
"  --a-close               close core board floppy disk A\n"
"  --b-close               close core board floppy disk B\n"
"  --c-close               close core board floppy disk C\n"
"  --d-close               close core board floppy disk D\n"
"\n"
#ifdef USE_LIBDSK
"                          LibDsk usage:\n"
"                          If LibDsk is to be used to access a floppy drive then\n"
"                          file path may be 'A:' or 'B:' for Windows or a device\n"
"                          file for Unices. i.e. /dev/fd0 and /dev/fd1 on Linux.\n"
"\n"
#endif
"                          General usage:\n"
"                          If a drive already has a disk open then the disk is\n"
"                          closed before opening a new one. Do NOT change disks\n"
"                          while the Z80 system is actively working on the\n"
"                          drive or has files open, changing disks requires the\n"
"                          same rules required by a Microbee to be observed.\n"
"                          i.e. type '^C' in CP/M 2.2 after changing disk(s).\n"
"\n"
"                          Dynamically named RAW FDD and HDD images:\n"
"                          A dynamic RAW image is where the file extension is\n"
"                          used to specify the CHS and optionally the sector\n"
"                          size of a RAW disk.  The format is '.hdd-C-H-S' and\n"
"                          '.fdd-C-H-S'. It is assumed they are 512 sector size\n"
"                          unless a size is also specified, i.e. 'C-H-S-128'.\n"
"\n"
"                          See 'File path searching' further on for detailed\n"
"                          information. The default area for disks is:\n"
"\n"
"                          @UBEE512@\\disks\\\n"
"\n"
#ifdef USE_LIBDSK
"  --cpm3                  Used by 'rcpmfs' driver to inform it a CP/M 3 file\n"
"                          system is in use. Default CP/M version is 2. This\n"
"                          must precede each Disk drive for each Disk drive\n"
"                          that has a CP/M 3 file system.\n"
"  --dstep                 Informs LibDsk the next Disk drives option uses\n"
"                          double stepping to support 48tpi DD disks in a 96tpi\n"
"                          DD drive. This option must precede each Disk drive\n"
"                          option when LibDsk is required.\n"
"\n"
"  --dstep-hd              Same use as the --dstep option except this is for\n"
"                          48tpi DD disks in 96tpi HD drives.\n"
"\n"
"  --format=type           Determines the disk format type when using LibDsk.\n"
"                          Using this option will cause the next Disk drives\n"
"                          option to use the LibDsk driver. This option must\n"
"                          precede each Disk drive option when LibDsk is\n"
"                          required. Additional disk formats can be placed into\n"
"                          the local libdskrc file.\n"
"\n"
"                          ds40 and ds80 formats will automatically make use of\n"
"                          the --side1as0 option. Use the ds401 and ds801\n"
"                          formats if this behaviour is not required. i.e. PC\n"
"                          formatted disks)\n"
"\n"
"  --lformat               Lists all the LibDsk built in and additional disk\n"
"                          formats that are available.\n"
"\n"
"  --ltype                 Lists all the LibDsk driver types that are available.\n"
"\n"
"  --side1as0              Informs LibDsk the next Disk drives option uses a\n"
"                          disk that has physical side one sectors containing 0\n"
"                          in the sector headers. The FDC write track emulation\n"
"                          will force the side information in the sector header\n"
"                          to use the physical side value with this option.\n"
"                          This option is no longer required to read and write\n"
"                          disks that have this issue.\n"
"\n"
"  --type=driver           Determines what LibDsk driver will be used for the\n"
"                          next Disk drives option. This option must precede\n"
"                          each Disk drive option when LibDsk is required, if\n"
"                          not then LibDsk will attempt to automatically detect\n"
"                          the driver type to use.\n"
"\n"
#endif
// +++++++++++++++++++++++++++ Display related +++++++++++++++++++++++++++++++++
" Display related:\n\n"
"  --aspect=n              Set the display aspect when using an SDL video mode\n"
"                          for rendering, n=1 is 1:1,  default aspect 2:1 (n=2),\n"
"                          n may be set to 1 or 2. 1:1 scaling may be enforced\n"
"                          for some CRTC6545 display sizes'.\n"
"\n"
"  -f, --fullscreen[=x]    Toggle state of full screen mode, the display\n"
"                          defaults to a window (use EMUKEY+ENTER to toggle).\n"
"                          If 'x' is specified then full screen mode can be set\n"
"                          with x=on or window mode set with x=off.\n"
"\n"
"  -m, --monitor=type      Monitor type, if this option is not specified a\n"
"                          colour monitor is the default when emulating colour\n"
"                          and green if a monochrome model. <type> may be one\n"
"                          of the following:\n"
"\n"
"                          a,  amber : amber monitor.\n"
"                          g,  green : green monitor.\n"
"                          w,  white : white monitor, white foreground on black\n"
"                                      background.\n"
"                          b,  black : black monitor, black foreground on white\n"
"                                      background.\n"
"                          u,   user : user's monochrome configuration.\n"
"                          c, colour : colour monitor.\n"
"\n"
"                          Note: This option by itself does not force the\n"
"                          emulation into a standard model Microbee, it's use\n"
"                          simply determines what monitor type is connected\n"
"                          to the emulated Microbee.\n"
"\n"
"  --mon-bg-x=level        Set the 3 user customised monochrome background\n"
"                          colours. x is the gun colour ('r', 'g', 'b'). The\n"
"                          level value is 0-255.\n"
"  --mon-bgi-x=level       Set the 3 user customised monochrome dual intensity\n"
"                          background colours. x is the gun colour\n"
"                          ('r', 'g', 'b'). The level value is 0-255. This\n"
"                          option is only for the Premium (alpha+) models for\n"
"                          dual intensity monochrome (see --dint).\n"
"  --mon-fg-x=level        Set the 3 user customised monochrome foreground\n"
"                          colours. x is the gun colour ('r', 'g', 'b'). The\n"
"                          level value is 0-255.\n"
"  --mon-fgi-x=level       Set the 3 user customised monochrome dual intensity\n"
"                          foreground colours. x is the gun colour\n"
"                          ('r', 'g', 'b'). The level value is 0-255. This\n"
"                          option is only for the Premium (alpha+) models for\n"
"                          dual intensity monochrome (see --dint).\n"
"\n"
"  --rgb-nn-x=level        48 options to customise the Premium (alpha+) colours.\n"
"                          nn is the colour value (00-15), x is the gun colour\n"
"                          ('r', 'g', 'b'). The level value is 0-255.\n"
"\n"
"  --video=x               Video initial start state. x=on to enable, x=off to\n"
"                          to disable. Default is enabled.\n"
"\n"
"  --video-depth=x         Video depth. Default is 16 bits per pixel. Other\n"
"                          depths may improve or degrade performance, i.e. sound\n"
"                          quality. These values only apply to SDL rendering. x\n"
"                          may be one of the following:\n"
"\n"
"                          8   : 8 bit colour.\n"
"                          8gs : 8 bit grey scale.\n"
"                          16  : 16 bit colour.\n"
"                          32  : 32 bit colour.\n"
"\n"
"  --video-type=type       Video type. The default type used is SDL hw rendering\n"
"                          Other types may improve or degrade performance, i.e.\n"
"                          sound quality. <type> may be one of the following:\n"
"\n"
"                          gl : OpenGL (textured) hardware rendering.\n"
"                          hw : SDL hardware rendering.\n"
"                          sw : SDL software rendering.\n"
"\n"
// +++++++++++++++++++++++++ On Screen Display (OSD) +++++++++++++++++++++++++++
" On Screen Display (OSD):\n\n"
"  --osd=args              OSD configuration.\n"
"\n"
"                          This option uses prefixed arguments. See the\n"
"                          'Prefixed arguments' section near the end of this\n"
"                          help for more information.\n"
"\n"
"                          The arguments supported are:\n"
"                          all     (+-) all selections.\n"
"                          animate (+-) animate OSD window minimising.\n"
"\n"
"  --osd-consize=x,y       Set the console dialogue size. The x,y values may\n"
"                          be passed in 3 ways using any combination of the\n"
"                          following:\n"
"\n"
"                          n   : These values match the X and Y values as used\n"
"                                by the CRTC emulation.\n"
"                          n\%  : A percentage of the CRTC display, values 1-100\n"
"                                percent are permitted.\n"
"                          max : Uses the maximum available area of the CRTC.\n"
"\n"
"  --osd-conpos=x,y        Set the console position. The x,y values may be\n"
"                          passed in 3 ways using any combination of the\n"
"                          following:\n"
"\n"
"                          n      : These values match the X and Y values as\n"
"                                   used by the CRTC emulation.\n"
"                          n\%     : A percentage of the CRTC display, values\n"
"                                   1-100 percent are permitted.\n"
"                          center : Positions the console in the center of the\n"
"                                   display on the x or y axis.\n"
"                          left   : Positions the console to the left of the\n"
"                                   display (for x only).\n"
"                          right  : Positions the console to the right of the\n"
"                                   display (for x only).\n"
"                          top    : Positions the console at the top of the\n"
"                                   display (for y only).\n"
"                          bottom : Positions the console at the bottom of the\n"
"                                   display (for y only).\n"
"\n"
"  --osd-cursor=n          Set the OSD console cursor flash rate and type, the\n"
"                          flash rate is in milliseconds, a value of 0 gives a\n"
"                          solid cursor.\n"
"\n"
"  --osd-list              List all the built in OSD schemes supported.\n"
"\n"
"  --osd-scheme=x          Set an OSD scheme, if not specified the 'default'\n"
"                          scheme is used. Any further options that makes\n"
"                          changes to a scheme will work on the currently\n"
"                          selected scheme. When changing schemes the console\n"
"                          size and position values are inherited from the\n"
"                          current scheme. The --osd-list option can be used\n"
"                          to list what schemes are available. Other values\n"
"                          are as follows:\n"
"\n"
"                          default : Select Default scheme.\n"
"                          user    : Select User scheme.\n"
"                          reset   : Resets the console size and position\n"
"                                    native to the current scheme.\n"
"\n"
" Editing scheme colours:\n"
"\n"
"                          The following options are used to modify colours in\n"
"                          an existing scheme, the operations are applied to\n"
"                          the currently selected scheme only. The OSD scheme\n"
"                          is broken down into 3 main sections consisting of\n"
"                          'buttons', dialogue' and 'widgets' with each one\n"
"                          broken down into further properties of 'main' and\n"
"                          'text'.  Each option parameter consists of 2 or 4\n"
"                          arguments separated with ','. The following table\n"
"                          describes the argument structure:\n"
"\n"
"                          c,c,c,c : BGHL, BGLL, FGHL, FGLL.\n"
"                          c,c     : BG,   FG.\n"
"                          BG      : Background colour (RGB 0x123456).\n"
"                          FG      : Foreground colour (RGB 0x123456).\n"
"                          HL      : Highlight.\n"
"                          LL      : Lowlight.\n"
"                          x       : Do not set this colour (leave as is).\n"
"\n"
"  --osd-setbtnm=c,c,c,c   OSD Button main colours.\n"
"  --osd-setbtnt=c,c,c,c   OSD Button text colours.\n"
"  --osd-setdiam=c,c       OSD Dialogue main colours.\n"
"  --osd-setdiat=c,c       OSD Dialogue text colours.\n"
"  --osd-setwidi=c,c       OSD Widget icon colours.\n"
"  --osd-setwidm=c,c,c,c   OSD Widget main colours. The title box has no\n"
"                          BG highlight value and always uses the LL value.\n"
"  --osd-setwidt=c,c,c,c   OSD Widget text colours. The title box has no\n"
"                          BG highlight value and always uses the LL value.\n"
"\n"
#ifdef USE_OPENGL
// +++++++++++++++++++++++++++++ OpenGL rendering ++++++++++++++++++++++++++++++
" OpenGL rendering:\n\n"
"  --gl-aspect-bee=n       The aspect value you want for the Microbee display.\n"
"                          This default value is 4:3 aspect (1.333) but may be\n"
"                          changed with this option. Don't use this to fix\n"
"                          monitor aspects, use --gl-aspect-mon for that.\n"
"                          Use a floating point value for n. i.e. 4:3 aspect\n"
"                          is 4/3=1.333.\n"
"\n"
"  --gl-aspect-mon=n       This option overrides the monitor aspect ratio worked\n"
"                          out by the emulator. This should not be needed for\n"
"                          LCD monitors running in native resolution. The value\n"
"                          may be required if running a 4:3 CRT monitor with\n"
"                          non 4:3 resolution. Use a floating point value for n.\n"
"                          i.e. 4:3 aspect is 4/3=1.333.\n"
"\n"
"  filter options:         The OpenGL filter settings provide sharp or soft\n"
"                          display rendering. One is provided for each display\n"
"                          mode. The value for the current mode can be toggled\n"
"                          with the EMUKEY+F hot keys. The values allowed are:\n"
"\n"
"                          sharp : sharp display.\n"
"                          soft  : soft display.\n"
"\n"
"  --gl-filter-fs=x        filter setting for full screen mode. (sharp)\n"
"  --gl-filter-max=x       filter setting for maximised window mode. (sharp)\n"
"  --gl-filter-win=x       filter setting for resizable window mode. (soft)\n"
"\n"
"  --gl-max=x              Start up maximised if x=on, if x=off then start up in\n"
"                          a window or full screen mode depending on the use of\n"
"                          --fullscreen option. Default is off. This option is\n"
"                          currently not supported on Windows machines.\n"
"\n"
"  --gl-vsync=x            Vsync: swap buffers every n'th retrace. x=off to\n"
"                          disable, x=on to enable. Default is enabled.\n"
"\n"
"  --gl-winpct=n           The default window size on start up determined by a\n"
"                          percentage value from 5-100\% of the desktop X\n"
"                          resolution. If this option is not used the window\n"
"                          X size is 50\% of the desktop width.\n"
"\n"
"  --gl-winpix=n           The default window size on start up determined by\n"
"                          the number of pixels between 50 and the desktop X\n"
"                          resolution. If this option is not used the window\n"
"                          X size is 50\% of the desktop width.\n"
"\n"
#endif
// ++++++++++++++++++++++++++++++ File related +++++++++++++++++++++++++++++++++
" File related:\n\n"
"                          The file options are for use by the uBee512 support\n"
"                          tools. How these are used is entirely dependent on\n"
"                          the Z80 application accessing these values. See the\n"
"                          TOOLS.TXT file for detailed usage information.\n"
"\n"
"  --file-app=name         String name of up to 255 characters. Default is an\n"
"                          empty string.\n"
"  --file-exec=n           Z80 address 0-65535 (0000-FFFF hex). Default is 0.\n"
"  --file-list=files       Host file path(s) of up to 255 characters. @UBEE512@\n"
"                          variable if used will have double quotation\n"
"                          characters added if any space characters are found.\n"
"                          Default is an empty string. This option may be used\n"
"                          repeatedly to build up an array of strings.\n"
"  --file-list-q=files     Same as --file-list option except the entire string\n"
"                          will have double quotation characters added. No\n"
"                          double quotation characters will be placed around\n"
"                          the @UBEE512@ variable. This option may be used\n"
"                          repeatedly to build up an array of strings.\n"
"  --file-load=addr        Z80 address 0-65535 (0000-FFFF hex). Default is 0.\n"
"  --file-run=name         String name of up to 255 characters. Default is an\n"
"                          empty string.\n"
"  --file-exit=x           Enables/disables the state of the flag value, x=on\n"
"                          to enable, x=off to disable. Default is enabled.\n"
"\n"
// +++++++++++++++++++++++++++ Information output ++++++++++++++++++++++++++++++
" Information output:\n\n"
"  -h, --help, --usage     Display help information on command line usage.\n"
"\n"
"  --conio                 Switches on verbose console output for Windows port.\n"
"                          By default only fatal errors and some option's output\n"
"                          is sent to the console.\n"
"\n"
"  --lcon                  List the [section] names found in the configuration\n"
"                          file.\n"
"  --lconw                 Same as --lcon option except uses a wide format.\n"
"  --lcons=n               Sets the list start point for --lcon and --lconw\n"
"                          options. Default value is 1.\n"
"\n"
"  --version               Obtain the version number of uBee512 and other\n"
"                          components (z80, SDL) being used.\n"
"\n"
// +++++++++++++++++++++++++++ Joystick emulation ++++++++++++++++++++++++++++++
" Joystick emulation:\n\n"

"                          Joystick emulation requires the parallel port device\n"
"                          to be set to 'joystick',  this is not required if the\n"
"                          joystick is being mapped to keys. See --parallel-port\n"
"                          option.\n"
"\n"
"  --js=n                  Joystick number to use, n=0 for first joystick. n=-1\n"
"                          to disable an existing setting.\n"
"\n"
"  --js-axis=x             Joystick axis mapping to buttons. x=on to enable,\n"
"                          x=off to disable. Default is enabled.\n"
"  --js-axisb=n            Joystick axis buttons base number. The axis button\n"
"                          offsets are 0=up, 1=right, 2=down and 3=left. The\n"
"                          base number n is added to the offsets to generate a\n"
"                          button number. n may be any value from 0 to 255.\n"
"                          Default value is 0x80 (128).\n"
"  --js-axisl=n            Determines the thresh hold level for button\n"
"                          detection. n may be any value from 1 to 32767.\n"
"                          Default value is 3200.\n"
"\n"
"  --js-hat=x              Joystick Hat mapping to buttons. x=on to enable,\n"
"                          x=off to disable. Default is enabled.\n"
"  --js-hatb=n             Joystick Hat buttons base number. The Hat button\n"
"                          offsets are 0=up, 1=right, 2=down and 3=left. The\n"
"                          base number n is added to the offsets to generate\n"
"                          a button number. n may be any value from 0 to 255.\n"
"                          Default value is 0x90 (144).\n"
"\n"
"  --js-shift=n            Joystick button to be used as a SHIFT button. n may\n"
"                          0-127 or -1 to disable. When a button is pressed in\n"
"                          conjunction with the SHIFT button the button's value\n"
"                          becomes the sum of 256 plus the button's normal\n"
"                          value. Default value is 0x07 (7).\n"
"\n"
"  --js-clear              Clear all Microbee joystick button settings.\n"
"  --js-mbee=x             Microbee joystick emulation control. x=on to enable,\n"
"                          x=off to disable. Default is enabled.\n"
"  --js-ACTION=n[,n..]     Associate joystick ACTION with button(s) 'n'. The\n"
"                          values for ACTION and the default values are:\n"
"                          up    : 0x80, 0x90.\n"
"                          right : 0x81, 0x91.\n"
"                          down  : 0x82, 0x92.\n"
"                          left  : 0x83, 0x93.\n"
"                          fire  : 0x00, 0x01, 0x0b.\n"
"                          play1 : 0x04, 0x08.\n"
"                          play2 : 0x05, 0x09.\n"
"                          spare : 0x02, 0x03, 0x06.\n"
"\n"
"  --js-clist              List the command names available for joystick mapping.\n"
"  --js-klist              List the key names available for joystick mapping.\n"
"  --js-kbd=x              Joystick to Microbee keys mapping control. x=on to\n"
"                          enable, x=off to disable. Default value is enabled.\n"
"  --js-kb=n               Button n to be associated with last --js-kk=k option.\n"
"                          Values of n=256-511 are processed as shifted buttons.\n"
"                          The --js-kkb option is the preferred method.\n"
"  --js-kk=k               Microbee key to be mapped to a joystick button. See\n"
"                          --js-klist option for more information.\n"
"                          The --js-kkb option is the preferred method.\n"
"  --js-kkb=k,n[,n..]      Replaces --js-kk and --js-kb options. This option can\n"
"                          be used to associate multiple buttons to a single\n"
"                          key. Values of n=256-511 are processed as shifted\n"
"                          buttons. See --js-klist option for more information.\n"
"  --js-ksel=n             Select the joystick key set to use. n may be a value\n"
"                          of (0-255) or alternatively specify n as a character\n"
"                          from A-Z, the letter will be converted to numbers\n"
"                          0-25. Default selection is 0.\n"
"  --js-kset=n             Place the joystick keys into set n and make active.\n"
"                          There are 256 sets of joystick keys (0-255) that may\n"
"                          be used or alternatively specify n as a character from\n"
"                          A-Z, the letter will be converted to numbers 0-25.\n"
"\n"
// ++++++++++++++++++++++++++++ Model emulation ++++++++++++++++++++++++++++++++
" Model emulation:\n\n"
"                          See 'File path searching' further on for detailed\n"
"                          information. The default area for roms is:\n"
"\n"
"                          @UBEE512@\\roms\\\n"
"\n"
"  --basic=file            Used for defining 4, 8, and 16K BASIC ROM parts.\n"
"  --basica=file           Same as --basic option.\n"
"  --basicb=file           Used for 4, 8K and ppc85 ROM part B.\n"
"  --basicc=file           Used for 4K ROM part C.\n"
"  --basicd=file           Used for 4K ROM part D.\n"
"\n"
"                          These --basicx options allows the ROM file image\n"
"                          specified to be used instead of the built in model\n"
"                          defaults for the BASIC ROM(s).\n"
"\n"
"  --basram                The memory locations 0xA000-0xBFFF for ROM based\n"
"                          models defaults to ROM. This option will force this\n"
"                          8K area to SRAM emulation. The contents will contain\n"
"                          a typical SRAM pattern on start up,  it will not be\n"
"                          associated with a ROM image file.\n"
"\n"
"  --charrom=file          Allows the character ROM file image specified to be\n"
"                          used instead of the built in predefined 'charrom.bin'\n"
"                          ROM.\n"
"\n"
"  --col                   Enables colour emulation for standard models. This\n"
"                          option has no affect when emulating a Premium, 256TC\n"
"                          or Teleterm model. Defaults to an RGBrgb digital\n"
"                          monitor type when used.\n"
"\n"
"  --col-type=n            Same as --col option except the monitor type may be\n"
"                          selected. n=0 selects RGB analogue, and n=1 selects\n"
"                          RGBrgb monitor emulation. Default is disabled. This\n"
"                          option has no affect when emulating a Premium, 256TC\n"
"                          or Teleterm model.\n"
"\n"
"  --colprom=file          Use the file values to override the internal 82s123\n"
"                          IC 7 standard colour values. This option has no\n"
"                          affect when emulating a Premium, 256TC or Teleterm\n"
"                          model.\n"
"\n"
"  --dint=x, --hint=x      Dual intensity monochrome emulation for Premium\n"
"                          (alpha+) models. x=on to enable, x=off to disable.\n"
"                          This is set to 'on' by default for the 256TC and\n"
"                          upgraded Premium models. This option has no\n"
"                          affect when emulating a standard model. The --hint\n"
"                          option name should not be used any more.\n"
"\n"
"  --edasm=[r,]file        This option is identical to the --pak0 option. See\n"
"                          the --pak0 option for more information.\n"
"\n"
"  --hardware=x            Enable/Disable emulation of various hardware\n"
"                          sections allowing variations in models.\n"
"\n"
"                          This option uses prefixed arguments. See the\n"
"                          'Prefixed arguments' section near the end of this\n"
"                          help for more information.\n"
"\n"
"                          The arguments supported are:\n"
"                          wd2793      (+-) WD2793 FDC emulation.\n"
"                          sn76489     (-+) SN76489 sound IC emulation\n"
"                                           (premium only).\n"
"                          sn76489init (-+) SN76489 sound IC emulation with all\n"
"                                           voices initially silenced\n"
"                                           (premium only).\n"
"\n"
"  --hwflash=x             Hardware inverse and flashing video emulation for\n"
"                          Premium (alpha+) models using one of two possible\n"
"                          methods. x=on to enable (v4), x=off to disable, x=v3\n"
"                          enables using Premium version 3 main board flashing.\n"
"                          x=v4 may also be used instead of x=on for Premium\n"
"                          version 4 main board, 256TC and Teleterm models.\n"
"                          This is set to 'on' (v4) by default for the 256TC\n"
"                          and upgraded Premium models. This option has no\n"
"                          affect when emulating a standard model.\n"
"\n"
"  --hwflashr=x            Set the video hardware flashing rate for Premium\n"
"                          (alpha+) models, the true rate is determined by\n"
"                          CRTC 6545 values (VSYNC). The default value is 320\n"
"                          milliseconds. x may be one of the following timer\n"
"                          values: 20, 40, 80, 160, 320, 640, 1280 or 2560\n"
"                          milliseconds. Alternatively a link setting may be\n"
"                          used, v3 and v4 boards differ for W63 and W64:\n"
"\n"
"                          Link  v3    v4   (milliseconds)\n"
"                          w61   160   160\n"
"                          w62   320   320\n"
"                          w63   1280  640\n"
"                          w64   640   1280\n"
"\n"
"  --lmodel                List the available model types.\n"
"\n"
"  --lpen                  Enables the use of the 6545 Light pen keys emulation.\n"
"                          This is enabled by default for all models except for\n"
"                          the 256tc and Teleterm models.\n"
"\n"
"  --model=type            Model type, if this option is not specified the p512k\n"
"                          model is emulated. The 'p' denotes a Premium\n"
"                          variation in a model. This option should be used\n"
"                          before any other options.\n"
"\n"
"                          The following models are supported:\n"
"                          256tc  : Telecomputer 256k DRAM with FDC.\n"
"                          p1024k : Premium 1024k (Premium plus).\n"
"                          1024k  : Standard 1024k (Premium plus).\n"
"                          p512k  : Premium 512k (PJB upgrade of 128k).\n"
"                          512k   : Standard 512k (PJB upgrade of 128k).\n"
"                          p256k  : Premium 256k (PJB upgrade of 64k).\n"
"                          256k   : Standard 256k (PJB upgrade of 64k).\n"
"                          p128k  : Premium 128k DRAM with FDC.\n"
"                          128k   : Standard 128k DRAM with FDC.\n"
"                          p64k   : Premium 64k DRAM with FDC.\n"
"                          64k    : Standard 64k DRAM with FDC.\n"
"                          56k    : 56k APC (50W expansion for FDC).\n"
"                          tterm  : Teleterm (ROM).\n"
"                          ppc85  : Premium Personal Communicator 85 (ROM).\n"
"                          pc85b  : Personal Communicator 85 (ROM) later.\n"
"                          pc85   : Personal Communicator 85 (ROM).\n"
"                          pc     : Personal Communicator (ROM).\n"
"                          ic     : First 3.375 MHz CPU clock (ROM).\n"
"                          2mhz   : Original 2 MHz kit and first units.\n"
"                          2mhzdd : Dreamdisk @ 2 MHz CPU clock.\n"
"                          dd     : Dreamdisk @ 3.375 MHz CPU clock.\n"
"                          scf    : Standard Compact Flash (CF) core board.\n"
"                          pcf    : Premium Compact Flash (CF) core board.\n"
"\n"
"  --mono                  Disables colour circuit emulation for standard models.\n"
"                          This option when emulating a Premium, 256TC or\n"
"                          Teleterm model has the same affect as --monitor=g.\n"
"\n"
"  --netram                The memory locations 0xE000-0xEFFF for ROM based\n"
"                          models defaults to ROM. This option will force this\n"
"                          4K area to SRAM emulation. The contents will contain\n"
"                          a typical SRAM pattern on start up,  it will not be\n"
"                          associated with a ROM image file.\n"
"\n"
"  --netrom=file           Allows the ROM file image specified to be used\n"
"                          instead of the built in model defaults for the NET\n"
"                          ROM.\n"
"\n"
"  --pak(n)=[r,]file       The --pak(n) options allows the ROM file image\n"
"                          specified to be used instead of the built in model\n"
"                          default for the PAKn ROM 0 to 7 locations.\n"
"                          4K ROM images can be specified by using the optional\n"
"                          'r,' argument. 'r' may be ROM 'a' or 'b'. The 'b'\n"
"                          ROM is only loaded if the 'a' ROM is 4K in size.\n"
"\n"
"  --pakram=n              The PAK n location will use SRAM instead of ROM\n"
"                          emulation. The contents will contain a typical SRAM\n"
"                          pattern on start up,  it will not be associated with\n"
"                          a ROM image file. n may be any PAK number from 0-7.\n"
"\n"
"  --pcg=n                 Premium (alpha+) model option that sets size of PCG\n"
"                          RAM to be emulated. n is the size of the PCG memory\n"
"                          in Kilobytes and can be any even value between 2 and\n"
"                          32. 256TC, Premium and Teleterm models are 16K,\n"
"                          upgraded Premium models are 32K. This option has no\n"
"                          affect when emulating a standard model.\n"
"\n"
"  --piob7=signal          Determines what signal is used for PIO port B bit 7.\n"
"                          The default value depends on the model emulated.\n"
"                          The source values are: rtc, vsync, net, and pup.\n"
"\n"
"  --port58h               Enables 3rd party WD1002-5/WD2793 (port 0x58) support\n"
"                          to allow selecting the required drive interface.\n"
"\n"
"  --rom1=file             Allows the ROM file image specified to be used\n"
"                          instead of the built in model default for the boot\n"
"                          ROM. This ROM is only used by all FDC models.\n"
"  --rom2=file             Allows the ROM file image specified to be used\n"
"                          instead of the built in model default for ROM2. This\n"
"                          ROM is used by all DRAM FDC models, except for the\n"
"                          256TC.\n"
"  --rom3=file             Allows the ROM file image specified to be used\n"
"                          instead of the built in model default for ROM3. This\n"
"                          ROM is used by all DRAM FDC models, except for the\n"
"                          256TC.\n"
"\n"
"  --rom256k=file          Allows the 256K ROM file image specified to be used\n"
"                          instead of the built in model default for the 256K\n"
"                          ROM. This ROM is used by some 3rd party designs.\n"
"                          Set file to 'none' to disable the 256K ROM image.\n"
"\n"
"  --sram=n                ROM models option that sets size of static RAM to be\n"
"                          emulated. n is the size of the SRAM memory in\n"
"                          Kilobytes and can be any value between 0 and 32.\n"
"                          Default value is 32.\n"
"  --sram-backup=x         Enables (x=on) or disables (x=off) CMOS battery\n"
"                          backup emulation for CMOS RAM. Default is enabled.\n"
"  --sram-file=file        Use this file name instead of the default model\n"
"                          'model.ram' file name for the CMOS battery backup\n"
"                          emulation.\n"
"  --sram-load=x           Enables (x=on) or disables (x=off) CMOS battery\n"
"                          backup emulation file loading on start-up for ROM\n"
"                          based and 56k models, Default is enabled.\n"
"  --sram-save=x           Enables (x=on) or disables (x=off) CMOS battery\n"
"                          backup emulation saving when exiting the emulator\n"
"                          for ROM based and 56k models, Default is enabled.\n"
"\n"
"  --sys=name              Defines a system name,  this will be appended to some\n"
"                          files so that different operating systems using the\n"
"                          same model can still have unique names for certain\n"
"                          files. By default this name contains nothing. An\n"
"                          example is the loading and saving of RTC values. By\n"
"                          default for a p128k model this would be 'p128k.rtc',\n"
"                          by defining name to be 'mysys' would then use\n"
"                          'p128k-mysys.rtc' for the file, the emulator inserts\n"
"                          the hyphen character.\n"
"\n"
"  --vdu=n                 Premium (alpha+) model option that sets size of VDU\n"
"                          RAM to be emulated. n may be 2 or 8. The VDU RAM\n"
"                          size determines the number of screen, attribute and\n"
"                          colour RAM banks. Default is 2K. Don't use this\n"
"                          option to increase to 8K unless you understand the\n"
"                          problems associated with it.\n"
"\n"
// +++++++++++++++++++++++ Microbee mouse emulation ++++++++++++++++++++++++++++
" Microbee mouse emulation:\n\n"
"  --mouse=x               Microbee mouse emulation. x=on to enable Microbee\n"
"                          mouse emulation on start-up. Default is 'off'.\n"
"\n"
// ++++++++++++++++++++++ Parallel printer emulation +++++++++++++++++++++++++++
" Parallel printer emulation:\n\n"
"                          Parallel printer emulation requires the parallel port\n"
"                          device to be set to 'printer'. See --parallel-port\n"
"                          option.\n"
"\n"
"                          See 'File path searching' further on for detailed\n"
"                          information. The default area for printer is:\n"
"\n"
"                          @UBEE512@\\printer\\\n"
"\n"
"  --print=file            Printer output to file,  the output is not modified.\n"
"                          If an open printer file is already in use then that\n"
"                          file will be closed first before creating the new\n"
"                          printer file.\n"
"  --print-close           Closes a currently open printer file. This allows the\n"
"                          file to be accessed externally without exiting the\n"
"                          emulator.\n"
"\n"
"  --printa=file           Printer output to file,  the output is converted to\n"
"                          ASCII decimal. If an open printer file is already in\n"
"                          use then that file will be closed first before\n"
"                          creating the new printer file.\n"
"  --printa-close          Closes a currently open ASCII printer file. This\n"
"                          allows the file to be accessed externally without\n"
"                          exiting the emulator.\n"
"\n"
// ++++++++++++++++++++ Parallel port device selection +++++++++++++++++++++++++
" Parallel port device selection:\n\n"
"  --parallel-port=device  Select the external parallel port peripheral device\n"
"                          to be emulated on PIO port A.  The default device is\n"
"                          'printer'. The following devices are supported:\n"
"\n"
"                          none      : no device.\n"
"                          printer   : parallel printer.\n"
"                          joystick  : joystick. (not required for mapped keys)\n"
"                          beetalker : Microbee BeeTalker speech emulation.\n"
"                          beethoven : Microbee BeeThoven sound synthesiser.\n"
"                          dac       : 8 bit audio DAC.\n"
"                          compumuse : EA Compumuse sound synthesiser.\n"
"\n"
// ++++++++++++++++++++ Compumuse device option selection ++++++++++++++++++++++
" Compumuse options:\n\n"

"                          Compumuse emulation requires that the parallel port\n"
"                          device be set to 'compumuse'. See the\n"
"                          --parallel-port option.\n"
"\n"
"  --compumuse-init        Silences the emulated sn76489 when the Compumuse is\n"
"                          initialised.\n"
"  --compumuse-clock=n     Sets the Compumuse's clock frequency, in Megahertz.\n"
"                          Valid values for n are 1, 2, 4. Default value is 2.\n"
"\n"
// +++++++++++++++++++++++++++ Quickload support +++++++++++++++++++++++++++++++
" Quickload support:\n\n"
"  --ql-list=file          List description contained in a quickload file.\n"
"  --ql-load=file[,x]      Load a quickload file, an optional 'x' will cause\n"
"                          the code to be executed once loaded.\n"
"  --ql-x                  Execute the quickload file in memory.\n"
#ifdef USE_ARC
"\n"
"  --qla-arc=file          Specify a quickload archive file to be used for\n"
"                          further operations. Only ZIP archives are currently\n"
"                          supported. Any archive currently open will be closed\n"
"                          first.\n"
"  --qla-dir=file|*[,+v]   The entire archive directory will be listed if '*'\n"
"                          is specified for file or a single file within the\n"
"                          archive matching 'file' may be specified.  An\n"
"                          optional verbose argument of '+v' may be specified\n"
"                          for more information.\n"
"  --qla-list=file|*       The entire archive directory will be listed if '*' is\n"
"                          specified for file or a single file within the\n"
"                          archive matching 'file' may be specified.\n"
"  --qla-load=file[,x]     Load file from the current quickload archive, an\n"
"                          optional 'x' will cause the code to be executed\n"
"                          once loaded.\n"
#endif
"\n"
// ++++++++++++++++ Real Time Clock (RTC) emulation and time +++++++++++++++++++
" Real Time Clock (RTC) emulation and time:\n\n"
"  --century=n             This value can be used to correct the century date\n"
"                          used in ROMs. Changes are temporary and are made to\n"
"                          the image(s) in memory only. An MD5 value will be\n"
"                          used to identify what loaded ROM images need\n"
"                          modifying. The value of n is expected to be in BCD\n"
"                          format. The century '20' should use n=0x20 (hex) or\n"
"                          n=32 (dec). The following ROMs with matching MD5s\n"
"                          will be modified in memory:\n"
"\n"
"                          256TC v1.15: md5=13ddba203bd0b8228f748111421bad5f\n"
"                          256TC v1.20: md5=24d6682ff7603655b0cbf77be6731fb0\n"
"                          256TC v1.31: md5=4170a8bb9495aa189afb986c1d0424a4\n"
"\n"
"  --rtc=n                 Real Time Clock (RTC) emulation. n=1 to enable, n=0\n"
"                          to disable. The following models use RTC by\n"
"                          default: 256tc, p1024k, 1024k, p512k, 512k, p256k,\n"
"                          256k and tterm.\n"
"\n"
// +++++++++++++++++++++++++ Serial port emulation +++++++++++++++++++++++++++++
" Serial port emulation:\n\n"
"  --baud=rate             Set serial communications baud rate for both TX and\n"
"                          RX. A value from 1 to 38400 is allowed. Default\n"
"                          rate is 300 baud. If Individual baud rates are\n"
"                          required for TX and RX then use the --baudtx and\n"
"                          --baudrx options instead. This value must match the\n"
"                          Microbee serial application's value.\n"
"\n"
"  --baudtx=rate           Set serial communications baud rate for TX only. A\n"
"                          value from 1 to 38400 is allowed. Default rate is\n"
"                          300 baud. This value must match the Microbee serial\n"
"                          application's value.\n"
"\n"
"  --baudrx=rate           Set serial communications baud rate for RX only. A\n"
"                          value from 1 to 38400 is allowed. Default rate is\n"
"                          300 baud. This value must match the Microbee serial\n"
"                          application's value.\n"
"\n"
"  --coms=port             Serial communications port for emulation of RS232.\n"
"                          On Unices specify a device, on Windows specify the\n"
"                          com port number. No serial communications will be\n"
"                          emulated if this option is not specified. If a\n"
"                          serial port is already open then that port will be\n"
"                          closed first before opening a new serial port.\n"
"\n"
"  --coms-close            Closes the RS232 serial port if currently open.\n"
"\n"
"  --datab=bits            Set serial communications number of data bits. A\n"
"                          value from 5 to 8 is allowed. Default value is 8\n"
"                          data bits. This value must match the Microbee\n"
"                          serial application's value.\n"
"\n"
"  --stopb=bits            Set serial communications number of stop bits. A\n"
"                          value from 1 to 2 is allowed. Default value is 1\n"
"                          data bits. This value must match the Microbee\n"
"                          serial application's value for TX.\n"
"\n"
// +++++++++++++++++++++++++++ Sound emulation +++++++++++++++++++++++++++++++++
" Sound emulation:\n\n"
"  --sound=method          Determine the method used for sound,  the default\n"
"                          method is 'prop':\n"
"\n"
"                          off    : sound is turned off\n"
"                          prop   : sound is proportional to CPU clock frequency\n"
"                          normal : sound rate forced as if 3.375 MHz CPU clock\n"
"\n"
"  --snd-freq=f            Set the sound sampling rate, f may be a value from\n"
"                          5512 to 176400 Hz. Default frequency is 44100 Hz.\n"
"  --snd-hq                Sets high quality sound. How well this works will be\n"
"                          dependent on the host platform. This option has the\n"
"                          same effect as setting all these values:\n"
"                            --snd-samples=2048.\n"
"                            --snd-freq=88200.\n"
"  --snd-mute=x            Sound mute, use to start the emulator with the sound\n"
"                          muted until enabled. x=on to enable, x=off to\n"
"                          disable. Default is off.\n"
"  --snd-samples=n         Sets the SDL callback data size. n must be a power of\n"
"                          2. Values from 1 to 32768 are allowed. Default is\n"
"                          1024 samples.\n"
"  --snd-volume=l --vol=l  Set the sound volume level. A level of 0 to 100\% is\n"
"                          allowed. Default is 45\%.\n"
"\n"
// ++++++++++++++++++++++++++++ Speed related ++++++++++++++++++++++++++++++++++
" Speed related:\n\n"
"  --clock=f               Set the Z80 clock frequency for emulation in MHz.\n"
"                          Standard emulation frequencies are 3.375 and 2.0\n"
"                          MHz. All other frequencies are classed as 'hacking'.\n"
"                          3.375 MHz is the default clock frequency. Frequency\n"
"                          is entered as a floating point number.\n"
"\n"
"                          The highest frequency possible is determined by the\n"
"                          host platform. As the value increases to the point\n"
"                          where uBee512 can no longer regulate the Z80\n"
"                          execution rate the frame rate will decrease (slower\n"
"                          screen update periods).\n"
"\n"
"  --clock-def=f           Set the Z80 clock frequency for emulation in MHz when\n"
"                          the uBee512 API restore function is called. Default\n"
"                          frequency is 3.375 MHz.\n"
"\n"
"  --frate=fps             Frame rate, an integer value between 1 and 1,000,000\n"
"                          is allowed. Default is 50 FPS.\n"
"\n"
"  --maxcpulag=n           This is the maximum time the Z80 CPU emulation is\n"
"                          allowed to get behind before 'catch up' is bypassed\n"
"                          for the currently lagged cycles. A very high value\n"
"                          for n will cause the 'catch up' mode to always be in\n"
"                          affect,  using a value of 0 for n will effectively\n"
"                          disable this feature and act like 2.5.0 and earlier\n"
"                          versions. Default value is 250ms.\n"
"\n"
"  --speedsel=n            CPU speed selection emulation. n=1 to enable, n=0\n"
"                          to disable. The following models are enabled by\n"
"                          default: 256tc, p512k, 512k, p256k, 256k.\n"
"\n"
"  -t, --turbo[=x]         Turbo mode, executes Z80 code as fast as possible.\n"
"                          Without this option the emulation attempts to keep\n"
"                          Z80 CPU execution to match the CPU clock value.\n"
"                          If 'x' is specified then turbo mode can be set with\n"
"                          x=on or off with x=off. This option is intended for\n"
"                          'hacking' and code development use. There are much\n"
"                          faster methods if more speed is required. (see the\n"
"                          README file)\n"
"\n"
"  --vblank=method         Vertical blanking method to be employed. This is\n"
"                          only intended for 'hacking' when experimenting with\n"
"                          turbo mode and/or high CPU clock speeds. It is not\n"
"                          required or even recommended to be used if 3.375 MHz\n"
"                          or 2 MHz emulation is desired.\n"
"\n"
"                          0 : 50 Hz VBLANK rate derived from Z80 cycles and is\n"
"                              proportional to the CPU clock frequency.\n"
"                          1 : 50 Hz VBLANK rate derived from the host timer.\n"
"\n"
"                          When running in turbo mode then setting the <method>\n"
"                          equal to 1 will ensure that a VBLANK rate of 50Hz\n"
"                          will be used. Without this, key repeating may be\n"
"                          too fast.\n"
"\n"
"  -x, --xtal=f            Old non preferred options to set the Z80 clock\n"
"                          frequency. Use --clock option instead.\n"
"\n"
"  --z80div=n              Determines the number of Z80 blocks emulated per z80\n"
"                          frame. This value allows the polling rate to be\n"
"                          increased or decreased. The polling rate per second\n"
"                          is the product of the frame rate (--frate) and this\n"
"                          value. The value of n may range from 1 to 5000. On\n"
"                          versions prior to 2.7.0 this value was 1. Default\n"
"                          value is 25.\n"
"\n"
// +++++++++++++++++++++++++ Tape port emulation +++++++++++++++++++++++++++++++
" Tape port emulation:\n\n"
"                          See 'File path searching' further on for detailed\n"
"                          information. The default area for tapes is:\n"
"\n"
"                          @UBEE512@\\tapes\\\n"
"\n"
"                          WAV and TAP files are supported and the input and\n"
"                          output method can be mixed.\n"
"\n"
"  --tapei=file            Tape input from a WAV file. If an open tape input\n"
"                          file is already in use then that file will be closed\n"
"                          first before opening the new tape input file.\n"
"  --tapei-close           Closes a currently open tape input file. This allows\n"
"                          the file to be accessed externally without exiting\n"
"                          the emulator.\n"

"  --tapei-det=value       Optional input high and low detection percentage for\n"
"                          simulating tape input hysteresis threshold levels.\n"
"                          This value if specified is used in place of the\n"
"                          internally set value of 0\%.\n"
"\n"
"  --tapeo=file            Tape output to a WAV file. If an open tape output\n"
"                          file is already in use then that file will be closed\n"
"                          first before creating the new tape output file.\n"
"  --tapeo-close           Closes a currently open tape output file. This allows\n"
"                          the file to be accessed externally without exiting\n"
"                          the emulator.\n"
"\n"
"  --tapesamp=frequency    Tape output sample frequency in Hz. Default is\n"
"                          22050 Hz.\n"
"\n"
"  --tapevol=level         Tape output wave file volume level. A level of 0 to\n"
"                          100\% is allowed. Default is 15\%.\n"
"\n"
"  --tapfile-list=file     List all the DGOS tape file names contained in the\n"
"                          TAP file.\n"
"  --tapfilei=file         TAP file input and output options, these work in the\n"
"  --tapfileo=file         same fashion as that described for the tape WAV file\n"
"  --tapfilei-close        options. No initial tape rewind is required for TAP\n"
"  --tapfileo-close        files.\n"
"\n"
// ++++++++++++++++++++++++ Preconfigured variables ++++++++++++++++++++++++++++
" Variables\n"
" ---------\n"
"                          The --varset option may be used to create variables.\n"
"                          These variables are pre-configured:\n"
"\n"
"  UBEE_USERHOME           User's home path on Unices, or the directory\n"
"                          containing the ubee512 exe on Windows systems.\n"
"  UBEE512 or ubee512      Path to the ubee512 account.\n"
"  UBEE_VERSION            Emulator version.\n"
"  UBEE_HOST               Host system (UNIX or WIN).\n"
"  UBEE_SYS_MAJOR          On Unix systems this will be 'UNIX' On Windows\n"
"                          systems it contains one of the following:\n"
"                          win9x_me, nt4, nt5 or nt6. Both systems will use\n"
"                          upper case for the variable value.\n"
"  UBEE_SYS_MAJOR_VAL      Windows OSVERSIONINFOEX dwMajorVersion member value.\n"
"  UBEE_SYS_MINOR          On Unix systems this will be the value of the\n"
"                          uname.sysname member. On Windows systems it contains\n"
"                          one of the following: w95, w98, me, nt4_ws,\n"
"                          nt4_server, w2000, xp, server_2003, vista, w7, w8,\n"
"                          w8.1 or w10. Both systems will use upper case for\n"
"                          the variable value.\n"
"  UBEE_SYS_MINOR_VAL      Windows OSVERSIONINFOEX dwMinorVersion member value.\n"
"  UBEE_MODEL              Microbee model selected for emulation.\n"
"  UBEE_RAM                Amount of main memory (kb) for the emulated model.\n"
"\n"
" Time and Date            These variables are normally used to form other\n"
"                          variables, see TD variable usage in 'ubee512rc'.\n"
"\n"
"  SS                      Seconds. (00-59)\n"
"  MM                      Minutes. (00-59)\n"
"  HH                      Hours. (00-23) \n"
"  DD                      Month day. (01-2x/3x) \n"
"  mm                      Month. (01-12) \n"
"  YYYY                    4 digit year. (1900-20xx)\n"
"  YY                      2 digit year. (00-99)\n"
"  ww                      Week day Sun-Sat. (0-6)\n"
"  ac                      Week day capitalised. (Sun-Sat)\n"
"  al                      Week day lower case. (sun-sat)\n"
"  au                      Week day upper case. (SUN-SAT)\n"
"\n"
// ++++++++++++++++++++++++++++++++ arguments ++++++++++++++++++++++++++++++++++
" Arguments\n"
" ---------\n"
"  Integer arguments:\n"
"  Integer values may be entered using Decimal, Hexadecimal or Octal notation.\n"
"  For Hexadecimal input a leading '0x' or 0X' must precede the actual value.\n"
"  i.e. '0x12', 0x1234'.  For Octal input a leading '0' must precede the actual\n"
"  value. i.e. '012', '01234'. The default input notation is decimal.\n"
"\n"
"  Floating point arguments:\n"
"  Floating point values may be entered using Decimal or Hexadecimal notation\n"
"  in the same way as for Integer arguments.\n"
"\n"
"  Prefixed arguments:\n"
"  Prefixed arguments must commence with a '+' or '-' character, a '+' prefix\n"
"  enables while a '-' prefix disables.\n"
"\n"
"  The prefixes supported by each argument will be shown in brackets along\n"
"  side it, i.e. (+-). The first prefix shown represents the default state.\n"
"\n"
"  Additional arguments may be declared in the same option. A '+' prefixed\n"
"  argument may be negated by using a '-' prefixed one.\n"
"\n"
"  Bank t arguments:\n"
"  These values are used by some of the debugging options that operate on\n"
"  banks of memory.\n"
"\n"
"  't' type     RAM type\n"
"  --------     --------\n"
"  att          attribute memory.\n"
"  col          colour memory.\n"
"  pcg          PCG memory.\n"
"  scr          screen memory.\n"
"  mem          DRAM memory.\n"
"  vid          all video memory.\n"
"\n"
// ++++++++++++++++++++++++++ General information ++++++++++++++++++++++++++++++
" File path searching\n"
" -------------------\n"
"  Path slash characters:\n"
"  Forward or back slashes may be used in file paths irrespective of the\n"
"  program being run under Unices or Windows environments when slash\n"
"  conversion is enabled. See --slashes option. Unices will see '\\' as an\n"
"  escape sequence when used on the command line and also when found in\n"
"  configuration files and slash conversion is disabled.\n"
"\n"
"  Files to be opened:\n"
"  Existing files will first be searched for in the current directory, if the\n"
"  path is not found a second search in the default directory will take place.\n"
"  For the second search the file path specified will be appended to the default\n"
"  directory path. The second search is not carried out if a '\\', '.\\', or\n"
"  '..\\' are the first characters of the path or a ':' character is used under\n"
"  Windows.\n"
"\n"
"  Files to be created:\n"
"  Files to be created will be placed into the default directory unless a path\n"
"  to another location is specified by using a '\\', '.\\', or '..\\' as the\n"
"  first characters of the path or a ':' character is used under Windows.\n"
"\n"
// +++++++++++++++++++++++++++++++++ other +++++++++++++++++++++++++++++++++++++
"If you have any new feature suggestions, bug reports, etc. then post a new\n"
"topic at www.microbee-mspp.org.au\n";

#ifdef MINGW
#define page_help 1
#else
#define page_help 0
#endif

 while (1)
    {
     switch (help->state)
        {
         case 0 :
            if ((! page_help) && (! emu.runmode))
               {
                printf("%s", usage); // don't use xprintf() here!
                help->state = -1;    // done
                return;
               }

            if (console.xstdin == 0)
               help->lw = 8;
            else
               help->lw = 24;
            help->index = 0;
            help->lineswanted = help->lw;
            help->state++;
            break;
         case 1 :
            while (1)
               {
                if (! help->lineswanted) // done if no more lines
                   {
                    help->state = -1;
                    return;
                   }
                ch = (usage[help->index]);

                if (ch != '\n')
                   {
                    xprintf("%c", ch);
                    help->index++;
                   }
                else
                   {
                    help->index++;      // get past the \n character
                    xprintf("\n");

                    if (! usage[help->index])
                       {
                        if (console.xstdin)
                           {
                            xprintf("\r===== End of help information ================================== ESC=done =====");
                            xflush();
                            help->state = 3;
                            return;
                           }
                        else
                           {
                            xprintf("===== End of help information =====\n");
                            help->state = -1;
                            return;
                           }
                       }

                    if (--help->lineswanted == 0)
                       {
                        if (console.xstdin)
                           {
                            xprintf("\r===== ENTER=next line ============ SPACE=next screen =========== ESC=done =====");
                            xflush();
                           }
                        help->state++;
                        return;
                       }
                   }
               }
            break;
         case 2 : // get the ENTER, SPACE or ESC key
            if (console.xstdin)
               ch = getch();
            else
               ch = osd_getkey();
            if ((ch != 13) && (ch != 32) && (ch != 27))
               return;

            if (console.xstdin)
               {
                xprintf("\r                                                                               \r");
                xflush();
               }

            switch (ch)
               {
                case 13 : help->lineswanted = 1;
                          break;
                case 32 : help->lineswanted = help->lw;
                          break;
                case 27 : help->lineswanted = 0;
                          if (! console.xstdin)
                             {
                              xprintf("===== User exited help =====\n");
                              help->state = -1;
                              return;
                             }
                          break;
               }
            help->state = 1;
            break;
        case 3 : // get the ESC key to exit (used by stdin only)
           while (getch() != 27)
              ;
           help->state = -1;
           xprintf("\r                                                                               \r");
           xflush();
           return;
        }
    }
}

//==============================================================================
// Options usage
//
// The options usage information uses a a state machine.  This function is
// called when the 'options usage' option is processed.
//
// If the OSD console dialogue is active then only the first machine state
// is executed and any further states are called from the OSD module using
// the options_usage_state() function.
//
//   pass: void
// return: void
//==============================================================================
static void options_usage (void)
{
 help.state = 0;

 if (console.xstdin == 0)
    options_usage_state(&help);
 else
    {
     while (help.state != -1)
        options_usage_state(&help);
    }
}

//==============================================================================
// Print parameter error message and set exit status.
//
//   pass: void
// return: void
//==============================================================================
static void param_error_mesg (void)
{
#define PARMERR_MESG "ubee512: option `--%s' argument of '%s' is not permitted\n"
 xprintf(PARMERR_MESG, (char *)long_options[long_index].name, e_optarg);
 exitstatus = 1;
}

//==============================================================================
// Unset a uBee512 environment variable.
//
// The variable name 'varname' or the variable name and value. i.e
// 'varname=', 'varname=value' may be passed.
//
//   pass: s *char                      uBee512 environment variable name
// return: void
//==============================================================================
static void options_ubee512_envvar_unset (char *s)
{
 int i;
 int l;
 char *found;
 char *search;

 char temp_str[512];

 // remove '=' character and any trailing string if an '=' character is found.
 if ((search = strchr(s, '=')) == NULL)
    {
     l = strlen(s);
     search = s;
    }
 else
    {
     l = (search - s);
     strncpy(temp_str, s, l);
     temp_str[l] = 0;
     search = temp_str;
    }

 for (i = 0; i < emuenvc; i++)
    {
     found = strstr(emuenv[i], search);
     if ((found == emuenv[i]) && (emuenv[i][l] == '='))
        {
         free(emuenv[i]);   // free the memory allocated to it
         emuenv[i] = NULL;  // incase we only have one pointer
         emuenvc--;         // one less variable now
         if (emuenvc)       // if 1 or more than move pointers up
            memmove(&emuenv[i], &emuenv[i + 1], sizeof(emuenv[i]) * (emuenvc - i));
         return;
        }
    }
}

//==============================================================================
// Set a uBee512 environment variable.
//
// A check is first made to see if the variable already exists and if so is
// removed before the new variable is set.
//
// The variable uses the format 'variable_name=variable_value'.  If a
// variable being set does not have a '=' character one will be appended to
// the end of the variable name before being set.
//
//   pass: s *char                      uBee512 environment variable name/value
// return: int                          0 if no errors, else -1
//==============================================================================
int options_ubee512_envvar_set (char *s)
{
 int l;
 int add_equals = 0;

 // check and remove any variable already defined by this name
 options_ubee512_envvar_unset(s);

 l = strlen(s) + 1;

 // variable being searched for must have a '=' character in the string
 if (strchr(s, '=') == NULL)
    add_equals = 1;

 if (emuenvc < OPTIONS_ENV_SIZE)
    {
     if (! (emuenv[emuenvc] = options_malloc(l + add_equals)))
        return -1;
     strcpy(emuenv[emuenvc++], s);
     if (add_equals)
        strcat(emuenv[emuenvc-1], "=");
     return 0;
    }
 else
    return -1;
}

//==============================================================================
// Return a uBee512 environment variable.
//
// Searches a list of variables and returns a pointer to the value if the name
// matches.  A NULL is returned if no matches were found.
//
// The variable uses the format 'variable_name=variable_value'
//
//   pass: s *char                      uBee512 environment variable name
// return: char *                       pointer to variable else NULL
//==============================================================================
static char *options_ubee512_envvar_get (char *s)
{
 int i;

 char *x;

 for (i = 0; i < emuenvc; i++)
    {
     x = strstr(emuenv[i], s);
     if ((x == emuenv[i]) && (emuenv[i][strlen(s)] == '='))
        return x + strlen(s) + 1;  // pointer to the string after '='
    }
 return NULL;
}

//==============================================================================
// Compare a uBee512 'string1,string2' using a strvserscmp() or strcmp()
// function depending on the --if-cmpmode option.
//
// The name and value passed uses the format 'string1,string2'
//
//   pass: s *char              "string1,string2" (no leading spaces)
// return: int                  0=equals, -=less, +=greater, -0xf0000=error
//==============================================================================
static int options_ubee512_compare (char *s)
{
 char *x;

 char temp_str1[512];
 char temp_str2[512];

 strcpy(temp_str1, s);
 x = strstr(temp_str1, ",");
 if (x)
    {
     *x = 0;
     strcpy(temp_str2, x+1);
     switch (if_cmp_mode)
        {
         case 0 : return xstrverscmp(temp_str1, temp_str2);
         case 1 : return strcmp(temp_str1, temp_str2);
        }
    }

 return -0xf0000;
}

//==============================================================================
// Extract application, environment and built-in variables.
//
// Application, environment and built-in variables contained in the
// parameter are referenced by using an @ENVVAR@ format in configuration
// files.  Passing environment variables from the command line can use the
// shell's method instead.
//
// Variables may be defined using the --varset option, set as system
// environment variables or use predifined variables. The search order is as
// described so variables may be redefined as the first matching value will
// be used.
//
// Two variables types are returned, e_option_arg contains the @UBEE512@ as
// is and e_option_q_arg contains the same but with double quotation marks
// placed around the variable if any spaces are found,  this value is
// intended for passing to Z80 applications where needed.
//
// The built-in pre-defined variables are:
//      UBEE512 : path to the ubee512 account.
// UBEE_VERSION : version string of the emulator.
//    UBEE_HOST : host system.
//
// To set uBee512 environment variables using options:
// --if-system=win
//    --varset="UBEE_COM1=1"
// --if-end
// --if-system=linux
//    --varset="UBEE_COM1=/dev/ttyS0"
// --if-end
//
// To set system environment variables using the host system's shell method:
//
// Windows:
// set ENVVAR=value
//
// bash, sh:
// export ENVVAR=value
//
// csh, tsh:
// setenv ENVVAR value
//
//   pass: char *options_arg
//         char *e_options_arg          
//         char *e_options_q_arg        double quoted variable if any spaces
// return: void
//==============================================================================
static void extract_environment_vars (char *options_arg, char *e_options_arg, char *e_options_q_arg)
{
#define ENV_NAME_SIZE 1000
 char env[ENV_NAME_SIZE];

 char *ep;

 int o_save = 0;
 int o_index = 0;
 int e_index = 0;
 int e_index_q = 0;
 int v_index;

 e_options_arg[0] = 0;
 e_options_q_arg[0] = 0;

 if (options_arg == NULL)
    return;

 while ((options_arg[o_index]) && (o_index < OPTIONS_PARM_SIZE) && (e_index < OPTIONS_PARM_SIZE))
    {
     if (options_arg[o_index] != '@') // if not start of ENV string
        {
         e_options_arg[e_index++] = options_arg[o_index];
         e_options_q_arg[e_index_q++] = options_arg[o_index];
         e_options_arg[e_index] = 0;
         e_options_q_arg[e_index_q] = 0;
         o_index++;
        }
     else
        {
         v_index = 0;
         env[0] = 0;
         o_index++;
         o_save = o_index;
         while ((options_arg[o_index]) && (options_arg[o_index] != '@') &&
         (o_index < OPTIONS_PARM_SIZE) && (v_index < ENV_NAME_SIZE))
            env[v_index++] = options_arg[o_index++];

         env[v_index] = 0;

         if (options_arg[o_index] == '@')       // if end of ENV string
            {
             o_index++;
             int have_env_var = 0;
#ifdef MINGW
             char s[OPTIONS_PARM_SIZE];
             if (GetEnvironmentVariable(env, s, sizeof(s)) != 0)
                {
                 have_env_var = 1;
                 ep = s;
                }
#else
             ep = getenv(env);
             if (ep)
                have_env_var = 1;
#endif
             else
                {
                 ep = options_ubee512_envvar_get(env);
                 if (ep)
                     have_env_var = 1;
                }

             if (have_env_var)
                {
                 strcat(e_options_arg, ep);
                 if (strchr(ep, ' '))  // if contains any spaces
                    {
                     strcat(e_options_q_arg, "\"");  // double quote
                     strcat(e_options_q_arg, ep);
                     strcat(e_options_q_arg, "\"");  // double quote
                    }
                 else
                    strcat(e_options_q_arg, ep);
                }
             e_index = strlen(e_options_arg);
             e_index_q = strlen(e_options_q_arg);
            }
         else
            {
             o_index = o_save;  // go back to index after first '@' char
             e_options_arg[e_index++] = '@';  // treat it as a normal char.
             e_options_q_arg[e_index_q++] = '@';  // treat it as a normal char.
            }
        }
    }

 e_options_arg[e_index] = 0;
 e_options_q_arg[e_index_q] = 0;
}

//==============================================================================
// Get a prefixed argument.
//
// Calling this function with 'x=1' will also convert the arguments to lower
// case before being processed.
//
// A 0 is returned if no value found (end of string).  A 1 is returned if a
// legal prefixed/delimited string value was found.  A -1 is returned if any
// errors.
//
//   pass: int x                        prefixed argument wanted
//         int *pf                      prefix value result
//         char *use_args               array of arguments allowed
// return: int                          prefixed string number or -1 if done
//==============================================================================
int get_prefixed_argument (int x, int *pf, char *use_args[])
{
 int res;

 if (x == 1)
    tolower_string(e_optarg_x, e_optarg);

 res = string_prefix_get(e_optarg_x, temp_str, x, sizeof(temp_str));
 *pf = (res == '+');

 if (res == 0)  // exit if no more values found
    return -1;

 if (res != -1) // if prefixed value found
    res = string_search(use_args, temp_str);

 if (res == -1)
    {
     xprintf(PARMERR_MESG, (char *)long_options[long_index].name, e_optarg);
     exitstatus = 1;
    }

 return res;
}

//==============================================================================
// Set a single integer value from a passed arguments list.
//
//   pass: int *value                   pointer to the value to be set
//         char *use_args               array of arguments allowed
// return: int                          -1 if error else value
//==============================================================================
int set_int_from_list (int *value, char *use_args[])
{
 int res;

 res = string_search(use_args, e_optarg);

 if (res == -1)
    {
     xprintf(PARMERR_MESG, (char *)long_options[long_index].name, e_optarg);
     exitstatus = 1;
    }
 else
    *value = res;

 return res;
}

//==============================================================================
// Set a single integer value if between limits.
//
//   pass: int *value                   pointer to the value to be set
//         int min                      minimum value allowed
//         int max                      maximum value allowed
// return: int                          -1 if error else 0
//==============================================================================
int set_int_from_arg (int *value, int min, int max)
{
 if ((int_arg >= min) && (int_arg <= max))
    {
     *value = int_arg;
     return 0;
    }

 xprintf(PARMERR_MESG, (char *)long_options[long_index].name, e_optarg);
 exitstatus = 1;
 return -1;
}

//==============================================================================
// Set a single float value if between limits.
//
//   pass: float *value                 pointer to the value to be set
//         float min                    minimum value allowed
//         float max                    maximum value allowed
// return: int                          -1 if error else 0
//==============================================================================
int set_float_from_arg (float *value, float min, float max)
{
 if ((float_arg >= min) && (float_arg <= max))
    {
     *value = float_arg;
     return 0;
    }

 xprintf(PARMERR_MESG, (char *)long_options[long_index].name, e_optarg);
 exitstatus = 1;
 return -1;
}

//==============================================================================
// Process options: Short options.
//
//   pass: int c                        single option number
//         char *argv[]                 pointer to pointer to arguments
// return: void
//==============================================================================
static void options_short (int c, char *argv[])
{
 switch (c)
    {
     case   '?' :
        default : if (! opterr_msg[0])
                     {
                      exitstatus = 1;
                      xprintf("%s: unrecognised getopt error\n", argv[0]);
                      xprintf(TRY_MESG, argv[0], argv[0], argv[0]);
                     }
                  break;
    }
}

//==============================================================================
// Process options: Control related.
//
//   pass: int c                        option number
//         char *argv[]
// return: void
//==============================================================================
static void options_control (int c)
{
 char *mouse_wheel_args[] =
 {
  "none",
  "vol",
  "win",
  ""
 };

 char *cf_args[] =
 {
  "boot",
  "pc85",
  ""
 };

 char *keystd_mod_args[] =
 {
  "all",
  "ctrl_shift",
  ""
 };

 char *output_args[] =
 {
  "all",
  "osd",
  "stdout",
  ""
 };

 char *status_args[] =
 {
  "all",
  "d",
  "drive",
  "emu",
  "emuver",
  "joy",
  "left",
  "model",
  "mouse",
  "mute",
  "print",
  "ram",
  "speed",
  "serial",
  "sys",
  "tape",
  "title",
  "ver",
  "vol",
  "win",
  ""
 };

 char *args_error_args[] =
 {
  "unknown",
  ""
 };

 char *ptr;
 int pf;
 int res = 0;
 int x = 1;

 switch (c)
    {
     case OPT_ACCOUNT :
        break;
     case OPT_ALIAS_DISKS :
        set_int_from_list(&emu.alias_disks, offon_args);
        break;
     case OPT_ALIAS_ROMS :
        set_int_from_list(&emu.alias_roms, offon_args);
        break;
     case OPT_ARGS_ERROR :
        while (1)
           {
            res = get_prefixed_argument(x++, &pf, args_error_args);
            if (res == -1)
               break;

            switch (res)
               {
                case  0 : args_err_flags = (args_err_flags & ~0x01) | (0x01 * pf);
               }
           }
        break;
     case OPT_BOOTKEY :
        if (! e_optarg[0] || e_optarg[1])
           param_error_mesg();
        else
           {
            x = toupper(e_optarg[0]);
            if (isalpha(x))
               keyb_force((x - 'A') + 1, 1);
            else
               if (isdigit(x))
                  keyb_force((x - '0') + 32, 1);
           }
        break;
     case OPT_CFMODE :
        set_int_from_list(&emu.cfmode, cf_args);
        break;
     case OPT_CONFIG :
        break;
     case OPT_CMD_REPEAT1 :
        set_int_from_arg(&emu.cmd_repeat1, 1, MAXINT);
        break;
     case OPT_CMD_REPEAT2 :
        set_int_from_arg(&emu.cmd_repeat2, 1, MAXINT);
        break;
     case OPT_CPU_DELAY :
        set_int_from_arg(&emu.proc_delay_type, 0, 2);
        break;
     case OPT_DCLICK :
        set_int_from_arg(&gui.dclick_time, 100, 3000);
        break;
     case OPT_EXIT :
        exitstatus = int_arg;
        break;
     case OPT_EXIT_CHECK :
        set_int_from_list(&emu.exit_check, offon_args);
        break;
     case OPT_GUI_PERSIST :
        set_int_from_arg(&gui.persist_time, 1, MAXINT);
        break;
     case OPT_KEYSTD_MOD :
        while (1)
           {
            res = get_prefixed_argument(x++, &pf, keystd_mod_args);
            if (res == -1)
               break;
            keystd_proc_mod_args(res, pf);
           }
        break;
     case OPT_LOCKFIX_WIN32 :
        set_int_from_list(&emu.win32_lock_key_fix, offon_args);
        break;
     case OPT_LOCKFIX_X11 :
        set_int_from_list(&emu.x11_lock_key_fix, offon_args);
        break;
     case OPT_MD5_CREATE :
        set_int_from_list(&emu.roms_create_md5, offon_args);
        break;
     case OPT_MMODE :
        keyb_force(0x0d, 100);  // force into monitor mode
        break;
     case OPT_MOUSE_WHEEL :
        set_int_from_list(&gui.mouse_wheel, mouse_wheel_args);
        break;
     case OPT_NODISK :
        fdc.nodisk = 1;
        break;
     case OPT_OPTIONS_WARN :
        set_int_from_list(&runmode_warn, offon_args);
        break;
     case OPT_OUTPUT :
        while (1)
           {
            res = get_prefixed_argument(x++, &pf, output_args);
            if (res == -1)
               break;
            console_proc_output_args(res, pf);
           }
        break;
     case OPT_POWERCYC :
        emu.reset = EMU_RST_POWERCYC_NOW;
        emu.keyesc = 0;
        emu.keym = 0;
        break;
     case OPT_PREFIX :
        strcpy(emu.prefix_path, e_optarg);
        break;
     case OPT_SDL_PUTENV :
        // we have to keep the variables ourselves! SDL_putenv(e_optarg)
        // won't work as the value gets changed on each option!
        ptr = malloc(strlen(e_optarg) + 1);
        if (ptr)
           {
            strcpy(ptr, e_optarg);
            SDL_putenv(ptr);
           }
        else
           param_error_mesg();
        break;
     case OPT_RESET :
        emu.reset = EMU_RST_RESET_NOW;
        emu.keyesc = 0;
        emu.keym = 0;
        break;
     case OPT_RUNSECS :
        if ((int_arg != 0) && (int_arg < 5))  // can't use 'set_int_from_arg()' on this
           param_error_mesg();
        else
           emu.secs_exit = int_arg;
        break;
     case OPT_SLASHES :
        set_int_from_list(&emu.slashconv, offon_args);
        break;
     case OPT_SPAD :
        if (gui_status_padding(int_arg))
           param_error_mesg();
        break;
     case OPT_STATUS :
        while (1)
           {
            res = get_prefixed_argument(x++, &pf, status_args);
            if (res == -1)
               break;
            gui_proc_status_args(res, pf);
           }
        break;
     case OPT_TITLE :
        strncpy(gui.title, e_optarg, sizeof(gui.title));
        gui.title[sizeof(gui.title)-1] = 0;
        break;
     case OPT_VARSET :
        options_ubee512_envvar_set(e_optarg);
        break;
     case OPT_VARUSET :
        options_ubee512_envvar_unset(e_optarg);
        break;
     case OPT_VERBOSE :
        if (! e_optarg[0])
           emu.verbose = 1;
        else
           emu.verbose = int_arg;
        break;
    }
}

//==============================================================================
// Process options: Conditional option parsing.
//
//   pass: int c                        option number
//         char *argv[]
// return: void
//==============================================================================
static void options_conditional (int c)
{
 int x = 1;

 switch (c)
    {
     case OPT_IF_EGT :
        x = options_ubee512_compare(e_optarg);
        if (x == -0xf0000)
           param_error_mesg();
        else
           if_state[++if_pos] = (x >= 0) & if_state_prev;
        break;
     case OPT_IF_ELT :
        x = options_ubee512_compare(e_optarg);
        if (x == -0xf0000)
           param_error_mesg();
        else
           if_state[++if_pos] = (x <= 0) & if_state_prev;
        break;
     case OPT_IF_EQ :
        x = options_ubee512_compare(e_optarg);
        if (x == -0xf0000)
           param_error_mesg();
        else
           if_state[++if_pos] = (x == 0) & if_state_prev;
        break;
     case OPT_IF_GT :
        x = options_ubee512_compare(e_optarg);
        if (x == -0xf0000)
           param_error_mesg();
        else
           if_state[++if_pos] = (x > 0) & if_state_prev;
        break;
     case OPT_IF_LT :
        x = options_ubee512_compare(e_optarg);
        if (x == -0xf0000)
           param_error_mesg();
        else
           if_state[++if_pos] = (x < 0) & if_state_prev;
        break;
     case OPT_IF_NEGT :
        x = options_ubee512_compare(e_optarg);
        if (x == -0xf0000)
           param_error_mesg();
        else
           if_state[++if_pos] = (x < 0) & if_state_prev;
        break;
     case OPT_IF_NELT :
        x = options_ubee512_compare(e_optarg);
        if (x == -0xf0000)
           param_error_mesg();
        else
           if_state[++if_pos] = (x > 0) & if_state_prev;
        break;
     case OPT_IF_NEQ :
        x = options_ubee512_compare(e_optarg);
        if (x == -0xf0000)
           param_error_mesg();
        else
           if_state[++if_pos] = (x != 0) & if_state_prev;
        break;
     case OPT_IF_NGT :
        x = options_ubee512_compare(e_optarg);
        if (x == -0xf0000)
           param_error_mesg();
        else
           if_state[++if_pos] = (x <= 0) & if_state_prev;
        break;
     case OPT_IF_NLT :
        x = options_ubee512_compare(e_optarg);
        if (x == -0xf0000)
           param_error_mesg();
        else
           if_state[++if_pos] = (x >= 0) & if_state_prev;
        break;
     case OPT_IF_NSET :
        if_state[++if_pos] = (options_ubee512_envvar_get(e_optarg) == NULL) & if_state_prev;
        break;
     case OPT_IF_SET :
        if_state[++if_pos] = (options_ubee512_envvar_get(e_optarg) != NULL) & if_state_prev;
        break;
     case OPT_IF_SYSTEM :
#ifdef MINGW
        x = ((strcmp(e_optarg_x, "WIN") == 0) || (strcmp(e_optarg_x, win_major_ver) == 0) ||
        (strcmp(e_optarg_x, win_minor_ver) == 0));
#else
        x = ((strcmp(e_optarg_x, "UNIX") == 0) || (strcmp(e_optarg_x, emu.sysname) == 0));
#endif
        if_state[++if_pos] = x & if_state_prev;
        break;
     case OPT_IF_FALSE :
        if_state[++if_pos] = 0 & if_state_prev;
        break;
     case OPT_IF_TRUE :
        if_state[++if_pos] = 1 & if_state_prev;
        break;
     case OPT_IF_ELSE :
        if (if_state[if_pos - 1])
           if_state[if_pos] = ! if_state[if_pos];
        break;
     case OPT_IF_END :
        if (if_pos)
           if_pos--;
        break;
     case OPT_IF_CMPMODE :
        set_int_from_arg(&if_cmp_mode, 0, 1);
        break;
    }
}

//==============================================================================
// Process options: Debugging.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_debugging (int c)
{
 char *rst_args[] =
 {
  "00h",
  "08h",
  "10h",
  "18h",
  "20h",
  "28h",
  "30h",
  "38h",
  ""
 };

 char *modio_args[] =
 {
  "all",
  "log",
  "raminit",
  "beetalker",
  "beethoven",
  "clock",
  "compumuse",
  "crtc",
  "dac",
  "fdc",
  "fdc_wtd",
  "fdc_wth",
  "func",
  "hdd",
  "ide",
  "joystick",
  "keystd",
  "keytc",
  "mem",
  "options",
  "roms",
  "pioa",
  "piob",
  "piocont",
  "rtc",
  "sn76489",
  "tapfile",
  "ubee512",
  "vdu",
  "vdumem",
  "video",
  "z80",
  ""
 };

 char *regs_args[] =
 {
  "all",
  "crtc",
  "pio",
  "rtc",
  "z80",
  ""
 };

 char *debug_args[] =
 {
  "off",
  "on",
  "regs",
  "memr",
  "index",
  "alt",
  "count",
  "tstates",
  "all",
  
  "piopoll",
  "step",
  "step10",
  "step20",
  "trace",
  ""
 };

 int pf;
 int res = 0;
 int x = 1;

 z80debug_capture(1, (char *)long_options[long_index].name, optarg);

 switch (c)
    {
     case OPT_BP :
     case OPT_DB_BP :
        if (z80debug_pc_breakpoint_set(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_BPR :
     case OPT_DB_BPR :
        if (z80debug_pc_breakpoint_setr(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_BPCLR :
     case OPT_DB_BPCLR :
        if (z80debug_pc_breakpoints_clear(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_DB_BPOS :
        if (z80debug_pc_breakpoints_os(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_BPC :
     case OPT_DB_BPC :
        set_int_from_arg(&debug.break_point_count, 0, MAXINT);
        break;

     case OPT_DB_BP_PORT :
        if (z80debug_bp_port(e_optarg, 's') == -1)
           param_error_mesg();
        break;
     case OPT_DB_BPCLR_PORT :
        if (z80debug_bp_port(e_optarg, 'c') == -1)
           param_error_mesg();
        break;
     case OPT_DB_BPR_PORT :
        if (z80debug_bp_port(e_optarg, 'r') == -1)
           param_error_mesg();
        break;

     case OPT_DB_BP_RST :
        if (set_int_from_list(&x, rst_args) == -1)
           break;
        debug.rst_break_point[x] = 1;
        break;
     case OPT_DB_BPCLR_RST :
        if (set_int_from_list(&x, rst_args) == -1)
           break;
        debug.rst_break_point[x] = 0;
        break;
     case OPT_DB_BPR_RST :
        if (set_int_from_list(&x, rst_args) == -1)
           break;
        debug.rst_break_point[x] = 2;
        break;

     case OPT_DB_BREAK :
     case OPT_BREAK :
        if (! emu.paused)
           z80debug_command_exec(EMU_CMD_PAUSE, 0);
        break;

     case OPT_DB_BP_MEM :
     	if (z80debug_bp_mem(e_optarg, 's', 'a') == -1)
           param_error_mesg();
     	break;
     case OPT_DB_BPCLR_MEM :
     	if (z80debug_bp_mem(e_optarg, 'c', 'a') == -1)
           param_error_mesg();
     	break;
     case OPT_DB_BP_MEML :
     	if (z80debug_bp_mem(e_optarg, 's', 'l') == -1)
           param_error_mesg();
     	break;
     case OPT_DB_BPCLR_MEML :
     	if (z80debug_bp_mem(e_optarg, 'c', 'l') == -1)
           param_error_mesg();
     	break;

     case OPT_DB_CONT :
     case OPT_CONT :
        if (emu.paused)
           z80debug_command_exec(EMU_CMD_PAUSE, 0);
        break;

     case OPT_DB_DASM :
        if (z80debug_dasm(e_optarg, 'a') == -1)
           param_error_mesg();
        break;
     case OPT_DB_DASML :
        if (z80debug_dasm(e_optarg, 'l') == -1)
           param_error_mesg();
        break;

     case OPT_DB_DUMP :
        if (z80debug_dump_memory(e_optarg, 'a') == -1)
           param_error_mesg();
        break;
     case OPT_DB_DUMPB :
        if (z80debug_dump_bank(e_optarg, 'a') == -1)
           param_error_mesg();
        break;
     case OPT_DB_DUMPL :
        if (z80debug_dump_memory(e_optarg, 'l') == -1)
           param_error_mesg();
        break;
     case OPT_DB_DUMPLB :
        if (z80debug_dump_bank(e_optarg, 'l') == -1)
           param_error_mesg();
        break;
     case OPT_DB_DUMPP :
        if (z80debug_dump_port(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_DB_DUMPR :
        z80debug_dump_registers();
        break;

     case OPT_DB_FILLM :
        if (z80debug_fill_memory(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_DB_FILLB :
        if (z80debug_fill_bank(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DB_FINDB :
        if (z80debug_find_bank(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_DB_FINDM :
        if (z80debug_find_memory(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DB_GO :
        set_int_from_arg(&emu.new_pc, 0, 0xffff);
        break;

     case OPT_DB_LOADB :
        if (z80debug_load_bank(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_DB_LOADM :
        if (z80debug_load_memory(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DB_MOVE :
        if (z80debug_move_memory(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DB_POPM :
        if (z80debug_pop_mem(e_optarg) == -1)
           exitstatus = 1;
        break;
     case OPT_DB_POPR :
        if (z80debug_pop_regs(e_optarg) == -1)
           exitstatus = 1;
        break;

     case OPT_DB_PORTR :
        if (z80debug_port_read(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_DB_PORTW :
        if (z80debug_port_write(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DB_PUSHM :
        if (z80debug_push_mem(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_DB_PUSHR :
        if (z80debug_push_regs(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DB_SAVEB :
        if (z80debug_save_bank(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_DB_SAVEM :
        if (z80debug_save_memory(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DB_SETB :
        if (z80debug_set_bank(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_DB_SETM :
        if (z80debug_set_memory(e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_DB_SETR :
        if (z80debug_set_reg(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DB_STEP :
        if (z80debug_step(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DB_TRACE :
        if (z80debug_trace(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DB_TRACE_CLR :
        debug.cond_trace_addr_s = -1;
        break;

     case OPT_DEBUG :
        if ((strcmp(e_optarg, "off") == 0) || (strcmp(e_optarg, "on") == 0))
           {
            if (strcmp(e_optarg, "off") == 0)
               {
                res = 1;  // use the 'on' argument entry
                pf = 0;   // and turn it off
               }
            else
               {
                res = 1;  // use the 'on' argument entry
                pf = 1;   // and turn it on
               }
            z80debug_proc_debug_args(res, pf);
           }
        else
           while (1)
              {
               res = get_prefixed_argument(x++, &pf, debug_args);
               if (res == -1)
                  break;
               z80debug_proc_debug_args(res, pf);
              }
        break;

     case OPT_DEBUG_CLOSE :
        z80debug_debug_file_close();
        break;
     case OPT_DEBUG_OPEN :
        if (z80debug_debug_file_create(e_optarg) == -1)
           param_error_mesg();
        break;

     case OPT_DASM_LINES :
        set_int_from_arg(&debug.dasm_lines, 0, 0xffff);
        break;
     case OPT_DUMP :
        set_int_from_arg(&debug.dump_addr, 0, 0xffff);
        break;
     case OPT_DUMP_HEADER :
        set_int_from_list(&debug.dump_header, offon_args);
        break;
     case OPT_DUMP_LINES :
        set_int_from_arg(&debug.dump_lines, 1, 4096);
        break;

     case OPT_ECHO :
        xprintf("%s\n", e_optarg);
        break;
     case OPT_ECHOQ :
        xprintf("%s\n", e_optarg_q);
        break;

     case OPT_FIND_COUNT :
        set_int_from_arg(&debug.find_count, 1, MAXINT);
        break;

     case OPT_MODIO :
        while (1)
           {
            res = get_prefixed_argument(x++, &pf, modio_args);
            if (res == -1)
               break;
            z80debug_proc_modio_args(res, pf);
           }
        break;
     case OPT_REGS :
        while (1)
           {
            res = get_prefixed_argument(x++, &pf, regs_args);
            if (res == -1)
               break;
            z80debug_proc_regdump_args(res, pf);
           }
        break;
    }

 z80debug_capture(0, NULL, NULL);
}

//==============================================================================
// Process options: Disks.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_disks (int c)
{
 int i;
 disk_t disk;
 
 switch (c)
    {
     case OPT_DISK_CREATE : // create a disk image
        strcpy(disk.filename, e_optarg);
        disk_create(&disk, 0);
        break;
     case OPT_HDD0 : // WD1002-5 Winchester drive
     case OPT_HDD1 : // WD1002-5 Winchester drive
     case OPT_HDD2 : // WD1002-5 Winchester drive
     case OPT_HDD3 : // WD1002-5 floppy drive 1
     case OPT_HDD4 : // WD1002-5 floppy drive 2
     case OPT_HDD5 : // WD1002-5 floppy drive 3
     case OPT_HDD6 : // WD1002-5 floppy drive 4
        i = c - OPT_HDD0;
        strcpy(hdd_d.disk.filename, e_optarg);
#ifdef USE_LIBDSK
        strcpy(hdd_d.disk.libdsk_type, use_driver_type);
        strcpy(hdd_d.disk.libdsk_format, use_format_type);
        if (c >= OPT_HDD3)
           {
            hdd_d.disk.side1as0 = side1as0;
            hdd_d.disk.dstep = dstep;
            hdd_d.disk.dstep_hd = dstep_hd;
           }
        use_driver_type[0] = 0;
        use_format_type[0] = 0;
        side1as0 = 0;
        dstep = 0;
        dstep_hd = 0;
#endif
        hdd_set_drive(i, &hdd_d);
        break;
     case OPT_HDD3_CLOSE :
     case OPT_HDD4_CLOSE :
     case OPT_HDD5_CLOSE :
     case OPT_HDD6_CLOSE :
        i = (c - OPT_HDD3_CLOSE) + 3;
        hdd_unloaddisk(i);
        break;
     case OPT_IDE_A0 :
     case OPT_IDE_A1 :
     case OPT_IDE_B0 :
     case OPT_IDE_B1 :
        i = c - OPT_IDE_A0;
        strcpy(ide_d.disk.filename, e_optarg);
#ifdef USE_LIBDSK
        strcpy(ide_d.disk.libdsk_type, use_driver_type);
        strcpy(ide_d.disk.libdsk_format, use_format_type);
        use_driver_type[0] = 0;
        use_format_type[0] = 0;
#endif
        ide_set_drive(i, &ide_d);
        break;
     case OPT_IMAGE_A : // core board floppy drive 1 (A)
     case OPT_IMAGE_B : // core board floppy drive 2 (B)
     case OPT_IMAGE_C : // core board floppy drive 3 (C)
     case OPT_IMAGE_D : // core board floppy drive 4 (D)
        i = c - OPT_IMAGE_A;
        strcpy(fdc_d.disk.filename, e_optarg);
#ifdef USE_LIBDSK
        strcpy(fdc_d.disk.libdsk_type, use_driver_type);
        strcpy(fdc_d.disk.libdsk_format, use_format_type);
        fdc_d.disk.side1as0 = side1as0;
        fdc_d.disk.cpm3 = cpm3;
        fdc_d.disk.dstep = dstep;
        fdc_d.disk.dstep_hd = dstep_hd;
        use_driver_type[0] = 0;
        use_format_type[0] = 0;
        side1as0 = 0;
        cpm3 = 0;
        dstep = 0;
        dstep_hd = 0;
#endif
        fdc_set_drive(i, &fdc_d);
        break;
     case OPT_A_CLOSE :
     case OPT_B_CLOSE :
     case OPT_C_CLOSE :
     case OPT_D_CLOSE :
        i = c - OPT_A_CLOSE;
        fdc_unloaddisk(i);
        break;
#ifdef USE_LIBDSK
     case OPT_CPM3 :
        cpm3 = 1;
        break;
     case OPT_DSTEP :
        dstep = 1;
        break;
     case OPT_DSTEP_HD :
        dstep_hd = 1;
        dstep = 1;
        break;
     case OPT_FORMAT :
        strncpy(use_format_type, e_optarg, sizeof(use_format_type));
        break;
     case OPT_LFORMAT :
        format = FMT_180K;
        while (dg_stdformat(NULL, format++, &fname, &fdesc) == DSK_ERR_OK)
           xprintf("%-10.10s : %s\n", fname, fdesc);
        exitstatus = -1;
        break;
     case OPT_LTYPE :
        i = 0;
        do
           {
            dsk_type_enum(i++, &xstr);
            if (xstr)
               xprintf("%s\n", xstr);
           }
        while (xstr);
        exitstatus = -1;
        break;
     case OPT_SIDE1AS0 :
        side1as0 = 1;
        break;
     case OPT_TYPE :
        strncpy(use_driver_type, e_optarg, sizeof(use_driver_type));
        break;
#endif
     case OPT_PSEC :
        xprintf("ubee512: Option `--psec' has been removed and is no longer required.\n");
        break; // no longer an option as probing is always used now.
    }
}

//==============================================================================
// Process options: Display related.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_display (int c)
{
 char *video_type_args[] =
 {
  "sw",
  "hw",
  "gl",
  ""
 };

 char *video_depth_args[] =
 {
  "8",
  "8gs",
  "16",
  "32",
  ""
 };

#ifdef USE_OPENGL
 char *gl_filter_args[] =
 {
  "soft",
  "sharp",
  ""
 };
#endif

 int x = 0;

 switch (c)
    {
     case OPT_ASPECT :
        set_int_from_arg(&video.aspect, 1, 2);
        break;
     case OPT_FULLSCREEN :
        if (! e_optarg[0])
           video.fullscreen = ! video.fullscreen;
        else
           set_int_from_list(&video.fullscreen, offon_args);
        break;
     case OPT_MONITOR :
        if (set_int_from_list(&x, monitor_args) == -1)
           break;
        if (x > 5)
           crtc.monitor = x - 6;
        else
           crtc.monitor = x;
        if (emu.runmode)
           {
            vdu_setcolourtable();
            crtc_set_redraw();
           }
        break;

     case OPT_MON_BG_B :
     case OPT_MON_BG_G :
     case OPT_MON_BG_R :
     case OPT_MON_BGI_B :
     case OPT_MON_BGI_G :
     case OPT_MON_BGI_R :
     case OPT_MON_FG_B :
     case OPT_MON_FG_G :
     case OPT_MON_FG_R :
     case OPT_MON_FGI_B :
     case OPT_MON_FGI_G :
     case OPT_MON_FGI_R :
        if (set_int_from_arg(&x, 0, 255) == -1)
           break;
        vdu_set_mon_table(c - OPT_MON_BG_B, x);
        break;

     case OPT_MON_FGL_B :
     case OPT_MON_FGL_G :
     case OPT_MON_FGL_R :
        xprintf("ubee512: Options `--mon-fgl-x' are no longer supported and are ignored.\n");
        xprintf("ubee512: These should be removed and the replacemnt options used instead.\n");
        xprintf("ubee512: See the ubee512rc.sample and README files.\n");
        break;

     case OPT_RGB_00_R :
     case OPT_RGB_00_G :
     case OPT_RGB_00_B :
     case OPT_RGB_01_R :
     case OPT_RGB_01_G :
     case OPT_RGB_01_B :
     case OPT_RGB_02_R :
     case OPT_RGB_02_G :
     case OPT_RGB_02_B :
     case OPT_RGB_03_R :
     case OPT_RGB_03_G :
     case OPT_RGB_03_B :
     case OPT_RGB_04_R :
     case OPT_RGB_04_G :
     case OPT_RGB_04_B :
     case OPT_RGB_05_R :
     case OPT_RGB_05_G :
     case OPT_RGB_05_B :
     case OPT_RGB_06_R :
     case OPT_RGB_06_G :
     case OPT_RGB_06_B :
     case OPT_RGB_07_R :
     case OPT_RGB_07_G :
     case OPT_RGB_07_B :
     case OPT_RGB_08_R :
     case OPT_RGB_08_G :
     case OPT_RGB_08_B :
     case OPT_RGB_09_R :
     case OPT_RGB_09_G :
     case OPT_RGB_09_B :
     case OPT_RGB_10_R :
     case OPT_RGB_10_G :
     case OPT_RGB_10_B :
     case OPT_RGB_11_R :
     case OPT_RGB_11_G :
     case OPT_RGB_11_B :
     case OPT_RGB_12_R :
     case OPT_RGB_12_G :
     case OPT_RGB_12_B :
     case OPT_RGB_13_R :
     case OPT_RGB_13_G :
     case OPT_RGB_13_B :
     case OPT_RGB_14_R :
     case OPT_RGB_14_G :
     case OPT_RGB_14_B :
     case OPT_RGB_15_R :
     case OPT_RGB_15_G :
     case OPT_RGB_15_B :
        if (set_int_from_arg(&x, 0, 255) == -1)
           break;
        col_table_p[(c - OPT_RGB_00_R) / 3][2 - ((c - OPT_RGB_00_R) % 3)] = x;
        break;

     case OPT_VIDEO :
        set_int_from_list(&crtc.video, offon_args);
        break;
     case OPT_VIDEO_DEPTH :
        set_int_from_list(&video.depth, video_depth_args);
        break;
     case OPT_VIDEO_TYPE :
#ifndef USE_OPENGL
        if (strcmp(e_optarg, "gl") == 0)
           break;
#endif
        set_int_from_list(&video.type, video_type_args);
        break;
#ifdef USE_OPENGL
     // OpenGL rendering
     case OPT_GL_ASPECT_BEE :
        if (video_gl_set_aspect_bee(float_arg) == -1)
           param_error_mesg();
        break;
     case OPT_GL_ASPECT_MON :
        if (video_gl_set_aspect_mon(float_arg) == -1)
           param_error_mesg();
        break;
     case OPT_GL_FILTER_FS :
        if (set_int_from_list(&video.filter_fs, gl_filter_args) == -1)
           break;
        video_gl_filter_update();
        break;
     case OPT_GL_FILTER_MAX :
        if (set_int_from_list(&video.filter_max, gl_filter_args) == -1)
           break;
        video_gl_filter_update();
        break;
     case OPT_GL_FILTER_WIN :
        if (set_int_from_list(&video.filter_win, gl_filter_args) == -1)
           break;
        video_gl_filter_update();
        break;
     case OPT_GL_MAX :
#ifdef MINGW
        xprintf("ubee512: '--gl-max' option is not currently supported under Windows.\n");
        exitstatus = 1;
#else
        set_int_from_list(&video.max, offon_args);
#endif
        break;
     case OPT_GL_VSYNC :
        set_int_from_list(&video.vsync, offon_args);
        break;
     case OPT_GL_WINPCT :
        if (video_gl_set_size_percent(int_arg) == -1)
           param_error_mesg();
        break;
     case OPT_GL_WINPIX :
        if (video_gl_set_size_pixels(int_arg) == -1)
           param_error_mesg();
        break;
#endif
    }
}

//==============================================================================
// Process options: Model.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_model (int c)
{
 char *piob7_args[] =
 {
  "pup",
  "vsync",
  "rtc",
  "net",
  ""
 };

 char *flash_args[] =
 {
  "off",
  "on",
  "v3",
  "v4",
  ""
 };

 char *flashrate_v3_args[] =
 {
  "20",
  "40",
  "80",
  "160",
  "320",
  "640",
  "1280",
  "2560",
  "w61",
  "w62",
  "w64",  // w63 and w64 are swapped!
  "w63",
  ""
 };

 char *flashrate_v4_args[] =
 {
  "20",
  "40",
  "80",
  "160",
  "320",
  "640",
  "1280",
  "2560",
  "w61",
  "w62",
  "w63",
  "w64",
  ""
 };

 char *hardware_args[] =
 {
  "wd2793",
  "sn76489",
  "sn76489init",
  ""
 };

 int pf;
 int res = 0;
 int x = 1;

 switch (c)
    {
     case OPT_BASIC :
     case OPT_BASICA :
        strncpy(modelc.basica, e_optarg, sizeof(modelc.basica));
        modelc.basica[sizeof(modelc.basica)-1] = 0;
        break;
     case OPT_BASICB :
        strncpy(modelc.basicb, e_optarg, sizeof(modelc.basicb));
        modelc.basicb[sizeof(modelc.basicb)-1] = 0;
        break;
     case OPT_BASICC :
        strncpy(modelc.basicc, e_optarg, sizeof(modelc.basicc));
        modelc.basicc[sizeof(modelc.basicc)-1] = 0;
        break;
     case OPT_BASICD :
        strncpy(modelc.basicd, e_optarg, sizeof(modelc.basicd));
        modelc.basicd[sizeof(modelc.basicd)-1] = 0;
        break;

     case OPT_BASRAM :
        modelc.basram = 1;
        break;

     case OPT_CHARROM :
        strncpy(modelc.charrom, e_optarg, sizeof(modelc.charrom));
        modelc.charrom[sizeof(modelc.charrom)-1] = 0;
        break;

     case OPT_COL :
        if (! modelx.alphap)
           modelx.colour = 1;
        break;
     case OPT_COL_TYPE :
        if (set_int_from_arg(&crtc.std_col_type, 0, 1) == -1)
           break;
        if (! modelx.alphap)
           modelx.colour = 1;
        if (emu.runmode)
           crtc_init();
        break;
     case OPT_COLPROM :
        strncpy(modelc.colprom, e_optarg, sizeof(modelc.colprom));
        modelc.basica[sizeof(modelc.colprom)-1] = 0;
        break;

     case OPT_DINT :
     case OPT_HINT :
        set_int_from_list(&modelx.halfint, offon_args);
        break;
     case OPT_PORT58H :
        emu.port58h_use = 1;
        break;
     case OPT_HARDWARE :
        while (1)
           {
            res = get_prefixed_argument(x++, &pf, hardware_args);
            if (res == -1)
               break;
            
            switch (res)
               {
                case 0 :
                   if (pf)
                      emu.hardware |= HW_WD2793;
                   else
                      emu.hardware &= ~HW_WD2793;
                   break;
                case 1 :
                case 2 :
                   modelx.sn76489an = pf ? res : 0;
                   break;
               }
           }  
        break;
     case OPT_HWFLASH :
        if (set_int_from_list(&x, flash_args) == -1)
           break;
        switch (x)
           {
            case 0:
               modelx.hwflash = HFNO;
               break;           // hardware flashing off
            case 1:
            case 2:
               modelx.hwflash = HFV3;
               break;           // on defaults to V3
            case 3:
               modelx.hwflash = HFV4;
               break;
            default:
               modelx.hwflash = HFNO;
               break;
           }
        // model-specific constraints on the value
        // 256tc has the version 4 flashing circuit
        if (emu.model == MOD_256TC && modelx.hwflash != HFNO)
           modelx.hwflash = HFV4;
        break;
     case OPT_HWFLASHR :
        x = 0;
        if (modelx.hwflash == HFV3)
           x = string_search(flashrate_v3_args, e_optarg);
        else if (modelx.hwflash == HFV4)
           x = string_search(flashrate_v4_args, e_optarg);
        if (crtc_set_flash_rate(x) == -1)
           param_error_mesg();
        break;
     case OPT_LMODEL :
        x = 0;
        while (model_args[x][0])
           xprintf("%s\n", model_args[x++]);
        exitstatus = -1;
        break;
     case OPT_LPEN :
        modelx.lpen = 1;
        break;
     case OPT_MODEL :
        if (set_int_from_list(&emu.model, model_args) == -1)
           break;
        memcpy(&modelx, &model_data[emu.model], sizeof(model_t));
        snprintf(temp_str, sizeof(temp_str), "UBEE_MODEL=%s", model_args[emu.model]);
        options_ubee512_envvar_set(temp_str);
        snprintf(temp_str, sizeof(temp_str), "UBEE_RAM=%d", modelx.ram);
        options_ubee512_envvar_set(temp_str);
        break;
     case OPT_MONO :
        if (modelx.alphap)
           {
            crtc.monitor = string_search(monitor_args, "g");  // can't be an error
            if (emu.runmode)
               {
                vdu_setcolourtable();
                crtc_set_redraw();
               }
           }
        else
           modelx.colour = 0;
        break;
     case OPT_NETRAM :
        modelc.netram = 1;
        break;
     case OPT_NETROM :
        strncpy(modelc.netrom, e_optarg, sizeof(modelc.netrom));
        modelc.netrom[sizeof(modelc.netrom)-1] = 0; break;
        break;

     case OPT_PAK0 :
     case OPT_PAK1 :
     case OPT_PAK2 :
     case OPT_PAK3 :
     case OPT_PAK4 :
     case OPT_PAK5 :
     case OPT_PAK6 :
     case OPT_PAK7 :
        if (roms_proc_pak_argument(c-OPT_PAK0, e_optarg) == -1)
           param_error_mesg();
        break;
     case OPT_PAKRAM :
        if (set_int_from_arg(&x, 0, 7) == -1)
           break;
        modelc.pakram[x] = 1;
        break;

     case OPT_PCG :
        if ((int_arg < 2) || (int_arg > 32) || (int_arg % 2))
           param_error_mesg();
        else
           if (modelx.alphap)
              modelx.pcg = int_arg / 2;
        break;
     case OPT_PIOB7 :
        set_int_from_list(&modelx.piob7, piob7_args);
        break;

     case OPT_ROM1 :
        strncpy(modelc.rom1, e_optarg, sizeof(modelc.rom1));
        modelc.rom1[sizeof(modelc.rom1)-1] = 0;
        break;
     case OPT_ROM2 :
        strncpy(modelc.rom2, e_optarg, sizeof(modelc.rom2));
        modelc.rom2[sizeof(modelc.rom2)-1] = 0;
        break;
     case OPT_ROM3 :
        strncpy(modelc.rom3, e_optarg, sizeof(modelc.rom3));
        modelc.rom3[sizeof(modelc.rom3)-1] = 0;
        break;

     case OPT_ROM256K :
        strncpy(modelc.rom256k, e_optarg, sizeof(modelc.rom256k));
        modelc.rom256k[sizeof(modelc.rom256k)-1] = 0;
        break;

     case OPT_SRAM :
        if ((int_arg < 0) || (int_arg > 32))
           param_error_mesg();
        else
           if (modelx.rom)
              modelx.ram = int_arg;
        break;
     case OPT_SRAM_BACKUP :
        set_int_from_list(&memmap.backup, offon_args);
        break;
     case OPT_SRAM_FILE :
        strncpy(memmap.filepath, e_optarg, sizeof(memmap.filepath));
        memmap.filepath[sizeof(memmap.filepath)-1] = 0;
        break;        
     case OPT_SRAM_LOAD :
        set_int_from_list(&memmap.load, offon_args);
        break;
     case OPT_SRAM_SAVE :
        set_int_from_list(&memmap.save, offon_args);
        break;

     case OPT_SYS :
        strncpy(modelc.systname, e_optarg, sizeof(modelc.systname));
        modelc.systname[sizeof(modelc.systname)-1] = 0;
        break;
     case OPT_VDU :
        if ((int_arg != 2) && (int_arg != 8))
           param_error_mesg();
        else
           if (modelx.alphap)
              modelx.vdu = (int_arg / 2) - 1;
        break;
    }
}

//==============================================================================
// Process options: On Screen Display (OSD)
//
//   pass: int c                        option number
//         char *argv[]
// return: void
//==============================================================================
static void options_osd (int c)
{
 char *osd_args[] =
 {
  "all",
  "animate",
  ""
 };

 int pf;
 int res = 0;
 int x = 1;

 switch (c)
    {
     case OPT_OSD :
        while (1)
           {
            res = get_prefixed_argument(x++, &pf, osd_args);
            if (res == -1)
               break;
            osd_proc_osd_args(res, pf);
           }
        break;
     case OPT_OSD_CON_POS :
        if (osd_set_console_position(e_optarg) == -1)
           param_error_mesg();        
        break;
     case OPT_OSD_CON_SIZE :
        if (osd_set_console_size(e_optarg) == -1)
           param_error_mesg();        
        break;
     case OPT_OSD_CURSOR_RATE :
        if (set_int_from_arg(&x, 0, 5000) == -1)
           break;
        osd_set_cursor(x);
        break;
     case OPT_OSD_LIST :
        osd_list_schemes();
        break;
     case OPT_OSD_SCHEME :
        if (osd_set_scheme(e_optarg) == -1)
           param_error_mesg();        
        break;
     case OPT_OSD_SET_BTN_MAIN :
     case OPT_OSD_SET_BTN_TEXT :
     case OPT_OSD_SET_DIA_MAIN :
     case OPT_OSD_SET_DIA_TEXT :
     case OPT_OSD_SET_WID_ICON :
     case OPT_OSD_SET_WID_MAIN :
     case OPT_OSD_SET_WID_TEXT :
        if (osd_set_colour(e_optarg, c) == -1)
           param_error_mesg();
        break;
    }
}

//==============================================================================
// Process options: Information output.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_information (int c)
{
 int i;
 int x = 0;
 char vers[20];

 switch (c)
    {
     case OPT_CONIO :
        console.force_stdout = 1;
        break;
     case OPT_HELP :
     case OPT_USAGE :
        options_usage();
        exitstatus = -2;
        break;
     case OPT_LCON :
        for (i = list_config_start; i < ndefsc; i++)
           xprintf("%s\n", ndefsv[i]);
        exitstatus = -1;
        break;
     case OPT_LCONW :
        i = list_config_start;
        x = 0;
        while (i < ndefsc)
           {
            xprintf("%-16s", ndefsv[i++]);
            x++;
            if ((x % 5) == 0)
               xprintf("\n");
           }
        if ((x % 5) != 0)
           xprintf("\n");
        exitstatus = -1;
        break;
     case OPT_LCONS :
        if (set_int_from_arg(&x, 1, ndefsc) == -1)
           break;
        list_config_start = x - 1;
        break;
     case OPT_VERSION :
        xprintf("%s\n", APPVER);
        sdlv = SDL_Linked_Version();
        xprintf("SDL %u.%u.%u\n", sdlv->major, sdlv->minor, sdlv->patch);
        z80api_get_version(vers, sizeof(vers));
        xprintf("%s\n", vers);
        exitstatus = 1;
        break;
    }
}

//==============================================================================
// Process options: Parallel printer.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_parallel_printer (int c)
{
 switch (c)
    {
     case OPT_PRINT :
        printer_b_open(e_optarg, emu.runmode);
        break;
     case OPT_PRINT_CLOSE :
        printer_b_close();
        break;
     case OPT_PRINTA :
        printer_a_open(e_optarg, emu.runmode);
        break;
     case OPT_PRINTA_CLOSE :
        printer_a_close();
        break;
    }
}

//==============================================================================
// Process options: Serial port.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_serial_port (int c)
{
 int x = 0;

 switch (c)
    {
     case OPT_BAUD :
        if (set_int_from_arg(&x, 1, 38400) == -1)
           break;
        serial.tx_baud = x;
        serial.rx_baud = x;
        if (emu.runmode)
           serial_config(emu.cpuclock);
        break;
     case OPT_BAUDRX :
        if (set_int_from_arg(&serial.rx_baud, 1, 38400) == -1)
           break;
        if (emu.runmode)
           serial_config(emu.cpuclock);
        break;
     case OPT_BAUDTX :
        if (set_int_from_arg(&serial.tx_baud, 1, 38400) == -1)
           break;
        if (emu.runmode)
           serial_config(emu.cpuclock);
        break;
     case OPT_COMS :
        if (serial_open(e_optarg, 0, emu.runmode) == -1)
           param_error_mesg();
        break;
     case OPT_COMS_CLOSE :
        serial_close(0);
        break;
     case OPT_DATAB :
        if (set_int_from_arg(&serial.databits, 5, 8) == -1)
           break;
        if (emu.runmode)
           serial_config(emu.cpuclock);
        break;
     case OPT_STOPB :
        if (set_int_from_arg(&serial.stopbits, 1, 2) == -1)
           break;
        if (emu.runmode)
           serial_config(emu.cpuclock);
        break;
    }
}

//==============================================================================
// Process options: Sound emulation.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_sound (int c)
{
 int x;
 
 char *sound_args[] =
 {
  "off",
  "prop",
  "normal",
  ""
 };

 switch (c)
    {
     case OPT_SOUND :           // deprecated --sound=off
        set_int_from_list(&x, sound_args);
        if (x == -1)
           break;
        if (x)
           {
            audio.mode = x;
            break;
           }
        audio.mute = 1;
        xprintf("ubee512: Option `--sound=off' now sets --snd-mute=on.\n");
        break;
     case OPT_SND_ALG1 :	/* deprecated */
        break;
     case OPT_SND_FREQ :
        set_int_from_arg(&audio.frequency, 5512, 176400);
        break;
     case OPT_SND_FREQADJ :	/* deprecated */
        break;
     case OPT_SND_FREQLOW :	/* deprecated */
        break;
     case OPT_SND_HOLDOFF :	/* deprecated */
        break;
     case OPT_SND_HQ :
        audio.samples = 2048;
        audio.frequency = 88200;
        break;
     case OPT_SND_MUTE :
        set_int_from_list(&audio.mute, offon_args);
        break;
     case OPT_SND_SAMPLES :
        if (((int_arg & (int_arg - 1)) != 0) || (int_arg > 16384)) // check is power of 2
           param_error_mesg();
        else
           audio.samples = int_arg;    
        break;
     case OPT_SND_VOLUME :
     case OPT_VOL :
        if (set_int_from_arg(&audio.vol_percent, 0, 100) == -1)
           break;
        audio_set_master_volume(audio.vol_percent);
        break;
    }
}

//==============================================================================
// Process options: Speed related.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_speed (int c)
{
 switch (c)
    {
     case OPT_CLOCK :
     case OPT_XTAL :
        if (set_float_from_arg(&modelx.cpuclock, 0.0, 1E+12) == -1)
           break;
        if (emu.runmode)
           set_clock_speed(modelx.cpuclock, 0, 0);
        break;
     case OPT_CLOCK_DEF :
        set_float_from_arg(&emu.cpuclock_def, 0.0, 1E+12);
        break;
     case OPT_FRATE :
        if (set_int_from_arg(&emu.framerate, 1, 1000000L) == -1)
           break;
        if (emu.runmode)
           set_clock_speed(modelx.cpuclock, emu.z80_divider, emu.framerate);
        break;
     case OPT_MAXCPULAG :
        set_int_from_arg(&emu.maxcpulag, 0, MAXINT);
        break;
     case OPT_VBLANK :
        set_int_from_arg(&crtc.vblank_method, 0, 1);
        break;
     case OPT_SPEEDSEL :
        set_int_from_arg(&modelx.speed, 0, 1);
        break;
     case OPT_TURBO :
        if (! e_optarg[0])
           emu.turbo = 1;
        else
           set_int_from_list(&emu.turbo, offon_args);
        if (! emu.turbo)
           turbo_reset();
        break;
     case OPT_Z80DIV :
        if (set_int_from_arg(&emu.z80_divider, 1, 5000) == -1)
           break;
        if (emu.runmode)
           set_clock_speed(modelx.cpuclock, emu.z80_divider, 0);
        break;
    }
}

//==============================================================================
// Process options: Tape port.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_tape (int c)
{
 int x = 0;

 switch (c)
    {
     case OPT_TAPEI :
        if (tape_check(tape.tapeo, e_optarg) == 0)
           tape_i_open(e_optarg, emu.runmode);
        else
           param_error_mesg();
        break;
     case OPT_TAPEI_CLOSE :
        tape_i_close();
        break;
     case OPT_TAPE_DET :
        if (set_float_from_arg(&tape.detect, 0.0, 100) == -1)
           break;
     case OPT_TAPEO :
        if (tape_check(tape.tapei, e_optarg) == 0)
           tape_o_open(e_optarg, emu.runmode);
        else
           param_error_mesg();
        break;
     case OPT_TAPEO_CLOSE :
        tape_o_close();
        break;
     case OPT_TAPESAMP :
        if (set_int_from_arg(&tape.orate, 1, 1000000L) == -1)
           break;
        if (emu.runmode)
           tape_config_out(modelx.cpuclock);
        break;
     case OPT_TAPEVOL :
        if (set_int_from_arg(&x, 0, 100) == -1)
           break;
        else
           tape.olevel = (int)(127 * (x / 100.0));
        break;

     case OPT_TAPFILE_LIST :
        tapfile_list(e_optarg);
        break;
     case OPT_TAPFILEI :
        if (tapfile_check(tapfile.tapeo, e_optarg) == 0)
           tapfile_i_open(e_optarg, emu.runmode);
        else
           param_error_mesg();
        break;
     case OPT_TAPFILEI_CLOSE :
        tapfile_i_close();
        break;
     case OPT_TAPFILEO :
        if (tapfile_check(tapfile.tapei, e_optarg) == 0)
           tapfile_o_open(e_optarg, emu.runmode);
        else
           param_error_mesg();
        break;
     case OPT_TAPFILEO_CLOSE :
        tapfile_o_close();
        break;
    }
}

//==============================================================================
// Process options: Real Time Clock (RTC) emulation and time.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_rtc (int c)
{
 switch (c)
    {
     case OPT_CENTURY :
        set_int_from_arg(&emu.century, 0, 255);
        break;
     case OPT_RTC :
        set_int_from_arg(&modelx.rtc, 0, 1);
        break;
    }
}

//==============================================================================
// Process options: Joystick emulation.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_joystick (int c)
{
 switch (c)
    {
     case OPT_JS :
        set_int_from_arg(&joystick.used, -1, 127); // yes -1 !
        break;

     case OPT_JS_AXIS :
        set_int_from_list(&joystick.axis_used, offon_args);
        break;
     case OPT_JS_AXISB :
        set_int_from_arg(&joystick.axis_buttons, 0, 255);
        break;
     case OPT_JS_AXISL :
        set_int_from_arg(&joystick.axis_level, 1, 32767);
        break;

     case OPT_JS_HAT :
        set_int_from_list(&joystick.hat_used, offon_args);
        break;
     case OPT_JS_HATB :
        set_int_from_arg(&joystick.hat_buttons, 0, 255);
        break;

     case OPT_JS_SHIFT :
        set_int_from_arg(&joystick.shift_button, -1, 127);
        break;

     case OPT_JS_CLEAR :
        joystick_mbjoy_clear();
        break;

     case OPT_JS_MBEE :
        set_int_from_list(&joystick.mbee, offon_args);
        break;

     case OPT_JS_UP :
        if (joystick_mbjoy_set_action(JOY_MB_UP, e_optarg))
           param_error_mesg();
        break;
     case OPT_JS_RIGHT :
        if (joystick_mbjoy_set_action(JOY_MB_RIGHT, e_optarg))
           param_error_mesg();
        break;
     case OPT_JS_DOWN :
        if (joystick_mbjoy_set_action(JOY_MB_DOWN, e_optarg))
           param_error_mesg();
        break;
     case OPT_JS_LEFT :
        if (joystick_mbjoy_set_action(JOY_MB_LEFT, e_optarg))
           param_error_mesg();
        break;
     case OPT_JS_FIRE :
        if (joystick_mbjoy_set_action(JOY_MB_FIRE, e_optarg))
           param_error_mesg();
        break;
     case OPT_JS_PLAY1 :
        if (joystick_mbjoy_set_action(JOY_MB_PLAY1, e_optarg))
           param_error_mesg();
        break;
     case OPT_JS_PLAY2 :
        if (joystick_mbjoy_set_action(JOY_MB_PLAY2, e_optarg))
           param_error_mesg();
        break;
     case OPT_JS_SPARE :
        if (joystick_mbjoy_set_action(JOY_MB_SPARE, e_optarg))
           param_error_mesg();
        break;

     case OPT_JS_CLIST :
        joystick_kbjoy_listcommands();
        exitstatus = -1;
        break;
     case OPT_JS_KLIST :
        joystick_kbjoy_listkeys();
        exitstatus = -1;
        break;
     case OPT_JS_KBD :
        set_int_from_list(&joystick.kbd, offon_args);
        break;
     case OPT_JS_KK :
        if (joystick_kbjoy_key(e_optarg))
           param_error_mesg();
        break;
     case OPT_JS_KB :
        if (joystick_kbjoy_button(int_arg))
           param_error_mesg();
        break;
     case OPT_JS_KKB :
        if (joystick_kbjoy_keybuttons(e_optarg))
           param_error_mesg();
        break;
     case OPT_JS_KSET :
        if (joystick_kbjoy_set(int_arg, e_optarg))
           param_error_mesg();
        else
           joystick_kbjoy_select(int_arg, e_optarg);
        break;
     case OPT_JS_KSEL :
        if (joystick_kbjoy_select(int_arg, e_optarg))
           param_error_mesg();
        break;
    }
}

//==============================================================================
// Process options: Mouse emulation.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_mouse (int c)
{
 switch (c)
    {
     case OPT_MOUSE :
        set_int_from_list(&mouse.active, offon_args);
        break;
    }
}

//==============================================================================
// Process options: Application dependent.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_application (int c)
{
 switch (c)
    {
     case OPT_FILE_APP :
        strncpy(func.file_app, e_optarg, sizeof(func.file_app));
        func.file_app[sizeof(func.file_app)-1] = 0;
        break;
     case OPT_FILE_EXEC :
        set_int_from_arg(&func.file_exec, 0, 65535);
        break;
     case OPT_FILE_EXIT :
        set_int_from_list(&func.file_exit, offon_args);
        break;
     case OPT_FILE_LIST :
        if (func.file_list_count == FILE_LIST_ENTRIES)
           param_error_mesg();
        else
           {
            strncpy(func.file_list[func.file_list_count], e_optarg_q, FILE_STR_SIZE);
            func.file_list[func.file_list_count][FILE_STR_SIZE-1] = 0;
            convert_slash(func.file_list[func.file_list_count++]);
           }
        break;
     case OPT_FILE_LIST_Q :
        if (func.file_list_count == FILE_LIST_ENTRIES)
           param_error_mesg();
        else
           {
            snprintf(func.file_list[func.file_list_count], FILE_STR_SIZE, "\"%s\"", e_optarg);
            func.file_list[func.file_list_count][FILE_STR_SIZE-1] = 0;
            convert_slash(func.file_list[func.file_list_count++]);
           }
        break;
     case OPT_FILE_LOAD :
        set_int_from_arg(&func.file_load, 0, 65535);
        break;
     case OPT_FILE_RUN :
        strncpy(func.file_run, e_optarg, sizeof(func.file_run));
        func.file_run[sizeof(func.file_run)-1] = 0;
        break;
    }
}

//==============================================================================
// Process options: Parallel port devices.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_parallel_port(int c)
{
 char *parallel_port_args[] =
 {
  "none",
  "printer",
  "joystick",
  "beetalker",
  "beethoven",
  "dac",
  "compumuse",
  ""
 };

 int x;

 switch (c)
    {
     case OPT_PARALLEL_PORT :
        if (set_int_from_list(&x, parallel_port_args) < 0)
           break;               // error
        switch (x)
           {
            case 0: // no device
               pio_porta_connect(NULL);
               break;
            case 1: // parallel printer
               pio_porta_connect(&printer_ops);
               break;
            case 2: // joystick
               pio_porta_connect(&joystick_ops);
               break;
            case 3: // BeeTalker
               pio_porta_connect(&beetalker_ops);
               break;
            case 4: // BeeThoven
               pio_porta_connect(&beethoven_ops);
               break;
            case 5: // DAC audio device
               pio_porta_connect(&dac_ops);
               break;
            case 6: // Compumuse audio device
               pio_porta_connect(&compumuse_ops);
               break;
            default:
               break;
           }
        break;
    }
}

//==============================================================================
// Process options: Quickload support.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_quickload (int c)
{
 switch (c)
    {
     case OPT_QL_LIST :
        if ((exitstatus = quickload_list(e_optarg)) == -1)
           param_error_mesg();
        break;
     case OPT_QL_LOAD :
        if ((exitstatus = quickload_load(e_optarg)) == -1)
           param_error_mesg();
        break;
     case OPT_QL_X :
        exitstatus = quickload_execute();
        break;
#ifdef USE_ARC
     case OPT_QLA_ARC :
        if ((exitstatus = quickload_open_arc(e_optarg)) == -1)
           param_error_mesg();
        break;
     case OPT_QLA_DIR :
        if ((exitstatus = quickload_dir_arc(e_optarg)) == -1)
           param_error_mesg();
        break;
     case OPT_QLA_LIST :
        if ((exitstatus = quickload_list_arc(e_optarg)) == -1)
           param_error_mesg();
        break;
     case OPT_QLA_LOAD :
        if ((exitstatus = quickload_load_arc(e_optarg)) == -1)
           param_error_mesg();
        break;
#endif
    }
}

//==============================================================================
// Process options: Compumuse parameters.
//
//   pass: int c                        option number
// return: void
//==============================================================================
static void options_compumuse (int c)
{
 int x = 0;

 switch (c)
    {
     case OPT_COMPUMUSE_INIT :
        compumuse.init = 1;
        break;
     case OPT_COMPUMUSE_CLOCK :
        if (set_int_from_arg(&x, 0, 4) == -1)
           break;
        if (x == 0 || x == 3)
           break;
        compumuse_clock(x * 1000000);
        break;
    }
}

//==============================================================================
// Process options.
//
// If any non-options are found these are processed last (see man getopt).
// If any options processed causes exiting the options module before those
// non-options are reached then no error will be seen.
//
// Option types
// ------------
// OPT_RUN
// Option is allowed to be processed from the command line and during the
// running of the emulator.
//
// OPT_RTO
// Option is allowed only when the emulator is running (Run Time Only).
//
// OPT_Z
// Option can only be used on the command line or in the ubee512rc startup
// configuration file.
//
// In the "short options" string passed in xgetopt() a single ':' following
// an option letter indicates an argument for the optionis required.  Two
// '::' indicates an optional argument and no colon indicates no option.
// The optarg string will be empty if an option is set to 'optional
// argument' type and no argument was sepecified.
//
// 256 option groups are possible (0x00nn-0xffnn) with each each group
// allowed upto 256 options.
//
//   pass: int argc                     number of arguments
//         char *argv[]                 pointer to pointer to arguments
// return: void
//==============================================================================
static void options_getopt (int argc, char *argv[])
{
 int i;
 int c;
 int short_option;
 int allowed;

 long_index = 0;

 // clear these each time the options are processed
#ifdef USE_LIBDSK
 use_driver_type[0] = 0;
 use_format_type[0] = 0;
 side1as0 = 0;
 cpm3 = 0;
 dstep = 0;
 dstep_hd = 0;
#endif

 // the xgetopt variables must be reset each time this function is re-entered.
 xgetopt_init();

 while (! exitstatus)
    {
     c = xgetopt_long(argc, argv, "f::ht::a:b:c:d:m:v:x:z:", long_options, &long_index);

     // print any error messages generated by xgetopt_long()
     if (opterr_msg[0])
        {
         exitstatus = 1;
         xprintf("%s", opterr_msg);
         xprintf(TRY_MESG, argv[0], argv[0], argv[0]);
        }

     // detect the end of the options
     if (c == -1)
        break;

     // translate short options to a long option number
     short_option = (c < 0x100) * c;
     if (short_option)
        {
         i = 0;
         while ((short_options[i].option) && (short_options[i].option != c))
            i++;
         if (short_options[i].option == c)
            c = short_options[i].longno;
        }

     // check if not in run mode and the option is only allowed when in run mode
     if ((! emu.runmode) && (c & OPT_RTO))
        {
         if (short_option)
            xprintf("ubee512: option `-%c' is only supported in run mode.\n", short_option);
         else
            xprintf("ubee512: option `--%s' is only supported in run mode.\n", long_options[long_index].name);
         exitstatus = -1;
         break;  // abort the remainder of the command line
        }

     // check if processing an option is allowed when in run mode
     allowed = (! emu.runmode) || (c < 0x100) ||  ((c & (OPT_RUN | OPT_RTO)) != 0);

     if (! allowed)
        {
         if (runmode_warn)
            {
             if (short_option)
                xprintf("ubee512: WARNING ! option `-%c' not supported in run mode (ignored).\n", short_option);
             else
                xprintf("ubee512: WARNING ! option `--%s' not supported in run mode (ignored).\n", long_options[long_index].name);
             c = 0;  // keep going but don't process the option
            }
         else
            {
             if (short_option)
                xprintf("ubee512: option `-%c' not supported in run mode.\n", short_option);
             else
                xprintf("ubee512: option `--%s' not supported in run mode.\n", long_options[long_index].name);
             exitstatus = -1;
             break;  // abort the remainder of the command line
            }
        }

     c &= 0x0000ffff;  // strip off the flag bits

     // don't process options if currently off (except for --if-x options)
     if (((c < OPT_GROUP_CONDITIONAL) || (c > (OPT_GROUP_CONDITIONAL + 0xff))) && ((c != '?') && (c != ':')))
        {
         if (if_pos >= OPTIONS_MAXCOND)
            {
             xprintf("ubee512: all %d levels of conditionals used up!\n", OPTIONS_MAXCOND);
             exitstatus = -1;
             c = 0;
            }
         if ((if_state[if_pos]) == 0)
            c = 0;
        }

     if_state_prev = if_state[if_pos];

     if (c)
        {
         // extract environment variables from the argument
         extract_environment_vars(optarg, e_optarg, e_optarg_q);
         toupper_string(e_optarg_x, e_optarg);

#if 0
         xprintf("optarg=%s, e_optarg=%s e_optarg_q=%s\n", optarg, e_optarg, e_optarg_q);
#endif

         // integer and floating point conversions, a -1 or -1.0 value is
         // returned if an error ocurrs in the conversion.
         int_arg = get_integer_value(e_optarg);
         float_arg = get_float_value(e_optarg);

         switch (c & 0xff00)  // 256 possible groups each with upto 256 options
            {
             case OPT_GROUP_SHORT : // Short options
                options_short(c, argv);
                break;
             case OPT_GROUP_CONTROL : // Control related
                options_control(c);
                break;
             case OPT_GROUP_CONDITIONAL : // Conditional option parsing
                options_conditional(c);
                break;
             case OPT_GROUP_DEBUGGING : // Debugging tools
                options_debugging(c);
                break;
             case OPT_GROUP_DISKDRIVES : // Disk drive images
                options_disks(c);
                break;
             case OPT_GROUP_DISPLAY : // Display related
                options_display(c);
                break;
             case OPT_GROUP_MODEL : // Model emulation
                options_model(c);
                break;
             case OPT_GROUP_OSD : // On Screen Display (OSD)
                options_osd(c);
                break;                
             case OPT_GROUP_INFORMATION : // Information output
                options_information(c);
                break;
             case OPT_GROUP_PARALLEL_PRINTER : // Parallel printer emulation
                options_parallel_printer(c);
                break;
             case OPT_GROUP_SERIAL : // Serial port
                options_serial_port(c);
                break;
             case OPT_GROUP_SOUND : // Sound emulation
                options_sound(c);
                break;
             case OPT_GROUP_SPEED : // Speed related
                options_speed(c);
                break;
             case OPT_GROUP_TAPE : // Tape port
                options_tape(c);
                break;
             case OPT_GROUP_RTC : // Real Time Clock (RTC) emulation
                options_rtc(c);
                break;
             case OPT_GROUP_JOYSTICK : // Joystick emulation
                options_joystick(c);
                break;
             case OPT_GROUP_MOUSE : // Mouse emulation
                options_mouse(c);
                break;
             case OPT_GROUP_APPLICATION : // Application dependent
                options_application(c);
                break;
             case OPT_GROUP_PARALLEL_PORT : // parallel port devices
                options_parallel_port(c);
                break;
             case OPT_GROUP_QUICKLOAD : // Quickload support
                options_quickload(c);
                break;
             case OPT_GROUP_COMPUMUSE : // Compumuse options
                options_compumuse(c);
                break;
             case OPT_GROUP_RESERVED : // Reserved
                break;
             default :
                exitstatus = 1;
                break;
            }
        }
    }

 // Check if any other arguments are incorrectly specified
 if ((optind < argc) && (exitstatus == 0) && (args_err_flags & 0x01))
    {
     exitstatus = 1;
     xprintf("%s: %d additional arguments were specified that are not recognised:\n", argv[0], argc - optind);
     xprintf("%s: ", argv[0]);
     while (optind < argc)
        xprintf("%s ", argv[optind++]);
     xprintf("\n");
     xprintf(TRY_MESG, argv[0], argv[0], argv[0]);
    }
}

//==============================================================================
// Allocate memory.
//
//   pass: int size
// return: char *                       result of memory allocate
//==============================================================================
static char *options_malloc (int size)
{
 char *ptr;

 if (! (ptr = malloc(size)))
    {
     exitstatus = 1;
     xprintf("options_malloc: unable to malloc %d bytes of memory\n", size);
    }

 return ptr;
}

//==============================================================================
// Build a list of definitions found in the configuration file.
//
//   pass: void
// return: void
//==============================================================================
static void options_buildlist (void)
{
 int l;
 char s[OPTIONS_SIZE];

 if (! fp)
    return;

 fseek(fp, 0, SEEK_SET);

 while ((l = file_readline(fp, s, OPTIONS_SIZE)))
    {
     if ((s[0] == '[') && (s[l-1] == ']'))
        {
         if (! (ndefsv[ndefsc] = options_malloc(l-1)))  // space for name + 0
            return;
         strncpy(ndefsv[ndefsc], &s[1], l-2);
         ndefsv[ndefsc++][l-2] = 0;
        }
    }
}

//==============================================================================
// Find a definitions entry in the names list.
//
//   pass: const char *name             name of the definition to look for
// return: int                          0 if no match found, else index +1
//==============================================================================
static int options_findentry (const char *name)
{
 int i = 0;

 if (! fp)
    return 0;

 while ((i < ndefsc) && (strcmp(ndefsv[i], name) != 0))
    i++;

 if (i < ndefsc)
    return i+1;

 return 0;
}

//==============================================================================
// Get options for the named entry from the configuration file.
//
//   pass: const char *name             definition name
//         char *options                return options found for definition
//                                      string if entry not found or empty
// return: void
//==============================================================================
static void options_getoptstr (const char *name, char *options)
{
 int l;
 int finish = 0;

 char s[OPTIONS_SIZE];

 if (! fp)
    return;

 fseek(fp, 0, SEEK_SET);

 options[0] = 0;

 // find the definition entry
 while ((! finish) && (l = file_readline(fp, s, OPTIONS_SIZE)))
    {
     if ((s[0] == '[') && (s[l-1] == ']'))
        {
         strncpy(s, &s[1], l-2);
         s[l-2] = 0;
         finish = (strcmp(s, name) == 0);
        }
    }

 if (! finish)
    return;

 finish = 0;

 // concatenate all the definition entries into one string
 while (! finish)
    {
     if ((l = file_readline(fp, s, OPTIONS_SIZE)))
        {
         if (! (finish = ((s[0] == '[') && (s[l-1] == ']'))))
            {
             l = OPTIONS_SIZE - (strlen(options) + 1);
             if (l > 0)
                {
                 strcat(options, " ");
                 strncat(options, s, l-1);
                }
            }
        }
     else
        finish = 1;
    }

 return;
}

//==============================================================================
// Parse the options from the config file entry string and place into a
// xargv variable.
//
//   pass: const char *options          options to be parsed
// return: void
//==============================================================================
static void options_parse (const char *options)
{
 int i = 0;
 int l;

 char s[OPTIONS_SIZE];

 while (options[i])
    {
     while ((options[i]) && (options[i] <= ' '))  // move past white space
        i++;

     l = 0;
     while ((options[i]) && (options[i] != ' ') && (options[i] != '"'))
        {
#ifdef MINGW
#else
         if ((options[i] == '\\') && (! emu.slashconv))
            i++;
#endif
         if (options[i])
            {
             s[l++] = options[i];
             i++;
            }
        }
     if (options[i] == '"')
        {
         i++;
         while ((options[i]) && (options[i] != '"'))
            {
             s[l++] = options[i];
             i++;
            }
         i++;
        }
     s[l] = 0;

     if (! (xargv[xargc] = options_malloc(l+1)))  // space for argument + 0
        return;
     strcpy(xargv[xargc++], s);
    }
}

//==============================================================================
// Process early options found on the command line, these are special options
// and if used must be declared before all others.  These options will be
// ignored by the options_getopt() function.
//
// --account option:
// By default the account is @UBEE_USERHOME@/.ubee512 on Unices and the
// location of the executed ubee512.exe on Windows. This option may be used
// when an alternative account location is required. If this option is used
// it must be the first option on the command line.
//
// --config option:
// By default the ubee512rc file found in the home account is used unless
// another file is specified. If this option is used it must be the first or
// second option on the command line.
//
// Note: The number of arguments (argc) consists of the number of command
// line space delimited arguments (except where quoted), the program name is
// also included in the count.  The first argument is therefore the second
// argv pointer.
//
//   pass: char *s                      pointer to string
//         int argc                     number of arguments
//         char *argv[]                 pointer to pointer to arguments
// return: int                          0 if no error, 1 if error, neg if exit
//==============================================================================
static int options_early (char *s, int argc, char *argv[])
{
 int argv_pos = 1;

 // test if --account is the first command line option, this is used to set an
 // alternative '.ubee512' home account directory.
 if (argc > 1)
    {
     // option & arg may be 1 entry (i.e. --option=arg)
     if (strncmp("--account=", argv[argv_pos], 10) == 0)
        {
         extract_environment_vars(&argv[argv_pos][10], userhome, e_optarg_q);
         emu.home_account_set = 1;
         argv_pos++;
         argc--;
        }
     else
        // option & arg may be broken up over 2 entries (i.e. --option arg)
        if (strcmp("--account", argv[argv_pos]) == 0)
           {
            argv_pos++;
            extract_environment_vars(argv[argv_pos], userhome, e_optarg_q);
            emu.home_account_set = 1;
            argv_pos++;
            argc -= 2;
           }
    }

 // set all the account paths now that we know the home account to be used.
 if (set_account_paths())
    return 1;

 // test if --config is the next command line option,  this is used
 // to set an alternative configuration file.
 if (argc > 1)
    {
     // option & arg may be 1 entry (i.e. --option=arg)
     if (strncmp("--config=", argv[argv_pos], 9) == 0)
        extract_environment_vars(&argv[argv_pos][9], s, e_optarg_q);
     else
        // option & arg may be broken up over 2 entries (i.e. --option arg)
        if (strcmp("--config", argv[argv_pos]) == 0)
           extract_environment_vars(argv[++argv_pos], s, e_optarg_q);
    }

// xprintf("config=%s  userhome=%s\n", s, userhome);

 return 0;
}

//==============================================================================
// Process all options from the pointer list and the configuration file.
//
// If processing in run mode then the [global-start] and [global-end]
// sections will be ignored.  Other sections in the configuration file will
// be processed if specified.
//
//   pass: int argc                     number of arguments
//         char *argv[]                 pointer to pointer to arguments
// return: int                          0 if no error, 1 if error, neg if exit
//==============================================================================
int options_process (int argc, char *argv[])
{
 int i;

 char filepath[SSIZE1];
 char file_options[OPTIONS_SIZE];
 char file_section[40];

 exitstatus = 0;

 // close any open configuration file
 if (fp)
    fclose(fp);

 // free existing pointers in use for section entry names list
 while (ndefsc)
    free(ndefsv[--ndefsc]);

 // free existing pointers in use for arguments
 while (xargc)
    free(xargv[--xargc]);

 // set conditionals state
 if_pos = 0;
 if_state[if_pos] = 1;  // true at the base level

 // set default configuration file to use if none was set previously
 if (! config_file[0])
    strcpy(config_file, "ubee512rc");

 // process any special early options from the command line, the
 // configuration file may be changed with an option here.
 i = options_early(config_file, argc, argv);
 if (i)
    return i;

 // if use no configuration file has been requested then NULL the file pointer
 if (strcmp("none", config_file) == 0)
    fp = NULL;
 else
    {
     fp = open_file(config_file, userhome_confpath, filepath, "r");
     if (strlen(config_file) && (! fp) && (emu.verbose))
        xprintf("options_process: Configuration file not found: %s\n", config_file);
    }

 // build a list of the definitions contained in the configuration file
 options_buildlist();
 if (exitstatus)
    return exitstatus;

 // first argument will be the program name and must be inserted first
 if (! (xargv[xargc] = options_malloc(strlen(argv[0])+1))) // space for argument + 0
    return exitstatus;

 strcpy(xargv[xargc++], argv[0]);

 // process the configuration file's 'global-start' or 'global-start-runmode' options
 if (emu.runmode)
    strcpy(file_section, "global-start-runmode");
 else
    strcpy(file_section, "global-start");
 if (options_findentry(file_section))
    {
     options_getoptstr(file_section, file_options);
     if (strlen(file_options))
        options_parse(file_options);
    }
 if (exitstatus)
    return exitstatus;

 // process all the command line and configuration file arguments after the program name
 for (i = 1; i < argc; i++)
    {
     // if it's a command line option (-) or the entry is not a section entry
     if ((*argv[i] == '-') || (! options_findentry(argv[i])))
        {
         if (! (xargv[xargc] = options_malloc(strlen(argv[i])+1)))  // space for argument + 0
            return exitstatus;

         strcpy(xargv[xargc++], argv[i]);
        }
     else
        {
         // otherwise if a section entry grab all the options from the file
         if (options_findentry(argv[i]))
            {
             options_getoptstr(argv[i], file_options);
             if (strlen(file_options))
                options_parse(file_options);
            }
        }
    }

 // process the configuration file's 'global-end' or 'global-end-runmode' options
 if (emu.runmode)
    strcpy(file_section, "global-end-runmode");
 else
    strcpy(file_section, "global-end");
 if (options_findentry(file_section))
    {
     options_getoptstr(file_section, file_options);
     if (strlen(file_options))
        options_parse(file_options);
    }

 // process all the options
 if (! exitstatus)
    options_getopt(xargc, xargv);

 // report any options information if requested
 options_modio_info();

 return exitstatus;
}
