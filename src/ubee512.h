/* ubee512 Header */

#ifndef HEADER_UBEE512_H
#define HEADER_UBEE512_H

#include <SDL.h>
#include "console.h"
#include "log.h"

#ifdef MINGW
#define SLASHCHAR '\\'
#define SLASHCHAR_STR "\\"
#define SLASHCHAR_OTHER '/'
#define DIR_CONF "\\"
#define DIR_DOCS "\\doc\\"
#define DIR_DISKS "\\disks\\"
#define DIR_ROMS "\\roms\\"
#define DIR_TAPES "\\tapes\\"
#define DIR_PRINTER "\\printer\\"
#define DIR_IMAGES "\\images\\"
#define DIR_RTC "\\rtc\\"
#define DIR_SRAM "\\sram\\"
#define DIR_TOOLS "\\tools\\"
#define DIR_FILES "\\files\\"
#define DIR_SHARE "\\share\\"
#else
#define SLASHCHAR '/'
#define SLASHCHAR_STR "/"
#define SLASHCHAR_OTHER '\\'
#define DIR_CONF "/"
#define DIR_DOCS "/doc/"
#define DIR_DISKS "/disks/"
#define DIR_ROMS "/roms/"
#define DIR_TAPES "/tapes/"
#define DIR_PRINTER "/printer/"
#define DIR_IMAGES "/images/"
#define DIR_RTC "/rtc/"
#define DIR_SRAM "/sram/"
#define DIR_TOOLS "/tools/"
#define DIR_FILES "/files/"
#define DIR_SHARE "/share/"
#endif

#define SSIZE1 512

#define EMU_SYSTEM_UNIX    0x00000001
#define EMU_SYSTEM_DARWIN  0x00000002
#define EMU_SYSTEM_WINDOWS 0x00000004

#define EMU_VOLUME_CHANGE  1.0
#define EMU_REPEAT1        500
#define EMU_REPEAT2        50

#define EMU_EMU_CONTEXT    0
#define EMU_OSD_CONTEXT    1

#define EMU_RST_RESET_CON    1
#define EMU_RST_RESET_NOW    2
#define EMU_RST_POWERCYC_CON 3
#define EMU_RST_POWERCYC_NOW 4

#define EMU_INIT          0x00000001
#define EMU_INIT_POWERCYC 0x00000002
#define EMU_RST1          0x00000100
#define EMU_RST2          0x00000200

//==============================================================================
// Model property definitions
//==============================================================================
#define MODROM  1
#define MODCOL1 1
#define MODCOL2 2
#define MODSPD  1

#define MODPB7_PUP 0
#define MODPB7_VS 1
#define MODPB7_RTC 2
#define MODPB7_NET 3

#define MODRTC 1

#define MODFDC_AT 1             // Applied Technology floppy controller
#define MODFDC_DD 2             // DreamDisk floppy controller

//==============================================================================
// Compile time options,  these are normally only changed for testing purposes.
//==============================================================================
#ifdef MINGW
#define FRAMERATE 50            // target frame rate (frames per second) wanted
#define EMU_MAXLAG_MS 250       // maximum time (in mS) Z80 CPU is allowed lag
#define EMU_Z80_DIVIDER 25      // Z80 blocks executed in 1 Z80 emulation frame
#else
#define FRAMERATE 50            // target frame rate (frames per second) wanted
#define EMU_MAXLAG_MS 250       // maximum time (in mS) Z80 CPU is allowed lag
#define EMU_Z80_DIVIDER 25      // Z80 blocks executed in 1 Z80 emulation frame
#endif

// default host conversion of path slash characters
#define EMU_SLASHCONV 1

// paths to shared directories.
#define PATH_SHARED_IMAGES "/usr/local/share/ubee512/images/"
#define PATH_SHARED_DOCS "/usr/local/share/ubee512/doc/"
#define PATH_SHARED_TOOLS "/usr/local/share/ubee512/tools/"
#define PATH_SHARED_DISKS "/usr/local/share/ubee512/disks/"
#define PATH_SHARED_CONFIG "/usr/local/share/ubee512/config/"

// alias configuration files
#define ALIASES_ROMS "roms.alias"
#define ALIASES_DISKS "disks.alias"

// default fall-back boot image
#define BOOT_IMAGE "boot.dsk"

// default model to emulate
#define MOD_DEFAULT MOD_P512K

// define this for the default CPU clock frequency (in MHz)
#define CPU_CLOCK_FREQ 3.375

// define where the logged data will go
#define LOGFILE "ubee512_log.txt"

// define the following debug options to report on ports on stdout
//#define use_debug_disk_read_abort

//#define use_debug_getkeystate_shift
//#define use_debug_getkeystate_lower

// define the following options to be reported to stdout
#define use_info_disk_open
//#define use_info_disk_read

// define this to use faster memory reads but slower MMU configuring. The
// reverse is true if not defined (default is to use it)
#define MEMMAP_HANDLER_1

//==============================================================================
// emulator commands activated from the keyboard or joystick
//==============================================================================
enum
{
 EMU_CMD_DUMP=0,
 EMU_CMD_DUMP_N1,
 EMU_CMD_DUMP_N2,
 EMU_CMD_DUMP_B1,
 EMU_CMD_DUMP_B2,
 EMU_CMD_DUMP_REP,
 EMU_CMD_DUMPREGS,
 EMU_CMD_DBGOFF,
 EMU_CMD_DBGON,
 EMU_CMD_DBGTRACE,
 EMU_CMD_DBGSTEP01,
 EMU_CMD_DBGSTEP10,
 EMU_CMD_DBGSTEP20,
 EMU_CMD_DASML,
 EMU_CMD_PAUSE,

 EMU_CMD_FULLSCR,
 EMU_CMD_TAPEREW,
 EMU_CMD_JOYSTICK,
 EMU_CMD_MUTE,
 EMU_CMD_VOLUMEI,
 EMU_CMD_VOLUMED,
 EMU_CMD_SCREENI,
 EMU_CMD_SCREEND,
 EMU_CMD_VIDSIZE1,
 EMU_CMD_GL_FILTER,
 EMU_CMD_MWHEEL,
 EMU_CMD_MOUSE,
 EMU_CMD_CONSOLE,
 EMU_CMD_END_LIST
};

//==============================================================================
// hardware definitions (up to 32 hardware bits)
//==============================================================================
#define HW_WD2793          (1 << 0)

int set_account_paths (void);
void write_id_file (void);
void event_handler (void);
void set_clock_speed (float clock, int blocks, int frate);
void turbo_reset (void);

enum
{
 MOD_256TC,
 MOD_P1024K,
 MOD_1024K,
 MOD_P512K,
 MOD_512K,
 MOD_P128K,
 MOD_128K,
 MOD_P256K,
 MOD_256K,
 MOD_P64K,
 MOD_64K,
 MOD_56K,
 MOD_TTERM,
 MOD_PPC85,
 MOD_PC85B,
 MOD_PC85,
 MOD_PC,
 MOD_IC,
 MOD_2MHZ,
 MOD_2MHZDD,                 // dreamdisk 2 MHz
 MOD_DD,                     // dreamdisk 3.375 MHz
 MOD_SCF,
 MOD_PCF,
 MOD_TOTAL
};

typedef struct emu_t
{
 int done;
 int runmode;
 int model;
 int turbo;
 uint64_t z80_cycles;
 int z80_blocks;                // working number of Z80 blocks
 int z80_ratio;         // current z80 execution ratio (default = 1)
 int z80_divider;
 int new_pc;
 int maxcpulag;
 int cpuclock;
 int framerate;
 int keyesc;
 int keym;
 int display_context;
 int osd_focus;
 int install_files_req;
 int paused;
 int quit;
 int reset;
 int slashconv;
 int alias_roms;
 int alias_disks;
 int exit_check;
 int exit_warning;
 int win32_lock_key_fix;
 int x11_lock_key_fix;
 int cmd_repeat1;
 int cmd_repeat2;
 int home_account_set;
 int secs_init;
 int secs_run;
 int secs_exit;
 int century;
 int hardware;
 int verbose;
 int cfmode;
 int roms_create_md5;
 int roms_md5_file;
 int port50h;
 int port51h;
 int port58h;
 int port58h_use;
 int proc_delay_type;
 int sdl_version;
 int system;
 float cpuclock_def;
 char sysname[SSIZE1];
 char prefix_path[SSIZE1];
 SDL_Event event;
}emu_t;

// don't change order or insert without changing model table
typedef struct model_t
{
 int alphap;
 int tckeys;
 int rom;
 int ide;
 int hdd;
 int bootaddr;
 int fdc;
 int ram;
 int pcg;
 int vdu;
 int colour;
 int hwflash;
 int halfint;
 int lpen;
 int speed;
 int piob7;
 int rtc;
 float cpuclock;
// FIXME: this is at the end of the structure so that it is initialised to zero by default
 int sn76489an;
}model_t;

typedef struct model_custom_t
{
 char charrom[SSIZE1];
 char rom1[SSIZE1];
 char rom2[SSIZE1];
 char rom3[SSIZE1];
 char rom256k[SSIZE1];
 char pak_a[8][SSIZE1];
 char pak_b[8][SSIZE1];
 char netrom[SSIZE1];
 char basica[SSIZE1];
 char basicb[SSIZE1];
 char basicc[SSIZE1];
 char basicd[SSIZE1];
 char colprom[SSIZE1];
 char systname[SSIZE1];
 int pakram[8];
 int basram;
 int netram;
 int paksel;
}model_custom_t;

typedef struct messages_t
{
 int opengl_no;
}messages_t;

//==============================================================================
// Debugging related
//==============================================================================
typedef struct modio_t
{
 FILE *log;
 int level;
 int raminit;

 int beetalker;
 int beethoven;
 int clock;
 int compumuse;
 int crtc;
 int dac;
 int fdc;
 int fdc_wtd;
 int fdc_wth;
 int func;
 int ide;
 int hdd;
 int joystick;
 int keystd;
 int keytc;
 int mem;
 int options;
 int roms;
 int pioa;
 int piob;
 int piocont;
 int rtc;
 int tapfile;
 int ubee512;
 int vdu;
 int vdumem;
 int video;
 int z80;
 int sn76489an;
}modio_t;

typedef struct regdump_t
{
 int clock;
 int crtc;
 int fdc;
 int keytc;
 int mem;
 int roms;
 int pio;
 int rtc;
 int vdu;
 int z80;
}regdump_t;

typedef struct init_func_t
{
 int (*memory_call_init)(void);
 int (*memory_call_deinit)(void);
 int (*memory_call_reset)(void);
 int flags;
 char func_name[20];
}init_func_t;

#endif /* HEADER_UBEE512_H */
