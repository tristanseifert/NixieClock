/**
 * Emulates three dual tube drivers.
 */
#ifndef TUBEDRIVERS_H
#define TUBEDRIVERS_H

#include "BusPeripheral.h"

#include <cstdint>
#include <iostream>
#include <string>

class Emulator;

class TubeDrivers : public BusPeripheral {
  public:
    TubeDrivers(Emulator *emulator);
    virtual ~TubeDrivers();

    virtual void busWrite(uint32_t addr, uint32_t data, bus_size_t size);
    virtual uint32_t busRead(uint32_t addr, bus_size_t size);

  private:
    std::string dumpState(void);

    friend std::ostream& operator<<(std::ostream& os, const TubeDrivers& dt);

  private:
    // number of channels to emulate
    static const size_t numChannels = 4;

    class {
      public:
        uint8_t leftDigit, rightDigit;
        bool topColon, bottomColon;

        // brightness values for digits and colons
        uint8_t brightness[4];
    } state[TubeDrivers::numChannels];
};

#endif
