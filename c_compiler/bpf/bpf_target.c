#include "bpf_target.h"

#include <stdlib.h>

#include "bpf_assembler.h"
#include "bpf_codegen.h"
#include "bpf_emitter.h"
#include "common_emitter.h"

static void* GenerateCode(Generator* gen) {
  BPFGenerator* bpf = NewBPFGenerator(gen);
  BPFLower(bpf, gen);
  return bpf;
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  BPFEmitter emitter;
  BPFEmitterInit(&emitter, (BPFGenerator*)code);
  BPFPrintFunction(&emitter, asm_file);
  BPFEmitterDestruct(&emitter);
}

static FILE* CreateAssemblyFile(String* src_file, String* asm_file) {
  FILE* fp = EmitAssemblyFile(src_file, asm_file);
  if (fp == NULL) {
    return NULL;
  }
  fprintf(fp, "// linux eBPF assembly for %s\n", src_file->value);
  return fp;
}

static bool Assemble(String* asm_filename, String* object_filename) {
  BPFAssembler assembler;
  BPFAssemblerInit(&assembler, asm_filename, object_filename);
  AssemblerRun(&assembler.base, AssembleBPFInstruction);
  int num_errors = assembler.base.num_errors;
  BPFAssemblerDestruct(&assembler);
  return num_errors == 0;
}

static void Cleanup(void* code) { BPFGeneratorDelete(code); }

static void HandleOptions(Vector* options) { (void)options; }

CompilerTarget* NewBPFTarget(void) {
  CompilerTarget* target = malloc(sizeof(CompilerTarget));
  StringInit(&target->name, "bpf");
  target->pointer_size = 8;
  target->int_size = 4;
  target->bool_size = 1;
  target->short_size = 2;
  target->long_size = 8;
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
  target->ir_optimizations.tail_call = false;
  target->ir_optimizations.dce = true;
  target->ir_optimizations.copy_prop = true;
  target->ir_optimizations.loop_preheaders = true;
  target->ir_optimizations.induction_vars = true;
  target->ir_optimizations.derived_induction_vars = false;
  target->prepend_underscore = false;
  target->plain_char_is_signed = false;

  target->options = NULL;
  target->flags = 0;
  target->alignment = 8;
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
  target->emit_cxx_thunks = BPFPrintCXXAdjustorThunks;
  target->emit_debug = EmitDebug;
  target->dwarf_frame_register = BPF_REG_FP;
  target->emit_tdata_start = EmitTlsDataStart;
  target->emit_tbss_start = EmitTlsBSSStart;
  target->emit_tls_variable = EmitTlsVariable;
  target->emit_tbss_space = EmitTlsBSSVariable;
  target->handle_options = HandleOptions;
  return target;
}
