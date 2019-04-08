/**
 * Abstract class that implements the interface to the 68k bus.
 */
#ifndef BUSPERIPHERAL_H
#define BUSPERIPHERAL_H

#include <cstdint>
#include <string>
#include <stdexcept>

class Emulator;

class BusPeripheral {
  public:
    class BusError : public std::runtime_error {
      public:
        BusError(std::string what) : runtime_error(what) {}
    };

  public:
    typedef enum {
      kBusSize8Bits,
      kBusSize16Bits,
      kBusSize32Bits,
    } bus_size_t;

  public:
    BusPeripheral(Emulator *_emulator) : emulator(_emulator) {}
    virtual ~BusPeripheral() {};

    virtual void busWrite(uint32_t addr, uint32_t data, bus_size_t size) = 0;
    virtual uint32_t busRead(uint32_t addr, bus_size_t size) = 0;

  protected:
    Emulator *emulator = nullptr;

  private:
};

#endif
