/* Standard Keyboard Header */

#ifndef HEADER_KEYSTD_H
#define HEADER_KEYSTD_H

int keystd_init(void);
int keystd_deinit(void);
int keystd_reset(void);

void keystd_keydown_event (void);
void keystd_keyup_event (void);

void keystd_handler(int addr);
void keystd_checkall(void);
void keystd_force (int scan, int counts);
void keystd_force_none (int counts);

#endif  /* HEADER_KEYSTD_H */
