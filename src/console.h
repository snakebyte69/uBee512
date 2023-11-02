/* CONSOLE Header */

#ifndef HEADER_CONSOLE_H
#define HEADER_CONSOLE_H

#ifndef MINGW
 #ifdef __linux__
  #define TERMIO_S termio
 #else
  // for FreeBSD and MAC OSX
  #define TCSETA TIOCSETA
  #define TCGETA TIOCGETA
  #define TERMIO_S termios
 #endif
#endif

// keep this value fairly large (~10K)
#define XPRINT_BUFSIZE 10000

// stream devices used by xprintf and xputchar
#define CONSOLE_NONE   0x00000000
#define CONSOLE_OSD    0x00000001
#define CONSOLE_STDOUT 0x00000002
#define CONSOLE_BOTH   0x00000003
#define CONSOLE_DEBUG  0x00000100
#define CONSOLE_ALL    0x000000ff

int console_init (void);
int console_deinit (void);
int console_reset (void);

int xputchar (int c);

extern int xprintf (char * fmt, ...)
                __attribute__ ((format (printf, 1, 2)));

void xflush (void);

#ifndef MINGW
int getch (void);
#endif
int xgetch (void);
void console_set_keydevice (int d);
void console_set_devices (int d);
void console_add_device (int d);
int console_get_devices (void);
void console_get_devices_name (char *device);
void console_debug_message (char *mesg);
void console_debug_stream (int activate);
void console_proc_output_args (int arg, int pf);
void console_command (int cmd);
void console_exit_while_debugger_runs (void);
void console_resume_after_debugger_run (void);

typedef struct console_t
   {
    int streams;
    int key_device;
    int xstdin;
    int force_stdout;
    int console_stdout;
    int debug_only;
    FILE *debug;
    int end_by_debugger;
    int resume_by_debugger;
   }console_t;

extern console_t console;

#endif /* HEADER_CONSOLE_H */
