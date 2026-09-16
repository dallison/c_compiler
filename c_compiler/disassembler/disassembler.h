//
//  disassembler.h
//  c_compiler
//

#ifndef disassembler_h
#define disassembler_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "elf.h"
#include "elf_reader.h"

#define DASM_MAX_BYTES 16
#define DASM_MAX_TEXT 160

typedef enum {
  kDAsmUnknown,
  kDAsm6502,
  kDAsmRiscV,
  kDAsmAArch64,
  kDAsmARM,
  kDAsmX86_64,
  kDAsmXtensa,
} DAsmArchitecture;

typedef struct {
  uint64_t address;
  size_t size;
  bool known;
  bool has_target_address;
  uint64_t target_address;
  unsigned char bytes[DASM_MAX_BYTES];
  size_t num_bytes;
  char text[DASM_MAX_TEXT];
} DAsmInstruction;

typedef struct {
  uint64_t address;
  const char* name;
  bool is_function;
} DAsmSymbol;

typedef struct {
  uint64_t address;
  const char* name;
} DAsmRelocation;

typedef struct {
  DAsmArchitecture arch;
  uint64_t start_address;
  uint64_t length;
  bool has_start_address;
  bool has_length;
  bool print_section_names;
  const DAsmSymbol* symbols;
  size_t num_symbols;
  const DAsmRelocation* relocations;
  size_t num_relocations;
} DAsmOptions;

bool DAsmArchitectureFromELFMachine(int machine, DAsmArchitecture* arch);
const char* DAsmArchitectureName(DAsmArchitecture arch);
DAsmArchitecture DAsmArchitectureFromName(const char* name);
size_t DAsmDefaultInstructionSize(DAsmArchitecture arch);

bool DAsmDisassembleInstruction(DAsmArchitecture arch, const void* bytes,
                                size_t length, uint64_t address,
                                DAsmInstruction* inst);
void DAsmPrintInstruction(FILE* fp, const DAsmInstruction* inst);
bool DAsmDisassembleELF(ELFReaderFile* elf, const DAsmOptions* options,
                        FILE* fp);
bool DAsmDisassembleFile(const char* filename, const DAsmOptions* options,
                         FILE* fp);

int DAsmToolMain(int argc, const char** argv, DAsmArchitecture arch,
                 const char* tool_name);

#endif /* disassembler_h */
