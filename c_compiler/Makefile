DAVECC_SYSROOT_HDRS = $(PWD)/../libc/include

default: $(COMP_LIB)

CFLAGS = -fPIC -I$(PWD)/ELF -I$(PWD) -I$(PWD)/6502 -DDAVECC_SYSROOT_HDRS=$(DAVECC_SYSROOT_HDRS)

UTILS_OBJS = dstring.o list.o vector.o map.o hashtable.o buffer.o set.o bitset.o binary_tree.o config.o

UTILS_LIB = libutils.so

$(UTILS_LIB): $(UTILS_OBJS)
	$(CC) -o $(UTILS_LIB) $(UTILS_OBJS) -shared

clean:
	$(RM) -f $(UTILS_OBJS) $(UTILS_LIB) $(COMP_OBJS)


LEX_OBJS = lex.o tokens.o source.o preprocessor.o

SYNTAX_OBJS = \
	ast.o \
	expr_parser.o \
	statement_parser.o \
	symbol.o \
	symbol_table.o \
	syntax.o \
	type.o 

SEMANTIC_OBJS = \
	expr_evaluator.o \
	expr_semantics.o \
	init_semantics.o \
	statement_semantics.o \
	semantics.o

CODEGEN_OBJS = \
	codegen.o \
	basic_block.o \
	common_emitter.o \
	expr_codegen.o \
	ir.o \
	statement_codegen.o \
	var_analysis.o \
	target_basic_block.o \
	target_generator.o

OPTIMIZER_OBJS = \
	codemotion.o \
	constprop.o \
	gvn.o \
	optimizer.o \
	ssa.o

ASSEMBLER_OBJS = \
	assembler.o \
	asm_expr.o \
	dwarf.o \
	dwarf_defs.o

COMMON_OBJS =  \
	compiler.o \
	debug.o \
	errors.o


PCODE_OBJS = \
	p_code_assembler.o\
	p_code_codegen.o\
	p_code_emitter.o\
	p_code_optimize.o\
	p_code_reg_alloc.o\
	p_code_target.o

RISCV_OBJS = \
	risc_v_assembler.o\
	risc_v_codegen.o\
	risc_v_emitter.o\
	risc_v_optimize.o\
	risc_v_reg_alloc.o\
	risc_v_target.o

6502_OBJS = \
	6502/6502_reg_alloc.o \
	6502/6502_assembler.o \
	6502/6502_branches.o \
	6502/6502_codegen.o \
	6502/6502_emitter.o \
	6502/6502_optimize.o \
	6502/6502_spiller.o \
	6502/6502_target.o

COMP_OBJS = $(COMMON_OBJS) $(LEX_OBJS) $(SYNTAX_OBJS) $(SEMANTIC_OBJS) $(CODEGEN_OBJS) $(OPTIMIZER_OBJS) $(ASSEMLBER_OBJS) $(PCODE_OBJS) $(RISCV_OBJS) $(6502_OBJS) $(ASSEMBLER_OBJS)


COMP_LIB = libcompiler.so

COMP_LDFLAGS = -L. -LELF -lcelf -lutils -Wl,-rpath $(PWD)/ELF

$(COMP_LIB) : $(COMP_OBJS) $(UTILS_LIB)
	$(CC) -o $(COMP_LIB) $(COMP_OBJS) -shared $(COMP_LDFLAGS)



