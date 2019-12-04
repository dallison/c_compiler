//
//  risc_v_target.c
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_target.h"
#include <assert.h>
#include "risc_v_assembler.h"
#include "risc_v_codegen.h"
#include "risc_v_emitter.h"
#include "risc_v_reg_alloc.h"

static void* GenerateCode(Generator* gen) {
  RVGenerator* rv = NewRVGenerator(gen);
  RVLower(rv, gen);
  return rv;
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  RVEmitter emitter;
  RVEmitterInit(&emitter, (RVGenerator*)code);
  RVPrintFunction(&emitter, asm_file);
  RVEmitterDestruct(&emitter);
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
  return fp;
}

static bool Assemble(String* asm_filename, String* object_filename) {
  RVAssembler assembler;
  RVAssemblerInit(&assembler, asm_filename, object_filename);
  AssemblerRun(&assembler.base, AssembleRVInstruction);

  int num_errors = assembler.base.num_errors;
  RVAssemblerDestruct(&assembler);
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
    }
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

static void Cleanup(void* code) { RVGeneratorDelete(code); }

static void EmitDebug(FILE* fp) {
  fprintf(fp, "\t.section \".debug_line\", \"aMS\", @progbits\n");
}

// Create a new RV target.  The functions are called by the
// compiler.
CompilerTarget* NewRVTarget() {
  CompilerTarget* target = malloc(sizeof(CompilerTarget));
  StringInit(&target->name, "RISC-V");
  target->pointer_size = 8;
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
