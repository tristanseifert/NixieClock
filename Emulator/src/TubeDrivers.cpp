#include "TubeDrivers.h"

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <string>

#include <glog/logging.h>

/// log all register writes
#define LOG_REG_WRITE     1

/**
 * Initializes the tube drivers
 */
TubeDrivers::TubeDrivers(Emulator *emulator) : BusPeripheral(emulator) {

}
/**
 * Cleans up
 */
TubeDrivers::~TubeDrivers() {

}




/**
 * Handles writes to the tube drivers.
 */
void TubeDrivers::busWrite(uint32_t addr, uint32_t data, bus_size_t size) {
  // validate bus size
  if(size != kBusSize8Bits) {
    throw BusError("Tube driver supports only 8 bit writes");
  }

  // get reg number
  uint8_t reg = (addr & 0x03);
  uint8_t channel = ((addr & 0x0C) >> 2);
#if LOG_REG_WRITE
  VLOG(1) << "write reg $" << std::hex << ((int) reg) << " on channel "
          << ((int) channel) << std::setw(2) << ": $" << data;
#endif

  CHECK(channel < TubeDrivers::numChannels) << "Invalid channel: " << channel;

  // handle registers
  switch(reg) {
    // digits
    case 0x00:
      this->state[channel].leftDigit = ((data & 0xF0) >> 4);
      this->state[channel].rightDigit = (data & 0x0F);
      break;
    // colons
    case 0x01:
      break;

    // PWM controller command
    case 0x02:
      LOG(WARNING) << "Unimplemented PWM controller command: $" << std::hex << ((unsigned int) data);
      break;

    // unhandled
    default:
      std::stringstream msg;
      msg << "Invalid register " << ((int) reg);

      throw BusError(msg.str());
      break;
  }

  VLOG(2) << *this;
}
/**
 * Reads are not implemented.
 */
uint32_t TubeDrivers::busRead(uint32_t addr, bus_size_t size) {
  throw BusError("Tube driver does not support reads");
  return 0;
}



/**
 * Outputs the state of the tube driver.
 */
std::ostream& operator<<(std::ostream& os, const TubeDrivers& dt) {
  // set up a temporary stream
  std::stringstream ss;

  for(int i = 0; i < TubeDrivers::numChannels; i++) {
    ss << "Channel " << ((unsigned int) i + 1) << ": ";

    ss << ((unsigned int) dt.state[i].leftDigit) << " ";
    ss << ((unsigned int) dt.state[i].rightDigit) << " ";

    ss << ((dt.state[i].topColon) ? "top colon " : " ");
    ss << ((dt.state[i].bottomColon) ? "bottom colon " : " ");
    ss << std::endl;
  }

  // print the buffer
  os << ss.str();

  return os;
}
