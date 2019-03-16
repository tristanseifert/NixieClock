# Loader API
Application software can invoke various services in the bootloader (mainly pertaining to serial IO, but also for other services such as entering the monitor) by branching (`jsr`) the API entry point at `$7F80` in ROM.

When calling this routine, place the desired routine number into `d0`. Only the low byte is used to determine the appropriate function to call. The return code of the function (0 for success, negative for errors, positive for other indications) is in d0 when the call returns.

To immediately trap into the monitor, trap 0 can be taken.

## List of functions
### `$00`: No-Op
Does nothing.
