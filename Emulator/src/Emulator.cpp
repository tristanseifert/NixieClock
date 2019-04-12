#include "Emulator.h"

#include "MC68681.h"
#include "TubeDrivers.h"
#include "VFD.h"
#include "DS1244.h"

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

/// Should all instructions be logged?
#define LOG_INSTRUCTIONS        0
/// should we log memory accesses?
#define LOG_MEM_READ            0
#define LOG_MEM_WRITE           0


static Emulator *gEmulator = nullptr;



extern "C" void m68k_instruction_hook(void);
extern "C" void m68k_reset_called(void);



/**
 * Sets up the emulator.
 */
Emulator::Emulator(std::string romFilePath, std::string nvramFilePath) {
  // ensure we haven't already been allocated
  CHECK(gEmulator == nullptr) << "Already have an allocated emulator";
  gEmulator = this;

  // initialize peripherals
  this->duart = new MC68681(this);
  this->tubes = new TubeDrivers(this);
  this->vfd = new VFD(this);
  this->rtc = new DS1244(this, this->nvram);

  // load ROM
  this->loadROM(romFilePath);

  // set up CPU
  m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
  m68k_set_instr_hook_callback(m68k_instruction_hook);
  m68k_set_reset_instr_callback(m68k_reset_called);

  m68k_pulse_reset();
}


/**
 * Tears down the emulator.
 */
Emulator::~Emulator() {
  // clean up peripherals
  if(this->duart) {
    delete this->duart;
    this->duart = nullptr;
  }

  if(this->tubes) {
    delete this->tubes;
    this->tubes = nullptr;
  }

  if(this->vfd) {
    delete this->vfd;
    this->vfd = nullptr;
  }

  if(this->rtc) {
    delete this->rtc;
    this->rtc = nullptr;
  }
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
#if LOG_INSTRUCTIONS
  // disassemble
  char instrBuffer[48];
  memset(&instrBuffer, 0, sizeof(instrBuffer));

  int count = m68k_disassemble(instrBuffer, address, M68K_CPU_TYPE_68000);

  // log instruction and registers
  Emulator::M68kRegs regs;
  this->getRegs(regs);

  VLOG(2) << "TRACE: address = $" << std::hex << address << ": " << std::string(instrBuffer) << std::endl << regs;
#endif
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
 * Handles peripheral accesses.
 *
 * For reads, *data will deposit data in an uint32_t; for writes, it is read for
 * the data to write.
 *
 * Return negative number to abort memory access.
 */
int Handle68kPeriph(bool isRead, uint8_t width, uint32_t address, uint32_t *data) {
  // get width
  BusPeripheral::bus_size_t size;
  BusPeripheral *periph = nullptr;
  uint32_t offset;

  switch(width) {
    case 8:
      size = BusPeripheral::kBusSize8Bits;
      break;
    case 16:
      size = BusPeripheral::kBusSize16Bits;
      break;
    case 32:
      size = BusPeripheral::kBusSize32Bits;
      break;

    // should never get here
    default:
      LOG(FATAL) << "Invalid bus widthL " << width;
  }

  // DUART
  if(address >= 0x020000 && address <= 0x02FFFF) {
    offset = (address - 0x020000);
    periph = gEmulator->duart;
  }
  // tube drivers
  else if(address >= 0x040000 && address <= 0x04FFFF) {
    offset = (address - 0x040000);
    periph = gEmulator->tubes;
  }
  // VFD
  else if(address >= 0x050000 && address <= 0x05FFFF) {
    offset = (address - 0x050000);
    periph = gEmulator->vfd;
  }
  // RTC
  else if(address >= 0x030000 && address <= 0x03FFFF) {
    offset = (address - 0x030000);
    periph = gEmulator->rtc;
  }

  // if no peripheral found, abort
  if(!periph) {
    return -1;
  }

  // attempt bus operation
  try {
    // handle reads
    if(isRead) {
      *data = periph->busRead(offset, size);
    }
    // it's a write
    else {
      periph->busWrite(offset, *data, size);
    }
  } catch(BusPeripheral::BusError e) {
    LOG(ERROR) << "Bus error accessing peripheral at $" << std::hex << address
               << " for " << (isRead ? "read" : "write") << ": " << e.what();
  }

  // success
  return 0;
}

/**
 * Determines what to do for an unhandled bus transaction.
 */
void Unhandled68kTransaction(bool isRead, uint32_t address, uint32_t data, int width) {
  std::stringstream message;

  if(isRead) {
    message << "Unhandled " << width << "-bit read from $" << std::hex
            << address << std::endl;
  } else {
    message << "Unhandled " << width << "-bit write to $" << std::hex
            << address << " = $" << data << std::endl;
  }

  // dump state
  Emulator::M68kRegs regs;
  gEmulator->getRegs(regs);

  message << regs;

  // print message
  LOG(WARNING) << message.str();

  // infinite loop
  while(1) {}
}

/**
 * Reads from memory
 */
extern "C" unsigned int m68k_read_memory_8(unsigned int address) {
#if LOG_MEM_READ
  VLOG(2) << "Read (8 bit) from $" << std::hex << address;
#endif

  // handle simple reads
  void *buf = Get68kBuffer(true, address);

  if(buf) {
    return *((uint8_t *) buf);
  }

  // handle peripherals
  uint32_t tmp;

  if(Handle68kPeriph(true, 8, address, &tmp) >= 0) {
    return tmp;
  }

  // we need to handle it elsehow
  Unhandled68kTransaction(true, address, 0, 8);
  return 0;
}

extern "C" unsigned int m68k_read_memory_16(unsigned int address) {
#if LOG_MEM_READ
  VLOG(2) << "Read (16 bit) from $" << std::hex << address;
#endif

  // handle simple reads
  void *buf = Get68kBuffer(true, address);

  if(buf) {
    return __builtin_bswap16(*((uint16_t *) buf));
  }

  // handle peripherals
  uint32_t tmp;

  if(Handle68kPeriph(true, 16, address, &tmp) >= 0) {
    return __builtin_bswap16(tmp);
  }

  // we need to handle it elsehow
  Unhandled68kTransaction(true, address, 0, 16);
  return 0;
}

extern "C" unsigned int m68k_read_memory_32(unsigned int address) {
#if LOG_MEM_READ
  VLOG(2) << "Read (32 bit) from $" << std::hex << address;
#endif

  // handle simple reads
  void *buf = Get68kBuffer(true, address);

  if(buf) {
    return __builtin_bswap32(*((uint32_t *) buf));
  }

  // handle peripherals
  uint32_t tmp;

  if(Handle68kPeriph(true, 32, address, &tmp) >= 0) {
    return __builtin_bswap32(tmp);
  }

  // we need to handle it elsehow
  Unhandled68kTransaction(true, address, 0, 32);
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
extern "C" void m68k_write_memory_8(unsigned int address, unsigned int _value) {
  uint32_t value = (_value & 0xFF);

#if LOG_MEM_WRITE
  VLOG(2) << "Write (8 bit) to $" << std::hex << address << " = $" << value;
#endif

  // handle simple writes
  void *buf = Get68kBuffer(false, address);

  if(buf) {
    *((uint8_t *) buf) = value;
    return;
  }

  // handle peripherals
  if(Handle68kPeriph(false, 8, address, &value) >= 0) {
    return;
  }

  // yikes
  Unhandled68kTransaction(false, address, value, 8);
}

extern "C" void m68k_write_memory_16(unsigned int address, unsigned int _value) {
  uint32_t value = __builtin_bswap16(_value & 0xFFFF);

#if LOG_MEM_WRITE
  VLOG(2) << "Write (16 bit) to $" << std::hex << address << " = $" << value;
#endif

  // handle simple writes
  void *buf = Get68kBuffer(false, address);

  if(buf) {
    *((uint16_t *) buf) = value;
    return;
  }

  // handle peripherals
  if(Handle68kPeriph(false, 16, address, &value) >= 0) {
    return;
  }

  // yikes
  Unhandled68kTransaction(false, address, value, 16);
}

extern "C" void m68k_write_memory_32(unsigned int address, unsigned int _value) {
  uint32_t value = __builtin_bswap32(_value & 0xFFFFFFFF);

#if LOG_MEM_WRITE
  VLOG(2) << "Write (32 bit) to $" << std::hex << address << " = $" << value;
#endif

  // handle simple writes
  void *buf = Get68kBuffer(false, address);

  if(buf) {
    *((uint32_t *) buf) = value;
    return;
  }

  // handle peripherals
  if(Handle68kPeriph(false, 32, address, &value) >= 0) {
    return;
  }

  // yikes
  Unhandled68kTransaction(false, address, value, 32);
}

/**
 * Called when RESET is invoked
 */
extern "C" void m68k_reset_called(void) {
  LOG(INFO) << "RESET instruction called!";

  // print state
  std::stringstream message;

  // disassemble current address
  uint64_t address = m68k_get_reg(nullptr, M68K_REG_PC);

  char instrBuffer[48];
  memset(&instrBuffer, 0, sizeof(instrBuffer));

  int count = m68k_disassemble(instrBuffer, address, M68K_CPU_TYPE_68000);

  message << "Error at $" << std::hex << address << ": " << std::string(instrBuffer) << std::endl;

  // dump registers
  Emulator::M68kRegs regs;
  gEmulator->getRegs(regs);

  message << regs;

  // print message
  LOG(WARNING) << message.str();

  // loop forever
  while(1) {}
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
