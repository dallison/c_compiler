//
//  p_code_target.c
//  c_compiler
//
//  Created by David Allison on 1/8/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "p_code_target.h"
#include <assert.h>
#include <string.h>
#include "p_code_assembler.h"
#include "p_code_codegen.h"
#include "p_code_emitter.h"
#include "p_code_reg_alloc.h"
#include "common_emitter.h"
#include "compiler.h"

static void* GenerateCode(Generator* gen) {
  PCodeGenerator* pcode = NewPCodeGenerator(gen);
  PCodeLower(pcode, gen);
  if (compiler->print_back_end) {
    PCodePrint(pcode);
  }
  return pcode;
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  PCodeEmitter emitter;
  PCodeEmitterInit(&emitter, (PCodeGenerator*)code);
  PCodePrintFunction(&emitter, asm_file);
  PCodeEmitterDestruct(&emitter);
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
  PCodeAssembler assembler;
  PCodeAssemblerInit(&assembler, asm_filename, object_filename);
  AssemblerRun(&assembler.base, AssemblePCodeInstruction);

  PCodeAssemblerDestruct(&assembler);
  return assembler.base.num_errors == 0;
}

static void HandleOptions(Vector* options) {
  
}

static void Cleanup(void* code) { PCodeGeneratorDelete(code); }

// Create a new PCode target.  The functions are called by the
// compiler.
CompilerTarget* NewPCodeTarget() {
  CompilerTarget* target = malloc(sizeof(CompilerTarget));
  StringInit(&target->name, "P-CODE");
  target->pointer_size = 8;
  target->int_size = 4;
  target->bool_size = 1;
  target->short_size = 2;
  target->long_size = 8;
  target->long_long_size = 8;
  target->float_size = 4;
  target->double_size = 8;
  target->wchar_size = 4;
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
  // The p-code lowering depends on the existing loop/phi shape.  Keep the new
  // analysis available, but do not mutate that shape yet.
  target->ir_optimizations.loop_preheaders = false;
  target->ir_optimizations.induction_vars = false;
  target->ir_optimizations.derived_induction_vars = false;
  target->prepend_underscore = false;
  target->plain_char_is_signed = false;

  target->options = NULL;

  target->flags = 0;
  target->alignment = 8;
  target->stack_alignment = 8;
  target->codegen = GenerateCode;
  target->emit_object_file = NULL;
  target->emit_function_assembly = EmitFunctionAssembly;
  target->assemble = Assemble;
  target->cleanup = Cleanup;
  target->create_asm_file = CreateAssemblyFile;
  target->emit_assembly_preamble = NULL;
  target->assemble_string = NULL;
  target->emit_static_variable = EmitStaticVariable;
  target->emit_bss_space = EmitBSSVariable;
  target->emit_data_start = EmitDataStart;
  target->emit_tdata_start = EmitTlsDataStart;
  target->emit_tbss_start = EmitTlsBSSStart;
  target->emit_tls_variable = EmitTlsVariable;
  target->emit_tbss_space = EmitTlsBSSVariable;
  target->emit_literals_start = EmitStringLiteralSection;
  target->emit_literal = EmitLiteral;
  target->emit_cxx_thunks = NULL;
  target->emit_debug = EmitDebug;
  target->handle_options = HandleOptions;
  return target;
}
