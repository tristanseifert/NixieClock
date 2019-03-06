# Nixie Clock
A Nixie tube clock based on the Russian [IN-12A](http://www.tube-tester.com/sites/nixie/data/in-12a.htm) tubes. The clock is designed to be modular: one or more tube boards (which contain value latches, high voltage drivers, tube mounts, and current limiting resistors) can be connected to the control board (which contains a full 68008-based minicomputer!)

There is also provision for an USB FIFO (FT245R) for communication with a PC, a 32K NVRAM+RTC (DS1244) for timekeeping and configuration, a 100Hz periodic interrupt, and a connection to a Noritake CU24043-Y100 (or any other display with the same pinout) VFD.

The board is powered off of a regulated 12V supply, from which 5V (for the Raspberry Pi) and ~180V (for the anodes) is generated.
