#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <atomic>
#include <cstdint>
#include <iostream>

#include <unicorn/unicorn.h>
#include <capstone/capstone.h>

class Emulator {
  public:
    class M68kRegs {
      public:
        /// data registers
        uint32_t d0 = 0, d1 = 0, d2 = 0, d3 = 0, d4 = 0, d5 = 0, d6 = 0, d7 = 0;
        /// address registers
        uint32_t a0 = 0, a1 = 0, a2 = 0, a3 = 0, a4 = 0, a5 = 0, a6 = 0, a7 = 0;
        /// program counter
        uint32_t pc = 0;
        /// status register
        uint32_t sr = 0;

      friend std::ostream& operator<<(std::ostream& os, const M68kRegs& dt);
    };

  public:
    Emulator(std::string romFilePath, std::string nvramFilePath);
    ~Emulator();

    void start(void);
    void stop(void);

    void getRegs(M68kRegs &regs);

  private:
    void loadROM(const std::string path);
    void loadNVRAM(const std::string path);

  private:
    uc_engine *uc = nullptr;
    csh cs;

    uint32_t initialPc = 0;

    std::atomic_bool run = true;

    uint8_t memRom[0x20000];
    uint8_t memRam[0x20000];

  friend void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *ctx);
};

#endif
