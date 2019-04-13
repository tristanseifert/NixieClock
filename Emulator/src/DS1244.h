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
    uint8_t getCurrentMagicBit(void) {
      int arrOffset = (this->magicSeqOffset / 8);

      uint8_t bit = DS1244::kMagicSeq[arrOffset];
      uint8_t mask = 1 << ((this->magicSeqOffset % 8));

      return (bit & mask) ? 1 : 0;
    }

  private:
    static const size_t kNvramSize = 0x7FFF;

    static const uint8_t constexpr kMagicSeq[8] = {
      0xC5, 0x3A, 0xA3, 0x5C, 0xC5, 0x3A, 0xA3, 0x5C
    };

  private:
    uint8_t *nvram = nullptr;

    /// where in the activation sequence are we?
    int magicSeqOffset = 0;
    /// has the bit activation pattern been seen yet?
    bool activated = false;
    /// how many more bits to shift in/out
    int bitsToShift = 0;
};

#endif
