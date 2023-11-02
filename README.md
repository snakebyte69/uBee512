## uBee512
An emulator for the Microbee Z80 ROM, FDD and HDD based models

## Build

### Linux

    sudo apt install -y libsdl1.2-dev
    make
    ./src/build/ubee512
    cp disks/*.dsk ~/.ubee512/disks/

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

## Documentation

The full manual can be found under [doc/README.md](doc/README.md)

The quickstart guide can be found under [doc/QuickStart.md](doc/QuickStart.md)

The compilation guide can be found under [doc/INSTALL.md](doc/INSTALL.md)

This is an unoffical fork of the uBee512 source. For more information, see the Microbee Forum at https://microbeetechnology.com.au/forum/

