# uBee512
An emulator for the Microbee Z80 ROM, FDD and HDD based models. 
Working towards a unified makefile for Windows, macOS and Linux. 
For more information and build instructions for Windows, macOS and Linux; see the Microbee Programmers Discord https://discord.gg/hSsTfsA6gy

## Build

## macOS

### Homebrew packages ###
    brew install libdsk
    brew install libzzip
    brew install SDL12-compat

### Download z80ex ###
    https://github.com/snakebyte69/z80ex
    % make
    % sudo make install

### Then Build uBee512
    /src
    % make
    % sudo make install

### Copy ROMs and Disks ###
    - copy microbee roms into ~/.ubee512/roms/
      - basic_5.22e.rom
      - charrom.bin
      - rom1.bin
      - bn56.rom

    $ cp disks/*.dsk ~/.ubee512/disks/ (may not be needed after make install)

    - copy the disk you want to boot as "boot.dsk"
        e.g. microbee_cpm3_61k_ds80.dsk

# Linux

*** Linux Build ***
### Prerequiste apps ###
    $ apt install -y libsdl1.2-dev
    $ apt install -y libdsk-utils (maybe)
    $ apt install -y libdsk4-dev (confirmed)
    $ apt install -y libzzip-dev
    - copy __hints.h from the libzzip source to /usr/include/zzip/
    $ apt install -y libz80ex-dev
    $ apt install -y libbz2-dev

### MAKE ###
    $ cd src
    $ make
    $ sudo make install

### Copy ROMs and Disks ###
    - copy microbee roms into /usr/local/share/ubee512/roms/
      - basic_5.22e.rom
      - charrom.bin
      - rom1.bin
      - bn56.rom
    $ cp disks/*.dsk ~/.ubee512/disks/ (may not be needed after make install)
    - copy the disk you want to boot as boot.dsk
        e.g. microbee_cpm3_61k_ds80.dsk

## RUN ##
### Boots to Basic ###
    $ ubee512 --model=ic

### Boots a Premium 128k with Wordstar in B Drive ###
    $ ubee512 --model=p128k -a boot.dsk -b wordstar.dsk

## Run

ubee512 requires a copy of the bios roms from a Microbee computer

If you have a copy of the roms for mame, you can copy them from the mame roms directory:

### Linux

    cp /usr/share/games/mame/roms/mbee128p/bn56.rom  ~/.ubee512/roms/P128K.ROM
    cp /usr/share/games/mame/roms/mbee128p/bn56.rom  ~/.ubee512/roms/P512K.ROM
    cp /usr/share/games/mame/roms/mbee128p/charrom.bin ~/.ubee512/roms/
    cp /usr/share/games/mame/roms/mbeepc85/bas525a.rom ~/.ubee512/roms/PC85_BASIC_A.ROM
    cp /usr/share/games/mame/roms/mbeepc85/bas525b.rom ~/.ubee512/roms/PC85_BASIC_B.ROM

    ./src/build/ubee512 --model=p128k -a ./disks/boot.dsk -b disks/ubee512_cpm_tools.ds40_
    ./src/build/ubee512 --model=pc85 --conio --tapfilei=DiamondD.tap
    ./src/build/ubee512 --help

# Documentation

The full manual can be found under [doc/README.md](doc/README.md)

The quickstart guide can be found under [doc/QuickStart.md](doc/QuickStart.md)

The compilation guide can be found under [doc/INSTALL.md](doc/INSTALL.md)

This is an unoffical fork of the uBee512 source. For more information, see the Microbee Programmers Discord https://discord.gg/hSsTfsA6gy
