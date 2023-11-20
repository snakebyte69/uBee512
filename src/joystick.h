/* Joystick Header */

#ifndef HEADER_JOYSTICK_H
#define HEADER_JOYSTICK_H

#include <inttypes.h>
#include "z80.h"
#include "macros.h"

#define JOY_BUTTONS     128
#define JOY_KB_SETS     256
#define JOY_SHIFT_BASE  256
#define JOY_SHIFT_BTN   7

#define JOY_MB_UP       B8(00000001)
#define JOY_MB_RIGHT    B8(00001000)
#define JOY_MB_DOWN     B8(00000010)
#define JOY_MB_LEFT     B8(00000100)
#define JOY_MB_FIRE     B8(10000000)
#define JOY_MB_PLAY1    B8(00010000)
#define JOY_MB_PLAY2    B8(00100000)
#define JOY_MB_SPARE    B8(01000000)

int joystick_init (void);
int joystick_deinit (void);
int joystick_reset (void);

void joystick_buttondown_event (void);
void joystick_buttonup_event (void);
void joystick_hatmotion_event (void);
void joystick_axismotion_event (void);

void joystick_mbjoy_clear (void);
int joystick_mbjoy_set_action (int action, char *p);
int joystick_kbjoy_key (char *key);
int joystick_kbjoy_button (int button);
int joystick_kbjoy_keybuttons (char *p);
int joystick_kbjoy_set (int set, char *set_str);
void joystick_kbjoy_listkeys (void);
void joystick_kbjoy_listcommands (void);
int joystick_kbjoy_select (int set, char *set_str);
void joystick_command (int cmd, int p);

typedef struct joystick_t
   {
    SDL_Joystick *joy;
    int used;
    int mbee;
    int kbd;
    int shift_button;
    int shift_inuse;
    int set;
    int axis_used;
    int axis_buttons;
    int axis_level;
    int hat_used;
    int hat_buttons;
    uint8_t data;
   }joystick_t;

#endif     /* HEADER_JOYSTICK_H */
