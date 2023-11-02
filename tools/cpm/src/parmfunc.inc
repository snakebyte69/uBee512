function KeyCheck : char;
var
   key : char;
begin
   while (keypressed) do
      read(kbd, key);
   repeat until keypressed;
   read(kbd, key);
   KeyCheck := key;
end;

function locase (ch : char) : char;
begin
   if (ch in ['A'..'Z']) then
      locase := chr(ord(ch) + $20)
   else
      locase := ch
end;

function exec_file_function (cmd : integer) : integer;
begin
   filex.cmd := cmd;                                       { sub-function }
   filex.id := $AA55;                                          { check ID }
   port[255] := $50;                                     { file functions }
   port[255] := lo(addr(filex));
   port[255] := hi(addr(filex));
   exec_file_function := filex.res;                              { result }
end;

function exec_dir_function (cmd : integer) : integer;
begin
   dirx.cmd := cmd;                                        { sub-function }
   dirx.id := $AA55;                                           { check ID }
   port[255] := $60;                                { directory functions }
   port[255] := lo(addr(dirx));
   port[255] := hi(addr(dirx));
   exec_dir_function := dirx.res;                               { result }
end;

procedure ParameterUpcasing (var s : String255);
var
   count : integer;
   i     : integer;
begin
   i := 1;
   while (i <= length(s)) do
      begin
         if (s[i] = '''') then
            begin
               delete(s, i, 1);                          { delete the ''' }
               if (i <= length(s)) then
                  case s[i] of
                         '''' : i := i + 1;             { use the 2nd ''' }
                     '0'..'9' : begin
                                   count := ord(s[i]) - ord('0');
                                   if (count = 0) then
                                      count := 10;
                                   delete(s, i, 1);    { delete the digit }
                                   while ((count <> 0) and (i <= length(s))) do
                                      begin
                                         s[i] := upcase(s[i]);
                                         count := count - 1;
                                         i := i + 1;
                                      end;
                                end
                             else
                                begin
                                   s[i] := upcase(s[i]);
                                   i := i + 1;
                                end;
                  end;
            end
         else
            i := i + 1;
      end;
end;

function ParmCount : integer;
var
   i, PrmCnt, PrmLen : integer;
   delchr            : char;
begin
   i := 1;
   PrmCnt := 0;
   PrmLen := length(ComLne);
   while (i <= PrmLen) do
      begin
         while (i <= PrmLen) and (ComLne[i] = ' ') do
            i := i + 1;
         if (i <= PrmLen) then
            PrmCnt := PrmCnt + 1;

         delchr := ' ';
         while (i <= PrmLen) and (ComLne[i] <> delchr) do
            begin
               if (ComLne[i] = '"') then
                  begin
                     if (delchr = ' ') then
                        delchr := #0
                     else
                        delchr := ' ';
                  end;
               i := i + 1
            end;
         i := i + 1;

      end;
   ParmCount := PrmCnt
end;

function ParamStr (PrmNmb : integer) : String255;
var
   i, PrmCnt, PrmLen : integer;
   delchr            : char;
   PrmStr            : String255;
begin
   i := 1;
   PrmCnt := 0;
   PrmStr := '';
   PrmLen := length(ComLne);

   while (i <= PrmLen) and (PrmCnt < PrmNmb) do
      begin
         while (i <= PrmLen) and (ComLne[i] = ' ') do
            i := i + 1;
         if (i <= PrmLen) then
            PrmCnt := PrmCnt + 1;
         PrmStr := '';

         delchr := ' ';
         while (i <= PrmLen) and (ComLne[i] <> delchr) do
            begin
               if (ComLne[i] = '"') then
                  begin
                     if (delchr = ' ') then
                        delchr := #0
                     else
                        delchr := ' ';
                  end
               else
                  PrmStr := PrmStr + ComLne[i];
               i := i + 1
            end;
         i := i + 1;
      end;

   ParamStr := PrmStr
end;
