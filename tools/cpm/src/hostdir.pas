(*************************************************************************)
(*                               HOSTDIR                                 *)
(*                    (c) 2007-2008 Copyright uBee                       *)
(*                                                                       *)
(*                   Show directory contents on HOST                     *)
(*************************************************************************)

{    As TURBO v2.00a compiled programs overwrite part of the command      }
{    line parameters (only 1st 31 characters are intact) make sure that   }
{    FIXTURBO.COM is run on this compiled program to allow full access    }
{    to command line parameters.                                          }
{                                                                         }
{    Set the end address to $B000 to allow the compiled programs to work  }
{    with all Microbee CP/M systems.                                      }

{$C-}                                       { turn off ^C and ^S checking }

{
Revision history (most recent at top)
=====================================
v1.1.0 - 23 June 2008, uBee
- Rebuilt with include file PARMFUNC.INC split up. STARTVER.INC now
  contains the version checking code, UBEEFUNC.INC contains types.
- ScanForSwitches function now passes on any unmatched options as normal
  parameters, scanning ends when no options are found or non match.
- ParamCount and ParamStr functions now handle '-' chars correctly
  in file names.  Double quotes are now handled better.

v1.0.0 - 13 Dec 2007, uBee
- Initial release
}

const
   TITLE = 'HOSTDIR (HD)';
   VERSION = '1.1.0';
   DATE = '3-July-2008';

   EMU_V1 = 2;                         { minimum uBee512 emulator version }
   EMU_V2 = 2;
   EMU_V3 = 0;

   ComLneC : string[127] = 'PARAMETERS';        { filled in by patch code }
   ComLne  : string[255] = '';                       { working parameters }

   Done : boolean = false;

type
   String12  = string[12];
   String127 = string[127];
   String255 = string[255];

{$I UBEEFUNC.INC}

var
   HSTPth : String255;

   PrmTot : integer;
   PrmNmb : integer;
   SwtTot : integer;
   Pages  : boolean;

   Error  : integer;
   lines  : integer;

   slashchar : char;

   filex        : ub_file_t;
   dirx         : ub_file_t;
   ioresultHOST : integer;

{$I PARMFUNC.INC}
{$I STARTVER.INC}

procedure ShowDirectory;
var
   dirent   : String255;
   filename : String255;
   dirpath  : String255;
   dirfile  : String255;
   res      : integer;
   res_bit  : integer;
   i        : byte;
   key      : char;
begin
   HSTPth[length(HSTPth)+1] := #0;                       { NULL terminate }

   dirx.addr1 := addr(HSTPth) + 1;
   dirx.addr2 := addr(filename) + 1;          { filename / wildcards part }
   dirx.addr3 := addr(dirpath) + 1;                      { directory path }
   dirx.addr4 := addr(dirfile) + 1;             { directory and file name }

   writeln;
   writeln('Files: ', HSTPth);
   res := exec_dir_function($00);                { sub-function: open dir }
   if (res = 0) then
      begin
         dirx.addr1 := addr(dirent) + 1;

         if (dirx.val1 > 254) then
            filename[0] := chr(254)
         else
            filename[0] := chr(dirx.val1);

         repeat
            res := exec_dir_function($02);      { sub-function: get entry }
            if (dirx.val1 > 255) then
               dirent[0] := chr(255)
            else
               dirent[0] := chr(dirx.val1);
            if (dirent <> '') then
               begin
                  write(dirent);
                  if ((res and 4) <> 0) then
                     write(slashchar);
                  res_bit := 1;
                  for i := 0 to 6 do
                     begin
                        if ((res and res_bit) <> 0) then
                           begin
                              case (i) of
                                 00 : write(' -link');
                                 01 : ;
                                 02 : ;
                                 03 : write(' -char');
                                 04 : write(' -block');
                                 05 : write(' -fifo');
                                 06 : write(' -socket');
                              end;
                           end;
                        res_bit := res_bit shl 1;
                     end;
                  writeln;
               end;
           lines := lines + 1;
           if ((lines = 21) and (Pages)) then
              begin
                 lines := 0;
                 repeat
                    key := KeyCheck;
                 until key in [^M, ^[];
                 Done := key = ^[;
              end
         until ((dirent = '') or (Done));
         res := exec_dir_function($01);         { sub-function: close dir }
      end
   else
      writeln('Directory not found');
end;

procedure ShowUsage;
begin
   writeln(TITLE, ' ', 'v', VERSION, ' (c) Copyright  uBee  ', DATE);
   writeln;
   writeln('Use:- dirpath [dirpath]... [-e -h -p]');
   writeln;
   if (slashchar = '\') then
      writeln('dirpath = Windows directory drive and path')
   else
      writeln('dirpath = Unix directory path');
   writeln('     -e = exit interactive mode to CP/M');
   writeln('     -h = show this help information');
   writeln('     -p = paginate output');
   writeln;
   writeln('The host path is converted to lower case if specifying a command');
   writeln('line from CP/M.  Interactive mode does not change casing.');
end;

function ScanForSwitches (PrmNmb : integer) : integer;
var
   SwtStr    : String255;
   SwtPos, i : integer;
begin
   SwtTot := 0;
   Pages := false;
   repeat
      SwtStr := ParamStr(PrmNmb);
      SwtPos := pos('-', SwtStr);
      if (Swtpos = 1) then
         begin
            SwtTot := SwtTot + 1;
            PrmNmb := PrmNmb - 1;
            for i := 1 to length(SwtStr) do
               SwtStr[i] := upcase(SwtStr[i]);
            if (SwtStr = '-P') then
               Pages := true
            else
            if (SwtStr = '-H') then
               ShowUsage
            else
            if (SwtStr = '-E') then
               Done := true
            else
               begin
                  SwtTot := SwtTot - 1;
                  PrmNmb := PrmNmb + 1;
                  SwtPos := 0;
               end;
         end
   until SwtPos <> 1;
   ScanForSwitches := PrmNmb
end;

procedure HostDirectory;
var
   p           : integer;
   i           : integer;
   SwtStr      : String255;
   Interactive : boolean;
begin
   PrmNmb := 1;
   p := ScanForSwitches(ParmCount);

   Interactive := ((p = 0) and (SwtTot = 0));

   if (Interactive) then
      begin
         ShowUsage;
         writeln;
         writeln('Interactive command mode...');
      end;

   repeat
      lines := 0;
      Error := 0;
      if (Interactive) then
         begin
            repeat
               buflen := 255;
               write('HD>');
               readln(ComLne);
               PrmNmb := 1;
               PrmTot := ScanForSwitches(ParmCount);
            until ((PrmTot > 0) or (Done))
         end
      else
         begin
            PrmTot := p;
            for i := 1 to length(ComLne) do
               ComLne[i] := locase(ComLne[i]);
         end;

      if (PrmTot > 0) then
         begin
            repeat
               HSTPth := ParamStr(PrmNmb);
               if (not Interactive) then
                  ParameterUpcasing(HSTPth);
               PrmNmb := PrmNmb + 1;
               ShowDirectory;
            until (PrmNmb > PrmTot) or (Error <> 0)
         end

   until ((not Interactive) or (Done))
end;

begin
   if (Startup) then
      HostDirectory;
end.
