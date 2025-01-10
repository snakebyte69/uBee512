//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                             z80ex API module                               *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Provides an API for z80ex Z80 emulator/disassembler.
//
//==============================================================================
/*
 *  uBee512 - An emulator for the Microbee Z80 ROM, FDD and HDD based models.
 *  Copyright (C) 2007-2016 uBee   
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
//==============================================================================
// ChangeLog (most recent entries are at top)
//==============================================================================
// v5.7.0 - 21 July 2015, uBee
// - Changes to read_mem_cb(), read_mem_debug_cb(), write_mem_cb() and
//   write_mem_debug_cb() to use new define values of MEMMAP_MASK and
//   MEMMAP_SHIFT allowing a finer control of 1k blocks as compared to 4k
//   used before.
// - Implemented new z80ex 1.1.21 API functions z80ex_set_memread_callback()
//   and z80ex_set_memwrite_callback() and removed the z80ex_typedefs.h file
//   work around method.
// - Fixed crash when doing a Power-cycle as z80api_deinit() was not setting
//   z80 context to NULL.
//
// v5.5.0 - 23 June 2013, uBee
// - Added debug versions of read_mem_cb() and write_mem_cb() to reduce the
//   amount of overhead when not debugging so as to keep emulation speed
//   same as in previous version. These are set-up in z80api_set_memhook().
// v5.5.0 - 21 June 2013, B.Robinson
// - Added z80api_set_memhook() function for use with new debug mem break
//   options. Added code to read_mem_cb() and write_mem_cb() for same.
//
// v4.7.0 - 17 June 2010, K Duckmanton
// - Extended the set of actions which can be registered using the
//   z80api_*register_action() functions to include actions to be executed
//   after a set number of z80 tstates are executed.  For now, the actions
//   registered in this way are not executed,
//
// v4.6.0 - 15 May 2010, uBee
// - Added z80api_read_port() and z80api_write_port() API functions.
// - Added z80api_dasm() function to the API and read_byte_cb() function to
//   use z80ex's dasm.
// - Added z80api_set_regs() and z80api_set_pc() functions.
// - Added 'debug.piopoll' test to z80api_execute_complete() function. This
//   allows the extra PIO polling in step mode to be disabled so that it
//   works the same as normal execution.  This might be needed to find
//   problems in the emulator.
// - Added code to write_port_cb() and read_port_cb() functions to save port
//   states to array port_out_state[] and port_inp_state[]
// - Changes to z80api_regdump() to add binary output.
//
// v4.3.0 - 5 August 2009, uBee
// - Added functions z80api_register_action(), z80api_deregister_action() and
//   z80api_call_actions() needed for Dreamdisk model. (workerbee)
// - z80api_nonmaskable_intr() implemented for Dreamdisk model. (workerbee)
// - Changed z80api_maskable_intr() to accumulate tstates in emu.z80_cycles.
//
// v4.2.0 - 17 July 2009, uBee
// - Changes to z80api_execute_complete() to include pio polling calls after
//   every instruction.
//
// v4.1.0 - 27 June 2009, uBee
// - Fixed z80api_execute_complete() function as was not completing on all
//   dd/fd/cb/ed prefixed opcodes.
//
// v4.0.0 - 15 May 2009, uBee
// - Created new z80ex API file.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <z80ex/z80ex.h>
#include <z80ex/z80ex_dasm.h>

#include "z80api.h"
#include "z80.h"
#include "memmap.h"
#include "ubee512.h"
#include "z80debug.h"
#include "pio.h"
#include "support.h"

#define NUM_Z80_ACTIONS 10

static Z80EX_CONTEXT *z80;

static int exec_tstates;
static int poll_want_tstates;
static int poll_want_tstates_def;
static int poll_wait_tstates;
static int poll_repeats;

static int intr_vector;

static int z80_action_count;

static struct
{
 z80_event_t when;
 z80api_action_fn_t function;
}z80actions[NUM_Z80_ACTIONS];

static z80_device_interrupt_t z80_int_scratch;

static z80api_memhook z80_memhook = NULL;

extern char port_out_state[];
extern char port_inp_state[];

extern struct z80_memory_read_byte z80_mem_r[];
extern struct z80_memory_write_byte z80_mem_w[];

extern uint16_t (*z80_ports_r[])(uint16_t, struct z80_port_read *);
extern void (*z80_ports_w[])(uint16_t, uint8_t, struct z80_port_write *);

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;
extern debug_t debug;

Z80EX_BYTE read_mem_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state,
                        void *user_data);
Z80EX_BYTE read_mem_debug_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state,
                        void *user_data);
void write_mem_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value,
                   void *user_data);
void write_mem_debug_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value,
                   void *user_data);

Z80EX_BYTE read_port_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *user_data);
void write_port_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value,
                    void *user_data);
Z80EX_BYTE read_interrupt_vector_cb (Z80EX_CONTEXT *cpu, void *user_data);
Z80EX_BYTE read_byte_cb (Z80EX_WORD addr, void *user_data);

void z80api_reti(Z80EX_CONTEXT *z80, void *data);
void z80api_do_intr(void);
void z80api_do_reti(void);
int z80api_ieo(void);

//==============================================================================
// Z80 Initilization
//
//   pass: void
// return: int                          0
//==============================================================================
int z80api_init (void)
{
 // create and initialise CPU
 if (z80)
    z80api_deinit();

 if (z80_memhook == NULL)
    z80 = z80ex_create(read_mem_cb, NULL, write_mem_cb, NULL,
    read_port_cb, NULL, write_port_cb, NULL, read_interrupt_vector_cb, NULL);
 else
    z80 = z80ex_create(read_mem_debug_cb, NULL, write_mem_debug_cb, NULL,
    read_port_cb, NULL, write_port_cb, NULL, read_interrupt_vector_cb, NULL);

 z80_action_count = 0;
 z80_int_scratch.iei = &z80api_ieo;
 z80_int_scratch.intack = &z80api_do_reti;
 z80ex_set_reti_callback(z80, &z80api_reti, NULL);

 return 0;
}

//==============================================================================
// Z80 De-Initilization
//
//   pass: void
// return: int                          0
//==============================================================================
int z80api_deinit (void)
{
 z80ex_destroy(z80);
 z80 = NULL;
 return 0;
}

//==============================================================================
// Z80 Reset
//
// poll_want_tstates_def needs to be set to a reasonably low value of ~300
// to allow SW serial to work for Microbee Telcom v2.0 and Telcom ROMs pre
// v3.2.1.
//
//   pass: void
// return: int                          0
//==============================================================================
int z80api_reset (void)
{
 z80ex_reset(z80);

 // set the Z80 PC execution address
 z80ex_set_reg(z80, regPC, modelx.bootaddr);

 exec_tstates = 0;

 poll_want_tstates_def = 300;

 poll_want_tstates = poll_want_tstates_def;
 poll_repeats = 0;

 return 0;
}

//==============================================================================
// Get Z80 Program Counter (PC)
//
//   pass: void
// return: int                          PC
//==============================================================================
int z80api_getpc (void)
{
 return z80ex_get_reg(z80, regPC);
}

//==============================================================================
// Set default PIO polling rate.
//
// Set Z80 tstates between between each PIO polling.  This value is used by
// default and when the poll_repeats counter reaches 0.
//
//   pass: int tstates                  tstates between each PIO poll
// return: void
//==============================================================================
void z80api_set_poll_tstates_def (int tstates)
{
 poll_want_tstates_def = tstates;
}

//==============================================================================
// Set PIO polling rate and repeat count.
//
// Set Z80 tstates between between each PIO polling and the number of times
// to repeat before the default tstate count is used.
//
//   pass: int tstates                  tstates between each PIO poll
//         int repeats                  how many times to repeat until reset
// return: void
//==============================================================================
void z80api_set_poll_tstates (int tstates, int repeats)
{
 poll_want_tstates = tstates;
 poll_wait_tstates = 0;
 poll_repeats = repeats;
}

//==============================================================================
// Register an action to occur on a Z80 state change.
//
//   pass: z80_event_t when, pointer to action function
// return: void
//==============================================================================
void z80api_register_action (z80_event_t when, z80api_action_fn_t function)
{
 assert(z80_action_count >= 0 && z80_action_count < NUM_Z80_ACTIONS);
 z80actions[z80_action_count].when = when;
 z80actions[z80_action_count].function = function;
 z80_action_count++;
}

//==============================================================================
// De-register an action to occur on a Z80 state change.
//
//   pass: z80_event_t when, pointer to action function
// return: void
//==============================================================================
void z80api_deregister_action (z80_event_t when, z80api_action_fn_t function)
{
 int i;

 for (i = 0; i < z80_action_count; ++i)
    {
     if (z80actions[i].function == function && z80actions[i].when == when)
        {
         memmove(&z80actions[i], &z80actions[i + 1],
         sizeof(z80actions[0]) * (z80_action_count - i - 1));
         z80_action_count--;
         break;
        }
    }
}

//==============================================================================
// Call actions to occur on a Z80 state change.
//
//   pass: z80_event_t when
// return: void
//==============================================================================
void z80api_call_actions (z80_event_t when)
{
 int i;

 for (i = 0; i < z80_action_count; ++i)
 if (z80actions[i].when == when)
    (*z80actions[i].function)();
}

//==============================================================================
// Execute Z80 tstates.
//
//   pass: int tstates
// return: void
//==============================================================================
void z80api_execute (int tstates)
{
 int ts;
 exec_tstates = 0;

 while (exec_tstates < tstates)
    {
     ts = z80ex_step(z80);
     exec_tstates += ts;

     if (z80ex_doing_halt(z80))
         z80api_call_actions(Z80_HALT);

     // FIXME - can the actions() mechanism be generalised to
     //  encompass this sort of periodic callback?

     poll_wait_tstates -= ts;

     if (poll_wait_tstates < 1)
        {
         pio_polling();
         if (poll_repeats)
            poll_repeats--;
         else
            poll_want_tstates = poll_want_tstates_def;
         poll_wait_tstates = poll_want_tstates;
        }
    }

 emu.z80_cycles += exec_tstates;
 exec_tstates = 0;
}

//==============================================================================
// Execute a single instruction until completed.
//
// Executes dd/fd/cb/ed prefixed opcodes until completed. This function is
// intended for debug stepping. The method employed here is specific to the
// Z80 emulator in use.
//
//   pass: void
// return: void
//==============================================================================
void z80api_execute_complete (void)
{
 z80api_execute(1);
 if (debug.piopoll)
    pio_polling();
 while (z80ex_last_op_type(z80) != 0)
    {
     z80api_execute(1);
     if (debug.piopoll)
        pio_polling();
    }
}

//==============================================================================
// Set the PC register to a new address.
//
// The function first completes any unfinished instruction before changing
// the PC register.
//
// Executes dd/fd/cb/ed prefixed opcodes until completed.
//
//   pass: int addr
// return: void
//==============================================================================
void z80api_set_pc (int addr)
{
 while (z80ex_last_op_type(z80) != 0)
    z80api_execute(1);

 z80ex_set_reg(z80, regPC, addr);
}

//==============================================================================
// Return current Z80 tstate count.
//
//   pass: void
// return: uint64_t
//==============================================================================
uint64_t z80api_get_tstates (void)
{
 return emu.z80_cycles + exec_tstates;
}

//==============================================================================
// Add a peripheral device to the interrupt daisy chain.  The first device to be
// registered has the highest priority, the second device to be registered has
// the next highest priority, and so on.  For this to work, the handler functions
// have to follow the following conventions:
//
// A device must call the (*iei)() function in
// its device_interrupt_t structure and z80api_intr_possible(); if both return
// true the CPU may be interrupted, as no other higher priority device has an
// interrupt pending.
//
// When an interrupt has been serviced, a device must call the
// (*intack)() function in its device_interrupt_t structure
// before doing any processing of its own.
//
//   pass: z80_device_interrupt_t *scratch
//         z80api_status_fn_t ieo
//         z80api_action_fn_t intack
// return: void
//==============================================================================
void z80api_register_interrupting_device (z80_device_interrupt_t *scratch,
                                          z80api_status_fn_t ieo,
                                          z80api_action_fn_t intack)
{
 scratch->iei = z80_int_scratch.iei;
 z80_int_scratch.iei = ieo;
 scratch->intack = z80_int_scratch.intack;
 z80_int_scratch.intack = intack;
};

//==============================================================================
// Z80 Non Maskable interrupt.
//
//   pass: void
// return: void
//==============================================================================
void z80api_nonmaskable_intr (void)
{
 emu.z80_cycles += z80ex_nmi(z80);
}

//==============================================================================
// Z80 Maskable interrupt.
//
//   pass: int vector
// return: void
//==============================================================================
void z80api_maskable_intr (int vector)
{
 if (z80ex_int_possible(z80) == 1)
    {
     intr_vector = vector;
     emu.z80_cycles += z80ex_int(z80);
    }
}

//==============================================================================
// Return 1 if maskable interrupts are possible in current Z80 state.
//
//   pass: void
// return: int
//==============================================================================
int z80api_intr_possible (void)
{
 return z80ex_int_possible(z80);
}

//==============================================================================
// Return true if an interrupt is not being serviced by a device of
// higher priority.
//
//   pass: void
// return: int
//==============================================================================
int z80api_ieo(void)
{
 return 1;
}

//==============================================================================
// The initial RETI callback function
//
//   pass: void
// return: void
//==============================================================================
void z80api_do_reti(void)
{
 return;
}

//==============================================================================
// RETI callback function, called from the Z80EX core
//
//   pass: Z80EX_CONTEXT *z80
//         void *data
// return: z80_action_fn_t
//==============================================================================
void z80api_reti (Z80EX_CONTEXT *z80, void *data)
{
 (*z80_int_scratch.intack)();   /* call callback for the highest
                                 * priority device in the interrupt
                                 * priority chain. */
 return;                        /* then do processing for this
                                 * priority level (none!) */
}

//==============================================================================
// Return all Z80 registers (not called during execution of an instruction)
//
//   pass: z80regs_t *z80regs
// return: void
//==============================================================================
void z80api_get_regs (z80regs_t *z80regs)
{
 z80regs->af = z80ex_get_reg(z80, regAF);
 z80regs->bc = z80ex_get_reg(z80, regBC);
 z80regs->de = z80ex_get_reg(z80, regDE);
 z80regs->hl = z80ex_get_reg(z80, regHL);

 z80regs->af_p = z80ex_get_reg(z80, regAF_);
 z80regs->bc_p = z80ex_get_reg(z80, regBC_);
 z80regs->de_p = z80ex_get_reg(z80, regDE_);
 z80regs->hl_p = z80ex_get_reg(z80, regHL_);

 z80regs->ix = z80ex_get_reg(z80, regIX);
 z80regs->iy = z80ex_get_reg(z80, regIY);
 z80regs->pc = z80ex_get_reg(z80, regPC);
 z80regs->sp = z80ex_get_reg(z80, regSP);

 z80regs->i = z80ex_get_reg(z80, regI);
 z80regs->r = z80ex_get_reg(z80, regR);
}

//==============================================================================
// Set all Z80 registers (used by debug options only)
//
//   pass: z80regs_t *z80regs
// return: void
//==============================================================================
void z80api_set_regs (z80regs_t *z80regs)
{
 z80ex_set_reg(z80, regAF, z80regs->af);
 z80ex_set_reg(z80, regBC, z80regs->bc);
 z80ex_set_reg(z80, regDE, z80regs->de);
 z80ex_set_reg(z80, regHL, z80regs->hl);

 z80ex_set_reg(z80, regAF_, z80regs->af_p);
 z80ex_set_reg(z80, regBC_, z80regs->bc_p);
 z80ex_set_reg(z80, regDE_, z80regs->de_p);
 z80ex_set_reg(z80, regHL_, z80regs->hl_p);

 z80ex_set_reg(z80, regIX, z80regs->ix);
 z80ex_set_reg(z80, regIY, z80regs->iy);
 z80ex_set_reg(z80, regPC, z80regs->pc);
 z80ex_set_reg(z80, regSP, z80regs->sp);

 z80ex_set_reg(z80, regI, z80regs->i);
 z80ex_set_reg(z80, regR, z80regs->r);
}

//==============================================================================
// Return name of Z80 emulator and version.
//
// The format of the string should be 'name version'
//
//   pass: char *vers
//         int size                     size of string
// return: void
//==============================================================================
void z80api_get_version (char *vers, int size)
{
 // the version function is implemented in z80ex-1.1.17.  If compiling
 // against an older version then define Z80EX_NO_VERSION_CODE in the
 // Makefile.

#ifdef Z80EX_NO_VERSION_CODE
 snprintf(vers, size, "z80ex v???");
#else
 Z80EX_VERSION *version = z80ex_get_version();
 snprintf(vers, size, "z80ex %s", version->as_string);
#endif

 vers[size-1] = 0;
}

//==============================================================================
// Z80 register dump
//
// Dump the contents of the Z80 registers.
//
//   pass: void
// return: void
//==============================================================================
void z80api_regdump (void)
{
 z80regs_t z80regs;
 char s[17];

 z80api_get_regs(&z80regs);

 xprintf("\n");
 xprintf("Z80 Reg    Hex     Dec         Binary\n");
 xprintf("------------------------------------------\n");
 xprintf("%-10s %04x %7d %18s\n",
         "AF", z80regs.af, z80regs.af, i2bx(z80regs.af, 16, s));
 xprintf("%-10s %04x %7d %18s\n",
         "BC", z80regs.bc, z80regs.bc, i2bx(z80regs.bc, 16, s));
 xprintf("%-10s %04x %7d %18s\n",
         "DE", z80regs.de, z80regs.de, i2bx(z80regs.de, 16, s));
 xprintf("%-10s %04x %7d %18s\n",
         "HL", z80regs.hl, z80regs.hl, i2bx(z80regs.hl, 16, s));
 xprintf("%-10s %04x %7d %18s\n",
         "AF_p", z80regs.af_p, z80regs.af_p, i2bx(z80regs.af_p, 16, s));
 xprintf("%-10s %04x %7d %18s\n",
         "BC_p", z80regs.bc_p, z80regs.bc_p, i2bx(z80regs.bc_p, 16, s));
 xprintf("%-10s %04x %7d %18s\n",
         "DE_p", z80regs.de_p, z80regs.de_p, i2bx(z80regs.de_p, 16, s));
 xprintf("%-10s %04x %7d %18s\n",
         "HL_p", z80regs.hl_p, z80regs.hl_p, i2bx(z80regs.hl_p, 16, s));
 xprintf("%-10s %04x %7d %18s\n",
         "IX", z80regs.ix, z80regs.ix, i2bx(z80regs.ix, 16, s));
 xprintf("%-10s %04x %7d %18s\n",
         "IY", z80regs.iy, z80regs.iy, i2bx(z80regs.iy, 16, s));
 xprintf("%-10s %04x %7d %18s\n", "SP",
         z80regs.sp, z80regs.sp, i2bx(z80regs.sp, 16, s));
 xprintf("%-10s %04x %7d %18s\n",
         "PC", z80regs.pc, z80regs.pc, i2bx(z80regs.pc, 16, s));
 xprintf("%-10s %02x %9d %18s\n",
         "I", z80regs.i, z80regs.i, i2b(z80regs.i, s));
 xprintf("%-10s %02x %9d %18s\n", "R",
         z80regs.r & 0xff, z80regs.r & 0xff, i2b(z80regs.r & 0xff, s));
 xprintf("\n");
 xprintf("%-10s %02x %9d %18s\n",
         "A", z80regs.af >> 8, z80regs.af >> 8, i2b(z80regs.af >> 8, s));
 xprintf("%-10s %02x %9d %18s\n",
         "F", z80regs.af & 0xff, z80regs.af & 0xff, i2b(z80regs.af & 0xff, s));
 xprintf("%-10s %02x %9d %18s\n",
         "B", z80regs.bc >> 8, z80regs.bc >> 8, i2b(z80regs.bc >> 8, s));
 xprintf("%-10s %02x %9d %18s\n",
         "C", z80regs.bc & 0xff, z80regs.bc & 0xff, i2b(z80regs.bc & 0xff, s));
 xprintf("%-10s %02x %9d %18s\n",
         "D", z80regs.de >> 8, z80regs.de >> 8, i2b(z80regs.de >> 8, s));
 xprintf("%-10s %02x %9d %18s\n",
         "E", z80regs.de & 0xff, z80regs.de & 0xff, i2b(z80regs.de & 0xff, s));
 xprintf("%-10s %02x %9d %18s\n",
         "H", z80regs.hl >> 8, z80regs.hl >> 8, i2b(z80regs.hl >> 8, s));
 xprintf("%-10s %02x %9d %18s\n",
         "L", z80regs.hl & 0xff, z80regs.hl & 0xff, i2b(z80regs.hl & 0xff, s));
}

//==============================================================================
// Read a byte from a Z80 memory location.
//
//   pass: int addr
// return: Z80EX_BYTE
//==============================================================================
Z80EX_BYTE z80api_read_mem (int addr)
{
 return read_mem_cb(NULL, addr, 0, NULL);
}

//==============================================================================
// Write a byte to a Z80 memory location.
//
//   pass: int addr
//         uint8_t value
// return: void
//==============================================================================
void z80api_write_mem (int addr, uint8_t value)
{
 write_mem_cb(NULL, addr, value, NULL);
}

//==============================================================================
// Read a byte from a Z80 port location.
//
//   pass: uint8_t port
// return: Z80EX_BYTE
//==============================================================================
uint8_t z80api_read_port (int port)
{
 return read_port_cb(z80, port, NULL);
}

//==============================================================================
// Write a byte to a Z80 port location.
//
//   pass: int port
//         uint8_t value
// return: void
//==============================================================================
void z80api_write_port (int port, uint8_t value)
{
 write_port_cb(z80, port, value, NULL);
}

//==============================================================================
// Disassemble one z80 instruction.
//
// Return the following values in upper or lower case:
//
// mnemonic  - i.e 'LD'
// argument  - i.e 'A,12'
// t_states  - T-states of the instruction
// t_states2 - branching commands: T-states when PC is changed, else 0
//
//   pass: int addr                     address of z80 code
//         int lower                    non 0 if lower case required
//         char *mnemonic
//         char *argument
//         int *t_states
//         int *t_states2
// return: int                          number of bytes for instruction
//==============================================================================
int z80api_dasm (int addr, int lower, char *mnemonic, char *argument,
                 int *t_states, int *t_states2)
{
 int count;
 int t;
 int t2;
 int si;
 int di;
 char dis[80];

 count = z80ex_dasm(dis, sizeof(dis)-1, 0, &t, &t2, read_byte_cb, addr, NULL);

 si = 0;
 di = 0;
 while ((dis[si]) && (dis[si] <= ' '))
    si++;
 while ((dis[si]) && (dis[si] > ' '))
    {
     if (lower)
        mnemonic[di] = tolower(dis[si]);
     else
        mnemonic[di] = dis[si];
     si++;
     di++;
    }
 mnemonic[di] = 0;

 di = 0;
 while ((dis[si]) && (dis[si] <= ' '))
    si++;
 while ((dis[si]) && (dis[si] > ' '))
    {
     if (lower)
        argument[di] = tolower(dis[si]);
     else
        argument[di] = dis[si];
     si++;
     di++;
    }
 argument[di] = 0;

 *t_states = t;
 *t_states2 = t2;

 return count;
}

//==============================================================================
// Z80ex Read byte call back.
//
//   pass: Z80EX_WORD addr
//         void *user_data
// return: Z80EX_BYTE
//==============================================================================
Z80EX_BYTE read_byte_cb (Z80EX_WORD addr, void *user_data)
{
 return read_mem_cb(NULL, addr, 0, NULL);
}

//==============================================================================
// Z80ex Read memory call back.
//
// To keep the code here at a minimum a separate function is used when
// using ubee512's debug mode.
//
//   pass: Z80EX_CONTEXT *cpu
//         Z80EX_WORD addr
//         int m1_state
//         void *user_data
// return: Z80EX_BYTE
//==============================================================================
Z80EX_BYTE read_mem_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state,
                        void *user_data)
{
#ifdef MEMMAP_HANDLER_1
 return (Z80EX_BYTE)z80_mem_r[(addr & MEMMAP_MASK) >> 
 MEMMAP_SHIFT].memory_call(addr, NULL);
#else
 int i;

 for (i=0;;i++)
    {
     if (((int)addr >= z80_mem_r[i].low_addr) &&
        ((int)addr <= z80_mem_r[i].high_addr))
        return (Z80EX_BYTE)z80_mem_r[i].memory_call(addr, NULL);
    }
#endif
}

//==============================================================================
// Z80ex Read memory debug call back.
//
//   pass: Z80EX_CONTEXT *cpu
//         Z80EX_WORD addr
//         int m1_state
//         void *user_data
// return: Z80EX_BYTE
//==============================================================================
Z80EX_BYTE read_mem_debug_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state,
                              void *user_data)
{
 // call the debug memory hook
 z80_memhook(addr, 0);

#ifdef MEMMAP_HANDLER_1
 return (Z80EX_BYTE)z80_mem_r[(addr & MEMMAP_MASK) >>
 MEMMAP_SHIFT].memory_call(addr, NULL);
#else
 int i;

 for (i=0;;i++)
    {
     if (((int)addr >= z80_mem_r[i].low_addr) &&
        ((int)addr <= z80_mem_r[i].high_addr))
        return (Z80EX_BYTE)z80_mem_r[i].memory_call(addr, NULL);
    }
#endif
}

//==============================================================================
// Z80ex Write memory call back.
//
// To keep the code here at a minimum a separate function is used when
// using ubee512's debug mode.
//
//   pass: Z80EX_CONTEXT *cpu
//         Z80EX_WORD addr
//         Z80EX_BYTE value
//         void *user_data
// return: void
//==============================================================================
void write_mem_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value,
                   void *user_data)
{
#ifdef MEMMAP_HANDLER_1
 z80_mem_w[(addr & MEMMAP_MASK) >> MEMMAP_SHIFT].memory_call(addr, value, NULL);
#else
 int i;

 for (i=0;;i++)
    {
     if (((int)addr >= z80_mem_w[i].low_addr) &&
        ((int)addr <= z80_mem_w[i].high_addr))
        {
         z80_mem_w[i].memory_call(addr, value, NULL);
         return;
        }
    }
#endif
}

//==============================================================================
// Z80ex Write memory debug call back.
//
// To keep the code here at a minimum a separate function is used when
// debugging.
//
//   pass: Z80EX_CONTEXT *cpu
//         Z80EX_WORD addr
//         Z80EX_BYTE value
//         void *user_data
// return: void
//==============================================================================
void write_mem_debug_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value,
                         void *user_data)
{
#ifdef MEMMAP_HANDLER_1
 z80_mem_w[(addr & MEMMAP_MASK) >> MEMMAP_SHIFT].memory_call(addr, value, NULL);
#else
 int i;

 for (i=0;;i++)
    {
     if (((int)addr >= z80_mem_w[i].low_addr) &&
        ((int)addr <= z80_mem_w[i].high_addr))
        {
         z80_mem_w[i].memory_call(addr, value, NULL);
         return;
        }
    }
#endif

 // call the debug memory hook
 z80_memhook(addr, 1);
}

//==============================================================================
// Z80ex Read port call back.
//
//   pass: Z80EX_CONTEXT *cpu
//         Z80EX_WORD port
//         void *user_data
// return: Z80EX_BYTE
//==============================================================================
Z80EX_BYTE read_port_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *user_data)
{
 return (port_inp_state[port & 0x00ff]=z80_ports_r[port & 0x00ff](port, NULL));
}

//==============================================================================
// Z80ex Write port call back.
//
//   pass: Z80EX_CONTEXT *cpu
//         Z80EX_WORD port
//         Z80EX_BYTE value
//         void *user_data
// return: void
//==============================================================================
void write_port_cb (Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value,
                    void *user_data)
{
 port_out_state[port & 0x00ff] = value;
 z80_ports_w[port & 0x00ff](port, value, NULL);
}

//==============================================================================
// Z80ex Read interrupt vector call back.
//
//   pass: Z80EX_CONTEXT *cpu
// return: Z80EX_BYTE
//==============================================================================
Z80EX_BYTE read_interrupt_vector_cb (Z80EX_CONTEXT *cpu, void *user_data)
{
 return (Z80EX_BYTE)intr_vector;
}

//==============================================================================
// Z80ex Set memory read/write hook
//
//   pass: z80api_memhook hook          Callback function to be called on all
//                                      z80 memory read/write operations
// return: void
//==============================================================================
void z80api_set_memhook (z80api_memhook hook)
{
 // if the Z80 context has not been created yet create it (cmd line options!)
 if (z80 == NULL)
    z80api_init();
    
 z80_memhook = hook;
 
 // if memory hook is set then use the debug versions of read_mem_cb() and
 // write_mem_cb()

 if (z80_memhook != NULL)
    {
     z80ex_set_memread_callback(z80, read_mem_debug_cb, NULL);
     z80ex_set_memwrite_callback(z80, write_mem_debug_cb, NULL);
    }
 else
    {
     z80ex_set_memread_callback(z80, read_mem_cb, NULL);
     z80ex_set_memwrite_callback(z80, write_mem_cb, NULL);
    }
}
