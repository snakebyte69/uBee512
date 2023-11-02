(*************************************************************************)
(*                               CPM2HOST                                *)
(*                    (c) 2007-2008 Copyright uBee                       *)
(*                                                                       *)
(*                      Copies CP/M files to HOST                        *)
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
v1.2.0 - 27 August 2008, uBee
- File names are now converted to lower case when copied to the host system.
  Upper case may be forced by appending a '-u' switch.

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
   TITLE = 'CPM2HOST (C2H)';
   VERSION = '1.2.0';
   DATE = '27-August-2008';

   EMU_V1 = 2;                         { minimum uBee512 emulator version }
   EMU_V2 = 2;
   EMU_V3 = 0;

   ComLneC : string[127] = 'PARAMETERS';        { filled in by patch code }
   ComLne  : string[255] = '';                       { working parameters }

   ENDBUF = $3FFF;                                     { 16 K copy buffer }
   ENDDIR = $1F;                           { 32 filename directory buffer }
   TMPFLE = 'CPM2HOST.TMP';

   Done : boolean = false;

type
   String12  = string[12];
   String127 = string[127];
   String255 = string[255];

   HSTFle = record
               Path : array [0..255] of char;
                 fp : array [0..7] of byte;
            end;

{$I UBEEFUNC.INC}

var
   F      : file;
   Buffer : array [0..ENDBUF] of byte;
   DirBuf : array [0..ENDDIR] of String12;
   DMA    : array [0..3, 0..31] of char;
   FCB    : array [0..35] of byte;

   CPMdrv : byte;
   DirMsk : String127;
   HSTPth : String255;
   CPMPth : String127;

   PrmTot : integer;
   PrmNmb : integer;
   SwtTot : integer;

   Quiet  : boolean;
   ChkWrt : boolean;
   CopyUp : boolean;
   wstatus : integer;
   Error  : integer;

   slashchar : char;

   filex        : ub_file_t;
   dirx         : ub_file_t;
   ioresultHOST : integer;
   D            : HSTFle;

{$I PARMFUNC.INC}
{$I STARTVER.INC}

procedure AssignHOST (var F : HSTFle; Name : String255);
begin
   move(Name[1], F.Path, length(Name));
   F.Path[length(Name)] := #0;
   fillchar(F.fp, sizeof(F.fp), #0);
end;

procedure ResetHOST (var F : HSTFle);
const
   Mode : array [0..2] of char = 'rb'#0;   { open binary file for reading }
begin
   filex.addr1 := addr(F.Path);
   filex.addr2 := addr(Mode);
   ioresultHOST := exec_file_function($00);         { sub-function: fopen }
   move(filex.fp, F.fp, sizeof(F.fp));
end;

procedure RewriteHOST (var F : HSTFle);
const
   Mode : array [0..2] of char = 'wb'#0; { create binary file for writing }
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

procedure EraseHOST (var F : HSTFle);
begin
   filex.addr1 := addr(F.Path);
   ioresultHOST := exec_file_function($12);        { sub-function: remove }
end;

procedure RenameHOST (var F : HSTFle; Name : String255);
var
   X : HSTFle;
begin
   move(Name[1], X.Path, length(Name));
   X.Path[length(Name)] := #0;

   filex.addr1 := addr(F.Path);                                { old name }
   filex.addr2 := addr(X.Path);                                { new name }
   ioresultHOST := exec_file_function($13);        { sub-function: rename }
end;

procedure BlockWriteHOST (var F : HSTFle; var Buf; RecCnt : integer);
var
   res : integer;
begin
   move(F.fp, filex.fp, sizeof(filex.fp));
   filex.addr3 := addr(Buf);
   filex.size := 128;
   filex.num := RecCnt;
   res := exec_file_function($0F);                 { sub-function: fwrite }
   ioresultHOST := ord(RecCnt <> Res);
end;

procedure SetupFCB;
var
   PosGet, PosPut : integer;
begin
   PosGet := 1;
   PosPut := 1;
   fillchar(FCB, sizeof(FCB), 0);
   fillchar(FCB[1], 11, ' ');
   FCB[0] := CPMdrv;
   repeat
      while (PosGet <= length(DirMsk)) and
      (DirMsk[PosGet] <> '.') and (PosPut < 12) do
         begin
            if (DirMsk[PosGet] = '*') then
               repeat
                  FCB[PosPut] := ord('?');
                  PosPut := PosPut + 1
               until PosPut in [9, 12]
            else
               begin
                  FCB[PosPut] := ord(upcase(DirMsk[PosGet]));
                  PosPut := PosPut + 1
               end;
            PosGet := PosGet + 1
         end;
      if (DirMsk[PosGet] = '.') then
         begin
            PosGet := PosGet + 1;
            PosPut := 9
         end
   until (PosGet > length(DirMsk)) or (PosPut = 12)
end;

procedure LoadDirectoryBuffer (var DirTot : integer);
var
   DirPos, x, i : integer;
   SchDir       : byte;
   FleNme       : String12;
begin
   bdos(26, addr(DMA));
   i := DirTot;
   SchDir := 17;                              { search for first function }
   while i <> 0 do                   { move to correct directory position }
      begin
         bdos(SchDir, addr(FCB));                      { search directory }
         i := i - 1;
         SchDir := 18
      end;
   DirPos := 0;
   repeat
      x := bdos(SchDir, addr(FCB));
      if (x <> $FF) then
         begin
            FleNme := '';
            for i := 1 to 11 do
               begin
                  if (DMA[x, i] <> ' ') then
                     FleNme := FleNme + DMA[x, i];
                  if (i = 8) then
                     FleNme := FleNme + '.'
               end;
            DirBuf[DirPos] := FleNme;
            DirPos := DirPos + 1;
            DirTot := DirTot + 1
         end;
      SchDir := 18                             { search for next function }
   until ((x = $FF) or (DirPos > ENDDIR))
end;

function FileFound (FstDir : boolean; var FleNme : String12) : boolean;
const
   DirTot : integer = 0;
   DirNmb : integer = 0;
   GetPos : integer = 0;
begin
   if (FstDir) then
      begin
         DirNmb := 0;
         DirTot := 0;
         SetupFCB;
      end;
   if (DirNmb = DirTot) then
      begin
         LoadDirectoryBuffer(DirTot);
         GetPos := 0
      end;
   FileFound := DirNmb < DirTot;
   if (DirNmb < DirTot) then
      begin
         FleNme := DirBuf[GetPos];
         GetPos := GetPos + 1;
         DirNmb := DirNmb + 1
      end;
   if (DirTot = 0) then
      writeln('No files found to match: ', CPMPth, DirMsk)
end;

procedure OpenFiles (FleNme : String12; HstNme : String12);
var
   TmpStr : String255;
   UsrRes : string[1];
   Result : integer;
begin
   if (not Quiet) then
      writeln('Copying: ', HstNme);
{$I-}
   assign(F, CPMPth + FleNme);
   reset(F);
   Error := ioresult;
{$I+}
   if (Error = 0) then
      begin
         if ((ChkWrt) and (wstatus <> 2)) then
            begin
               AssignHOST(D, HSTPth + HstNme);
               ResetHOST(D);                           { does file exist ? }
               if (ioresultHOST = 0) then
                  begin
                     CloseHOST(D);
                     Result := ioresultHOST;
                     write('Overwrite ', HSTPth, HstNme, ' ? (Y)es/(N)o/(A)ll/(Q)uit): ');
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
               AssignHOST(D, HSTPth + TMPFLE);
               RewriteHOST(D);      { open temporary host file for writing }
               Error := ioresultHOST;
               if (Error <> 0) then
                  writeln('Error can not create host file')
            end
      end
   else
      writeln('Error opening CP/M file for reading')
end;

procedure CloseFiles (HstNme : String12);
var
   Result : integer;
begin
{$I-}
   close(F);                                            { close CP/M file }
   Error := ioresult;
{$I+}
   if (Error <> 0) then
      writeln('Error closing CP/M source file');
   if (((wstatus = 1) or (wstatus = 2)) and (Error = 0)) then
      begin
         CloseHOST(D);                        { close temporary HOST file }
         Error := ioresultHOST;
         if (Error = 0) then
            begin
               AssignHOST(D, HSTPth + HstNme);
               EraseHOST(D);
               Result := ioresultHOST;
               AssignHOST(D, HSTPth + TMPFLE);
               RenameHOST(D, HSTPth + HstNme);
               Error := ioresultHOST;
               if (Error <> 0) then
                  writeln('Error renaming temporary host file')
            end
         else
            writeln('Error closing temporary host file')
      end
end;

procedure CopyFile;
var
   BufPos : integer;
begin
   repeat
      BufPos := 0;
{$I-}
      while (not eof(F)) and (BufPos < ENDBUF) and (Error = 0) do
         begin
            blockread(F, Buffer[BufPos], 1);   { read data from CP/M file }
            Error := ioresult;
            BufPos := BufPos + 128
         end;
{$I+}
      if (Error = 0) then
         begin
            BlockWriteHOST(D, Buffer, BufPos div 128); { write to HOST file }
            Error := ioresultHOST;
            if (Error <> 0) then
               writeln('Error host disk full')
         end
      else
         writeln('Error reading CP/M file')
   until eof(F) or (Error <> 0)
end;

procedure TransferFiles;
var
   FstDir, Found : boolean;
   FleNme        : String12;
   HstNme        : String12;
   i             : integer;
begin
   wstatus := 1;
   if (pos(':', DirMsk) = 2) then
      begin
         CPMPth := DirMsk;
         CPMPth[0] := #2;
         CPMdrv := (ord(DirMsk[1]) - ord('A')) + 1;
         delete(DirMsk, 1, 2)
      end
   else
      begin
         CPMPth := '';
         CPMdrv := 0                                 { default CP/M drive }
      end;
   if (not (HSTPth[length(HSTPth)] in [':', slashchar])) then
      HSTPth := HSTPth + slashchar;
   FstDir := true;
   while ((wstatus <> 3) and (FileFound(FstDir, FleNme)) and (Error = 0)) do
      begin
         if (wstatus = 0) then      { if previous no to a single file copy }
            wstatus := 1;
         FstDir := false;

         HstNme := FleNme;
         if (CopyUp) then
            begin
               for i := 1 to length(HstNme) do
                  HstNme[i] := upcase(HstNme[i]);
            end
         else
            begin
               for i := 1 to length(HstNme) do
                  HstNme[i] := locase(HstNme[i]);
            end;

         OpenFiles(FleNme, HstNme);
         if (((wstatus = 1) or (wstatus = 2)) and (Error = 0)) then
            CopyFile;
         if (Error = 0) then
            CloseFiles(HstNme)
      end
end;

procedure ShowUsage;
var
   pathstr : string[10];
begin
   writeln(TITLE, ' ', 'v', VERSION, ' (c) Copyright  uBee  ', DATE);
   writeln;
   if (slashchar = '\') then
      pathstr := 'd:\path'
   else
      pathstr := 'path';
   writeln('Use:- cpm2host c:files [c:files]... ', pathstr, ' [-e -f -h -u -q]');
   writeln;
   writeln('c:files = CP/M drive and files');
   if (slashchar = '\') then
      writeln('d:\path = Windows destination drive and path')
   else
      writeln('   path = Unix destination path');
   writeln('     -e = exit interactive mode to CP/M');
   writeln('     -f = force overwriting of existing file(s)');
   writeln('     -h = show this help information');
   writeln('     -u = copy file names using upper case');
   writeln('     -q = quiet, no display of file names');
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
   CopyUp := false;
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
            if (SwtStr = '-U') then
               CopyUp := true
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
            write('C2H>');
            readln(ComLne);
            PrmNmb := 1;
            PrmTot := ScanForSwitches(ParmCount);
            HSTPth := ParamStr(PrmTot);
         until ((PrmTot > 0) or (Done))
      else
         begin
            PrmTot := p;
            HSTPth := ParamStr(PrmTot);
            for i := 1 to length(HSTPth) do
               HSTPth[i] := locase(HSTPth[i]);
            ParameterUpcasing(HSTPth);
         end;

      if (PrmTot > 1) then
         repeat
            DirMsk := ParamStr(PrmNmb);
            for i := 1 to length(DirMsk) do
               DirMsk[i] := upcase(DirMsk[i]);
            PrmNmb := PrmNmb + 1;
            TransferFiles;
         until (PrmNmb >= PrmTot) or (Error <> 0)

   until ((not Interactive) or (Done))
end;

begin
   if (Startup) then
      CopyFiles;
end.
