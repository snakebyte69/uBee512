(*************************************************************************)
(*                               HOST2CPM                                *)
(*                    (c) 2007-2008 Copyright uBee                       *)
(*                                                                       *)
(*                      Copies HOST files to CP/M                        *)
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
   TITLE = 'HOST2CPM (H2C)';
   VERSION = '1.1.0';
   DATE = '3-July-2008';

   EMU_V1 = 2;                         { minimum uBee512 emulator version }
   EMU_V2 = 2;
   EMU_V3 = 0;

   ComLneC : string[127] = 'PARAMETERS';        { filled in by patch code }
   ComLne  : string[255] = '';                       { working parameters }

   ENDBUF = $3FFF;                                     { 16 K copy buffer }
   ENDDIR = $1F;                           { 32 filename directory buffer }
   TMPFLE = 'HOST2CPM.$$$';

   Done : boolean = false;

type
   String127 = string[127];
   String255 = string[255];

{$I UBEEFUNC.INC}

   HSTFle = record
               Path : array [0..255] of char;
                 fp : array [0..7] of byte;
            end;

var
   Fi     : HSTFle;
   Fo     : file;
   Buffer : array [0..ENDBUF] of byte;

   CPMdrv : byte;
   HSTPth : String255;
   CPMPth : String127;

   dirpath : String255;
   dirfile : String255;
   filename : String255;
   dirent   : String255;

   PrmTot : integer;
   PrmNmb : integer;
   SwtTot : integer;

   Quiet  : boolean;
   ChkWrt : boolean;
   wstatus : integer;
   Error  : integer;

   slashchar : char;

   filex        : ub_file_t;
   dirx         : ub_file_t;
   ioresultHOST : integer;

{$I PARMFUNC.INC}
{$I STARTVER.INC}

procedure AssignHOST (var F : HSTFle; Name : String255);
begin
   move(Name[1], F.Path, length(Name));
   F.Path[length(Name)] := #0;
   fillchar(F.fp, sizeof(F.fp), #0);
end;

procedure OpenHOST (var F : HSTFle);
const
   Mode : array [0..2] of char = 'rb'#0;   { open binary file for reading }
begin
   filex.addr1 := addr(F.Path);
   filex.addr2 := addr(Mode);
   ioresultHOST := exec_file_function($00);         { sub-function: fopen }
   move(filex.fp, F.fp, sizeof(F.fp));
end;

procedure CloseHOST (var F : HSTFle);
begin
   move(F.fp, filex.fp, sizeof(filex.fp));
   ioresultHOST := exec_file_function($01);        { sub-function: fclose }
end;

function BlockReadHOST (var F : HSTFle; var Buf) : integer;
var
   res : integer;
begin
   move(F.fp, filex.fp, sizeof(filex.fp));
   filex.addr3 := addr(Buf);
   filex.size := 1;
   filex.num := ENDBUF+1;;
   res := exec_file_function($0A);                  { sub-function: fread }
   ioresultHOST := 0;
   BlockReadHOST := res;                           { number of bytes read }
end;

procedure OpenDirHOST (FleNme : String255);
var
   res  : integer;
begin
   HSTPth := FleNme + #0;                                { NULL terminate }

   dirx.addr1 := addr(HSTPth) + 1;         { pass directory path/filename }
   dirx.addr2 := addr(filename) + 1;      { ret filename / wildcards part }
   dirx.addr3 := addr(dirpath) + 1;                  { ret directory path }
   dirx.addr4 := addr(dirfile) + 1;        { ret path and directory match }

   res := exec_dir_function($00);                { sub-function: open dir }

   if (res = 0) then
      begin
         dirx.addr1 := addr(dirent) + 1;        { return dir entries here }

         if (dirx.val1 > 254) then
            filename[0] := chr(254)
         else
            filename[0] := chr(dirx.val1);
         filename[length(filename)+1] := #0;             { NULL terminate }

         ioresultHOST := 0;
      end
   else
      begin
         writeln('Not a directory');
         ioresultHOST := 1;
      end;
end;

procedure CloseDirHOST;
var
   res : integer;
begin
   res := exec_dir_function($01);               { sub-function: close dir }
end;

function SearchDirectoryHOST : boolean;
var
   eod    : boolean;
   match  : boolean;
   res    : integer;
begin
   Match := false;
   repeat
      res := exec_dir_function($02);             { sub-function: get entry }
      if (dirx.val1 > 255) then
         dirent[0] := chr(255)
      else
         dirent[0] := chr(dirx.val1);

      if (dirx.val2 > 254) then
         dirfile[0] := chr(254)
      else
         dirfile[0] := chr(dirx.val2);
      dirfile[length(dirfile)+1] := #0;                   { NULL terminate }

      eod := dirent = '';              { end of directory if no file found }
      if (not eod) then
         begin
            Match := res <> 4;
            if (Match) then                     { if not a directory entry }
               Match := exec_dir_function($04) = 0;            { 8.3 check }
         end;
   until ((Match) or (eod));
   SearchDirectoryHOST := Match;
end;

procedure OpenFiles (FleNme : String255);
var
   TmpStr : String255;
   UsrRes : string[1];
   Result : integer;
begin
   if (not Quiet) then
      writeln('Copying: ', FleNme);

   AssignHOST(Fi, dirfile);                { full host path and file name }
   OpenHOST(Fi);                             { open host file for reading }
   Error := ioresultHOST;

   if (Error = 0) then             { if no errors then open the CP/M file }
      begin
{$I-}
         if ((ChkWrt) and (wstatus <> 2)) then
            begin
               assign(Fo, CPMPth + FleNme);
               reset(Fo);                             { does file exist ? }
               if (ioresult = 0) then
                  begin
                     close(Fo);
                     Result := ioresult;
                     write('Overwrite ', CPMPth, FleNme, ' ? (Y)es/(N)o/(A)ll/(Q)uit): ');
                     buflen := 1;
                     readln(UsrRes);
                     wstatus := ord((UsrRes = 'y') or (UsrRes = 'Y')) * 1;
                     if (wstatus = 0) then
                        wstatus := ord((UsrRes = 'a') or (UsrRes = 'A')) * 2;
                     if (wstatus = 0) then
                        wstatus := ord((UsrRes = 'q') or (UsrRes = 'Q')) * 3;
                  end
            end;
         if ((wstatus = 1) or (wstatus = 2)) then
            begin
               assign(Fo, CPMPth + TMPFLE);
               rewrite(Fo);         { open temporary CP/M file for writing }
               Error := ioresult;
               if (Error <> 0) then
                  writeln('Error CP/M disk directory full')
            end
{$I+}
      end
   else
      writeln('Error opening host file for reading')
end;

procedure CloseFiles (FleNme : String255);
var
   Result : integer;
begin
   CloseHOST(Fi);                                       { close host file }
   Error := ioresultHOST;
   if (Error <> 0) then
      writeln('Error closing host source file');
   if (((wstatus = 1) or (wstatus = 2)) and (Error = 0)) then
      begin
{$I-}
         close(Fo);                           { close temporary CP/M file }
         Error := ioresult;
         if (Error = 0) then
            begin
               assign(Fo, CPMPth + FleNme);
               erase(Fo);
               Result := ioresult;
               assign(Fo, CPMPth + TMPFLE);
               rename(Fo, CPMPth + FleNme);
               Error := ioresult;
               if (Error <> 0) then
                  writeln('Error renaming temporary CP/M file')
            end
         else
            writeln('Error closing temporary CP/M file')
      end
{$I+}
end;

procedure CopyFile;
var
   Blocks : integer;
   EndFle : boolean;
   res    : integer;
begin
   repeat
      res := BlockReadHOST(Fi, Buffer);
      Blocks := (res + 127) div 128;
      EndFle := res < (ENDBUF + 1);
      if (EndFle) then
         fillchar(Buffer[res], 128 - (res mod 128), ^Z);
{$I-}
      if (Blocks <> 0) then
         begin
            blockwrite(Fo, Buffer, Blocks);          { write to CP/M file }
            Error := ioresult;
            if (Error <> 0) then
                writeln('Error CP/M disk full')
         end
{$I+}
   until ((EndFle) or (Error <> 0));
end;

procedure TransferFiles (FleNme : String255);
var
   EndFle : boolean;
begin
   wstatus := 1;
   OpenDirHOST(FleNme);
   if (ioresultHOST = 0) then
      begin
         while ((wstatus <> 3) and (SearchDirectoryHOST) and (Error = 0)) do
            begin
               if (wstatus = 0) then { if previous no to a single file copy }
                  wstatus := 1;
               OpenFiles(dirent);                 { the returned filename }
               if (((wstatus = 1) or (wstatus = 2)) and (Error = 0)) then
                  begin
                     CopyFile;
                     if (Error = 0) then
                        CloseFiles(dirent);
                  end;
            end;
         CloseDirHOST;
      end
end;

procedure ShowUsage;
var
   pathstr : string[40];
begin
   writeln(TITLE, ' ', 'v', VERSION, ' (c) Copyright  uBee  ', DATE);
   writeln;
   if (slashchar = '\') then
      pathstr := 'd:\path\files'
   else
      pathstr := '/path/files';
   writeln('Use:- host2cpm ', pathstr, ' [', pathstr, ']... c: [-e -f -h -q]');
   writeln;
   if (slashchar = '\') then
      writeln(pathstr, ' = Windows source drive and path to files')
   else
      writeln('  ', pathstr, ' = Unix source path to files');
   writeln('           c: = CP/M destination drive');
   writeln('           -e = exit interactive mode to CP/M');
   writeln('           -f = force overwriting of existing file(s)');
   writeln('           -h = show this help information');
   writeln('           -q = quiet, no display of file names');
   writeln;
   writeln('The host path is converted to lower case if specifying a command');
   writeln('line from CP/M.  Interactive mode does not change casing.');
end;

function ScanForSwitches (PrmNmb : integer) : integer;
var
   SwtStr    : String255;
   SwtPos, i : integer;
begin
   Quiet := false;
   ChkWrt := true;
   SwtTot := 0;
   repeat
      SwtStr := ParamStr(PrmNmb);
      SwtPos := pos('-', SwtStr);
      if (Swtpos = 1) then
         begin
            SwtTot := SwtTot + 1;
            PrmNmb := PrmNmb - 1;
            for i := 1 to length(SwtStr) do
               SwtStr[i] := upcase(SwtStr[i]);
            if (SwtStr = '-Q') then
               Quiet := true
            else
            if (SwtStr = '-F') then
               ChkWrt := false
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

procedure CopyFiles;
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
      Error := 0;
      if (Interactive) then
         repeat
            buflen := 255;
            write('H2C>');
            readln(ComLne);
            PrmNmb := 1;
            PrmTot := ScanForSwitches(ParmCount);
            CPMPth := ParamStr(PrmTot);
            for i := 1 to length(CPMPth) do
               CPMPth[i] := upcase(CPMPth[i]);
         until ((PrmTot > 0) or (Done))
      else
         begin
            PrmTot := p;
            for i := 1 to length(ComLne) do
               ComLne[i] := locase(ComLne[i]);
            CPMPth := ParamStr(PrmTot);
            for i := 1 to length(CPMPth) do
               CPMPth[i] := upcase(CPMPth[i]);
         end;

      if (PrmTot > 1) then
         repeat
            HSTPth := ParamStr(PrmNmb);
            if (not Interactive) then
               ParameterUpcasing(HSTPth);
            TransferFiles(HSTPth);
            PrmNmb := PrmNmb + 1;
         until ((PrmNmb >= PrmTot) or (Error <> 0))

   until ((not Interactive) or (Done))
end;

begin
   if (Startup) then
      CopyFiles;
end.
