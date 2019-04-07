This directory contains source to a simple emulator of the hardware. The emulation is relatively incomplete, and does not emulate some aspects (timings) of the hardware correctly, but it should be good enough to test the basic functioning of code.

It depends on [Unicorn](http://www.unicorn-engine.org) for CPU emulation. Additionally, [glog](https://github.com/google/glog) is used for logging.

## Usage
Invoke the binary, specifying the required information via these command line flags:

- `-r`: Path to boot ROM file
- `-n`: Path to NVRAM file; created if it doesn't already exist
- `-h`: Prints help
