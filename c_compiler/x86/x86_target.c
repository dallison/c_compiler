//
//  x86_target.c
//  c_compiler_library
//
//  Shared x86 backend target factories (i386 and AMD64 profiles).
//

#include "x86_target.h"

#include <stdlib.h>
#include <string.h>

#include "common_emitter.h"
#include "errors.h"
#include "x86_assembler.h"
#include "x86_codegen.h"
#include "x86_emitter.h"
#include "x86_profile.h"
#include "x86_reg_alloc.h"

static void* GenerateCodeWithProfile(Generator* gen, const X86Profile* profile) {
  X86Generator* rv = NewX86GeneratorWithProfile(gen, profile);
  int errors_before = NumErrors();
  X86Lower(rv, gen);
  if (NumErrors() != errors_before) {
    X86GeneratorDelete(rv);
    return NULL;
  }
  return rv;
}

static void* GenerateCode(Generator* gen) {
  return GenerateCodeWithProfile(gen, X86ProfileFromGenerator(gen));
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  X86Emitter emitter;
  X86EmitterInit(&emitter, (X86Generator*)code);
  X86PrintFunction(&emitter, asm_file);
  X86EmitterDestruct(&emitter);
}

static FILE* CreateAssemblyFile(String* src_file, String* asm_file) {
  FILE* fp = EmitAssemblyFile(src_file, asm_file);
  if (fp == NULL) {
    return NULL;
  }
  fprintf(fp, ".PCbegin:\n");
  return fp;
}

static bool AssembleWithProfile(String* asm_filename, String* object_filename,
                                const X86Profile* profile) {
  X86Assembler assembler;
  X86AssemblerInitWithProfile(&assembler, asm_filename, object_filename, profile);
  AssemblerRun(&assembler.base, AssembleX86Instruction);
  int num_errors = assembler.base.num_errors;
  X86AssemblerDestruct(&assembler);
  return num_errors == 0;
}

static bool Assemble(String* asm_filename, String* object_filename) {
  return AssembleWithProfile(asm_filename, object_filename, &kX86ProfileAMD64);
}

static bool AssembleI386(String* asm_filename, String* object_filename) {
  return AssembleWithProfile(asm_filename, object_filename, &kX86ProfileI386);
}

static void Cleanup(void* code) { X86GeneratorDelete(code); }

static void HandleOptions(Vector* options) { (void)options; }

static CompilerTarget* NewX86TargetWithProfile(const char* name,
                                               const X86Profile* profile) {
  CompilerTarget* target = malloc(sizeof(CompilerTarget));
  StringInit(&target->name, name);
  target->pointer_size = profile->pointer_size;
  target->int_size = 4;
  target->bool_size = 1;
  target->short_size = 2;
  target->long_size = profile->pointer_size;
  target->long_long_size = 8;
  target->float_size = 4;
  target->double_size = 8;
  target->long_double_size = profile->is_64bit ? 16 : 8;
  target->long_double_format = profile->is_64bit ? kLongDoubleFormatIntel80
                                                 : kLongDoubleFormatFloat64;
  target->wchar_size = 4;
  target->stack_alignment = profile->stack_alignment;
  target->code_preference = kCodeForSpeed;
  target->call_return_fixed_reg = true;
  target->keep_ssa = false;
  target->ir_optimizations.gvn = true;
  target->ir_optimizations.sccp = true;
  target->ir_optimizations.const_prop = true;
  target->ir_optimizations.code_motion = true;
  target->ir_optimizations.tail_call = profile->tail_call_opt;
  target->ir_optimizations.dce = true;
  target->ir_optimizations.copy_prop = true;
  target->ir_optimizations.loop_preheaders = true;
  target->ir_optimizations.induction_vars = true;
  target->ir_optimizations.derived_induction_vars = false;
  target->prepend_underscore = false;
  target->plain_char_is_signed = false;
  target->options = NULL;
  target->flags = 0;
  target->alignment = profile->data_alignment;
  target->codegen = GenerateCode;
  target->emit_program_file = NULL;
  target->emit_object_file = NULL;
  target->emit_function_assembly = EmitFunctionAssembly;
  target->assemble =
      profile->is_64bit ? Assemble : AssembleI386;
  target->cleanup = Cleanup;
  target->create_asm_file = CreateAssemblyFile;
  target->emit_assembly_preamble = NULL;
  target->assemble_string = NULL;
  target->emit_static_variable = EmitStaticVariable;
  target->emit_bss_space = EmitBSSVariable;
  target->emit_data_start = EmitDataStart;
  target->emit_literals_start = EmitStringLiteralSection;
  target->emit_literal = EmitLiteral;
  target->emit_cxx_thunks = X86PrintCXXAdjustorThunks;
  target->emit_debug = EmitDebug;
  target->dwarf_frame_register = profile->dwarf_frame_register;
  target->emit_tdata_start = EmitTlsDataStart;
  target->emit_tbss_start = EmitTlsBSSStart;
  target->emit_tls_variable = EmitTlsVariable;
  target->emit_tbss_space = EmitTlsBSSVariable;
  target->handle_options = HandleOptions;
  return target;
}

CompilerTarget* NewX86Target(void) {
  return NewX86TargetWithProfile("x86", &kX86ProfileI386);
}

CompilerTarget* NewX86_64Target(void) {
  return NewX86TargetWithProfile("x86-64", &kX86ProfileAMD64);
}
