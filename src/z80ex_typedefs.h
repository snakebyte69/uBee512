/*
 * Z80~Ex, ZILoG Z80 CPU emulator.
 *
 * by Boo-boo [boo_boo(^at^)inbox.ru]
 * contains code from the FUSE project (http://fuse-emulator.sourceforge.net)
 * released under GNU GPL v2
 *
 */

// uBee512 - SK 2013/06/23
// This is a temporary solution only until the author of z80ex adds some
// extra API functionality to be able to set all callbacks individually.
//
// Copied from z80ex version 1.19 and modified.  Required this to access
// callback members from ubee512 application.  Added compile time code to
// determine Endian format.  Commented out Z80EX_CONTEXT so as to not
// redefine again.
//
// typdefs.h in 1.1.20rev1 appears to have the same structure so this file
// is likely to work with versions 1.19 to 1.1.20rev1.  This will break if
// the structure changes in a later versions or is different in earlier
// versions.
//
// If building using a later version and check the _z80_cpu_context
// structure to see if it has changed.

#ifndef _Z80_TYPEDEFS_H_INCLUDED
#define _Z80_TYPEDEFS_H_INCLUDED

struct _z80_cpu_context;
//SK typedef struct _z80_cpu_context Z80EX_CONTEXT;
//SK #define __Z80EX_SELF_INCLUDE
//SK #include "z80ex.h"

// assume Little Endian if Win32, else if Unix let 'endian.h' decide
#ifdef MINGW
#   define WORDS_LITTLE_ENDIAN
#else
#   include <endian.h>
#   if __BYTE_ORDER == __BIG_ENDIAN
#      define WORDS_BIG_ENDIAN
#   endif
#endif

/* Union allowing a register pair to be accessed as bytes or as a word */
typedef union {
#ifdef WORDS_BIG_ENDIAN
  struct { Z80EX_BYTE h,l; } b;
#else
  struct { Z80EX_BYTE l,h; } b;
#endif
  Z80EX_WORD w;
} regpair;

typedef
enum {IM0=0,IM1=1,IM2=2} IM_MODE;

struct _z80_cpu_context {
	regpair af,bc,de,hl;
	regpair af_,bc_,de_,hl_;
	regpair ix,iy;
	Z80EX_BYTE i;
	Z80EX_WORD r;	
	Z80EX_BYTE r7; /* The high bit of the R register */
	regpair sp,pc;
	Z80EX_BYTE iff1, iff2; /*interrupt flip-flops*/
	regpair memptr; /*undocumented internal register*/
	IM_MODE im;
	int halted;

	unsigned long tstate; /*t-state clock of current/last step*/
	unsigned char op_tstate; /*clean (without WAITs and such) t-state of currently executing instruction*/
	
	int noint_once; /*disable interrupts before next opcode?*/
	int reset_PV_on_int; /*reset P/V flag on interrupt? (for LD A,R / LD A,I)*/
	int doing_opcode; /*is there an opcode currently executing?*/
	char int_vector_req; /*opcode must be fetched from IO device? (int vector read)*/
	Z80EX_BYTE prefix;
	
	/*callbacks*/
	z80ex_tstate_cb tstate_cb;
	void *tstate_cb_user_data;
	z80ex_pread_cb pread_cb;
	void *pread_cb_user_data;
	z80ex_pwrite_cb pwrite_cb;
	void *pwrite_cb_user_data;
	z80ex_mread_cb mread_cb;
	void *mread_cb_user_data;
	z80ex_mwrite_cb	mwrite_cb;
	void *mwrite_cb_user_data;
	z80ex_intread_cb intread_cb;
	void *intread_cb_user_data;
	z80ex_reti_cb reti_cb;
	void *reti_cb_user_data;
	
	/*other stuff*/
	regpair tmpword;
	regpair tmpaddr;
	Z80EX_BYTE tmpbyte;
	Z80EX_SIGNED_BYTE tmpbyte_s;
};

//SK typedef void (*z80ex_opcode_fn) (Z80EX_CONTEXT *cpu);

#endif
