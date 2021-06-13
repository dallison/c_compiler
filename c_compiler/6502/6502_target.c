//
//  6502_target.c
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_target.h"
#include <assert.h>
#include <string.h>
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
  FILE* fp;
  if (StringEqual(asm_file, "-")) {
    fp = stdout;
  } else {
    fp = fopen(asm_file->value, "w");
  }
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
  int addr = 0;  // TODO: allow override.
  static struct {
    char prefix;
    int num;
    int size;
  } registers[] = {{'b', _6502_NUM_B_REGS, 1}, {'i', _6502_NUM_I_REGS, 2},
                   {'l', _6502_NUM_L_REGS, 4}, {'x', _6502_NUM_X_REGS, 8},
                   {'f', _6502_NUM_F_REGS, 4}, {'d', _6502_NUM_D_REGS, 8}};

  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < registers[i].num; j++) {
      fprintf(fp, "\t.set __%c%d 0x%x\n", registers[i].prefix, j, addr);
      addr += registers[i].size;
    }
  }
  fprintf(fp, "\t.set __fp 0x%x\n", _6502_FP_REG);
  fprintf(fp, "\t.set __sp 0x%x\n", _6502_SP_REG);
  fprintf(fp, "\t.set __result 0x%x\n", _6502_RESULT_REG);
  fprintf(fp, "\t.set __t0 0x%x\n", _6502_T0_REG);
  fprintf(fp, "\t.set __t1 0x%x\n", _6502_T1_REG);
  fprintf(fp, "\t.set __t2 0x%x\n", _6502_T2_REG);
  fprintf(fp, "\t.set __t3 0x%x\n", _6502_T3_REG);
  fprintf(fp, "\t.set __mem_dest 0x%x\n", _6502_MDST_REG);
  fprintf(fp, "\t.set __mem_src 0x%x\n", _6502_MSRC_REG);
  fprintf(fp, "\t.set __mem_size 0x%x\n", _6502_MSZ_REG);

  fprintf(fp, "\n\n");
  return fp;
}

static bool Assemble(String* asm_filename, String* object_filename) {
  _6502Assembler assembler;
  _6502AssemblerInit(&assembler, asm_filename, object_filename);
  AssemblerRun(&assembler.base, Assemble6502Instruction);
  _6502AssemblerFinalize(&assembler);
  
  int num_errors = assembler.base.num_errors;
  _6502AssemblerDestruct(&assembler);
  return num_errors == 0;
}

static void DataStart(FILE* fp) { fprintf(fp, "\t.data\n"); }

static const char* VarName(InitializedStaticVariable* var, char* buf, size_t len) {
  if (var->is_local) {
    snprintf(buf, len, ".local.%s.%d", var->name.value, var->symbol_id);
  } else {
    strncpy(buf, var->name.value, len);
  }
  return buf;
}

static void StaticVariable(InitializedStaticVariable* var, FILE* fp) {
  char buf[256];
  fprintf(fp, "%s:\n", var->name.value);
  fprintf(fp, "\t.type   %s,@object\n", VarName(var, buf, sizeof(buf)));
  if (var->is_global) {
    fprintf(fp, "\t.global %s\n", VarName(var, buf, sizeof(buf)));
  } else {
    fprintf(fp, "\t.local  %s\n", VarName(var, buf, sizeof(buf)));
  }
  fprintf(fp, "\t.size   %s,%zd\n", VarName(var, buf, sizeof(buf)), var->size);
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
        fprintf(fp, "\t.short   %d\n", (int32_t)init->value.half);
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
        if (init->value.symbol->flags.is_local) {
          fprintf(fp, "\t.local %s\n", TargetSymbolName(init->value.symbol,  buf, sizeof(buf)));
        } else {
          fprintf(fp, "\t.global %s\n", TargetSymbolName(init->value.symbol,  buf, sizeof(buf)));
        }
        fprintf(fp, "\t.hword    %s\n", TargetSymbolName(init->value.symbol,  buf, sizeof(buf)));
        next_offset += 8;
        break;
      case kInitTypeString:
        fprintf(fp, "\t.hword    .str.%d\n", init->value.literal_id);
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
      }
    }
  }

  // Pad to full size.
  ssize_t pad = var->size - next_offset;
  if (pad > 0) {
    fprintf(fp, "\t.space  %zd\n", pad);
  }
  fprintf(fp, "\n");
}

static const char* VarName2(UnintializedStaticVariable* var, char* buf, size_t len) {
  if (var->is_local) {
    snprintf(buf, len, ".local.%s.%d", var->name.value, var->symbol_id);
  } else {
    strncpy(buf, var->name.value, len);
  }
  return buf;
}

static void BSSVariable(UnintializedStaticVariable* var, FILE* fp) {
  char buf[256];
  fprintf(fp, "\t.type   %s,@object\n", VarName2(var, buf, sizeof(buf)));
  if (var->is_global) {
    fprintf(fp, "\t.global %s\n", VarName2(var, buf, sizeof(buf)));
  } else {
    fprintf(fp, "\t.local  %s\n", VarName2(var, buf, sizeof(buf)));
  }
  fprintf(fp, "\t.comm   %s,%zd,%zd\n", VarName2(var, buf, sizeof(buf)), var->size,
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

static void EmitLiteral(Literal* literal, FILE* fp) {
  // A string literal might be disabled if it's used in an
  // asm statement and has alrady been emitted as assembly
  // language.
  if (literal->disabled) {
    return;
  }
  switch (literal->type) {
    case kLiteralString: {
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
      StringLiteral* slit = (StringLiteral*)literal;
      String escaped = {0};
      StringEscape(&slit->value, &escaped);
      fprintf(fp, "\t.asciz \"%s\"\n", escaped.value);
      StringDestruct(&escaped);

      // The literal is an object and the size includes the zero
      // at the end.
      fprintf(fp, "\t.type .str.%d, @object\n", literal->id);
      fprintf(fp, "\t.size .str.%d, %zd\n", literal->id,
              slit->value.length + 1);
      fprintf(fp, "\n");
      break;
    }
    case kLiteralBuffer: {
      fprintf(fp, ".lit.%d:\n", literal->id);
      BufferLiteral* buf = (BufferLiteral*)literal;
      for (size_t i = 0; i < buf->value.length; i++) {
        fprintf(fp, "\t.byte 0x%02x\n", buf->value.value[i] & 0xff);
      }
      fprintf(fp, "\t.type .lit.%d, @object\n", literal->id);
      fprintf(fp, "\t.size .lit.%d, %zd\n", literal->id, buf->value.length);
      fprintf(fp, "\n");
      break;
    }
  }
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
  target->int_size = 2;
  target->short_size = 2;
  target->bool_size = 1;
  target->long_size = 4;
  target->long_long_size = 8;
  target->stack_alignment = 1;
  target->code_preference = kCodeForSize;
  target->call_return_fixed_reg = false;
  target->callee_save = false;
  target->keep_ssa = true;
  target->codegen = GenerateCode;
  target->emit_function_assembly = EmitFunctionAssembly;
  target->assemble = Assemble;
  target->cleanup = Cleanup;
  target->create_asm_file = CreateAssemblyFile;
  target->emit_static_variable = StaticVariable;
  target->emit_bss_space = BSSVariable;
  target->emit_data_start = DataStart;
  target->emit_literals_start = StringLiteralSection;
  target->emit_literal = EmitLiteral;
  target->emit_debug = EmitDebug;
  return target;
}
