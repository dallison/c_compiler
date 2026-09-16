//
//  wasm32_target.c
//  c_compiler
//

#include "wasm32_target.h"

#include <stdlib.h>

#include "common_emitter.h"
#include "wasm32_codegen.h"
#include "wasm32_emitter.h"
#include "wasm32_module.h"
#include "wasm32_optimize.h"
#include "wasm32_reg_alloc.h"
#include "wasm32_stackify.h"

static void* GenerateCode(Generator* gen) {
  Wasm32Generator* wasm = NewWasm32Generator(gen);
  Wasm32Lower(wasm, gen);
  Wasm32Optimize(wasm);
  Wasm32Stackify(wasm);
  Wasm32OptimizeStackified(wasm);
  Wasm32AllocateLocals(wasm);
  return wasm;
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  Wasm32PrintFunction((Wasm32Generator*)code, asm_file);
}

static FILE* CreateAssemblyFile(String* src_file, String* asm_file) {
  FILE* fp = EmitAssemblyFile(src_file, asm_file);
  if (fp == NULL) {
    return NULL;
  }
  fprintf(fp, ";; wasm32 module listing for %s\n", src_file->value);
  return fp;
}

// The wasm binary format is not a text assembly language, so there is
// nothing to assemble: the object is serialized straight from the lowered
// functions the compiler is still holding.  The listing file written by
// EmitFunctionAssembly is a debugging aid only.
static bool Assemble(String* asm_filename, String* object_filename) {
  return Wasm32WriteObject(object_filename);
}

static void Wasm32EmitDataStart(FILE* fp) { fprintf(fp, ";; data\n"); }

static void Wasm32EmitStaticVariable(InitializedStaticVariable* var, FILE* fp) {
  fprintf(fp, ";; .data %s %zu bytes\n", var->symbol->name.value, var->size);
}

static void Wasm32EmitBSSVariable(UninitializedStaticVariable* var, FILE* fp) {
  fprintf(fp, ";; .bss %s %zu bytes\n", var->symbol->name.value, var->size);
}

static void Wasm32EmitLiteralsStart(FILE* fp) { fprintf(fp, ";; literals\n"); }

static void Wasm32EmitLiteral(Literal* literal, FILE* fp) {
  fprintf(fp, ";; .rodata literal %d\n", literal->id);
}

static void Wasm32EmitTlsDataStart(FILE* fp) {}
static void Wasm32EmitTlsBSSStart(FILE* fp) {}
static void Wasm32EmitTlsVariable(InitializedStaticVariable* var, FILE* fp) {}
static void Wasm32EmitTlsBSSVariable(UninitializedStaticVariable* var, FILE* fp) {}
static void Wasm32EmitDebugInfo(FILE* fp) {}

static void HandleOptions(Vector* options) {}

static void Cleanup(void* code) { Wasm32GeneratorDelete(code); }

CompilerTarget* NewWasm32Target(void) {
  CompilerTarget* target = malloc(sizeof(CompilerTarget));
  StringInit(&target->name, "wasm32");
  target->pointer_size = 4;
  target->int_size = 4;
  target->bool_size = 1;
  target->short_size = 2;
  target->long_size = 4;
  target->long_long_size = 8;
  target->float_size = 4;
  target->double_size = 8;
  target->wchar_size = 4;
  target->code_preference = kCodeForSpeed;
  target->call_return_fixed_reg = true;
  // Phi nodes have no wasm equivalent; SSA must be destroyed into local
  // assignments before lowering sees the code.
  target->keep_ssa = false;
  target->ir_optimizations.gvn = true;
  target->ir_optimizations.sccp = true;
  target->ir_optimizations.const_prop = true;
  target->ir_optimizations.code_motion = false;
  target->ir_optimizations.tail_call = false;
  target->ir_optimizations.dce = true;
  target->ir_optimizations.copy_prop = true;
  // Structuring an arbitrary CFG back into nested blocks is sensitive to
  // loop shape, so the passes that rewrite loops stay off.
  target->ir_optimizations.loop_preheaders = false;
  target->ir_optimizations.induction_vars = false;
  target->ir_optimizations.derived_induction_vars = false;
  target->prepend_underscore = false;
  target->plain_char_is_signed = true;

  target->options = NULL;
  target->flags = 0;
  target->alignment = 8;
  target->stack_alignment = 16;

  target->codegen = GenerateCode;
  target->emit_program_file = NULL;
  target->emit_object_file = NULL;
  target->emit_function_assembly = EmitFunctionAssembly;
  target->assemble = Assemble;
  target->cleanup = Cleanup;
  target->create_asm_file = CreateAssemblyFile;
  target->emit_assembly_preamble = NULL;
  target->assemble_string = NULL;
  target->emit_static_variable = Wasm32EmitStaticVariable;
  target->emit_bss_space = Wasm32EmitBSSVariable;
  target->emit_data_start = Wasm32EmitDataStart;
  target->emit_tdata_start = Wasm32EmitTlsDataStart;
  target->emit_tbss_start = Wasm32EmitTlsBSSStart;
  target->emit_tls_variable = Wasm32EmitTlsVariable;
  target->emit_tbss_space = Wasm32EmitTlsBSSVariable;
  target->emit_literals_start = Wasm32EmitLiteralsStart;
  target->emit_literal = Wasm32EmitLiteral;
  target->emit_cxx_thunks = NULL;
  target->emit_debug = Wasm32EmitDebugInfo;
  target->dwarf_frame_register = -1;
  target->handle_options = HandleOptions;
  return target;
}
