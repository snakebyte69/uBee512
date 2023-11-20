/* SCC Header */

#ifndef HEADER_SCC_H
#define HEADER_SCC_H

#include "z80.h"

int scc_init(void);
int scc_deinit(void);
int scc_reset(void);

uint16_t scc_r (uint16_t port, struct z80_port_read *port_s);
void scc_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

#endif  /* HEADER_SCC_H */
