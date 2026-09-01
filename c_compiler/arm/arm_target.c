//
//  arm_target.c
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "arm_target.h"
#include <assert.h>
#include <string.h>
#include "arm_assembler.h"
#include "arm_codegen.h"
#include "arm_emitter.h"
#include "arm_reg_alloc.h"
#include "common_emitter.h"

static void* GenerateCode(Generator* gen) {
  ARMGenerator* rv = NewARMGenerator(gen);
  ARMLower(rv, gen);
  return rv;
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  ARMEmitter emitter;
  ARMEmitterInit(&emitter, (ARMGenerator*)code);
  ARMPrintFunction(&emitter, asm_file);
  ARMEmitterDestruct(&emitter);
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
  ARMAssembler assembler;
  ARMAssemblerInit(&assembler, asm_filename, object_filename);
  AssemblerRun(&assembler.base, AssembleARMInstruction);

  int num_errors = assembler.base.num_errors;
  ARMAssemblerDestruct(&assembler);
  return num_errors == 0;
}


static void Cleanup(void* code) { ARMGeneratorDelete(code); }

static void HandleOptions(Vector* options) {
  
}

// Create a new RV target.  The functions are called by the
// compiler.
CompilerTarget* NewARMTarget() {
  CompilerTarget* target = malloc(sizeof(CompilerTarget));
  StringInit(&target->name, "arm");
  target->pointer_size = 4;
  target->int_size = 4;
  target->bool_size = 1;
  target->short_size = 2;
  target->long_size = 4;
  target->long_long_size = 8;
  target->float_size = 4;
  target->double_size = 8;
  target->wchar_size = 4;
  target->stack_alignment = 8;
  target->code_preference = kCodeForSpeed;
  target->call_return_fixed_reg = true;
  target->keep_ssa = false;
  target->ir_optimizations.gvn = true;
  target->ir_optimizations.sccp = true;
  target->ir_optimizations.const_prop = true;
  target->ir_optimizations.code_motion = true;
  target->ir_optimizations.tail_call = true;
  target->ir_optimizations.dce = true;
  target->ir_optimizations.copy_prop = true;
  target->ir_optimizations.loop_preheaders = true;
  target->ir_optimizations.induction_vars = true;
  // ARM benefits from replacing a per-iteration multiply/add with a pointer
  // recurrence; its allocator keeps the invented pointer out of memory.
  target->ir_optimizations.derived_induction_vars = true;
  target->prepend_underscore = false;
  target->plain_char_is_signed = false;

  target->options = NULL;

  target->flags = 0;
  target->alignment = 8;
  target->codegen = GenerateCode;
  target->prepare_function_emission = NULL;
  target->emit_function_assembly = EmitFunctionAssembly;
  target->assemble = Assemble;
  target->cleanup = Cleanup;
  target->create_asm_file = CreateAssemblyFile;
  target->emit_assembly_preamble = NULL;
  target->assemble_string = NULL;
  target->emit_static_variable = EmitStaticVariable;
  target->emit_bss_space = EmitBSSVariable;
  target->emit_data_start = EmitDataStart;
  target->emit_literals_start = EmitStringLiteralSection;
  target->emit_literal = EmitLiteral;
  target->emit_cxx_thunks = ARMPrintCXXAdjustorThunks;
  target->emit_debug = EmitDebug;
  target->emit_tdata_start = EmitTlsDataStart;
  target->emit_tbss_start = EmitTlsBSSStart;
  target->emit_tls_variable = EmitTlsVariable;
  target->emit_tbss_space = EmitTlsBSSVariable;
  target->handle_options = HandleOptions;
  return target;
}
