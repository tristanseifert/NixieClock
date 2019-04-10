#include "MC68681.h"

#include <iostream>
#include <iomanip>
#include <cstdint>

#include <glog/logging.h>

extern "C" {
  #include "musashi/m68k.h"
}

#define CHANNEL_NAME(x) ((x == kChannelA) ? "Channel A" : "Channel B")

/// log all register writes
#define LOG_REG_WRITE     0
/// log all register reads
#define LOG_REG_READ      0



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
  for(int i = 0; i < 2; i++) {
    if(this->channelState[i].socket) {
      close(this->channelState[i].socket);
    }
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
#if LOG_REG_WRITE
  VLOG(1) << "write reg $" << std::hex << ((int) reg) << std::setw(2) << ": $" << data;
#endif

  // handle register
  switch(reg) {
    // mode register, channel A
    case 0x00:
      this->modeRegWrite(kChannelA, data);
      break;
    // mode register, channel B
    case 0x08:
      this->modeRegWrite(kChannelB, data);
      break;

    // clock select, channel A
    case 0x01:
      this->clockSelWrite(kChannelA, data);
      break;
    // clock select, channel B
    case 0x09:
      this->clockSelWrite(kChannelB, data);
      break;

    // command register, channel A
    case 0x02:
      this->commandWrite(kChannelA, data);
      break;
    // command register, channel B
    case 0x0A:
      this->commandWrite(kChannelB, data);
      break;

    // TX holding register, channel A
    case 0x03:
      this->uartWrite(kChannelA, data);
      break;
    // TX holding register, channel B
    case 0x0B:
      this->uartWrite(kChannelB, data);
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
      LOG(FATAL) << CHANNEL_NAME(kChannelA) << "Reads from mode register are not implemented";
      break;
    // mode register, channel B
    case 0x08:
      LOG(FATAL) << CHANNEL_NAME(kChannelB) << "Reads from mode register are not implemented";
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
      outData = this->uartRead(kChannelA);
      break;
    // RX holding register, channel B
    case 0x0B:
      outData = this->uartRead(kChannelB);
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
      this->startTimer();
      break;
    // stop timer/counter
    case 0x0F:
      this->stopTimer();
      break;

    // should never reach this
    default:
      LOG(FATAL) << "Invalid register: $" << std::hex << reg;
      break;
  }

  // output reg data
#if LOG_REG_READ
  VLOG(1) << "read reg $" << std::hex << ((unsigned int) reg) << std::setw(2) << ": $" << ((unsigned int) outData);
#endif

  return outData;
}



/**
 * Handles a write to the channel's mode register.
 */
void MC68681::modeRegWrite(ChannelType type, uint8_t data) {
#if LOG_REG_WRITE
  VLOG(1) << CHANNEL_NAME(type) << " mode register: $" << std::hex
          << ((unsigned int) data);
#endif

  // TODO: implement
}

/**
 * Handles a write to the channel's clock selection register.
 */
void MC68681::clockSelWrite(ChannelType type, uint8_t data) {
#if LOG_REG_WRITE
  VLOG(1) << CHANNEL_NAME(type) << " clock select register: $" << std::hex
          << ((unsigned int) data);
#endif

  // TODO: implement
}

/**
 * Handles a write to a channel's command register.
 */
void MC68681::commandWrite(ChannelType type, uint8_t data) {
#if LOG_REG_WRITE
  VLOG(1) << CHANNEL_NAME(type) << " command register: $" << std::hex
          << ((unsigned int) data);
#endif

  // get command and rx/tx state and check
  uint8_t command = ((data & 0xF0) >> 4);
  uint8_t rxTxState = (data & 0x0F);

  if(command) {
    // handle the command
    switch(command) {
      // reset mode register write pointer
      case 0b0001:
        VLOG(2) << CHANNEL_NAME(type) << ": reset mode register write ptr";

        this->channelState[type].modeRegPtr = 0;
        break;
      // reset receiver
      case 0b0010:
        VLOG(2) << CHANNEL_NAME(type) << ": reset rx";

        this->channelState[type].rxOn = false;
        this->channelState[type].rxFifo.clear();
        break;
      // reset transmitter
      case 0b0011:
        VLOG(2) << CHANNEL_NAME(type) << ": reset tx";

        this->channelState[type].txOn = false;
        this->channelState[type].txFifo.clear();
        break;
      // reset error flags
      case 0b0100:
        VLOG(2) << CHANNEL_NAME(type) << ": reset error flags";

        this->channelState[type].breakRx = false;
        this->channelState[type].parityErr = false;
        this->channelState[type].framingErr = false;
        this->channelState[type].overrunErr = false;
        break;
      // reset break change interrupt
      case 0b0101:
        VLOG(2) << CHANNEL_NAME(type) << ": reset break change irq";

        this->channelState[type].breakChangeIrq = false;
        break;

      // set RX BRG extend bit
      case 0b1000:
        VLOG(2) << CHANNEL_NAME(type) << ": set RX BRG extend bit";
        this->channelState[type].baudExtendRx = true;
        break;
      // clear RX BRG extend bit
      case 0b1001:
        VLOG(2) << CHANNEL_NAME(type) << ": clear RX BRG extend bit";
        this->channelState[type].baudExtendRx = false;
        break;

      // set TX BRG extend bit
      case 0b1010:
        VLOG(2) << CHANNEL_NAME(type) << ": set TX BRG extend bit";
        this->channelState[type].baudExtendTx = true;
        break;
      // clear TX BRG extend bit
      case 0b1011:
        VLOG(2) << CHANNEL_NAME(type) << ": clear TX BRG extend bit";
        this->channelState[type].baudExtendTx = false;
        break;

      // set standby mode
      case 0b1100:
        LOG(WARNING) << "Standby not implemented, ignoring request to enter standby mode";
        break;
      // set active mode
      case 0b1101:
        LOG(WARNING) << "Standby not implemented, ignoring request to enter active mode";
        break;

      // unhandled
      default:
        LOG(FATAL) << "Unknown command for " << CHANNEL_NAME(type) << ": $"
                   << std::hex << ((unsigned int) command);
        break;
    }
  }
  if(rxTxState) {
    // transmitter state change?
    if((rxTxState & 0b1100)) {
      // enable transmitter?
      if((rxTxState & 0b0100)) {
        this->channelState[type].txOn = true;
      }
      // disable it otherwise
      else if((rxTxState & 0b1000)) {
        this->channelState[type].txOn = false;
      }

      VLOG(2) << CHANNEL_NAME(type) << " TX enabled: " << this->channelState[type].txOn;
    }
    // receiver state change?
    if((rxTxState & 0b0011)) {
      // enable receiver?
      if((rxTxState & 0b0001)) {
        this->channelState[type].rxOn = true;
      }
      // disable it otherwise
      else if((rxTxState & 0b0010)) {
        this->channelState[type].rxOn = false;
      }

      VLOG(2) << CHANNEL_NAME(type) << " RX enabled: " << this->channelState[type].rxOn;
    }
  }
}


/**
 * Writes a byte out to the UART TX FIFO.
 */
void MC68681::uartWrite(ChannelType type, uint8_t write) {
  // TODO: implement
}
/**
 * Fetches a byte out of the UART holding register.
 */
uint8_t MC68681::uartRead(ChannelType type) {
  // TODO: implement
  return 0;
}



/**
 * Starts the timer.
 */
void MC68681::startTimer(void) {
  LOG(INFO) << "Timer started";
}
/**
 * Stops the timer.
 */
void MC68681::stopTimer(void) {
  LOG(INFO) << "Timer stopped";
}
