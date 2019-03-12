#!/bin/sh

# build loader
echo "Assembling loader..."
vasmm68k_mot -Fbin -m68008 -o loader.bin -L loader.lst loader.68k

# build app
echo "\n\nAssembling application..."
vasmm68k_mot -Fbin -m68008 -o app.bin -L app.lst app.68k
