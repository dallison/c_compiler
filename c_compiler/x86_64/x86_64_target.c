//
//  x86_64_target.c
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "x86_64_target.h"
#include <assert.h>
#include <string.h>
#include "x86_64_assembler.h"
#include "x86_64_codegen.h"
#include "x86_64_emitter.h"
#include "x86_64_reg_alloc.h"
#include "common_emitter.h"

static void* GenerateCode(Generator* gen) {
  X86_64Generator* rv = NewX86_64Generator(gen);
  X86_64Lower(rv, gen);
  return rv;
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  X86_64Emitter emitter;
  X86_64EmitterInit(&emitter, (X86_64Generator*)code);
  X86_64PrintFunction(&emitter, asm_file);
  X86_64EmitterDestruct(&emitter);
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
  X86_64Assembler assembler;
  X86_64AssemblerInit(&assembler, asm_filename, object_filename);
  AssemblerRun(&assembler.base, AssembleX86_64Instruction);

  int num_errors = assembler.base.num_errors;
  X86_64AssemblerDestruct(&assembler);
  return num_errors == 0;
}


static void Cleanup(void* code) { X86_64GeneratorDelete(code); }

static void HandleOptions(Vector* options) {
  
}

// Create a new RV target.  The functions are called by the
// compiler.
CompilerTarget* NewX86_64Target() {
  CompilerTarget* target = malloc(sizeof(CompilerTarget));
  StringInit(&target->name, "x86-64");
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
  target->keep_ssa = false;
  target->ir_optimizations.gvn = true;
  target->ir_optimizations.const_prop = true;
  target->ir_optimizations.code_motion = true;
  target->ir_optimizations.tail_call = true;
  target->prepend_underscore = false;
  target->plain_char_is_signed = false;

  target->options = NULL;

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
  target->emit_debug = EmitDebug;
  target->emit_tdata_start = EmitTlsDataStart;
  target->emit_tbss_start = EmitTlsBSSStart;
  target->emit_tls_variable = EmitTlsVariable;
  target->emit_tbss_space = EmitTlsBSSVariable;
  target->handle_options = HandleOptions;
  return target;
}
