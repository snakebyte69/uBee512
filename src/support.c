//******************************************************************************
//*                                  uBee512                                   *
//*       An emulator for the Microbee Z80 ROM, FDD and HDD based models       *
//*                                                                            *
//*                              Support module                                *
//*                                                                            *
//*                       Copyright (C) 2007-2016 uBee                         *
//******************************************************************************
//
// General support functions that don't belong to any particular module.
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
// v5.7.0 - 1 February 2014, uBee
// - Added get_psh() function.
// - Added get_parameter_count() to find count of parameters in string.
// - Added string_case_search() for upper and lower case search.
// - Added string_struct_search() to find a string in a structure.
// - added sup_strncpy() function to safely copy strings.
// - Changes to get_next_parameter() to move past leading white space.
// - Fix return value (NULL not 0) on W32 strcasestr() function.
//
// v5.3.0 - 11 April 2011, uBee
// - Moved get_mwb_version() from quickload.c to this file as is now also
//   used by tapfile.c.
// - Recoded get_mwb_version() to return ROM and disk model Basic models as
//   integer numbers 500-699and an optional string.  Each Basic has it's own
//   entry check location.
// - Changes made to get_integer_value() so that a '%' value can be passed
//   in the string and be ignored.
//
// v5.0.0 - 13 July 2010, K Duckmanton
// - Removed all references to the 'sound' global variable and replaced them
//   with references to the 'audio' global instead.
//
// v4.7.0 - 17 June 2010, K Duckmanton
// - Fixed a problem with casts in time_get_ms() for non win32 systems.
//
// v4.6.0 - 13 May 2010, uBee
// - Added get_next_parameter(), string_prefix_get(), tolower_string(),
//   array_search(), get_date_and_time(), i2bx() and i2b() functions.
// - Added get_integer_value() function to convert a string to an integer.
// - Added get_float_value() function to convert a string to an float.
// - Improvements made to find_file_alias() function.
// - Modified the file_readline() function so that trailing '\' line append
//   characters can be used.
//
// v4.5.0 - 2 April 2010, uBee
// - Created new file for various support functions. The functions from
//   function.c not matching function_*() have been moved to this module.
//==============================================================================

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>

#ifdef MINGW
#include <windows.h>
#else
#include <sys/types.h>          // various type definitions, like pid_t
#include <sys/time.h>
#endif

#include "support.h"
#include "ubee512.h"
#include "function.h"
#include "z80debug.h"
#include "gui.h"
#include "crtc.h"
#include "rtc.h"
#include "roms.h"
#include "joystick.h"
#include "console.h"
#include "z80.h"
#include "z80api.h"
#include "md5.h"
#include "memmap.h"

//==============================================================================
// structures and variables
//==============================================================================
#ifdef MINGW
#else
 static struct timeval tod;
#endif

extern char userhome_confpath[];
extern char userhome_romspath[];
extern char userhome_diskpath[];

extern emu_t emu;
extern model_t modelx;
extern modio_t modio;

//==============================================================================
// String copy with length checking.
//
//   pass: char *d                      destination string
//         const char *s                source string
//         int size                     size of the destination string
// return: char *                       pointer to destination string
//==============================================================================
char *sup_strncpy (char *d, const char *s, int size)
{
 strncpy(d, s, size - 1);
 d[size-1] = 0;
 return d;
}

//==============================================================================
// Get the current clock time in seconds
//
//   pass: void
// return: int                          number of seconds
//==============================================================================
int time_get_secs (void)
{
 return time_get_ms() / 1000;
}

//==============================================================================
// Get the current clock time in milliseconds
//
//   pass: void
// return: uint64_t                     number of milliseconds
//==============================================================================
uint64_t time_get_ms (void)
{
#ifdef MINGW
 return (uint64_t)clock();
#else
 gettimeofday(&tod, NULL);
 return (((uint64_t)tod.tv_sec * 1000) + ((uint64_t)tod.tv_usec / 1000));
#endif
}

//==============================================================================
// Time delay in milliseconds. Gives up host CPU time to other applications.
//
//   pass: int ms                       number of milliseconds
// return: void
//==============================================================================
void time_delay_ms (int ms)
{
 SDL_Delay(ms);
}

//==============================================================================
// Time wait in milliseconds. Does NOT give up host CPU time!
//
//   pass: int ms                       number of milliseconds
// return: void
//==============================================================================
void time_wait_ms (int ms)
{
 int t = time_get_ms() + ms;
 while (time_get_ms() < t)
    ;
}

//==============================================================================
// Get date and time.
//
//   pass: char *s                      string to contain date and time
// return: void
//==============================================================================
void get_date_and_time (char *s)
{
 typedef struct tm tm_t;

 time_t result;
 tm_t resultp;

 time(&result);
#ifdef MINGW
 memcpy(&resultp, localtime(&result), sizeof(resultp));
#else
 localtime_r(&result, &resultp);
#endif
 if (resultp.tm_sec > 59)               // 2 second leap-seconds ignore
    resultp.tm_sec = 59;

 sprintf(s, "%4d/%02d/%02d %02d:%02d:%02d", resultp.tm_year+1900, resultp.tm_mon+1,
 resultp.tm_mday, resultp.tm_hour, resultp.tm_min, resultp.tm_sec);
}

//==============================================================================
// Get the z80 map pointer and return the address masked with 0x7fff
//
//   pass: int *addr
// return: unsigned char *
//==============================================================================
uint8_t *get_z80mem_ptr_and_addr (int *addr)
{
 uint8_t *map = memmap_get_z80_ptr(*addr);
 *addr &= 0x7fff;
 return map;
}

//==============================================================================
// Convert string to an integer value with error checking.  A -1 value is
// returned if an error is found.
//
// The function uses the strtol() function with the BASE value set to 0 to
// use auto base detection.
//
// A '%' symbol may be used as the last charater in the string passed.  The
// calling process should determine how the value returned is to be
// interpretted by checking to see if the string value contained a '%' as
// the last character.
//
//   pass: char *s                      string to be converted
// return: int                          integer value
//==============================================================================
int get_integer_value (char *s)
{
 char *check;
 int i;

 if (*s == 0)  // if an empty string
    return -1;

 i = (int)strtol(s, &check, 0);

 if ((*check == '%') && (*(check + 1) == 0))
    return i;

 if (*check != 0)  // if not end of string then it's an error
    i = -1;

 return i;
}

//==============================================================================
// Convert string to a floating point value with error checking.  A -1.0
// value is returned if an error is found.
//
// The function uses the strtof() function which uses auto base detection.
//
//   pass: char *s                      string to be converted
// return: float                        float value
//==============================================================================
float get_float_value (char *s)
{
 char *check;
 float f;

 f = strtof(s, &check);

 if (*check != 0)  // if not end of string then it's an error
    f = -1.0;

 return f;
}

//==============================================================================
// Parse a string and return the number of paramters found.
//
//   pass: char *s                      string to be parsed
//         int delimiter                sub parameter delimiter
// return: int                          number of parameters found
//==============================================================================
int get_parameter_count (char *s, int delimiter)
{
 int c = 0;

 if (s == NULL)
    return 0;

 while (*s)
    {
     while ((*s) && (*s != delimiter))
        s++;
     c++;
     
     if (*s)
        s++;      // get past the delimiter
    }

 return c;
}

//==============================================================================
// Parse a string and return the next sub-parameter as a string and an
// integer value.  A pointer to the next parameter position or a NULL is
// returned if the end of string is reached.
//
//   pass: char *s                      string to be parsed
//         int delimiter                sub parameter delimiter
//         char *sps                    sub parameter string returned here
//         int spi                      sub parameter integer returned here
//         int maxlen                   maximum length allowed
// return: char *s                      pointer to next entry
//==============================================================================
char *get_next_parameter (char *s, int delimiter, char *sps, int *spi, int maxlen)
{
 int c = 0;
 char *sps_x = sps;

 if (s == NULL)
    {
     *spi = -1;
     *sps = 0;
     return NULL;
    }

 // get past any leading white space
 while ((*s) && (*s <= ' ') && (c++ < maxlen))
    s++;

 while ((*s) && (*s != delimiter) && (c++ < maxlen))
    *(sps++) = *(s++);
 *sps = 0;

 *spi = get_integer_value(sps_x);

 if (*s)
    {
     s++;      // get past the delimiter
     if (! *s) // check if delimiter was at end of the line
        return NULL;
     return s;
    }
 else
    return NULL;
}

//==============================================================================
// Return a physical shift value for a power of 2 value. Does the reverse of
// a left shift. i.e. 128 << 3 = 1024.
//
// To do reverse: 3 = disk_get_psh(1024)
//
//   pass: int value
// return: int                          physical shift value
//==============================================================================
int get_psh (int value)
{
 int psh = 0;
 value >>= 8;
 while (value)
    {
     value >>= 1;
     psh++;
    }
 return psh;
}

//==============================================================================
// Convert an unsigned 16 bit integer value to a binary string.
//
// The string length returned will be dependent on the number of 'bits'
// requested
//
//   pass: int value                    value to be converted
//         int bits                     number of bits wanted
//         char *s                      binary string (must be bits length + 1)
// return: char *                       pointer to s
//==============================================================================
char *i2bx (int value, int bits, char *s)
{
 char *s_ptr = s;
 int i;

 if ((value < 0) || (value > 0xffff))
    {
     strcpy(s, "ERROR");
     return s_ptr;
    }

 s += bits;  // start at the end of the string and work backwards
 *(s--) = 0;

 for (i = 0; i < bits; i++)
    {
     *(s--) = (value & 1) + '0';
     value >>= 1;
    }

 return s_ptr;
}

//==============================================================================
// Convert an unsigned 16 bit integer value to a binary string.
//
// The string returned will be 8 characters if the value passed is less than
// 256 or 16 characters if 256 or greater.
//
//   pass: int value                    value to be converted
//         char *s                      binary string (minimum of 17 characters)
// return: char *                       pointer to s
//==============================================================================
char *i2b (int value, char *s)
{
 return i2bx(value, ((value > 255) + 1) * 8, s);
}

//==============================================================================
// Copy a string to upper case.  Source and destination may be the same.
//
//   pass: char *dest                   destination string
//         char *src                    source string
// return: void
//==============================================================================
void toupper_string (char *dest, char *src)
{
 int i = 0;

 while (src[i])
    {
     dest[i] = toupper(src[i]);
     i++;
    }
 dest[i] = 0;
}

//==============================================================================
// Copy a string to lower case.  Source and destination may be the same.
//
//   pass: char *dest                   destination string
//         char *src                    source string
// return: void
//==============================================================================
void tolower_string (char *dest, char *src)
{
 int i = 0;

 while (src[i])
    {
     dest[i] = tolower(src[i]);
     i++;
    }
 dest[i] = 0;
}

//==============================================================================
// Read a text line from a file.
//
// - Leading spaces are removed.
// - Comment and empty lines are not returned.
// - Control characters and space characters are removed from the end of the
//   line.
// - Lines with trailing '\' line append characters are allowed.
// - An empty string is returned if an error occurs or the end of the
//   file is reached.
//
//   pass: FILE *fp
//         char *s
//         int size
// return: int                          string length
//==============================================================================
int file_readline (FILE *fp, char *s, int size)
{
 int l;
 int append = 0;
 int done = 0;
 int size_x = size;
 char temp_str[5000];
 char *ts;

 while (! done)
    {
     done = (fgets(temp_str, sizeof(temp_str)-1, fp) == NULL);

     if (! done)
        {
         ts = temp_str;

         // remove leading spaces
         while (*ts && (*ts <= ' '))
            ts++;

         // ignore commented out line unless it's an append
         if (((*ts != '#') && (*ts != ';')) || append)
            {
             // otherwise remove trailing control and space characters
             l = strlen(ts);
             while (l && (*(ts+(l-1)) <= ' '))
                l--;
             *(ts+l) = 0;

             // check for a line append character '\' (preceded by space character)
             if ((l >= 2) && (*(ts+(l-2)) == ' ') && (*(ts+(l-1)) == '\\'))
                {
                 l -= 2;
                 *(ts+l) = 0;
                }
             else
                {
                 if (*ts)  // not done yet if empty line
                    done = 1;
                }

             // append the ts string to s
             if (l)
                {
                 strncpy(s + append, ts, size);
                 s[size_x-1] = 0;
                 append += l;
                 size -= l;
                 if (size < 0)
                    size = 0;
                }
            }
        }
     else
        {
         s[0] = 0;
         return 0;
        }
    }

 return append;
}

//==============================================================================
// Copy a file from one location to another.
//
//   pass: char *dest                   destination file path and name
//         char *src                    source file path and name
// return: int                          0 if no error, else -1
//==============================================================================
int copy_file (char *dest, char *src)
{
 FILE *srcfp;
 FILE *destfp;
 char *buf[0x4000];
 int len;

 srcfp = fopen(src, "rb");
 if (srcfp == NULL)
    return -1;

 destfp = fopen(dest, "wb");
 if (destfp == NULL)
    {
     fclose(srcfp);             // close the source file
     return -1;
    }

 while (! feof(srcfp))
    {
     len = fread(buf, 1, sizeof(buf), srcfp);
     if (len)
        fwrite(buf, 1, len, destfp);
    }

 fclose(srcfp);
 fclose(destfp);

 return 0;
}

//==============================================================================
// Convert path slash characters to host format
//
//   pass: char *path           path to be converted
// return: void
//==============================================================================
void convert_slash (char *path)
{
 int i = 0;

// convert slash characters to the host's file format if enabled.
 if (emu.slashconv)
    {
     while (path[i])
        {
         if (path[i] == SLASHCHAR_OTHER)
            path[i] = SLASHCHAR;
         i++;
        }
    }
}

//==============================================================================
// Open, create or test a file and return the full path.
//
// Mode does not contain 'w'
// -------------------------
// When opening an existing file a test is done to see if the file path
// exists, if not then checks the default location.  If mode is empty ("")
// then only test if the file exists using "rb" as the mode.
//
// Modes:
// "r"   Open a text file for reading
// "a"   Append to a text file
// "rb"  Open a binary file for reading
// "ab"  Append to a binary file
// "r+"  Open a text file for read/write
// "a+"  Open a text file for read/write
// "rb+" Open a binary file for read/write
// "ab+" Open a binary file for read/write
//
// Mode contains 'w'
// -----------------
// If a file is to be created then the default path is prepended if no
// path slash characters are found.
//
// Modes:
// "w"   Create a text file for writing
// "wb"  Create a binary file for writing
// "w+"  Create a text file for read/write
// "wb+" Create a binary file for read/write
//
// If emu.slashconv is true then slash slash characters are converted to the
// host system format so either '\' or '/' may be used with the same
// results.
//
//   pass: char *path1          path to be tested
//         char *path2          alternate path prefix to be tried
//         char *path3          the full path filled in by this code
//         char *mode           how to open the file, or "" if test only
//
// return: *FILE                NULL if no path or file could not be created
//==============================================================================
FILE *open_file (const char *path1, const char *path2, char *path3, const char *mode)
{
 int uses_path1 = 0;
 char pathx[SSIZE1];
 FILE *fp;

 sup_strncpy(pathx, path1, sizeof(pathx));

 // convert slash characters to the host's file format if enabled.
 convert_slash(pathx);

 // if a '/', './' or '../' characters are found at the start then treat as
 // the path to be used, the default path (path2) will not be prepended in
 // that case.
 if ((strstr(pathx, "."SLASHCHAR_STR) == pathx) ||
    (strstr(pathx, ".."SLASHCHAR_STR) == pathx) ||
    (strstr(pathx, SLASHCHAR_STR) == pathx))
    uses_path1 = 1;

 // if Win32 then a ':' character will also be treated as a path to be used.
#ifdef MINGW
 if (strchr(pathx, ':'))
    uses_path1 = 1;
#endif

 strcpy(path3, pathx);

 // create file if mode 'w'
 if (strchr(mode, 'w'))
    {
     if (! uses_path1)
        snprintf(path3, SSIZE1, "%s%s", path2, pathx);
     fp = fopen(path3, mode);
     return fp;
    }

 // test if a file exists, if uses_path1 is true then only the current
 // directory will be checked.
 if (! *mode)
    {
     fp = fopen(path3, "rb");
     if (fp)
        {
         fclose(fp);
         return fp;
        }

     if (uses_path1)
        return fp;

     snprintf(path3, SSIZE1, "%s%s", path2, pathx);
     fp = fopen(path3, "rb");
     if (fp)
        fclose(fp);
     return fp;
    }

 // open an existing file, if uses_path1 is true then only the current
 // directory will be checked.
 fp = fopen(path3, mode);
 if (fp || uses_path1)
    return fp;

 snprintf(path3, SSIZE1, "%s%s", path2, pathx);
 fp = fopen(path3, mode);
 return fp;
}

//==============================================================================
// Test if a file exists and return the full path.
//
//   pass: char *path1          path to be tested
//         char *path2          alternate path prefix to be tried
//         char *path3          the full path that was used
//
// return: *FILE                NULL if no path or file found
//==============================================================================
FILE *test_file (const char *path1, const char *path2, char *path3)
{
 return open_file(path1, path2, path3, "");
}

//==============================================================================
// Search an array of strings for the first occurence of the passed search
// string.  The string array must be be terminated by an empty string.
//
//   pass: char *strg_array     pointer to a string array to be checked
//         char *strg_find      pointer to a string to search for
// return: int                  index if found, else -1
//==============================================================================
int string_search (char *strg_array[], char *strg_find)
{
 int i;

 for (i = 0; strg_array[i][0] != 0; i++)
    {
     if (strcmp(strg_find, strg_array[i]) == 0)
        return i;
    }
 return -1;
}

//==============================================================================
// Same as string_search() but matches upper and lower case.
//
//   pass: char *strg_array     pointer to a string array to be checked
//         char *strg_find      pointer to a string to search for
// return: int                  index if found, else -1
//==============================================================================
int string_case_search (char *strg_array[], char *strg_find)
{
 int i;

 for (i = 0; strg_array[i][0] != 0; i++)
    {
     if (strcasecmp(strg_find, strg_array[i]) == 0)
        return i;
    }
 return -1;
}

//==============================================================================
// Search an array of strings with associated values in any case for the
// first occurence of the passed search string.  The string array must be
// terminated by an empty string.
//
//   pass: sup_args_t *args     pointer to array structure
//         char *strg_find      pointer to a string to search for
// return: int                  index if found, else -1
//==============================================================================
int string_struct_search (sup_args_t *args, char *strg_find)
{
 int x = 0;

 while (*args[x].name && strcasecmp(args[x].name, strg_find) != 0)
    x++;

 if (! *args[x].name)
    return -1;

 return x;
}

//==============================================================================
// Search an array 's1' for the first matching occurrence of array 's2'.
//
// The arrays are not null terminated.
//
//   pass: uint8_t *s1          pointer to an array to be checked. if the Z80
//                              memory map is to be used pass NULL.
//         uint8_t *s2          pointer to an array to searched for
//         int start            starting index in 's1'
//         int finish           finish index in 's1'
//         int size             number of bytes in 's2' to be checked
//         int any              match any case if 1
// return: int                  index if found, else -1
//==============================================================================
int array_search (uint8_t *s1, uint8_t *s2, int start, int finish, int size, int any)
{
 int s1i;
 int s2i;
 char c1;
 char c2;
 int match;

 while (1)
    {
     s1i = start;
     s2i = 0;
     match = 1;

     while (match && (s2i < size) && (s1i <= finish))
        {
         if (s1 == NULL)
            c1 = z80api_read_mem(s1i++);
         else
            c1 = s1[s1i++];

         c2 = s2[s2i++];

         if (any)
            match = (toupper(c1) == toupper(c2));
         else
            match = (c1 == c2);
        }

     if (match && (s2i == size))
        return start;
     else
        if (s1i > finish)
           return -1;

     start++;
    }
}

//==============================================================================
// Find and return a string value at 'position' from a string containing
// multiple string values separated by prefixes.
//
// The string to be checked must have string values prefixed and delimited
// with '+' or '-' characters.
//
// A 0 is returned if no value found (end of string).  A 1 is returned if a
// legal prefixed/delimited string value was found.  A -1 is returned if any
// errors.
//
//   pass: char *strg_scan      string to be scanned.
//         char *strg           string located at position.
//         int position         string value wanted position (1 is first)
//         int maxlen           maximum string length (inc terminator)
// return: int                  prefix if successful, 0 if EOS, -1 if error
//==============================================================================
int string_prefix_get (char *strg_scan, char *strg, int position, int maxlen)
{
 int i;
 int pf = 0;

 while (position--)
    {
     if (*strg_scan == 0)
        return 0;

     // must have a leading '+' or '-' character
     if ((*strg_scan != '+') && (*strg_scan != '-'))
        return -1;  // error
     pf = *(strg_scan++);

     // must not be immediately followed by another or end of string
     if ((*strg_scan == 0) || (*strg_scan == '+') || (*strg_scan == '-'))
        return -1;  // error

     // extract the string value
     i = 0;
     while ((*strg_scan != 0) && (*strg_scan != '+') && (*strg_scan != '-') && (i < maxlen-1))
        strg[i++] = *(strg_scan++);
     strg[i] = 0;
    }

 return pf;
}

//==============================================================================
// Open a text file and search for a non case sensitive matching string
// entry in the first column, if found then return the entry from the second
// column.  If the second column is empty then a null terminated string is
// returned.
//
// Lines beginning with '#' or ';' are ignored.
//
//   pass: char *filename       file name
//         char *strg_search    string to be checked (1st column)
//         char *strg_value     string returned (2nd column)
// return: int                  1 if entry matched and has a value, else 0
//==============================================================================
int find_file_entry (char *filename, char *strg_search, char *strg_value)
{
 FILE *fp;
 char filepath[SSIZE1];
 char s[1000];
 int i;

 strg_value[0] = 0;
 fp = open_file(filename, userhome_confpath, filepath, "r");
 if (! fp)
    return 0;

 while (1)
    {
     if (file_readline(fp, s, sizeof(s)))
        {
         if (strcasestr(s, strg_search) == s)
            {
             fclose(fp);
             i = 0;
             while (s[i] > ' ')
                i++;
             while (s[i] <= ' ')
                i++;
             if (i < strlen(s))
                {
                 strncpy(strg_value, &s[i], SSIZE1);
                 strg_value[SSIZE1-1] = 0;
                 return 1;
                }
             return 0;
            }
        }
     else
        {
         fclose(fp);
         return 0;
        }
    }
}

//==============================================================================
// Open a text file and search for a non case sensitive matching string
// entry in the first column, if found then check if the file in the second
// column exists, if not then repeat until another matching MD5 and file is
// found or the end of file is reached.
//
// Lines beginning with '#' or ';' are ignored.
//
//   pass: char *filename       MD5 file to be used
//         char *strg_search    MD5 value to be searched for (1st column)
//         char *strg_value     file name returned (2nd column)
// return: int                  1 if entry matched and file existed, else 0
//==============================================================================
int find_md5_file_entry (char *filename, char *strg_search, char *strg_value)
{
 FILE *fp;
 char filepath[SSIZE1];
 char s[1000];
 int i;
 int found;

 strg_value[0] = 0;
 fp = open_file(filename, userhome_confpath, filepath, "r");
 if (! fp)
    return 0;

 while (1)
    {
     if (file_readline(fp, s, sizeof(s)))
        {
         if (strcasestr(s, strg_search) == s)
            {
             i = 0;
             while (s[i] > ' ')
                i++;
             while (s[i] <= ' ')
                i++;

             if (i < strlen(s))
                {
                 strncpy(strg_value, s+i, SSIZE1);
                 strg_value[SSIZE1-1] = 0;

                 // if the file was calculated as a binary MD5 remove the leading '*'
                 if (strg_value[0] == '*')
                    strcpy(strg_value, &strg_value[1]);

                 // check if a file exists for this MD5 entry
                 if ((strcmp(filename, "roms.md5.user") == 0) || (strcmp(filename, "roms.md5.auto") == 0))
                    found = (test_file(strg_value, userhome_romspath, filepath) != NULL);
                 else
                    found = (test_file(strg_value, userhome_diskpath, filepath) != NULL);

                 if (emu.verbose)
                    {
                     xprintf("%s=%s %s", filename, strg_search, strg_value);
                     if (found)
                        xprintf("\n");
                     else
                        xprintf(" (no file available)\n");
                    }

                 if (found)
                    {
                     fclose(fp);
                     return 1;
                    }
                }
            }
        }
     else
        {
         fclose(fp);
         return 0;
        }
    }
}

//==============================================================================
// Search a text file for a matching 'strg_search' entry in the first
// column, if found then return the entry from the second column or if the
// value in the second column is prefixed with 'md5=' use the MD5 value in a
// second search of the 'roms.md5.auto' or 'roms.md5.user' for ROMs or
// 'disks.md5' for disks file for a corresponding filename.
//
// The returned 'strg_value' will be set to 'strg_search' if:
// - no matching entry is found for 'strg_search'.
// - no second column entry was found.
// - no matching MD5 entry was found.
//
// Lines beginning with '#' or ';' are ignored.
//
//   pass: char *alias_filename file name
//         char *strg_search    string to be checked (1st column)
//         char *strg_value     string returned (2nd column)
// return: int                  0=not MD5, 1=MD5, -1=MD5 not found in MD5 file
//==============================================================================
int find_file_alias (char *alias_filename, char *strg_search, char *strg_value)
{
 char *p;
 char md5_filename[20];
 char md5_search[512];

 strcpy(strg_value, strg_search);

 if (find_file_entry(alias_filename, strg_search, strg_value))
    {
     // if an MD5 is present then find the MD5 file
     p = strcasestr(strg_value, "md5=");

     if (p == strg_value)
        {
         strcpy(md5_search, p+4);
         if (strcmp(alias_filename, ALIASES_ROMS) == 0)
            {
             if (emu.roms_md5_file == ROMS_MD5_USER)
                strcpy(md5_filename, "roms.md5.user");
             else
                strcpy(md5_filename, "roms.md5.auto");
            }
         else
            if (strcmp(alias_filename, ALIASES_DISKS) == 0)
               strcpy(md5_filename, "disks.md5");
            else
               return -1;
            if (! find_md5_file_entry(md5_filename, md5_search, strg_value))
               return -1;
            return 1;
        }
     else
        return 0;   // have a non-MD5 alias entry
    }
 else
    { // no entry or no second column entry
     strcpy(strg_value, strg_search);
     return 0;
    }
}

//==============================================================================
// Create an MD5 message digest.
//
//   pass: char *filename       file to create MD5 for
//         char *md5            pointer to a 33 byte buffer for MD5
// return: void
//==============================================================================
void create_md5 (char *filename, char *md5)
{
 FILE *fp;
 uint8_t resblock[16];
 int i;

 md5[0] = 0;
 fp = fopen(filename, "rb");
 if (fp)
    {
     if (md5_stream(fp, &resblock) == 0)
        {
         for (i = 0; i < 16; i++)
            sprintf(md5, "%s%02x", md5, resblock[i]);
        }
     fclose(fp);
    }
}

#ifdef MINGW
//==============================================================================
// strcasestr for systems that don't have this function.
//
//   pass: char *haystack
//         char *needle
// return: char *
//
// Returns a pointer to the beginning of the substring, or NULL if the
// substring is not found. 
//==============================================================================
char *strcasestr (char *haystack, char *needle)
{
 char *p, *startn = 0, *np = 0;

 for (p = haystack; *p; p++)
    {
     if (np)
        {
         if (toupper(*p) == toupper(*np))
            {
             if (!*++np)
                return startn;
            }
         else
            np = 0;
        }
     else
        if (toupper(*p) == toupper(*needle))
           {
            np = needle + 1;
            startn = p;
           }
    }
 return NULL;
}

//==============================================================================
// strcasecmp for systems that don't have this function.
//
//   pass: const char *s1
//         const char *s2
// return: int
//
// Returns an integer less than, equal to, or greater than zero if s1 (or
// the first n bytes thereof) is found, respectively, to be less than, to
// match, or be greater than s2.
//==============================================================================
int strcasecmp (const char *s1, const char *s2)
{
 if (!s1 && !s2)
    return 0;

 if (!s1)
    return 1;

 if (!s2)
    return -1;

 while (*s1 && *s2 && tolower((unsigned char)*s1) == tolower((unsigned char)*s2))
    {
     s1++;
     s2++;
    }

 return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}
#endif

//==============================================================================
// Compare strings while treating digits characters numerically.
//
// Compare S1 and S2 as strings holding indices/version numbers, returning
// less than, equal to or greater than zero if S1 is less than, equal to or
// greater than S2 (for more info, see the texinfo doc).
//
// This code has been included as mingw does not appear to have a function
// to do this.
//
// Changed the function name to xstrverscmp() as strverscmp() is an illegal
// use of 'str' for the start of a non 'C' library function name.
//
// Copyright (C) 1997, 2000, 2002, 2004, 2006 Free Software Foundation, Inc.
// This function is part of the GNU C Library.  Contributed by Jean-François
// Bignolles <bignolle@ecoledoc.ibp.fr>, 1997.
//
//   pass: const char *s1
//         const char *s2
// return: int
//==============================================================================
// states: S_N: normal, S_I: comparing integral part, S_F: comparing
//           fractional parts, S_Z: idem but with leading Zeroes only

#define S_N    0x0
#define S_I    0x4
#define S_F    0x8
#define S_Z    0xC

// result_type: CMP: return diff; LEN: compare using len_diff/diff
#define CMP    2
#define LEN    3

// ISDIGIT differs from isdigit, as follows:
//  - Its arg may be any int or unsigned int; it need not be an unsigned char
//    or EOF.
//  - It's typically faster.
//  POSIX says that only '0' through '9' are digits.  Prefer ISDIGIT to
//  isdigit unless it's important to use the locale's definition
//  of `digit' even when the host does not conform to POSIX.

#define ISDIGIT(c) ((unsigned int) (c) - '0' <= 9)

int xstrverscmp (const char *s1, const char *s2)
{
 const unsigned char *p1 = (const unsigned char *) s1;
 const unsigned char *p2 = (const unsigned char *) s2;
 unsigned char c1, c2;
 int state;
 int diff;

 // Symbol(s)    0       [1-9]   others  (padding)
 // Transition   (10) 0  (01) d  (00) x  (11) -
 static const unsigned int next_state[] =
 {
     /* state    x    d    0    - */
     /* S_N */  S_N, S_I, S_Z, S_N,
     /* S_I */  S_N, S_I, S_I, S_I,
     /* S_F */  S_N, S_F, S_F, S_F,
     /* S_Z */  S_N, S_F, S_Z, S_Z
 };

 static const int result_type[] =
 {
     /* state   x/x  x/d  x/0  x/-  d/x  d/d  d/0  d/-
                0/x  0/d  0/0  0/-  -/x  -/d  -/0  -/- */

     /* S_N */  CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP,
                CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
     /* S_I */  CMP, -1,  -1,  CMP,  1,  LEN, LEN, CMP,
                 1,  LEN, LEN, CMP, CMP, CMP, CMP, CMP,
     /* S_F */  CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP,
                CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
     /* S_Z */  CMP,  1,   1,  CMP, -1,  CMP, CMP, CMP,
                -1,  CMP, CMP, CMP
 };

 if (p1 == p2)
    return 0;

 c1 = *p1++;
 c2 = *p2++;

 // Hint: '0' is a digit too.
 state = S_N | ((c1 == '0') + (ISDIGIT (c1) != 0));

 while ((diff = c1 - c2) == 0 && c1 != '\0')
    {
     state = next_state[state];
     c1 = *p1++;
     c2 = *p2++;
     state |= (c1 == '0') + (ISDIGIT (c1) != 0);
    }

 state = result_type[state << 2 | ((c2 == '0') + (ISDIGIT (c2) != 0))];

 switch (state)
    {
     case CMP:
        return diff;

        case LEN:
           while (ISDIGIT (*p1++))
              if (!ISDIGIT (*p2++))
                 return 1;

           return ISDIGIT (*p2) ? -1 : diff;

        default:
           return state;
    }
}

//==============================================================================
// Test if system uses Big Endian or Little Endian format.
//
//   pass: void
// return: int                  1 if Big Endian, 0 if Little Endian
//==============================================================================
int IsBigEndian (void)
{
#if defined(__aix__)
 return 1;
#else
 int i = 1;
 int c = *((char*)&i);
 return c == 0;
#endif
}

//==============================================================================
// Swap Endianess. Only works for even number variable sizes.
//
//   pass: unsigned char *na
//         int size             size in bytes of the variable
// return: void
//==============================================================================
void swap_endian (unsigned char *na, int size)
{
 int i;
 int x;
 unsigned char *nb = na + (size - 1);  // pointer to end of variable

 for (i = 0; i < (size / 2); i++)
    {
     x = *na;
     *na++ = *nb;
     *nb-- = x;
    }
}

//==============================================================================
// Little Endian to host endian,  and host endian to Little Endian functions.
// Swaps byte order if on a Big Endian host.
//
// Don't use these functions directly.  use the macros defined in the header
// file as these make the usage clearer.
//
//   pass: xintxx_t n                   little endian, or host endian format
// return: xintxx_t                     little endian format
//==============================================================================
int16_t little_endian_16 (int16_t n)
{
 if (IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

uint16_t little_endian_u16 (uint16_t n)
{
 if (IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

int32_t little_endian_32 (int32_t n)
{
 if (IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

uint32_t little_endian_u32 (uint32_t n)
{
 if (IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

int64_t little_endian_64 (int64_t n)
{
 if (IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

uint64_t little_endian_u64 (uint64_t n)
{
 if (IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

//==============================================================================
// Big Endian to host endian,  and host to Big Endian functions.
// Swaps byte order if on a Little Endian host.
//
//   pass: xintxx_t n                   big endian, or host endian format
// return: xintxx_t                     host endian, or big endian format
//==============================================================================
int16_t big_endian_16 (int16_t n)
{
 if (! IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

uint16_t big_endian_u16 (uint16_t n)
{
 if (! IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

int32_t big_endian_32 (int32_t n)
{
 if (! IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

uint32_t big_endian_u32 (uint32_t n)
{
 if (! IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

int64_t big_endian_64 (int64_t n)
{
 if (! IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

uint64_t big_endian_u64 (uint64_t n)
{
 if (! IsBigEndian())
    swap_endian((unsigned char *)&n, sizeof(n));
 return(n);
}

//==============================================================================
// set Z80 memory 8, 16 and 32 bit signed and unsigned variables in little
// endian format.  If host uses big endian format the values are swapped first.
//
//   pass: int addr
//         xintxx_t  n
// return: void
//==============================================================================
#if 0
// Note: All these will need z80mem replaced using new access methods.
static void z80_set_8 (int addr, int8_t n)
{
 z80mem[addr] = n;
}

static void z80_set_u8 (int addr, uint8_t n)
{
 z80mem[addr] = n;
}

static void z80_set_16 (int addr, int16_t n)
{
 n = host_to_le16(n);
 memcpy(z80mem+addr, &n, 2);
}

static void z80_set_u16 (int addr, uint16_t n)
{
 n = host_to_leu16(n);
 memcpy(z80mem+addr, &n, 2);
}

static void z80_set_32 (int addr, int32_t n)
{
 n = host_to_le32(n);
 memcpy(z80mem+addr, &n, 4);
}

static void z80_set_u32 (int addr, uint32_t n)
{
 n = host_to_leu32(n);
 memcpy(z80mem+addr, &n, 4);
}

static void z80_set_64 (int addr, int64_t n)
{
 n = host_to_le64(n);
 memcpy(z80mem+addr, &n, 8);
}

static void z80_set_u64 (int addr, uint64_t n)
{
 n = host_to_leu64(n);
 memcpy(z80mem+addr, &n, 8);
}
#endif

//==============================================================================
// get Z80 memory 8, 16 and 32 bit signed and unsigned variables in little
// endian format and convert to host endian format.
//
//   pass: int addr
// return: xintxx_t                     in host endian format
//==============================================================================
#if 0
// Note: All these will need z80mem replaced using new access methods.
static int8_t z80_get_8 (int addr)
{
 return z80mem[addr];
}

static uint8_t z80_get_u8 (int addr)
{
 return z80mem[addr];
}

static int16_t z80_get_16 (int addr)
{
 int16_t n;
 memcpy(&n, z80mem+addr, 2);
 return (le16_to_host(n));
}

static uint16_t z80_get_u16 (int addr)
{
 uint16_t n;
 memcpy(&n, z80mem+addr, 2);
 return (leu16_to_host(n));
}

static int32_t z80_get_32 (int addr)
{
 int32_t n;
 memcpy(&n, z80mem+addr, 4);
 return (le32_to_host(n));
}

static uint32_t z80_get_u32 (int addr)
{
 uint32_t n;
 memcpy(&n, z80mem+addr, 4);
 return (leu32_to_host(n));
}

static int64_t z80_get_64 (int addr)
{
 int64_t n;
 memcpy(&n, z80mem+addr, 8);
 return (le64_to_host(n));
}

static uint64_t z80_get_u64 (int addr)
{
 uint64_t n;
 memcpy(&n, z80mem+addr, 8);
 return (leu64_to_host(n));
}
#endif

//==============================================================================
// Wildcards
//
// Source obtained from GPLv2 wildcards v1.2 project.
// Copyright (C) 1996, 1997, 1998, 1999, 2000 Florian Schintke.
//
// Only structural changes have been made to keep inline with the rest of
// the emulator's style.
//
// int set (char **wildcard, char **test)
// Scans a set of characters and returns 0 if the set mismatches at this
// position in the teststring and 1 if it is matching wildcard is set to the
// closing ] and test is unmodified if mismatched and otherwise the char
// pointer is pointing to the next character
//
// int asterisk (char **wildcard, char **test)
// scans an asterisk
//
// int wildcardfit (char *wildcard, char *test)
// The function to call to compare wildcards to the test string.
//
//   pass: char *wildcard
//         char *test
// return: int                  1 if wildcards fit, else 0
//==============================================================================
int asterisk (char **wildcard, char **test);
int wildcardfit (char *wildcard, char *test);

int set (char **wildcard, char **test)
{
 int fit = 0;
 int negation = 0;
 int at_beginning = 1;

 if ('!' == **wildcard)
    {
     negation = 1;
     (*wildcard)++;
    }
 while ((']' != **wildcard) || (1 == at_beginning))
    {
     if (0 == fit)
        {
         if (('-' == **wildcard)
            && ((*(*wildcard - 1)) < (*(*wildcard + 1)))
            && (']' != *(*wildcard + 1))
            && (0 == at_beginning))
            {
             if (((**test) >= (*(*wildcard - 1))) && ((**test) <= (*(*wildcard + 1))))
                {
                 fit = 1;
                 (*wildcard)++;
                }
            }
         else
            if ((**wildcard) == (**test))
               {
                fit = 1;
               }
        }
     (*wildcard)++;
     at_beginning = 0;
    }
 if (1 == negation)
 // change from zero to one and vice versa
 fit = 1 - fit;
 if (1 == fit)
    (*test)++;
 return (fit);
}

int asterisk (char **wildcard, char **test)
{
 // Warning: uses multiple returns
 int fit = 1;

 // erase the leading asterisk
 (*wildcard)++;
 while (('\000' != (**test)) && (('?' == **wildcard) || ('*' == **wildcard)))
    {
     if ('?' == **wildcard)
        (*test)++;
     (*wildcard)++;
    }

 // Now it could be that test is empty and wildcard contains
 // aterisks. Then we delete them to get a proper state
 while ('*' == (**wildcard))
    (*wildcard)++;

 if (('\0' == (**test)) && ('\0' != (**wildcard)))
    return (fit = 0);
 if (('\0' == (**test)) && ('\0' == (**wildcard)))
    return (fit = 1);
 else
    {
     // Neither test nor wildcard are empty!
     // the first character of wildcard isn't in [*?]
     if (0 == wildcardfit(*wildcard, (*test)))
        {
         do
            {
             (*test)++;
              // skip as much characters as possible in the teststring
              // stop if a character match occurs
              while (((**wildcard) != (**test)) && ('['  != (**wildcard)) && ('\0' != (**test)))
                 (*test)++;
            }
         while ((('\0' != **test))? (0 == wildcardfit (*wildcard, (*test))) : (0 != (fit = 0)));
        }
     if (('\0' == **test) && ('\0' == **wildcard))
        fit = 1;
     return (fit);
    }
}

int wildcardfit (char *wildcard, char *test)
{
 int fit = 1;

 for (; ('\000' != *wildcard) && (1 == fit) && ('\000' != *test); wildcard++)
    {
     switch (*wildcard)
        {
         case '[': wildcard++; // leave out the opening square bracket
                   fit = set (&wildcard, &test);
                   // we don't need to decrement the wildcard as in case
                   // of asterisk because the closing ] is still there
                   break;
         case '?': test++;
                   break;
         case '*': fit = asterisk(&wildcard, &test);
                   // the asterisk was skipped by asterisk() but the loop will
                   // increment by itself. So we have to decrement
                   wildcard--;
                   break;
          default: fit = (int) (*wildcard == *test);
                   test++;
        }
    }
 while ((*wildcard == '*') && (1 == fit))
 // here the teststring is empty otherwise you cannot
 // leave the previous loop
 wildcard++;
 return (int) ((1 == fit) && ('\0' == *test) && ('\0' == *wildcard));
}

//==============================================================================
// Open a directory for reading.
//
// This is called once followed by calls to sup_readdir() for each entry.
//
// Call this function with the following values in *sup_file_t:
// char *dpn    Set a directory path/name for the required files. If a empty
//              sting is passed then '*' will be used for file matching.
//
// The function will return the following values in *sup_file_t:
// char *fnwc   A string containing just the filename part or wildcard
//              characters to be used in following calls to sup_readdir()
// char *mfp    A modified file path dependent on the host system.
// uint16 val1  String length of *fnwc.
// uint16 val2  String length of *mfp.
// unit16 res   Result of sup_opendir(), 0=success, 1=error.
//
//   pass: sup_file_t *f
// return: void
//==============================================================================
void sup_opendir (sup_file_t *f)
{
 struct stat st;

 int l;
 int res;

 *f->fnwc = 0;
 f->val1 = 0;
 res = 1;

 strcpy(f->filepath, f->dpn);
 l = strlen(f->filepath);

 if (! l)                                     // add '.' if an empty path
    strcat(f->filepath, ".");
 else
    if (strcmp(f->filepath, "."SLASHCHAR_STR) == 0)  // if './' or '.\'
       f->filepath[l-1] = 0;                  // remove the slash character
#ifdef MINGW
    else
       if (f->filepath[l-1] == ':')           // if last char is ':'
          strcat(f->filepath, SLASHCHAR_STR); // add a '\'
#endif

 if (stat(f->filepath, &st) == 0)
    {
     res = ((st.st_mode & S_IFDIR) == 0);
     if (! res)
        {
         *f->fnwc = '*';
         *(f->fnwc+1) = 0;
        }
    }
 else
    res = 1;

 if (res)
    {
     l = strlen(f->filepath);
#ifdef MINGW
     while ((l) && (f->filepath[l-1] != SLASHCHAR) && (f->filepath[l-1] != ':'))
#else
     while ((l) && (f->filepath[l-1] != SLASHCHAR))
#endif
        l--;

     strcpy(f->fnwc, f->filepath+l);       // copy the name part
     if ((l > 1) && (f->filepath[l-1] == SLASHCHAR))  // if '/' or '\'
        l--;
     f->filepath[l] = 0;                   // remove the name part

     if (! strlen(f->fnwc))                // if file name is blank
        {
         *f->fnwc = '*';
         *(f->fnwc+1) = 0;
        }

     l = strlen(f->filepath);
     if (! l)                              // add '.' if empty path
        strcat(f->filepath, ".");

     if (stat(f->filepath, &st) == 0)      // try again without name
        res = ((st.st_mode & S_IFDIR) == 0);
     else
        res = 1;
    }

 if (! res)
    {
     f->fp.d = opendir(f->filepath);

     l = strlen(f->filepath);
     if ((l) && (f->filepath[l-1] != SLASHCHAR))  // add '/' or '\' if no end slash
        strcat(f->filepath, SLASHCHAR_STR);

     res = f->fp.d == NULL;
    }

 f->val1 = strlen(f->fnwc);
 strcpy(f->mfp, f->filepath);              // return the file path
 f->val2 = strlen(f->filepath);
 f->res = res;
}

//==============================================================================
// Return the next directory entry. (with wildcards)
//
// Call this function with the following values in *sup_file_t:
// char *mfp    A modified file path created earlier by sup_readdir()
// char *fnwc   A string containing just the filename part or wildcards
//              to be used for matching and created earlier by sup_readdir()
//
// The function will return the following values in *sup_file_t:
// char *dpn    File name if a matching entry is found. An empty string is
//              returned if no match found.
// char *fpfnm  Full path and file name if a matching entry is found. An
//              empty string is returned if no match found.
// uint16 val1  String length of *dpn if match else 0.
// uint16 val2  String length of *fpfnm if match else 0.
// unit16 res   dir_type if match else 0.
//
//   pass: sup_file_t *f
// return: void
//==============================================================================
void sup_readdir (sup_file_t *f)
{
 struct dirent *de;
 struct stat st;

 char filename_dir[1024];
 char filename_search[1024];

 int match;
 int dir_type;

 strcpy(f->filepath, f->mfp);
 match = 0;

 strncpy(filename_search, f->fnwc, sizeof(filename_search));
 filename_search[sizeof(filename_search)-1] = 0;
#ifdef MINGW
 toupper_string(filename_search, filename_search);
#endif
 do
    {
     de = readdir(f->fp.d);
     if (de)
        {
         strncpy(filename_dir, de->d_name, sizeof(filename_dir));
         filename_dir[sizeof(filename_dir)-1] = 0;
#ifdef MINGW
         toupper_string(filename_dir, filename_dir);
#endif
         match = wildcardfit(filename_search, filename_dir);

         if (match)
            {
             strcpy(f->dpn, de->d_name);
             snprintf(f->filename, sizeof(f->filename), "%s%s", f->filepath, de->d_name);
             if (stat(f->filename, &st) == 0)
                {
                 dir_type = 0;
#ifdef MINGW
#else
                 dir_type |= (S_ISLNK(st.st_mode) != 0);
#endif
                 dir_type |= ((S_ISREG(st.st_mode) != 0) << 1);
                 dir_type |= ((S_ISDIR(st.st_mode) != 0) << 2);
                 dir_type |= ((S_ISCHR(st.st_mode) != 0) << 3);
                 dir_type |= ((S_ISBLK(st.st_mode) != 0) << 4);
                 dir_type |= ((S_ISFIFO(st.st_mode) != 0) << 5);
#ifdef MINGW
#else
                 dir_type |= ((S_ISSOCK(st.st_mode) != 0)  << 6);
#endif
                }
             else
                dir_type = 0;
             strcpy(f->fpfnm, f->filename);
             f->val2 = strlen(f->filename);
             f->res = dir_type;
             f->val1 = strlen(f->dpn);
            }
        }
    }
 while ((de) && (! match));

 if (! match)
    {
     *f->dpn = 0;
     *f->fpfnm = 0;
     f->res = 0;
     f->val1 = 0;
     f->val2 = 0;
    }
}

//==============================================================================
// Get the Microworld Basic version currently in memory.
//
// Indentify what version of Microworld Basic is in memory,  ROM models will
// return a value of 500-599 and disk models 600-699.  A value of -1 is
// returned if no recognised Basic version is found.
//
// Note that 5.29 Basic and banked disk Basic used bank switching and could
// possibly cause problems if executing this function under certain
// conditions.
//
//   pass: int msg                      Non zero if error message required
//         char *vers                   Pointer to location to copy ver
//                                      string to, pass NULL if not required.
//                                      Must be 5 characters if used.
// return: int                          'nnn' if version found else -1
//==============================================================================
int get_mwb_version (int msg, char *vers)
{
 basicver_t basicver[] =
 {
  {"5.00", 0xb890},  /* this version has not been tested and is a guess */
  {"5.10", 0xb890},

  {"5.22", 0xb88c},
  {"5.24", 0xb88c},
  {"5.25", 0xb88c},
  {"5.29", 0xb88c},

  {"6.22", 0xbd92},
  {"6.23", 0xbd92},
  
  {"6.26", 0xbd9e},
  {"6.28", 0xbd9e},
  
  {"6.30", 0xbd9f},
  {"6.31", 0xbd9f},
  {"6.34", 0xbd9f},
  {"6.35", 0xbd9f},

  {"",         -1}
 };
 
 char ver_id[5];
 double res;
 int a;
 int b;
 int i;

 if (vers != NULL)
    vers[0] = 0;
 
 for (b = 0; ;b++)
    {
     if ((a = basicver[b].addr) == -1)
        {
         if (msg)
            xprintf("No known Basic version located in memory.\n");
         return -1;
        } 

     for (i = 0; i < 4; i++)
        ver_id[i] = z80api_read_mem(a++);
     ver_id[i] = 0;
     
     if (strcmp(ver_id, basicver[b].id) == 0)
        {
         res = get_float_value(ver_id);
         if (res < 1.0)
            return -1;
         else
            {
             if (vers != NULL)
                strcpy(vers, ver_id);

             // we add 0.5 to eliminate floating point rounding errors
             return (int)((res * 100.0) + 0.5);
            }
        }
    }
}
