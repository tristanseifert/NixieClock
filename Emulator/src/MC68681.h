/**
 * Emulation of the 68681 DUART. Timings are not correctly emulated, but the
 * general functionality is there.
 */
#ifndef MC68681_H
#define MC68681_H

#include "BusPeripheral.h"

#include <cstdint>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

class Emulator;

class MC68681 : public BusPeripheral {
  public:
    typedef enum {
      kChannelA = 0,
      kChannelB = 1
    } ChannelType;

  private:
    static const unsigned int uartAPort = 4200;
    static const unsigned int uartBPort = 4201;

  public:
    MC68681(Emulator *emulator);
    virtual ~MC68681();

    virtual void busWrite(uint32_t addr, uint32_t data, bus_size_t size);
    virtual uint32_t busRead(uint32_t addr, bus_size_t size);

  private:
    void modeRegWrite(ChannelType type, uint8_t data);
    void clockSelWrite(ChannelType type, uint8_t data);
    void commandWrite(ChannelType type, uint8_t data);
    uint8_t statusRead(ChannelType type);

    void uartWrite(ChannelType type, uint8_t write);
    uint8_t uartRead(ChannelType type);

    void startTimer(void);
    void stopTimer(void);

    void openSocket(ChannelType channel, unsigned int port);
    void readerThread(ChannelType channel);

  private:
    std::atomic_bool run = true;

    uint16_t timerPeriod = 0;
    uint8_t irqVector = 0;

    class {
      public:
        // listening socket
        int listenSocket = 0;
        // connection to client
        int socket = 0;

        // thread to read from socket
        std::thread *readerThread = nullptr;

        // are the receiver/transmitter on?
        bool txOn = false, rxOn = false;
        // receive and transmit FIFOs
        std::queue<uint8_t> rxFifo, txFifo;
        std::mutex rxFifoLock, txFifoLock;

        // error flags
        bool breakRx = false, parityErr = false, framingErr = false,
             overrunErr = false;

        // interrupt flags
        bool breakChangeIrq = false;

        // baud rate extend flags (BRG extend bit)
        bool baudExtendRx = false, baudExtendTx = false;

        // mode register pointer and data
        int modeRegPtr = 0;
    } channelState[2];

    friend void MC68681_ReaderThreadEntry(void *ctx, ChannelType channel);
};

#endif
