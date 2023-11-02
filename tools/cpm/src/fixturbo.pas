(*************************************************************************)
(*                                                                       *)
(*          FIXTURBO v1.00 (c) Copyright uBee  25th April 1995           *)
(*                                                                       *)
(*        Allows TURBO v2.00a compiled programs to fully access          *)
(*                      command line parameters                          *)
(*                                                                       *)
(*************************************************************************)

{$C-}                                       { turn off ^C and ^S checking }

const
   FixDta : array [0..24] of byte =
    (
     $C3,$05,$01,          { start:  jp      patch        ;skip data      }
     $CD,                  {         db      0cdh         ; ?             }
     $AB,                  {         db      0abh         ; ?             }
     $21,$80,$00,          { patch:  ld      hl,0080h     ;command line   }
     $11,$E7,$1F,          {         ld      de,1fe7h     ;string const   }
     $01,$80,$00,          {         ld      bc,0080h     ;amount to move }
     $ED,$B0,              {         ldir                 ;move to const  }
     $21,$C9,$1F,          {         ld      hl,1fc9h     ;original jump  }
     $22,$01,$01,          {         ld      (start+1),hl ;replace jump   }
     $C3,$00,$01           {         jp      start        ;original jump  }
    );

var
   F      : file;
   FleNme : string[10];
   TstStr : string[10];
   ComLne : string[127] absolute $0080;
   ComPrm : string[127];
   Buffer : array [0..127] of byte;


procedure ShowUsage;
begin
   writeln('FIXTURBO v1.00 (c) Copyright  uBee  25th April 1995');
   writeln;
   writeln('Allows TURBO v2.00a compiled programs to fully access');
   writeln('command line parameters');
   writeln;
   writeln('Use:- FIXTURBO FILENAME.COM')
end;


begin
   writeln;
   ComPrm := ComLne;
   while pos(' ', ComPrm) = 1 do
      delete(ComPrm, 1, 1);
   if ComPrm <> '' then
      begin
         assign(F, ComPrm);
         reset(F);
         seek(F, ($1FE7 - $100) div 128);
         blockread(F, Buffer, 1);
         move(Buffer[($1FE7 - $100) mod 128], TstStr, sizeof(TstStr));
         if TstStr = 'PARAMETERS' then
            begin
               seek(F, 0);
               blockread(F, Buffer, 1);
               if (Buffer[1] = $C9) and (Buffer[2] = $1F) then
                  begin
                     move(FixDta, Buffer, sizeof(FixDta));
                     seek(F, 0);
                     blockwrite(F, Buffer, 1);
                     close(F);
                     writeln(ComPrm, ' program now modified')
                  end
               else
                  writeln(ComPrm, ' program allready modified')
            end
         else
            writeln(ComPrm, ' program did not contain string ID')
      end
   else
      ShowUsage
end.
