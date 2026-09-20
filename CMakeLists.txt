cmake_minimum_required(VERSION 3.20)

project(davecc VERSION 0.1.0 LANGUAGES C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(DAVECC_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/davecc"
    CACHE STRING "DaveCC libc header installation directory")
set(DAVECC_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}/davecc"
    CACHE STRING "DaveCC target libc archive installation directory")
set(DAVECC_INSTALL_LIBEXECDIR "${CMAKE_INSTALL_LIBEXECDIR}/davecc"
    CACHE STRING "DaveCC private executable installation directory")

set(DAVECC_COMMON_INCLUDES
  6502_interpreter
  Linker
  Linker/Architecture
  c_compiler
  c_compiler/frontend/lexical
  c_compiler/frontend/syntax
  c_compiler/frontend/semantic
  c_compiler/backend
  c_compiler/optimization
  c_compiler/assembler
  c_compiler/util
  c_compiler/serialize
  c_compiler/driver
  c_compiler/lto
  c_compiler/disassembler
  c_compiler/6502
  c_compiler/aarch64
  c_compiler/arm
  c_compiler/bpf
  c_compiler/p_code
  c_compiler/risc_v
  c_compiler/risc_v32
  c_compiler/xtensa
  c_compiler/wasm32
  c_compiler/x86
  c_compiler/x86_64
  c_compiler/AR
  c_compiler/ELF
  c_compiler/Loader
  c_compiler/Loader/Architecture
  risc_v_interpreter
  arm_interpreter
  aarch64_interpreter
  x86_64_interpreter
)

if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
  set(DAVECC_AARCH64_COMPILE_OPTIONS -mbranch-protection=none)
else()
  set(DAVECC_AARCH64_COMPILE_OPTIONS)
endif()

function(davecc_common_target target)
  target_include_directories(${target} PUBLIC ${DAVECC_COMMON_INCLUDES})
  if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${target} PRIVATE -fPIC)
  endif()
endfunction()

function(davecc_library target)
  add_library(${target} STATIC ${ARGN})
  davecc_common_target(${target})
endfunction()

function(davecc_executable target output_name)
  add_executable(${target} ${ARGN})
  davecc_common_target(${target})
  set_target_properties(${target} PROPERTIES OUTPUT_NAME "${output_name}")
  if(APPLE AND CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang")
    target_link_options(${target} PRIVATE LINKER:-no_warn_duplicate_libraries)
  endif()
endfunction()

davecc_library(davecc_utils
  c_compiler/util/binary_tree.c
  c_compiler/util/bitset.c
  c_compiler/util/buffer.c
  c_compiler/util/config.c
  c_compiler/util/dstring.c
  c_compiler/util/hashtable.c
  c_compiler/util/list.c
  c_compiler/util/map.c
  c_compiler/util/set.c
  c_compiler/util/vector.c
)

davecc_library(davecc_elf
  c_compiler/ELF/elf_format.c
  c_compiler/ELF/elf_reader.c
  c_compiler/ELF/elf_writer.c
)
target_link_libraries(davecc_elf PUBLIC davecc_utils)

davecc_library(davecc_xtensa_isa
  c_compiler/xtensa/xtensa_isa.c
)
target_link_libraries(davecc_xtensa_isa PUBLIC davecc_utils)

davecc_library(davecc_wasm32_object
  c_compiler/wasm32/wasm32_object.c
)
target_link_libraries(davecc_wasm32_object PUBLIC davecc_utils)

davecc_library(davecc_wireformat
  c_compiler/serialize/wireformat.c
)

davecc_library(davecc_filesystem_host
  c_compiler/Loader/filesystem_host.c
)

davecc_library(davecc_chrono_host
  c_compiler/Loader/chrono_host.c
)
target_link_libraries(davecc_chrono_host PUBLIC davecc_filesystem_host)

davecc_library(davecc_random_host
  c_compiler/Loader/random_host.c
)

davecc_library(davecc_guest_addr_wait
  c_compiler/Loader/guest_addr_wait.c
)

davecc_library(davecc_disassembler
  c_compiler/disassembler/disassembler.c
  c_compiler/disassembler/disassembler_6502.c
  c_compiler/disassembler/disassembler_aarch64.c
  c_compiler/disassembler/disassembler_arm.c
  c_compiler/disassembler/disassembler_bpf.c
  c_compiler/disassembler/disassembler_riscv.c
  c_compiler/disassembler/disassembler_tool.c
  c_compiler/disassembler/disassembler_pcode.c
  c_compiler/disassembler/disassembler_wasm.c
  c_compiler/disassembler/disassembler_x86_64.c
  c_compiler/disassembler/disassembler_xtensa.c
)
target_link_libraries(davecc_disassembler PUBLIC davecc_elf davecc_utils davecc_xtensa_isa)

davecc_library(davecc_ar
  c_compiler/AR/ar.c
)
target_link_libraries(davecc_ar PUBLIC davecc_elf davecc_utils)

davecc_library(davecc_loader
  c_compiler/Loader/Architecture/loader_arch_6502.c
  c_compiler/Loader/Architecture/loader_arch_aarch64.c
  c_compiler/Loader/Architecture/loader_arch_arm.c
  c_compiler/Loader/Architecture/loader_arch_x86_64.c
  c_compiler/Loader/Architecture/loader_arch_pcode.c
  c_compiler/Loader/Architecture/loader_arch_bpf.c
  c_compiler/Loader/Architecture/loader_arch_riscv.c
  c_compiler/Loader/Architecture/loader_arch_xtensa.c
  c_compiler/Loader/loader.c
  c_compiler/Loader/loader_dynamic.c
  c_compiler/Loader/loader_lifecycle.c
)
target_link_libraries(davecc_loader PUBLIC davecc_elf davecc_utils)

davecc_library(davecc_linker
  Linker/Architecture/linker_arch_6502.c
  Linker/Architecture/linker_arch_aarch64.c
  Linker/Architecture/linker_arch_arm.c
  Linker/Architecture/linker_arch_x86.c
  Linker/Architecture/linker_arch_x86_64.c
  Linker/Architecture/linker_arch_xtensa.c
  Linker/Architecture/linker_arch_pcode.c
  Linker/Architecture/linker_arch_bpf.c
  Linker/Architecture/linker_arch_riscv.c
  Linker/linker.c
  Linker/linker_config.c
  Linker/linker_script.c
  Linker/linker_dynamic.c
  Linker/linker_file.c
  Linker/linker_gc.c
  Linker/linker_main.c
  Linker/linker_reloc.c
  Linker/linker_stacktrace.c
  Linker/linker_symbols.c
)
target_link_libraries(davecc_linker PUBLIC davecc_ar davecc_elf davecc_loader davecc_utils davecc_xtensa_isa)

davecc_library(davecc_compiler
  c_compiler/6502/6502_assembler.c
  c_compiler/6502/6502_branches.c
  c_compiler/6502/6502_codegen.c
  c_compiler/6502/6502_emitter.c
  c_compiler/6502/6502_optimize.c
  c_compiler/6502/6502_reg_alloc.c
  c_compiler/6502/6502_spiller.c
  c_compiler/6502/6502_target.c
  c_compiler/aarch64/aarch64_assembler.c
  c_compiler/aarch64/aarch64_codegen.c
  c_compiler/aarch64/aarch64_encode.c
  c_compiler/aarch64/aarch64_emitter.c
  c_compiler/aarch64/aarch64_object.c
  c_compiler/aarch64/aarch64_program.c
  c_compiler/aarch64/aarch64_optimize.c
  c_compiler/aarch64/aarch64_reg_alloc.c
  c_compiler/aarch64/aarch64_target.c
  c_compiler/arm/arm_assembler.c
  c_compiler/arm/arm_codegen.c
  c_compiler/arm/arm_emitter.c
  c_compiler/arm/arm_optimize.c
  c_compiler/arm/arm_reg_alloc.c
  c_compiler/arm/arm_target.c
  c_compiler/bpf/bpf_assembler.c
  c_compiler/bpf/bpf_codegen.c
  c_compiler/bpf/bpf_emitter.c
  c_compiler/bpf/bpf_optimize.c
  c_compiler/bpf/bpf_reg_alloc.c
  c_compiler/bpf/bpf_target.c
  c_compiler/assembler/asm_expr.c
  c_compiler/assembler/asm_module.c
  c_compiler/assembler/asm_object.c
  c_compiler/assembler/assembler.c
  c_compiler/assembler/dwarf.c
  c_compiler/backend/basic_block.c
  c_compiler/backend/codegen.c
  c_compiler/backend/common_emitter.c
  c_compiler/backend/eh_abi_sections.c
  c_compiler/backend/eh_metadata.c
  c_compiler/backend/expr_codegen.c
  c_compiler/backend/ir.c
  c_compiler/backend/statement_codegen.c
  c_compiler/backend/target_basic_block.c
  c_compiler/backend/target_generator.c
  c_compiler/driver/compiler.c
  c_compiler/driver/errors.c
  c_compiler/driver/injection.c
  c_compiler/driver/listing.c
  c_compiler/driver/options.c
  c_compiler/lto/lto_module.c
  c_compiler/frontend/lexical/lex.c
  c_compiler/frontend/lexical/preprocessor.c
  c_compiler/frontend/lexical/source.c
  c_compiler/frontend/lexical/tokens.c
  c_compiler/frontend/lexical/unicode_name.c
  c_compiler/frontend/lexical/generated/unicode_name_data.c
  c_compiler/frontend/semantic/concepts.c
  c_compiler/frontend/semantic/contracts.c
  c_compiler/frontend/semantic/coro_semantics.c
  c_compiler/frontend/semantic/expr_semantics.c
  c_compiler/frontend/semantic/expansion_semantics.c
  c_compiler/frontend/semantic/init_semantics.c
  c_compiler/frontend/semantic/reflection_semantics.c
  c_compiler/frontend/semantic/reflection_meta_synthesis.c
  c_compiler/frontend/semantic/reflection_meta_traits.c
  c_compiler/frontend/semantic/rtti.c
  c_compiler/frontend/semantic/semantics.c
  c_compiler/frontend/semantic/statement_semantics.c
  c_compiler/frontend/semantic/type_traits_semantics.c
  c_compiler/frontend/syntax/ast.c
  c_compiler/frontend/syntax/constexpr.c
  c_compiler/frontend/syntax/constexpr_pcode.c
  c_compiler/frontend/syntax/debug.c
  c_compiler/frontend/syntax/dwarf_defs.c
  c_compiler/frontend/syntax/expr_evaluator.c
  c_compiler/frontend/syntax/expr_parser.c
  c_compiler/frontend/syntax/member_pointer.c
  c_compiler/frontend/syntax/module_identity.c
  c_compiler/frontend/syntax/module_syntax.c
  c_compiler/frontend/syntax/module_unit.c
  c_compiler/frontend/syntax/reflection.c
  c_compiler/frontend/syntax/statement_parser.c
  c_compiler/frontend/syntax/symbol.c
  c_compiler/frontend/syntax/symbol_table.c
  c_compiler/frontend/syntax/syntax.c
  c_compiler/frontend/syntax/typo_correction.c
  c_compiler/frontend/syntax/type_core.c
  c_compiler/frontend/syntax/type_print.c
  c_compiler/frontend/syntax/type_compare.c
  c_compiler/frontend/syntax/type_parse.c
  c_compiler/frontend/syntax/type_enum.c
  c_compiler/frontend/syntax/type_member.c
  c_compiler/frontend/syntax/type_layout.c
  c_compiler/frontend/syntax/type_inheritance.c
  c_compiler/frontend/syntax/type_special_member.c
  c_compiler/frontend/syntax/type_class.c
  c_compiler/frontend/syntax/type_template_substitute.c
  c_compiler/frontend/syntax/type_template_clone.c
  c_compiler/frontend/syntax/type_template_instantiate.c
  c_compiler/optimization/alias.c
  c_compiler/optimization/codemotion.c
  c_compiler/optimization/constprop.c
  c_compiler/optimization/copyprop.c
  c_compiler/optimization/dce.c
  c_compiler/optimization/gvn.c
  c_compiler/optimization/induction.c
  c_compiler/optimization/loop_info.c
  c_compiler/optimization/memopt.c
  c_compiler/optimization/optimizer.c
  c_compiler/optimization/sccp.c
  c_compiler/optimization/sroa.c
  c_compiler/optimization/ssa.c
  c_compiler/optimization/unroll.c
  c_compiler/optimization/vectorize.c
  c_compiler/p_code/p_code_assembler.c
  c_compiler/p_code/p_code_codegen.c
  c_compiler/p_code/p_code_encoding.c
  c_compiler/p_code/p_code_emitter.c
  c_compiler/p_code/p_code_object.c
  c_compiler/p_code/p_code_optimize.c
  c_compiler/p_code/p_code_reg_alloc.c
  c_compiler/p_code/p_code_target.c
  c_compiler/p_code/p_code_vm.c
  c_compiler/risc_v/risc_v_assembler.c
  c_compiler/risc_v/risc_v_codegen.c
  c_compiler/risc_v/risc_v_emitter.c
  c_compiler/risc_v/risc_v_optimize.c
  c_compiler/risc_v/risc_v_reg_alloc.c
  c_compiler/risc_v/risc_v_target.c
  c_compiler/risc_v32/risc_v32_assembler.c
  c_compiler/risc_v32/risc_v32_codegen.c
  c_compiler/risc_v32/risc_v32_emitter.c
  c_compiler/risc_v32/risc_v32_optimize.c
  c_compiler/risc_v32/risc_v32_reg_alloc.c
  c_compiler/risc_v32/risc_v32_target.c
  c_compiler/xtensa/xtensa_assembler.c
  c_compiler/xtensa/xtensa_codegen.c
  c_compiler/xtensa/xtensa_emitter.c
  c_compiler/xtensa/xtensa_optimize.c
  c_compiler/xtensa/xtensa_profile.c
  c_compiler/xtensa/xtensa_reg_alloc.c
  c_compiler/xtensa/xtensa_target.c
  c_compiler/wasm32/wasm32_codegen.c
  c_compiler/wasm32/wasm32_data.c
  c_compiler/wasm32/wasm32_emitter.c
  c_compiler/wasm32/wasm32_link.c
  c_compiler/wasm32/wasm32_module.c
  c_compiler/wasm32/wasm32_optimize.c
  c_compiler/wasm32/wasm32_reg_alloc.c
  c_compiler/wasm32/wasm32_stackify.c
  c_compiler/wasm32/wasm32_target.c
  c_compiler/x86/x86_assembler.c
  c_compiler/x86/x86_codegen.c
  c_compiler/x86/x86_i386_wide.c
  c_compiler/x86/x86_emitter.c
  c_compiler/x86/x86_optimize.c
  c_compiler/x86/x86_profile.c
  c_compiler/x86/x86_reg_alloc.c
  c_compiler/x86/x86_target.c
)
target_compile_definitions(davecc_compiler PUBLIC DAVECC_SYSROOT_HDRS=libc/include)
target_link_libraries(davecc_compiler PUBLIC
  davecc_disassembler
  davecc_elf
  davecc_utils
  davecc_wasm32_object
  davecc_xtensa_isa
)

davecc_library(davecc_serialize
  c_compiler/serialize/ast_serialize.c
  c_compiler/serialize/constraint_serialize.c
  c_compiler/serialize/ir_serialize.c
  c_compiler/serialize/lto_archive.c
  c_compiler/serialize/module_archive.c
  c_compiler/serialize/module_import.c
  c_compiler/serialize/module_install.c
  c_compiler/serialize/module_reachability.c
  c_compiler/serialize/serialize.c
  c_compiler/serialize/serialize_common.c
  c_compiler/serialize/symbol_serialize.c
  c_compiler/serialize/type_serialize.c
)
target_link_libraries(davecc_serialize PUBLIC
  davecc_ar
  davecc_compiler
  davecc_utils
  davecc_wireformat
)

davecc_library(davecc_6502_lib
  6502_interpreter/6502_debugger.c
  6502_interpreter/6502_devices.c
  6502_interpreter/6502_disassembler.c
  6502_interpreter/6502_interpreter.c
)
target_link_libraries(davecc_6502_lib PUBLIC
  davecc_chrono_host
  davecc_elf
  davecc_filesystem_host
  davecc_linker
  davecc_loader
  davecc_random_host
  davecc_utils
)

davecc_library(davecc_risc_v_lib
  risc_v_interpreter/risc_v_debugger.c
  risc_v_interpreter/risc_v_disassembler.c
  risc_v_interpreter/risc_v_interpreter.c
  risc_v_interpreter/risc_v_process.c
)
target_link_libraries(davecc_risc_v_lib PUBLIC
  davecc_chrono_host
  davecc_elf
  davecc_filesystem_host
  davecc_guest_addr_wait
  davecc_linker
  davecc_loader
  davecc_random_host
  davecc_utils
  m
)

davecc_library(davecc_aarch64_lib
  aarch64_interpreter/aarch64_interpreter.c
  aarch64_interpreter/aarch64_native.c
  aarch64_interpreter/aarch64_process.c
  aarch64_interpreter/aarch64_runtime.c
  aarch64_interpreter/aarch64_syscalls.c
)
target_compile_options(davecc_aarch64_lib PRIVATE ${DAVECC_AARCH64_COMPILE_OPTIONS})
target_link_libraries(davecc_aarch64_lib PUBLIC
  davecc_chrono_host
  davecc_elf
  davecc_filesystem_host
  davecc_guest_addr_wait
  davecc_linker
  davecc_loader
  davecc_random_host
  davecc_utils
)

davecc_library(davecc_arm_lib
  arm_interpreter/arm_interpreter.c
  arm_interpreter/arm_debugger.c
  arm_interpreter/arm_process.c
)
target_link_libraries(davecc_arm_lib PUBLIC
  davecc_chrono_host
  davecc_elf
  davecc_filesystem_host
  davecc_guest_addr_wait
  davecc_linker
  davecc_loader
  davecc_random_host
  davecc_utils
  m
)

davecc_library(davecc_x86_64_lib
  x86_64_interpreter/x86_64_interpreter.c
  x86_64_interpreter/x86_64_native.c
  x86_64_interpreter/x86_64_process.c
  x86_64_interpreter/x86_64_runtime.c
  x86_64_interpreter/x86_64_syscalls.c
)
target_link_libraries(davecc_x86_64_lib PUBLIC
  davecc_chrono_host
  davecc_elf
  davecc_filesystem_host
  davecc_guest_addr_wait
  davecc_linker
  davecc_loader
  davecc_random_host
  davecc_utils
)

davecc_library(davecc_xtensa_lib
  xtensa_interpreter/xtensa_interpreter.c
)
target_link_libraries(davecc_xtensa_lib PUBLIC davecc_elf davecc_loader davecc_utils davecc_xtensa_isa)

davecc_library(davecc_pcode_lib
  p_code_interpreter/p_code_disassembler.c
  p_code_interpreter/p_code_interpreter.c
  p_code_interpreter/p_code_syscalls.c
)
target_include_directories(davecc_pcode_lib PUBLIC p_code_interpreter)
target_link_libraries(davecc_pcode_lib PUBLIC
  davecc_chrono_host
  davecc_elf
  davecc_filesystem_host
  davecc_linker
  davecc_loader
  davecc_random_host
  davecc_utils
)

davecc_executable(davecc_bin davecc davecc/main.c)
target_link_libraries(davecc_bin PRIVATE
  davecc_ar
  davecc_compiler
  davecc_elf
  davecc_linker
  davecc_loader
  davecc_serialize
  davecc_utils
)

davecc_executable(archivist_bin archivist archivist/main.c)
target_link_libraries(archivist_bin PRIVATE
  davecc_6502_lib
  davecc_ar
  davecc_elf
  davecc_linker
  davecc_loader
  davecc_risc_v_lib
  davecc_utils
  davecc_wasm32_object
)

davecc_executable(elfdump_bin elfdump elfdump/main.c)
target_link_libraries(elfdump_bin PRIVATE davecc_disassembler davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(daveld_bin daveld daveld/main.c)
target_link_libraries(daveld_bin PRIVATE davecc_linker davecc_utils)

davecc_executable(davecc_6502_bin 6502 6502_interpreter/main.c)
target_link_libraries(davecc_6502_bin PRIVATE davecc_6502_lib davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(davecc_6502asm_bin 6502asm 6502asm/main.c)
target_link_libraries(davecc_6502asm_bin PRIVATE davecc_compiler davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(davecc_6502dasm_bin 6502dasm 6502dasm/main.c)
target_link_libraries(davecc_6502dasm_bin PRIVATE davecc_6502_lib davecc_elf davecc_loader davecc_utils)

davecc_executable(davecc_riscvdasm_bin riscvdasm riscvdasm/main.c)
target_link_libraries(davecc_riscvdasm_bin PRIVATE davecc_disassembler davecc_elf davecc_utils)

davecc_executable(davecc_aarch64dasm_bin aarch64dasm aarch64dasm/main.c)
target_compile_options(davecc_aarch64dasm_bin PRIVATE ${DAVECC_AARCH64_COMPILE_OPTIONS})
target_link_libraries(davecc_aarch64dasm_bin PRIVATE davecc_disassembler davecc_elf davecc_utils)

davecc_executable(davecc_armdasm_bin armdasm armdasm/main.c)
target_link_libraries(davecc_armdasm_bin PRIVATE davecc_disassembler davecc_elf davecc_utils)

davecc_executable(davecc_x86_64dasm_bin x86_64dasm x86_64dasm/main.c)
target_link_libraries(davecc_x86_64dasm_bin PRIVATE davecc_disassembler davecc_elf davecc_utils)

davecc_executable(davecc_xtensadasm_bin xtensadasm xtensadasm/main.c)
target_link_libraries(davecc_xtensadasm_bin PRIVATE davecc_disassembler davecc_elf davecc_utils)

davecc_executable(davecc_bpfdasm_bin bpfdasm bpfdasm/main.c)
target_link_libraries(davecc_bpfdasm_bin PRIVATE davecc_disassembler davecc_elf davecc_utils)

davecc_executable(davecc_pcodedasm_bin pcodedasm pcodedasm/main.c)
target_link_libraries(davecc_pcodedasm_bin PRIVATE davecc_disassembler davecc_elf davecc_utils)

davecc_executable(davecc_wasmdasm_bin wasmdasm wasmdasm/main.c)
target_link_libraries(davecc_wasmdasm_bin PRIVATE davecc_disassembler davecc_elf davecc_utils)

davecc_executable(davecc_x86asm_bin x86asm x86asm/main.c)
target_link_libraries(davecc_x86asm_bin PRIVATE davecc_compiler davecc_elf davecc_utils)

davecc_executable(davecc_rvasm_bin rvasm rvasm/main.c)
target_link_libraries(davecc_rvasm_bin PRIVATE davecc_compiler davecc_elf davecc_utils)

davecc_executable(davecc_aarch64asm_bin aarch64asm aarch64asm/main.c)
target_compile_options(davecc_aarch64asm_bin PRIVATE ${DAVECC_AARCH64_COMPILE_OPTIONS})
target_link_libraries(davecc_aarch64asm_bin PRIVATE davecc_compiler davecc_elf davecc_utils)

davecc_executable(davecc_armasm_bin armasm armasm/main.c)
target_link_libraries(davecc_armasm_bin PRIVATE davecc_compiler davecc_elf davecc_utils)

davecc_executable(davecc_xtensaasm_bin xtensaasm xtensaasm/main.c)
target_link_libraries(davecc_xtensaasm_bin PRIVATE davecc_compiler davecc_elf davecc_utils)

davecc_executable(davecc_riscv_bin riscv risc_v_interpreter/main.c)
target_link_libraries(davecc_riscv_bin PRIVATE davecc_elf davecc_linker davecc_loader davecc_risc_v_lib davecc_utils)

davecc_executable(davecc_aarch64_bin aarch64 aarch64_interpreter/main.c)
target_compile_options(davecc_aarch64_bin PRIVATE ${DAVECC_AARCH64_COMPILE_OPTIONS})
target_link_libraries(davecc_aarch64_bin PRIVATE davecc_aarch64_lib davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(davecc_arm_bin arm arm_interpreter/main.c)
target_link_libraries(davecc_arm_bin PRIVATE davecc_arm_lib davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(davecc_x86_64_bin x86_64 x86_64_interpreter/main.c)
target_link_libraries(davecc_x86_64_bin PRIVATE davecc_x86_64_lib davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(davecc_esp32_bin esp32 xtensa_interpreter/main.c)
target_link_libraries(davecc_esp32_bin PRIVATE davecc_xtensa_lib davecc_elf davecc_loader davecc_utils)

davecc_library(davecc_bpf_lib
  bpf_interpreter/bpf_interpreter.c
)
target_include_directories(davecc_bpf_lib PUBLIC bpf_interpreter)
target_link_libraries(davecc_bpf_lib PUBLIC davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(davecc_bpf_bin bpf bpf_interpreter/main.c)
target_include_directories(davecc_bpf_bin PRIVATE bpf_interpreter)
target_link_libraries(davecc_bpf_bin PRIVATE davecc_bpf_lib davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(davecc_pcode_bin pcode p_code_interpreter/main.c)
target_include_directories(davecc_pcode_bin PRIVATE p_code_interpreter)
target_link_libraries(davecc_pcode_bin PRIVATE
  davecc_elf
  davecc_linker
  davecc_loader
  davecc_pcode_lib
  davecc_utils
)

davecc_executable(davecc_run_bin run run/main.c)
target_link_libraries(davecc_run_bin PRIVATE davecc_elf davecc_utils)

add_custom_target(davecc_tools
  DEPENDS
    davecc_bin
    archivist_bin
    daveld_bin
    elfdump_bin
    davecc_6502_bin
    davecc_6502asm_bin
    davecc_6502dasm_bin
    davecc_riscvdasm_bin
    davecc_aarch64dasm_bin
    davecc_armdasm_bin
    davecc_x86_64dasm_bin
    davecc_xtensadasm_bin
    davecc_bpfdasm_bin
    davecc_pcodedasm_bin
    davecc_wasmdasm_bin
    davecc_x86asm_bin
    davecc_rvasm_bin
    davecc_aarch64asm_bin
    davecc_armasm_bin
    davecc_xtensaasm_bin
    davecc_riscv_bin
    davecc_aarch64_bin
    davecc_arm_bin
    davecc_x86_64_bin
    davecc_esp32_bin
    davecc_bpf_bin
    davecc_pcode_bin
    davecc_run_bin
)

include(cmake/DaveccGuestLibc.cmake)

file(GLOB DAVECC_PCODE_RUNTIME CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/pcode support/*.s")

davecc_guest_libc(libc65c02.a 65c02
  CFLAGS -Xkeep-asm
  SOURCES ${DAVECC_GUEST_LIBC_65C02_C}
  RUNTIME
    "6502 support/start.s"
    "6502 support/enter.s"
    "6502 support/var_addr.s"
    "6502 support/var_value.s"
    "6502 support/load_indirect.s"
    "6502 support/load_indirect8.s"
    "6502 support/store_indirect.s"
    "6502 support/store_indirect8.s"
    "6502 support/push.s"
    "6502 support/pull_drop.s"
    "6502 support/frame_push.s"
    "6502 support/inline_mem_params.s"
    "6502 support/pushmem.s"
    "6502 support/copymem.s"
    "6502 support/zeromem.s"
    "6502 support/incdec.s"
    "6502 support/jump_table.s"
    "6502 support/jmpi.s"
    "6502 support/mul.s"
    "6502 support/longmul.s"
    "6502 support/longlongmul.s"
    "6502 support/div.s"
    "6502 support/longdiv.s"
    "6502 support/longlongdiv.s"
    "6502 support/spill.s"
    "6502 support/fcommon.s"
    "6502 support/fadd.s"
    "6502 support/fconv.s"
    "6502 support/fcmp.s"
    "6502 support/fneg.s"
    "6502 support/fmul.s"
    "6502 support/fdiv.s"
    "6502 support/syscall.s"
    "6502 support/intrinsic.s"
    "6502 support/setjmp.s"
    "6502 support/stacktrace.s"
)
davecc_guest_libc(libcx86_64.a x86_64
  SHARED
  CFLAGS -O1
  EXTRA_C libc/guest_heap.c
  SHARED_EXCLUDE libc/abs.c
  RUNTIME
    "x86_64 support/cxx_guard.s"
    "x86_64 support/setjmp.s"
    "x86_64 support/longjmp.s"
    "x86_64 support/eh_transfer.s"
    "x86_64 support/syscall.s"
    "x86_64 support/abs.s"
)
davecc_guest_libc(libcx86.a x86
  SHARED
  CFLAGS -O0
  EXTRA_C libc/guest_heap.c
  RUNTIME
    "x86 support/cxx_guard.s"
    "x86 support/cxx_rtti_vtables.s"
    "x86 support/eh_transfer.s"
    "x86 support/setjmp.s"
    "x86 support/longjmp.s"
    "x86 support/stdio_aliases.s"
    "x86 support/syscall.s"
  SHARED_RUNTIME
    "x86 support/cxx_guard.s"
    "x86 support/cxx_rtti_vtables.s"
    "x86 support/eh_transfer.s"
    "x86 support/setjmp.s"
    "x86 support/longjmp.s"
    "x86 support/stdio_aliases.s"
    "x86 support/syscall.s"
)
davecc_guest_libc(libcriscv.a riscv
  SHARED
  CFLAGS -O1
  EXTRA_C libc/guest_heap.c
  RUNTIME
    "RISCV_support/cxx_guard.s"
    "RISCV_support/start.s"
    "RISCV_support/eh_transfer.s"
    "RISCV_support/longjmp.s"
    "RISCV_support/setjmp.s"
)
davecc_guest_libc(libcaarch64.a aarch64
  SHARED
  CFLAGS -O0
  EXTRA_C libc/guest_heap.c
  RUNTIME
    "aarch64 support/cxx_guard.s"
    "aarch64 support/eh_transfer.s"
    "aarch64 support/longjmp.s"
    "aarch64 support/setjmp.s"
    "aarch64 support/syscall.s"
)
davecc_guest_libc(libcriscv32.a riscv32
  SHARED
  CFLAGS -O1
  EXTRA_C libc/guest_heap.c
  RUNTIME
    "RISCV32_support/cxx_guard.s"
    "RISCV32_support/cxx_rtti_vtables.s"
    "RISCV32_support/start.s"
    "RISCV32_support/eh_transfer.s"
    "RISCV32_support/longjmp.s"
    "RISCV32_support/setjmp.s"
)
davecc_guest_libc(libcarm.a arm
  SHARED
  CFLAGS -O0
  EXTRA_C libc/guest_heap.c
  RUNTIME
    "arm support/cxx_guard.s"
    "arm support/cxx_rtti_vtables.s"
    "arm support/eh_personality.s"
    "arm support/eh_unwind.s"
    "arm support/eh_transfer.s"
    "arm support/longjmp.s"
    "arm support/setjmp.s"
    "arm support/syscall.s"
)
davecc_guest_libc(libcpcode.a pcode
  SHARED
  CFLAGS -O1
  EXTRA_C libc/guest_heap.c
  RUNTIME ${DAVECC_PCODE_RUNTIME}
  EXCLUDE libc/eh_frame.c
)
davecc_guest_libc(libcxtensa.a esp32
  CFLAGS -O1 -nostdinc -D__DAVECC_HAS_HEAP_LOCK__=1
  NO_CXX
  EXTRA_C xtensa_support/heap.c
  RUNTIME
    xtensa_support/syscall.s
    xtensa_support/setjmp.s
    xtensa_support/longjmp.s
  EXCLUDE
    libc/cxx_tls.c
    libc/cxx_tls_stubs.c
    libc/dynamic_*
    libc/eh_*
    libc/gxx_personality.c
    libc/linux_*
    libc/guest_heap.c
    libc/lsda.c
    libc/unwind_api.c
    libc/wasi_*
    libc/acos.c
    libc/asin.c
    libc/atan.c
    libc/atan2.c
    libc/ceil.c
    libc/ckdint.c
    libc/complex_*.c
    libc/exp.c
    libc/fabs.c
    libc/fenv.c
    libc/floor.c
    libc/fpfuncs.c
    libc/frexp.c
    libc/ftoa.c
    libc/ldexp.c
    libc/log.c
    libc/math_extra.c
    libc/modf.c
    libc/pow.c
    libc/scanf.c
    libc/sincos.c
    libc/sqrt.c
    libc/stdlib_extra.c
    libc/strtod.c
    libc/strtoll.c
    libc/strtoull.c
    libc/tan.c
    libc/threads.c
    libc/time.c
    libc/wchar.c
)

add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/libc/esp32_start.o"
  COMMAND $<TARGET_FILE:davecc_bin> -target esp32 -nostdinc -c
          "${CMAKE_SOURCE_DIR}/xtensa_support/start.s"
          -o "${CMAKE_BINARY_DIR}/libc/esp32_start.o"
  DEPENDS davecc_bin "${CMAKE_SOURCE_DIR}/xtensa_support/start.s"
  COMMENT "Assembling ESP32 start object"
  VERBATIM
)

add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/6502_support/6502rom.exe"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/6502_support"
  COMMAND $<TARGET_FILE:davecc_bin>
          -target 65c02
          -I "${CMAKE_SOURCE_DIR}/6502 support"
          -Wl,-T"${CMAKE_SOURCE_DIR}/6502 support/Interpreter ROM/introm.map"
          -Wl,-tintrom
          -static
          -nostdinc
          -nostdlib
          "${CMAKE_SOURCE_DIR}/6502 support/Interpreter ROM/introm.s"
          -o "${CMAKE_BINARY_DIR}/6502_support/6502rom.exe"
  DEPENDS davecc_bin
          "${CMAKE_SOURCE_DIR}/6502 support/Interpreter ROM/introm.s"
          "${CMAKE_SOURCE_DIR}/6502 support/Interpreter ROM/introm.map"
  COMMENT "Linking 65C02 interpreter ROM"
  VERBATIM
)

add_custom_target(davecc_libc
  DEPENDS
    davecc_libc65c02.a
    davecc_libcx86.a
    davecc_libcx86.so
    davecc_libcx86_crt.a
    davecc_libcx86_64.a
    davecc_libcx86_64.so
    davecc_libcx86_64_crt.a
    davecc_libcriscv.a
    davecc_libcriscv.so
    davecc_libcriscv_crt.a
    davecc_libcaarch64.a
    davecc_libcaarch64.so
    davecc_libcaarch64_crt.a
    davecc_libcriscv32.a
    davecc_libcriscv32.so
    davecc_libcriscv32_crt.a
    davecc_libcarm.a
    davecc_libcarm.so
    davecc_libcarm_crt.a
    davecc_libcpcode.a
    davecc_libcpcode.so
    davecc_libcpcode_crt.a
    davecc_libcxtensa.a
    "${CMAKE_BINARY_DIR}/libc/esp32_start.o"
    "${CMAKE_BINARY_DIR}/6502_support/6502rom.exe"
)

install(TARGETS
  davecc_bin
  archivist_bin
  davecc_6502_bin
  davecc_riscv_bin
  davecc_aarch64_bin
  davecc_arm_bin
  davecc_x86_64_bin
  davecc_esp32_bin
  davecc_bpf_bin
  davecc_pcode_bin
  RUNTIME DESTINATION "${DAVECC_INSTALL_LIBEXECDIR}"
)

install(TARGETS
  elfdump_bin
  daveld_bin
  davecc_6502asm_bin
  davecc_6502dasm_bin
  davecc_riscvdasm_bin
  davecc_aarch64dasm_bin
  davecc_armdasm_bin
  davecc_x86_64dasm_bin
  davecc_xtensadasm_bin
  davecc_bpfdasm_bin
  davecc_pcodedasm_bin
  davecc_wasmdasm_bin
  davecc_x86asm_bin
  davecc_rvasm_bin
  davecc_aarch64asm_bin
  davecc_armasm_bin
  davecc_xtensaasm_bin
  RUNTIME DESTINATION "${DAVECC_INSTALL_LIBEXECDIR}"
)

install(TARGETS
  elfdump_bin
  daveld_bin
  davecc_6502asm_bin
  davecc_6502dasm_bin
  davecc_riscvdasm_bin
  davecc_aarch64dasm_bin
  davecc_armdasm_bin
  davecc_x86_64dasm_bin
  davecc_xtensadasm_bin
  davecc_bpfdasm_bin
  davecc_pcodedasm_bin
  davecc_wasmdasm_bin
  davecc_x86asm_bin
  davecc_rvasm_bin
  davecc_aarch64asm_bin
  davecc_armasm_bin
  davecc_xtensaasm_bin
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(TARGETS davecc_run_bin RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(PROGRAMS
  bin/davecc-common.sh
  bin/davecc-65c02
  bin/davecc-aarch64
  bin/davecc-arm
  bin/davecc-riscv
  bin/davecc-riscv32
  bin/davecc-x86
  bin/davecc-x86_64
  bin/davecc-esp32
  bin/run-65c02
  bin/run-aarch64
  bin/run-arm
  bin/run-riscv
  bin/run-riscv32
  bin/run-esp32
  bin/run-x86
  bin/run-x86_64
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY libc/include/ DESTINATION "${DAVECC_INSTALL_INCLUDEDIR}")

install(CODE "
  file(WRITE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/davecc-env.sh\" \"# Generated by CMake install.\\nDAVECC_LIBEXEC_DIR=${CMAKE_INSTALL_PREFIX}/${DAVECC_INSTALL_LIBEXECDIR}\\nDAVECC_INCLUDE_DIR=${CMAKE_INSTALL_PREFIX}/${DAVECC_INSTALL_INCLUDEDIR}\\nDAVECC_LIB_DIR=${CMAKE_INSTALL_PREFIX}/${DAVECC_INSTALL_LIBDIR}\\nDAVECC_6502_ROM=${CMAKE_INSTALL_PREFIX}/${DAVECC_INSTALL_LIBDIR}/6502rom.exe\\n\")
")

foreach(archive
    libc65c02.a
    libcaarch64.a
    libcaarch64.so
    libcaarch64_crt.a
    libcarm.a
    libcarm.so
    libcarm_crt.a
    libcpcode.a
    libcpcode.so
    libcpcode_crt.a
    libcriscv.a
    libcriscv.so
    libcriscv_crt.a
    libcriscv32.a
    libcriscv32.so
    libcriscv32_crt.a
    libcxtensa.a
    libcx86.a
    libcx86.so
    libcx86_crt.a
    libcx86_64.a
    libcx86_64.so
    libcx86_64_crt.a)
  install(FILES "${CMAKE_BINARY_DIR}/libc/${archive}"
          DESTINATION "${DAVECC_INSTALL_LIBDIR}"
          OPTIONAL)
endforeach()

install(FILES
  "${CMAKE_BINARY_DIR}/6502_support/6502rom.exe"
  "${CMAKE_BINARY_DIR}/libc/esp32_start.o"
        DESTINATION "${DAVECC_INSTALL_LIBDIR}"
        OPTIONAL)

# Build guest archives before `cmake --install` so the OPTIONAL copies exist.
install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" --build \"${CMAKE_BINARY_DIR}\" --target davecc_libc)")
