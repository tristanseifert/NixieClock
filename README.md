# Nixie Clock
A Nixie tube clock based on the Russian [IN-12A](http://www.tube-tester.com/sites/nixie/data/in-12a.htm) tubes. The clock is designed to be modular: one or more tube boards (which contain a shift register, tube mounts, and current limiting resistors) can be connected to one control board (which contains the high voltage supply, voltage regulation, and connection to a Raspberry Pi.)

The board is powered off of a regulated 12V supply, from which 5V (for the Raspberry Pi) and ~180V (for the anodes) is generated.
