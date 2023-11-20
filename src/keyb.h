/* Keyboard Header */

#ifndef HEADER_KEYB_H
#define HEADER_KEYB_H

int keyb_init(void);
int keyb_deinit(void);
int keyb_reset(void);

void keyb_set_unicode (int enable);
void keyb_emu_command (int cmd, int p);
void keyb_repeat_start (void);
void keyb_repeat_stop (void);
void keyb_update (void);

void keyb_keydown_event (void);
void keyb_keyup_event (void);

void keyb_force (int scan, int counts);
void keyb_force_none (int counts);

#endif  /* HEADER_KEYB_H */
