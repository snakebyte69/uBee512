/* GUI Header */

#ifndef HEADER_GUI_H
#define HEADER_GUI_H

// default persist time
#define GUI_PERSIST_TIME 3000

// mouse cursor persist tim
#define GUI_CURSOR_TIME 5000

// default padding between GUI status values
#define GUI_SPADDING 5

// persist flags, use 0x00000001 - 0x80000000
#define GUI_PERSIST_DRIVE 0x00000001
#define GUI_PERSIST_VOL   0x00000002
#define GUI_PERSIST_WIN   0x00000004

// assignments for mouse wheel events
#define GUI_MOUSE_WHEEL_NONE 0
#define GUI_MOUSE_WHEEL_VOL  1
#define GUI_MOUSE_WHEEL_WIN  2

int gui_init (void);
int gui_deinit (void);
int gui_reset (void);

int gui_toggledisplay (void);
int gui_message_box (int buttons, char *s);

int gui_status_padding (int n);
void gui_status_update (void);
void gui_status_set_persist (int f, int p);

void gui_mousebuttondown_event (void);
void gui_mousebuttonup_event (void);
void gui_mousemotion_event (void);

void gui_update (void);
void gui_changed_videostate (void);
void gui_proc_status_args (int arg, int pf);
void gui_command (int cmd);

#define BUTTON_OK 1
#define BUTTON_OKCANCEL 2

#define BUTTON_IDOK 1
#define BUTTON_IDCANCEL 2

typedef struct gui_t
   {
    int button_l;
    int button_m;
    int button_r;
    int button_wu;
    int button_wd;
    int dclick_time;
    int mouse_wheel;
    int persist_flags;
    int persist_time;
    uint64_t drive_persist_timer;
    uint64_t volume_persist_timer;
    uint64_t window_persist_timer;
    char title[50];
   }gui_t;

typedef struct gui_status_t
   {
    int emuver;
    int emu;
    int left;
    int longdrive;
    int joy;
    int model;
    int mouse;
    int mute;
    int print;
    int ram;
    int speed;
    int serial;
    int shortdrive;
    int sys;
    int tape;
    int title;
    int ver;
    int vol;
    int win;
   }gui_status_t;

#endif /* HEADER_GUI_H */
