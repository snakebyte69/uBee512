/* 6545 CRTC Header */

#ifndef HEADER_CRTC_H
#define HEADER_CRTC_H

#include "z80.h"

#define CRTC_HTOT           0
#define CRTC_HDISP          1
#define CRTC_HSYNC_POS      2
#define CRTC_SYNC_WIDTH     3
#define CRTC_VTOT           4
#define CRTC_VTOT_ADJ       5
#define CRTC_VDISP          6
#define CRTC_VSYNC_POS      7
#define CRTC_MODE           8
#define CRTC_SCANLINES      9
#define CRTC_CUR_START      10
#define CRTC_CUR_END        11
#define CRTC_DISP_START_H   12
#define CRTC_DISP_START_L   13
#define CRTC_CUR_POS_H      14
#define CRTC_CUR_POS_L      15
#define CRTC_LPEN_H         16
#define CRTC_LPEN_L         17
#define CRTC_SETADDR_H      18
#define CRTC_SETADDR_L      19

#define CRTC_DOSETADDR      31

extern int disp_start;

int crtc_init (void);
int crtc_deinit (void);
int crtc_reset (void);

void crtc_redraw (void);
void crtc_set_redraw (void);
void crtc_redraw_char (int addr, int dostdout);

void crtc_lpen (int addr);

int crtc_vblank (void);
uint16_t crtc_status_r (uint16_t port, struct z80_port_read *port_s);
void crtc_address_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);
uint16_t crtc_data_r (uint16_t port, struct z80_port_read *port_s);
void crtc_data_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

void crtc_update (void);
void crtc_regdump (void);
int crtc_set_flash_rate (int n);
void crtc_clock (int cpuclock);

typedef struct crtc_t
{
 int video;
 int vblank_method;
 int monitor;
 int flashvideo;
 int latchrom;
 int hdisp;
 int vdisp;
 int disp_start;
 int scans_per_row;
 int std_col_type;
 int resized;
 int flashrate;
 int flashvalue_c;
 int flashvalue_t;
 int lpen_valid;
 int update_strobe;
 int update;
}crtc_t;

#endif     /* HEADER_CRTC_H */
