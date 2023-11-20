/* Standard Keyboard Header */

#ifndef HEADER_KEYSTD_H
#define HEADER_KEYSTD_H

#define KEYSTD_MOD_CTRL_SHIFT 0x00000001
#define KEYSTD_MOD_ALL        0xffffffff

int keystd_init(void);
int keystd_deinit(void);
int keystd_reset(void);

void keystd_keydown_event (void);
void keystd_keyup_event (void);

void keystd_handler(int addr);
void keystd_checkall(void);
void keystd_force (int scan, int counts);
void keystd_force_none (int counts);
void keystd_scan_set (int scan);
void keystd_scan_clear (int scan);
void keystd_proc_mod_args (int arg, int pf);

typedef struct keystd_t
   {
    int key_mod;
    int lockkey_fix;
   }keystd_t;

#endif  /* HEADER_KEYSTD_H */
