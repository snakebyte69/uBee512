/* z80debug Header */

#ifndef HEADER_Z80DEBUG_H
#define HEADER_Z80DEBUG_H

#define Z80DEBUG_SEARCH_SIZE 256

// dissasembly flags
#define Z80DEBUG_REGS   (1 << 0)
#define Z80DEBUG_MEMR   (1 << 1)
#define Z80DEBUG_INDEX  (1 << 2)
#define Z80DEBUG_ALTREG (1 << 3)
#define Z80DEBUG_COUNT  (1 << 4)
#define Z80DEBUG_TSTATE (1 << 5)
#define Z80DEBUG_ALL    0xffffffff

// dump flags
#define Z80DEBUG_DUMP_HEAD  0x00000001
#define Z80DEBUG_DUMP_8BIT  0x00000002
#define Z80DEBUG_DUMP_NOASC 0x00000004

// break point flags
// _BP_ = break point, _BPR_ = break point repeats
// _XXXXR_ = read operation, _XXXXW_ = write operation
#define Z80DEBUG_BP_FLAG        0x00000001
#define Z80DEBUG_BPR_FLAG       0x00000002
#define Z80DEBUG_BP_PORTR_FLAG  0x00000004
#define Z80DEBUG_BPR_PORTR_FLAG 0x00000008
#define Z80DEBUG_BP_PORTW_FLAG  0x00000010
#define Z80DEBUG_BPR_PORTW_FLAG 0x00000020
#define Z80DEBUG_BP_MEMR_FLAG   0x00000040
#define Z80DEBUG_BP_MEMW_FLAG   0x00000080

// debug.mode state values
#define Z80DEBUG_MODE_OFF          0 // Debugger disabled
#define Z80DEBUG_MODE_RUN          1 // Running, watching for breakpoints,
                                     // not tracing
#define Z80DEBUG_MODE_TRACE        2 // Running, tracing instructions as
                                     // they execute
#define Z80DEBUG_MODE_STOP         3 // Stopped in debugger
#define Z80DEBUG_MODE_STEP_VERBOSE 4 // Instruction stepping, disassembling at
                                     // the same time
#define Z80DEBUG_MODE_STEP_QUIET   5 // Instruction stepping, no disassembly
                                     // (used by step over for non-call
                                     // instruction)

int z80debug_init (void);
int z80debug_deinit (void);
int z80debug_reset (void);
void z80debug_capture (int action, char *option, char *optarg);
void z80debug_debug_file_close (void);
int z80debug_debug_file_create (char *fn);
int z80debug_before (void);
void z80debug_after (void);
void z80debug_dump_lines (uint8_t *source, int addr, int lines, int htype);
int z80debug_bp_port (char *p, int style);
int z80debug_bp_mem (char *p, int kind, int style);
int z80debug_dasm (char *p, int style);
int z80debug_dump_memory (char *p, int style);
int z80debug_dump_bank (char *p, int style);
int z80debug_dump_port (char *p);
void z80debug_dump_registers (void);
int z80debug_fill_bank (char *p);
int z80debug_fill_memory (char *p);
int z80debug_find_bank (char *p);
int z80debug_find_memory (char *p);
int z80debug_move_memory (char *p);
int z80debug_port_read (char *p);
int z80debug_port_write (char *p);
int z80debug_save_bank (char *p);
int z80debug_load_bank (char *p);
int z80debug_save_memory (char *p);
int z80debug_load_memory (char *p);
int z80debug_set_bank (char *p);
int z80debug_set_memory (char *p);
int z80debug_set_reg (char *p);
int z80debug_pop_regs (char *p);
int z80debug_push_regs (char *p);
int z80debug_pop_mem (char *p);
int z80debug_push_mem (char *p);
int z80debug_step (char *p);
int z80debug_pc_breakpoint_set (char *p);
int z80debug_pc_breakpoint_setr (char *p);
int z80debug_pc_breakpoints_clear (char *p);
int z80debug_pc_breakpoints_os (char *p);
int z80debug_trace (char *p);
void z80debug_proc_debug_args (int arg, int pf);
void z80debug_proc_modio_args (int arg, int pf);
void z80debug_proc_regdump_args (int arg, int pf);
void z80debug_command_exec (int cmd, int msg);
void z80debug_command (int cmd, int msg);
int z80debug_print_console_prompt (void);

typedef struct bank_data_t
{
 uint8_t *ptr;
 int banks;
 int size;
}bank_data_t;

typedef struct debug_t
{
 char break_point[0x10000];
 char rst_break_point[8];
 char last_option[50];
 int capture_state;
 int cond_trace_flag;
 int cond_trace_addr_s;
 int cond_trace_addr_f;
 int pc_bp_os_flag;
 int pc_bp_os_addr_s;
 int pc_bp_os_addr_f;
 int mode;
 int show;
 int step;
 int debug_count;
 int break_point_count;
 int piopoll;
 int dasm_addr;
 int dasm_lines;
 int dump_addr;
 int dump_lines;
 int dump_header;
 int find_count;
 int pushed_regs;
 int pushed_mem_start;
 int pushed_mem_finish;
 uint32_t memory_break_point_addr;
 int memory_break_point_type;
}debug_t;

extern debug_t debug;

#endif  /* HEADER_Z80DEBUG_H */
