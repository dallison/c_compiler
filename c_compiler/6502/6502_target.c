//
//  6502_target.c
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_target.h"
#include <assert.h>
#include "6502_assembler.h"
#include "6502_codegen.h"
#include "6502_emitter.h"

static void* GenerateCode(Generator* gen) {
  _6502Generator* g = New6502Generator(gen);
  _6502Lower(g, gen);
  return g;
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  _6502Emitter emitter;
  _6502EmitterInit(&emitter, (_6502Generator*)code);
  _6502PrintFunction(&emitter, asm_file);
  _6502EmitterDestruct(&emitter);
}

static void FilePrinter(int index, File* file, void* data) {
  FILE* fp = data;
  if (index == 0) {
    // Don't emit index 0 as this is the current file.
    return;
  }
  fprintf(fp, "\t.file %d \"%s\"\n", index, file->name.value);
}

static FILE* CreateAssemblyFile(String* src_file, String* asm_file) {
  FILE* fp = fopen(asm_file->value, "w");
  if (fp == NULL) {
    return NULL;
  }
  
  // Emit a .file directive without the file index.  This tells the
  // assembler the name of the current file.
  fprintf(fp, "\t.file   \"%s\"\n", src_file->value);
  
  if (compiler->debug_output) {
    // Print all source files.
    SourceTraverseFiles(fp, FilePrinter);
  }
  fprintf(fp, "\t.text\n");
  if (compiler->pic) {
    fprintf(fp, "\t.option pic\n");
  }
  
  // Define the registers.
  int addr = 0;       // TODO: allow override.
  static struct {
    char prefix;
    int num;
    int size;
  } registers[] = {
    {'b', _6502_NUM_B_REGS, 1},
    {'a', _6502_NUM_A_REGS, 2},
    {'i', _6502_NUM_I_REGS, 4},
    {'x', _6502_NUM_X_REGS, 8},
    {'f', _6502_NUM_F_REGS, 4},
    {'d', _6502_NUM_D_REGS, 8}};
  
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < registers[i].num; j++) {
      fprintf(fp, "\t.set %c%d %d\n", registers[i].prefix, j, addr);
      addr += registers[i].size;
    }
  }
  fprintf(fp, "\t.set t2 %d\n", _6502_T2_REG);
  fprintf(fp, "\t.set t4 %d\n", _6502_T4_REG);
  fprintf(fp, "\t.set t8 %d\n", _6502_T8_REG);
  fprintf(fp, "\t.set ap %d\n", _6502_AP_REG);
  fprintf(fp, "\t.set fp %d\n", _6502_FP_REG);
  fprintf(fp, "\t.set sp %d\n", _6502_SP_REG);
  
  // Support subroutines.
  fprintf(fp, "\t.global __move_reg_8\n");
  fprintf(fp, "\t.global __push_reg_1\n");
  fprintf(fp, "\t.global __push_reg_2\n");
  fprintf(fp, "\t.global __push_reg_4\n");
  fprintf(fp, "\t.global __push_reg_8\n");
  fprintf(fp, "\t.global __pull_reg_1\n");
  fprintf(fp, "\t.global __pull_reg_2\n");
  fprintf(fp, "\t.global __pull_reg_4\n");
  fprintf(fp, "\t.global __pull_reg_8\n");
  fprintf(fp, "\t.global __load_zero_2\n");
  fprintf(fp, "\t.global __load_zero_3\n");
  fprintf(fp, "\t.global __load_zero_4\n");
  fprintf(fp, "\t.global __load_zero_5\n");
  fprintf(fp, "\t.global __load_zero_6\n");
  fprintf(fp, "\t.global __load_zero_7\n");
  fprintf(fp, "\t.global __enter\n");
  fprintf(fp, "\t.global __rts\n");
  fprintf(fp, "\n\n");
  return fp;
}

static bool Assemble(String* asm_filename, String* object_filename) {
  _6502Assembler assembler;
  _6502AssemblerInit(&assembler, asm_filename, object_filename);
  AssemblerRun(&assembler.base, Assemble6502Instruction);
  
  int num_errors = assembler.base.num_errors;
  _6502AssemblerDestruct(&assembler);
  return num_errors == 0;
}

static void DataStart(FILE* fp) { fprintf(fp, "\t.data\n"); }

static void StaticVariable(InitializedStaticVariable* var, FILE* fp) {
  fprintf(fp, "%s:\n", var->name.value);
  fprintf(fp, "\t.type   %s,@object\n", var->name.value);
  if (var->is_global) {
    fprintf(fp, "\t.global %s\n", var->name.value);
  } else {
    fprintf(fp, "\t.local  %s\n", var->name.value);
  }
  fprintf(fp, "\t.size   %s,%zd\n", var->name.value, var->size);
  int alignment = (int)var->alignment - 1;
  assert(alignment >= 0);
  // The p2align directive takes, as its first argument the number
  // of bits to align to.  We calculate this by counting the
  // lower order 1 bits in the alignment.
  int p2align_arg = 0;
  for (int i = 0; i < 16; i++) {
    if ((alignment & (1 << i)) != 0) {
      p2align_arg++;
    } else {
      break;
    }
  }
  
  fprintf(fp, "\t.p2align  %d\n", p2align_arg);
  int next_offset = 0;
  for (size_t i = 0; i < var->initializers.length; i++) {
    Initializer* init = var->initializers.value.p[i];
    if (init->offset > next_offset) {
      int diff = init->offset - next_offset;
      fprintf(fp, "\t.space  %d\n", diff);
      next_offset += diff;
    }
    switch (init->type) {
      case kInitTypeByte:
        fprintf(fp, "\t.byte   %d\n", (int32_t)init->value.byte);
        next_offset += 1;
        break;
      case kInitTypeHalf:
        fprintf(fp, "\t.half   %d\n", (int32_t)init->value.half);
        next_offset += 2;
        break;
      case kInitTypeWord:
        fprintf(fp, "\t.word   %d\n", init->value.word);
        next_offset += 4;
        break;
      case kInitTypeLong:
        fprintf(fp, "\t.long   %lld\n", (int64_t)init->value.byte);
        next_offset += 8;
        break;
      case kInitTypeSymbol:
        fprintf(fp, "\t.long    %s\n", init->value.symbol->name.value);
        next_offset += 8;
        break;
      case kInitTypeString:
        fprintf(fp, "\t.long    .str.%d\n", init->value.literal_id);
        next_offset += 8;
        break;
      case kInitTypeMemory: {
        int byte_count = 0;
        const char* sep = "";
        const int kByteLimit = 16;  // 16 bytes per line.
        for (size_t i = 0; i < init->value.memory.length; i++) {
          if (byte_count == 0) {
            fprintf(fp, "\t.byte ");
          }
          fprintf(fp, "%s0x%02x", sep, init->value.memory.value[i]);
          sep = ",";
          byte_count++;
          if (byte_count == kByteLimit) {
            byte_count = 0;
            fprintf(fp, "\n");
            sep = "";
          }
        }
        fprintf(fp, "\n");
        break;
      }    }
  }
  
  // Pad to full size.
  size_t pad = var->size - next_offset;
  if (pad > 0) {
    fprintf(fp, "\t.space  %zd\n", pad);
  }
  fprintf(fp, "\n");
}

static void BSSVariable(UnintializedStaticVariable* var, FILE* fp) {
  fprintf(fp, "\t.type   %s,@object\n", var->name.value);
  if (var->is_global) {
    fprintf(fp, "\t.global %s\n", var->name.value);
  } else {
    fprintf(fp, "\t.local  %s\n", var->name.value);
  }
  fprintf(fp, "\t.comm   %s,%zd,%zd\n", var->name.value, var->size,
          var->alignment);
  fprintf(fp, "\n");
}

static void StringLiteralSection(FILE* fp) {
  // String literals are in their own section.  The flags
  // mean:
  //  a: allocated in program
  //  M: can be merged with other rodata sections.
  //  S: contains strings.
  // These are used by the linker.
  fprintf(fp, "\t.section \".rodata\", \"aMS\", @progbits\n");
}

static void EmitLiteral(StringLiteral* literal, FILE* fp) {
  // A string literal might be disabled if it's used in an
  // asm statement and has alrady been emitted as assembly
  // language.
  if (literal->disabled) {
    return;
  }
  // Each string literal has its own symbol.  The symbol
  // is of the form ".str.xx" where 'xx' is the literal
  // id (assigned when the string literal is compiled).
  // A reference to this symbol is output to a movxc
  // instruction with a relocation so that the linker
  // can set the address.
  fprintf(fp, ".str.%d:\n", literal->id);
  
  // Print the literal in escaped form. Any non-printable
  // characters are encoded in hex or as their usual
  // ANSI C escape characters.
  String escaped;
  StringInit(&escaped, NULL);
  StringEscape(&literal->value, &escaped);
  fprintf(fp, "\t.asciz \"%s\"\n", escaped.value);
  StringDestruct(&escaped);
  
  // The literal is an object and the size includes the zero
  // at the end.
  fprintf(fp, "\t.type .str.%d, @object\n", literal->id);
  fprintf(fp, "\t.size .str.%d, %zd\n", literal->id, literal->value.length + 1);
  fprintf(fp, "\n");
}

static void Cleanup(void* code) { _6502GeneratorDelete(code); }

static void EmitDebug(FILE* fp) {
  fprintf(fp, "\t.section \".debug_line\", \"aMS\", @progbits\n");
}

// Create a new _6502 target.  The functions are called by the
// compiler.
CompilerTarget* New6502Target() {
  CompilerTarget* target = malloc(sizeof(CompilerTarget));
  StringInit(&target->name, "6502");
  target->pointer_size = 2;
  target->codegen = GenerateCode;
  target->emit_function_assembly = EmitFunctionAssembly;
  target->assemble = Assemble;
  target->cleanup = Cleanup;
  target->create_asm_file = CreateAssemblyFile;
  target->emit_static_variable = StaticVariable;
  target->emit_bss_space = BSSVariable;
  target->emit_data_start = DataStart;
  target->emit_literals_start = StringLiteralSection;
  target->emit_string_literal = EmitLiteral;
  target->emit_debug = EmitDebug;
  return target;
}
