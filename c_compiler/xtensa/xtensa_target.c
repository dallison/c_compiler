//
//  xtensa_target.c
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "xtensa_target.h"
#include <assert.h>
#include <string.h>
#include "common_emitter.h"
#include "xtensa_assembler.h"
#include "xtensa_codegen.h"
#include "xtensa_emitter.h"
#include "xtensa_reg_alloc.h"

static void* GenerateCode(Generator* gen) {
  XTENSAGenerator* rv = NewXTENSAGenerator(gen);
  XTENSALower(rv, gen);
  return rv;
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  XTENSAEmitter emitter;
  XTENSAEmitterInit(&emitter, (XTENSAGenerator*)code);
  XTENSAPrintFunction(&emitter, asm_file);
  XTENSAEmitterDestruct(&emitter);
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
  fprintf(fp, "\t.section \".xtensa.info\", \"\", @note\n");
  fprintf(fp, "\t.align 2\n");
  fprintf(fp, "\t.4byte 12\n");
  fprintf(fp, "\t.4byte 32\n");
  fprintf(fp, "\t.4byte 1\n");
  fprintf(fp, "\t.asciz \"Xtensa_Info\"\n");
  fprintf(fp, "\t.asciz \"USE_ABSOLUTE_LITERALS=0\\nABI=0\\n\"\n");
  fprintf(fp, "\t.byte 0\n");
  fprintf(fp, "\t.text\n");
  return fp;
}

static bool Assemble(String* asm_filename, String* object_filename) {
  XTENSAAssembler assembler;
  XTENSAAssemblerInit(&assembler, asm_filename, object_filename);
  AssemblerRun(&assembler.base, AssembleXTENSAInstruction);

  int num_errors = assembler.base.num_errors;
  XTENSAAssemblerDestruct(&assembler);
  return num_errors == 0;
}

static void Cleanup(void* code) { XTENSAGeneratorDelete(code); }

static void HandleOptions(Vector* options) {}

// Create the classic ESP32 Xtensa LX6 target.
CompilerTarget* NewXTENSATarget() {
  CompilerTarget* target = malloc(sizeof(CompilerTarget));
  StringInit(&target->name, "esp32");
  target->pointer_size = 4;
  target->int_size = 4;
  target->bool_size = 1;
  target->short_size = 2;
  target->long_size = 4;
  target->long_long_size = 8;
  target->float_size = 4;
  target->double_size = 8;
  target->long_double_size = 8;
  target->long_double_format = kLongDoubleFormatFloat64;
  target->wchar_size = 4;
  target->stack_alignment = 16;
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
  // Revisit after loop-carried temporary spilling is pressure-aware.
  target->ir_optimizations.derived_induction_vars = false;
  target->prepend_underscore = false;
  target->plain_char_is_signed = false;

  target->options = NULL;

  target->flags = 0;
  target->alignment = 4;
  target->codegen = GenerateCode;
  target->emit_program_file = NULL;
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
  target->emit_literals_start = EmitStringLiteralSection;
  target->emit_literal = EmitLiteral;
  target->emit_cxx_thunks = XTENSAPrintCXXAdjustorThunks;
  target->emit_debug = EmitDebug;
  target->dwarf_frame_register = 8;
  target->emit_tdata_start = EmitTlsDataStart;
  target->emit_tbss_start = EmitTlsBSSStart;
  target->emit_tls_variable = EmitTlsVariable;
  target->emit_tbss_space = EmitTlsBSSVariable;
  target->handle_options = HandleOptions;
  return target;
}
