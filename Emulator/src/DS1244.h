/**
 * Emulates the DS1244 RTC/NVRAM combination device
 */
#ifndef DS1244_H
#define DS1244_H

#include "BusPeripheral.h"

#include <cstdint>
#include <iostream>
#include <string>

class Emulator;



class DS1244 : public BusPeripheral {
  public:
    DS1244(Emulator *emulator, uint8_t *buffer);
    virtual ~DS1244();

    virtual void busWrite(uint32_t addr, uint32_t data, bus_size_t size);
    virtual uint32_t busRead(uint32_t addr, bus_size_t size);

  private:
    static const size_t kNvramSize = 0x7FFF;

  private:
    uint8_t *nvram = nullptr;

    /// has the bit activation pattern been seen yet?
    bool activated = false;
    /// how many more bits to shift out
    int bitsToShift = 0;
};

#endif
