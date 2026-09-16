//
//  disassembler_internal.h
//  c_compiler
//

#ifndef disassembler_internal_h
#define disassembler_internal_h

#include <stdarg.h>

#include "disassembler.h"

static inline uint16_t DAsmRead16LE(const unsigned char* p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t DAsmRead32LE(const unsigned char* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline uint64_t DAsmRead64LE(const unsigned char* p) {
  return (uint64_t)DAsmRead32LE(p) | ((uint64_t)DAsmRead32LE(p + 4) << 32);
}

static inline int64_t DAsmSignExtend(uint64_t value, int bits) {
  uint64_t mask = 1ULL << (bits - 1);
  return (int64_t)((value ^ mask) - mask);
}

void DAsmInitInstruction(DAsmInstruction* inst, const void* bytes,
                         size_t available, uint64_t address, size_t size);
void DAsmFormat(DAsmInstruction* inst, const char* fmt, ...);
void DAsmSetTarget(DAsmInstruction* inst, uint64_t target_address);
void DAsmUnknownInstruction(DAsmInstruction* inst, const char* directive_fmt,
                            uint64_t value);

bool DAsmDisassemble6502(const void* bytes, size_t length, uint64_t address,
                         DAsmInstruction* inst);
bool DAsmDisassembleRiscV(const void* bytes, size_t length, uint64_t address,
                          DAsmInstruction* inst);
bool DAsmDisassembleAArch64(const void* bytes, size_t length, uint64_t address,
                            DAsmInstruction* inst);
bool DAsmDisassembleARM(const void* bytes, size_t length, uint64_t address,
                        DAsmInstruction* inst);
bool DAsmDisassembleX86_64(const void* bytes, size_t length, uint64_t address,
                           DAsmInstruction* inst);
bool DAsmDisassembleXtensa(const void* bytes, size_t length, uint64_t address,
                           DAsmInstruction* inst);
bool DAsmDisassembleBPF(const void* bytes, size_t length, uint64_t address,
                        DAsmInstruction* inst);

#endif /* disassembler_internal_h */
