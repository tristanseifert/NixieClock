#include "MC68681.h"

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <queue>
#include <mutex>
#include <thread>

#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <glog/logging.h>

extern "C" {
  #include "musashi/m68k.h"
}

#define CHANNEL_NAME(x) ((x == kChannelA) ? "Channel A" : "Channel B")

void MC68681_ReaderThreadEntry(void *_ctx, MC68681::ChannelType channel);

/// log all register writes
#define LOG_REG_WRITE     0
/// log all register reads
#define LOG_REG_READ      0



/**
 * Initializes the controller.
 */
MC68681::MC68681(Emulator *emulator) : BusPeripheral(emulator) {
  // open listening sockets
  this->openSocket(kChannelA, MC68681::uartAPort);
}

/**
 * Cleans up sockets and associated resources.
 */
MC68681::~MC68681() {
  // clear run flag
  this->run = false;

  // close sockets and delete threads
  for(int i = 0; i < 2; i++) {
    // close regular socket
    if(this->channelState[i].socket) {
      close(this->channelState[i].socket);
    }

    // close listening socket
    if(this->channelState[i].listenSocket) {
      close(this->channelState[i].listenSocket);
    }

    // delete thread
    if(this->channelState[i].readerThread) {
      delete this->channelState[i].readerThread;
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
      outData = this->statusRead(kChannelA);
      break;
    // status register, channel B
    case 0x09:
      outData = this->statusRead(kChannelB);
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

  // XXX: we don't really care about what's written to this register
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
        std::queue<uint8_t>().swap(this->channelState[type].rxFifo);
        break;
      // reset transmitter
      case 0b0011:
        VLOG(2) << CHANNEL_NAME(type) << ": reset tx";

        this->channelState[type].txOn = false;
        std::queue<uint8_t>().swap(this->channelState[type].txFifo);
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
 * Gets the status register for the given channel.
 */
uint8_t MC68681::statusRead(ChannelType type) {
  uint8_t status = 0;

  // break received?
  if(this->channelState[type].breakRx) {
    status |= (1 << 7);
  }
  // framing error?
  if(this->channelState[type].framingErr) {
    status |= (1 << 6);
  }
  // parity error?
  if(this->channelState[type].parityErr) {
    status |= (1 << 5);
  }
  // overrun error?
  if(this->channelState[type].overrunErr) {
    status |= (1 << 4);
  }

  // tx fifo empty AND tx enabled?
  if(this->channelState[type].txFifo.empty() && this->channelState[type].txOn) {
    status |= (1 << 3);
  }
  // tx ready when tx fifo has less than 2 characters
  if(this->channelState[type].txFifo.size() < 3 && this->channelState[type].txOn) {
    status |= (1 << 2);
  }
  // FFULL set if if there are 3 bytes in the RX fifo
  if(this->channelState[type].rxFifo.size() <= 3) {
    status |= (1 << 1);
  }
  // receiver ready bit (at least one byte ready)
  if(this->channelState[type].rxFifo.size() > 0) {
    status |= (1 << 0);
  }

  return status;
}


/**
 * Writes a byte out to the UART TX FIFO.
 */
void MC68681::uartWrite(ChannelType type, uint8_t write) {
  int err;

  // push onto queue
  this->channelState[type].txFifo.push(write);

  // pop from queue and transmit lol
  CHECK(this->channelState[type].socket != 0) << "No output socket";

  uint8_t data = this->channelState[type].txFifo.front();
  this->channelState[type].txFifo.pop();

  err = ::write(this->channelState[type].socket, &data, sizeof(data));
  PCHECK(err > 0) << "Error writing to socket";
}
/**
 * Fetches a byte out of the UART holding register.
 */
uint8_t MC68681::uartRead(ChannelType type) {
  // return character if there is one
  if(this->channelState[type].rxFifo.empty() == false) {
    uint8_t byte = this->channelState[type].rxFifo.front();
    this->channelState[type].rxFifo.pop();

    return byte;
  }

  // nothing in the FIFO
  LOG(ERROR) << CHANNEL_NAME(type) << ": attempted read with nothing in FIFO";
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



/**
 * Opens the listening socket for the given UART.
 */
void MC68681::openSocket(ChannelType channel, unsigned int port) {
  int sockfd, connfd, err;
  socklen_t cliLen;
  struct sockaddr_in servaddr, cli;

  int yes = 1;

  LOG(INFO) << CHANNEL_NAME(channel) << ": listening on port " << port;

  // create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  PCHECK(sockfd != -1) << "Error creating socket";

  this->channelState[channel].listenSocket = sockfd;

  // enable the SO_REUSEADDR option
  err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  PCHECK(err == 0) << "Error setting SO_REUSEADDR";

  // enable the TCP_NODELAY option
  err = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
  PCHECK(err == 0) << "Error setting TCP_NODELAY";

  // assign address
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  err = bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
  PCHECK(err == 0) << "Error binding socket";

  // listen
  err = listen(sockfd, 5);
  PCHECK(err == 0) << "Error listening socket";

  // wait for a connection
  LOG(WARNING) << "Waiting for connection at port " << port;

  connfd = accept(sockfd, (struct sockaddr *) &cli, &cliLen);
  PCHECK(connfd != -1) << "Error accepting connection";

  // assign it
  this->channelState[channel].socket = connfd;
  VLOG(1) << "Accepted socket: " << connfd;

  // start a worker thread
  this->channelState[channel].readerThread = new std::thread(MC68681_ReaderThreadEntry, this, channel);
}

/**
 * Entry point for the reader thread.
 *
 * ctx contains a pointer to the MC68681 instance.
 */
void MC68681_ReaderThreadEntry(void *_ctx, MC68681::ChannelType channel) {
  // TODO: actually figure out what channel is being used
  MC68681 *ctx = static_cast<MC68681 *>(_ctx);

  // call into handler
  ctx->readerThread(channel);
}

/**
 * Main loop for the reader thread
 */
void MC68681::readerThread(ChannelType channel) {
  int err;

  int sock = this->channelState[channel].socket;
  CHECK(sock != 0) << "Error getting read socket for " << CHANNEL_NAME(channel);

  while(this->run) {
    uint8_t byte = 0;

    // read from socket
    err = ::read(sock, &byte, sizeof(byte));
    PLOG_IF(ERROR, (err == -1)) << "Error reading from socket for " << CHANNEL_NAME(channel);

    VLOG(2) << "Received byte: $" << std::hex << ((unsigned int) byte);

    // if not an error, push it
    if(err == 1) {
      // push into queue
      {
        std::lock_guard<std::mutex> guard(this->channelState[channel].rxFifoLock);
        this->channelState[channel].rxFifo.push(byte);
      }

      // TODO: handle irq's, etc
    }
  }
}
