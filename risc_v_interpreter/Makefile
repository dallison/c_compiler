CC = clang
CFLAGS = -I../c_compiler \
	-I../c_compiler/ELF \
	-I../c_compiler/AR \
	-I../c_compiler/6502 \
	-I../Linker \
	-I../c_compiler/Loader \
	-I../c_compiler/Loader/Architecture \
	$(OPT_DEBUG)

LDFLAGS = -L../c_compiler/Loader -lloader \
	-L../c_compiler -lutils \
	-L../Linker -llinker  \
	-L../c_compiler/ELF -lelf  \
	-lm


LIB_OBJS = \
  risc_v_disassembler.o \
  risc_v_interpreter.o \
  risc_v_debugger.o

EXE_OBJS = \
  main.o 

LIB = librisc_v.a
EXE = riscv
default: $(EXE)

$(LIB): $(LIB_OBJS)
	$(AR) -r $@ $(LIB_OBJS)

$(EXE): $(LIB) $(EXE_OBJS)
	$(CC) -o $@ $(EXE_OBJS) $(LIB) $(LDFLAGS)

clean:
	rm -f $(EXE) $(LIB) $(LIB_OBJS) $(EXE_OBJS)

