# NixieClock Software
This is a set of 68k assembly files that form the NixieClock firmware. This should be compiled with [vasm](http://sun.hasenbraten.de/vasm/).

It consists of two parts:

## Bootloader
The first 32K of flash are reserved for the bootloader. This bootloader initializes hardware and allows re-flashing of the system over the 68681's UARTs.

The first kilobyte of RAM ($60000 - $607FF) is reserved for use by the bootloader, and should not be tampered with by the application.

## Application
Any remaining space in flash is reserved for the application.

# Memory Map
Addresses are decoded as follows:

- $00000 - $1FFFF: ROM
- $20000 - $2FFFF: 68681
- $30000 - $3FFFF: RTC (DS1244)
- $40000 - $4FFFF: Nixie output registers
- $50000 - $5FFFF: VFD (CU24043-Y100)
- $60000 - $7FFFF: RAM

Memory is incompletely decoded, which means that this 512K space of address space is mirrored throughout the entire 32-bit address space of the CPU.
