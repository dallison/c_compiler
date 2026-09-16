//
//  x86_profile.h
//  c_compiler
//
//  Runtime profile for the shared x86 backend (i386 and AMD64).
//

#ifndef x86_profile_h
#define x86_profile_h

#include <stdbool.h>
#include <stdint.h>

#include "codegen.h"

typedef enum {
  kX86ModeI386,
  kX86ModeAMD64,
} X86TargetMode;

typedef struct X86Profile {
  X86TargetMode mode;
  bool is_64bit;

  // Target sizes and ELF.
  int pointer_size;
  int stack_slot_size;
  int elf_machine;
  int dwarf_frame_register;

  // Hardware register counts.
  int num_hw_int_regs;
  int num_hw_float_regs;

  // Logical register file (register allocator).
  int num_int_regs;
  int num_float_regs;
  int num_int_args;
  int num_fp_args;

  int int_zero_reg;
  int int_return_value_0;
  int int_return_value_1;

  int int_arg_start;
  int int_arg_end;
  int fp_arg_start;
  int fp_arg_end;

  int vararg_save_area_size;
  int vararg_fp_save_offset;

  int int_saved_start_1;
  int int_saved_end_1;
  int int_saved_start_2;
  int int_saved_end_2;
  int int_temp_start_1;
  int int_temp_end_1;
  int int_temp_start_2;
  int int_temp_end_2;

  int spill_addr;

  int fp_return_value_0;
  int fp_return_value_1;
  int fp_saved_start_1;
  int fp_saved_end_1;
  int fp_saved_start_2;
  int fp_saved_end_2;
  int fp_temp_start_1;
  int fp_temp_end_1;
  int fp_temp_start_2;
  int fp_temp_end_2;

  // Logical slot indices for sp/fp/ret in the register file.
  int sp_reg;
  int fp_reg;
  int ret_reg;

  int stack_frame_header_size;
  int stack_alignment;
  int red_zone_size;

  int first_int_reg_var;
  int last_int_reg_var;
  int first_leaf_int_reg_var;
  int last_leaf_int_reg_var;
  int first_fp_reg_var;
  int last_fp_reg_var;
  int first_leaf_fp_reg_var;
  int last_leaf_fp_reg_var;

  // i386 EBP-relative frame layout (unused on AMD64).
  int saved_ebp_offset;
  int retaddr_ebp_offset;
  int first_arg_ebp_offset;
  int entry_esp_mod;
  int post_push_esp_mod;
  int frame_size_mod;

  // Byte register constraints (setcc/movb).
  int byte_reg_start;
  int byte_reg_end;

  // Emitter scratch register names (AT&T syntax, without %).
  const char* emit_scratch;
  const char* sse_scratch;
  const char* sse_scratch2;

  // Assembler behavior.
  bool supports_rex;
  bool supports_rip_relative;
  bool uses_rela;
  bool rip_relative_symbols;

  // IR / target flags.
  bool tail_call_opt;
  int data_alignment;
} X86Profile;

extern const X86Profile kX86ProfileI386;
extern const X86Profile kX86ProfileAMD64;

const X86Profile* X86ProfileFromTargetName(const char* name);
const X86Profile* X86ProfileFromGenerator(Generator* gen);

#define X86_P(rv) ((rv)->profile)
#define X86_IS_64BIT(rv) ((rv)->profile->is_64bit)

#endif /* x86_profile_h */
