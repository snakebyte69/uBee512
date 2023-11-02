/* Serial Header */

#ifndef HEADER_SERIAL_H
#define HEADER_SERIAL_H

#include <inttypes.h>
#include "z80.h"
#include "ubee512.h"

#define SERIAL_RX_BAUD 300
#define SERIAL_TX_BAUD 300
#define SERIAL_DATABITS 8
#define SERIAL_STOPBITS 1
#define SERIAL_STARTBIT_TX 0
#define SERIAL_STOPBIT_TX 1

int serial_init (void);
int serial_deinit (void);
int serial_reset (void);
int serial_open (char *s, int port, int action);
int serial_close (int port);
void serial_interrupt_adjust (void);
int serial_r (void);
void serial_w (uint8_t data);
void serial_config (int cpuclock);

typedef struct serial_t
{
 int tx_baud;
 int rx_baud;
 int databits;
 int stopbits;
 int byte_rx;
 char coms1[SSIZE1];
}serial_t;

#endif     /* HEADER_SERIAL_H */
