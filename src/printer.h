/* Printer Header */

#ifndef HEADER_PRINTER_H
#define HEADER_PRINTER_H

#include <inttypes.h>
#include "ubee512.h"
#include "z80.h"

void printer_a_close (void);
void printer_b_close (void);
int printer_a_open (char *s, int action);
int printer_b_open (char *s, int action);

typedef struct printer_t
   {
    FILE *print_a_file;
    FILE *print_b_file;
    char printa[SSIZE1];
    char printb[SSIZE1];
    int count;
    int busy;
    uint8_t data;
    uint64_t strobe_due;
   }printer_t;

#endif     /* HEADER_PRINTER_H */
