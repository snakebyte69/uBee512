#ifndef _sn76489an_h
#define _sn76489an_h
/* $Id: sn76489an.h,v 1.1.1.1 2011/03/27 06:04:42 krd Exp $ */

int sn76489an_init (void);
int sn76489an_deinit (void);
int sn76489an_reset (void);
uint16_t sn76489an_r (uint16_t port, struct z80_port_read *port_s);
void sn76489an_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

#endif /* _sn76489an_h */
