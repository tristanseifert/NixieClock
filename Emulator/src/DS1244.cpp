#include "DS1244.h"

#include <cstdint>
#include <iostream>
#include <string>

#include <glog/logging.h>


/**
 * Sets up the DS1244 emulator.
 *
 * The specified buffer must be at least 32K. It's used to back the NVRAM.
 */
DS1244::DS1244(Emulator *_emulator, uint8_t *_buffer) : BusPeripheral(_emulator), nvram(_buffer) {

}
/**
 * Cleans up DS1244 resources
 */
DS1244::~DS1244() {

}


/**
 * Handles bus writes
 */
void DS1244::busWrite(uint32_t addr, uint32_t data, bus_size_t size) {
  // validate bus size
  if(size != kBusSize8Bits) {
    throw BusError("DS1244 supports only 8 bit writes");
  }

  // limit address to 32K
  addr &= DS1244::kNvramSize;

  // write into NVRAM
  this->nvram[addr] = data;

  // pattern recognition (in DQ0)
  if(!this->activated) {
    uint8_t nextPatternBit = (data & 0b00000001);

    // does the current bit match the correct spot in the sequence?
    if(this->getCurrentMagicBit() == nextPatternBit) {
      this->magicSeqOffset++;

      // did we match all 64 bits of the sequence?
      if(this->magicSeqOffset == 64) {
        LOG(INFO) << "RTC phanthom clock activated";

        this->bitsToShift = 64;
        this->activated = true;
      }
    }
    // if not, reset sequence
    else {
      this->magicSeqOffset = 0;
    }
  }
  // handle writing to RTC registers
  else {
    // take it one bit at a time
    uint8_t dataBit = (data & 0b00000001);

    // decrement counter
    if(--this->bitsToShift <= 0) {
      LOG(INFO) << "Wrote 64 bits of data, deactivating phanthom clock";
      this->activated = false;
    }
  }
}
/**
 * Handles bus reads
 */
uint32_t DS1244::busRead(uint32_t addr, bus_size_t size) {
  // validate bus size
  if(size != kBusSize8Bits) {
    throw BusError("DS1244 supports only 8 bit reads");
  }

  // limit address to 32K
  addr &= DS1244::kNvramSize;

  // read from the NVRAM
  if(!this->activated) {
    return this->nvram[addr];
  } else {
    // TODO: shift out a bit

    // if no bits left, return to normal mode
    if(--this->bitsToShift <= 0) {
      LOG(INFO) << "Read 64 bits of data, deactivating phanthom clock";
      this->activated = false;
    }
  }

  return 0;
}
