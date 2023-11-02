//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                               video module                                 *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Provides SDL and OpenGL video rendering.
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
// - Add additional logic to debug OpenGL calls.
// - OpenGL rendering requires the creation of a sufficiently large texture;
//   if the OpenGL driver won't allow a suitably sized texture to be
//   created, fall back to software rendering.
//
// v5.0.0 - 3 August 2010, K Duckmanton
// - Optimised video_putpixel().  Pixel drawing is now done by a function
//   optimised for the depth of the video surface, resulting in a 2x speed
//   increase.
//
// v4.7.0 - 4 June 2010, uBee
// - Conditional compilation for MINGW and DARWIN has been removed from the
//   video sections. Testing and branching is now based on an emu.system and
//   emu.sdl_version variable as SDL-1.2.14 fixes some issues in Windows.
//
// v4.5.0 - 1 April 2010, uBee
// - Changed all occurences of video_t members gl_screen_w and gl_screen_h
//   to gl_imagedim_w and gl_imagedim_h as these were the same values.  The
//   gl_screen_w and gl_screen_h members have been removed from the
//   structure.
//
// v4.2.0 - 13 July 2009, uBee
// - Prevent mouse cursor enabling if emulating the Microbee mouse.
//
// v4.0.0 - 7 June 2009 uBee
// - Mac OSX also destroys OpenGL texture when the window is resized.  Make
//   conditionals compile in the same methods used for Windows.
// - Fixed mouse cursor positioning when switching to and from full screen
//   and window displays in video_toggledisplay() function.
//
// v3.1.0 - 22 April 2009, uBee
// - Removed all occurrences of console_output() function calls.
// - Removed all occurrences of SDL_ShowCursor(SDL_DISABLE), as auto cursor
//   hiding for full screen mode is implemented in gui.c module.
// - Changed video_putpixel() function to ignore negative X and Y values
//   and changed ordering of case statements.
// - Added OSD related code to overlay CRTC and fullscreen cursor changes.
// - Moved commands over from function.c to new video_command() function.
// - Changed all printf() calls to use a local xprintf() function.
// - Added video_gl_filter_update() to allows options to update OpenGL
//   filters immidiately.
// - Added OpenGL conditional code compilation using USE_OPENGL.
//
// v3.0.1 - 13 October 2008, uBee
// - Toggling full screen under Unices and using SDL video rendering fails
//   due to another variable now used to flag the current full screen state.
//   The video_toggledisplay() function now sets the video.flags.
//
// v3.0.0 - 20 Sepetember 2008, uBee
// - Created a new file using the video related code from the gui.c module
//   and implement OpenGL texture rendering mode.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <SDL.h>

#ifdef USE_OPENGL
#include <gl.h>
#endif

#ifdef MINGW
#ifdef USE_OPENGL
#include <glext.h>
#endif
#include <windows.h>
#else
#endif

#include "ubee512.h"
#include "gui.h"
#include "video.h"
#include "crtc.h"
#include "vdu.h"
#include "mouse.h"
#include "osd.h"

//==============================================================================
// #defined constants
//==============================================================================

// Set to 1 to include code that checks the OpenGL error number after each
// OpenGL function is called.
#define DEBUG_OPENGL 0

//==============================================================================
// macros
//==============================================================================
#ifdef USE_OPENGL
// report OpenGL error codes from the previous openGL operation
#if DEBUG_OPENGL
#define glERR(x, l)                                                     \
   do {                                                                 \
    GLenum glerr;                                                       \
    while ((glerr = glGetError()) != GL_NO_ERROR) {                     \
     if (modio.video)                                                   \
        xprintf("Line %d %s() returned 0x%04x\n", l, #x, glerr);        \
     assert(0);                                                         \
    }                                                                   \
   } while(0)
#define __glCLEARERROR(l)                                               \
   do {                                                                 \
    GLenum glerr;                                                       \
    int i = 100;                                                        \
    while (i > 0 && (glerr = glGetError()) != GL_NO_ERROR) {            \
    --i;                                                                \
    switch (i) {                                                        \
    case 0:                                                             \
        if (modio.video)                                                \
           xprintf("line %d: too many openGL errors (0x%04x)\n", l, glerr); \
        assert(0);                                                      \
        break;                                                          \
    case 25:                                                            \
    case 50:                                                            \
    case 75:                                                            \
        if (modio.video)                                                \
           xprintf("line %d: waiting for the openGL errors to clear (0x%04x)\n", l, glerr); \
        break;                                                          \
     default:                                                           \
        break;                                                          \
    }                                                                   \
    }                                                                   \
   } while(0)
#else
#define glERR(x, l)                                                     \
    do {                                                                \
        GLenum glerr;                                                   \
        while ((glerr = glGetError()) != GL_NO_ERROR) {                 \
        }                                                               \
    } while(0)
#define __glCLEARERROR(l) \
   glGetError()
#endif  /* DEBUG_OPENGL */

// call an OpenGL function, printing an error code if any afterwards
#define glCALL(__FUNC, __ARGS) glCALL2(__LINE__, __FUNC, __ARGS)
#define glCALL2(__LINE, __FUNC, __ARGS)                         \
   do {                                                         \
    __FUNC __ARGS;                                              \
    glERR(__FUNC, __LINE);                                      \
   } while(0)
#define glCLEARERROR() __glCLEARERROR(__LINE__)

#endif

#define NUMENTRIES(x) (sizeof(x)/sizeof(x[0]))

#define MASK(WIDTH, SHIFT)                      \
 (~((~0) << (WIDTH)) << (SHIFT))

//==============================================================================
// structures and variables
//==============================================================================
video_t video =
{
 .depth = VIDEO_16,             // default depth used by SDL only
 .type = VIDEO_SDLHW,           // default video rendering mode
#ifdef USE_OPENGL
 .filter_fs = VIDEO_SHARP,
 .filter_win = VIDEO_SOFT,
 .filter_max = VIDEO_SHARP,
 .aspect_bee = VIDEO_ASPECT_BEE,
 .vsync = VIDEO_VSYNC_ON,
 .initial_x_percent = 50,
#endif
 .yscale = 1,                   // default scaling ratio
 .aspect = 2,                   // default window aspect ratio
};

#ifdef USE_OPENGL
typedef int RGB_Size[4];

static video_gl_t video_gl;
static SDL_Surface *gl_screen;
static RGB_Size rgb_size;
static int ignore_one_resize_event;

static RGB_Size rgb_sizes[3] = {
 { 3, 3, 2, 0 },                /* 8bpp */
 { 5, 6, 5, 0 },                /* 15/16 bpp */
 { 8, 8, 8, 0 },                /* 24 bpp */
};

#endif

SDL_Surface *screen;
static SDL_VideoInfo video_info;
static SDL_Color colors[256];
video_putpixel_fast_fn video_putpixel_fast_p;

extern emu_t emu;
extern crtc_t crtc;
extern gui_t gui;
extern gui_status_t gui_status;
extern modio_t modio;
extern mouse_t mouse;


struct _dynamic_array {
   void *p;                     /* pointer to the array of things */
   int entrysize;               /* size of each entry */
   int numentries;              /* number of active entries in the
                                 * array  */
   int size;                    /* current size of the array */
};

struct video_update_regions {
   union {
      struct _dynamic_array a;
      SDL_Rect *p;
   } rects;
   union {
      struct _dynamic_array a;
      SDL_Rect **p;
   } free_rects;
};

struct video_update_regions *video_update_regions;


#ifdef USE_OPENGL
static void video_gl_initialise_context (void);
static void video_gl_clear_display (void);
static void video_gl_clear_display_borders (void);
static void video_gl_update_transformation_matrix (void);

static int video_gl_values (int crt_w, int crt_h, int win_w, int win_h);
static void video_gl_probe_preferred_texture_format (void);
#endif

static void video_update_sdl_video_flags();

void video_putpixel_fast_8bpp(int x, int y, int val);
void video_putpixel_fast_16bpp(int x, int y, int val);
void video_putpixel_fast_32bpp(int x, int y, int val);

void video_free_update_regions();
void video_init_update_regions();



//==============================================================================
// Video initialise.
//
// Get video information before any SDL_SetVideoMode() is made to save the
// desktop width and height values.
//
// Create an initial window no larger than the maximum size of the root window
//
//   pass: void
// return: int                  0 if no error, -1 if error
//==============================================================================
int video_init (void)
{
 int win_w;
 int win_h;
 int crt_w;
 int crt_h;
 int i;

 video_info = *SDL_GetVideoInfo();
 video.desktop_w = video_info.current_w;
 video.desktop_h = video_info.current_h;

 /*
  * Compute the initial size of the window
  *
  * Default display dimensions and default initial GL window size
  */
 video.last_win_w = 
    win_w = 
    crt_w = 
    crtc.hdisp * 8;           /* Default display dimensions */
 video.last_win_h =
    win_h = 
    crt_h = 
    crtc.vdisp * crtc.scans_per_row;
 video.yscale = video.aspect;
#ifdef USE_OPENGL
 if (video.type == VIDEO_GL)
    {
     video.yscale = video.aspect = 1;

     if (video.max)
        {
         video.maximised = 1;
         win_w = video.desktop_w;
         win_h = video.desktop_h;
        }
     else if (video.initial_x_percent)
        {
         win_w = video.desktop_w * video.initial_x_percent / 100;
         win_h = 0;             /* the height will be a function of
                                 * the display aspect ratio and the
                                 * width */
        }
     else if (video.initial_x_pixels)
        {
         win_w = video.initial_x_pixels;
         win_h = 0;             /* as above */
        }

     video.bpp = (video_info.vfmt->BitsPerPixel < 8) 
        ? 8
        : video_info.vfmt->BitsPerPixel;
     switch (video.bpp)
        {
         case  8:
            i = 0;
            break;
         case 15:
         case 16:
            i = 1;
            break;
         default:
            i = 2;
         break;
        }
     memcpy(rgb_size, &rgb_sizes[i], sizeof(rgb_size));
    }
#endif

 if (
#ifdef USE_OPENGL
    video_gl_values(crt_w, crt_h * video.yscale, win_w, win_h) == -1 ||
#endif
    win_w > video.desktop_w
    )
    {
     xprintf("video_init: %d pixel wide window will not fit on screen\n", win_w);
     return -1;
    }
 video_update_sdl_video_flags();
 if (video_create_window(win_w, win_h) == -1)
    {
     xprintf("video_init: Unable to create the application window\n");
     return -1;
    }
 video_init_update_regions();
 if (video_create_surface(crt_w, crt_h * video.yscale) == -1)
    {
     xprintf("video_init: Unable to create the display surface\n");
     return -1;
    }

 return 0;
}

//==============================================================================
// Video de-initialise.
//
//   pass: void
// return: int                  0
//==============================================================================
int video_deinit (void)
{
 video_free_update_regions();
 return 0;
}

//==============================================================================
// Video reset.
//
//   pass: void
// return: int                  0
//==============================================================================
int video_reset (void)
{
 return 0;
}

//==============================================================================
// Convert mouse X, Y values to CRTC scaled values.
//
// When using OpenGL mode the mouse X, Y values returned are true screen
// co-ordinates and not the resized values.  This requires the values to be
// converted back to CRTC like values before they can be used.
//
//   pass: int mouse_x                  mouse X position
//         int mouse_y                  mouse Y position
//         int *x                       scaled to CRTC X
//         int *y                       scaled to CRTC Y
// return: void
//==============================================================================
void video_convert_mouse_to_crtc_xy (int mouse_x, int mouse_y, int *x, int *y)
{
#ifdef USE_OPENGL
 if (video.type == VIDEO_GL)
    {
     mouse_x -= video_gl.texture_region.x;
     mouse_y -= video_gl.texture_region.y;
     if (mouse_x < 0)
        mouse_x = 0;
     if (mouse_y < 0)
        mouse_y = 0;
     if (mouse_x > video_gl.texture_region.w)
        mouse_x = video_gl.texture_region.w;
     if (mouse_y > video_gl.texture_region.h)
        mouse_y = video_gl.texture_region.h;
     *x = crtc.hdisp *                  8 * mouse_x / video_gl.texture_region.w;
     *y = crtc.vdisp * crtc.scans_per_row * mouse_y / video_gl.texture_region.h;
    }
 else
#endif
    {
     *x = mouse_x;
     *y = mouse_y / video.yscale;
    }
}

//==============================================================================
// Convert CRTC X, Y values to mouse values.
//
//   pass: int crtc_x                   CRTC X position
//         int crtc_y                   CRTC Y position
//         int *x                       scaled to mouse X
//         int *y                       scaled to mouse Y
// return: void
//==============================================================================
void video_convert_crtc_to_mouse_xy (int crtc_x, int crtc_y, int *x, int *y)
{
#ifdef USE_OPENGL
 if (video.type == VIDEO_GL)
    {
     *x = crtc_x * video_gl.texture_region.w / (crtc.hdisp *                  8);
     *y = crtc_y * video_gl.texture_region.h / (crtc.vdisp * crtc.scans_per_row);
     *x += video_gl.texture_region.x;
     *y += video_gl.texture_region.y;
    }
 else
#endif
    {
     *x = crtc_x;
     *y = crtc_y * video.yscale;
    }
}

//==============================================================================
// Report video information.
//
//   pass: void
// return: void
//==============================================================================
static void video_report_information (void)
{
 static int reported = 0;
 int driver_initialised = 0;
 char driver_name[80];

 if ((! modio.video) || (reported++))
    return;

driver_initialised =
   (SDL_VideoDriverName(driver_name, sizeof(driver_name) - 1) != NULL);

 xprintf("\n");
 xprintf("SDL GENERAL INFORMATION\n");
 xprintf("-----------------------\n");
 xprintf("Video driver   : %s\n", driver_initialised ? driver_name : "[not initialised]");
 xprintf("Desktop width  : %d\n", video.desktop_w);
 xprintf("Desktop height : %d\n", video.desktop_h);
 xprintf("Initial BPP    : %d\n", video_info.vfmt->BitsPerPixel);

#ifdef USE_OPENGL
 if (video.type >= VIDEO_GL)
    {
     int value;

     xprintf("\n");
     xprintf("OPENGL INFORMATION\n");
     xprintf("------------------\n");
     xprintf("Vendor              : %s\n", glGetString(GL_VENDOR));
     xprintf("Renderer            : %s\n", glGetString(GL_RENDERER));
     xprintf("Version             : %s\n", glGetString(GL_VERSION));
#if 0
     xprintf("Extensions          : %s\n", glGetString(GL_EXTENSIONS));
#endif
     xprintf("\n");
     SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &value);
     xprintf("SDL_GL_RED_SIZE     : requested %d, got %d\n", rgb_size[0], value);
     SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &value );
     xprintf("SDL_GL_GREEN_SIZE   : requested %d, got %d\n", rgb_size[1], value);
     SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &value );
     xprintf("SDL_GL_BLUE_SIZE    : requested %d, got %d\n", rgb_size[2], value);
     SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE, &value );
     xprintf("SDL_GL_ALPHA_SIZE   : requested %d, got %d\n", rgb_size[3], value);
     SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &value );
     xprintf("SDL_GL_DEPTH_SIZE   : requested %d, got %d\n", video.bpp, value);
     SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &value );
     xprintf("SDL_GL_DOUBLEBUFFER : requested 1, got %d\n", value);
    }
#endif
 xprintf("\n");
}


//==============================================================================
// Create the top level window
//
// If not using OpenGL, or if the renderer selected is not an OpenGL renderer,
// this function does nothing as the top level window will be implicitly
// created by video_create_surface() when SDL_SetVideoMode() is called.
//
// If OpenGL is enabled, this function creates the application window.  This window
// is only destroyed when the application exits.
// 
//==============================================================================
int video_create_window(int width, int height)
{
#ifndef USE_OPENGL
 return 0;                      /* success */
#else
 if (video.type != VIDEO_GL)
    return 0;                   /* OpenGL mode not in use, return success */

 SDL_GL_SetAttribute(SDL_GL_RED_SIZE, rgb_size[0]);
 SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, rgb_size[1]);
 SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, rgb_size[2]);
 SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, rgb_size[3]);
 SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
 SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

 // Vsync: swap buffers every n'th retrace (0 to disable, 1 is the default)
 SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, video.vsync);

 // don't free this surface if resetting!
 // gl_window_w and gl_window_h have already been set before calling this function
 if ((gl_screen = SDL_SetVideoMode(video.gl_window_w, video.gl_window_h, 0, video.flags)) == NULL)
    {
     xprintf("video_create_surface: SDL_SetVideoMode failed - %s\n", SDL_GetError());
     return -1;
    }

 SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &video_gl.bpp );

 video_gl_initialise_context();
 video_gl_update_transformation_matrix();

 video_gl_probe_preferred_texture_format();

 if (0
     && video_gl.pixel_format == 0
     && video_gl.pixel_type == 0
     && video_gl.internal_format == 0)
    {
     xprintf("video_create_surface: could not create the largest texture required,\n"
             "                      falling back to software rendering\n");
     video.type = VIDEO_SDLSW;
     video.aspect = 2;
     video.yscale = 2;   // closest to the intended aspect ratio
     SDL_FreeSurface(gl_screen);
     gl_screen = NULL;
     return 0;
    }

 video_gl_clear_display();

 glFlush();

 return 0;
#endif
}


#ifdef USE_OPENGL
//==============================================================================
// Initialise the OpenGL context
//==============================================================================
static void video_gl_initialise_context (void)
{
#if DEBUG_OPENGL
 /* debugging colour! */
 glClearColor(0.5, 0.5, 1.0, 1.0);
#else
 glClearColor(0.0, 0.0, 0.0, 1.0);
#endif
 glCALL(glShadeModel,(GL_FLAT));
 glCALL(glDisable,(GL_CULL_FACE));
 glCALL(glDisable,(GL_DEPTH_TEST));
 glCALL(glDisable,(GL_ALPHA_TEST));
 glCALL(glDisable,(GL_LIGHTING));
 glCALL(glHint,(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST));
}

//==============================================================================
// Clear the OpenGL display
//==============================================================================
static void video_gl_clear_display (void)
{
#if DEBUG_OPENGL
 SDL_FillRect(gl_screen, NULL, SDL_MapRGB(gl_screen->format, 0xff, 0x80, 0xff));
#else
 SDL_FillRect(gl_screen, NULL, SDL_MapRGB(gl_screen->format, 0, 0, 0));
#endif
 glCALL(glClear,(GL_COLOR_BUFFER_BIT));
 SDL_GL_SwapBuffers();
 glCALL(glClear,(GL_COLOR_BUFFER_BIT));
}

//==============================================================================
// Clear the OpenGL display's borders
//==============================================================================
static void video_gl_clear_display_borders (void)
{
 SDL_Rect r;
 Uint32 colour = 
    SDL_MapRGB(gl_screen->format, 
#if DEBUG_OPENGL
               0xff, 0x80, 0xff
#else
               0, 0, 0
#endif
       );
 if (video_gl.texture_region.x == 0 && video_gl.texture_region.y != 0)
    {
     // clearing the top and bottom of the screen
     // top first
     r.x = 0;
     r.y = 0;
     r.w = gl_screen->w;
     r.h = video_gl.texture_region.y;
     SDL_FillRect(gl_screen, &r, colour);
     // then bottom.  Width & height stay the same, only the y position changes
     r.y = gl_screen->h - video_gl.texture_region.y;
     SDL_FillRect(gl_screen, &r, colour);
    }
 else if (video_gl.texture_region.y == 0 && video_gl.texture_region.x != 0)
    {
     // clearing the left and right sides of the screen
     // left first
     r.x = 0;
     r.y = 0;
     r.h = gl_screen->h;
     r.w = video_gl.texture_region.x;
     SDL_FillRect(gl_screen, &r, colour);
     // then right.  Width & height stay the same, only the x position changes
     r.x = gl_screen->w - video_gl.texture_region.x;
     SDL_FillRect(gl_screen, &r, colour);
    }
 glCALL(glClear,(GL_COLOR_BUFFER_BIT));
 SDL_GL_SwapBuffers();
 glCALL(glClear,(GL_COLOR_BUFFER_BIT));
}




//==============================================================================
// update the transformation matrix
//==============================================================================
static void video_gl_update_transformation_matrix (void)
{
 glCALL(glViewport,(0, 0, video.gl_window_w, video.gl_window_h));
 glCALL(glMatrixMode,(GL_PROJECTION));
 glCALL(glLoadIdentity,());

 // make the OpenGL display top left 0, 0
 glCALL(glOrtho,(0.0, (double)video.gl_window_w,
                 (double)video.gl_window_h, 0.0,
                 0.0, 1.0));

 glCALL(glMatrixMode,(GL_MODELVIEW));
 glCALL(glLoadIdentity,());

}

//==============================================================================
// Calculate values for OpenGL texture rendering method.
//
// This is called from the video_create_surface() function.
//
//   pass: int crt_w            CRT display width
//         int crt_h            CRT display height
//         int win_w            wanted window width
//         int win_h            wanted window height
// return: int                  0 if values are ok, else -1
//==============================================================================
static void video_gl_set_texture_region(float aspect)
{
 float corrected = video.aspect_bee;
 float window_w = (float)video.gl_window_w, window_h = (float)video.gl_window_h;
 if (video.aspect_mon > 0.1)
    corrected *= aspect / video.aspect_mon;

 if (window_h * corrected > window_w)
    {
     // window taller than it is wide
     video_gl.texture_region.w = video.gl_window_w;
     video_gl.texture_region.h = (int)(window_w / corrected + 0.5);
     // offset the origin
     video_gl.texture_region.x = 0;
     video_gl.texture_region.y = (video.gl_window_h - video_gl.texture_region.h) / 2;
    }
 else
    {
     // window wider than it is tall
     video_gl.texture_region.h = video.gl_window_h;
     video_gl.texture_region.w = (int)(window_h * corrected);
     // offset the origin
     video_gl.texture_region.x = (video.gl_window_w - video_gl.texture_region.w) / 2;
     video_gl.texture_region.y = 0;
    }
}

//
// Calculate the smallest power of two that is still greater than or equal to n
//
static int npot(int n)
{
 int pot = 1;
 n--;
 while (n > 0)
    {
     n >>= 1;
     pot <<= 1;
    }
 return pot;
}

static int video_gl_values (int crt_w, int crt_h, int win_w, int win_h)
{
 // check the window width wanted
 if (win_w > video.desktop_w)
    return -1;

 if (!video_gl.pot)
    {
     video_gl.texture_w = crt_w;
     video_gl.texture_h = crt_h;
     // fraction of the texture actually used, in texture co-ordinates
     video_gl.texture_region_used_w = 1.0f;
     video_gl.texture_region_used_h = 1.0f;
    }
 else
    {
     video_gl.texture_w = npot(crt_w);
     video_gl.texture_h = npot(crt_h);
     // fraction of the texture actually used, in texture co-ordinates
     video_gl.texture_region_used_w = (float)crt_w / (float)video_gl.texture_w;
     video_gl.texture_region_used_h = (float)crt_h / (float)video_gl.texture_h;
    }

 // Correct for monitors that are not being driven at their native resolutions
 // e.g. a 640x480 video mode (4:3) being displayed on a 1280x1024 (5:4) screen
 // If not specified, assume that the desktop size is the monitor's native size
 // and compute the monitor's aspect ratio based on that.

 // FIXME: aspect ratios should be specified as integer RATIOs, not floats

 if (video.fullscreen)
    {
     // fairly self-evident
     video.gl_window_w = video.desktop_w;
     video.gl_window_h = video.desktop_h;

     // set the texture display region, with corrections, based on the
     // desktop size aka monitor size
     video_gl_set_texture_region((float)video.desktop_w / (float)video.desktop_h);
    }
 else if (video.maximised)
    {
     video.last_win_w = win_w;
     video.last_win_h = win_h;

     // If the window is maximised it cannot be resized
     video.gl_window_w = win_w;
     video.gl_window_h = win_h;

     // set the texture display region, with corrections, based on the
     // window size
     video_gl_set_texture_region((float)video.gl_window_w / (float)video.gl_window_h);
    }
 else
    {
     float corrected = video.aspect_bee;
     if (video.aspect_mon > 0.1)
        corrected /= video.aspect_mon;

     video.last_win_w = win_w;
     video.last_win_h = win_h;

     // calculate a new window size maintaining the corrected aspect ratio.
     video.gl_window_w = win_w;
     video.gl_window_h = (int)((float)win_w / corrected + 0.5);

     // set the texture display region, with corrections, based on the
     // window size (yes, this is basically repeating the calculations above
     // to get the same answer, or at least as same as floating point ever
     // gets)
     video_gl_set_texture_region((float)video.gl_window_w / (float)video.gl_window_h);
    }

 // current screen size as a percentage of the desktop width
 video.percent_size = (int)((float)video.gl_window_w / (float)video.desktop_w * 100.0);

 return 0;
}

//==============================================================================
// Get one of 3 possible filters depending on the current display type/size
// and return the OpenGL value.
//
//   pass: void
// return: int                  OpenGL filter value
//==============================================================================
static int video_gl_selected_filter (void)
{
 int filter;

 if (video.fullscreen)
    filter = video.filter_fs;
 else if (video.maximised)
    filter = video.filter_max;
 else
    filter = video.filter_win;

 switch (filter)
    {
     case 0 :
        return GL_LINEAR;
     case 1 :
        return GL_NEAREST;
    }
 return 0;
}

//==============================================================================
// Set OpenGL filter value into the texture.
//
//   pass: int filter           OpenGL filter value
// return: void
//==============================================================================
static void video_gl_filter_set (int filter)
{
 video_gl.filter = filter;
 glBindTexture(GL_TEXTURE_2D, video_gl.texture);
 // texture parameter calls are allowed to fail
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, video_gl.filter);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, video_gl.filter);
 glCLEARERROR();
}


//==============================================================================
// Determine the most appropriate texture format to use
//
//   pass: void
// return: void
//==============================================================================
static void video_gl_probe_preferred_texture_format (void)
{
 static struct {
    int screen_bpp;             /* bit depth of the screen */
    int minver;                 /* minimum OpenGL version */
    GLint i;
    GLenum pt;                  /* pixel type */
    int bpp;                    /* SDL surface's bit depth */
    GLenum pf;                  /* pixel format */
    Uint32 Rmask;               /* red mask */
    Uint32 Gmask;               /* green mask */
    Uint32 Bmask;               /* blue mask */
    Uint32 Amask;               /* alpha mask */
 } *format, formats[] = {
  // openGL 1.1 - again, may not work
  {   8, 101, 1, GL_BYTE,
      8, GL_RGB, MASK(3, 5), MASK(3, 2), MASK(2, 0), MASK(0, 0), },
  // openGL 1.2
  {   8, 102, GL_R3_G3_B2, GL_UNSIGNED_BYTE,
      8, GL_RGB, MASK(3, 5), MASK(3, 2), MASK(2, 0), MASK(0, 0), },

  // openGL 1.1 - this may not work!
  {  16, 101, 2, GL_BYTE,
     32, GL_RGB, MASK(8,24), MASK(8,16), MASK(8, 8), MASK(0, 0), },
  // openGL 1.2
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  {  16, 102, GL_BGR, GL_UNSIGNED_SHORT_5_6_5,
     16, GL_BGR, MASK(5,11), MASK(6, 5), MASK(5, 0), MASK(0, 0), },
#else
  {  16, 102, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
     16, GL_RGB, MASK(5,11), MASK(6, 5), MASK(5, 0), MASK(0, 0), },
#endif

  // openGL 1.1
  {  32, 101, 4, GL_BYTE,
     32, GL_RGB, MASK(8,24), MASK(8,16), MASK(8, 8), MASK(0, 0), },
  // openGL 1.2
  {  32, 102, 4, GL_UNSIGNED_INT_8_8_8_8,
     32, GL_RGBA, MASK(8,24), MASK(8,16), MASK(8, 8), MASK(0, 0), },
  {  32, 102, 4, GL_UNSIGNED_INT_8_8_8_8,
     32, GL_BGRA, MASK(8, 8), MASK(8,16), MASK(8,24), MASK(0, 0), },

 };

 int i;
 GLuint texture;
 GLenum glerror = 0;
 int glMajor, glMinor, glVer;
 GLint width;
 int texture_error = 0;
 int texture_pot = 0;

 sscanf((char *)glGetString(GL_VERSION), "%d.%d", &glMajor, &glMinor);
 glVer = glMajor * 100 + glMinor;

 // prefer formats that match the video bit depth?
 i = NUMENTRIES(formats);
 format = &formats[i];

 while (i-- > 0)
    {
     format--;
     if (format->screen_bpp > video_gl.bpp)
        continue;
     if (format->minver > glVer)
        continue;

     /*
      * Test each candidate pixel format by creating a 1x1 proxy
      * texture.  If the proxy texture cannot be created, OpenGL will
      * reset the texture's width to zero
      *
      * Reference:
      *   https://www.opengl.org/archives/resources/faq/technical/texture.htm
      */
     texture_error = 0;
     glCALL(glGenTextures, (1, &texture));
     glCALL(glBindTexture,(GL_TEXTURE_2D, texture));
     glTexImage2D(GL_PROXY_TEXTURE_2D, 0,
                  format->i,
                  1, 1, 0,
                  format->pf,
                  format->pt,
                  NULL);          /* don't allocate pixel storage just yet */
     glerror = glGetError();
     if (glerror != GL_NO_ERROR)
        {
         texture_error = !0;
        }
     else
        {
         glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width); 
         texture_error = (width == 0);
        }
     glCALL(glDeleteTextures, (1, &texture));
     if (texture_error)
        {
         if (modio.video)
            {
             xprintf("video_gl_probe_preferred_texture_format:\n"
                     "\tFormat %d failed, trying the next one\n", i);
            }
         continue;
        }

     /*
      * determine whether a maximally sized, non-power-of-two texture
      * can be created
      *
      * For this to work, the OpenGL driver must support OpenGL 2.0 or
      * OpenGL 1.4 with the GL_ARB_texture_non_power_of_two extension
      *
      * References:
      *  https://www.khronos.org/opengl/wiki/NPOT_Texture
      *  https://www.opengl.org/registry/specs/ARB/texture_non_power_of_two.txt
      *
      * (The better way of doing this might be to test for the
      * presence of the required extension)
      */
     texture_error = 0;
     texture_pot = 0;
     glCALL(glGenTextures, (1, &texture));
     glCALL(glBindTexture,(GL_TEXTURE_2D, texture));
     glTexImage2D(GL_PROXY_TEXTURE_2D, 0,
                  format->i,
                  MAX_VIDEO_WIDTH, MAX_VIDEO_HEIGHT, 0,
                  format->pf,
                  format->pt,
                  NULL);
     glerror = glGetError();
     if (glerror != GL_NO_ERROR)
        {
         texture_error = !0;
        }
     else
        {
         glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width); 
         texture_error = (width == 0);
        }
     glCALL(glDeleteTextures, (1, &texture));
     if (!texture_error)
        {
         break;                 /* success! */
        }
     else
        {
         if (modio.video)
            {
             xprintf("video_gl_probe_preferred_texture_format:\n"
                     "\tFormat %d doesn't support sufficiently large non-power-of-two textures,\n"
                     , i);
            }
        }


     /*
      * Determine whether a maximally-sized, power-of-two texture can
      * be created with this pixel format.  1x1 is a power of two in
      * both dimensions, but that texture size is not useful.
      */
     texture_error = 0;
     texture_pot = !0;
     glCALL(glGenTextures, (1, &texture));
     glCALL(glBindTexture,(GL_TEXTURE_2D, texture));
     glTexImage2D(GL_PROXY_TEXTURE_2D, 0,
                  format->i,
                  MAX_VIDEO_WIDTH_POT, MAX_VIDEO_HEIGHT_POT, 0,
                  format->pf,
                  format->pt,
                  NULL);
     glerror = glGetError();
     if (glerror != GL_NO_ERROR)
        {
         texture_error = !0;
        }
     else
        {
         glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width); 
         texture_error = (width == 0);
        }
     glCALL(glDeleteTextures, (1, &texture));
     if (texture_error)
        {
         if (modio.video)
            {
             xprintf("video_gl_probe_preferred_texture_format:\n"
                     "\tFormat %d doesn't support sufficiently large POT textures,\n"
                     "\ttrying the next one\n"
                     , i);
            }
         continue;
        }

     /*
      * Found a usable pixel format
      */
     break;
    }

 if (i < 0)
    {
     video_gl.pixel_format = 0;
     video_gl.pixel_type = 0;
     video_gl.internal_format = 0;
     video_gl.Rmask = 0;
     video_gl.Gmask = 0;
     video_gl.Bmask = 0;
     video_gl.Amask = 0;
     video_gl.bpp = video.bpp;

     xprintf("Could not determine a suitable texture format\n");
    }
 else
    {
     video_gl.pixel_format = format->pf;
     video_gl.pixel_type = format->pt;
     video_gl.internal_format = format->i;
     video_gl.Rmask = format->Rmask;
     video_gl.Gmask = format->Gmask;
     video_gl.Bmask = format->Bmask;
     video_gl.Amask = format->Amask;
     video_gl.bpp = format->bpp;
     video_gl.pot = texture_pot;

     if (modio.video)
        xprintf(
           "\n***********\n"
           "\n"
           "video_gl_probe_preferred_texture_format:\n"
           "picked texture format %d"
           "\n\t"
           " %2d bpp"
           " internal %04x"
           " pixel format %04x"
           " pixel type %04x"
           "\n\t"
           " Rmask 0x%08x"
           " Gmask 0x%08x"
           " Bmask 0x%08x"
           " Amask 0x%08x"
           "\n"
           "\n***********\n"
           "\n"
           "\n",
           format - formats,
           format->bpp,
           format->i,
           format->pf,
           format->pt,
           format->Rmask,
           format->Gmask,
           format->Bmask,
           format->Amask
           );
    }
}


//==============================================================================
// Create an OpenGL texture.
//
//   pass: void
// return: void
//==============================================================================
static void video_gl_create_texture (void)
{

 glCLEARERROR();

 if (video_gl.ntextures)
    {
     glCALL(glDeleteTextures,(video_gl.ntextures, &video_gl.texture));
     video_gl.texture = 0;
     video_gl.ntextures = 0;
    }

 if (screen)
     SDL_FreeSurface(screen);

 screen = SDL_CreateRGBSurface(SDL_SWSURFACE, 
                               video_gl.texture_w, video_gl.texture_h, video_gl.bpp,
                               video_gl.Rmask, video_gl.Gmask, video_gl.Bmask, video_gl.Amask);

 if (screen == NULL)
    return;                     /* FIXME: need to signal a failure! */
 
 switch (screen->format->BytesPerPixel)
    {
     case 4 :
        video_putpixel_fast_p = video_putpixel_fast_32bpp;
        break;
     case 2 :
        video_putpixel_fast_p = video_putpixel_fast_16bpp;
        break;
     case 1 :
        video_putpixel_fast_p = video_putpixel_fast_8bpp;
        break;
    }

 video_gl.ntextures = 1;
 glCALL(glGenTextures,(video_gl.ntextures, &video_gl.texture));
 glCALL(glBindTexture,(GL_TEXTURE_2D, video_gl.texture));
 glCALL(glTexImage2D, (GL_TEXTURE_2D, 0,
                        video_gl.internal_format,
                        screen->w, screen->h, 0,
                        video_gl.pixel_format,
                        video_gl.pixel_type,
                        screen->pixels));
 glCALL(glEnable,(GL_TEXTURE_2D));
 // Calls to set texture parameters may fail silently if the parameter
 // isn't supported by the driver.
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

 video_gl_filter_set(video_gl_selected_filter());

 glFlush();
 glCLEARERROR();
}

//==============================================================================
// Window OpenGL resizing.
//
// Resizes a Window and switches between windowed and fullscreen mode.
//
//   pass: int crt_w            CRT display width
//         int crt_h            CRT display height
//         int win_w            window width resolution wanted
//         int win_h            window height resolution wanted
// return: void
//==============================================================================
static void video_gl_window_resize (int crt_w, int crt_h, int win_w, int win_h)
{
 // don't do anything if the values were not accepted
 if (video_gl_values(crt_w, crt_h, win_w, win_h) == -1)
    return;

 video_update_sdl_video_flags();

 if ((gl_screen = SDL_SetVideoMode(video.gl_window_w, video.gl_window_h, 0, video.flags)) == NULL)
    xprintf("video_create_surface: SDL_SetVideoMode failed - %s\n", SDL_GetError());
 video_gl_clear_display_borders();

 if ( (emu.system & EMU_SYSTEM_UNIX) != EMU_SYSTEM_UNIX )
    {
     /*
      * On Windows and MacOS X, the OpenGL context is destroyed when
      * the window size is changed, taking any textures with it.
      */
     video_gl_initialise_context();
     video_gl_create_texture();
    }
 video_gl_update_transformation_matrix();
 video_gl_filter_set(video_gl_selected_filter());
}
#endif

//==============================================================================
// Create an SDL surface on which the emulator's pixel output is to be drawn
//
// This surface is either a display surface (software rendering) or a texture
// (OpenGL).
//
// This function will endeavour to resize the application window to display
// the contents of the new surface given the desired display aspect ratio
// (set with video_configure()) and whether the application window has been maximised
// or is full screen.
//
//   pass: int crt_w            CRT display width
//         int crt_h            CRT display height
// return: int                  0 if no error, -1 if error
//==============================================================================
void video_update_sdl_video_flags(void)
{
 video.flags = 0;

 if (video.fullscreen)
    video.flags |= SDL_FULLSCREEN;

 switch (video.type)
    {
     case VIDEO_SDLSW : // SDL software rendering
         video.flags |= SDL_SWSURFACE | SDL_ASYNCBLIT;
        break;
     case VIDEO_SDLHW : // SDL hardware rendering
         video.flags |= SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ASYNCBLIT;
        break;
#ifdef USE_OPENGL
     case VIDEO_GL : // OpenGL texture method
        video.flags |= SDL_OPENGL | SDL_RESIZABLE;
        break;
#endif
    }
}

int video_create_surface (int crt_w, int crt_h)
{
 int i;

 video_update_sdl_video_flags();

 if (video.fullscreen)
    SDL_ShowCursor(SDL_DISABLE); // don't show the mouse cursor
 else if (! mouse.host_in_use)
    SDL_ShowCursor(SDL_ENABLE); // show the mouse cursor

 switch (video.type)
    {
     /*
      * SDL rendering modes
      */
     case VIDEO_SDLSW:
     case VIDEO_SDLHW:
        switch (video.depth)
           {
            case VIDEO_8 :
               video.bpp = 8;
               screen = SDL_SetVideoMode(crt_w, crt_h, 8, video.flags);
               break;
            case VIDEO_8GS :
               video.bpp = 8;
               screen = SDL_SetVideoMode(crt_w, crt_h, 8, video.flags);
               if (screen)
                  {
                   for (i = 0; i < 256; i++) // create a grey scale
                      {
                       colors[i].r = i;
                       colors[i].g = i;
                       colors[i].b = i;
                      }
                   SDL_SetColors(screen, colors, 0, 256);
                   SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, colors, 0, 256);
                  }
               break;
            case VIDEO_16 :
               video.bpp = 16;
               screen = SDL_SetVideoMode(crt_w, crt_h, 16, video.flags);
               break;
            case VIDEO_32 :
               video.bpp = 32;
               screen = SDL_SetVideoMode(crt_w, crt_h, 32, video.flags);
               break;
           }
        if (screen == NULL)
           {
            xprintf("video_create_surface: SDL_SetVideoMode failed - %s\n", SDL_GetError());
            return -1;
           }
        switch (screen->format->BytesPerPixel)
           {
            case 4 :
               video_putpixel_fast_p = video_putpixel_fast_32bpp;
               break;
            case 2 :
               video_putpixel_fast_p = video_putpixel_fast_16bpp;
               break;
            case 1 :
               video_putpixel_fast_p = video_putpixel_fast_8bpp;
               break;
           }
        break;

#ifdef USE_OPENGL
        /*
         * OpenGL rendering modes
         */
     case VIDEO_GL :
        video.aspect = 1;   // OpenGL will do all the Y axis stretching

        // Calculate a desired window size based on whether the window
        // is maximised or fullscreen.  Do nothing if the proposed
        // values won't fit.
        if (video.maximised && !video.fullscreen)
           {
            if (video_gl_values(crt_w, crt_h,
                                video.maximised_w, video.maximised_h) == -1)
               return -1;
           }
        else
           {
            if (video_gl_values(crt_w, crt_h,
                                video.last_win_w, video.last_win_h) == -1)
               return -1;
           }

        video_gl_create_texture();

        break;
#endif
    }

 video_free_update_regions();
 video_init_update_regions();

 video_report_information();

 return 0;
}

//==============================================================================
// Video renderer.
//
// video type   rendering method
// ----------   ----------------
//     0        SDL Software rendering
//     1        SDL Hardware rendering
//     2        OpenGL texture rendering
//
//   pass: void
// return: void
//==============================================================================
void video_render (void)
{

#ifdef USE_OPENGL
 if (video.type == VIDEO_GL)
    {
     GLenum glerror = 0;

     glCLEARERROR();
     glBindTexture(GL_TEXTURE_2D, video_gl.texture);
     glerror = glGetError();
     if (glerror != GL_NO_ERROR)
        {
         // An error message is preferable to crashing
         xprintf("video_render: could not bind texture (%d) 0x%04x"
                 " format 0x%04x type 0x%04x"
                 " (error 0x%04x)\n",
                 video_gl.ntextures, video_gl.texture,
                 video_gl.pixel_format, video_gl.pixel_type,
                 glerror);
         glCLEARERROR();
        }
     /*
      * Textures which are in use can't be updated until the video
      * card has finished rendering a frame, so the number of calls to
      * glTexSubImage2D() should be minimised.
      *
      * glTexSubImage2D() expects the pixels comprising the region to
      * be contiguous.
      *
      * Compute the vertical extent of the texture regions updated
      */
     {
      int i;
      Sint16 miny = 0, maxy = 0;
      SDL_Rect *rp;
      void *pixptr;
      for (i = 0, rp = video_update_regions->rects.p;
           i < video_update_regions->rects.a.numentries;
           ++i, ++rp)
         {
          if (rp->w == 0 || rp->h == 0)
             continue;
          if (rp->y < miny)
             miny = rp->y;
          if (rp->y + rp->h > maxy)
             maxy = rp->y + rp->h;
         }
      if (maxy - miny > 0)
         {
          pixptr = 
             screen->pixels
             + miny * screen->pitch;
          glTexSubImage2D(GL_TEXTURE_2D, 0, 
                          0, miny, screen->w, maxy - miny,
                          video_gl.pixel_format, video_gl.pixel_type,
                          pixptr);
          glerror = glGetError();
         }
     }
#endif
     if (glerror != GL_NO_ERROR)
        {
         // An error message is preferable to crashing
         xprintf("video_render: can't update texture"
                 " format 0x%04x type 0x%04x"
                 " (error 0x%04x)\n",
                 video_gl.pixel_format, video_gl.pixel_type,
                 glerror);
         glCLEARERROR();
        }
     else
        {
         glBegin(GL_QUADS);
         glerror = glGetError();
         if (glerror == GL_NO_ERROR)
            {
             /* vertices are defined going anti-clockwise around the quadrilateral */
             /* top left */
             glTexCoord2f(0.0, 0.0);
             glVertex2i(video_gl.texture_region.x,
                        video_gl.texture_region.y);
             /* bottom left */
             glTexCoord2f(video_gl.texture_region_used_w, 0.0);
             glVertex2i(video_gl.texture_region.x + video_gl.texture_region.w, 
                        video_gl.texture_region.y);
             /* bottom right */
             glTexCoord2f(video_gl.texture_region_used_w, video_gl.texture_region_used_h);
             glVertex2i(video_gl.texture_region.x + video_gl.texture_region.w, 
                        video_gl.texture_region.y + video_gl.texture_region.h);
             /* top right */
             glTexCoord2f(0.0, video_gl.texture_region_used_h);
             glVertex2i(video_gl.texture_region.x, 
                        video_gl.texture_region.y + video_gl.texture_region.h);
             glEnd();
            }
         glCLEARERROR();
         SDL_GL_SwapBuffers();
        }
    }
 else
    if (video.type == VIDEO_SDLSW ||
        (screen->flags & SDL_DOUBLEBUF) != SDL_DOUBLEBUF)
       {
        // SDL software rendering, or rendering to a screen that isn't
        // double buffered
        SDL_UpdateRects(screen, 
                        video_update_regions->rects.a.numentries,
                        video_update_regions->rects.p);
       }
    else if (video.type == VIDEO_SDLHW)
       {
        // SDL hardware rendering
        SDL_Flip(screen);
       }
    else
       {
        // default, which shouldn't happen!
       }
 video_free_update_regions();
 video_init_update_regions();
}

#ifdef USE_OPENGL
//==============================================================================
// Resize or create a new surface depending on the platform in use.
//
//   pass: int crt_w            CRT display width
//         int crt_h            CRT display height
//         int win_w            wanted window width
//         int win_h            wanted window height
// return: int                  0 if no error, -1 if error
//==============================================================================
int video_gl_create_surface (int crt_w, int crt_h, int win_w, int win_h)
{
 video_gl_window_resize(crt_w, crt_h, win_w, win_h);

 crtc_set_redraw();

 if (emu.display_context == EMU_OSD_CONTEXT)
    osd_redraw();
 video_render();
 return 0;
}

//==============================================================================
// Window resize event handler.
//
// When changing the window size manually and the dimensions exceed or equal
// the maximum window size a resize event may be issued.  Those places that
// do this can set 'ignore_one_resize_event' equal to 1 to ignore the event
// and preventing it interfering with the manual change.
//
// The 'ignore_one_resize_event' variable is cleared on each video_update()
// call to prevent normal resize events being missed.  This may cancel out
// any ignore request but won't matter when scrolling window sizes.
//
//   pass: void
// return: void
//==============================================================================
void video_gl_resize_event (void)
{
 int crt_w;
 int crt_h;

 if (video.type != VIDEO_GL)
    return;

// ignore one resize event caused when manually changing the display
 if (ignore_one_resize_event)
    {
     ignore_one_resize_event = 0;
     return;
    }

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;

 /*
  * Detecting that the window has been maximised is tricky.
  * SDL_GetVideoInfo() returns only information about the current
  * video mode; the displayable area on the desktop is assumed to be
  * exactly the same as the screen size.  The problem is that the
  * maximum displayable area depends on the decorations and toolbars
  * displayed by the window manager and/or commonly installed office
  * software.
  *
  * A heuristic is used to determine if the window is maximised.  If
  * the window size from the event is the same as the screen size in
  * one dimension and within say 90% in the other dimension, assume
  * that the window has been maximised.
  */

 video.maximised = 
     ((emu.event.resize.w == video.desktop_w && emu.event.resize.h >= video.desktop_h * 9 / 10)
      ||
      (emu.event.resize.h >= video.desktop_h * 9 / 10 && emu.event.resize.w >= video.desktop_w * 9 / 10)
      );

 if (video.maximised)
    {
     video.maximised_w = emu.event.resize.w;
     video.maximised_h = emu.event.resize.h;

     video_gl_clear_display_borders();
    }

 video_gl_create_surface(crt_w, crt_h, emu.event.resize.w, emu.event.resize.h);

 gui_status_set_persist(GUI_PERSIST_WIN, 0);
 gui_status_update();
}

//==============================================================================
// Window size setting by command.
//
//   pass: int p                p is percentage in multiples of 10% of the
//                              desktop width or 0 to use current CRT width
//                              size.
// return: void
//==============================================================================
void video_gl_set_size (int p)
{
 int crt_w;
 int crt_h;
 int win_w;

 if ((video.type != VIDEO_GL) || (video.maximised))
    return;

 video.fullscreen = 0;

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;

 if (p == 0)
    {
     // set window size equal to the current CRT size
     video_gl_create_surface(crt_w, crt_h, crt_w, crt_h);
    }
 else
    if ((p > 0) && (p < 10))
       {
        // set window size by percentage of desktop width
        win_w = (int)(((double)video.desktop_w / 10.0) * p);
        video_gl_create_surface(crt_w, crt_h, win_w, 0);
       }
}

//==============================================================================
// Window size setting by incrementing a percentage value.
//
//   pass: int increment
// return: void
//==============================================================================
void video_gl_set_size_increment (int increment)
{
 int crt_w;
 int crt_h;
 int win_w;

 if ((video.type != VIDEO_GL) || video.fullscreen || video.maximised)
    return;

// make the next resize event be ignored
 ignore_one_resize_event = 1;

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;

 video.percent_size += increment;
 if (video.percent_size > VIDEO_MAX_PERCENT)
    video.percent_size = VIDEO_MAX_PERCENT;
 if (video.percent_size < VIDEO_MIN_PERCENT)
    video.percent_size = VIDEO_MIN_PERCENT;

 win_w = (int)(((double)video.desktop_w / 100.0) * video.percent_size);
 video_gl_create_surface(crt_w, crt_h, win_w, 0);
}

//==============================================================================
// Update the display after a change to the OpenGL video filter settings
//
//   pass: void
// return: void
//==============================================================================
void video_gl_filter_change_redraw (void)
{
 video_gl_filter_set(video_gl_selected_filter());
 video_render();
}

//==============================================================================
// Toggle OpenGL (textured) filter mode.  Provides soft and sharp display
// rendering for each of the 3 possible filters depending on the current
// display type/size.
//
//   pass: void
// return: void
//==============================================================================
void video_gl_filter_toggle (void)
{
 if (video.type != VIDEO_GL)
    return;

 if (video.fullscreen)
    video.filter_fs = ! video.filter_fs;
 else if (video.maximised)
    video.filter_max = ! video.filter_max;
 else
    video.filter_win = ! video.filter_win;
 video_gl_filter_change_redraw();
}

//==============================================================================
// Set the aspect ratio of the Microbee window.
//
//   pass: float aspect
// return: int                          0 if no error, -1 if error
//==============================================================================
int video_gl_set_aspect_bee (float aspect)
{
 if ((aspect < 0.1) || (aspect > 10.0))
    return -1;

 video.aspect_bee = aspect;
 return 0;
}

//==============================================================================
// Set the aspect ratio of the host monitor.
//
//   pass: float aspect
// return: int                          0 if no error, -1 if error
//==============================================================================
int video_gl_set_aspect_mon (float aspect)
{
 if ((aspect < 0.1) || (aspect > 10.0))
    return -1;

 video.aspect_mon = aspect;
 return 0;
}

//==============================================================================
// Set initial window size value from a set number of pixels wide.  If the
// value is large i.e. side window borderings + width => desktop width than
// SDL will place the window into maximised window mode.
//
//   pass: int pixels                   number of piexels wide
// return: int                          0 if no error, -1 if error
//==============================================================================
int video_gl_set_size_pixels (int pixels)
{
 if (pixels < 50)
    return -1;

 video.initial_x_pixels = pixels;
 video.initial_x_percent = 0;
 return 0;
}

//==============================================================================
// Set initial window size value from a percentage value.  If the value
// is large i.e. side window borderings + width => desktop width than SDL
// will place the window into maximised window mode.
//
//   pass: int pixels                   number of piexels wide
// return: int                          0 if no error, -1 if error
//==============================================================================
int video_gl_set_size_percent (int percent)
{
 if ((percent < 5) || (percent > 100))
    return -1;

 video.initial_x_percent = percent;
 video.initial_x_pixels = 0;
 return 0;
}

//==============================================================================
// Update the OpenGL filter to the current values.  This is intended to be
// called from the options during run mode.
//
//   pass: int pixels                   number of piexels wide
// return: void
//==============================================================================
void video_gl_filter_update (void)
{
 if (!emu.runmode)
    return;
 if (video.type != VIDEO_GL)
    return;
 video_gl_filter_change_redraw();
}
#endif

//==============================================================================
// Video configure.
//
// Determines the displayed aspect ratio to use.
//
//   pass: int aspect
// return: void
//==============================================================================
void video_configure (int aspect)
{
 if (video.type != VIDEO_GL)
    {
     video.yscale = aspect;
    }
 else
    {
     video.yscale = 1;
    }
}

//==============================================================================
// Write a pixel to the display buffer.
//
// If X or Y co-ordinates are out of range then the function returns without
// doing anything.  Out of range co-ordinates can be considered normal when
// OSD dialogues are dragged off screen.
//
//   pass: int x
//         int y
//         uint32_t col
// return: void
//==============================================================================
void video_putpixel (int x, int y, int col)
{
 if ((x < 0) || (x >= screen->w) || (y < 0) || (y >= screen->h))
    return;
 video_putpixel_fast(x, y, col);
}

//==============================================================================
// "Fast" pixel drawing functions.
//==============================================================================
void video_putpixel_fast_8bpp(int x, int y, int val)
{
    void *p = screen->pixels + y * screen->pitch + x * 1;
    *(uint8_t *)p = val;
}
void video_putpixel_fast_16bpp(int x, int y, int val)
{
    void *p = screen->pixels + y * screen->pitch + x * 2;
    *(uint16_t *)p = val;
}
void video_putpixel_fast_32bpp(int x, int y, int val)
{
    void *p = screen->pixels + y * screen->pitch + x * 4;
    *(uint32_t *)p = val;
}


//==============================================================================
// Toggle the display mode between fullscreen and windowed
//
//   pass: void
// return: int                  0 if no error, -1 if error
//==============================================================================
int video_toggledisplay (void)
{
 int mouse_x;
 int mouse_y;

 int x;
 int y;

 int crt_w;
 int crt_h;

 crt_w = crtc.hdisp * 8;
 crt_h = crtc.vdisp * crtc.scans_per_row;

 video.fullscreen = (! video.fullscreen);

 SDL_GetMouseState(&mouse_x, &mouse_y);
 video_convert_mouse_to_crtc_xy(mouse_x, mouse_y, &x, &y);

#ifdef USE_OPENGL
 if (video.type == VIDEO_GL)
    {
     video_gl_create_surface(crt_w, crt_h, video.last_win_w, video.last_win_h);
    }
 else
#endif
    {
     /*
      * Having changed the fullscreen flag, create a new video
      * surface.  Note that SDL on a Unix platform may change the
      * video mode (if it supports that) to the best fit for the
      * requested surface size.  If the surface won't fit, set the
      * aspect ratio to 1 and try again.
      */
     if (video_create_surface(crt_w, crt_h * video.yscale) == -1)
        {
         video_configure(1);
         if (video_create_surface(crt_w, crt_h * video.yscale) == -1)
            return -1;
        }
     vdu_configure(video.yscale);

     crtc_set_redraw();

     crtc_redraw();
     if (emu.display_context == EMU_OSD_CONTEXT)
        osd_redraw();
     video_render();
    }
 gui_changed_videostate();
 video_convert_crtc_to_mouse_xy(x, y, &mouse_x, &mouse_y);
 SDL_WarpMouse(mouse_x, mouse_y);
 return 0;
}

//==============================================================================
// Video update. This is called after each Z80 code frame has completed.
//
// Redraws the surface then updates the display if required. The crtc.update
// flag greatly reduces host CPU time.
//
//   pass: void
// return: void
//==============================================================================
void video_update (void)
{
 osd_update();          // sets the crtc.update flag if OSD needs refreshing

 crtc_redraw();         // only redraws if corresponding flag is set.

#ifdef USE_OPENGL
// re-enable resize events after changing window size manually.
 ignore_one_resize_event = 0;
#endif

 if (crtc.update)
    {
     if (emu.display_context == EMU_OSD_CONTEXT)
        osd_redraw();
     video_render();
     crtc.update = 0;
    }
}

//==============================================================================
// Video commands.
//
//   pass: int cmd                      video command
//         int p                        parameter
// return: void
//==============================================================================
void video_command (int cmd, int p)
{
 switch (cmd)
    {
     case EMU_CMD_FULLSCR :
        video_toggledisplay();
        break;
     case EMU_CMD_SCREENI :
#ifdef USE_OPENGL
        if (video.type != VIDEO_GL)
           break;
        if ((gui_status.win) || (gui.persist_flags & GUI_PERSIST_WIN))
           video_gl_set_size_increment(VIDEO_INCREMENT_PERCENT);
        if (! gui_status.win)
           gui_status_set_persist(GUI_PERSIST_WIN, 0);
#endif
        break;
     case EMU_CMD_SCREEND :
#ifdef USE_OPENGL
        if (video.type != VIDEO_GL)
           break;
        if ((gui_status.win) || (gui.persist_flags & GUI_PERSIST_WIN))
           video_gl_set_size_increment(-VIDEO_INCREMENT_PERCENT);
        if (! gui_status.win)
           gui_status_set_persist(GUI_PERSIST_WIN, 0);
#endif
        break;
     case EMU_CMD_VIDSIZE1 :
#ifdef USE_OPENGL
        if (video.type != VIDEO_GL)
           break;
        video_gl_set_size(p);
        if (! gui_status.win)
           gui_status_set_persist(GUI_PERSIST_WIN, 0);
#endif
        break;
     case EMU_CMD_GL_FILTER :
#ifdef USE_OPENGL
        if (video.type != VIDEO_GL)
           break;
        video_gl_filter_toggle();
#endif
        break;
    }
}

/* ==============================================================================
 *
 * Update region management
 *
 * Rectangular regions to update are stored as an array of SDL_Rects.
 *
 * Regions added to the set of regions are coalesced where possible.
 * Two regions are coalesced when
 * a) they are vertically adjacent and their widths are equal
 * b) they are horizontally adjacent and their heights are equal
 *
 */

int video_resize_array(struct _dynamic_array *ap, int size)
{
 struct _dynamic_array *p;

 if ((p = realloc(ap->p, size * ap->entrysize)) == NULL)
    return 0;
 ap->p = p;
 ap->size = size;
 return !0;
}

void video_free_array(struct _dynamic_array *ap)
{
 if (ap->p != NULL)
    free(ap->p);
 ap->p = NULL;
 ap->size = 0;
 ap->entrysize = 0;
 ap->numentries = 0;
}

void video_init_array(struct _dynamic_array *ap, int entrysize)
{
 ap->p = NULL;
 ap->size = 0;
 ap->entrysize = entrysize;
 ap->numentries = 0;
}

void video_add_update_region(SDL_Rect r)
{
 int i;
 SDL_Rect *p = video_update_regions->rects.p;
 
 for (i = 0; i < video_update_regions->rects.a.numentries; ++i)
    {
     if (p[i].w == 0)
        {
         p[i] = r;
         return;
        }
    }
 if (i >= video_update_regions->rects.a.size)
    {
     video_resize_array(&video_update_regions->rects.a,
                        video_update_regions->rects.a.size + 20);
     p = video_update_regions->rects.p;
    }
 p[i] = r;
 video_update_regions->rects.a.numentries++;
}

//==============================================================================
//
// Add a rectangular region to the list of rectangular regions to redraw
//
//==============================================================================
void video_update_region(SDL_Rect r)
{
 /* This is a naive implementation which scans through the array of
  * regions.  O(n^2) */
 int i;
 SDL_Rect *p = video_update_regions->rects.p;

 for (i = 0; i < video_update_regions->rects.a.numentries; ++i)
    {
     /* Wholly contained within another region? */
     if (r.x >= p[i].x && r.x + r.w <= p[i].x + p[i].w &&
         r.y >= p[i].y && r.y + r.h <= p[i].y + p[i].h)
        return;                 /* this rectangle is already being
                                 * updated */
     /* Overlapping on top? */
     if (r.x == p[i].x && r.w == p[i].w &&
         r.y < p[i].y && r.y + r.h >= p[i].y)
        {
         r.h = p[i].y + p[i].h - r.y;
         p[i].w = p[i].h = 0;   /* mark slot free */
         video_update_region(r); /* coalesce the new region with
                                  * others, if possible */
         return;
        }
     /* Overlapping on the bottom? */
     if (r.x == p[i].x && r.w == p[i].w &&
         r.y >= p[i].y && r.y <= p[i].y + p[i].h)
        {
         r.h = r.y + r.h - p[i].y;
         r.y = p[i].y;
         p[i].w = p[i].h = 0;
         video_update_region(r);
         return;
        }
     /* Overlapping on the left? */
     if (r.y == p[i].y && r.h == p[i].h &&
         r.x < p[i].x && r.x + r.w >= p[i].x)
        {
         r.w = p[i].x + p[i].w - r.x;
         p[i].w = p[i].h = 0;
         video_update_region(r);
         return;
        }
     /* Overlapping on the right? */
     if (r.y == p[i].y && r.h == p[i].h &&
         r.x >= p[i].x && r.x <= p[i].x + p[i].w)
        {
         r.w = r.x + r.w - p[i].x;
         r.x = p[i].x;
         p[i].w = p[i].h = 0;
         video_update_region(r);
         return;
        }
    }
 /* The region couldn't be coalesced, so add it in the first free slot */
 video_add_update_region(r);
}

void video_free_update_regions()
{
 video_free_array(&video_update_regions->rects.a);
 video_free_array(&video_update_regions->free_rects.a);
}

void video_init_update_regions()
{
 // calloc enough space for the update_regions struct
 video_update_regions = calloc(1, sizeof(*video_update_regions));
 video_init_array(&video_update_regions->rects.a,
                  sizeof(*video_update_regions->rects.p));
 video_init_array(&video_update_regions->free_rects.a,
                  sizeof(*video_update_regions->free_rects.p));
}


