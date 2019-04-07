#include "Emulator.h"

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdexcept>
#include <cstdint>

#include <glog/logging.h>

extern "C" {
  #include "musashi/m68k.h"
}


static Emulator *gEmulator = nullptr;



extern "C" void m68k_instruction_hook(void);



/**
 * Sets up the emulator.
 */
Emulator::Emulator(std::string romFilePath, std::string nvramFilePath) {
  // ensure we haven't already been allocated
  CHECK(gEmulator == nullptr) << "Already have an allocated emulator";
  gEmulator = this;

  // load ROM
  this->loadROM(romFilePath);

  // set up CPU
  m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
  m68k_set_instr_hook_callback(m68k_instruction_hook);

  m68k_pulse_reset();
}


/**
 * Tears down the emulator.
 */
Emulator::~Emulator() {

}



/**
 * Loads the ROM file from disk.
 */
void Emulator::loadROM(const std::string path) {
  memset(this->memRom, 0xFF, sizeof(this->memRom));

  // read file
  VLOG(1) << "Reading ROM from `" << path << "`";
  std::fstream romFile(path, std::ios::in | std::ios::binary | std::ios::ate);

  if(!romFile.is_open()) {
    throw std::runtime_error("Couldn't open ROM file at " + path);
  }

  std::streamsize size = romFile.tellg();
  romFile.seekg(0, std::ios::beg);

  std::vector<char> rom(size);
  if(romFile.read(rom.data(), size)) {
    // write it into memory now
    memcpy(this->memRom, rom.data(), size);

    // also, extract stack and PC and set that
    this->initialSp = __builtin_bswap32(*((uint32_t *) rom.data()));
    this->initialPc = __builtin_bswap32(*((uint32_t *) (rom.data() + 4)));

    // m68k_set_reg(M68K_REG_PC, this->initialPc);
    // m68k_set_reg(M68K_REG_SP, this->initialSp);

    LOG(INFO) << "Initial PC = $" << std::hex << this->initialPc
              << "; stack = $" << this->initialSp << std::endl;
  } else {
    throw std::runtime_error("Couldn't read ROM file to memory");
  }

  // done
  romFile.close();
}

/**
 * Loads NVRAM from disk.
 */
void Emulator::loadNVRAM(const std::string path) {
  // TODO: implement this
}


/**
 * Dumps the registers from the CPU.
 */
void Emulator::getRegs(M68kRegs &regs) {
  // dump registers
  const size_t numRegs = 18;

  const m68k_register_t regIds[numRegs] = {
    M68K_REG_D0, M68K_REG_D1, M68K_REG_D2, M68K_REG_D3,
    M68K_REG_D4, M68K_REG_D5, M68K_REG_D6, M68K_REG_D7,

    M68K_REG_A0, M68K_REG_A1, M68K_REG_A2, M68K_REG_A3,
    M68K_REG_A4, M68K_REG_A5, M68K_REG_A6, M68K_REG_A7,

    M68K_REG_PC, M68K_REG_SR
  };
  uint32_t *regVals[numRegs] = {
    &regs.d0, &regs.d1, &regs.d2, &regs.d3,
    &regs.d4, &regs.d5, &regs.d6, &regs.d7,

    &regs.a0, &regs.a1, &regs.a2, &regs.a3,
    &regs.a4, &regs.a5, &regs.a6, &regs.a7,

    &regs.pc, &regs.sr
  };

  for(int i = 0; i < numRegs; i++) {
    // read reg
    *regVals[i] = m68k_get_reg(nullptr, regIds[i]);
  }
}


/**
 * Starts emulation.
 */
void Emulator::start(void) {
  while(this->run) {
    // run weed processor
    m68k_execute(100000);
  }
}

/**
 * Stops emulation.
 */
void Emulator::stop(void) {
  this->run = false;
}



/**
 * Instruction executed hook
 */
void Emulator::cpuExecutedInstruction(uint64_t address) {
  // disassemble
  char instrBuffer[48];
  memset(&instrBuffer, 0, sizeof(instrBuffer));

  int count = m68k_disassemble(instrBuffer, address, M68K_CPU_TYPE_68000);

  // log instruction and registers
  Emulator::M68kRegs regs;
  this->getRegs(regs);

  VLOG(2) << "TRACE: address = $" << std::hex << address << ": " << std::string(instrBuffer) << std::endl << regs;
}

/**
 * Memory access hook
 */
void Emulator::cpuHookMem(bool read, uint64_t addr, int size, int64_t value) {
  VLOG(2) << "MEM: " << (read ? "read" : "write") << " at address $" << std::hex << addr << ", size $" << size;
}

/**
 * Interrupt hook
 */
void Emulator::cpuInt(uint32_t intno) {
  if(intno == 61) {
    LOG(ERROR) << "Unsupported exception: " << intno;
    exit(-1);
  }

  LOG(INFO) << "Interrupt: " << intno;
}




/**
 * Outputs a register structure.
 */
std::ostream& operator<<(std::ostream& os, const Emulator::M68kRegs& regs) {
  // set up a temporary stream
  std::stringstream ss;
  ss << std::setfill('0') << std::right << std::hex
     << "D0: $" << std::setw(8) << regs.d0 << "\t"
     << "D1: $" << std::setw(8) << regs.d1 << std::endl
     << "D2: $" << std::setw(8) << regs.d2 << "\t"
     << "D3: $" << std::setw(8) << regs.d3 << std::endl
     << "D4: $" << std::setw(8) << regs.d4 << "\t"
     << "D5: $" << std::setw(8) << regs.d5 << std::endl
     << "D6: $" << std::setw(8) << regs.d6 << "\t"
     << "D7: $" << std::setw(8) << regs.d7 << std::endl
     << "A0: $" << std::setw(8) << regs.a0 << "\t"
     << "A1: $" << std::setw(8) << regs.a1 << std::endl
     << "A2: $" << std::setw(8) << regs.a2 << "\t"
     << "A3: $" << std::setw(8) << regs.a3 << std::endl
     << "A4: $" << std::setw(8) << regs.a4 << "\t"
     << "A5: $" << std::setw(8) << regs.a5 << std::endl
     << "A6: $" << std::setw(8) << regs.a6 << "\t"
     << "A7: $" << std::setw(8) << regs.a7 << std::endl
     << "PC: $" << std::setw(8) << regs.pc << "\t"
     << "SR: $" << std::setw(8) << regs.sr << std::endl;

  // print the buffer
  os << ss.str();

  return os;
}



/**
 * Returns the address of read/write buffers for fixed memories like ROM and
 * RAM.
 */
void *Get68kBuffer(bool isRead, uint32_t addr) {
  // limit address to 512K
  addr &= 0x07FFFF;

  // is this read access?
  if(isRead) {
    // ROM
    if(addr < 0x020000) {
      return &gEmulator->memRom[addr];
    }
    // RAM?
    else if(addr >= 0x060000 && addr < 0x07FFFF) {
      return &gEmulator->memRam[(addr - 0x060000)];
    }
  }
  // otherwise, it's a write
  else {
    // RAM?
    if(addr >= 0x060000 && addr < 0x07FFFF) {
      return &gEmulator->memRam[(addr - 0x060000)];
    }
  }

  // no success
  return nullptr;
}

/**
 * Reads from memory
 */
extern "C" unsigned int m68k_read_memory_8(unsigned int address) {
  VLOG(2) << "Read (8 bit) from $" << std::hex << address;

  // handle simple reads
  void *buf = Get68kBuffer(true, address);

  if(buf) {
    return *((uint8_t *) buf);
  }

  // we need to handle it elsehow
  LOG(WARNING) << "Unhandled 8-bit read from $" << std::hex << address;
  return 0;
}

extern "C" unsigned int m68k_read_memory_16(unsigned int address) {
  VLOG(2) << "Read (16 bit) from $" << std::hex << address;

  // handle simple reads
  void *buf = Get68kBuffer(true, address);

  if(buf) {
    return __builtin_bswap16(*((uint16_t *) buf));
  }

  // we need to handle it elsehow
  LOG(WARNING) << "Unhandled 16-bit read from $" << std::hex << address;
  return 0;
}

extern "C" unsigned int m68k_read_memory_32(unsigned int address) {
  VLOG(2) << "Read (32 bit) from $" << std::hex << address;

  // handle simple reads
  void *buf = Get68kBuffer(true, address);

  if(buf) {
    return __builtin_bswap32(*((uint32_t *) buf));
  }

  // we need to handle it elsehow
  LOG(WARNING) << "Unhandled 32-bit read from $" << std::hex << address;
  return 0;
}

extern "C" uint32_t m68k_read_disassembler_16(uint32_t addr) {
  return m68k_read_memory_16(addr);
}

extern "C" uint32_t m68k_read_disassembler_32(uint32_t addr) {
  return m68k_read_memory_32(addr);
}

/**
 * Writes to memory
 */
extern "C" void m68k_write_memory_8(unsigned int address, unsigned int value) {
  VLOG(2) << "Write (8 bit) to $" << std::hex << address << " = $" << value;

  // handle simple writes
  void *buf = Get68kBuffer(false, address);

  if(buf) {
    *((uint8_t *) buf) = (value & 0xFF);
    return;
  }

  // yikes
  LOG(WARNING) << "Unhandled 8-bit write to $" << std::hex << address;
}

extern "C" void m68k_write_memory_16(unsigned int address, unsigned int value) {
  VLOG(2) << "Write (16 bit) to $" << std::hex << address << " = $" << value;

  // handle simple writes
  void *buf = Get68kBuffer(false, address);

  if(buf) {
    *((uint16_t *) buf) = __builtin_bswap16(value & 0xFFFF);
    return;
  }

  // yikes
  LOG(WARNING) << "Unhandled 16-bit write to $" << std::hex << address;
}

extern "C" void m68k_write_memory_32(unsigned int address, unsigned int value) {
  VLOG(2) << "Write (32 bit) to $" << std::hex << address << " = $" << value;

  // handle simple writes
  void *buf = Get68kBuffer(false, address);

  if(buf) {
    *((uint32_t *) buf) = __builtin_bswap32(value & 0xFFFFFFFF);
    return;
  }

  // yikes
  LOG(WARNING) << "Unhandled 32-bit write to $" << std::hex << address;
}

/**
 * Called when RESET is invoked
 */
extern "C" void m68k_reset_called(void) {
  LOG(INFO) << "RESET instruction called!";
}

/**
 * Called before each instruction
 */
extern "C" void m68k_instruction_hook(void) {
  // get PC
  uint32_t pc = 0;

  pc = m68k_get_reg(nullptr, M68K_REG_PC);

  // run callback
  gEmulator->cpuExecutedInstruction(pc);
}
