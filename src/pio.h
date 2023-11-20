/* Z80 PIO Header */

#ifndef HEADER_PIO_H
#define HEADER_PIO_H

#include <inttypes.h>

#include "z80.h"
#include "parint.h"

int pio_init (void);
int pio_deinit (void);
int pio_reset (void);
void pio_polling (void);
void pio_porta_strobe(void);
void pio_configure (int cpuclock);
void pio_regdump (void);
uint16_t pio_r (uint16_t port, struct z80_port_read *port_s);
void pio_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
int pio_porta_connect(parint_ops_t *device);

#define PIO_B_CASIN     B8(00000001)
#define PIO_B_CASOUT    B8(00000010)
#define PIO_B_KEY256TC  B8(00000010)
#define PIO_B_RS232_DTR B8(00000100)
#define PIO_B_RS232_CTS B8(00001000)
#define PIO_B_RS232_RX  B8(00010000)
#define PIO_B_RS232_TX  B8(00100000)
#define PIO_B_SPEAKER   B8(01000000)
#define PIO_B_CLOCK     B8(10000000)
#define PIO_B_NETDIR    B8(10000000)
#define PIO_B_PUP       B8(10000000)
#define PIO_B_RTC       B8(10000000)

#define PIO_A_INTRPEND  0x00000001
#define PIO_B_INTRPEND  0x00000010

struct pio_s
{
 uint8_t data;
 uint8_t cont;
 uint8_t mode;
 uint8_t vector;
 uint8_t maskword;
 uint8_t direction;
 uint8_t data_in;
 uint8_t data_out;
 int action;
 int ienable;
 int andor;
 int hilo;
 int ienableff;
 int pending;
 int change;
 int last;
 SDL_mutex *pending_mutex;
};

typedef struct pio_s pio_t;

#endif     /* HEADER_PIO_H */
