function GetHostType : integer;
var
   status : ub_cmdres_t;
begin
   status.cmd := $01;         { sub-function: Get host system information }
   port[255] := $00;                                   { status functions }
   port[255] := lo(addr(status));
   port[255] := hi(addr(status));
   GetHostType := status.res;                         { 0=win32, 1=unices }
end;

function CheckEmulatorVersion (v1, v2, v3 : integer) : integer;
const
   EMUPORT=$FF;
var
   version : ub_version_t;
   name    : string[100];
   i       : integer;
   c       : char;
   res     : integer;
begin
   res := 0;
   name := '';
   i := 0;
   repeat
      i := i + 1;
   until (port[EMUPORT] = 0) or (i = 100);

   if (i <> 100) then
      begin
         i := 0;
         repeat
            i := i + 1;
            c := upcase(chr(port[EMUPORT]));
            if (c <> #0) then
               name := name + c;
         until ((c = #0) or (i = 100));

         if (name = 'UBEE512') then
            begin
               res := 1;                              { incorrect version }
               version.cmd := $00;            { sub-function: get version }
               port[255] := $00;                       { status functions }
               port[255] := lo(addr(version));
               port[255] := hi(addr(version));
               if (version.ver1 >= v1) then
                  begin
                     if (version.ver1 > v1) then
                        res := 2
                     else
                        if (version.ver2 >= v2) then
                           begin
                              if (version.ver2 > v2) then
                                 res := 2
                              else
                                 if (version.ver3 >= v3) then
                                    res := 2;
                           end;
                  end;
            end;
      end;
   CheckEmulatorVersion := res;
end;

function Startup : boolean;
var
   x : integer;
begin
   Startup := false;
   x := CheckEmulatorVersion(EMU_V1, EMU_V2, EMU_V3);
   if (x = 0) then
      writeln('Must be run under the uBee512 emulator! (v2.2.0 onwards)')
   else
      if (x = 1) then
         begin
            writeln('Not the correct uBee512 emulator version!');
            writeln('Requires v', EMU_V1, '.', EMU_V2, '.', EMU_V3, ' or later');
         end
      else
         begin
            if (GetHostType = 0) then
               slashchar := '\'                                 { Windows }
            else
               slashchar := '/';                                 { Unices }
            ComLne := ComLneC;  { start with CP/M command line parameters }
            Startup := true;
         end
end;
