type
   ub_cmdres_t = record
           cmd : integer;
            id : integer;
           res : integer;
      reserved : array[0..7] of byte;
   end;

   ub_file_t = record
        cmd : integer;             { sub command }
         id : integer;             { 0xAA55 check ID }
        res : integer;             { result }
         fp : array[0..7] of byte; { file pointer }
      addr1 : integer;             { string 1 address in Z80 map }
      addr2 : integer;             { string 2 address in Z80 map }
      addr3 : integer;             { string 3 address in Z80 map }
      addr4 : integer;             { string 4 address in Z80 map }
       size : integer;             { size of object }
        num : integer;             { number of chars }
       val1 : integer;             { value 1 }
       val2 : integer;             { value 2 }
   end;

   ub_version_t = record
       cmd : integer;
      ver1 : integer;
      ver2 : integer;
      ver3 : integer;
   end;

   ub_stdio_char_t = record
       cmd : integer;
       res : integer;
       val : integer;
   end;

   ub_stdio_str_t = record
       cmd : integer;
       res : integer;
      addr : integer;
   end;
