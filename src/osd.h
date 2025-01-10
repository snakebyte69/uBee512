/* OSD Header */

#ifndef HEADER_OSD_H
#define HEADER_OSD_H

#define BOX_ATTR_PIXEL0     0x00000000
#define BOX_ATTR_PIXEL1     0x00000001
#define BOX_ATTR_PIXEL2     0x00000002
#define BOX_ATTR_PIXEL3     0x00000003
#define BOX_ATTR_PIXEL4     0x00000004
#define BOX_ATTR_PIXEL5     0x00000005
#define BOX_ATTR_PIXEL6     0x00000006
#define BOX_ATTR_PIXEL7     0x00000007
#define BOX_ATTR_DASHED     0x00000008
#define BOX_ATTR_HIGH       0x00000010
#define BOX_ATTR_NOEXIT     0x00000020

#define MBOX_ATTR_VBTNS_LJ  0x00000001
#define MBOX_ATTR_VBTNS_RJ  0x00000002
#define MBOX_ATTR_MOUSEPOS  0x00000004
#define MBOX_ATTR_RESIZABLE 0x00000008
#define MBOX_ATTR_MAXIMISED 0x00000010

#define BOX_COMP_TITLE      0x00000001
#define BOX_COMP_MIN        0x00000002
#define BOX_COMP_MAX        0x00000004
#define BOX_COMP_CLOSE      0x00000008
#define BOX_COMP_BTN        0x00000010

#define OKCANCEL_BTN_OK     1
#define OKCANCEL_BTN_CANCEL 2

#define MENU_BTN_ABOUT      1
#define MENU_BTN_CONSOLE    2
#define MENU_BTN_OUTPUT     3
#define MENU_BTN_FULLSCREEN 4
#define MENU_BTN_SOUND      5
#define MENU_BTN_VOLUMEI    6
#define MENU_BTN_VOLUMED    7
#define MENU_BTN_TAPE       8
#define MENU_BTN_RESET      9
#define MENU_BTN_POWERCYC   10
#define MENU_BTN_EXIT       11

#define OUTPUT_BTN_NONE     1
#define OUTPUT_BTN_OSD      2
#define OUTPUT_BTN_STDOUT   3
#define OUTPUT_BTN_BOTH     4

#define OSD_ANIMATED_FRAMES 10
#define OSD_ANIMATED_TIME_TOTAL 100
#define OSD_ANIMATED_TIME_FRAME (OSD_ANIMATED_TIME_TOTAL / OSD_ANIMATED_FRAMES)

#if 1
#define OSD_FONT_DEPTH 8
#define OSD_FONT_WIDTH 8
#else
#define OSD_FONT_DEPTH 10
#define OSD_FONT_WIDTH 8
#endif

#define OSD_POS_MOUSEORCENTER 0
#define OSD_POS_UPDATE        1

#define OSD_FLAG_ANIMATE    0x00000001
#define OSD_FLAG_ALL        0xffffffff

#define OSD_CON_PERCENT_000 1000
#define OSD_CON_PERCENT_001 1001
#define OSD_CON_PERCENT_025 1025
#define OSD_CON_PERCENT_050 1050
#define OSD_CON_PERCENT_075 1075
#define OSD_CON_PERCENT_100 1100
#define OSD_CON_CENTER      2000
#define OSD_CON_LEFT        2001
#define OSD_CON_RIGHT       2002
#define OSD_CON_TOP         2003
#define OSD_CON_BOTTOM      2004
#define OSD_CON_MAX         2005

#define DIALOGUE_NOTINUSE   0
#define DIALOGUE_EXIT       1
#define DIALOGUE_POWERCYC   2
#define DIALOGUE_RESET      3
#define DIALOGUE_DEVMESG    4
#define DIALOGUE_OPENGL     5
#define DIALOGUE_CONSOLE    6
#define DIALOGUE_ABOUT      7
#define DIALOGUE_OUTPUT     8
#define DIALOGUE_MENU       9

#define DIALOGUE_PENDING_SIZE 20

#define MINIMISED_BOX_WIDTH 50

#define DIALOGUE_MENU_BUTTONS 11
#define DIALOGUE_MENU_WIDTH   117
#define DIALOGUE_OUTPUT_BUTTONS 4
#define BUTTON_WIDTH 80
#define BUTTON_DEPTH 17

#define SHARED_SIZE         1000
#define CONSOLE_SIZE        10000

int osd_init (void);
int osd_deinit (void);
int osd_reset (void);
void osd_dialogue_exit (void);
int osd_dialogue_result (int dialogue);
int osd_getkey (void);
void osd_keydown_event (void);
void osd_keyup_event (void);
void osd_set_focus (void);
void osd_mousebuttondown_event (void);
void osd_mousebuttonup_event (void);
void osd_mousemotion_event (void);
void osd_console_putchar (int c);
int osd_printf (char * fmt, ...)
               __attribute__ ((format (printf, 1, 2)));
void osd_set_dialogue (int dialogue);
void osd_proc_osd_args (int arg, int pf);
void osd_redraw (void);
void osd_update (void);

void osd_list_schemes (void);
void osd_set_cursor (int rate);
int osd_set_console_position (char *p);
int osd_set_console_size (char *p);
int osd_set_scheme (char *p);
int osd_set_colour (char *p, int option);

typedef struct box_t
   {
    int posx_s;          // box start x
    int posx_f;          // box finish x
    int posy_s;          // box start y
    int posy_f;          // box finish y
    int bcol;            // background colour
    int fcol;            // foreground colour
    int attr;            // attribute bits
    char *text;          // box text pointer
    int cursor_rate;     // cursor type
    int text_posx_s;     // text start x
    int text_posx_f;     // text finish x
    int text_posy_s;     // text start y
    int text_posy_f;     // text finish y
    int text_bcol;       // text background colour
    int text_fcol;       // text foreground colour
    int text_width;      // text characters width
    int text_depth;      // text characters depth
    int text_buf_count;  // text buffer count
    int text_buf_start;  // text buffer start position
    int text_buf_put;    // text buffer put position
   }box_t;

typedef struct mbox_t
   {
    box_t main;          // data for main dialogue box
    box_t title;         // data for a title box
    box_t min;           // data for a minimize box
    box_t max;           // data for a maximising box
    box_t close;         // data for a close box
    box_t btn[20];       // data for n button boxes
    char **icon;         // pointer to an XPM icon/image
    int attr;            // attribute bits
    int width;           // width of the main dialogue
    int depth;           // depth of the main dialogue
    int bwidth;          // width of the buttons
    int bdepth;          // depth of the buttons
    int text_posx_ofs;   // start text in from left
    int text_posy_ofs;   // start text down from top
    int dialogue;        // dialogue ID number
    int components;      // flags for box components
    int buttons;         // number of buttons
    int button_focus;    // current button focus
    int minimised;       // set if dialogue is minimised
    int result;
    int reset;           // force use default size/pos
   }mbox_t;

typedef struct font_t
   {
    uint8_t *data;
    int depth;
    int width;
    int x_s;
    int x_f;
    int y_s;
    int y_f;
    int xorig;
    int yorig;
    int bgc;
    int fgc;
   }font_t;

typedef struct x11_rgb_col_t
   {
    char *colour;
    int value;
   }x11_rgb_col_t;

typedef struct osd_t
   {
    int initialised;
    int dialogue;
    int flags;
    int key;
    int scheme;
    int schemes;
    int scheme_user;
   }osd_t;

// this must match the ordering for values found in osdsch_t
typedef struct osdsch_col_t
   {
    int col1;
    int col2;
    int col3;
    int col4;
   }osdsch_col_t;

// this values in each section must keep the order as shown
typedef struct osdsch_t
   {
    int dialogue_main_bcol;
    int dialogue_main_fcol;

    int dialogue_text_bcol;
    int dialogue_text_fcol;

    int widget_main_bcol_hl;
    int widget_main_bcol_ll;
    int widget_main_fcol_hl;
    int widget_main_fcol_ll;

    int widget_text_bcol_hl;
    int widget_text_bcol_ll;
    int widget_text_fcol_hl;
    int widget_text_fcol_ll;

    int widget_xpm_hl;
    int widget_xpm_ll;

    int button_main_bcol_hl;
    int button_main_bcol_ll;
    int button_main_fcol_hl;
    int button_main_fcol_ll;

    int button_text_bcol_hl;
    int button_text_bcol_ll;
    int button_text_fcol_hl;
    int button_text_fcol_ll;

    int console_cursor_rate;
    int console_width;
    int console_depth;
    int console_pos_x;
    int console_pos_y;
   }osdsch_t;

#endif /* HEADER_OSD_H */
