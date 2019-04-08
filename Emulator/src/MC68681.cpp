#include "MC68681.h"

#include <iostream>
#include <iomanip>
#include <cstdint>

#include <glog/logging.h>

extern "C" {
  #include "musashi/m68k.h"
}

/**
 * Initializes the controller.
 */
MC68681::MC68681(Emulator *emulator) : BusPeripheral(emulator) {

}

/**
 * Cleans up sockets and associated resources.
 */
MC68681::~MC68681() {
  // close sockets
  if(this->uart1Sock) {
    close(this->uart1Sock);
  }

  if(this->uart2Sock) {
    close(this->uart2Sock);
  }
}

/**
 * Performs a write into the peripheral registers.
 */
void MC68681::busWrite(uint32_t addr, uint32_t data, bus_size_t size) {
  // validate bus size
  if(size != kBusSize8Bits) {
    throw BusError("DUART supports only 8 bit writes");
  }

  // get reg number
  uint8_t reg = (addr & 0x0F);
  VLOG(1) << "68681: Write reg $" << std::hex << ((int) reg) << std::setw(2) << ": $" << data;
}

/**
 * Returns the value of a bus read.
 */
uint32_t MC68681::busRead(uint32_t addr, bus_size_t size) {
  // validate bus size
  if(size != kBusSize8Bits) {
    throw BusError("DUART supports only 8 bit reads");
  }

  uint8_t reg = (addr & 0x0F);

  // TODO: output reg data
  VLOG(1) << "68681: Read reg $" << std::hex << ((int) reg) << std::setw(2) << ": $" << 0x00;

  // nothing yet
  return 0;
}
