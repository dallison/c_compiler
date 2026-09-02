//
//  aarch64_program.c
//  c_compiler
//

#include "aarch64_program.h"

#include <stdlib.h>
#include <string.h>

#include "aarch64_assembler.h"
#include "assembler.h"
#include "disassembler.h"
#include "elf.h"

static void DestroyInstruction(void* value) {
  AARCH64ProgramInstruction* instruction = value;
  StringDestruct(&instruction->symbol);
  StringDestruct(&instruction->symbolic_text);
  free(instruction);
}

static AssemblerSymbol* GetOrCreateSymbol(Assembler* assembler,
                                          const char* name) {
  AssemblerSymbol* symbol = AssemblerFindSymbol(assembler, name);
  if (symbol == NULL && assembler->object.pass == 1) {
    symbol = NewAssemblerSymbol(name, 0, SYM_TYPE(none), SYM_BIND(global), 0);
    AssemblerInsertSymbol(assembler, symbol);
  }
  return symbol;
}

static bool FixupRangeValid(Assembler* assembler, AARCH64FixupKind kind,
                            int64_t offset) {
  int bits;
  switch (kind) {
    case kAARCH64FixupRelocationOnly:
      return true;
    case kAARCH64FixupBranch26:
      bits = 28;
      break;
    case kAARCH64FixupBranch19:
      bits = 21;
      break;
    case kAARCH64FixupTestBranch14:
      bits = 16;
      break;
    case kAARCH64FixupADR21:
      bits = 21;
      break;
    case kAARCH64FixupADRP21:
      bits = 32;
      break;
    case kAARCH64FixupNone:
      return true;
  }
  int64_t minimum = -(1LL << (bits - 1));
  int64_t maximum = (1LL << (bits - 1)) - 1;
  if (offset < minimum || offset > maximum ||
      (kind != kAARCH64FixupADR21 && (offset & 3) != 0)) {
    AssemblerError(assembler, "AArch64 branch/address offset out of range");
    return false;
  }
  return true;
}

static uint32_t ApplyFixup(Assembler* assembler, uint32_t word,
                           AARCH64FixupKind kind, int64_t offset) {
  if (!FixupRangeValid(assembler, kind, offset)) {
    return word;
  }
  switch (kind) {
    case kAARCH64FixupNone:
    case kAARCH64FixupRelocationOnly:
      return word;
    case kAARCH64FixupBranch26:
      return (word & ~0x03ffffffu) |
             ((uint32_t)(offset >> 2) & 0x03ffffffu);
    case kAARCH64FixupBranch19:
      return (word & ~(0x7ffffu << 5)) |
             (((uint32_t)(offset >> 2) & 0x7ffffu) << 5);
    case kAARCH64FixupTestBranch14:
      return (word & ~(0x3fffu << 5)) |
             (((uint32_t)(offset >> 2) & 0x3fffu) << 5);
    case kAARCH64FixupADR21: {
      uint32_t immediate = (uint32_t)offset & 0x1fffffu;
      return (word & ~((3u << 29) | (0x7ffffu << 5))) |
             ((immediate & 3u) << 29) |
             (((immediate >> 2) & 0x7ffffu) << 5);
    }
    case kAARCH64FixupADRP21:
      // ADRP depends on final load addresses and is always linker-resolved.
      return word;
  }
  return word;
}

static void EmitInstruction(Assembler* assembler, const void* value) {
  const AARCH64ProgramInstruction* instruction = value;
  uint32_t word = instruction->word;
  if (instruction->fixup != kAARCH64FixupNone) {
    AssemblerSymbol* symbol =
        GetOrCreateSymbol(assembler, instruction->symbol.value);
    int32_t address = (int32_t)AssemblerCurrentAddress(assembler);
    bool known =
        symbol != NULL && symbol->defined &&
        symbol->section == assembler->object.current_section &&
        symbol->binding != SYM_BIND(weak) &&
        !instruction->force_relocation &&
        instruction->fixup != kAARCH64FixupADRP21 &&
        instruction->fixup != kAARCH64FixupRelocationOnly;
    if (known) {
      word = ApplyFixup(assembler, word, instruction->fixup,
                        symbol->value + instruction->addend - address);
    } else if (assembler->object.pass == ASM_OBJECT_FINAL_PASS &&
               symbol != NULL) {
      AssemblerAddRelocationForSymbol(
          assembler, symbol, instruction->relocation_type,
          assembler->object.current_section, address, instruction->addend);
    }
  }
  AssemblerEmitWord(assembler, assembler->object.current_section,
                    (int32_t)word);
}

static bool WriteInstruction(FILE* out, const void* value) {
  const AARCH64ProgramInstruction* instruction = value;
  if (instruction->symbolic_text.length != 0) {
    return fprintf(out, "\t%s\n", instruction->symbolic_text.value) >= 0;
  }
  DAsmInstruction decoded;
  if (!DAsmDisassembleInstruction(kDAsmAArch64, &instruction->word,
                                  sizeof(instruction->word), 0, &decoded) ||
      !decoded.known) {
    return fprintf(out, "\t.word 0x%08x\n", instruction->word) >= 0;
  }
  return fprintf(out, "\t%s\n", decoded.text) >= 0;
}

static const AsmModuleTargetOps kTargetOps = {
    .destroy_instruction = DestroyInstruction,
    .emit_instruction = EmitInstruction,
    .write_instruction = WriteInstruction,
    .assemble_text = AssembleAARCH64Instruction,
};

const AsmModuleTargetOps* AARCH64ProgramTargetOps(void) {
  return &kTargetOps;
}

void AARCH64ProgramEmitWord(AsmModule* module, uint32_t word) {
  AARCH64ProgramEmitFixup(module, word, kAARCH64FixupNone, 0, NULL, 0, false,
                          NULL);
}

void AARCH64ProgramEmitWordText(AsmModule* module, uint32_t word,
                                const char* symbolic_text) {
  AARCH64ProgramEmitFixup(module, word, kAARCH64FixupNone, 0, NULL, 0, false,
                          symbolic_text);
}

void AARCH64ProgramEmitFixup(AsmModule* module, uint32_t word,
                             AARCH64FixupKind fixup, int32_t relocation_type,
                             const char* symbol, int32_t addend,
                             bool force_relocation,
                             const char* symbolic_text) {
  AARCH64ProgramInstruction* instruction =
      calloc(1, sizeof(*instruction));
  if (instruction == NULL) {
    module->failed = true;
    return;
  }
  instruction->word = word;
  instruction->fixup = fixup;
  instruction->relocation_type = relocation_type;
  instruction->force_relocation = force_relocation;
  if (symbol != NULL) {
    AsmExpr expression;
    AsmExprInitSymbol(&expression, symbol, addend);
    instruction->addend = (int32_t)expression.addend;
    StringInit(&instruction->symbol, expression.symbol.value);
    AsmExprDestruct(&expression);
  } else {
    instruction->addend = addend;
  }
  if (symbolic_text != NULL) {
    StringInit(&instruction->symbolic_text, symbolic_text);
  }
  AsmModuleInstruction(module, instruction);
}
