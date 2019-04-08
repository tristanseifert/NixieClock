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

  // handle register
  switch(reg) {
    // mode register, channel A
    case 0x00:
      break;
    // mode register, channel B
    case 0x08:
      break;

    // clock select, channel A
    case 0x01:
      break;
    // clock select, channel B
    case 0x09:
      break;

    // command register, channel A
    case 0x02:
      break;
    // command register, channel B
    case 0x0A:
      break;

    // TX holding register, channel A
    case 0x03:
      break;
    // TX holding register, channel B
    case 0x0B:
      break;

    // Aux control register
    case 0x04:
      break;

    // interrupt mask register
    case 0x05:
      break;

    // counter/timer upper byte
    case 0x06:
      this->timerPeriod &= 0x00FF;
      this->timerPeriod |= ((data & 0xFF) << 8);
      break;

    // counter/timer lower byte
    case 0x07:
      this->timerPeriod &= 0xFF00;
      this->timerPeriod |= (data & 0xFF);
      break;

    // interupt vector register
    case 0x0C:
      this->irqVector = (data & 0xFF);
      break;

    // output port config
    case 0x0D:
      break;

    // set output bits (they're actually physically 0)
    case 0x0E:
      break;
    // clear output bits (they're actually physically 1)
    case 0x0F:
      break;

    // should never reach this
    default:
      LOG(FATAL) << "Invalid register: $" << std::hex << reg;
      break;
  }
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
  uint8_t outData = 0x00;

  // handle register
  switch(reg) {
    // mode register, channel A
    case 0x00:
      break;
    // mode register, channel B
    case 0x08:
      break;

    // status register, channel A
    case 0x01:
      break;
    // status register, channel B
    case 0x09:
      break;

    // masked interrupt status register, channel A
    case 0x02:
      break;

    // RX holding register, channel A
    case 0x03:
      break;
    // RX holding register, channel B
    case 0x0B:
      break;

    // input port change register
    case 0x04:
      break;

    // interrupt status register
    case 0x05:
      break;

    // counter/timer upper byte
    case 0x06:
      outData = ((this->timerPeriod & 0xFF00) >> 8);
      break;

    // counter/timer lower byte
    case 0x07:
      outData = (this->timerPeriod & 0x00FF);
      break;

    // interupt vector register
    case 0x0C:
      outData = this->irqVector;
      break;

    // input port data
    case 0x0D:
      break;

    // start timer/counter
    case 0x0E:
      break;
    // stop timer/counter
    case 0x0F:
      break;

    // should never reach this
    default:
      LOG(FATAL) << "Invalid register: $" << std::hex << reg;
      break;
  }

  // output reg data
  VLOG(1) << "68681: Read reg $" << std::hex << ((int) reg) << std::setw(2) << ": $" << ((unsigned int) outData);
  return outData;
}
