/* 256TC Keyboard Header */

#ifndef HEADER_KEYTC_H
#define HEADER_KEYTC_H

#include "z80.h"

int keytc_init(void);
int keytc_deinit(void);
int keytc_reset(void);

void keytc_keydown_event (void);
void keytc_keyup_event (void);
int keytc_poll (void);
void keytc_force (int scan, int counts);
void keytc_force_none (int counts);

uint16_t keytc_r (uint16_t port, struct z80_port_read *port_s);
void keytc_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

#endif  /* HEADER_KEYTC_H */
