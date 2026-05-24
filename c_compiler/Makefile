default: libcompiler.a

DAVECC_SYSROOT_HDRS = $(PREFIX)/libc/include
CC = clang
CFLAGS = \
	-I. \
	-IELF \
	-I6502 \
	-Iaarch64 \
	-Iarm \
	-Ip_code \
	-Irisc_v \
	$(OPT_DEBUG) $(ADD_FLAGS) \
 	-DDAVECC_SYSROOT_HDRS=$(DAVECC_SYSROOT_HDRS)

UTILS_OBJS = \
    dstring.o \
    vector.o \
    hashtable.o \
    map.o \
    list.o \
    binary_tree.o \
	set.o \
	bitset.o \
    buffer.o \
	config.o

RISC_V_OBJS = \
    risc_v/risc_v_reg_alloc.o \
    risc_v/risc_v_codegen.o \
    risc_v/risc_v_emitter.o \
    risc_v/risc_v_assembler.o \
    risc_v/risc_v_optimize.o \
    risc_v/risc_v_target.o \

AARCH64_OBJS = \
    aarch64/aarch64_reg_alloc.o \
    aarch64/aarch64_codegen.o \
    aarch64/aarch64_emitter.o \
    aarch64/aarch64_assembler.o \
    aarch64/aarch64_target.o \

ARM_OBJS = \
    arm/arm_reg_alloc.o \
    arm/arm_codegen.o \
    arm/arm_emitter.o \
    arm/arm_assembler.o \
    arm/arm_optimize.o \
    arm/arm_target.o \

P_CODE_OBJS = \
    p_code/p_code_reg_alloc.o \
    p_code/p_code_codegen.o \
    p_code/p_code_emitter.o \
    p_code/p_code_assembler.o \
    p_code/p_code_optimize.o \
    p_code/p_code_target.o \

6502_OBJS = \
    6502/6502_reg_alloc.o \
    6502/6502_codegen.o \
    6502/6502_emitter.o \
    6502/6502_assembler.o \
    6502/6502_target.o \
    6502/6502_optimize.o \
    6502/6502_branches.o \
    6502/6502_spiller.o \


COMPILER_OBJS = \
    tokens.o \
    source.o \
    lex.o \
    preprocessor.o \
    ast.o \
    errors.o \
    compiler.o \
    options.o \
	type.o \
    debug.o \
    dwarf_defs.o \
	syntax.o \
	symbol.o \
	symbol_table.o \
	expr_parser.o \
	expr_evaluator.o \
	statement_parser.o \
    semantics.o \
    expr_semantics.o \
    init_semantics.o \
    statement_semantics.o \
	ir.o \
	codegen.o \
	expr_codegen.o \
	statement_codegen.o \
	basic_block.o \
	ssa.o \
	gvn.o \
	constprop.o \
	codemotion.o \
	optimizer.o \
	target_generator.o \
        target_basic_block.o \
	common_emitter.o \
	assembler.o \
	asm_expr.o \
	dwarf.o \
	$(RISC_V_OBJS) \
	$(AARCH64_OBJS) \
	$(ARM_OBJS) \
	$(P_CODE_OBJS) \
	$(6502_OBJS) \


    
libutils.a: $(UTILS_OBJS)
	$(AR) -r $@  $(UTILS_OBJS)

libcompiler.a: $(COMPILER_OBJS) libutils.a
	$(AR) -r $@ $(COMPILER_OBJS)

clean:
	rm -f $(UTILS_OBJS) $(COMPILER_OBJS) libutils.a libcompiler.a

