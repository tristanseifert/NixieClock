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
#include <unicorn/unicorn.h>

/**
 * Trace handler
 */
void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *ctx) {
  uc_err ucErr;

  // get context
  Emulator *emu = static_cast<Emulator *>(ctx);

  // disassemble the instruction
  uint8_t instr[16];
  memset(&instr, 0, sizeof(instr));

  ucErr = uc_mem_read(uc, address, &instr, sizeof(instr));
  CHECK(ucErr == 0) << "Error reading instruction: " << uc_strerror(ucErr);

  cs_insn *insn;
  int count = cs_disasm(emu->cs, instr, sizeof(instr), 0, 1, &insn);

  std::stringstream instrStr;

  if(count > 0) {
    instrStr << insn[0].mnemonic << " " << insn[0].op_str;

    // clean up
    cs_free(insn, count);
  }

  // log instruction and registers
  Emulator::M68kRegs regs;
  emu->getRegs(regs);

  VLOG(2) << "TRACE: address = $" << std::hex << address << ": " << instrStr.str() << std::endl << regs;
}


/**
 * Sets up the emulator.
 */
Emulator::Emulator(std::string romFilePath, std::string nvramFilePath) {
  cs_err csErr;
  uc_err ucErr;

  // set up disassembly context
  csErr = cs_open(CS_ARCH_M68K, CS_MODE_M68K_000, &this->cs);
  CHECK(csErr == CS_ERR_OK) << "Error creating disassembly context: " << cs_strerror(csErr);

  // create CPU context and set up memory regions
  ucErr = uc_open(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, &this->uc);
  CHECK(ucErr == 0) << "Error opening CPU: " << uc_strerror(ucErr);

  ucErr = uc_mem_map_ptr(this->uc, 0x000000, 0x020000, (UC_PROT_READ | UC_PROT_EXEC), this->memRom);
  CHECK(ucErr == 0) << "Error mapping ROM: " << uc_strerror(ucErr);

  memset(this->memRam, 0x00, sizeof(this->memRam));
  ucErr = uc_mem_map_ptr(this->uc, 0x060000, 0x020000, UC_PROT_ALL, this->memRam);
  CHECK(ucErr == 0) << "Error mapping RAM: " << uc_strerror(ucErr);

  this->loadROM(romFilePath);

  // add hook
  uc_hook trace;
  ucErr = uc_hook_add(uc, &trace, UC_HOOK_CODE, (void *) hook_code, this, 1, 0);
}


/**
 * Tears down the emulator.
 */
Emulator::~Emulator() {
  // clean up emulator state
  uc_close(this->uc);
  cs_close(&this->cs);
}



/**
 * Loads the ROM file from disk.
 */
void Emulator::loadROM(const std::string path) {
  uc_err ucErr;

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
    ucErr = uc_mem_write(this->uc, 0x000000, rom.data(), size);
    CHECK(ucErr == 0) << "Error loading ROM into memory map: " << uc_strerror(ucErr);

    // also, extract stack and PC and set that
    uint32_t stack = __builtin_bswap32(*((uint32_t *) rom.data()));
    this->initialPc = __builtin_bswap32(*((uint32_t *) (rom.data() + 4)));

    LOG(INFO) << "Initial PC = $" << std::hex << this->initialPc
              << "; stack = $" << stack << std::endl;

    ucErr = uc_reg_write(this->uc, UC_M68K_REG_A7, &stack);
    CHECK(ucErr == 0) << "Error setting stack pointer: " << uc_strerror(ucErr);

    ucErr = uc_reg_write(this->uc, UC_M68K_REG_PC, &this->initialPc);
    CHECK(ucErr == 0) << "Error setting PC: " << uc_strerror(ucErr);
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
  uc_err ucErr;

  // dump registers
  int regIds[18] = {
    UC_M68K_REG_D0, UC_M68K_REG_D1, UC_M68K_REG_D2, UC_M68K_REG_D3,
    UC_M68K_REG_D4, UC_M68K_REG_D5, UC_M68K_REG_D6, UC_M68K_REG_D7,

    UC_M68K_REG_A0, UC_M68K_REG_A1, UC_M68K_REG_A2, UC_M68K_REG_A3,
    UC_M68K_REG_A4, UC_M68K_REG_A5, UC_M68K_REG_A6, UC_M68K_REG_A7,

    UC_M68K_REG_PC, UC_M68K_REG_SR
  };
  void *regVals[18] = {
    &regs.d0, &regs.d1, &regs.d2, &regs.d3,
    &regs.d4, &regs.d5, &regs.d6, &regs.d7,

    &regs.a0, &regs.a1, &regs.a2, &regs.a3,
    &regs.a4, &regs.a5, &regs.a6, &regs.a7,

    &regs.pc, &regs.sr
  };

  ucErr = uc_reg_read_batch(this->uc, regIds, regVals, 18);
  // ucErr = uc_reg_read(this->uc, UC_M68K_REG_A0, &regs.a0);
  CHECK(ucErr == 0) << "Error reading registers: " << uc_strerror(ucErr);
}


/**
 * Starts emulation.
 */
void Emulator::start(void) {
  uc_err ucErr;

  while(this->run) {
    ucErr = uc_emu_start(this->uc, this->initialPc, 0xFFFFFF, 0, 0);
    CHECK(ucErr == 0) << "Error starting emulation: " << uc_strerror(ucErr);
  }
}

/**
 * Stops emulation.
 */
void Emulator::stop(void) {
  this->run = false;
}




/**
 * Outputs a register structure.
 */
std::ostream& operator<<(std::ostream& os, const Emulator::M68kRegs& regs) {
  // set up a temporary stream
  std::stringstream ss;
  ss << std::setfill('0') << std::right
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
