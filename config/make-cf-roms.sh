#!/bin/bash
#===============================================================================
# Compact Flash CB 256K ROMs, the images hold all the required ROMs as would
# be found on a banked DRAM and PC85 Microbee.  The boot ROMs must be
# modified versions to boot from the CF IDE HDD.
#
# See CF project 'CF_Coreboard_MMU_notes.txt' for more information.
#
#  ROM Address  Contents  Destination  When
#  00000-03fff  Boot A    8000-bfff    mode=0
#  04000-07fff  Boot B    c000-ffff    mode=0 and rom_sel=0
#  08000-09fff  Boot C    e000-ffff    mode=0 and rom_sel=1
#  0a000-0bfff  Basic A   8000-9fff    mode=1 and prem_rom=0 
#  0c000-0dfff  Basic B   a000-bfff    mode=1 
#  0e000-0ffff  Basic C   8000-9fff    mode=1 and prem_rom=1
#  10000-11fff  NET 0     e000-ffff    mode=1 and net=0
#  12000-13fff  NET 1     e000-ffff    mode=1 and net=1
#  14000-15fff  NET 2     e000-ffff    mode=1 and net=2
#  16000-17fff  NET 3     e000-ffff    mode=1 and net=3
#  18000-19fff  PAK 0     c000-dfff    mode=1 and pak=0
#  1a000-1bfff  PAK 1     c000-dfff    mode=1 and pak=1
#  1c000-1dfff  PAK 2     c000-dfff    mode=1 and pak=2
#  1e000-1ffff  PAK 3     c000-dfff    mode=1 and pak=3
#  20000-21fff  PAK 4     c000-dfff    mode=1 and pak=4
#  22000-23fff  PAK 5     c000-dfff    mode=1 and pak=5
#  24000-25fff  PAK 6     c000-dfff    mode=1 and pak=6
#  26000-27fff  PAK 7     c000-dfff    mode=1 and pak=7
#  28000-29fff  PAK 8     c000-dfff    mode=1 and pak=8
#  2a000-2bfff  PAK 9     c000-dfff    mode=1 and pak=9
#  2c000-2dfff  PAK A     c000-dfff    mode=1 and pak=a
#  2e000-2ffff  PAK B     c000-dfff    mode=1 and pak=b
#  30000-31fff  PAK C     c000-dfff    mode=1 and pak=c
#  32000-33fff  PAK D     c000-dfff    mode=1 and pak=d
#  34000-35fff  PAK E     c000-dfff    mode=1 and pak=e
#  36000-37fff  PAK F     c000-dfff    mode=1 and pak=f
#===============================================================================

# this detemines the location of the ROMs
ROM_DIR=~/.ubee512/roms/

# temporary file
FILE_FF=/tmp/FILE_FF.BIN

#===============================================================================
# Create a temporary 1K file containing all 0xFFs
#===============================================================================
rm -f $FILE_FF

for i in `seq 1 1024`;
   do
      echo -n "\0377" >> $FILE_FF
   done

#===============================================================================
# Write multiples of 1024 bytes of 0xFF to the output file
#===============================================================================
write_ffs ()
{
 for i in `seq 1 $1`;
    do
       cat $FILE_FF >> $ROM
    done
}

#===============================================================================
# Create a Premium PC85 ROM image
#===============================================================================
create_pcf_rom ()
{
 ROM=PCF.ROM

# Boot A (16K)
 dd if=BN56CF.ROM bs=1024 count=8 > $ROM
 write_ffs 8
# Boot B (16K)
 write_ffs 16
# Boot C (8K)
 write_ffs 8

# Basic A (8K)
 dd if=PPC85A.ROM bs=1024 count=8 >> $ROM
# Basic B (8K)
 dd if=PPC85B.ROM bs=1024 count=8 >> $ROM
# Basic C (8K)
 dd if=PPC85A.ROM bs=1024 skip=8 count=8 >> $ROM

# Net 0 (8K)
 dd if=PPC85D.ROM bs=1024 skip=0 count=4 >> $ROM
 write_ffs 4
# Net 1 (8K)
 dd if=PPC85D.ROM bs=1024 skip=4 count=4 >> $ROM
 write_ffs 4
# Net 2 (8K)
 write_ffs 8
# Net 3 (8K)
 write_ffs 8

# Pak 0 (8K)
 dd if=PPC85C.ROM bs=1024 count=8 >> $ROM
# Pak 1 (8K)
 dd if=PPC85E.ROM bs=1024 count=8 >> $ROM
# Pak 2 (8K)
 dd if=PPC85F.ROM bs=1024 count=8 >> $ROM
# Pak 3 (8K)
 dd if=PPC85G.ROM bs=1024 count=8 >> $ROM
# Pak 4 (8K)
 dd if=PPC85H.ROM bs=1024 count=8 >> $ROM
# Pak 5 (8K)
 dd if=PPC85I.ROM bs=1024 count=8 >> $ROM
# Pak 6 (8K)
 write_ffs 8
# Pak 7 (8K)
 write_ffs 8

# Pak 8 (8K)
 write_ffs 8
# Pak 9 (8K)
 write_ffs 8
# Pak 10 (8K) Pak 2+8
 dd if=PPC85F.ROM bs=1024 skip=8 count=8 >> $ROM
# Pak 11 (8K) pak 3+8
 dd if=PPC85G.ROM bs=1024 skip=8 count=8 >> $ROM
# Pak 12 (8K)
 write_ffs 8
# Pak 13 (8K)
 write_ffs 8
# Pak 14 (8K)
 write_ffs 8
# Pak 15 (8K)
 write_ffs 8

# Filler (32K)
 write_ffs 32

 ln -s $ROM pcf.rom
}

#===============================================================================
# Create a Standard PC85 ROM image
#===============================================================================
create_scf_rom ()
{
 ROM=SCF.ROM
 
# Boot A (16K)
 dd if=BN56CF.ROM bs=1024 count=8 > $ROM
 write_ffs 8
# Boot B (16K)
 write_ffs 16
# Boot C (8K)
 write_ffs 8

# Basic A (8K)
 dd if=PC85A.ROM bs=1024 count=8 >> $ROM
# Basic B (8K)
 dd if=PC85B.ROM bs=1024 count=8 >> $ROM
# Basic C (8K)
 write_ffs 8

# Net 0 (8K)
 dd if=PC85D.ROM bs=1024 skip=0 count=4 >> $ROM
 write_ffs 4
# Net 1 (8K)
 dd if=PC85D.ROM bs=1024 skip=4 count=4 >> $ROM
 write_ffs 4
# Net 2 (8K)
 write_ffs 8
# Net 3 (8K)
 write_ffs 8

# Pak 0 (8K)
 dd if=PC85C.ROM bs=1024 count=8 >> $ROM
# Pak 1 (8K)
 dd if=PC85E.ROM bs=1024 count=8 >> $ROM
# Pak 2 (8K)
 write_ffs 8
# Pak 3 (8K)
 write_ffs 8
# Pak 4 (8K)
 write_ffs 8
# Pak 5 (8K)
 dd if=PC85I.ROM bs=1024 count=8 >> $ROM
# Pak 6 (8K)
 write_ffs 8
# Pak 7 (8K)
 write_ffs 8

# Pak 8 (8K)
 write_ffs 8
# Pak 9 (8K)
 write_ffs 8
# Pak 10 (8K)
 write_ffs 8
# Pak 11 (8K)
 write_ffs 8
# Pak 12 (8K)
 write_ffs 8
# Pak 13 (8K)
 write_ffs 8
# Pak 14 (8K)
 write_ffs 8
# Pak 15 (8K)
 write_ffs 8

# Filler (32K)
 write_ffs 32

 ln -s $ROM scf.rom
}

CUR_DIR=$PWD
cd $ROM_DIR
create_pcf_rom
create_scf_rom
rm -f $FILE_FF
cd $CUR_DIR
