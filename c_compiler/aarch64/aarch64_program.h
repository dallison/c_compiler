//
//  aarch64_program.h
//  c_compiler
//

#ifndef aarch64_program_h
#define aarch64_program_h

#include <stdbool.h>
#include <stdint.h>

#include "asm_module.h"

typedef enum {
  kAARCH64FixupNone,
  kAARCH64FixupRelocationOnly,
  kAARCH64FixupBranch26,
  kAARCH64FixupBranch19,
  kAARCH64FixupTestBranch14,
  kAARCH64FixupADR21,
  kAARCH64FixupADRP21,
} AARCH64FixupKind;

typedef struct {
  uint32_t word;
  AARCH64FixupKind fixup;
  int32_t relocation_type;
  int32_t addend;
  bool force_relocation;
  // Fixup target symbol, or NULL.
  char* symbol;
  // Optional symbolic spelling for instructions whose encoded immediate is
  // intentionally zero until the linker applies a relocation; NULL if absent.
  char* symbolic_text;
} AARCH64ProgramInstruction;

const AsmModuleTargetOps* AARCH64ProgramTargetOps(void);

void AARCH64ProgramEmitWord(AsmModule* module, uint32_t word);
void AARCH64ProgramEmitWordText(AsmModule* module, uint32_t word,
                                const char* symbolic_text);
void AARCH64ProgramEmitFixup(AsmModule* module, uint32_t word,
                             AARCH64FixupKind fixup, int32_t relocation_type,
                             const char* symbol, int32_t addend,
                             bool force_relocation,
                             const char* symbolic_text);

#endif /* aarch64_program_h */
