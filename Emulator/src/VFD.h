/**
 * VFD driver
 */
#ifndef VFD_H
#define VFD_H

#include "BusPeripheral.h"

#include <cstdint>
#include <iostream>
#include <string>

class Emulator;



class VFD : public BusPeripheral {
  public:
    VFD(Emulator *emulator);
    virtual ~VFD();

    virtual void busWrite(uint32_t addr, uint32_t data, bus_size_t size);
    virtual uint32_t busRead(uint32_t addr, bus_size_t size);

};

#endif
