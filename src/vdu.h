/* VDU Header */

#ifndef HEADER_VDU_H
#define HEADER_VDU_H

#include "ubee512.h"
#include "z80.h"

// Alpha+ (Premium) variables, 8K for Screen, Colour and attribute, 32K for PCG
#define SCR_RAM_BANKS 4
#define COL_RAM_BANKS 4
#define ATT_RAM_BANKS 4
#define PCG_RAM_BANKS 16

#define CHR_ROM_SIZE 0x4000
#define SCR_RAM_SIZE 0x0800 * SCR_RAM_BANKS
#define COL_RAM_SIZE 0x0800 * COL_RAM_BANKS
#define ATT_RAM_SIZE 0x0800 * ATT_RAM_BANKS
#define PCG_RAM_SIZE 0x0800 * PCG_RAM_BANKS

// #defines for the hardware flashing circuit
#define HFNO  0
#define HFV3  1
#define HFV4  2

// CGA Colour monitor intensity values
#if 0
#define C_FI    252     // colour full intensity
#define C_HI    168     // colour half intensity
#define C_LI    84      // colour low intensity
#else
// use CGA values as defined at:
// http://en.wikipedia.org/wiki/Color_Graphics_Adapter
#define C_FI    0xff    // colour full intensity
#define C_HI    0xaa    // colour half intensity
#define C_LI    0x55    // colour low intensity
#endif

// EGA Colour monitor intensity values
#define E_FF    255     // colour full intensity
#define E_AA    170     // colour half intensity
#define E_55    85      // colour low intensity

// Green monitor
#define MONGR_BGR   0
#define MONGR_BGG   0
#define MONGR_BGB   0
#define MONGR_BGR_I 0
#define MONGR_BGG_I 80
#define MONGR_BGB_I 0
#define MONGR_FGR   0
#define MONGR_FGG   210
#define MONGR_FGB   0
#define MONGR_FGR_I 0
#define MONGR_FGG_I 252
#define MONGR_FGB_I 0

// Amber monitor
#define MONAM_BGR   0
#define MONAM_BGG   0
#define MONAM_BGB   0
#define MONAM_BGR_I 158
#define MONAM_BGG_I 110
#define MONAM_BGB_I 0
#define MONAM_FGR   238
#define MONAM_FGG   166
#define MONAM_FGB   0
#define MONAM_FGR_I 238
#define MONAM_FGG_I 200
#define MONAM_FGB_I 0

// White monitor
#define MONWB_BGR   0
#define MONWB_BGG   0
#define MONWB_BGB   0
#define MONWB_BGR_I 128
#define MONWB_BGG_I 128
#define MONWB_BGB_I 128
#define MONWB_FGR   192
#define MONWB_FGG   192
#define MONWB_FGB   192
#define MONWB_FGR_I 255
#define MONWB_FGG_I 255
#define MONWB_FGB_I 255

// Black monitor
#define MONBW_BGR   255
#define MONBW_BGG   255
#define MONBW_BGB   255
#define MONBW_BGR_I 84
#define MONBW_BGG_I 84
#define MONBW_BGB_I 84
#define MONBW_FGR   0
#define MONBW_FGG   0
#define MONBW_FGB   0
#define MONBW_FGR_I 115
#define MONBW_FGG_I 115
#define MONBW_FGB_I 115

// User monitor (Yellow on Blue)
#define MONUSR_BGR   0
#define MONUSR_BGG   0
#define MONUSR_BGB   168
#define MONUSR_BGR_I 84
#define MONUSR_BGG_I 84
#define MONUSR_BGB_I 252
#define MONUSR_FGR   252
#define MONUSR_FGG   252
#define MONUSR_FGB   84
#define MONUSR_FGR_I 252
#define MONUSR_FGG_I 252
#define MONUSR_FGB_I 150


int vdu_init (void);
int vdu_deinit (void);
int vdu_reset (void);

void vdu_draw_char(SDL_Surface *screen, int x, int y,
                   int maddr,     /* CRTC address of character to draw */
                   uint8_t lines, /* number of lines to draw */
                   uint8_t flashvideo, /* output from the character
                                        * flashing timer */
                   uint8_t cursor, uint8_t cur_start, uint8_t cur_end);
void vdu_redraw_char(int addr);
uint8_t vdu_char_is_redrawn(int addr);
void vdu_char_clear_redraw(int addr);
void vdu_propagate_pcg_updates(int maddr, int size);
void vdu_propagate_flashing_attr(int maddr, int size);

void vdu_write_char_data(int bank, int offset, uint8_t *data, int numbytes);
void vdu_write_pcg_data(int bank, int offset, uint8_t *data, int numbytes);

uint8_t vdu_vidmem_r (uint32_t addr, struct z80_memory_read_byte *mem_s);
void vdu_vidmem_w (uint32_t addr, uint8_t data, struct z80_memory_write_byte *mem_s);

uint16_t vdu_colcont_r (uint16_t port, struct z80_port_read *port_s);
void vdu_colcont_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

uint16_t vdu_colwait_r (uint16_t port, struct z80_port_read *port_s);
void vdu_colwait_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

uint16_t vdu_lvdat_r (uint16_t port, struct z80_port_read *port_s);
void vdu_lvdat_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

void vdu_latchrom_w (uint16_t port, uint8_t data, struct z80_port_write *port_s);

void vdu_set_mon_table (int pos, int col);
void vdu_setcolourtable();
void vdu_configure (int aspect);


typedef struct vdu_t
{
 /*
  * VDU state variables
  */
 uint8_t colour_cont, x_colour_cont;
 uint8_t lv_dat, x_lv_dat;
 int extendram;              /* Alpha+ video enabled */
 int attribram;              /* Attribute RAM selected */
 int colourram;              /* Colour RAM selected if */
 int videobank;              /* currently selected video bank */
 /*
  * Pointers to the regions of attribute and screen memory mapped
  * at 0xf000/0x8000 and PCG and colour memory mapped at
  * 0xf800/0x8800
  */
 uint8_t *scr_ptr;           /* screen RAM */
 uint8_t *atr_ptr;           /* attribute RAM */
 uint8_t *pcg_ptr;           /* PCG RAM */
 uint8_t *col_ptr;           /* colour RAM */
 /*
  * Used internally to track the character positions that need to be redrawn
  */
 uint8_t *redraw_ptr;        /* redraw RAM */
 /*
  * The Alpha+/256TC video hardware supports up to 8k of
  * screen/attribute/colour RAM
  */
 uint16_t scr_mask;          /* screen RAM size mask */

 /*
  * Character/screen/colour/attribute/PCG/redraw arrays
  */
 uint8_t chr_rom[CHR_ROM_SIZE];
 uint8_t scr_ram[SCR_RAM_SIZE];
 uint8_t col_ram[COL_RAM_SIZE];
 uint8_t att_ram[ATT_RAM_SIZE];
 uint8_t pcg_ram[PCG_RAM_SIZE];  // last bank is a dummy bank
 uint8_t redraw[SCR_RAM_SIZE];
 uint8_t pcg_redraw[PCG_RAM_SIZE / 16]; /* FIXME: need a character height constant */
}vdu_t;

#endif /* HEADER_VDU_H */
