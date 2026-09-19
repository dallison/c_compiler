//
//  x86_profile.c
//  c_compiler
//

#include "x86_profile.h"

#include <string.h>

#include "elf.h"
#include "x86_machine.h"

#include "compiler.h"

const X86Profile kX86ProfileI386 = {
    .mode = kX86ModeI386,
    .is_64bit = false,

    .pointer_size = 4,
    .stack_slot_size = 4,
    .elf_machine = ELF_MACHINE_TYPE_386,
    .dwarf_frame_register = 5,

    .num_hw_int_regs = 8,
    .num_hw_float_regs = 8,

    .num_int_regs = 8,
    .num_float_regs = 8,
    .num_int_args = 0,
    .num_fp_args = 0,

    .int_zero_reg = 0,
    .int_return_value_0 = X86_REG_EAX,
    .int_return_value_1 = X86_REG_EDX,

    .int_arg_start = 8,
    .int_arg_end = 7,
    .fp_arg_start = 8,
    .fp_arg_end = 7,

    .vararg_save_area_size = 0,
    .vararg_fp_save_offset = 0,

    .int_saved_start_1 = 3,
    .int_saved_end_1 = 3,
    .int_saved_start_2 = 6,
    .int_saved_end_2 = 7,
    .int_temp_start_1 = 0,
    .int_temp_end_1 = 2,
    .int_temp_start_2 = 1,
    .int_temp_end_2 = 2,

    .spill_addr = 1,

    .fp_return_value_0 = 0,
    .fp_return_value_1 = 1,
    // xmm6/xmm7 are emitter scratch and are not allocatable.  The remaining
    // xmm0-xmm5 are all caller-saved; list them as both temps and saved so a
    // can_use_temp=false allocation still has somewhere to go.  An empty
    // saved range (start > end) made FindSpillVictim abort on FP-heavy code.
    .fp_saved_start_1 = 0,
    .fp_saved_end_1 = 5,
    .fp_saved_start_2 = 8,
    .fp_saved_end_2 = 7,
    .fp_temp_start_1 = 0,
    .fp_temp_end_1 = 5,
    .fp_temp_start_2 = 8,
    .fp_temp_end_2 = 7,

    .sp_reg = 4,
    .fp_reg = 5,
    .ret_reg = 0,

    .stack_frame_header_size = 8,
    .stack_alignment = 16,
    .red_zone_size = 0,

    .first_int_reg_var = 6,
    .last_int_reg_var = 7,
    .first_leaf_int_reg_var = 0,
    .last_leaf_int_reg_var = 2,
    .first_fp_reg_var = 8,
    .last_fp_reg_var = 7,
    .first_leaf_fp_reg_var = 0,
    .last_leaf_fp_reg_var = 7,

    .saved_ebp_offset = 0,
    .retaddr_ebp_offset = 4,
    .first_arg_ebp_offset = 8,
    .entry_esp_mod = 12,
    .post_push_esp_mod = 8,
    .frame_size_mod = 8,

    .byte_reg_start = 0,
    .byte_reg_end = 3,

    .emit_scratch = "ebx",
    .sse_scratch = "xmm7",
    .sse_scratch2 = "xmm6",

    .supports_rex = false,
    .supports_rip_relative = false,
    .uses_rela = false,
    .rip_relative_symbols = false,

    .tail_call_opt = false,
    .data_alignment = 4,
};

const X86Profile kX86ProfileAMD64 = {
    .mode = kX86ModeAMD64,
    .is_64bit = true,

    .pointer_size = 8,
    .stack_slot_size = 8,
    .elf_machine = ELF_MACHINE_TYPE_X86_64,
    .dwarf_frame_register = 6,

    .num_hw_int_regs = 16,
    .num_hw_float_regs = 16,

    .num_int_regs = 32,
    .num_float_regs = 32,
    .num_int_args = 6,
    .num_fp_args = 8,

    .int_zero_reg = 0,
    .int_return_value_0 = X86_REG_RAX,
    .int_return_value_1 = X86_REG_RDX,

    .int_arg_start = 10,
    .int_arg_end = 15,
    .fp_arg_start = 0,
    .fp_arg_end = 7,

    .vararg_save_area_size = (6 * 8 + 8 * 16 + 8),
    .vararg_fp_save_offset = (6 * 8),

    .int_saved_start_1 = 8,
    .int_saved_end_1 = 9,
    .int_saved_start_2 = 18,
    .int_saved_end_2 = 27,
    .int_temp_start_1 = 5,
    .int_temp_end_1 = 7,
    .int_temp_start_2 = 28,
    .int_temp_end_2 = 31,

    .spill_addr = 3,

    .fp_return_value_0 = 0,
    .fp_return_value_1 = 1,
    .fp_saved_start_1 = 8,
    .fp_saved_end_1 = 15,
    .fp_saved_start_2 = 18,
    .fp_saved_end_2 = 27,
    .fp_temp_start_1 = 0,
    .fp_temp_end_1 = 7,
    .fp_temp_start_2 = 28,
    .fp_temp_end_2 = 31,

    .sp_reg = 2,
    .fp_reg = 8,
    .ret_reg = 1,

    .stack_frame_header_size = 16,
    .stack_alignment = 16,
    .red_zone_size = 128,

    .first_int_reg_var = 18,
    .last_int_reg_var = 27,
    .first_leaf_int_reg_var = 28,
    .last_leaf_int_reg_var = 31,
    .first_fp_reg_var = 18,
    .last_fp_reg_var = 27,
    .first_leaf_fp_reg_var = 28,
    .last_leaf_fp_reg_var = 31,

    .saved_ebp_offset = 0,
    .retaddr_ebp_offset = 0,
    .first_arg_ebp_offset = 0,
    .entry_esp_mod = 0,
    .post_push_esp_mod = 0,
    .frame_size_mod = 0,

    .byte_reg_start = 0,
    .byte_reg_end = 15,

    .emit_scratch = "r11",
    .sse_scratch = "xmm15",
    .sse_scratch2 = "xmm14",

    .supports_rex = true,
    .supports_rip_relative = true,
    .uses_rela = true,
    .rip_relative_symbols = true,

    .tail_call_opt = true,
    .data_alignment = 8,
};

const X86Profile* X86ProfileFromTargetName(const char* name) {
  if (name == NULL) {
    return &kX86ProfileAMD64;
  }
  if (strcmp(name, "x86") == 0 || strcmp(name, "i386") == 0 ||
      strcmp(name, "i486") == 0 || strcmp(name, "i586") == 0 ||
      strcmp(name, "i686") == 0 || strcmp(name, "x86-32") == 0) {
    return &kX86ProfileI386;
  }
  return &kX86ProfileAMD64;
}

const X86Profile* X86ProfileFromGenerator(Generator* gen) {
  (void)gen;
  if (compiler == NULL || compiler->target == NULL) {
    return &kX86ProfileAMD64;
  }
  return X86ProfileFromTargetName(compiler->target->name.value);
}
