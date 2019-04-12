#include "VFD.h"

#include <cstdint>
#include <iostream>
#include <string>

#include <glog/logging.h>

/**
 * Sets up the VFD emulator.
 */
VFD::VFD(Emulator *_emulator) : BusPeripheral(_emulator) {

}
/**
 * Cleans up VFD resources
 */
VFD::~VFD() {

}


/**
 * Handles bus writes
 */
void VFD::busWrite(uint32_t addr, uint32_t data, bus_size_t size) {
  // validate bus size
  if(size != kBusSize8Bits) {
    throw BusError("VFD supports only 8 bit writes");
  }

}
/**
 * Handles bus reads
 */
uint32_t VFD::busRead(uint32_t addr, bus_size_t size) {
  throw BusError("VFD does not support reads");
  return 0;
}
