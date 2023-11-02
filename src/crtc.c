//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              6545 CRTC module                              *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Emulate the 6545 CRT for monochrome, standard colour and Premium/256TC
// colour models.
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
// - Refactored this module to only redraw those parts of the screen that
//   have been changed.
// - Moved all code relating to pixels, pixel formats, pixel handling and
//   colours to vdu.c/vdu.h.
// - When a character cell needs to be redrawn (either by changing character
//   memory or PCG memory for any character currently on screen) signal this
//   to the VDU module that it needs to be redrawn.
//
// v5.8.0 - 13 December 2016, uBee
// - Fixed the test in crtc_configure().
//
// v5.7.0 - 26 February 2015, uBee
// - Made variable 'lpen_valid' a member of the crtc_t structure.
// - Changed function names crtc_status() to crtc_status_r() and
//   crtc_address() to crtc_address_w().
// - Fixed register 12 and 13 writes to update the display.
//
// v5.5.1 - 26 February 2015, uBee   
// - Added calls to crtc_set_redraw() and crtc_redraw() for display start
//   high and low registers 12 and 13 in crtc_data_w().
//
// v5.5.0 - 7 July 2013, uBee
// - Changes to the CGA colour intensity values (crtc.h) to match the values
//   defined in http://en.wikipedia.org/wiki/Color_Graphics_Adapter
// - Changes to confusing comments/structure for the col_table_p[]
//   definitions and the code required to access it.
// - Re-instated correct declaration for col_table_p[] by removing the
//   'const' declaration as this structure can be set with options.
// v5.0.0 - 3 August 2010, K Duckmanton
// - Refactored crtc_redraw(), crtc_draw_char(), crtc_update_cursor() and
//   crtc_setmonitor().  Foreground/background pixel values are now
//   calculated in crtc_setmonitor() (which is called when the video surface
//   is created) and the precalculated values are used in crtc_redraw() and
//   crtc_draw_char().  crtc_draw_char() now uses the 'fast' pixel drawing
//   function in video.c.
//
// v4.6.0 - 4 May 2010, uBee
// - Changes to crtc_regdump() to add binary output.
// - Renamed log_data() calls to log_data_1() and log_port() to log_port_1().
//
// v4.1.0 - 22 June 2009, uBee
// - Made improvements to the logging process.
//
// v4.0.0 - 6 June 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Masked off appended register values (upper 8 bits) from port values where
//   these were required.
// - The Vertical blanking period has been increased to 15% to produce
//   better sound with z80ex and when MWB v6.30e is used. This needs to be
//   calulated from the current CRT values rather than a constant.
//
// v3.1.0 - 6 December 2008, uBee
// - Constant used for the vertical blanking period has changed from 12/100
//   to 10/100 of the VSYNC period.
// - Added crtc_calc_vsync_freq() function to calculate the vertical sync
//   frequency from current CRTC 6545 values.
// - Changed dual intensity feature (--hint, now use --dint option) as was
//   incorrectly implemented.
// - Changed the way hardware flashing works, now also emulates early Premium
//   (main board v3) flash method.  See modelx.hwflash.
// - Added crtc_set_flash_rate() function to set the flash rate.
// - Changed the cursor blinking modes to use the correct blink rate.
// - Changed all printf() calls to use a local xprintf() function.
// - Modified crtc_configure() function to ignore SDL aspect if OpenGL mode.
// - Added OpenGL conditional code compilation using USE_OPENGL.
//
// v3.0.0 - 6 October 2008, uBee
// - Simplified the timer by introducing a time_get_ms() function.
// - Changed 'gui_' function prefix names to 'video_', added include video.h
//   and removed include gui.h,  other minor changes to function calls.
// - Random crashes (exits) on Windows when an OpenGL window was manually
//   resized was fixed by updating the display immediately in the
//   crtc_data_w() function when any of the CRTC registers 1, 6 or 9 or
//   changed, the code that had handled this in the crtc_update() function
//   has been removed. It should be remembered that this affects how big the
//   rendering buffer will be and so should be acted on immediately or make
//   sure that the values used for the screen set up are the ones used for
//   checking data boundaries.
// - Changed crtc_videochange() function to have better boundary checking.
//
// v2.8.0 - 18 August 2008, uBee
// - SDL video rendering has seen some major changes, SW and HW surfaces are
//   now supported along with 8, 16 or 32 bits per pixel modes.
// - Reversed engineered the contents of the standard IC 7 82s23 PROM
//   resulting in a new PROM table and changes to the order of values in the
//   existing standard colour tables.
// - Added a standard colour bit swap table for back ground colours to
//   convert from RGBrgb to BGRbgr order.
// - crtc.early_col_type is now crtc.std_col_type
// - Changed the default colour monitor type to RGBrgb for standard colour.
// - Added a crtc.flashbits counter that will determine when any flashing
//   video attribute bits are set, this should reduce host CPU time when
//   emulating alpha+ models.
//
// v2.7.0 - 3 July 2008, uBee
// - Premium colour table is now col_table_p.  Two colour tables have been
//   used for the early colour circuit as 2 monitor types were possible.
// - Code has been modified for the early colour model emulation.
// - Colour PROM table has been modified based on some photo results from
//   bgcol1.com and fgcol1.com Microbee colour test patterns.
// - Vertical blanking modified to return the correct duty cycle timing.
// - Added crtc_update() function, moved code over from the ubee512.c module.
// - Added structure emu_t and moved variables into it.
// - Added crtc_t structure. video member is able to be used to disable video
//   to the SDL window.
// - Added functions crtc_set_mon_table() and to set colour tables.
// - gui_videochange() is now gui_create_surface();
//
// v2.5.0 - 29 March 2008, uBee
// - Make 2MHZ model only able to access the first 2K of character ROM.
// - Added a configurable mon_table and monochrome selection to the
//   crtc_setmonitor() function.
// - Removed the const declaration from ic_82s23 array as the table values
//   are now able to be configured.
// - Removed the const declaration from col_table_2 array as the colours are
//   now able to be configured.
//
// v2.4.0 - 17 February 2008, uBee
// - Changed variables decleared in crtc_redraw() and crtc_redraw_char() to
//   static type that may result in faster execution.
//
// v2.3.0 - 8 January 2008, uBee
// - Added half intensity monochrome emulation for alpha+ models.
// - Added crtc_regdump() function to dump CRTC6545 registers.
// - Changes to crtc_setmonitor(), removed modelx.standard.
// - Added conditional test of (modelx.lpen) before the keystd_handler()
//   function is called in CRTC_DOSETADDR.
// - Added modio_t structure.
//
// v2.2.0 - 16 November 2007, uBee
// - Added original colour board emulation predating Premium and 256TC models.
// - Swapped RGB to BGR ordering in tables as this ordering was easier to work
//   out the colours for the original colour circuit.
//
// v2.1.0 - 27 October 2007, uBee
// - Reversed the full and half intensity colours in the col_table as these
//   were in the wrong order and re-adjusted the intensity levels to suit.
// - Added another intensity level to generate dark grey (was black).
// - Fiddled colour values (in crtc.h) to get better matching colours.
// - 6545 light pen keys emulation option added for 256tc model.
// - vblank status now in crtc_vblank() so that it can be accessed by the pio.c
//   module when used for port B bit 7.
// - Implement the modelx information structure.
//
// v2.0.0 - 14 October 2007, uBee
// - keyb_checkall() changed to keystd_checkall() and keyb_handler() changed
//   to keystd_handler().  These functions only called if not emulataing the
//   256TC model.
//
// v1.4.0 - 29 September 2007, uBee
// - Changes to Y scaling to achieve better aspect ratio.
// - Coding improvements to opt_monitor and opt_colour.  Some code tidy up.
// - Removed the non aplpha+ source build option.
//
// v1.0.0 - 14 August 2007, uBee
// - Added crtc_clock function to set xtal frequency and methods to be used for
//   the VBLANK status.  Now uses Z80 cycles or host timer to gererate VBLANK.
//   The host timer method is needed to keep VBLANK at 50 Hz in turbo mode.
// - Modifed the CRTC vertical blanking status method used.  The new method
//   now gives the correct delays needed for keyboard encoding and some games.
// - Implemented cursor modes (R10 bits 5,6), includes no cursor, steady cursor
//   and blinking cursor.
// - Writes to the CRTC registers (R1, R6, R9) will now cause the display to
//   be resized automatically as required.
// - Implemented the alpha+ inverse and flashing attribute bits.
// - Added standard and standard+colour model options.
// - Implemeted alpha+ (Premium) graphics in functions crtc_redraw and
//   crtc_redraw_char.
// - Added amber, green, black and white monitor options, default is colour.
//
// v0.0.0 - 21 June 2007, uBee
// - Start with "nanowasp" source distribution version 0.22. An emulator for
//   the microbee 128k. Copyright (C) 2000-2003  David G. Churchill.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <SDL.h>

#include "ubee512.h"
#include "z80api.h"
#include "macros.h"
#include "support.h"
#include "crtc.h"
#include "keystd.h"
#include "vdu.h"
#include "video.h"

//==============================================================================
// structures and variables
//==============================================================================
static void crtc_calc_vsync_freq (void);
int crtc_update_cursor (void);

crtc_t crtc =
{
 .video = 1,
 .hdisp = 80,
 .vdisp = 25,
 .scans_per_row = 11,
 .vblank_method = 0,
 .monitor = 0,
 .std_col_type = 1,
 .flashrate = 4,
};

static double vsync_freq;

static int cur_blink_rate_t1r32;
static int cur_blink_rate_t1r16;
static int cur_blink_rate_c1r32;
static int cur_blink_rate_c1r16;

static int cur_blink_last;
static int cur_blink;
static int cur_mode;
static int cur_pos;

static int flashvideo_last;

static int crtc_regs_data[32];
static int vblank_divval;
static int vblank_cmpval;

static int htot;
static int vtot;
static int vtot_adj;
static int cur_start;
static int cur_end;
static int lpen;
static int reg;

static int mem_addr;
static int redraw;

#ifdef MINGW
#else
struct timeval tod_x;
#endif

static char *crtc_regs_names[] =
{
 "Horiz Total-1",
 "Horiz Displayed",
 "Horiz Sync Position",
 "VSYSNC, HSYNC Widths",
 "Vert Total-1",
 "Vert Total Adjust",
 "Vert Displayed",
 "Vert Sync Position",
 "Mode Control",
 "Scan Lines-1",
 "Cursor Start",
 "Cursor End",
 "Display Start Addr (H)",
 "Display Start Addr (L)",
 "Cursor Position (H)",
 "Cursor Position (L)",
 "Light Pen Reg (H)",
 "Light Pen Reg (L)",
 "Update Address Reg (H)",
 "Update Address Reg (L)"
 };

extern SDL_Surface *screen;
extern emu_t emu;
extern model_t modelx;
extern modio_t modio;
extern vdu_t vdu;
extern video_t video;


//==============================================================================
// CRTC Change Video
//
//
//   pass: void
// return: int                  0 if success, -1 if error
//==============================================================================
static int crtc_videochange (void)
{
 int crt_w;
 int crt_h;

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;

 if (crt_w == 0 || crt_h == 0 || 
     crt_w > 720 || crt_h > 600)
    return -1;
 /*
  * For programs running in 40 column mode (such as Videotex),
  * the aspect ratio is forced to 1 as it looks better.
  */
 video_configure((crtc.hdisp < 50) ? 1 : video.aspect);
 vdu_configure(video.yscale);
 video_create_surface(crt_w, crt_h * video.yscale);

 crtc_set_redraw();
 crtc_redraw();
 video_render();

 crtc.resized = 0;          // clear the resized flag

 return 0;
}

//==============================================================================
// CRTC Initialise
//
//   pass: void
// return: int                  0 if sucess, -1 if error
//==============================================================================
int crtc_init (void)
{
 return 0;
}

//==============================================================================
// CRTC de-initialise
//
//   pass: void
// return: int                  0
//==============================================================================
int crtc_deinit (void)
{
 return 0;
}

//==============================================================================
// CRTC reset
//
//   pass: void
// return: int                  0
//==============================================================================
int crtc_reset (void)
{
 reg = 0;

 return 0;
}

//==============================================================================
// CRTC vblank status
//
// The Vertical blanking status is generated from the Z80 clock cycles that
// have elapsed or the host timer depending on the mode required.
//
//   pass: void
// return: int                  vblank status (in bit 7)
//==============================================================================
int crtc_vblank (void)
{
 uint64_t cycles_now;

 if (crtc.vblank_method == 0)
    {
     cycles_now = z80api_get_tstates();
     if ((cycles_now % vblank_divval) < vblank_cmpval)
        return B8(10000000);
    }
 else
    {
     if ((time_get_ms() / 10) & 1)      // div 10mS (100Hz)
        return B8(10000000);            // return true at a 50Hz rate
    }

 return 0;
}

//==============================================================================
// CRTC status
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t             status
//==============================================================================
uint16_t crtc_status_r (uint16_t port, struct z80_port_read *port_s)
{
 int status = 0;

 crtc.update_strobe = B8(10000000);

 if (modelx.lpen && ! crtc.lpen_valid)
    keystd_checkall();

 if (modelx.lpen && crtc.lpen_valid)     // NB: this is not an else because
    status |= 0x40;                     // keystd_checkall might set lpen_valid

 if (crtc_vblank())
    status |= 0x20;

 if (modio.crtc)
    log_port_1("crtc_status_r", "status", port, status);

 return crtc.update_strobe | status;
}

//==============================================================================
// CRTC Light Pen
//
// Called from keystd_handler() and keystd_checkall() in keystd.c when a key
// is detected as pressed and sets lpen valid bit.
//
// If the passed 
//
//   pass: int addr
// return: void
//==============================================================================
void crtc_lpen (int addr)
{
 if (! crtc.lpen_valid)
    {
     crtc.lpen_valid = 1;
     lpen = addr;
     if (modio.crtc)
        log_data_1("crtc_lpen", "addr", addr);
    }
}

//==============================================================================
// Set CRTC reg address - Port function
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void crtc_address_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 reg = data & 0x1F;

 if (modio.crtc)
    log_port_1("crtc_address_w", "data", port, data);
}

//==============================================================================
// Read CRTC register data - Port function
//
//   pass: uint16_t port
//         struct z80_port_read *port_s
// return: uint16_t register    register data
//==============================================================================
uint16_t crtc_data_r (uint16_t port, struct z80_port_read *port_s)
{
 uint16_t val;

 switch (reg)
    {
     case CRTC_CUR_POS_H:       // R14
        val = (cur_pos >> 8) & 0x3F;
        break;
     case CRTC_CUR_POS_L:       // R15
        val = cur_pos & 0xFF;
        break;

     case CRTC_LPEN_H:          // R16
        crtc.lpen_valid = 0;
        val = (lpen >> 8) & 0x3F;
        break;
     case CRTC_LPEN_L:          // R17
        crtc.lpen_valid = 0;
        val = lpen & 0xFF;
        break;

     case CRTC_DOSETADDR:       // R31
        crtc.update_strobe = 0;
        val = 0xFFFF;
        break;

     default:
        val = 0xFFFF;
    }

 if (modio.crtc)
    log_port_2("crtc_data_r", "reg", "val", port, reg, val);

 return val;
}

//==============================================================================
// Write CRTC register data - Port function
//
// crtc_redraw, and crtc_redraw_char functions
// -------------------------------------------
// uses: crtc.hdisp, crtc.vdisp, crtc.disp_start, crtc.scans_per_row,
//       cur_start, cur_end, cur_pos, cur_mode
//
// Setting up the display resolution
// ---------------------------------
// The CRTC_HDISP data is placed in crtc.hdisp.
// The CRTC_VDISP data is placed in crtc.vdisp.
// The CRTC_SCANLINES data is placed in crtc.scans_per_row
//
// The X resolution is determined from crtc.hdisp * 8 The Y resolution is
// determined from crtc.vdisp * crtc.scans_per_row
//
//   pass: uint16_t port
//         uint8_t data
//         struct z80_port_write *port_s
// return: void
//==============================================================================
void crtc_data_w (uint16_t port, uint8_t data, struct z80_port_write *port_s)
{
 int old_curpos;

 if (modio.crtc)
    log_port_1("crtc_data_w", "data", port, data);

 crtc_regs_data[reg] = data;

 switch (reg)
    {
     case CRTC_HTOT:            // R0
        htot = (data & 0xFF) + 1;
        crtc_calc_vsync_freq();
        break;
     case CRTC_HDISP:           // R1
        if (crtc.hdisp != (data & 0xFF)) // if the value has really changed
           {
            crtc.hdisp = data & 0xFF;
            crtc.resized = 1;
           }
        crtc_calc_vsync_freq();
        break;

     // R2 - Not implemented
     // R3 - Not implemented

     case CRTC_VTOT:            // R4
        vtot = (data & 0x7F) + 1;
        crtc_calc_vsync_freq();
        break;
     case CRTC_VTOT_ADJ:        // R5
        vtot_adj = data & 0x1F;
        crtc_calc_vsync_freq();
        break;

     case CRTC_VDISP:           // R6
        if (crtc.vdisp != (data & 0x7F)) // if the value has really changed
           {
            crtc.vdisp = data & 0x7F;
            crtc.resized = 1;
           }
        break;

     // R7 - Not implemented
     
     // R8 - Not implemented
     // Mode Control - this will normally be programmed with 01001000
     // bit 6 set=pin 34 functions as an update strobe.
     // bit 3 set=transparent memory addressing.

     case CRTC_SCANLINES:       // R9
        if (crtc.scans_per_row != ((data & 0x1F) + 1)) // if the value has
                                                       // really changed
           {
            crtc.scans_per_row = (data & 0x1F) + 1;
            crtc.resized = 1;
           }
        crtc_calc_vsync_freq();
        break;

     case CRTC_CUR_START:       // R10
        cur_start = data & 0x1F;
        cur_mode = (data >> 5) & 0x03;
        crtc_update_cursor();
        crtc_redraw_char(cur_pos, 0);
        break;
     case CRTC_CUR_END:         // R11
        cur_end = data & 0x1F;
        crtc_redraw_char(cur_pos, 0);
        break;

     case CRTC_DISP_START_H:    // R12
        crtc.disp_start &= 0xFF;
        crtc.disp_start |= (data & 0x3F) << 8;
        crtc_set_redraw();
        break;
     case CRTC_DISP_START_L:    // R13
        crtc.disp_start &= 0x3F00;
        crtc.disp_start |= data & 0xFF;
        crtc_set_redraw();
        break;

     case CRTC_CUR_POS_H:       // R14
        old_curpos = cur_pos;
        cur_pos &= 0xFF;
        cur_pos |= (data & 0x3F) << 8;
        crtc_redraw_char(old_curpos, 0);
        crtc_redraw_char(cur_pos, 0);
        break;
     case CRTC_CUR_POS_L:       // R15
        old_curpos = cur_pos;
        cur_pos &= 0x3F00;
        cur_pos |= data & 0xFF;
        crtc_redraw_char(old_curpos, 0);
        crtc_redraw_char(cur_pos, 0);
        break;

     // R16 - Is a read only register
     // R17 - Is a read only register

     case CRTC_SETADDR_H:       // R18
        mem_addr = (mem_addr & 0x00FF) | ((data & 0x3F) << 8);
        break;
     case CRTC_SETADDR_L:       // R19
        mem_addr = (mem_addr & 0x3F00) | (data & 0xFF);
        break;

     case CRTC_DOSETADDR:       // R31
        crtc.update_strobe = 0;
        if (modelx.lpen)
           keystd_handler(mem_addr);
        break;
   }
}

//==============================================================================
// redraw one screen address character position.
//
//   pass: int addr             address to be redrawn
//         int dostdio          1 if stdout should be used
// return: void
//==============================================================================
void crtc_redraw_char (int maddr, int dostdout)
{
 if ((crtc.hdisp == 0) || (! crtc.video))
    return;
 vdu_redraw_char(maddr);
}

//==============================================================================
// Set the redraw flag so that the next crtc_redraw function call is carried
// out.
//
//   pass: void
// return: void
//==============================================================================
void crtc_set_redraw (void)
{
 redraw = 1;
}

//==============================================================================
// Update the whole screen area if the global redraw flag is set, otherwise
// only those character positions that have changed.
//
//   pass: void
// return: void
//==============================================================================
void crtc_redraw (void)
{
 int i, j, x, y, l;
 int maddr;

 if (!crtc.video)
    return;                     /* redraws disabled */

 vdu_propagate_pcg_updates(crtc.disp_start, crtc.vdisp * crtc.hdisp);

 maddr = crtc.disp_start;
 l = video.yscale * crtc.scans_per_row;
 for (y = 0, i = 0; i < crtc.vdisp; i++, y += l)
    for (x = 0, j = 0; j < crtc.hdisp; j++, x += 8)
       {
        maddr &= 0x3fff;
        if (redraw || vdu_char_is_redrawn(maddr))
           {
           vdu_draw_char(screen, 
                         x, y,
                         maddr,
                         crtc.scans_per_row,
                         crtc.flashvideo,
                         (maddr == cur_pos) ? cur_blink : 0x00, cur_start, cur_end);
           vdu_char_clear_redraw(maddr);
           /* Signal to the video module that the screen needs to be redrawn */
           crtc.update = 1;
           }
        maddr++;
       }
 redraw = 0;
}

//==============================================================================
// CRTC update cursor.
//
// This function updates the state of the cursor.  It returns true if the
// cursor state has changed.
//
//   pass: void
// return: int
//==============================================================================
int crtc_update_cursor(void)
{
 // Determine the current status for the CRTC blinking cursor and refresh it
 // if this has changed.  The method used here depends on if turbo mode is
 // used. If turbo mode is used then Z80 execution speed will not be known as
 // no delays will be inserted, if not turbo then the rate must be determined
 // by the Z80 cycle count to achieve smooth results.
 switch (cur_mode)
    {
     default:
     case 0:
        cur_blink_last = cur_blink = 0xff; /* cursor always displayed */
        break;
     case 1:
        cur_blink_last = cur_blink = 0x00; /* cursor off */
        break;
     case 2:
        // blinking at 1/32 field rate
        cur_blink =
           (((emu.turbo) ?
             (time_get_ms() / cur_blink_rate_t1r32) :
             (emu.z80_cycles / cur_blink_rate_c1r32)) & 0x01) ? 0xff: 0x00;
        break;
     case 3:
        // blinking at 1/16 field rate
        cur_blink =
           (((emu.turbo) ?
             (time_get_ms() / cur_blink_rate_t1r16) :
             (emu.z80_cycles / cur_blink_rate_c1r16)) & 0x01) ? 0xff: 0x00;
        break;
    }
 if (cur_blink != cur_blink_last)
    {
     cur_blink_last = cur_blink;
     return 1;                  // changed
    }
 else
    return 0;
}


//==============================================================================
// CRTC update.
//
// Updates the CRTC periodically.
//
//   pass: void
// return: void
//==============================================================================
void crtc_update (void)
{
  if (crtc.resized)
     crtc_videochange();        // resets crtc.resized value

 if (crtc_update_cursor())
    crtc_redraw_char(cur_pos, 0);
 // Determine the current state of the alpha+ flashing video and refresh it
 // if this has changed.
 if (vdu.extendram)                    // only if extended RAM selected
    {
     if (emu.turbo)
        {
         if ((time_get_ms() / crtc.flashvalue_t) & 0x01)
            crtc.flashvideo = modelx.hwflash;
         else
            crtc.flashvideo = 0;
        }
     else
        {
         if ((emu.z80_cycles / crtc.flashvalue_c) & 0x01)
            crtc.flashvideo = modelx.hwflash;
         else
            crtc.flashvideo = 0;
        }
     if (crtc.flashvideo != flashvideo_last)
        {
         flashvideo_last = crtc.flashvideo;
         vdu_propagate_flashing_attr(crtc.disp_start, crtc.vdisp * crtc.hdisp);
        }
    }
 crtc_redraw();
}

//==============================================================================
// CRTC register dump
//
// Dump the contents of the crtc registers.
//
//   pass: void
// return: void
//==============================================================================
void crtc_regdump (void)
{
 int i;
 char s[17];

 // CRTC_CUR_POS_H
 crtc_regs_data[14] = (cur_pos >> 8) & 0x3F;

 // CRTC_CUR_POS_L
 crtc_regs_data[15] = cur_pos & 0xFF;

 // CRTC_LPEN_H
 crtc_regs_data[16] = (lpen >> 8) & 0x3F;

 // CRTC_LPEN_L
 crtc_regs_data[17] = lpen & 0xFF;

 xprintf("\n");
 xprintf("6545 CRTC Registers                Hex  Dec    Binary\n");
 xprintf("------------------------------------------------------\n");

 for (i = 0; i < 20; i++)
    xprintf("0x%02x (%02dd) %-22s  %02x %5d %10s\n", i, i, crtc_regs_names[i],
    crtc_regs_data[i], crtc_regs_data[i], i2b(crtc_regs_data[i],s));
}

//==============================================================================
// CRTC set hardware flash rate.
//
// Sets the flash rate for the alpha+ flashing attribute bit.  The flash
// rate is determined by IC60 a dual 4-bit binary counter, 4 link settings
// (W6x) and the VSYNC signal (typ 50Hz).  The settings for a V4 main board
// are as follows:
//
// Number   74LS393   Link      Rate (milliseconds)
// 0        1QA                 20
// 1        1QB                 40
// 2        1QC                 80
// 3/8      1QD       W61 A-B   160
// 4/9      2QA       W62 A-B   320
// 5/10     2QB       W63 A-B   640
// 6/11     2QC       W64 A-B   1280
// 7        2QD                 2560
//
// Four link settings (W61-W64) are provided on the main board, other values
// are possible by connecting to other pins.
//
// NOTE: Version 3 boards have 1280mS for W63 and 640mS for W64.
//
// This function should be called when the flash rate option is used and
// after the CPU clock speed is set.
//
//   pass: int n                flash rate number 0-11
// return: int                  0 if sucess, -1 if error
//==============================================================================
int crtc_set_flash_rate (int n)
{
 double t;

 if ((n < 0) || (n > 11))
    return -1;

 if (n < 8)
    crtc.flashrate = n;
 else
    crtc.flashrate = (n - 8) + 3;

 t = (1.0 / vsync_freq) * (1 << n);

 crtc.flashvalue_c = (int)(emu.cpuclock * t);
 crtc.flashvalue_t = (int)(t * 1000.0);

 return 0;
}

//==============================================================================
// CRTC clock calculations.
//
// VERTICAL BLANKING
// -----------------
// The Vertical blanking status is generated from the Z80 clock cycles that
// have elapsed or the host timer depending on the mode required.
//
// The Vertical blanking period is emulated to produce about a 15% (was 10%)
// on duty cycle. The emulated blanking frequency is ~50 frames per second
// for normal usage or can be a proportional value calculated from the CPU
// clock frequency.
//
// The vertical blanking period can not be derived from the host timer in
// the manner expected because the timer is continuous and the Z80 CPU
// emulation is achieved in frames, so a basic 50% duty is returned for this
// mode.  This is mainly intended for when running high speed emulation to
// keep key repeat speed usable.
//
// The vertical blanking is commonly used for keyboard encoding delays and for
// delays used in some games.
//
// CURSOR BLINKING
// ---------------
// blinking time (1/16 field rate) = 16 / vsync_freq
// blinking time (1/32 field rate) = 32 / vsync_freq
//
//   pass: int cpuclock         CPU clock frequency in Hz.
// return: void
//==============================================================================
void crtc_clock (int cpuclock)
{
 vblank_divval = (int)(cpuclock / vsync_freq);  // 67500 if 50Hz
 vblank_cmpval = (int)(vblank_divval * (15.0/100.0));

 // blinking at 1/32 field rate
 cur_blink_rate_t1r32 = (int)((32.0 / vsync_freq) * 1000);
 cur_blink_rate_c1r32 = (int)(cpuclock * (32.0 / vsync_freq));

 // blinking at 1/16 field rate
 cur_blink_rate_t1r16 = (int)((16.0 / vsync_freq) * 1000);
 cur_blink_rate_c1r16 = (int)(cpuclock * (16.0 / vsync_freq));

 crtc_set_flash_rate(crtc.flashrate);
}

//==============================================================================
// CRTC VSYNC calculations.
//
// Calculate the vertical sync frequency.  2MHz models use 12MHz and all
// others use a 13.5MHz crystal.
//
//   pass: void
// return: void
//==============================================================================
static void crtc_calc_vsync_freq (void)
{
 double vdu_xtal;

 if (emu.model == MOD_2MHZ)
    vdu_xtal = 12.0E+6;
 else
    vdu_xtal = 13.5E+6;

 if (htot && vtot && crtc.scans_per_row)
    {
     vsync_freq = (vdu_xtal / (htot * 8)) /
     (vtot * crtc.scans_per_row + vtot_adj);
     // adjust everything that relies on the VSYNC frequency
     crtc_clock(emu.cpuclock);
    }

 // avoid divide by 0 errors
 if (vsync_freq < 5.0)
    vsync_freq = 1.0;

#if 0
 xprintf("crtc_sync: vsync_freq=%f\n", vsync_freq);
#endif
}
