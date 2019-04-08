/**
 * Emulation of the 68681 DUART. Timings are not correctly emulated, but the
 * general functionality is there.
 */
#ifndef MC68681_H
#define MC68681_H

#include "BusPeripheral.h"

class Emulator;

class MC68681 : public BusPeripheral {
  public:
    MC68681(Emulator *emulator);
    virtual ~MC68681();

    virtual void busWrite(uint32_t addr, uint32_t data, bus_size_t size);
    virtual uint32_t busRead(uint32_t addr, bus_size_t size);

  private:

  private:
    int uart1Sock = 0, uart2Sock = 0;

    uint16_t timerPeriod = 0;
    uint8_t irqVector = 0;
};

#endif
