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
  c_compiler/driver
  c_compiler/disassembler
  c_compiler/6502
  c_compiler/aarch64
  c_compiler/arm
  c_compiler/p_code
  c_compiler/risc_v
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

davecc_library(davecc_disassembler
  c_compiler/disassembler/disassembler.c
  c_compiler/disassembler/disassembler_aarch64.c
  c_compiler/disassembler/disassembler_arm.c
  c_compiler/disassembler/disassembler_riscv.c
  c_compiler/disassembler/disassembler_tool.c
  c_compiler/disassembler/disassembler_x86_64.c
)
target_link_libraries(davecc_disassembler PUBLIC davecc_elf davecc_utils)

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
  c_compiler/Loader/Architecture/loader_arch_riscv.c
  c_compiler/Loader/loader.c
  c_compiler/Loader/loader_dynamic.c
)
target_link_libraries(davecc_loader PUBLIC davecc_elf davecc_utils)

davecc_library(davecc_linker
  Linker/Architecture/linker_arch_6502.c
  Linker/Architecture/linker_arch_aarch64.c
  Linker/Architecture/linker_arch_arm.c
  Linker/Architecture/linker_arch_x86_64.c
  Linker/Architecture/linker_arch_pcode.c
  Linker/Architecture/linker_arch_riscv.c
  Linker/linker.c
  Linker/linker_config.c
  Linker/linker_dynamic.c
  Linker/linker_file.c
  Linker/linker_main.c
  Linker/linker_reloc.c
  Linker/linker_symbols.c
)
target_link_libraries(davecc_linker PUBLIC davecc_ar davecc_elf davecc_loader davecc_utils)

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
  c_compiler/aarch64/aarch64_emitter.c
  c_compiler/aarch64/aarch64_reg_alloc.c
  c_compiler/aarch64/aarch64_target.c
  c_compiler/arm/arm_assembler.c
  c_compiler/arm/arm_codegen.c
  c_compiler/arm/arm_emitter.c
  c_compiler/arm/arm_optimize.c
  c_compiler/arm/arm_reg_alloc.c
  c_compiler/arm/arm_target.c
  c_compiler/assembler/asm_expr.c
  c_compiler/assembler/assembler.c
  c_compiler/assembler/dwarf.c
  c_compiler/backend/basic_block.c
  c_compiler/backend/codegen.c
  c_compiler/backend/common_emitter.c
  c_compiler/backend/expr_codegen.c
  c_compiler/backend/ir.c
  c_compiler/backend/statement_codegen.c
  c_compiler/backend/target_basic_block.c
  c_compiler/backend/target_generator.c
  c_compiler/driver/compiler.c
  c_compiler/driver/errors.c
  c_compiler/driver/options.c
  c_compiler/frontend/lexical/lex.c
  c_compiler/frontend/lexical/preprocessor.c
  c_compiler/frontend/lexical/source.c
  c_compiler/frontend/lexical/tokens.c
  c_compiler/frontend/semantic/expr_semantics.c
  c_compiler/frontend/semantic/init_semantics.c
  c_compiler/frontend/semantic/semantics.c
  c_compiler/frontend/semantic/statement_semantics.c
  c_compiler/frontend/syntax/ast.c
  c_compiler/frontend/syntax/debug.c
  c_compiler/frontend/syntax/dwarf_defs.c
  c_compiler/frontend/syntax/expr_evaluator.c
  c_compiler/frontend/syntax/expr_parser.c
  c_compiler/frontend/syntax/statement_parser.c
  c_compiler/frontend/syntax/symbol.c
  c_compiler/frontend/syntax/symbol_table.c
  c_compiler/frontend/syntax/syntax.c
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
  c_compiler/optimization/codemotion.c
  c_compiler/optimization/constprop.c
  c_compiler/optimization/gvn.c
  c_compiler/optimization/optimizer.c
  c_compiler/optimization/ssa.c
  c_compiler/p_code/p_code_assembler.c
  c_compiler/p_code/p_code_codegen.c
  c_compiler/p_code/p_code_emitter.c
  c_compiler/p_code/p_code_optimize.c
  c_compiler/p_code/p_code_reg_alloc.c
  c_compiler/p_code/p_code_target.c
  c_compiler/risc_v/risc_v_assembler.c
  c_compiler/risc_v/risc_v_codegen.c
  c_compiler/risc_v/risc_v_emitter.c
  c_compiler/risc_v/risc_v_optimize.c
  c_compiler/risc_v/risc_v_reg_alloc.c
  c_compiler/risc_v/risc_v_target.c
  c_compiler/x86_64/x86_64_assembler.c
  c_compiler/x86_64/x86_64_codegen.c
  c_compiler/x86_64/x86_64_emitter.c
  c_compiler/x86_64/x86_64_optimize.c
  c_compiler/x86_64/x86_64_reg_alloc.c
  c_compiler/x86_64/x86_64_target.c
)
target_compile_definitions(davecc_compiler PUBLIC DAVECC_SYSROOT_HDRS=libc/include)
target_link_libraries(davecc_compiler PUBLIC davecc_elf davecc_utils)

davecc_library(davecc_6502_lib
  6502_interpreter/6502_debugger.c
  6502_interpreter/6502_devices.c
  6502_interpreter/6502_disassembler.c
  6502_interpreter/6502_interpreter.c
)
target_link_libraries(davecc_6502_lib PUBLIC davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_library(davecc_risc_v_lib
  risc_v_interpreter/risc_v_debugger.c
  risc_v_interpreter/risc_v_disassembler.c
  risc_v_interpreter/risc_v_interpreter.c
)
target_link_libraries(davecc_risc_v_lib PUBLIC davecc_elf davecc_linker davecc_loader davecc_utils m)

davecc_library(davecc_aarch64_lib
  aarch64_interpreter/aarch64_interpreter.c
  aarch64_interpreter/aarch64_native.c
  aarch64_interpreter/aarch64_runtime.c
  aarch64_interpreter/aarch64_syscalls.c
)
target_compile_options(davecc_aarch64_lib PRIVATE ${DAVECC_AARCH64_COMPILE_OPTIONS})
target_link_libraries(davecc_aarch64_lib PUBLIC davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_library(davecc_arm_lib
  arm_interpreter/arm_interpreter.c
  arm_interpreter/arm_debugger.c
)
target_link_libraries(davecc_arm_lib PUBLIC davecc_elf davecc_linker davecc_loader davecc_utils m)

davecc_library(davecc_x86_64_lib
  x86_64_interpreter/x86_64_interpreter.c
  x86_64_interpreter/x86_64_native.c
  x86_64_interpreter/x86_64_runtime.c
  x86_64_interpreter/x86_64_syscalls.c
)
target_link_libraries(davecc_x86_64_lib PUBLIC davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(davecc_bin davecc davecc/main.c)
target_link_libraries(davecc_bin PRIVATE davecc_ar davecc_compiler davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(archivist_bin archivist archivist/main.c)
target_link_libraries(archivist_bin PRIVATE davecc_6502_lib davecc_ar davecc_elf davecc_linker davecc_loader davecc_risc_v_lib davecc_utils)

davecc_executable(elfdump_bin elfdump elfdump/main.c)
target_link_libraries(elfdump_bin PRIVATE davecc_disassembler davecc_elf davecc_linker davecc_loader davecc_utils)

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

davecc_executable(davecc_x86asm_bin x86asm x86asm/main.c)
target_link_libraries(davecc_x86asm_bin PRIVATE davecc_compiler davecc_elf davecc_utils)

davecc_executable(davecc_rvasm_bin rvasm rvasm/main.c)
target_link_libraries(davecc_rvasm_bin PRIVATE davecc_compiler davecc_elf davecc_utils)

davecc_executable(davecc_aarch64asm_bin aarch64asm aarch64asm/main.c)
target_compile_options(davecc_aarch64asm_bin PRIVATE ${DAVECC_AARCH64_COMPILE_OPTIONS})
target_link_libraries(davecc_aarch64asm_bin PRIVATE davecc_compiler davecc_elf davecc_utils)

davecc_executable(davecc_armasm_bin armasm armasm/main.c)
target_link_libraries(davecc_armasm_bin PRIVATE davecc_compiler davecc_elf davecc_utils)

davecc_executable(davecc_riscv_bin riscv risc_v_interpreter/main.c)
target_link_libraries(davecc_riscv_bin PRIVATE davecc_elf davecc_linker davecc_loader davecc_risc_v_lib davecc_utils)

davecc_executable(davecc_aarch64_bin aarch64 aarch64_interpreter/main.c)
target_compile_options(davecc_aarch64_bin PRIVATE ${DAVECC_AARCH64_COMPILE_OPTIONS})
target_link_libraries(davecc_aarch64_bin PRIVATE davecc_aarch64_lib davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(davecc_arm_bin arm arm_interpreter/main.c)
target_link_libraries(davecc_arm_bin PRIVATE davecc_arm_lib davecc_elf davecc_linker davecc_loader davecc_utils)

davecc_executable(davecc_x86_64_bin x86_64 x86_64_interpreter/main.c)
target_link_libraries(davecc_x86_64_bin PRIVATE davecc_x86_64_lib davecc_elf davecc_linker davecc_loader davecc_utils)

add_custom_target(davecc_tools
  DEPENDS
    davecc_bin
    archivist_bin
    elfdump_bin
    davecc_6502_bin
    davecc_6502asm_bin
    davecc_6502dasm_bin
    davecc_riscvdasm_bin
    davecc_aarch64dasm_bin
    davecc_armdasm_bin
    davecc_x86_64dasm_bin
    davecc_x86asm_bin
    davecc_rvasm_bin
    davecc_aarch64asm_bin
    davecc_armasm_bin
    davecc_riscv_bin
    davecc_aarch64_bin
    davecc_arm_bin
    davecc_x86_64_bin
)

install(TARGETS
  davecc_bin
  archivist_bin
  davecc_6502_bin
  davecc_riscv_bin
  davecc_aarch64_bin
  davecc_arm_bin
  davecc_x86_64_bin
  RUNTIME DESTINATION "${DAVECC_INSTALL_LIBEXECDIR}"
)

install(TARGETS
  elfdump_bin
  davecc_6502asm_bin
  davecc_6502dasm_bin
  davecc_riscvdasm_bin
  davecc_aarch64dasm_bin
  davecc_armdasm_bin
  davecc_x86_64dasm_bin
  davecc_x86asm_bin
  davecc_rvasm_bin
  davecc_aarch64asm_bin
  davecc_armasm_bin
  RUNTIME DESTINATION "${DAVECC_INSTALL_LIBEXECDIR}"
)

install(TARGETS
  elfdump_bin
  davecc_6502asm_bin
  davecc_6502dasm_bin
  davecc_riscvdasm_bin
  davecc_aarch64dasm_bin
  davecc_armdasm_bin
  davecc_x86_64dasm_bin
  davecc_x86asm_bin
  davecc_rvasm_bin
  davecc_aarch64asm_bin
  davecc_armasm_bin
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(PROGRAMS
  bin/davecc-common.sh
  bin/davecc-65c02
  bin/davecc-aarch64
  bin/davecc-arm
  bin/davecc-riscv
  bin/davecc-x86_64
  bin/run-65c02
  bin/run-aarch64
  bin/run-arm
  bin/run-riscv
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
    libcarm.a
    libcriscv.a
    libcx86_64.a)
  install(FILES "${CMAKE_BINARY_DIR}/libc/${archive}"
          DESTINATION "${DAVECC_INSTALL_LIBDIR}"
          OPTIONAL)
endforeach()

install(FILES "${CMAKE_BINARY_DIR}/6502_support/6502rom.exe"
        DESTINATION "${DAVECC_INSTALL_LIBDIR}"
        OPTIONAL)
