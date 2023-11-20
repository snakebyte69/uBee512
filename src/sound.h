/* Speaker sound Header */

#ifndef HEADER_SOUND_H
#define HEADER_SOUND_H

int speaker_init (void);
int speaker_deinit (void);
int speaker_reset (void);
void speaker_w (uint8_t data);

#endif     /* HEADER_SOUND_H */
