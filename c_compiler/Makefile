default: libcompiler.so

CC = davecc
CFLAGS = -target risc-v -fPIC -O2 \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler \
	-I/Users/dallison/Google\ Drive/c_compiler/libc/include \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/ELF \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/6502 \
    -Xkeep-asm -Xsave-ir

UTILS_OBJS = \
    dstring.o \
    vector.o \
    hashtable.o \
    map.o \
    list.o \
    binary_tree.o \
	set.o \
	bitset.o \
    buffer.o

RISC_V_OBJS = \
    risc_v_reg_alloc.o \
    risc_v_codegen.o \
    risc_v_emitter.o \
    risc_v_assembler.o \
    risc_v_optimize.o \
    risc_v_target.o \

P_CODE_OBJS = \
    p_code_reg_alloc.o \
    p_code_codegen.o \
    p_code_emitter.o \
    p_code_assembler.o \
    p_code_optimize.o \
    p_code_target.o \

6502_OBJS = \
    6502/6502_reg_alloc.o \
    6502/6502_codegen.o \
    6502/6502_emitter.o \
    6502/6502_assembler.o \
    6502/6502_target.o \


COMPILER_OBJS = \
    tokens.o \
    source.o \
    lex.o \
    preprocessor.o \
    ast.o \
    errors.o \
    compiler.o \
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
	optimizer.o \
	target_generator.o \
	target_basic_block.o \
	assembler.o \
	dwarf.o \
	$(RISC_V_OBJS) \
	$(P_CODE_OBJS) \
	$(6502_OBJS) \


    
LDFLAGS = -L/Users/dallison/Google\ Drive/c_compiler/libc -lc \
	-rpath /Users/dallison/Google\ Drive/c_compiler/libc

libutils.so: $(UTILS_OBJS)
	$(CC) -o $@  -shared $(UTILS_OBJS) $(LDFLAGS)

libcompiler.so: $(COMPILER_OBJS) libutils.so
	$(CC) -o $@  -shared $(COMPILER_OBJS) libutils.so $(LDFLAGS)

clean:
	rm -f $(UTILS_OBJS) $(COMPILER_OBJS) libutils.so
