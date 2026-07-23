//
//  aarch64_target.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseAARCH64ed.
//

#include "aarch64_target.h"

#include <assert.h>
#include <string.h>
#include "aarch64_assembler.h"
#include "aarch64_codegen.h"
#include "aarch64_emitter.h"
#include "aarch64_reg_alloc.h"
#include "common_emitter.h"

static void* GenerateCode(Generator* gen) {
  AARCH64Generator* g = NewAARCH64Generator(gen);
  AARCH64Lower(g, gen);
  return g;
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  AARCH64Emitter emitter;
  AARCH64EmitterInit(&emitter, (AARCH64Generator*)code);
  AARCH64PrintFunction(&emitter, asm_file);
  AARCH64EmitterDestruct(&emitter);
}

static COMPILER_UNUSED void FilePrinter(int index, File* file, void* data) {
  FILE* fp = data;
  if (index == 0) {
    // Don't emit index 0 as this is the current file.
    return;
  }
  fprintf(fp, "\t.file %d \"%s\"\n", index, file->name.value);
}

static FILE* CreateAssemblyFile(String* src_file, String* asm_file) {
  FILE* fp = EmitAssemblyFile(src_file, asm_file);
  if (fp == NULL) {
    return NULL;
  }
    
  fprintf(fp, ".PCbegin:\n");
  return fp;
}

static bool Assemble(String* asm_filename, String* object_filename) {
  AARCH64Assembler assembler;
  AARCH64AssemblerInit(&assembler, asm_filename, object_filename);
  AssemblerRun(&assembler.base, AssembleAARCH64Instruction);

  int num_errors = assembler.base.num_errors;
  AARCH64AssemblerDestruct(&assembler);
  return num_errors == 0;
}


static void Cleanup(void* code) { AARCH64GeneratorDelete(code); }

static void HandleOptions(Vector* options) {
  
}

// Create a new AARCH64 target.  The functions are called by the
// compiler.
CompilerTarget* NewAARCH64Target() {
  CompilerTarget* target = malloc(sizeof(CompilerTarget));
  StringInit(&target->name, "aarch64");
  target->pointer_size = 8;
  target->int_size = 4;
  target->bool_size = 4;
  target->short_size = 2;
  target->long_size = 8;
  target->long_long_size = 8;
  target->float_size = 4;
  target->double_size = 8;
  target->wchar_size = 4;
  target->stack_alignment = 16;
  target->code_preference = kCodeForSpeed;
  target->call_return_fixed_reg = true;

  target->options = NULL;

  target->keep_ssa = false;
  target->ir_optimizations.gvn = true;
  target->ir_optimizations.sccp = true;
  target->ir_optimizations.const_prop = true;
  target->ir_optimizations.code_motion = true;
  target->ir_optimizations.tail_call = true;
  target->ir_optimizations.dce = true;
  target->ir_optimizations.copy_prop = true;
  // Preheader LICM currently lengthens C++ member-loop live ranges enough to
  // expose allocator aliasing; keep the architecture-neutral analysis and IV
  // cleanup enabled while gating that motion.
  target->ir_optimizations.loop_preheaders = false;
  target->ir_optimizations.induction_vars = true;
  target->ir_optimizations.derived_induction_vars = false;
  // The aarch64 backend emits ELF objects whose references and function
  // labels use unprefixed names; prefixing only the data-symbol definitions
  // (via the common emitter) made every global/common symbol unresolvable at
  // link time.  Match the other ELF targets (riscv/x86_64) and use no prefix.
  target->prepend_underscore = false;
  target->plain_char_is_signed = false;
  target->flags = 0;
  target->alignment = 8;
  target->codegen = GenerateCode;
  target->emit_function_assembly = EmitFunctionAssembly;
  target->assemble = Assemble;
  target->cleanup = Cleanup;
  target->create_asm_file = CreateAssemblyFile;
  target->emit_static_variable = EmitStaticVariable;
  target->emit_bss_space = EmitBSSVariable;
  target->emit_data_start = EmitDataStart;
  target->emit_literals_start = EmitStringLiteralSection;
  target->emit_literal = EmitLiteral;
  target->emit_cxx_thunks = AARCH64PrintCXXAdjustorThunks;
  target->emit_debug = EmitDebug;
  target->emit_tdata_start = EmitTlsDataStart;
  target->emit_tbss_start = EmitTlsBSSStart;
  target->emit_tls_variable = EmitTlsVariable;
  target->emit_tbss_space = EmitTlsBSSVariable;
  target->handle_options = HandleOptions;
  return target;
}
