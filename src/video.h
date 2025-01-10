/* VIDEO Header */

#ifndef HEADER_VIDEO_H
#define HEADER_VIDEO_H

#ifdef USE_OPENGL
#include <SDL/SDL_opengl.h>
#endif

// Reasonable maximum video resolution (80x25 @ 11 lines/char)
#define MAX_VIDEO_WIDTH (80 * 8)
#define MAX_VIDEO_HEIGHT (25 * 11)
#ifdef USE_OPENGL
#define MAX_VIDEO_WIDTH_POT 1024
#define MAX_VIDEO_HEIGHT_POT 512
#endif

// video types
#define VIDEO_SDLSW 0
#define VIDEO_SDLHW 1
#define VIDEO_GL 2

// video depths
#define VIDEO_8 0
#define VIDEO_8GS 1
#define VIDEO_16 2
#define VIDEO_32 3

#ifdef USE_OPENGL
// OpenGL video filter values
#define VIDEO_SOFT 0
#define VIDEO_SHARP 1

// OpenGL screen size incrementing
#define VIDEO_INCREMENT_PERCENT 2
#define VIDEO_MIN_PERCENT 5
#define VIDEO_MAX_PERCENT 99 // don't use 100%

// aspect ratio of a Microbee screen.  Must be expressed as a floating
// point fraction to preserve as much precision as possible.
#define VIDEO_ASPECT_BEE (4.0f / 3.0f)

// Full screen aspect handling
#define VIDEO_ASPECT_FS_AUTO    0
#define VIDEO_ASPECT_FS_STRETCH 1
#define VIDEO_ASPECT_FS_KEEP    2

// OpenGL vsync values
#define VIDEO_VSYNC_OFF 0
#define VIDEO_VSYNC_ON 1
#endif

// Convert floating point display ratios to an integer for testing
#define VIDEO_DISP_RATIO (int)(disp_ratio * 100)
#define VIDEO_UBEE_RATIO (int)(video.aspect_bee * 100)
#define VIDEO_STRETCH_RATIO (int)(video.stretch_fs * 100)

int video_init (void);
int video_deinit (void);
int video_reset (void);
void video_convert_mouse_to_crtc_xy (int mouse_x, int mouse_y, int *x, int *y);
void video_convert_crtc_to_mouse_xy (int crtc_x, int crtc_y, int *x, int *y);
int video_create_window (int crt_w, int crt_h);
int video_create_surface (int crt_w, int crt_h);
void video_render (void);

#ifdef USE_OPENGL
void video_gl_set_size (int p);
void video_gl_set_size_increment (int dir);
int video_gl_set_size_pixels (int pixels);
int video_gl_set_size_percent (int percent);
void video_gl_resize_event (void);
void video_gl_filter_toggle (void);
int video_gl_set_aspect_bee (float aspect);
int video_gl_set_aspect_mon (float aspect);
int video_gl_stretch_fs (float aspect);
void video_gl_filter_update (void);
#endif
void video_configure (int aspect);

typedef void (*video_putpixel_fast_fn)(int x, int y, int val);
void video_putpixel (int x, int y, int col);
static inline void video_putpixel_fast(int x, int y, int val)
{
 extern video_putpixel_fast_fn video_putpixel_fast_p;
 (*video_putpixel_fast_p)(x, y, val);
}
int video_toggledisplay (void);
void video_update (void);
void video_update_region(SDL_Rect r);
void video_command (int cmd, int p);

#ifdef USE_OPENGL
typedef struct video_gl_t
   {
    int ntextures;
    GLuint texture;
    int texture_w;
    int texture_h;

    int pot;         /* texture's dimensions must be a power of two */

    SDL_Rect texture_region;
    float texture_region_used_w, texture_region_used_h;

    GLenum target;
    GLint internal_format;
    GLenum pixel_format;
    GLenum pixel_type;
    GLint filter;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;
   }video_gl_t;
#endif

typedef struct video_t
   {
    int desktop_w;
    int desktop_h;

    int offset_x;
    int offset_y;

    int depth;
    int type;
    int fullscreen;

    int aspect;
    int yscale;

    int flags;
    int bpp;

#ifdef USE_OPENGL
    int gl_window_w;
    int gl_window_h;

    int maximised_w;
    int maximised_h;

    int last_win_w;
    int last_win_h;

    int maximised;

    int filter_fs;
    int filter_win;
    int filter_max;

    float aspect_mon;
    float aspect_bee;
    float stretch_fs;
    int aspect_fs;

    int max;
    int vsync;

    int initial_x_pixels;
    int initial_x_percent;
    int percent_size;
#endif
   } video_t;

#endif /* HEADER_VIDEO_H */
