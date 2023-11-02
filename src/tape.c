//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                                Tape module                                 *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// This module is used to emulate the tape out and in circuit.
//
// tape out: data will be placed into a WAV file specified on the command line
//           with the --tapeo=filename option.
//
//  tape in: data will be read from a WAV file specified on the command line
//           with the --tapei=filename option.
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
// v5.4.0 - 7 September 2011, uBee
// - Removed 'tape_i_center' and replaced with code to test 2 levels of
//   'tape_i_high' and 'tape_i_low', this provides a hysteresis type action
//   instead of a single level which really was pretty useless.
//
// v5.3.0 - 3 April 2011, uBee
// - Changes made to tape_command() to only set tape status and report 'Tape
//   rewind.' if a tape input file is open.
//
// v5.1.0 - Septemper 2010, uBee
// - Fixed a problem where emulator crashes on a 'Power Cycle' when a tape
//   input file was specified.  tape_i_close() now sets 'tape.tape_i_file =
//   NULL' after file is closed.
//
// v4.7.0 - 29 June 2010, uBee
// - Changes made to fread() function to use the result as some compilers
//   report warning: declared with attribute warn_unused_result.
//
// v4.1.0 - 14 June 2009, uBee
// - Fixed a couple of Big Endian issues for wav_o.chunk_size and error
//   checking on input wav file for wav_i.audio_format.  Tape input and
//   output now works on PPC HW.
//
// v4.0.0 - 9 June 2009, uBee
// - Changes made to incorporate a Z80 API.  Specific Z80 code used here
//   has been replaced with code usable by any Z80 emulator.
// - Changed all Endian functions to use macros in function.h as these make
//   the intention clearer.
//
// v3.1.0 - 22 April 2009, uBee
// - Removed all occurrences of console_output() function calls.
// - Added a 'detect' member to the tape_t structure. If non zero the value
//   will be used as the tape_i_center value for input level detection.
// - Changed 'big_endian_u32(tape_o_size)' to 'little_endian_u32(tape_o_size),
//   had not caused an issue as the swap_endian() function did nothing!
// - Change 'level = 128 - tape.olevel' to 'level = 127 - tape.olevel' as
//   a percentage value of 127 is now used.
// - Changed tape_i_center to be a unsigned 32 bit integer type.
// - Disable checking of 'data' in WAV file header as some files have this
//   located at offset 38 instead of 36.  Another WAV format?
// - Moved commands over from function.c to new tape_command() function.
// - Added tape_check(), tape_o_close(), tape_i_close(), tape_o_open() and
//   tape_i_open() functions and reorganised opening and closing of tape files.
// - Changed all printf() calls to use a local xprintf() function.
//
// v2.8.0 - 31 August 2008, uBee
// - Changes to the way gui_status_update() is called.
// - Changed documentation references from XTAL to CPU clock.
//
// v2.7.0 - 2 June 2008, uBee
// - New open_file() function is now used to create tape files.
// - Added structure emu_t and moved variables into it.
//
// v2.6.0 - 12 May 2008, uBee
// - Tape variables have been placed into structure tape_t.
//
// v2.0.0 - 18 October 2007, uBee
// - Added error message if failed to open tape input file.
//
// v1.4.0 - 26 September 2007, uBee
// - Added changes to error reporting.
//
// v1.3.0 - 26 August 2007, uBee
// - Added tape volume level as a user settable option.
//
// v1.2.0 - 20 August 2007, uBee
// - Created a new file and implement the tape emulation.
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "z80api.h"
#include "support.h"
#include "tape.h"
#include "gui.h"
#include "ubee512.h"

#include "macros.h"

//==============================================================================
// structures and variables
//==============================================================================
tape_t tape =
{
 .orate = TAPE_SAMPLE_FREQ,
 .olevel = TAPE_VOLUME
};

static wav_t wav_i;

static uint64_t cycles_i_start;

static int tape_i_cpuclock;
static int tape_i_divval;
static int tape_i_off_cmp;
static int tape_i_datasize;
static int tape_i_channels;
static int tape_i_lastlevel;
static uint32_t tape_i_high;
static uint32_t tape_i_low;

static wav_t wav_o;
static uint64_t cycles_o_now;
static uint64_t cycles_o_before;
static uint64_t cycles_o_elapsed;
static int tape_o_now;
static int tape_o_before;
static int tape_o_divval;
static int tape_o_off_cmp;
static int tape_o_size;

static uint8_t buf[1024];

extern char userhome_tapepath[];

extern emu_t emu;

//==============================================================================
// Tape Initialise.
//
// Open the tape input file and read in the WAV file header.
//
//   pass: void
// return: int                          0 if success, -1 if error
//==============================================================================
int tape_init (void)
{
 if (tape.tapei[0])
    return tape_i_open(tape.tapei, 1);
 return 0;
}

//==============================================================================
// Tape de-initialise.
//
// For tape out the WAV file header is filled in and the file closed.
// For tape in the file is closed.
//
//   pass: void
// return: int                          0
//==============================================================================
int tape_deinit (void)
{
 tape_i_close();
 tape_o_close();
 return 0;
}

//==============================================================================
// Tape reset.
//
//   pass: void
// return: int                          0
//==============================================================================
int tape_reset (void)
{
 tape_o_close();

 if (tape.tapei[0])
    return tape_i_open(tape.tapei, 1);
 return 0;
}

//==============================================================================
// Tape name check.
//
// Ensure that the tape.tapei and tape.tapeo file names will not be the same.
//
//   pass: char *s1                     file name of tape path #1
//         char *s2                     file name of tape path #2
// return: int                          0 if different, -1 if same
//==============================================================================
int tape_check (char *s1, char *s2)
{
 if (strcmp(s1, s2) == 0)
    {
     xprintf("ubee512: tape in and out can not be the same file\n");
     return -1;
    }
 return 0;
}

//==============================================================================
// Tape output close.
//
// Close the tape output file.
//
//   pass: void
// return: void
//==============================================================================
void tape_o_close (void)
{
 if (tape.tape_o_file == NULL)
    return;

 memcpy(&wav_o.chunk_id, "RIFF", 4);
 memcpy(&wav_o.format, "WAVE", 4);

 memcpy(&wav_o.sub_chunk1_id, "fmt ", 4);
 wav_o.sub_chunk1_size = host_to_leu32(16);
 wav_o.audio_format = host_to_leu16(1); // PCM = 1 (i.e. Linear quantization)
 wav_o.num_channels = host_to_leu16(1); // channels (mono)
 wav_o.sample_rate = host_to_leu32(tape.orate); // sample rate
 wav_o.byte_rate = host_to_leu32(tape.orate); // byte rate
 wav_o.block_align = host_to_leu16(1);
 wav_o.bits_per_sample = host_to_leu16(8); // bits per sample

 memcpy(&wav_o.sub_chunk2_id, "data", 4);
 wav_o.sub_chunk2_size = host_to_leu32(tape_o_size);
 wav_o.chunk_size = host_to_leu32(36 + tape_o_size);

 fseek(tape.tape_o_file, 0, SEEK_SET);
 fwrite(&wav_o, sizeof(wav_o), 1, tape.tape_o_file); // write the wave file header
 fclose(tape.tape_o_file);

 tape.tape_o_file = NULL;
}

//==============================================================================
// Tape input close.
//
// Close the tape input file.
//
//   pass: void
// return: void
//==============================================================================
void tape_i_close (void)
{
 if (tape.tape_i_file != NULL)
    {
     fclose(tape.tape_i_file);
     tape.tape_i_file = NULL;
    }
}

//==============================================================================
// Tape output open.
//
// Open a tape file for output.
//
//   pass: char *s                      tape file name path
//         int action                   0 saves file name only, 1 creates file
// return: int                          0 if success, -1 if error
//==============================================================================
int tape_o_open (char *s, int action)
{
 char filepath[SSIZE1];

 tape_o_close();
 strcpy(tape.tapeo, s);

 if (action == 0)
    return 0;

 tape_o_size = 0;
 tape.tape_o_file = open_file(tape.tapeo, userhome_tapepath, filepath, "wb");

 if (tape.tape_o_file == NULL)
    {
     xprintf("tape_o_open: Unable to create tape output file %s\n", filepath);
     tape.tapeo[0] = 0;
     return -1;
    }

 gui_status_update();
 memset(&wav_o, 0, sizeof(wav_o));
 fwrite(&wav_o, sizeof(wav_o), 1, tape.tape_o_file);  // write an empty wave header

 return 0;
}

//==============================================================================
// Tape input open.
//
// Open a tape file for input. If a tape file is already open it will be
// closed before opening the new file.
//
//   pass: char *s                      tape file name path
//         int action                   0 saves file name only, 1 opens file
// return: int                          0 if success, -1 if error
//==============================================================================
int tape_i_open (char *s, int action)
{
 int error;
 int half;
 int x;
 char temp[5];
 char filepath[SSIZE1];

 strcpy(tape.tapei, s);
 if (action == 0)
    return 0;

 tape_i_close();

 tape.tape_i_file = open_file(tape.tapei, userhome_tapepath, filepath, "rb");

 if (tape.tape_i_file == NULL)
    {
     xprintf("tape_i_open: Unable to open tape input file: %s\n", tape.tapei);
     tape.tapei[0] = 0;
     tape.in_status = 0;
     gui_status_update();
     return -1;
    }

 if (fread(&wav_i, sizeof(wav_i), 1, tape.tape_i_file) != 1)
    {
     fclose(tape.tape_i_file);
     xprintf("tape_i_open: Unable to read from tape input file: %s\n", tape.tapei);
     return -1;
    }

 tape_i_divval = tape_i_cpuclock / leu32_to_host(wav_i.sample_rate);
 tape_i_off_cmp = leu32_to_host(wav_i.sample_rate);
 tape_i_datasize = leu16_to_host(wav_i.bits_per_sample) / 8;
 tape_i_channels = leu16_to_host(wav_i.num_channels);

 // set the tape detection levels, this assumes signed data at level 0 will
 // have a value that is 1/2 the maxium value for the data size.  i.e.  8
 // bit data = 128, 16 bit data = 32768
 half = (0x01 << ((tape_i_datasize * 8) - 1));

 if (tape.detect > 0.0)
    {
     x = (int)(half * (tape.detect / 100.0));
     tape_i_high = half + x;
     tape_i_low = half - x;
    } 
 else
    {
     tape_i_high = half;
     tape_i_low = half;
    }

 cycles_i_start = 0;
 tape.in_status = 0;
 gui_status_update();

#if 1
 xprintf("tape_i_divval=%d  tape_i_off_cmp=%d  tape_i_datasize=%d tape_i_channels=%d tape_i_high=%d tape_i_low=%d\n",
 tape_i_divval,  tape_i_off_cmp,  tape_i_datasize, tape_i_channels, tape_i_high, tape_i_low);
#endif

 error = 0;
 temp[4] = 0;

 memcpy(temp, wav_i.chunk_id, 4);
 error |= strcmp(temp, "RIFF");

 memcpy(temp, wav_i.format, 4);
 error |= strcmp(temp, "WAVE");

 memcpy(temp, wav_i.sub_chunk1_id, 4);
 error |= strcmp(temp, "fmt ");

#if 0
 memcpy(temp, wav_i.sub_chunk2_id, 4);
 error |= strcmp(temp, "data");
#endif

 error |= (leu16_to_host(wav_i.audio_format) != 1);

 error |= ((tape_i_datasize == 0) | (tape_i_datasize > 4));

 if (error)
    {
     fclose(tape.tape_i_file);
     xprintf("tape_i_open: Unsupported wave file format\n");
     return -1;
    }

 return 0;
}

//==============================================================================
// Tape read.
//
// Only call this if the tape input file is known to be open and
// tape.in_status > 0
//
// Note: 8 bit sound data in RIFF files uses unsigned data. WAV files that
// have data < 128 are 0 bits, otherwise 1.  Calculated (digital) WAV files
// that use values of 0 and 255 will work perfectly.  Analogue WAV files
// recorded must have a high enough level for 1 bits to be detected. This
// means that 16 bit data would be more preferrable for recording from
// analogue sources as the data is signed.
//
//   pass: void
// return: int                          tape input bit value
//==============================================================================


int tape_r (void)
{
 int count;
 int filepos;
 uint32_t data;         // must be 32 bit
 int bytesread;


 uint64_t cycles_i_elapsed;
 uint64_t cycles_i_now;

 // pressing the Tape reset key will cause tape.in_status to be set to 2
 if (tape.in_status == 2)
    {
     tape_i_open(tape.tapei, 1);
     tape.in_status = 1;
     gui_status_update();
    }

 // calculate the elapsed time since the last read of the tape
 // input,  this will be used to determine the amount to step into the WAV file
 // to pull the next sample out.
 if (cycles_i_start == 0)
    {
     fseek(tape.tape_i_file, sizeof(wav_i), SEEK_SET);
     if (! ferror(tape.tape_i_file))
        {
         data = 0;  // clear all bytes in data
         bytesread = fread(&data, tape_i_datasize, 1, tape.tape_i_file);  // read in one data sample
         data = leu32_to_host(data);
         if (! bytesread)
            {
             tape_i_open(tape.tapei, 1);
             return 0;
            }
         cycles_i_start = z80api_get_tstates();
        }
    }
 else
    {
     cycles_i_now = z80api_get_tstates();
     cycles_i_elapsed = (cycles_i_now - cycles_i_start);

     // determine the sample offset we need
     count = (cycles_i_elapsed / tape_i_divval);

     filepos = sizeof(wav_i) + tape_i_datasize * tape_i_channels * count;
     fseek(tape.tape_i_file, filepos, SEEK_SET);  // seek from start
     if (! ferror(tape.tape_i_file))
        {
         data = 0;  // clear all bytes in data
         bytesread = fread(&data, tape_i_datasize, 1, tape.tape_i_file);  // read in one data sample
         data = leu32_to_host(data);
         if (! bytesread)
            {
             tape_i_open(tape.tapei, 1);
             return 0;
            }
        }
    }

#if 0
 static int debugc = 0;

 xprintf("%d %d - ", count, data);
 if (debugc++ == 10)
   {
    xprintf("\n");
    debugc = 0;
   }
#endif

 // return the state of the level, if in between the high and low values
 // return the last value detected.  The high and low levels simulates
 // hysteresis and is programmable.
 if (data <= tape_i_low)
    {
     tape_i_lastlevel = 0;
     return B8(00000000);
    } 
 if (data >= tape_i_high)
    {
     tape_i_lastlevel = 1;
     return B8(00000001);
    }
 return tape_i_lastlevel;
}

//==============================================================================
// Tape write.
//
// This should only be called if the 'tape.tapeo' variable holds a string
// value.
//
//   pass: int data
// return: void
//==============================================================================
void tape_w (int data)
{
 int count;
 int level;
 int x;

 // mask of the tape out bit
 tape_o_now = data & B8(00000010);

 // Only do something if the tape state changes.
 if (tape_o_now != tape_o_before)
    {
//   xprintf("tape_w: data=0x%02X\n", data);
     tape_o_before = tape_o_now;

     // Create the output wave file now if not already done so.  We do it here
     // as it gives the user a chance to exit and not overwrite another file.
     if ((tape.tapeo[0]) && (tape.tape_o_file == NULL))
        tape_o_open(tape.tapeo, 1);

     // if no tape output file exists then do nothing and exit
     if (! tape.tape_o_file)
        return;

     // calculate the elapsed time since the last change of the tape
     // output,  this will be used to determine the amount of data to be
     // written out.
     cycles_o_now = z80api_get_tstates();
     cycles_o_elapsed = cycles_o_now - cycles_o_before;
     cycles_o_before = cycles_o_now;

     // Determine the number of repeat values to be written out to the wave file.
     count = cycles_o_elapsed / tape_o_divval;

     // We don't want a massive file so if long period cut it back shorter
     // This is fine for normal DGOS tape saves but not good for other things that
     // use the tape port.  Will need an option to overide the behaviour.
     if (count > tape_o_off_cmp)
        {
         count = tape_o_off_cmp * 50;   // put out 5 seconds of silence
         level = 128;                   // silence
        }
     else

     // get the actual fill value to be placed into the buffer.  The value used is
     // the tape level value on the previous occasion but it probably makes
     // no difference anyway.
        {
         if (tape_o_before)
            level = 128 + tape.olevel;
         else
            level = 127 - tape.olevel;
        }

     while (count)
        {
         if (count >= sizeof(buf))
            x = sizeof(buf);
         else
            x = count;

         memset(buf, level, x);
         fwrite(buf, x, 1, tape.tape_o_file);
         count -= x;
         tape_o_size += x;
        }
    }
}

//==============================================================================
// Tape output configuration.
//
// This determines the divider value to be used when creating the wave file
// data and data to placed into the wave header.
//
// The sample rate is the only parameter that can be changed for creating
// wave files.
//
// The output wave file format:
// mono, 8 bit data (128 is the center point),
//
// The calcuations are always based on either a 3.375 or 2 MHz CPU clock as
// these were the original speeds.  The correct application software is
// required for each CPU clock frequency.  i.e. Old 2 MHz requires old BASIC.
//
//   pass: int cpuclock         CPU clock frequency in Hz.
// return: void
//==============================================================================
void tape_config_out (int cpuclock)
{
 if ((cpuclock == 3375000) || (cpuclock == 2000000))
    tape_o_divval = cpuclock / tape.orate;
 else
    tape_o_divval = 3375000 / tape.orate;
 tape_o_off_cmp = tape.orate / 10;
}

//==============================================================================
// Tape input configuration.
//
// This saves the cpuclock value to be used later when loading a wave file
//
// The calcuations are always based on either a 3.375 or 2 MHz CPU cloack as
// these were the original speeds.  The correct application software is
// required for each CPU clock frequency.  i.e. Old 2 MHz requires old BASIC.
//
//   pass: int cpuclock         CPU clock frequency in Hz.
// return: void
//==============================================================================
void tape_config_in (int cpuclock)
{
 if ((cpuclock == 3375000) || (cpuclock == 2000000))
    tape_i_cpuclock = cpuclock;
 else
    tape_i_cpuclock = 3375000;
}

//==============================================================================
// Tape commands.
//
//   pass: int cmd                      tape command
// return: void
//==============================================================================
void tape_command (int cmd)
{
 switch (cmd)
    {
     case EMU_CMD_TAPEREW :
        if (tape.tape_i_file != NULL)
           {
            tape.in_status = 2;
            xprintf("Tape rewind.\n");
           }
        break;
    }
}
