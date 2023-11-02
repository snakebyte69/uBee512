(*************************************************************************)
(*                              UBSCRIPT                                 *)
(*                           (c) 2008 uBee                               *)
(*                                                                       *)
(*      Creates a submit and MWB file dependent on uBee512 options       *)
(*************************************************************************)

{    Set the end address to $B000 to allow the compiled programs to work  }
{    with all Microbee CP/M systems.                                      }

{$C-}                                       { turn off ^C and ^S checking }

{
Revision history (most recent at top)
=====================================
v1.1.0 - 26 August 2008, uBee
- Added multiple file_list argument support.
- Added 'A1$=KEY$' to loader as BASIC (v6.30) when started with a file
  does not appear to return correctly on the first key check.
- Incorrect minimum version was set to 2.2.7, changed to 2.8.0.
- Added version reporting.

v1.0.0 - 3 July 2008, uBee
- Initial release
}

const
   TITLE = 'UBSCRIPT';
   VERSION = '1.1.0';
   DATE = '26-August-2008';

   EMU_V1 = 2;                         { minimum uBee512 emulator version }
   EMU_V2 = 8;
   EMU_V3 = 0;

type
   String80 = string[80];
   String127 = string[127];
   String255 = string[255];

{$I UBEEFUNC.INC}

const
   FILE_NAME = 'UBSCRIPT.SUB';
   ComLne : String255 = '';                         { working parameters }
   bufpos : integer = 0;

var
   ComLneC    : String127 ABSOLUTE $0080;
   Buffer     : array [0..1023] of byte;
   lnecnt     : integer;
   Error      : integer;
   slashchar  : char;
   CPMdrv     : String127;
   CPMpth     : String127;

   file_list  : string255;
   file_run   : string255;
   file_app   : string255;
   file_load  : integer;
   file_exec  : integer;
   file_exit  : integer;
   file_listc : integer;

   emu_opts   : ub_stdio_str_t;
   emu_set    : ub_stdio_char_t;
   Fo         : text;
   Fm         : file;
   file_run_noext : String255;

procedure get_string (n : integer; var s : String255);
begin
   emu_opts.cmd := n;
   emu_opts.addr := addr(s[1]);
   port[255] := $80;
   port[255] := lo(addr(emu_opts));
   port[255] := hi(addr(emu_opts));
   s[0] := chr(emu_opts.res);
end;

function get_value (n : integer) : integer;
begin
   emu_opts.cmd := n;
   port[255] := $80;
   port[255] := lo(addr(emu_opts));
   port[255] := hi(addr(emu_opts));
   get_value := emu_opts.res;
end;

procedure set_value (n : integer; v : integer);
begin
   emu_set.cmd := $10+n;
   emu_set.val := v;
   port[255] := $80;
   port[255] := lo(addr(emu_set));
   port[255] := hi(addr(emu_set));
end;

{$I STARTVER.INC}

function string_upper (s : String255) : String255;
var
   i  : integer;
   su : String255;
begin
   su := '';
   for i := 1 to length(s) do
      su := su + upcase(s[i]);
   string_upper := su
end;

procedure sound_video_speed;
begin
   port[$ff] := $e0;     { turn video, sound on.  restore CPU clock rate }
   port[$ff] := $00;
   port[$ff] := $00;
end;

procedure report_error (ErrStr : String80);
begin
   sound_video_speed;
   writeln('UBSCRIPT.COM Error, ', ErrStr)
end;

procedure write_buffer_byte (x : byte);
begin
   buffer[bufpos] := x;
   bufpos := bufpos + 1;
   lnecnt := lnecnt + 1;
end;

procedure write_buffer_integer (x : integer);
begin
   write_buffer_byte(lo(x));
   write_buffer_byte(hi(x));
end;

procedure write_buffer_line_no (x : integer);
begin
   write_buffer_byte(hi(x));
   write_buffer_byte(lo(x));
end;

procedure write_buffer_char (x : char);
begin
   write_buffer_byte(ord(x));
end;

procedure write_buffer_string (x : String80);
var
   i : integer;
begin
   for i := 1 to length(x) do
      write_buffer_byte(ord(x[i]));
end;

procedure write_buffer_flush;
var
   i : integer;
begin
   blockwrite(Fm, Buffer, (bufpos + 127) div 128);
   bufpos := 0;
end;

procedure create_mwb_loader;
const
   header : array [0..63] of byte =
   (
    $08,$02,$06,$00,$09,$34,$07,$0D,$48,$00,$00,$00,$00,$00,$32,$09,
    $00,$09,$2C,$09,$00,$04,$00,$08,$2D,$09,$00,$00,$00,$7F,$3E,$46,
    $20,$3E,$00,$08,$01,$00,$00,$00,$00,$00,$00,$00,$00,$7E,$0A,$78,
    $00,$00,$00,$00,$00,$00,$08,$6E,$00,$00,$00,$00,$00,$00,$00,$C9
   );
var
   i         : integer;
   x         : integer;
   mwb_file  : boolean;
   ls        : String255;
   es        : String255;
   line_last : integer;
begin
   assign(Fm, CPMdrv + 'loader.mwb');
   rewrite(Fm);

   for i := 0 to 63 do                             { write the MWB header }
      write_buffer_byte(header[i]);

   line_last := 100;

   str(file_load, ls);                 { convert load address to a string }
   str(file_exec, es);                 { convert exec address to a string }
   if (file_load = 0) then
      ls := es;

   mwb_file := (pos('.MWB', string_upper(file_run)) = (length(file_run) - (4-1)));

   { RUNM command if load and exec same or either is 0, RUN if MWB Basic }
   if ((file_load = file_exec) or (file_load = 0) or (file_exec = 0) or (mwb_file)) then
      begin
         write_buffer_line_no(line_last);                      { line 100 }
         lnecnt := 0;
         x := bufpos;                  { save the current buffer position }
         write_buffer_byte($00);                { char count place holder }
         write_buffer_string(' A1$=');
         write_buffer_byte($DE);                              { KEY token }
         write_buffer_string('$');
         write_buffer_char(^M);                             { end of line }
         buffer[x] := lnecnt;               { char count for current line }

         line_last := line_last + 10;
         write_buffer_line_no(line_last);                      { line 110 }
         lnecnt := 0;
         x := bufpos;                  { save the current buffer position }
         write_buffer_byte($00);                { char count place holder }
         write_buffer_char(' ');
         write_buffer_byte($A0);                              { OUT token }
         write_buffer_string(' 255,224:');                      { $FF,$E0 }
         write_buffer_byte($A0);                              { OUT token }
         write_buffer_string(' 255,0:');                        { $FF,$00 }
         write_buffer_byte($A0);                              { OUT token }
         write_buffer_string(' 255,0');                         { $FF,$00 }
         write_buffer_char(^M);                             { end of line }
         buffer[x] := lnecnt;               { char count for current line }

         line_last := line_last + 10;
         write_buffer_line_no(line_last);                      { line 120 }
         lnecnt := 0;
         x := bufpos;                  { save the current buffer position }
         write_buffer_byte($00);                { char count place holder }
         write_buffer_char(' ');
         write_buffer_byte($97);                              { RUN token }
         if (not mwb_file) then
            write_buffer_string('M');                 { make RUNM command }
         write_buffer_string(' "');
         write_buffer_string(string_upper(file_run));
         write_buffer_string('" ');
         if (not mwb_file) then
            write_buffer_string(ls);                       { load address }
         write_buffer_char(^M);                             { end of line }
         buffer[x] := lnecnt;               { char count for current line }
      end
   else
      begin
         write_buffer_line_no(line_last);                      { line 100 }
         lnecnt := 0;
         x := bufpos;                  { save the current buffer position }
         write_buffer_byte($00);                { char count place holder }
         write_buffer_string(' A1$=');
         write_buffer_byte($DE);                              { KEY token }
         write_buffer_string('$');
         write_buffer_char(^M);                             { end of line }
         buffer[x] := lnecnt;               { char count for current line }

         line_last := line_last + 10;
         write_buffer_line_no(line_last);                      { line 110 }
         lnecnt := 0;
         x := bufpos;                  { save the current buffer position }
         write_buffer_byte($00);                { char count place holder }
         write_buffer_char(' ');
         write_buffer_byte($E1);                             { LOAD token }
         write_buffer_string('M "');          { make LOADM command, quote }
         write_buffer_string(string_upper(file_run));
         write_buffer_string('" ');
         write_buffer_string(ls);                          { load address }
         write_buffer_char(^M);                             { end of line }
         buffer[x] := lnecnt;               { char count for current line }

         line_last := line_last + 10;
         write_buffer_line_no(line_last);                      { line 120 }
         lnecnt := 0;
         x := bufpos;                  { save the current buffer position }
         write_buffer_byte($00);                { char count place holder }
         write_buffer_char(' ');
         write_buffer_byte($A0);                              { OUT token }
         write_buffer_string(' 255,224:');                      { $FF,$E0 }
         write_buffer_byte($A0);                              { OUT token }
         write_buffer_string(' 255,0:');                        { $FF,$00 }
         write_buffer_byte($A0);                              { OUT token }
         write_buffer_string(' 255,0');                         { $FF,$00 }
         write_buffer_char(^M);                             { end of line }
         buffer[x] := lnecnt;               { char count for current line }

         line_last := line_last + 10;
         write_buffer_line_no(line_last);                      { line 130 }
         lnecnt := 0;
         x := bufpos;                  { save the current buffer position }
         write_buffer_byte($00);                { char count place holder }
         write_buffer_string(' U = ');
         write_buffer_byte($BF);                              { USR token }
         write_buffer_char('(');
         write_buffer_string(es);                       { execute address }
         write_buffer_char(')');
         write_buffer_char(^M);                             { end of line }
         buffer[x] := lnecnt;               { char count for current line }
      end;

   write_buffer_integer($ffff);                      { end of file marker }

   x := bufpos;                        { save the current buffer position }
   bufpos := $12;                  { program end pointer LSB+1, (last ff) }
   write_buffer_integer(x-$40-1);     { less the header, point to last ff }
   bufpos := $2F;
   write_buffer_line_no(line_last+10);                     { next line no }
   bufpos := $37;
   write_buffer_line_no(line_last);                        { last line no }
   bufpos := x;

   write_buffer_flush;                                     { flush buffer }
   close(Fm);
end;

procedure submit_create;
var
   error : integer;
begin
   assign(Fo, CPMpth);
{$I-}
   rewrite(Fo);                               { open CP/M file for writing }
   error := ioresult;
   if (error <> 0) then
      begin
         sound_video_speed;
         report_error('Can not create submit file');
         halt;
      end
{$I+}
end;

procedure submit_copy;
var
   i : integer;
   x : integer;
   s : string127;
begin
   for x := 0 to file_listc-1 do
      begin
         set_value(0, x);              { set the string we want access to }
         get_string(0, file_list);
         s := '';
         for i := 1 to length(file_list) do
            begin
               { if it's Unix we are using then prefix upper case characters with ' }
               if ((slashchar = '/') and (file_list[i] in ['A'..'Z'])) then
                  s := s + #39; { ' }
               s := s + file_list[i];
            end;
         writeln(Fo, 'host2cpm ', s, ' ', CPMdrv, ' -f');
      end
end;

procedure submit_mwb_bas_sub;
begin
   writeln(Fo, CPMdrv);                           { set the working drive }
   if (pos('.BAS', string_upper(file_run)) = (length(file_run) - (4-1))) then
      begin
         writeln(Fo, 'vscrest');
         writeln(Fo, file_app, ' ', file_run);
         if (file_exit <> 0) then
            writeln(Fo, 'exitemu');
      end
   else
      if (pos('.SUB', string_upper(file_run)) = (length(file_run) - (4-1))) then
         writeln(Fo, file_app, ' ', file_run)
      else
         begin
            create_mwb_loader;
            writeln(Fo, file_app, ' loader.mwb');
            if (file_exit <> 0) then
               writeln(Fo, 'exitemu');
         end
end;

procedure submit_close;
var
   error : integer;
begin
{$I-}
   close(Fo);
   error := ioresult;
   if (error <> 0) then
      begin
         sound_video_speed;
         report_error('Can not close submit file');
         halt;
      end
{$I+}
end;

function cpm_parameter : boolean;
var
   GotPrm : boolean;
begin
   while pos(' ', CPMdrv) = 1 do
      delete(CPMdrv, 1, 1);
   GotPrm := (CPMdrv <> '') and (length(CPMdrv) <= 2);
   if (GotPrm) then
      begin
         if (length(CPMdrv) = 2) then
            GotPrm := CPMdrv[2] = ':'
         else
            CPMdrv := CPMdrv + ':'
      end;
   CPMpth := CPMdrv + FILE_NAME;
   cpm_parameter := GotPrm
end;

procedure ubscript_make;
var
   i : integer;
begin
   CPMdrv := ComLne;
   if (cpm_parameter) then
      begin
         get_string(0, file_list);
         get_string(1, file_run);
         get_string(2, file_app);
         file_load := get_value(3);
         file_exec := get_value(4);
         file_exit := get_value(5);
         file_listc := get_value(6);

{ if no file_run specified use the entry from the last file_list }
         if (file_run = '') then
            begin
               if (file_listc <> 0) then
                  begin
                     set_value(0, file_listc-1);
                     get_string(0, file_list);
                  end
               else
                  file_list := '';
               i := length(file_list);
               while ((i > 0) and (not (file_list[i] in ['/', '\', ':']))) do
                  i := i - 1;
               i := i + 1;
               file_run := copy(file_list, i, 14);  { room for 2x'"' chars }
               while (pos('"', file_run) <> 0) do
                  delete(file_run, pos('"', file_run), 1);
            end;
         if (length(file_run) > 12) then
            file_run[0] := #12;

{ check to see there if a script that needs to be created and exit if not }
         if ((file_run = '') and (file_app = '')) then
            begin
               sound_video_speed;
               halt;
            end;

{ determine the default application helper if none specified }
         if (file_app = '') then
            begin
               if (pos('.BAS', string_upper(file_run)) = (length(file_run) - (4-1))) then
                  file_app := 'mbasic'
               else
                  if (pos('.SUB', string_upper(file_run)) = (length(file_run) - (4-1))) then
                     file_app := 'supersub'
                  else
                     file_app := 'basic'
            end;

{ create the submit file }
         submit_create;

         if (file_listc <> 0) then
            submit_copy;

{ remove the file extension from the run name }
         file_run_noext := file_run;
         i := pos('.', file_run_noext);
         if (i <> 0) then
            file_run_noext[0] := chr(i - 1);

         if (file_run <> '') then

{ run a command (and CCP) and other types after changing the current drive, }
{ parameters can not be passed using this method. }
            begin
               if (pos('.COM', string_upper(file_run)) = (length(file_run) - (4-1))) then
                  begin
                     writeln(Fo, CPMdrv);               { set the working drive }
                     if ((file_load = 0) and (file_exec = 0)) then
                        begin
                           writeln(Fo, 'vscrest');
                           writeln(Fo, file_run_noext);
                           if (file_exit <> 0) then
                              writeln(Fo, 'exitemu');
                        end
                     else
                        submit_mwb_bas_sub;
                  end
               else
                  submit_mwb_bas_sub;
            end
         else

{ run a command (and CCP) from the current drive, use this to execute a COM }
{ file or CCP command containing optional parameters in --file-app with     }
{ --file-list and --file-run not set. }
            begin
               if (file_app <> '') then
                  begin
                     writeln(Fo, file_app);
                     if (file_exit <> 0) then
                        writeln(Fo, 'exitemu');
                  end
            end;

         submit_close;
{
         writeln('file_listc=', file_listc);
         writeln(' file_list=', file_list);
         writeln('  file_run=', file_run);
         writeln('  file_app=', file_app);
         writeln(' file_load=', file_load);
         writeln(' file_exec=', file_exec);
         writeln(' file_exit=', file_exit);
}
      end
   else
      report_error('CP/M path error')
end;

begin
   writeln(TITLE, ' ', 'v', VERSION, ' (c) Copyright  uBee  ', DATE);
   if (Startup) then
      ubscript_make;
end.
