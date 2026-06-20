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
#include "common_emitter.h"

static void* GenerateCode(Generator* gen) {
  W65C02Generator* g = New6502Generator(gen);
  W65C02Lower(g, gen);
  return g;
}

static void EmitFunctionAssembly(void* code, FILE* asm_file) {
  W65C02Emitter emitter;
  W65C02EmitterInit(&emitter, (W65C02Generator*)code);
  W65C02PrintFunction(&emitter, asm_file);
  W65C02EmitterDestruct(&emitter);
}

static FILE* CreateAssemblyFile(String* src_file, String* asm_file) {
  FILE* fp = EmitAssemblyFile(src_file, asm_file);
  if (fp == NULL) {
    return NULL;
  }

  // Define the registers.
  static struct {
    char prefix;
    int num;
    int size;
  } registers[] = {{'b', W65C02_NUM_B_REGS, 1}, {'i', W65C02_NUM_I_REGS, 2},
                   {'l', W65C02_NUM_L_REGS, 4}, {'x', W65C02_NUM_X_REGS, 8},
                {'f', W65C02_NUM_F_REGS, 4}
  };

  // Get start address for registers from target options.
  int start_addr = ((W65C02Target*)compiler->target)->regs_start;
  int addr = start_addr;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < registers[i].num; j++) {
      fprintf(fp, "\t.set __%c%d 0x%x\n", registers[i].prefix, j, addr);
      addr += registers[i].size;
    }
  }
  
  fprintf(fp, "\t.set __sp 0x%x\n", W65C02_SP_REG + start_addr);
  fprintf(fp, "\t.set __fp 0x%x\n", W65C02_FP_REG + start_addr);
  fprintf(fp, "\t.set __result 0x%x\n", W65C02_RESULT_REG + start_addr);
  fprintf(fp, "\t.set __t0 0x%x\n", W65C02_T0_REG + start_addr);
  fprintf(fp, "\t.set __t1 0x%x\n", W65C02_T1_REG + start_addr);
  fprintf(fp, "\t.set __t2 0x%x\n", W65C02_T2_REG + start_addr);
  fprintf(fp, "\t.set __t3 0x%x\n", W65C02_T3_REG + start_addr);
  fprintf(fp, "\t.set __mem_src 0x%x\n", W65C02_MSRC_REG + start_addr);
  fprintf(fp, "\t.set __mem_dest 0x%x\n", W65C02_MDST_REG + start_addr);
  fprintf(fp, "\t.set __mem_size 0x%x\n", W65C02_MSZ_REG + start_addr);

  fprintf(fp, "\n\n");
  return fp;
}

static bool Assemble(String* asm_filename, String* object_filename) {
  W65C02Assembler assembler;
  W65C02AssemblerInit(&assembler, asm_filename, object_filename);
  AssemblerRun(&assembler.base, Assemble6502Instruction);
  W65C02AssemblerFinalize(&assembler);
  
  int num_errors = assembler.base.num_errors;
  W65C02AssemblerDestruct(&assembler);
  return num_errors == 0;
}

static void Cleanup(void* code) { W65C02GeneratorDelete(code); }

static CompilerOptionDefinition options[] = {
  {"-freg-start", kCompilerOptionInt, k6502OptionRegStart, false, "Specify start address for zero-page registers"},
  {NULL}
};

static void HandleOptions(Vector* options) {
  W65C02Target* target = (W65C02Target*)compiler->target;
  target->regs_start = OptionIntValue(k6502OptionRegStart, options, 0);
}

// Create a new W65C02 target.  The functions are called by the
// compiler.
CompilerTarget* New6502Target() {
  W65C02Target* t = malloc(sizeof(W65C02Target));
  CompilerTarget* target = &t->base;
  StringInit(&target->name, "6502");
  target->pointer_size = 2;
  target->int_size = 2;
  target->short_size = 2;
  target->bool_size = 1;
  target->long_size = 4;
  target->long_long_size = 8;
  target->stack_alignment = 1;
  target->float_size = 4;
  target->double_size = 4;
  target->wchar_size = 2;
  target->code_preference = kCodeForSize;
  target->call_return_fixed_reg = false;
  target->keep_ssa = false;
  target->plain_char_is_signed = false;

  target->options = options;
  
  target->ir_optimizations.gvn = false;      // Makes 6502 worse.
  target->ir_optimizations.const_prop = true;
  target->ir_optimizations.code_motion = false;  // Increases spills.
  target->ir_optimizations.tail_call = false;   // Possible in 6502?

  target->prepend_underscore = false;
  target->flags = 0;
  target->alignment = 1;
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
  target->emit_cxx_thunks = NULL;
  target->emit_debug = EmitDebug;
  target->emit_tdata_start = EmitTlsDataStart;
  target->emit_tbss_start = EmitTlsBSSStart;
  target->emit_tls_variable = EmitTlsVariable;
  target->emit_tbss_space = EmitTlsBSSVariable;
  target->handle_options = HandleOptions;
  
  // Targer specific options.
  t->regs_start = 0;
  return target;
}

CompilerTarget* New65c02Target() {
  CompilerTarget* t = New6502Target();
  StringSet(&t->name, "65c02");
  t->flags |= k65c02Target;
  return t;
}
