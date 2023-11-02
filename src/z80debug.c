//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                           Z80 debugging module                             *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// Provides debugging tools to aid in the development of the emulator and
// for debugging Z80 code generally.
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
// v6.0.0 - 1 January 2017, K Duckmanton
// - Microbee memory is now an array of uint8_t rather than char.
//
// v5.7.0 - 23 November 2013, uBee
// - Added optional reporting of Z80 register contents.
// - Added optional reporting of memory contents pointed to by 16 bit registers.
// - Fixed --bpr, --db-bpr options as had failed to take a single argument.
// - Fixed --db-bp-port, -db-bpclr-port and --db-bpr-port 'p,n' range values.
//
// v5.5.0 - 29 June 2013, uBee
// - Added z80debug_trace() for --db-trace option.
// - Added z80debug_pc_breakpoint_set() for --db-bp to now set 1 or more PC
//   break points in the one option.
// - Added z80debug_pc_breakpoint_setr() for --db-bpr to now set 1 or more PC
//   break points in the one option.
// - Added z80debug_pc_breakpoints_clear() for --db-bpclr option to now
//   clear 1 or all PC break points.
// - Added z80debug_pc_breakpoints_os() for --db-bpos option.
// - Added z80debug_pc_breakpoints() and moved existing single address BPs
//   to it and added the new outside range BP detection code.
// - Added z80debug_conditional_tracing() to conditionally disable tracing
//   for the new --db-trace option.
// v5.5.0 - 21 June 2013, B.Robinson
// - Added memory break point support.
// - Added debugger step-over command to step over function calls.
// - Added ability to run code from debugger without returning to emulation
//   window.
// - Consolidated debug.mode and debug.debug and centralized debugger mode
//   management.
// - Added Debugger step-out command to --db-step=x.
// - Added z80debug_memhook() for the memory break functionality.
// - Added z80debug_does_cc_match() for the memory break functionality.
// - Added z80debug_set_mode().
// - Added z80debug_bp_mem() for setting of memory break functionality.
// - Added z80debug_print_console_prompt().
// - Moved dasm code from z80debug_before() into a new function
//   z80debug_prepare_dasm().
// - Moved dasm code from z80debug_after() into new function
//   z80debug_print_dasm() with changes.
// - Added display of PC to alternate registers in show_registers().
// 
// v5.3.0 - 4 April 2011, uBee
// - Added modio.tapfile to z80debug_proc_modio_args()
//
// v5.2.0 - 22 March 2011, uBee
// - Fixed z80debug_dump_lines() to output ASCII characters if value is
//   32-126 instead of using isalpha() and isdigit().
// v5.2.0 - 5 February 2011, K Duckmanton
// - Added modio.compumuse to z80debug_proc_modio_args()
//
// v5.0.0 - 4 August 2010, uBee
// - Added 'dac' argument to z80debug_proc_modio_args().
//
// v4.7.0 - 29 June 2010, uBee
// - Changes made to fread() function to use the result as some compilers
//   report warning: declared with attribute warn_unused_result.
// - Added beetalker and beethoven arguments to z80debug_proc_modio_args().
//
// v4.6.0 - 29 May 2010, uBee
// - Added the EMU_CMD_DASML command to z80debug_command().
// - Removed the z80debug disassembler code.  This has now been replaced by
//   the z80ex_dasm library code.
// - Added option to z80debug_command() to enable/disable messages.
// - Added z80debug_bp_port() function for --db-bp*-port options.
// - Added z80debug_dasm() function for --db-dasm and --db-dasml options.
// - Added z80debug_dump_bank() function for --db-dumpb and --db-dumplb
//   options.
// - Added z80debug_dump_memory() function for --db-dump and --db-dumpl
//   options.
// - Added z80debug_dump_ports() function for --db-dumpp option.
// - Added z80debug_dump_registers() function for --db-regs option.
// - Added z80debug_fill_bank() function for --db-fillb option.
// - Added z80debug_fill_memory() function for --db-fillm option.
// - Added z80debug_find_bank() function for --db-findb option.
// - Added z80debug_find_memory() function for --db-findm option.
// - Added z80debug_load_bank() function for --db-loadb option.
// - Added z80debug_load_memory() function for --db-loadm option.
// - Added z80debug_move_memory() function for --db-move option.
// - Added z80debug_pop_mem() function for --db-popm option.
// - Added z80debug_pop_regs() function for --db-popr option.
// - Added z80debug_port_read() function for --db-portr option.
// - Added z80debug_port_write() function for --db-portw option.
// - Added z80debug_push_mem() function for --db-pushm option.
// - Added z80debug_push_regs() function for --db-pushr option.
// - Added z80debug_save_bank() function for --db-saveb option.
// - Added z80debug_save_memory() function for --db-savem option.
// - Added z80debug_set_bank() function for --db-setb option.
// - Added z80debug_set_reg() function for --db-setr option.
// - Added z80debug_set_memory() function for --db-setm option.
// - Added z80debug_step() function for --db-step option.
// - Added z80debug_proc_debug_args() function for --debug option.
// - Added z80debug_proc_modio_args() function for --modio option.
// - Added z80debug_proc_regdump_args() function for --regs option.
// - z80debug_dump() function call from z80debug_command() now uses a new
//   debug.dump_lines variable settable with a --dump-lines option and a
//   debug.dump_header value settable with --dump-header.
// - Added z80debug_capture(), console_debug_file_create() and
//   console_debug_file_close() functions for debug capturing.
// - Added RST and port read/write break points to z80debug_before() and
//   z80debug_after() functions.
// - Added 'piopoll' member to the debug structure.
// - Renamed z80debug_dump() function to z80debug_dump_lines() and added a
//   source pointer to the parameters.
// - Changed z80debug_dump_lines() function to output a new line when header
//   is requested and removed new lines preceeding calls to this.
// - Changed z80debug_dump_lines() parameter htype to flags to give some
//   other flexibility in the way data is output.
// - Removed the time_delay_ms(1) call and other code for key testing from
//   the z80debug_before() function when in paused mode as this is handled
//   in a better way in ubee512.c now.
//
// v4.2.0 - 19 July 2009, uBee
// - Added pio_regdump() to z80debug_command() function.
// - Changed debug.break_point to use one-time or repeated break points.
//
// v4.0.0 - 8 June 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Replaced all z80mem accesses with z80api_read_mem() function.
// - Fixed incorrect disassembly of IX and IY values by reloading op_1 and
//   op_2 in the z80debug_dis_Op1andC0_xC0_e_x05() function.  This bug was
//   likely to have been introduced when the original code was restructured.
// - Changed z80debug_before() to return 1 instead of 0 when step needed.
//
// v3.1.0 - 31 January 2009, uBee
// - Moved commands over from function.c to new z80debug_command() function.
// - Removed debug.debugcount = -1 from z80debug_init(), this and functions
//   z80debug_deinit() and z80debug_reset() were not even being called.
// - Changed all printf() calls to use a local xprintf() function.
// - Changed all putchar() to use local xprintf() function.
//
// v3.0.0 - 6 October 2008, uBee
// - SDL_Delay() replaced by time_delay_ms() function.
//
// v2.8.0 - 27 August 2008, uBee
// - Added code for pause mode, this uses methods similar to debug.
// - Added SDL_Delay() functions to step mode to reduce host CPU time.
//
// v2.7.0 - 22 May 2008, uBee
// - Added structure emu_t and moved variables into it.
//
// v2.3.0 - 23 January 2008, uBee
// - Created a new file to implement the Z80 debugging tools.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "ubee512.h"
#include "z80debug.h"
#include "z80api.h"
#include "z80.h"
#include "pio.h"
#include "crtc.h"
#include "rtc.h"
#include "support.h"
#include "memmap.h"
#include "vdu.h"
#include "console.h"
#include "gui.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
debug_t debug =
{
 .show = Z80DEBUG_TSTATE | Z80DEBUG_REGS | Z80DEBUG_MEMR,
 .cond_trace_addr_s = -1,
 .pc_bp_os_addr_s = -1,
 .piopoll = 1,
 .dasm_addr = 0,
 .dasm_lines = 1,
 .dump_addr = 0,
 .dump_header = 1,
 .dump_lines = 8,
 .find_count = 20,
};

 char *bank_args[] =
 {
  "scr",
  "col",
  "att",
  "pcg",
  "mem",
  "vid",
  ""
 };

 char *bank2_args[] =
 {
  "scr",
  "col",
  "att",
  "pcg",
  "mem",
  ""
 };

 char *header_args[] =
 {
  "-h",
  "+h",
  ""
 };

 char *direction_args[] =
 {
  "i",
  "o",
  ""
 };

 char *direction_rw_args[] =
 {
  "r",
  "w",
  ""
 };

#define IS_OPCODE_CALL(opcode) (opcode == 0xcd || (opcode & 0xc7)==0xc7)
#define IS_OPCODE_RET(opcode) (opcode == 0xc9 || (opcode & 0xc7)==0xc0)

static char cmds[100];

static int port_out_bp_value[256];
static int port_inp_bp_value[256];

static z80regs_t z80before;

static int dump_addr_x;
static int check_port = -1;

static z80regs_t z80_pushed_regs;
static char *pushed_mem;

static char xmnemonic[30];
static char xargument[30];
static char xtstates[30];
static int xt_states;
static int xt_states2;

static int dasm_addr;
static int dump_addr;

typedef char *str_p;

static int z80pc_before;
static int z80pc_same;

static int z80_step_over_stop_address;
static int z80_call_depth;

extern uint8_t *const block_ptrs[];
extern uint8_t port_out_state[];
extern uint8_t port_inp_state[];

extern emu_t emu;
extern model_t modelx;
extern regdump_t regdump;
extern modio_t modio;
extern vdu_t vdu;
extern console_t console;

//==============================================================================
// z80debug initialise.
//
//   pass: void
// return: int                          0
//==============================================================================
int z80debug_init (void)
{
 dump_addr_x = debug.dump_addr;
 return 0;
}

//==============================================================================
// z80debug de-initialise.
//
//   pass: void
// return: int                          0
//==============================================================================
int z80debug_deinit (void)
{
 // close any debug file that may be open
 z80debug_debug_file_close();

 return 0;
}

//==============================================================================
// z80debug reset.
//
//   pass: void
// return: int                          0
//==============================================================================
int z80debug_reset (void)
{
 z80pc_same = 0;
 return 0;
}

//==============================================================================
// Debugging capture file.
//
// Possible actions:
//
// 0 : disable capture.
// 1 : enable capture (time stamp).
// 2 : disable capture if last action was not 1.
// 3 : enable capture if last action was not 1. (no overheads).
//
//   pass: int action                   capture action
//         char *option                 only if action=1, else pass NULL
//         char *optarg                 only if action=1, else pass NULL
// return: void
//==============================================================================
void z80debug_capture (int action, char *option, char *optarg)
{
 char s[100];

 if (! console.debug)
    return;

 switch (action)
    {
     case 0 : // disable capture
        console_debug_stream(0);
        debug.capture_state = action;
        break;
     case 1 : // enable capture (time stamp)
        get_date_and_time(s);
        console_debug_stream(1);
        console.debug_only = 1;
        xprintf("\n***** %s\n", s);
        if (optarg)
           xprintf("--%s %s\n", option, optarg);
        else
           xprintf("--%s\n", option);
        console.debug_only = 0;
        debug.capture_state = action;
        strcpy(debug.last_option, option);
        strcpy(cmds, option);
        break;
     case 2 : // disable capture if last action was not 1.
        if (debug.capture_state != 1)
           console_debug_stream(0);
        break;
     case 3 : // enable capture if last action was not 1. (no overheads)
        if (debug.capture_state != 1)
           console_debug_stream(1);
        if (strcmp(debug.last_option, option) != 0)
           {
            get_date_and_time(s);
            console.debug_only = 1;
            xprintf("\n***** %s\n", s);
            xprintf("%s\n", option);
            console.debug_only = 0;
            strcpy(debug.last_option, option);
           }
        break;
    }
}

//==============================================================================
// Close debug capture file.
//
//   pass: void
// return: void
//==============================================================================
void z80debug_debug_file_close (void)
{
 // close any debug file that may already be open
 if (console.debug)
    {
     fclose(console.debug);
     console.debug = NULL;
    }
}

//==============================================================================
// Create debug capture file.
//
// The debug capture file is used by the debugging group of options.  It
// provides a way to capture all output to a file when the xprintf() and
// xputchar() functions are called and the CONSOLE_DEBUG stream is active.
//
//   pass: char *fn                     debug logging file name
// return: int                          0 if no error, else -1
//==============================================================================
int z80debug_debug_file_create (char *fn)
{
 // force a new date/time stamp when used again
 debug.last_option[0] = 0;

 // close any debug file that may be open
 z80debug_debug_file_close();

 console.debug = fopen(fn, "wb");
 if (console.debug == NULL)
    return -1;

 return 0;
}

//==============================================================================
// Create an array to be used for searching purposes.
//
// The function will recognise the following argument values with each one
// separated by a ',':
//
// a     : Following value is ASCII (next only).
// b     : Following values are bytes (default).
// c     : As for 'a' but matches any case for everything! Avoid searching
//         for integer values in the same search if using this.
// w     : Following values are words.
// byte  : Byte value.
// word  : Word value.
// ASCII : ASCII characters.
//
//   pass: char *c              current parameter pointer
//         uint_8 *search       array of values to search for
//         int *any_case        gets set to 1 if 'c' is specified
// return: int                  search size if no error else -1
//==============================================================================
static int z80debug_create_search_array (char *c, uint8_t *search, int *any_case)
{
 char *find_args[] =
 {
  "a",
  "b",
  "c",
  "w",
  ""
 };

 char sp[512];

 int value;
 int l;
 int x;
 int ins_pos = 0;
 int max_value = 0xff;
 int ins_size = 1;
 int ins_type = 0;

 while (c != NULL)
    {
     c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1);

     if (sp[0] == 0)
        break;

     switch (ins_type)
        {
         case 1 :
            ins_type = 0;
            l = strlen(sp);
            if ((ins_pos + l) >= Z80DEBUG_SEARCH_SIZE)
               return -1;
            memcpy(search+ins_pos, sp, l);
            ins_pos += l;
            break;
         case 0 :
            x = string_search(find_args, sp);
            if (x != -1)
               {
                switch (x)
                   {
                    case 0 : // a - ASCII input
                       ins_type = 1;
                       break;
                    case 1 : // b - byte size
                       ins_size = 1;
                       max_value = 0xff;
                       break;
                    case 2 : // c - ASCII input, any case matches
                       ins_type = 1;
                       *any_case = 1;
                       break;
                    case 3 : // w - word size
                       ins_size = 2;
                       max_value = 0xffff;
                       break;
                   }
               }
            else
               {
                // must be an integer value if reached this point
                if ((value < 0) || (value > max_value))
                   return -1;

                // make sure the byte/word will fit into the search array
                if ((ins_pos + ins_size) >= Z80DEBUG_SEARCH_SIZE)
                   return -1;

                // if it's a word get the MSB
                if (ins_size == 2)
                   search[ins_pos++] = (value & 0xff00) >> 8;

                // get the LSB (or byte)
                search[ins_pos++] = value & 0x00ff;
               }
            break;
        }
    }

#if 0
 printf("search size=%d\n", ins_pos);
 z80debug_dump_lines(search, 0, 8, Z80DEBUG_DUMP_HEAD);
#endif

 return ins_pos;
}

//==============================================================================
// Show Z80 registers.
//
//   pass: z80regs_t *z80x
//         int show                     show flags
//         int indent                   indenting value for Alternate, I, R
// return: void
//==============================================================================
static void show_registers (z80regs_t *z80x, int show, int indent)
{
 int SP_C;
 char flags[9];
 char format[20];

 // show the standard Z80 register set
 if (show & Z80DEBUG_REGS)
    {
     flags[0] = z80x->af & B8(00000001)?'C':'-';
     flags[1] = z80x->af & B8(00000010)?'N':'-';
     flags[2] = z80x->af & B8(00000100)?'P':'-';
     flags[3] = z80x->af & B8(00001000)?'-':'-';
     flags[4] = z80x->af & B8(00010000)?'A':'-';
     flags[5] = z80x->af & B8(00100000)?'-':'-';
     flags[6] = z80x->af & B8(01000000)?'Z':'-';
     flags[7] = z80x->af & B8(10000000)?'S':'-';
     flags[8] = 0;
     xprintf("FLG: %s AF:%04x BC:%04x DE:%04x HL:%04x",
     flags, z80x->af, z80x->bc, z80x->de, z80x->hl);
    }

 // show memory contents pointed to by BC, DE, HL, IX and IY registers
 if (show & Z80DEBUG_MEMR)
    {
     if (strstr(xargument, "(bc"))
        xprintf(" BC(%02x)", z80api_read_mem(z80x->bc));
     else if (strstr(xargument, "(de"))
        xprintf(" DE(%02x)", z80api_read_mem(z80x->de));
     else if (strstr(xargument, "(hl"))
        xprintf(" HL(%02x)", z80api_read_mem(z80x->hl));
     else if (strstr(xargument, "(ix"))
        xprintf(" IX(%02x)", z80api_read_mem(z80x->ix));
     else if (strstr(xargument, "(iy"))
        xprintf(" IY(%02x)", z80api_read_mem(z80x->iy));
    }

 // show the index and SP registers
 if (show & Z80DEBUG_INDEX)
    {                                            // fetch the previous contents
     SP_C = (z80api_read_mem(z80x->sp+1) << 8) | z80api_read_mem(z80x->sp);
     xprintf(" IX:%04x IY:%04x SP:%04x (%04x)\n",
     z80x->ix, z80x->iy, z80x->sp, SP_C);
    }
 else
    xprintf("\n");

 // show the alternate, I and R registers
 if (show & Z80DEBUG_ALTREG)
    {
     if (show & Z80DEBUG_COUNT)
        indent += 9;
     if (show & Z80DEBUG_TSTATE)
        indent += 7;
     sprintf(format, "%s%d%s", "%", indent, "s");
     xprintf(format, "Alternate ");
     xprintf("AF:%04x BC:%04x DE:%04x HL:%04x I:%02x R:%02x PC:%04x\n",
     z80x->af_p, z80x->bc_p, z80x->de_p, z80x->hl_p, z80x->i, z80x->r,
     z80x->pc);
    }
}

//==============================================================================
// Prepare to print the disassembly of an instruction by disassembling it.
//
// Called from z80debug_before when instruction tracing is enabled
//
//   pass: void
// return: void
//==============================================================================
void z80debug_prepare_dasm (void)
{
 z80api_dasm(z80before.pc, 1, xmnemonic, xargument, &xt_states, &xt_states2);
 if (xt_states2)
    snprintf(xtstates, sizeof(xtstates), "t%d/%d", xt_states, xt_states2);
 else
    snprintf(xtstates, sizeof(xtstates), "t%d", xt_states);
}

//==============================================================================
// Print the previously prepared disassembly of the last executed instruction 
// and the resulting (current) register values.
//
// Called from z80debug_after when instruction tracing is enabled, or when a 
// memory or a port breakpoint is triggered.
//
//   pass: int prepare          force disassembly of the instruction
// return: 1                    if the instruction was printed
//==============================================================================
int z80debug_print_dasm (int prepare)
{
 // If the instruction wasn't prepared, prepare it now if requested
 if (xmnemonic[0] == '\0' && prepare)
    z80debug_prepare_dasm();

 // Only show the disassembly if it's been prepared
 if (xmnemonic[0] != '\0')
    {
     z80regs_t regs;
     z80api_get_regs(&regs);

     z80debug_capture(3, cmds, NULL);

     // show the instruction counter if enabled
     if (debug.show & Z80DEBUG_COUNT)
        xprintf("%08x ", debug.debug_count);

     xprintf("%04x: %-8s%-12s", z80before.pc, xmnemonic, xargument);
     
     if (debug.show & Z80DEBUG_TSTATE)
        xprintf("%-7s", xtstates);

     show_registers(&regs, debug.show, 40);

     z80debug_capture(2, NULL, NULL);

     return 1;
    }

 return 0;
}

//==============================================================================
// Check memory read/write breakpoints
//
// Called by z80 for all memory reads/writes (only when the hook is installed)
//
//   pass: uint32_t addr        address being read/write
//         int is_write         non-zero if this is a write operation
// return: void
//==============================================================================
void z80debug_memhook (uint32_t addr, int is_write)
{
 // NB: We don't need to check for debug mode, since this hook is only
 // installed when in debug mode.
 if (debug.break_point[addr] &
    (is_write ? Z80DEBUG_BP_MEMW_FLAG : Z80DEBUG_BP_MEMR_FLAG))
    {
     // Memory break point has been hit.
     if (debug.memory_break_point_type == 0)
        {
         // only record the first hit for LDIR
         debug.memory_break_point_addr = addr;
         debug.memory_break_point_type =
         (is_write ? Z80DEBUG_BP_MEMW_FLAG : Z80DEBUG_BP_MEMR_FLAG);
        }
    }
}

//==============================================================================
// Setup the current mode of the debugger
//
//   pass: int mode             new Z80DEBUG_MODE_xxx setting
// return: void
//==============================================================================
void z80debug_set_mode (int mode)
{
 // Redundant? (B.Robinson)
 if (debug.mode == mode)
   return;

 // Store new mode
 debug.mode = mode;

 // Install hook
 z80api_set_memhook(mode == Z80DEBUG_MODE_OFF ? NULL : z80debug_memhook);

 // Clear various breakpoint related variables
 z80_step_over_stop_address = -1;
 z80_call_depth = -1;

 // Handle modes
 switch (mode)
    {
     case Z80DEBUG_MODE_OFF :
        emu.z80_blocks = 0; // restart normal execution
        break;

     case Z80DEBUG_MODE_RUN :
        break;

     case Z80DEBUG_MODE_TRACE :
        break;
     
     case Z80DEBUG_MODE_STOP :
        {
         console_resume_after_debugger_run();
         debug.step = 0;
         break;
        }

     case Z80DEBUG_MODE_STEP_QUIET :
     case Z80DEBUG_MODE_STEP_VERBOSE :
        // Default to 1, called to update to >1 for multiple steps
        debug.step = 1;
        break;
    }

 // Update title bar to show new debug mode
 gui_status_update();
}

//==============================================================================
// Compare conditional RET or CALL opcode to flags register
//
// Used by step out functionality to see if a CALL or RET will be followed
//
//   pass: int opcode      The opcode of a "CALL", "RET", "CALL cc" or "RET cc"
//                         instruction.
//         int f
// return: 1               if instruction will be followed.
//==============================================================================
int z80debug_does_cc_match (int opcode, int f)
{
 if (opcode & 0x01)  // Non conditional CALL or RET
    return 1;

 switch ((opcode & 0x38) >> 3)
    {
     case 0: return (f & B8(01000000)) == 0;      // NZ
     case 1: return (f & B8(01000000)) != 0;      // Z
     case 2: return (f & B8(00000001)) == 0;      // NC
     case 3: return (f & B8(00000001)) != 0;      // C
     case 4: return (f & B8(00000010)) == 0;      // PO
     case 5: return (f & B8(00000010)) != 0;      // PE
     case 6: return (f & B8(10000000)) == 0;      // P
     case 7: return (f & B8(10000000)) != 0;      // M
    }

 return 0;
}

//==============================================================================
// Test if tracing should be conditionally disabled.
//
// If conditional tracing based on PC start/finish address is in effect then
// tracing is disabled if outside the address range.  First time it is
// detected as off it is reported.
//
//   pass: void
// return: int                  1 if tracing, else 0
//==============================================================================
int z80debug_conditional_tracing (void)
{
 // conditional disassembly between 2 addressess
 if (debug.mode != Z80DEBUG_MODE_TRACE)
    return 1;
   
 // tracing is on if no range condition has been set
 if (debug.cond_trace_addr_s == -1)
    return 1;
 
 if (z80before.pc >= debug.cond_trace_addr_s &&
 z80before.pc <= debug.cond_trace_addr_f)
    {
     debug.cond_trace_flag = 1;
     return 1;
    }

 // only report "TRACE is off" once
 if (! debug.cond_trace_flag)
    return 0;
 
 debug.cond_trace_flag = 0;
 xprintf("\nTRACE Dissasembly off: PC: %04x COUNT: %08x\n\n",
 z80before.pc, debug.debug_count);
 z80debug_capture(2, NULL, NULL);

 return 0;
}

//==============================================================================
// Test if individual PC break points are detected or if a PC break point
// outside of a range of start and finish addresses.
//
//   pass: void
// return: int                  1 if break point, else 0
//==============================================================================
int z80debug_pc_breakpoints (void)
{
 // check for program counter (PC) break points
 if (debug.break_point[z80before.pc] & (Z80DEBUG_BP_FLAG | Z80DEBUG_BPR_FLAG))
    {
     debug.break_point[z80before.pc] &= ~Z80DEBUG_BP_FLAG;
     return 1;
    }

 // Test if a PC break point outside a range of start and finish addresses.

 // exit if no PC outside range break points have been set
 if (debug.pc_bp_os_addr_s == -1)
    return 0;
 
 if (z80before.pc < debug.pc_bp_os_addr_s ||
 z80before.pc > debug.pc_bp_os_addr_f)
    {
     // only generate the break point once when leaving range
     if (debug.pc_bp_os_flag)
        return 0;    
   
     debug.pc_bp_os_flag = 1;
     return 1;
    }

 // PC is within range so no break point is generated
 debug.pc_bp_os_flag = 0;
 return 0;
}

//==============================================================================
// z80debug before instruction execution.
//
// Check for debugging break points.
//
// Disassemble an instruction at the current Z80 PC.  This is placed into a
// string and is output in the z80debug_after() function.
//
//   pass: void
// return: int                          1 to step code on return, else -1
//==============================================================================
int z80debug_before (void)
{
 int bp = 0;
 int disassemble = 0;
 int i = 0;
 int opcode = 0;
 int rst;

 // clear the memory break point flag
 debug.memory_break_point_type = 0;

 // if stopped then return (no code execution)
 if (debug.mode == Z80DEBUG_MODE_STOP)
    {
     if (emu.quit || emu.reset)
        emu.z80_blocks = 0;     // kill the current block loop
     else
        time_delay_ms(1);  // prevent excessive host CPU time whilst
                           // in step mode
     return -1;
    }

 // if exit might be +1 we must save the current z80 register values
 z80api_get_regs(&z80before);

 // test and do the disassembly only if needed
 if (debug.mode == Z80DEBUG_MODE_TRACE ||
 debug.mode == Z80DEBUG_MODE_STEP_VERBOSE)
    disassemble = 1;

 // check for code break points in code execution
 if ((debug.mode != Z80DEBUG_MODE_STEP_QUIET &&
 debug.mode != Z80DEBUG_MODE_STEP_VERBOSE) || debug.step > 1) // if not step mode
    {
     z80debug_capture(3, cmds, NULL);

     opcode = z80api_read_mem(z80before.pc);

     // Check if we've reached the step over stop address
     if (z80before.pc == z80_step_over_stop_address)
        bp = 1;

     // Has the current step out operation finished?  (ie: previous instruction
     // was the RET that clear the call depth back to 0)
     if (z80_call_depth == 0)
        {
         z80_call_depth = -1;
         bp = 1;
        }

     // Check for CALL and RETURN statements and count the call depth
     if (z80_call_depth >= 1)
        {
         if (IS_OPCODE_CALL(opcode) &&
            z80debug_does_cc_match(opcode, z80before.af))
            z80_call_depth++;
         else
            if (IS_OPCODE_RET(opcode) &&
            z80debug_does_cc_match(opcode, z80before.af))
               {
                z80_call_depth--; // Don't actually break here, we break on
                                  // the start of the next instruction
                if (z80_call_depth == 0)
                   xprintf(
                   "Z80 Debugging exited call via RET instruction at 0x%04x\n",
                   z80before.pc);
               }
        }

     // check for program counter (PC) break points
     if (! bp)
        {
         bp = z80debug_pc_breakpoints();
         if (bp)
            xprintf("Z80 Debugging break point at PC: 0x%04x\n", z80before.pc);
        }
     
     // check for port read/write and RST break points, may only work with
     // documented Z80 instructions.
     switch (opcode)
        {
         case 0xdb : // in a,(n)
            i = z80api_read_mem(z80before.pc+1);
            if (debug.break_point[i] &
            (Z80DEBUG_BP_PORTR_FLAG | Z80DEBUG_BPR_PORTR_FLAG))
               check_port = i;
            break;
         case 0xd3 : // out (n),a
            i = z80api_read_mem(z80before.pc+1);
            if (debug.break_point[i] &
            (Z80DEBUG_BP_PORTW_FLAG | Z80DEBUG_BPR_PORTW_FLAG))
               check_port = i + 256;
            break;
         case 0xed : // in r,(c) ini inir ind indr out (c),r outi otir outd otdr
            switch (z80api_read_mem(z80before.pc+1) & B8(11000111))
               {
                case B8(01000000) : // in r,(c) and 'in (c)'
                case B8(10000010) : // ini inir ind indr
                   i = z80before.bc & 0xff;
                   if (debug.break_point[i] &
                   (Z80DEBUG_BP_PORTR_FLAG | Z80DEBUG_BPR_PORTR_FLAG))
                      check_port = i;
                   break;
                case B8(01000001) : // out (c),r
                   i = z80before.bc & 0xff;
                   if (debug.break_point[i] &
                   (Z80DEBUG_BP_PORTW_FLAG | Z80DEBUG_BPR_PORTW_FLAG))
                      check_port = i + 256;
                   break;
                case B8(10000011) : // outi otir outd otdr
                   i = z80api_read_mem(z80before.hl);
                   if (debug.break_point[i] &
                   (Z80DEBUG_BP_PORTW_FLAG | Z80DEBUG_BPR_PORTW_FLAG))
                      check_port = i + 256;
                   break;
               }
            break;
         default : // check for RST break points
            if ((opcode & B8(11000111)) != B8(11000111))
               break;
            rst = (opcode & B8(00111000)) >> 3;
            if (debug.rst_break_point[rst])
               {
                xprintf(
                "Z80 'RST %02xH' Debugging break point at PC: 0x%04x\n",
                rst * 0x08, z80before.pc);
                if (debug.rst_break_point[rst] == 1)  // if once only
                   debug.rst_break_point[rst] = 0;
                bp = 1;
               }
        }

     // check for debug count break point
     if ((debug.debug_count == debug.break_point_count) && debug.debug_count)
        {
         xprintf(
         "Z80 Debugging break point at count: 0x%08x (%d) at PC: 0x%04x\n",
         debug.debug_count, debug.debug_count, z80before.pc);
         bp = 1;
        }

     z80debug_capture(2, NULL, NULL);

     if (bp)
        {
         z80debug_set_mode(Z80DEBUG_MODE_STOP);
         return -1;  // no stepping (no code execute) wanted on return
        }
    }

  // disassemble the code at the current PC (get's used in z80debug_after())
  if (disassemble || (check_port != -1))
     z80debug_prepare_dasm();
  else
     xmnemonic[0] = '\0'; // Indicated instruction wasn't disassembled,
                          // so we can't output it.

 z80pc_before = z80before.pc;
 z80pc_same++;
 debug.debug_count++;

 return 1;
}

//==============================================================================
// z80debug after instruction execution.
//
// Check for a port read/write breakpoint and enter step mode and report if
// detected.
//
// Output a disassembly of the instruction just executed and show state of
// registers.  The registers and other information displayed depends on
// option settings.
//
//   pass: void
// return: void
//==============================================================================
void z80debug_after (void)
{
 int i = 0;
 int bp = 0;
 int dasm_shown = 0;

 // if we're in any sort of trace or step mode, check for same PC
 if (debug.mode == Z80DEBUG_MODE_TRACE || 
 debug.mode == Z80DEBUG_MODE_STEP_VERBOSE ||
 debug.mode == Z80DEBUG_MODE_STEP_QUIET)
    {
     z80regs_t z80x;
     z80api_get_regs(&z80x);

     // don't keep showing same address unless bigger than max BC value
     if ((z80pc_before == z80x.pc) && (z80pc_same < 66000L))
        return;
    }

 z80pc_same = 0;

 // Show disassembly if not conditionally switched off
 if (z80debug_conditional_tracing())
    {
     if (debug.mode == Z80DEBUG_MODE_TRACE ||
     debug.mode == Z80DEBUG_MODE_STEP_VERBOSE)
        dasm_shown = z80debug_print_dasm(0);
    }

 // Check for memory and port breakpoints (unless we're in one of the
 // step modes)
 if ((debug.mode != Z80DEBUG_MODE_STEP_QUIET &&
 debug.mode != Z80DEBUG_MODE_STEP_VERBOSE) || debug.step > 1)
    {
     // Check for memory break point hit
     if (debug.memory_break_point_type != 0)
        {
         // Before showing the breakpoint, shown the instruction
         // that's causing it
         if (! dasm_shown)
            dasm_shown = z80debug_print_dasm(1);

         // break point hit!
         if (debug.memory_break_point_type == Z80DEBUG_BP_MEMW_FLAG)
            {
             z80debug_capture(3, cmds, NULL);
             xprintf(
             "Z80 'Write to memory address 0x%04x' Debugging break point"
             " at PC: 0x%04x\n", debug.memory_break_point_addr, z80before.pc);
             z80debug_capture(2, NULL, NULL);
            }
         else
            {
             z80debug_capture(3, cmds, NULL);
             xprintf(
             "Z80 'Read from  memory address 0x%04x' Debugging break point"
             " at PC: 0x%04x\n", debug.memory_break_point_addr, z80before.pc);
             z80debug_capture(2, NULL, NULL);
            }
         bp = 1;
        }

     // check for read/write ports break point value match
     if (check_port != -1)
        {
         i = (check_port & 0xff);

         if (check_port < 256)
            {
             if ((port_inp_state[i] == port_inp_bp_value[i]) ||
             (port_inp_bp_value[i] == -1))
                {
                 if (! dasm_shown)
                    dasm_shown = z80debug_print_dasm(1);

                 bp = 1;
                 z80debug_capture(3, cmds, NULL);
                 xprintf(
                 "Z80 'Read from port 0x%02x' Debugging break point"
                 " at PC: 0x%04x\n", i, z80before.pc);
                 z80debug_capture(2, NULL, NULL);
                 debug.break_point[i] &= ~Z80DEBUG_BP_PORTR_FLAG;
                }
            }
         else
            {
             if ((port_out_state[i] == port_out_bp_value[i]) ||
             (port_out_bp_value[i] == -1))
                {
                 if (! dasm_shown)
                    dasm_shown = z80debug_print_dasm(1);

                 bp = 1;
                 z80debug_capture(3, cmds, NULL);
                 xprintf(
                 "Z80 'Write to port 0x%02x' Debugging break point"
                 " at PC: 0x%04x\n", i, z80before.pc);
                 z80debug_capture(2, NULL, NULL);
                 debug.break_point[i] &= ~Z80DEBUG_BP_PORTW_FLAG;
                }
            }

         check_port = -1;
        }
    }

 // If we're stepping, check the step count
 if (debug.mode == Z80DEBUG_MODE_STEP_VERBOSE ||
 debug.mode == Z80DEBUG_MODE_STEP_QUIET)
    {
     debug.step--;
     if (debug.step == 0)
        z80debug_set_mode(Z80DEBUG_MODE_STOP);
    }

 // If a breakpoint was hit, switch to stop mode
 if (bp)
    z80debug_set_mode(Z80DEBUG_MODE_STOP);
}

//==============================================================================
// Dump lines of data.
//
// This may dump data from the Z80 memory map using an address or from a
// memory location using an offset.  The display of the header, ASCII dump,
// the address/offset shown as 8 or 16 bit are controlled by the flags
// formatting parameter.
//
//   pass: uint8_t *source      pointer to data or NULL if use Z80 memory map
//         int addr             Z80 memory address or an offset
//         int lines            number of lines (16 bytes per line), 0=64K
//         int flag             formatting flags.
// return: void
//==============================================================================
void z80debug_dump_lines (uint8_t *source, int addr, int lines, int flags)
{
 int c;
 int a;
 int mask;
 char x;

 if (flags & Z80DEBUG_DUMP_8BIT)
    {
     mask = 0x00ff;
     if ((lines > 16) || (lines <= 0))
        lines = 16;                     // 256 bytes maximum
    }
 else
    {
     mask = 0xffff;
     if ((lines > 4096) || (lines <= 0))
        lines = 4096;                   // 64K maximum
    }

 if (flags & Z80DEBUG_DUMP_HEAD)
    {
     if (flags & Z80DEBUG_DUMP_8BIT)
        xprintf("   ");
     else
        xprintf("     ");
     for (c = 0; c < 16; c++)
        xprintf(" %02X", c);
     xprintf("  ");
     for (c = 0; c < 16; c++)
        xprintf("%1X", c);
     xprintf("\n");
    }

 while (lines--)
    {
     addr &= mask;
     a = addr;

     if (flags & Z80DEBUG_DUMP_8BIT)
        xprintf("%02X:", a);
     else
        xprintf("%04X:", a);
     for (c = 0; c < 16; c++)
        {
         a &= mask;
         if (source == NULL)
            xprintf(" %02X", z80api_read_mem(a));
         else
            xprintf(" %02X", *(source + a));
         a++;
        }

     if (flags & Z80DEBUG_DUMP_NOASC)
        xprintf("\n");
     else
        {
         xprintf(" \"");
         a = addr;
         for (c = 0; c < 16; c++)
            {
             a &= mask;
             if (source == NULL)
                x = z80api_read_mem(a);
             else
                x = *(source + a);
             a++;
             if (x > 31 && x < 127)
                xprintf("%c", x);
             else
                xprintf(".");
            }
         xprintf("\"\n");
        }

     addr += 16;
    }
}

//==============================================================================
// get bank values for the passed bank type.
//
//   pass: int bank_type        bank type number
//         int bank             logical bank number
//         bank_data_t *b
// return: int                  0 if supported else -1
//==============================================================================
static int z80debug_get_bank_values (int bank_type, int bank, bank_data_t *b)
{
 b->size = 0;
 b->banks = 0;

 switch (bank_type)
    {
     case 0 : // screen RAM
        b->size = 0x0800;
        b->banks = modelx.vdu + 1;
        b->ptr = vdu.scr_ram + b->size * bank;
        break;
     case 1 : // colour RAM
        if (! modelx.colour) // if no colour emulation
           return -1;
        b->size = 0x0800;
        b->banks = modelx.vdu + 1;
        b->ptr = vdu.col_ram + b->size * bank;
        break;
     case 2 : // attribute RAM
        if (! modelx.alphap) // if no attribute RAM
           return -1;
        b->size = 0x0800;
        b->banks = modelx.vdu + 1;
        b->ptr = vdu.att_ram + b->size * bank;
        break;
     case 3 : // PCG RAM
        b->size = 0x0800;
        b->banks = modelx.pcg;
        b->ptr = vdu.pcg_ram + b->size * bank;
        break;
     case 4 : // main DRAM memory
        b->size = BLOCK_SIZE;
        b->banks = modelx.ram / 32;
        if (modelx.ram <= 56)
           return -1;  // 0-56K RAM models don't have DRAM banks
        if ((bank < 0) || (bank >= b->banks))
           return -1;
        b->ptr = block_ptrs[bank];
        break;
     case 5 : // all video RAM
        return 0;
     default :
        return -1;
    }

 if ((bank < 0) || (bank >= b->banks))
    return -1;

 return 0;
}

//==============================================================================
// Process --db-bp-port, --db-bpclr-port and --db-bpr-port options.
//
//  --db-bp-port=d,p,n
//
//  Set a breakpoint for a read/write on port 'p' with a matching value 'n'.
//  'n=*' may be used to match any value. The port direction 'd', may be 'w'
//  for writes and 'r' for reads.
//
//  --db-bpclr-port=d,p
//
//  Clear a breakpoint for port 'p', for port direction 'd', where 'd' may
//  be 'w' for writes and 'r' for reads.
//
//  --db-bpr-port=d,p,n
//
//  Same action as --db-bp-port option except the break point is not cleared
//  after detection.
//
//   pass: char *p              parameters
//         int style            's' for --db-bp-port, 'c' for --db-bpclr-port
//                              and 'r' for --db-bpr-port.
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_bp_port (char *p, int style)
{
 char *c;
 char sp[100];

 int temp;

 int rw_dir;
 int port;
 int value;

 // get the direction type 'd'
 c = get_next_parameter(p, ',', sp, &temp, sizeof(sp)-1);
 if ((rw_dir = string_search(direction_rw_args, sp)) == -1)
    return -1;

 // get the port number 'p'
 c = get_next_parameter(c, ',', sp, &port, sizeof(sp)-1);
 if ((port < 0) || (port > 255))
    return -1;

 if (style == 'c')
    {
     if (c != NULL)
        return -1;
     if (rw_dir == 0)
        debug.break_point[port] &=
        ~(Z80DEBUG_BP_PORTR_FLAG | Z80DEBUG_BPR_PORTR_FLAG);
     else
        debug.break_point[port] &=
        ~(Z80DEBUG_BP_PORTW_FLAG | Z80DEBUG_BPR_PORTW_FLAG);
     return 0;
    }

 // get the port value 'n'
 c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1);
 if (c != NULL)
    return -1;

 if (strcmp(sp, "*") == 0)
    value = -1;
 else
    if ((value < 0) || (value > 255))
       return -1;

 // set the value to cause a break point (-1 for any)
 if (rw_dir == 0)
    port_inp_bp_value[port] = value;
 else
    port_out_bp_value[port] = value;

 // set a single or repeated port break point
 if (style == 's')
    {
     if (rw_dir == 0)
        debug.break_point[port] |= Z80DEBUG_BP_PORTR_FLAG;
     else
        debug.break_point[port] |= Z80DEBUG_BP_PORTW_FLAG;
    }
 else
    {
     if (rw_dir == 0)
        debug.break_point[port] |= Z80DEBUG_BPR_PORTR_FLAG;
     else
        debug.break_point[port] |= Z80DEBUG_BPR_PORTW_FLAG;
    }

 return 0;
}

//==============================================================================
// Process --db-bp-mem, --db-bp-mem, --db-bpclr-mem, --db-bpclr-meml, 
//
//  --db-bp-mem=d,s[,f]
//
//  Set a breakpoint for a read/write on memory address range 's' to 'f' 
//  (inclusive). The direction 'd', may be 'w' for writes and 'r' for reads.
//
//  --db-bpclr-mem=d,s[,f]
//
//  Clear a breakpoint for a read/write on memory address range 's' to 'f'. 
//  (inclusive). The direction 'd', may be 'w' for writes and 'r' for reads.
//
//  --db-bp-meml=d,s,l
//  --db-bpclr-meml=d,s,l
//
//  Same as above except the last parameter is a length in bytes, rather than
//  an end address.
//
//   pass: char *p              parameters
//         int kind             's' for --db-bp-mem[l]
//                              'c' for --db-bpclr-mem[l]
//         int style            'a' for --db-bp[clr]-mem
//                              'l' for --db-bp[clr]-meml
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_bp_mem (char *p, int kind, int style)
{
 int start;
 int i;
 int finish = 0;
 int temp;
 int rw_dir;
 char* c;
 char sp[100];

 // get the direction type 'd'
 c = get_next_parameter(p, ',', sp, &temp, sizeof(sp)-1);
 if ((rw_dir = string_search(direction_rw_args, sp)) == -1)
    return -1;

 // get the start address 's'
 c = get_next_parameter(c, ',', sp, &start, sizeof(sp)-1);
 if ((start < 0) || (start > 0xffff))
    return -1;

 if (c == NULL)
    // If second parameter not specified, assume 1 byte
    finish = start;
 else
    {
     // get the finish address 'f', or length 'l'
     c = get_next_parameter(c, ',', sp, &finish, sizeof(sp)-1);

     // Is second parameter a length?
     if (style == 'l')
        finish = start + finish - 1;

     if ((finish < 0) || (finish > 0xffff))
        return -1;

     if (start > finish)
        return -1;
    }

 // Set/Clear appropriate flags
 for (i = start; i <= finish; i++)
    {
     switch (kind)
        {
         case 's' :
            debug.break_point[i] |=
            rw_dir ? Z80DEBUG_BP_MEMW_FLAG : Z80DEBUG_BP_MEMR_FLAG;
            break;

         case 'c' :
            debug.break_point[i] &=
            ~(rw_dir ? Z80DEBUG_BP_MEMW_FLAG : Z80DEBUG_BP_MEMR_FLAG);
            break;
        }
    }

 return 0;
}

//==============================================================================
// Process --db-dasm and --db-dasml options.
//
// Various forms of the command are possible:
//
// --db-dasm s,f
//
// Disassemble Z80 code starting at address 's' and finishing at 'f'.
//
// --db-dasml=[s[,l]]
//
// Disassemble Z80 code starting at address 's' for 'l' number of lines. If
// the optional parameters are omitted the disassembly continues on from the
// last address for the current line value as set with the --dasm-lines
// option.
//
// This function works on the current Z80 memory map arrangement, memory
// locations will be dependent on the current port 0x50 setting on DRAM
// models, other things like character ROM may also be in the memory map and
// needs to be taken into account.
//
//   pass: char *p              parameters
//         int style            'l' for lines, 'a' for start and finish address
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_dasm (char *p, int style)
{
 char mnemonic[30];
 char argument[30];
 int t_states;
 int t_states2;

 char sp[100];

 char *c;
 int start;
 int finish = 0;
 int value;
 int count;
 int lines = debug.dasm_lines;

 // --db-dasm s,f
 if (style == 'a')
    {
     // get the start address 's'
     c = get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
     if ((start < 0) || (start > 0xffff))
        return -1;

     // get the finish address 'f'
     c = get_next_parameter(c, ',', sp, &finish, sizeof(sp)-1);
     if ((finish < 0) || (finish > 0xffff))
        return -1;

     if (start > finish)
        return -1;

     dasm_addr = start;
     lines = 1;
    }

 // --db-dasml=[s[,l]]
 if (style == 'l')
    {
     finish = 0x10000;  // value can't be reached
     if (p[0])  // if no parameters specified
        {
         // get the start address 's'
         c = get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
         if ((start < 0) || (start > 0xffff))
            return -1;

         dasm_addr = start;

         c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1);
         if (sp[0])
            {
             if (value < 1)
                return -1;
             else
                lines = value;
            }
        }
    }

 while ((dasm_addr < finish) && lines)
    {
     count =
     z80api_dasm(dasm_addr, 1, mnemonic, argument, &t_states, &t_states2);

     xprintf("%04x: %-8s%-12s", dasm_addr, mnemonic, argument);

     if (t_states2)
        xprintf("t%d/%d\n", t_states, t_states2);
     else
        xprintf("t%d\n", t_states);

     dasm_addr += count;
     dasm_addr &= 0xffff;
     if (style == 'l')
        lines--;
    }

 return 0;
}

//==============================================================================
// Process --db-dump and --db-dumpl options.
//
// Various forms of the command are possible:
//
// --db-dump=s,f[,h]
//
// Dump memory starting at address 's' and finishing at 'f'. The optional
// 'h' value determines if a header is used. A '+h' enables and a '-h'
// disables the header. The default header setting is determined by the
// --dump-header option if the 'h' value is omitted.
//
// --db-dumpl=[s[,l][,h]]
//
// Dump memory starting at address 's' for 'l' number of lines. If the
// optional parameters are omitted the dump continues on from the last
// address for the current line value as set with the --dump-lines option.
// The 'h' value is the same as that described for the --db-dump option.
//
// This function works on the current Z80 memory map arrangement, memory
// locations will be dependent on the current port 0x50 setting on DRAM
// models, other things like character ROM may also be in the memory map and
// needs to be taken into account.
//
//   pass: char *p              parameters
//         int style            'l' for lines, 'a' for start and finish address
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_dump_memory (char *p, int style)
{
 char sp[100];

 char *c;
 int start;
 int finish;
 int value;
 int lines = debug.dump_lines;
 int header = debug.dump_header;

 // --db-dump=s,f[,h]
 if (style == 'a')
    {
     // get the start address 's'
     c = get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
     if ((start < 0) || (start > 0xffff))
        return -1;

     // get the finish address 'f'
     c = get_next_parameter(c, ',', sp, &finish, sizeof(sp)-1);
     if ((finish < 0) || (finish > 0xffff))
        return -1;

     if (start > finish)
        return -1;

     c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1);
     if (sp[0])
        {
         if ((header = string_search(header_args, sp)) == -1)
            return -1;
        }
     dump_addr = start;
     lines = ((finish - start) + 16) / 16;
     z80debug_dump_lines(NULL, start, lines, header * Z80DEBUG_DUMP_HEAD);
     dump_addr = start + 16 + lines;
     return 0;
    }

 // --db-dumpl
 if (! p[0])
    {
     z80debug_dump_lines(NULL, dump_addr, lines, header * Z80DEBUG_DUMP_HEAD);
     dump_addr = start + 16 + lines;
     return 0;
    }

 // --db-dumpl=[s[,l][,h]]
 c = get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
 if ((start < 0) || (start > 0xffff))
    return -1;

 dump_addr = start;
 c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1);
 if (sp[0])
    {
     if (value != -1)
        {
         lines = value;
         c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1);
         if (sp[0])
            {
             header = string_search(header_args, sp);
             if (header == -1)
                return -1;
            }
        }
     else
        header = string_search(header_args, sp);
        if (header == -1)
           return -1;
    }

 z80debug_dump_lines(NULL, dump_addr, lines, header * Z80DEBUG_DUMP_HEAD);
 dump_addr = start + 16 * lines;

 return 0;
}

//==============================================================================
// Process --db-dumpb and --db-dumplb options.
//
// Various forms of the command are possible:
//
// --db-dumpb=t,b,s,f[,h]
//
// Dump bank memory type 't', bank 'b', starting at offset 's' and finishing
// at 'f'. The optional 'h' value determines if a header is used. A '+h'
// enables and a '-h' disables the header. The default header setting is
// determined by the --dump-header option if the 'h' value is omitted.
//
// --db-dumplb=t,b,s,l[,h]
//
// Dump bank memory type 't', bank 'b', starting at offset 's' for number of
// lines 'l'. The 'h' value is the same as that described for the --db-dumpb
// option.
//
//   pass: char *p              parameters
//         int style            'l' for lines, 'a' for start and finish offsets
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_dump_bank (char *p, int style)
{
 char *c;
 char sp[100];

 bank_data_t b;
 int temp;

 int bank_type;
 int bank;
 int start;
 int finish;
 int lines;
 int header = debug.dump_header;

 // get the bank type 't'
 c = get_next_parameter(p, ',', sp, &temp, sizeof(sp)-1);
 if ((bank_type = string_search(bank2_args, sp)) == -1)
    return -1;

 // get the bank number 'b'
 c = get_next_parameter(c, ',', sp, &bank, sizeof(sp)-1);

 if (z80debug_get_bank_values(bank_type, bank, &b) == -1)
    return -1;

 // get the start offset 's'
 c = get_next_parameter(c, ',', sp, &start, sizeof(sp)-1);

 if ((start < 0) || (start >= b.size))
    return -1;

 if (style == 'a')
    {
     // get the finish offset 'f'
     c = get_next_parameter(c, ',', sp, &finish, sizeof(sp)-1);
     if ((finish < 0) || (finish >= b.size) || (start > finish))
        return -1;
     lines = ((finish - start) + 16) / 16;
    }
 else
    {
     // get the lines wanted 'l'
     c = get_next_parameter(c, ',', sp, &lines, sizeof(sp)-1);
     if ((lines < 0) || (lines > ((((b.size-1) - start) + 16) / 16)))
         return -1;
    }

 // get the optional header flag 'h'
 c = get_next_parameter(c, ',', sp, &temp, sizeof(sp)-1);
 if (sp[0])
    {
     if ((header = string_search(header_args, sp)) == -1)
        return -1;
    }

 z80debug_dump_lines((uint8_t*)b.ptr, start, lines,
 header * Z80DEBUG_DUMP_HEAD);

 return 0;
}

//==============================================================================
// Process --db-dumpp option.
//
// --db-portd=d,p[,p..]
//
// Dump the current Z80 8 bit port 'p' input/output state values for
// direction 'd', where 'd=i' for inputs and 'd=o' for outputs. All 256
// ports will be dumped if 'a' or 'all' is specified for 'p'. This option
// will not read or write to the port.
//
//   pass: char *p              parameters
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_dump_port (char *p)
{
 char *c;
 char sp[100];

 int temp;
 int direction;
 int port;
 uint8_t *port_state;

 // get port input or output direction 't'
 c = get_next_parameter(p, ',', sp, &temp, sizeof(sp)-1);
 if ((direction = string_search(direction_args, sp)) == -1)
    return -1;

 if (direction == 1)
    port_state = port_out_state;
 else
    port_state = port_inp_state;

 if (c == NULL) // must be at least one 'p' value
    return -1;

 while (c != NULL)
    {
     c = get_next_parameter(c, ',', sp, &port, sizeof(sp)-1); // get a value
     if (! sp[0])
        return 0;

     if ((strcasecmp(sp, "a") == 0) || (strcasecmp(sp, "all") == 0))
        z80debug_dump_lines((uint8_t*)port_state, 0, 16, Z80DEBUG_DUMP_8BIT |
        Z80DEBUG_DUMP_HEAD);
     else
        {
         if ((port < 0) || (port > 0xff))
            return -1;
         xprintf("Port 0x%02x: 0x%02x (%d)\n", port, port_state[port],
         port_state[port]);
        }
    }

 return 0;
}

//==============================================================================
// Process --db-fillb option.
//
// --db-fillb=t,b,v
//
// Fill bank memory type 't', bank 'b' using value 'v'. All banks belonging
// to type 't' may be filled by specifying 'a' or 'all' for bank 'b'.
//
// TYPE         RAM type
// ---          --------
// att          attribute
// col          colour
// pcg          PCG
// scr          screen
// vid          all video
// mem          DRAM
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_fill_bank (char *p)
{
 char *c;
 char sp[100];

 bank_data_t b;
 bank_data_t x;

 int i;
 int temp;
 int all_banks = 0;

 int bank_type;
 int bts;
 int btf;
 int bank;
 int value;

 // get the bank type 't'
 c = get_next_parameter(p, ',', sp, &temp, sizeof(sp)-1);
 if ((bank_type = string_search(bank_args, sp)) == -1)
    return -1;

 // get the bank number 'b'
 c = get_next_parameter(c, ',', sp, &bank, sizeof(sp)-1);
 if ((strcasecmp(sp, "a") == 0) || (strcasecmp(sp, "all") == 0))
    {
     bank = 0;
     all_banks = 1;
    }

 if (z80debug_get_bank_values(bank_type, bank, &b) == -1)
    return -1;

 c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1);
 if ((value < 0) || (value > 255))
    return -1;

 if (bank_type == 5)
    {
     bts = 0;
     btf = 3;
     all_banks = 1;
    }
 else
    {
     bts = bank_type;
     btf = bank_type;
    }

 if (all_banks)
    {
     while (bts <= btf)
        {
         z80debug_get_bank_values(bts, 0, &x);
         for (i = 0; i < x.banks; i++)
            {
             z80debug_get_bank_values(bts, i, &x);
             memset(x.ptr, value, x.size);
            }
         bts++;
        }
    }
 else
    memset(b.ptr, value, b.size);

 return 0;
}

//==============================================================================
// Process --db-findb option.
//
// --db-findb=t,s,f,o,d
//
// Search banked memory type 't', starting with bank 's', finishing at bank
// 'f' with an initial starting offset of 'o' in the first bank.  The 'f'
// value may be 'a' or 'all' for all remaining banks. The 'bank:offset'
// values where matches are found will be displayed. The search criteria is
// passed in 'd' which may consist of any of the arguments as specified in
// the z80debug_create_search_array() function description.
//
// Examples:
// --db-findb mem,0,all,0,c,miCroBeE
// --db-findb "mem,1,1,0,a,Some text,0xaa,0x55,w,0xaa55,0x1234,b,0x00"..
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_find_bank (char *p)
{
 uint8_t search[Z80DEBUG_SEARCH_SIZE];

 char sp[100];
 char *c;

 bank_data_t b;

 int temp;
 int bank_type;
 int start_bank;
 int finish_bank;
 int offset;
 int ofs = 0;
 int l = 0;
 int any_case = 0;
 int matches = 0;

 // get the bank type 't'
 c = get_next_parameter(p, ',', sp, &temp, sizeof(sp)-1);
 if ((bank_type = string_search(bank2_args, sp)) == -1)
    return -1;

 // get the starting bank number 's'
 c = get_next_parameter(c, ',', sp, &start_bank, sizeof(sp)-1);

 if (z80debug_get_bank_values(bank_type, start_bank, &b) == -1)
    return -1;

 // get the finishing bank number 'f'
 c = get_next_parameter(c, ',', sp, &finish_bank, sizeof(sp)-1);

 if ((strcasecmp(sp, "a") == 0) || (strcasecmp(sp, "all") == 0))
    finish_bank = b.banks - 1;

 // get the initial starting offset 'o'
 c = get_next_parameter(c, ',', sp, &offset, sizeof(sp)-1);

 if ((finish_bank >= b.banks) || (start_bank > finish_bank) ||
 (offset >= b.size))
    return -1;

 if (c == NULL) // must be at least one 'd' value
    return -1;

 // create a search array from all the 'd' values
 if ((l = z80debug_create_search_array(c, search, &any_case)) == -1)
    return -1;

 while ((ofs != -1) && (matches < debug.find_count) &&
 (start_bank <= finish_bank))
    {
     if ((ofs = array_search(b.ptr, search, offset, b.size-1, l, any_case))
     != -1)
        {
         xprintf("0x%02x:0x%04x ", start_bank, ofs);
         matches++;
         if ((matches % 6) == 0)
            xprintf("\n");
         offset = ofs + 1; // next search

         if (offset > b.size)
            {
             if (++start_bank <= finish_bank)
                {
                 // get the pointer to the next bank
                 z80debug_get_bank_values(bank_type, start_bank, &b);
                 offset = 0;
                }
            }
        }
     else
        {
         if (++start_bank <= finish_bank)
            {
             offset = 0;
             ofs = 0;
             // get the pointer to the next bank
             z80debug_get_bank_values(bank_type, start_bank, &b);
            }
        }
    }

 if (! matches)
    xprintf("No match found.\n");
 else
    {
     if ((matches % 6) != 0)
        xprintf("\n");
    }

 // check and report if there are any more matches possible
 if ((ofs != -1) && (matches == debug.find_count) &&
 (start_bank <= finish_bank))
    {
     if ((ofs = array_search(b.ptr, search, offset, b.size-1, l, any_case))
     != -1)
        xprintf(
        "More matches were found. Use --find-count option to increase.\n");
    }

 return 0;
}

//==============================================================================
// Process --db-findm option.
//
// --db-findm=s,f,d
//
// Search memory starting at address 's' and finishing at 'f' with the
// address displayed where a succesful search was located. The search
// criteria is passed in 'd' which may consist of any of the arguments as
// specified in the z80debug_create_search_array() function description.
//
// Examples:
// --db-findm 0xe000,0xefff,c,miCroBeE
// --db-findm "0xe000,0xefff,a,Some text,0xaa,0x55,w,0xaa55,0x1234,b,0x00"..
//
// This function works on the current Z80 memory map arrangement, memory
// locations will be dependent on the current port 0x50 setting on DRAM
// models, other things like character ROM may also be in the memory map and
// needs to be taken into account.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_find_memory (char *p)
{
 uint8_t search[Z80DEBUG_SEARCH_SIZE];

 char sp[100];
 char *c;

 int start;
 int finish;
 int l = 0;
 int any_case = 0;
 int addr = 0;
 int matches = 0;

#if 0
 // clear the array to make it easier to view when debugging this code
 memset(search, 0, Z80DEBUG_SEARCH_SIZE);
#endif

 // get the start address 's'
 c = get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
 if ((start < 0) || (start > 0xffff))
    return -1;

 // get the finish address 'f'
 c = get_next_parameter(c, ',', sp, &finish, sizeof(sp)-1);
 if ((finish < 0) || (finish > 0xffff))
    return -1;

 if (start > finish)
    return -1;

 if (c == NULL) // must be at least one 'd' value
    return -1;

 // create a search array from all the 'd' values
 if ((l = z80debug_create_search_array(c, search, &any_case)) == -1)
    return -1;

 while ((addr != -1) && (matches < debug.find_count) && (start <= finish))
    {
     if ((addr = array_search(NULL, search, start, finish, l, any_case)) != -1)
        {
         xprintf("0x%04x ", addr);
         matches++;
         if ((matches % 10) == 0)
            xprintf("\n");
         start = addr + 1; // next search
        }
    }

 if (! matches)
    xprintf("No match found.\n");
 else
    {
     if ((matches % 10) != 0)
        xprintf("\n");
    }

 // check and report if there are any more matches possible
 if ((addr != -1) && (matches == debug.find_count) && (start <= finish))
    {
     if ((addr = array_search(NULL, search, start, finish, l, any_case)) != -1)
        xprintf(
        "More matches were found. Use --find-count option to increase.\n");
    }

 return 0;
}

//==============================================================================
// Process --db-fillm option.
//
// --db-fill s,f,v
//
// Fills memory starting at address 's' and finishing at 'f' with value 'v'.
//
// This function works on the current Z80 memory map arrangement, memory
// locations will be dependent on the current port 0x50 setting on DRAM
// models, other things like character ROM may also be in the memory map and
// needs to be taken into account.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_fill_memory (char *p)
{
 int i;
 int start;
 int finish;
 int value;

 char *c;
 char sp[100];

 // get the start address 's'
 c = get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
 if ((start < 0) || (start > 0xffff))
    return -1;

 // get the finish address 'f'
 c = get_next_parameter(c, ',', sp, &finish, sizeof(sp)-1);
 if ((finish < 0) || (finish > 0xffff))
    return -1;

 if (start > finish)
    return -1;

 c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1);
 if ((value < 0) || (value > 0xff))
    return -1;

 for (i = start; i <= finish; i++)
    z80api_write_mem(i, value);

 return 0;
}

//==============================================================================
// Process --db-move option.
//
// --db-move s,d,a
//
// Move (copy) memory from source 's' to destination 'd' for amount 'a'
//
// This function works on the current Z80 memory map arrangement, memory
// locations will be dependent on the current port 0x50 setting on DRAM
// models, other things like character ROM may also be in the memory map and
// needs to be taken into account.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_move_memory (char *p)
{
 int source;
 int destination;
 int amount;
 int data;

 char *c;
 char sp[100];

 c = get_next_parameter(p, ',', sp, &source, sizeof(sp)-1);
 if ((source < 0) || (source > 0xffff))
    return -1;

 c = get_next_parameter(c, ',', sp, &destination, sizeof(sp)-1);
 if ((destination < 0) || (destination > 0xffff))
    return -1;

 c = get_next_parameter(c, ',', sp, &amount, sizeof(sp)-1);
 if ((amount < 0) || (amount > 0xffff))
    return -1;

 if ((! amount) || (source == destination))
    return 0;  // not an error, do nothing

 if (source > destination)
    {
     while (amount--)
        {
         data = z80api_read_mem(source++);
         z80api_write_mem(destination++, data);
         source &= 0xffff;
         destination &= 0xffff;
        }
    }
 else
    {
     source += (amount - 1);
     destination += (amount - 1);
     while (amount--)
        {
         data = z80api_read_mem(source--);
         z80api_write_mem(destination--, data);
         if (source < 0)
            source = 0xffff;
         if (destination < 0)
            destination = 0xffff;
        }
    }

 return 0;
}

//==============================================================================
// Process --db-portr option.
//
// --db-portr p[,m]
//
// Read port 'p' and display the value. An optional 'm' value if specified
// will be placed onto the MSB of the port address, if 'm' is omitted 0 will
// be used.
//
//   pass: char *p              parameters
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_port_read (char *p)
{
 char *c;
 char sp[100];

 int port;
 int value;
 int msb = 0;

 // get port number 'p'
 c = get_next_parameter(p, ',', sp, &port, sizeof(sp)-1);
 if ((port < 0) || (port > 0xff))
    return -1;

 // get optional 'm' MSB value
 c = get_next_parameter(c, ',', sp, &msb, sizeof(sp)-1); // get a value
 if (sp[0])
    {
     if ((msb < 0) || (msb > 0xff))
        return -1;
    }
 else
    msb = 0;

 value = z80api_read_port(port | (msb << 8));

 xprintf("0x%02x (%d)\n", value, value);

 return 0;
}

//==============================================================================
// Process --db-portw option.
//
//  --db-portw p,v[,v..]
//
//  Write value 'v' to port 'p'.
//
//   pass: char *p              parameters
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_port_write (char *p)
{
 char *c;
 char sp[100];

 int port;
 int value;

 // get port number 'p'
 c = get_next_parameter(p, ',', sp, &port, sizeof(sp)-1);
 if ((port < 0) || (port > 0xff))
    return -1;

 if (c == NULL) // must be at least one 'v' value
    return -1;

 // get values 'v' to write to port
 while (c != NULL)
    {
     c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1); // get a value
     if (! sp[0])
        return 0;

     if ((value < 0) || (value > 0xff))
        return -1;

     z80api_write_port(port, value);
    }

 return 0;
}

//==============================================================================
// Process --db-saveb option.
//
// --db-saveb t,b,file
//
// Saves bank memory type 't', bank 'b', to a file.  All banks that belong to
// type 't' will be saved if 'a' or 'all' is specified for 'b'.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_save_bank (char *p)
{
 FILE *fp;

 char *c;
 char sp[100];

 bank_data_t b;
 bank_data_t x;

 int i;
 int temp;
 int all_banks = 0;

 int bank_type;
 int bts;
 int btf;
 int bank;

 // get the bank type 't'
 c = get_next_parameter(p, ',', sp, &temp, sizeof(sp)-1);
 if ((bank_type = string_search(bank_args, sp)) == -1)
    return -1;

 // get the bank number 'b'
 c = get_next_parameter(c, ',', sp, &bank, sizeof(sp)-1);
 if ((strcasecmp(sp, "a") == 0) || (strcasecmp(sp, "all") == 0))
    {
     bank = 0;
     all_banks = 1;
    }

 if (z80debug_get_bank_values(bank_type, bank, &b) == -1)
    return -1;

 // get the file name 'file'
 c = get_next_parameter(c, ',', sp, &temp, sizeof(sp)-1);
 if (! sp[0])
    return -1;

 fp = fopen(sp, "wb");
 if (fp == NULL)
    {
     xprintf("z80debug_save_bank: Unable to create file: %s\n", sp);
     return -1;
    }

 if (bank_type == 5)
    {
     bts = 0;
     btf = 3;
     all_banks = 1;
    }
 else
    {
     bts = bank_type;
     btf = bank_type;
    }

 if (all_banks)
    {
     while (bts <= btf)
        {
         z80debug_get_bank_values(bts, 0, &x);
         for (i = 0; i < x.banks; i++)
            {
             z80debug_get_bank_values(bts, i, &x);
             fwrite(x.ptr, x.size, 1, fp);
            }
         bts++;
        }
    }
 else
    fwrite(b.ptr, b.size, 1, fp);

 fclose(fp);
 return 0;
}

//==============================================================================
// Process --db-loadb option.
//
// --db-loadb=t,b,file
//
// Loads bank memory type 't', bank 'b', with data from a file.  All banks
// that belong to type 't' will be loaded if 'a' or 'all' is specified for
// 'b'.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_load_bank (char *p)
{
 FILE *fp;

 char *c;
 char sp[100];

 bank_data_t b;
 bank_data_t x;

 int i;
 int temp;
 int all_banks = 0;

 int bank_type;
 int bts;
 int btf;
 int bank;

 // get the bank type 't'
 c = get_next_parameter(p, ',', sp, &temp, sizeof(sp)-1);
 if ((bank_type = string_search(bank_args, sp)) == -1)
    return -1;

 // get the bank number 'b'
 c = get_next_parameter(c, ',', sp, &bank, sizeof(sp)-1);
 if ((strcasecmp(sp, "a") == 0) || (strcasecmp(sp, "all") == 0))
    {
     bank = 0;
     all_banks = 1;
    }

 if (z80debug_get_bank_values(bank_type, bank, &b) == -1)
    return -1;

 // get the file name 'file'
 c = get_next_parameter(c, ',', sp, &temp, sizeof(sp)-1);
 if (! sp[0])
    return -1;

 fp = fopen(sp, "rb");
 if (fp == NULL)
    {
     xprintf("z80debug_load_bank: Unable to open file: %s\n", sp);
     return -1;
    }

 if (bank_type == 5)
    {
     bts = 0;
     btf = 3;
     all_banks = 1;
    }
 else
    {
     bts = bank_type;
     btf = bank_type;
    }

 if (all_banks)
    {
     while (bts <= btf)
        {
         z80debug_get_bank_values(bts, 0, &x);
         for (i = 0; i < x.banks; i++)
            {
             z80debug_get_bank_values(bts, i, &x);
             if (fread(x.ptr, x.size, 1, fp) != 1)
                ; // no error
            }
         bts++;
        }
    }
 else
    if (fread(b.ptr, b.size, 1, fp) != 1)
       ; // no error
 fclose(fp);
 return 0;
}

//==============================================================================
// Process --db-savem option.
//
// --db-savem s,f,file
//
// Save memory starting at address 's' and finishing at 'f' to a file.
//
// This function works on the current Z80 memory map arrangement, memory
// locations will be dependent on the current port 0x50 setting on DRAM
// models, other things like character ROM may also be in the memory map and
// needs to be taken into account.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_save_memory (char *p)
{
 FILE *fp;

 int start;
 int finish;
 int x;

 uint8_t data;

 char *c;
 char sp[512];

 // get the start address 's'
 c = get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
 if ((start < 0) || (start > 0xffff))
    return -1;

 // get the finish address 'f'
 c = get_next_parameter(c, ',', sp, &finish, sizeof(sp)-1);
 if ((finish < 0) || (finish > 0xffff))
    return -1;

 if (start > finish)
    return -1;

 c = get_next_parameter(c, ',', sp, &x, sizeof(sp)-1);
 if (! sp[0])
    return -1;

 fp = fopen(sp, "wb");

 if (fp == NULL)
    {
     xprintf("z80debug_save_memory: Unable to create file: %s\n", sp);
     return -1;
    }

 while (start <= finish)
    {
     data = z80api_read_mem(start++);
     fwrite(&data, 1, 1, fp);
    }
 fclose(fp);

 return 0;
}

//==============================================================================
// Process --db-loadm option.
//
// --db-loadm a,file
//
// Loads memory address 'a' with data from a file.  Up to 65536 bytes may be
// loaded, if the value is exceeded the process terminates without error.
//
// This function works on the current Z80 memory map arrangement, memory
// locations will be dependent on the current port 0x50 setting on DRAM
// models, other things like character ROM may also be in the memory map and
// needs to be taken into account.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_load_memory (char *p)
{
 FILE *fp;

 int addr;
 int count;
 int x;

 uint8_t data;

 char *c;
 char sp[512];

 c = get_next_parameter(p, ',', sp, &addr, sizeof(sp)-1);
 if ((addr < 0) || (addr > 0xffff))
    return -1;

 c = get_next_parameter(c, ',', sp, &x, sizeof(sp)-1);
 if (! sp[0])
    return -1;

 fp = fopen(sp, "rb");

 if (fp == NULL)
    {
     xprintf("z80debug_load_memory: Unable to open file: %s\n", sp);
     return -1;
    }

 count = 0;
 while (count++ < 0x10000)
    {
     if (fread(&data, 1, 1, fp) == 1)
        z80api_write_mem(addr++, data);
     else
        break;
     addr &= 0xffff;
    }

 fclose(fp);
 return 0;
}

//==============================================================================
// Process --db-setb option.
//
// --db-setb=t,b,o,v[,v..]
//
// Set memory in bank type 't', bank 'b' at offset 'o' with value(s) 'v'.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_set_bank (char *p)
{
 char *c;
 char sp[100];

 bank_data_t b;
 int temp;

 int bank_type;
 int bank;
 int offset;
 int value;

 // get the bank type 't'
 c = get_next_parameter(p, ',', sp, &temp, sizeof(sp)-1);
 if ((bank_type = string_search(bank2_args, sp)) == -1)
    return -1;

 // get the bank number 'b'
 c = get_next_parameter(c, ',', sp, &bank, sizeof(sp)-1);

 if (z80debug_get_bank_values(bank_type, bank, &b) == -1)
    return -1;

 // get the start offset 'o'
 c = get_next_parameter(c, ',', sp, &offset, sizeof(sp)-1);

 if ((offset < 0) || (offset >= b.size))
    return -1;

 if (c == NULL) // must be at least one 'v' value
    return -1;

 while (c != NULL)
    {
     c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1); // get a value
     if (sp[0])
        {
         if ((value < 0) || (value > 0xff))
            return -1;

         *(b.ptr + offset++) = value;
         offset &= (b.size - 1);
        }
    }

 return 0;
}

//==============================================================================
// Process --db-setm option.
//
// --db-setm=a,v[,v..]
//
// Set memory locations starting at address 'a' with value(s) 'v'.
//
// This function works on the current Z80 memory map arrangement, memory
// locations will be dependent on the current port 0x50 setting on DRAM
// models, other things like character ROM may also be in the memory map and
// needs to be taken into account.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_set_memory (char *p)
{
 char *c;
 char sp[100];

 int addr;
 int value;

 c = get_next_parameter(p, ',', sp, &addr, sizeof(sp)-1);
 if ((addr < 0) || (addr > 0xffff))
    return -1;

 if (c == NULL) // must be at least one 'v' value
    return -1;

 while (c != NULL)
    {
     c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1); // get a value
     if (sp[0])
        {
         if ((value < 0) || (value > 0xff))
            return -1;

         z80api_write_mem(addr++, value);
         addr &= 0xffff;
        }
    }

 return 0;
}

//==============================================================================
// Process --db-setr option.
//
// --db-setr=r,v
//
// Set an 8 or 16 bit register 'r', with value 'v'.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_set_reg (char *p)
{
 char *reg_args[] =
 {
  "af",   "bc",   "de",   "hl",
  "af_p", "bc_p", "de_p", "hl_p",
  "ix",   "iy",   "pc",   "sp",
  "i",    "r",
  "a",    "f",    "b",    "c",
  "d",    "e",    "h",    "l",
  "a_p",   "f_p", "b_p",  "c_p",
  "d_p",   "e_p", "h_p",  "l_p",
  ""
 };

 z80regs_t z80x;

 char *c;
 char sp[100];

 int *regs_table[] =
 {
  &z80x.af,   &z80x.bc,   &z80x.de,   &z80x.hl,
  &z80x.af_p, &z80x.bc_p, &z80x.de_p, &z80x.hl_p,
  &z80x.ix,   &z80x.iy,   &z80x.pc,   &z80x.sp,
  &z80x.i,    &z80x.r,
  &z80x.af,   &z80x.af,   &z80x.bc,   &z80x.bc,
  &z80x.de,   &z80x.de,   &z80x.hl,   &z80x.hl,
  &z80x.af_p, &z80x.af_p, &z80x.bc_p, &z80x.bc_p,
  &z80x.de_p, &z80x.de_p, &z80x.hl_p, &z80x.hl_p
 };

 int x;
 int regs_n;
 int *regs_v;
 int value;

 c = get_next_parameter(p, ',', sp, &x, sizeof(sp)-1);
 regs_n = string_search(reg_args, sp);
 if (regs_n == -1)
    return -1;

 c = get_next_parameter(c, ',', sp, &value, sizeof(sp)-1);
 if ((value < 0) || ((regs_n < 12) && (value > 0xffff)) ||
 ((regs_n >= 12) && (value > 0xff)))
    return -1;

 z80api_get_regs(&z80x);

 regs_v = regs_table[regs_n];

 if (regs_n >= 14)  // 8 bit operations on 16 bit regs
    {
     if (regs_n & 1)  // if registers f, c, e, l, f_p, c_p, e_p, l_p
        *regs_v = (*regs_v & 0xff00) | value;
     else             // else a, b, d, h, a_p, b_p, d_p, h_p
        *regs_v = (*regs_v & 0x00ff) | (value << 8);
    }
 else
    *regs_v = value;  // 16 bit and i, r registers

 z80api_set_regs(&z80x);

 return 0;
}

//==============================================================================
// Process --db-popr option.
//
// --db-popr
//
// Restore state of Z80 registers from an earlier --db-pushr option.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_pop_regs (char *p)
{
 if (! debug.pushed_regs)
   {
    xprintf("No registers to be popped.\n");
    return -1;
   }

 z80api_set_regs(&z80_pushed_regs);

 return 0;
}

//==============================================================================
// Process --db-pushr option.
//
// --db-pushr
//
// Save state of Z80 registers. Only one level is allowed.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_push_regs (char *p)
{
 z80api_get_regs(&z80_pushed_regs);
 debug.pushed_regs = 1;

 return 0;
}

//==============================================================================
// Process --db-popm option.
//
// --db-popm
//
// Restore state of memory from an earlier --db-pushm option.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_pop_mem (char *p)
{
 char *ptr;
 int start;
 int finish;

 if (pushed_mem == NULL)
    {
     xprintf("No memory to be popped.\n");
     return -1;
    }

 ptr = pushed_mem;
 start = debug.pushed_mem_start;
 finish = debug.pushed_mem_finish;

 while (start <= finish)
    z80api_write_mem(start++, *(ptr++));

 free(pushed_mem);
 pushed_mem = NULL;

 return 0;
}

//==============================================================================
// Process --db-pushm option.
//
// --db-pushm=s,f
//
// Save state of memory starting from address 's' and finishing at 'f'. Only
// one level is allowed.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_push_mem (char *p)
{
 char sp[100];
 char *c;

 char *ptr;
 int start;
 int finish;

 // get the start address 's'
 c = get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
 if ((start < 0) || (start > 0xffff))
    return -1;

 // get the finish address 'f'
 c = get_next_parameter(c, ',', sp, &finish, sizeof(sp)-1);
 if ((finish < 0) || (finish > 0xffff))
    return -1;

 if (start > finish)
    return -1;

 // if we already have pushed memory free it first.
 if (pushed_mem)
    free(pushed_mem);

 // allocate some memory (up to 64K)
 pushed_mem = malloc((finish - start) + 1);
 if (pushed_mem == NULL)
    return -1;

 ptr = pushed_mem;
 debug.pushed_mem_start = start;
 debug.pushed_mem_finish = finish;

 while (start <= finish)
    *(ptr++) = z80api_read_mem(start++);

 return 0;
}

//==============================================================================
// Process --db-step option.
//
// --db-step lines
//
// Step lines of instructions.  For continous operation pass 'c' or 'cont'
// and to stop pass 's', 'stop' or '0' for lines.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_step (char *p)
{
 char *step_args[] =
 {
  "c",
  "cont",
  "s",
  "stop",
  "o",
  "over",
  "x",
  "exit",
  ""
 };

 char sp[100];

 int res;

 get_next_parameter(p, ',', sp, &res, sizeof(sp)-1);

 if (res == -1)
    {
     res = string_search(step_args, sp);
     if (res == -1)
        return -1;

     switch (res)
        {
         case 0 :
         case 1 :
            z80debug_set_mode(Z80DEBUG_MODE_TRACE);
            break;

         case 2 :
         case 3 :
            z80debug_set_mode(Z80DEBUG_MODE_RUN);
            break;

         case 4 :
         case 5 :
            if (debug.mode == Z80DEBUG_MODE_STOP)  // Step over only works when
                                                // already stopped
               {
                // Need to check if the current instruction is actually a call 
                // (so that jp, ret etc... get stepped through correctly)
                z80regs_t z80regs;
                int opcode;
                z80api_get_regs(&z80regs);
                opcode = z80api_read_mem(z80regs.pc);

                if (IS_OPCODE_CALL(opcode))
                   {
                    // CALL or CALL cc
                    z80debug_set_mode(Z80DEBUG_MODE_RUN);
                    z80_step_over_stop_address = z80regs.pc + 3;
                   }
                else
                   {
                    // Not a CALL instruction, do a regular instruction step.
                    z80debug_set_mode(Z80DEBUG_MODE_STEP_QUIET);
                   }
               }
            else
               // already running, just switch to instruction step mode
               // and break
               z80debug_set_mode(Z80DEBUG_MODE_STEP_QUIET);
            break;

         case 6 :
         case 7 :
            if (debug.mode == Z80DEBUG_MODE_STOP)
               {
                // Switch to run mode
                z80debug_set_mode(Z80DEBUG_MODE_RUN);

                // And start call depth tracking
                z80_call_depth = 1;
               }
            else
               {
                xprintf("Can't step-out unless code execution is stopped\n");
                return 0;
               }
        }
    }
 else
    {
     if (res == 0)  // let 0 stop
        z80debug_set_mode(Z80DEBUG_MODE_RUN);
     else
        {
         z80debug_set_mode(Z80DEBUG_MODE_STEP_VERBOSE);
         debug.step = res;
        }
    }

 // If called from the console, tell it to exit so the step command
 // will run immediately.
 console_exit_while_debugger_runs();

 return 0;
}

//==============================================================================
// Process --bp, --db-bp option.
//
// --db-bp addr[,addr..]
//
// Set a Z80 PC address break point(s). This option can be used to set one
// or more break points separated by comma characters.  The break point is
// cleared after detection.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_pc_breakpoint_set (char *p)
{
 char *c;
 char sp[100];

 int addr;

 // get the address
 c = get_next_parameter(p, ',', sp, &addr, sizeof(sp)-1);

 if ((addr < 0) || (addr > 0xffff))
    return -1;
    
 debug.break_point[addr] = (debug.break_point[addr] &
 ~Z80DEBUG_BPR_FLAG) | Z80DEBUG_BP_FLAG;    

 while (c != NULL)
    {
     c = get_next_parameter(c, ',', sp, &addr, sizeof(sp)-1); // get address
     if (sp[0])
        {
         if ((addr < 0) || (addr > 0xffff))
            return -1;
         debug.break_point[addr] = (debug.break_point[addr] &
         ~Z80DEBUG_BPR_FLAG) | Z80DEBUG_BP_FLAG;    
        }
    }

 return 0;
}

//==============================================================================
// Process --bpr, --db-bpr option.
//
// --db-bpr addr[,addr..]
//
// Set a Z80 PC address break point(s). This option can be used to set one
// or more break points separated by comma characters.  The break point is
// NOT cleared after detection.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_pc_breakpoint_setr (char *p)
{
 char *c;
 char sp[100];

 int addr;

 // get the address
 c = get_next_parameter(p, ',', sp, &addr, sizeof(sp)-1);

 if ((addr < 0) || (addr > 0xffff))
    return -1;

 debug.break_point[addr] = (debug.break_point[addr] &
  ~Z80DEBUG_BP_FLAG) | Z80DEBUG_BPR_FLAG;

 while (c != NULL)
    {
     c = get_next_parameter(c, ',', sp, &addr, sizeof(sp)-1); // get address
     if (sp[0])
        {
         if ((addr < 0) || (addr > 0xffff))
            return -1;
         debug.break_point[addr] = (debug.break_point[addr] &
         ~Z80DEBUG_BP_FLAG) | Z80DEBUG_BPR_FLAG;
        }
    }

 return 0;
}

//==============================================================================
// Process --bpclr, --db-bpclr option.
//
// --db-bpclr addr
//
// Clear a Z80 address break point. 'a' or 'all' may be specified for 'addr'
// to clear all break points.
//
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_pc_breakpoints_clear (char *p)
{
 char sp[100];

 int start;

 // get the start address 's'
 get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
 if ((strcasecmp(sp, "a") == 0) || (strcasecmp(sp, "all") == 0))
    {
     for (start = 0; start < 0x10000; start++)
        debug.break_point[start] &= ~(Z80DEBUG_BP_FLAG | Z80DEBUG_BPR_FLAG);
     return 0;
    }

 if ((start < 0) || (start > 0xffff))
    return -1;

 debug.break_point[start] &= ~(Z80DEBUG_BP_FLAG | Z80DEBUG_BPR_FLAG);

 return 0;
} 

//==============================================================================
// Process --db-bpos option.
//
// --db-bpos s,f
//
// Set a break point when the PC is outside of the address range 's' and 'f'
// (inclusive).  This may be cleared using a 'c' or 'clr' for 's'.  The
// break point once triggered must re-enter the address range before another
// break can occur.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_pc_breakpoints_os (char *p)
{
 char sp[100];
 char *c;

 int start;
 int finish;

 // get the start address 's'
 c = get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
 if ((strcasecmp(sp, "c") == 0) || (strcasecmp(sp, "clr") == 0))
    {
     debug.pc_bp_os_addr_s = -1;
     return 0;
    }

 if ((start < 0) || (start > 0xffff))
    return -1;

 // get the finish address 'f'
 c = get_next_parameter(c, ',', sp, &finish, sizeof(sp)-1);
 if ((finish < 0) || (finish > 0xffff))
    return -1;

 if (start > finish)
    return -1;

 debug.pc_bp_os_addr_s = start;
 debug.pc_bp_os_addr_f = finish;

 return 0;
}

//==============================================================================
// Process --db-trace option.
//
// --db-trace s,f
//
// Trace only if PC is between addresses 's' and 'f' inclusively. Default is
// trace any PC value.
//
//   pass: char *p              parameter
// return: int                  0 if no error else -1
//==============================================================================
int z80debug_trace (char *p)
{
 char sp[100];
 char *c;

 int start;
 int finish;

 // get the start address 's'
 c = get_next_parameter(p, ',', sp, &start, sizeof(sp)-1);
 if ((start < 0) || (start > 0xffff))
    return -1;

 // get the finish address 'f'
 c = get_next_parameter(c, ',', sp, &finish, sizeof(sp)-1);
 if ((finish < 0) || (finish > 0xffff))
    return -1;

 if (start > finish)
    return -1;

 debug.cond_trace_addr_s = start;
 debug.cond_trace_addr_f = finish;

 return 0;
}

//==============================================================================
// process --db-dumpr option.
//
// Dump current value of all Z80 registers using 'all' output settings.
//
//   pass: void
// return: void
//==============================================================================
void z80debug_dump_registers (void)
{
 z80regs_t z80x;

 z80api_get_regs(&z80x);
 show_registers(&z80x, Z80DEBUG_ALL & ~Z80DEBUG_TSTATE, 5);
}

//==============================================================================
// Process --debug option.
//
//   pass: int arg              argument number
//         int pf               prefix used 0='-', 1='+'
// return: void
//==============================================================================
void z80debug_proc_debug_args (int arg, int pf)
{
 switch (arg)
    {
     case  0 : // off
        if (pf)
           z80debug_command_exec(EMU_CMD_DBGOFF, 0);
        else
           z80debug_command_exec(EMU_CMD_DBGON, 0);
        break;
     case  1 : // on
        if (pf)
           z80debug_command_exec(EMU_CMD_DBGON, 0);
        else
           z80debug_command_exec(EMU_CMD_DBGOFF, 0);
        break;
     case  2 : // Z80 standard registers
        debug.show = (debug.show & ~Z80DEBUG_REGS) | (Z80DEBUG_REGS * pf);
        break;
     case  3 : // Memory contents at register location (RR)
        debug.show = (debug.show & ~Z80DEBUG_MEMR) | (Z80DEBUG_MEMR * pf);
        break;
     case  4 : // index
        debug.show = (debug.show & ~Z80DEBUG_INDEX) | (Z80DEBUG_INDEX * pf);
        break;
     case  5 : // alt
        debug.show = (debug.show & ~Z80DEBUG_ALTREG) | (Z80DEBUG_ALTREG * pf);
        break;
     case  6 : // count
        debug.show = (debug.show & ~Z80DEBUG_COUNT) | (Z80DEBUG_COUNT * pf);
        break;
     case  7 : // tstates
        debug.show = (debug.show & ~Z80DEBUG_TSTATE) | (Z80DEBUG_TSTATE * pf);
        break;
     case  8 : // all
        debug.show = (debug.show & ~Z80DEBUG_ALL) | (Z80DEBUG_ALL * pf);
        break;
     case  9 : // piopoll
        debug.piopoll = pf;
        break;
     case 10 : // step
        if (pf)
           z80debug_command_exec(EMU_CMD_DBGSTEP01, 0);
        break;
     case 11 : // step10
        if (pf)
           z80debug_command_exec(EMU_CMD_DBGSTEP10, 0);
        break;
     case 12 : // step20
        if (pf)
           z80debug_command_exec(EMU_CMD_DBGSTEP20, 0);
        break;
     case 13 : // trace
        if (pf)
           z80debug_command_exec(EMU_CMD_DBGTRACE, 0);
        break;
    }
}

//==============================================================================
// Process --modio option.
//
// Note: The table of value pointers does not contain an entry for 'all'.
// The first entry is therefore arg=1.
//
//   pass: int arg              argument number (0=all)
//         int pf               prefix used 0='-', 1='+'
// return: void
//==============================================================================
void z80debug_proc_modio_args (int arg, int pf)
{
 int *modio_values[] =
 {
  &modio.level,
  &modio.raminit,
                                // The order of the pointers must match the
                                // order of the modio args in options.c
  &modio.beetalker,
  &modio.beethoven,
  &modio.clock,
  &modio.compumuse,
  &modio.crtc,
  &modio.dac,
  &modio.fdc,
  &modio.fdc_wtd,
  &modio.fdc_wth,
  &modio.func,
  &modio.hdd,
  &modio.ide,
  &modio.joystick,
  &modio.keystd,
  &modio.keytc,
  &modio.mem,
  &modio.options,
  &modio.roms,
  &modio.pioa,
  &modio.piob,
  &modio.piocont,
  &modio.rtc,
  &modio.sn76489an,
  &modio.tapfile,
  &modio.ubee512,
  &modio.vdu,
  &modio.vdumem,
  &modio.video,
  &modio.z80,
  NULL
 };

 if (arg)
    {
     // one value (log level raminit ignored if in run mode)
     if (emu.runmode && (arg <= 2))
        return;
     *modio_values[arg-1] = pf;
    }
 else
    {
     // all values except for log level and raminit
     arg += 2;
     while (modio_values[arg])
        *modio_values[arg++] = pf;
    }
}

//==============================================================================
// Process --regdump option.
//
// Note: The table of value pointers does not contain an entry for 'all'.
// The first entry is therefore arg=1.
//
//   pass: int arg              argument number (0=all)
//         int pf               prefix used 0='-', 1='+'
// return: void
//==============================================================================
void z80debug_proc_regdump_args (int arg, int pf)
{
 int *regdump_values[] =
 {
  &regdump.crtc,
  &regdump.pio,
  &regdump.rtc,
  &regdump.z80,
  NULL
 };

 if (arg)
    // one value
    *regdump_values[arg-1] = pf;
 else
    {
     // all values
     while (regdump_values[arg])
        *regdump_values[arg++] = pf;
    }
}

//==============================================================================
// z80debug commands.
//
// These may be called from various locations including options so any values
// here must not be initialised in z80debug_init() or z80debug_reset()
// functions.
//
//   pass: int cmd                      debugging command
//         int msg                      allow messages if non zero
// return: void
//==============================================================================
void z80debug_command_exec (int cmd, int msg)
{
 switch (cmd)
    {
     case EMU_CMD_DUMP :
        dump_addr_x = debug.dump_addr;
        z80debug_dump_lines(NULL, dump_addr_x, debug.dump_lines,
        debug.dump_header * Z80DEBUG_DUMP_HEAD);
        break;
     case EMU_CMD_DUMP_N1 :
        dump_addr_x += (16 * debug.dump_lines);
        z80debug_dump_lines(NULL, dump_addr_x, debug.dump_lines,
        debug.dump_header * Z80DEBUG_DUMP_HEAD);
        dump_addr_x &= 0xffff;
        break;
     case EMU_CMD_DUMP_N2 :
        dump_addr_x += 0x1000;
        z80debug_dump_lines(NULL, dump_addr_x, debug.dump_lines,
        debug.dump_header * Z80DEBUG_DUMP_HEAD);
        dump_addr_x &= 0xffff;
        break;
     case EMU_CMD_DUMP_B1 :
        dump_addr_x -= (16 * debug.dump_lines);
        z80debug_dump_lines(NULL, dump_addr_x, debug.dump_lines,
        debug.dump_header * Z80DEBUG_DUMP_HEAD);
        dump_addr_x &= 0xffff;
        break;
     case EMU_CMD_DUMP_B2 :
        dump_addr_x -= 0x1000;
        z80debug_dump_lines(NULL, dump_addr_x, debug.dump_lines,
        debug.dump_header * Z80DEBUG_DUMP_HEAD);
        dump_addr_x &= 0xffff;
        break;
     case EMU_CMD_DUMP_REP :
        z80debug_dump_lines(NULL, dump_addr_x, debug.dump_lines,
        debug.dump_header * Z80DEBUG_DUMP_HEAD);
        dump_addr_x &= 0xffff;
        break;
     case EMU_CMD_DUMPREGS :
        if (regdump.crtc)
           crtc_regdump();
        if (regdump.rtc)
           rtc_regdump();
        if (regdump.z80)
           z80api_regdump();
        if (regdump.pio)
           pio_regdump();
        break;
     case EMU_CMD_DBGOFF :
        z80debug_set_mode(Z80DEBUG_MODE_OFF);
        if (msg)
           xprintf("Debug mode is now off\n");
        break;
     case EMU_CMD_DBGON :
        z80debug_set_mode(Z80DEBUG_MODE_RUN);
        emu.z80_blocks = 0;  // make debug and normal finish ASAP
        if (msg)
           xprintf("Debug mode is now running\n");
        break;
     case EMU_CMD_DBGTRACE :
        if (debug.mode == Z80DEBUG_MODE_TRACE)
          z80debug_set_mode(Z80DEBUG_MODE_RUN);
        else
          z80debug_set_mode(Z80DEBUG_MODE_TRACE);
        break;
     case EMU_CMD_DBGSTEP01 :
        z80debug_set_mode(Z80DEBUG_MODE_STEP_VERBOSE);
        break;
     case EMU_CMD_DBGSTEP10 :
        z80debug_set_mode(Z80DEBUG_MODE_STEP_VERBOSE);
        debug.step = 10;
        break;
     case EMU_CMD_DBGSTEP20 :
        z80debug_set_mode(Z80DEBUG_MODE_STEP_VERBOSE);
        debug.step = 20;
        break;
     case EMU_CMD_DASML :
        z80debug_dasm("", 'l');
        break;
     case EMU_CMD_PAUSE :
        if (emu.paused)
           emu.paused = 0;
        else
           {
            emu.paused = 1;
            emu.z80_blocks = 0;  // make debug and normal finish ASAP
           }
        break;
    }

 if ((cmd >= EMU_CMD_DBGSTEP01) && (cmd <= EMU_CMD_DBGSTEP20))
    {
     if ((debug.step > 1) && (! emu.paused))
        xprintf("\n");
    }
}

//==============================================================================
// z80debug commands.
//
// This function should not be called by an option, options should call the
// z80debug_command_exec() function instead.  The normal use for this
// function is for EMUKEY and Joystick commands.
//
//   pass: int cmd              debugging command
//         int msg              allow messages if non zero
// return: void
//==============================================================================
void z80debug_command (int cmd, int msg)
{
 static char *command_names[] =
 {
  "dump",
  "dump+16*lines",
  "dump+0x1000",
  "dump-16*lines",
  "dump-0x1000",
  "dump rep",
  "dump peripheral registers",
  "debug off",
  "debug on",
  "trace toggle",
  "step 1",
  "step 10",
  "step 20",
  "dasm line(s)",
  "pause toggle"
 };

 sprintf(cmds, "z80debug_command: %s", command_names[cmd]);
 z80debug_capture(3, cmds, NULL);
 z80debug_command_exec(cmd, msg);
 z80debug_capture(2, NULL, NULL);
}

//==============================================================================
// Show debug mode pre-console prompt
//
// Called by the console just before prompting for input.  When debugger is 
// stopped, show current state information
//
//   pass: void
// return: int                  0 if debug mode is not STOP, else 1
//==============================================================================
int z80debug_print_console_prompt (void)
{
 z80regs_t z80regs;

 if (debug.mode != Z80DEBUG_MODE_STOP)
    return 0;

 z80api_get_regs(&z80regs);
 z80api_dasm(z80regs.pc, 1, xmnemonic, xargument, &xt_states, &xt_states2);
 xprintf("%04x: %-8s%-12s<---\n", z80regs.pc, xmnemonic, xargument);

 return 1;
}
