#!/bin/sh

# add date
rm -f builddate.txt
date > builddate.txt

# build loader
echo "Assembling loader..."
vasmm68k_mot -Fbin -m68008 -o loader.bin -L loader.lst loader.68k

# build app
echo "\n\nAssembling application..."
vasmm68k_mot -Fbin -m68008 -o app.bin -L app.lst app.68k

./tools/checksum app.bin

# combine them into a rom image
echo "\n\nCreating ROM image"
rm -f rom.bin
dd if=/dev/zero bs=131072 count=1 | LANG=C tr "\000" "\377" > rom.bin
dd if=loader.bin of=rom.bin seek=0 conv=notrunc
dd if=app.bin of=rom.bin seek=32768 bs=1 conv=notrunc
