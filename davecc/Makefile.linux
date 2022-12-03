CC = clang

CFLAGS= \
	-I../c_compiler \
	-I../c_compiler/ELF \
	-I../c_compiler/6502 \
	-I../c_compiler/AR \
	-I../Linker \
	-I../Linker/Architecture \
	-I../c_compiler/Loader \
	-I../c_compiler/Loader/Architecture \
    -Wno-unused-function \
	-g \
    -DDAVECC_SYSROOT_HDRS="/home/dallison/c_compiler/libc/include"


EXE = davecc


UTILS_OBJS = \
    ../c_compiler/dstring.o \
    ../c_compiler/vector.o \
    ../c_compiler/hashtable.o \
    ../c_compiler/map.o \
    ../c_compiler/list.o \
    ../c_compiler/binary_tree.o \
	../c_compiler/set.o \
	../c_compiler/bitset.o \
    ../c_compiler/buffer.o \
	../c_compiler/config.o

RISC_V_OBJS = \
    ../c_compiler/risc_v_reg_alloc.o \
    ../c_compiler/risc_v_codegen.o \
    ../c_compiler/risc_v_emitter.o \
    ../c_compiler/risc_v_assembler.o \
    ../c_compiler/risc_v_optimize.o \
    ../c_compiler/risc_v_target.o \

P_CODE_OBJS = \
    ../c_compiler/p_code_reg_alloc.o \
    ../c_compiler/p_code_codegen.o \
    ../c_compiler/p_code_emitter.o \
    ../c_compiler/p_code_assembler.o \
    ../c_compiler/p_code_optimize.o \
    ../c_compiler/p_code_target.o \

6502_OBJS = \
    ../c_compiler/6502/6502_reg_alloc.o \
    ../c_compiler/6502/6502_codegen.o \
    ../c_compiler/6502/6502_emitter.o \
    ../c_compiler/6502/6502_assembler.o \
    ../c_compiler/6502/6502_target.o \
    ../c_compiler/6502/6502_optimize.o \
    ../c_compiler/6502/6502_branches.o \
    ../c_compiler/6502/6502_spiller.o \


COMPILER_OBJS = \
    ../c_compiler/tokens.o \
    ../c_compiler/source.o \
    ../c_compiler/lex.o \
    ../c_compiler/preprocessor.o \
    ../c_compiler/ast.o \
    ../c_compiler/errors.o \
    ../c_compiler/compiler.o \
	../c_compiler/type.o \
    ../c_compiler/debug.o \
    ../c_compiler/dwarf_defs.o \
	../c_compiler/syntax.o \
	../c_compiler/symbol.o \
	../c_compiler/symbol_table.o \
	../c_compiler/expr_parser.o \
	../c_compiler/expr_evaluator.o \
	../c_compiler/statement_parser.o \
    ../c_compiler/semantics.o \
    ../c_compiler/expr_semantics.o \
    ../c_compiler/init_semantics.o \
    ../c_compiler/statement_semantics.o \
	../c_compiler/ir.o \
	../c_compiler/codegen.o \
	../c_compiler/expr_codegen.o \
	../c_compiler/statement_codegen.o \
	../c_compiler/basic_block.o \
	../c_compiler/ssa.o \
	../c_compiler/gvn.o \
	../c_compiler/constprop.o \
	../c_compiler/codemotion.o \
	../c_compiler/optimizer.o \
	../c_compiler/target_generator.o \
        ../c_compiler/target_basic_block.o \
	../c_compiler/assembler.o \
	../c_compiler/asm_expr.o \
	../c_compiler/dwarf.o \
	../c_compiler/common_emitter.o \
	$(RISC_V_OBJS) \
	$(P_CODE_OBJS) \
	$(6502_OBJS) \


AR_OBJS = \
	../c_compiler/AR/ar.o

ELF_OBJS = \
  ../c_compiler/ELF/elf_reader.o \
  ../c_compiler/ELF/elf_writer.o

    
LOADER_ARCH_OBJS = \
	../c_compiler/Loader/Architecture/loader_arch_riscv.o \
	../c_compiler/Loader/Architecture/loader_arch_pcode.o \
	../c_compiler/Loader/Architecture/loader_arch_6502.o \

LOADER_OBJS = \
	../c_compiler/Loader/loader.o \
	../c_compiler/Loader/loader_dynamic.o \
	$(LOADER_ARCH_OBJS)


LINKER_ARCH_OBJS = \
	../Linker/Architecture/linker_arch_riscv.o \
	../Linker/Architecture/linker_arch_pcode.o \
	../Linker/Architecture/linker_arch_6502.o \

LINKER_OBJS = \
	../Linker/linker.o \
	../Linker/linker_main.o \
	../Linker/linker_file.o \
	../Linker/linker_symbols.o \
	../Linker/linker_reloc.o \
	../Linker/linker_dynamic.o \
	../Linker/linker_config.o \
	$(LINKER_ARCH_OBJS)

MAIN_OBJS = \
	main.o

OBJS = $(MAIN_OBJS) $(UTILS_OBJS) $(COMPILER_OBJS) $(AR_OBJS) $(ELF_OBJS) $(LOADER_OBJS) $(LINKER_OBJS)

$(EXE) : $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) $(EXE)


