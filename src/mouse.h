/* mouse Header */

#ifndef HEADER_MOUSE_H
#define HEADER_MOUSE_H

#include "ubee512.h"

#define MOUSE_OFF       0
#define MOUSE_ON        1

#define MOUSE_HOST      0
#define MOUSE_MSOFT     1
#define MOUSE_MOUSESYST 2

int mouse_init (void);
int mouse_deinit (void);
int mouse_reset (void);
void mouse_configure (int x);
int mouse_r (void);
void mouse_sync_clear (void);
void mouse_mousebuttondown_event (void);
void mouse_mousebuttonup_event (void);
void mouse_mousemotion_event (void);
void mouse_command (int cmd);

typedef struct mouse_t
   {
    int button_l;
    int button_r;
    int x;
    int y;
    int protocol;
    int active;
    int host_in_use;
   }mouse_t;

#endif  /* HEADER_MOUSE_H */
