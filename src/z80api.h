/* z80api Header */

#ifndef HEADER_Z80API_H
#define HEADER_Z80API_H

#include <stdint.h>

typedef struct z80regs_t
{
 int af;
 int bc;
 int de;
 int hl;
 int af_p;
 int bc_p;
 int de_p;
 int hl_p;
 int ix;
 int iy;
 int pc;
 int sp;
 int i;
 int r;
}z80regs_t;

// Action function type, for actions that can occur on Z80 state
// changes (e.g. Z80 halt, reti callback, that sort of thing)
typedef void (*z80api_action_fn_t)(void);
typedef int (*z80api_status_fn_t)(void);
typedef enum { Z80_HALT = 0 } z80_event_t;
typedef struct z80_device_interrupt_t {
   // Interrupt enable IN function
   z80api_status_fn_t iei;
   // Interrupt acknowledgement function, called when the CPU has
   // executed a reti function (and interrupts have been enabled)
   z80api_action_fn_t intack;
} z80_device_interrupt_t;

void z80api_register_action (z80_event_t when, z80api_action_fn_t function);
void z80api_deregister_action (z80_event_t when, z80api_action_fn_t function);

int z80api_init (void);
int z80api_deinit (void);
int z80api_reset (void);
int z80api_getpc (void);
void z80api_set_poll_tstates_def (int tstates);
void z80api_set_poll_tstates (int tstates, int repeats);
void z80api_execute (int tstates);
void z80api_execute_complete (void);
void z80api_set_pc (int addr);
uint64_t z80api_get_tstates (void);
void z80api_register_interrupting_device (z80_device_interrupt_t *scratch,
                                          z80api_status_fn_t ieo,
                                          z80api_action_fn_t intack);
int z80api_intr_possible (void);
void z80api_nonmaskable_intr (void);
void z80api_maskable_intr (int vector);
void z80api_get_regs (z80regs_t *z80regs);
void z80api_set_regs (z80regs_t *z80regs);
void z80api_get_version (char *vers, int size);
void z80api_regdump (void);
uint8_t z80api_read_mem (int addr);
void z80api_write_mem (int addr, uint8_t value);
uint8_t z80api_read_port (int port);
void z80api_write_port (int port, uint8_t value);
int z80api_dasm (int addr, int lower, char *mnemonic, char *argument,
                 int *t_states, int *t_states2);


typedef void (*z80api_memhook)(uint32_t addr, int is_write);

void z80api_set_memhook (z80api_memhook hook);

#endif /* HEADER_Z80API_H */
