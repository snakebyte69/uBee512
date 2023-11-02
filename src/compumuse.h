/* COMPUMUSE Header */

#ifndef HEADER_COMPUMUSE_H
#define HEADER_COMPUMUSE_H

int compumuse_init (void);
int compumuse_deinit (void);
int compumuse_reset (void);
void compumuse_clock (int clock);

typedef struct {
   sn76489an_t sn76489;
   int busy;
   uint64_t strobe_due;
   int clock;
   int init;
} compumuse_t;

#endif     /* HEADER_COMPUMUSE_H */
