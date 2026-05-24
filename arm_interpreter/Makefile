CC = clang
CFLAGS = -I../c_compiler \
	-I../c_compiler/ELF \
	-I../c_compiler/AR \
	-I../c_compiler/6502 \
	-I../c_compiler/arm \
	-I../Linker \
	-I../c_compiler/Loader \
	-I../c_compiler/Loader/Architecture \
	$(OPT_DEBUG) $(ADD_FLAGS)

LDFLAGS = -L../c_compiler/Loader -lloader \
	-L../c_compiler -lutils \
	-L../Linker -llinker  \
	-L../c_compiler/ELF -lelf  \
	-lm


LIB_OBJS = \
  arm_disassembler.o \
  arm_interpreter.o \
  arm_debugger.o

EXE_OBJS = \
  main.o 

LIB = libarm.a
EXE = arm
default: $(EXE)

$(LIB): $(LIB_OBJS)
	$(AR) -r $@ $(LIB_OBJS)

$(EXE): $(LIB) $(EXE_OBJS)
	$(CC) -o $@ $(EXE_OBJS) $(LIB) $(LDFLAGS)

clean:
	rm -f $(EXE) $(LIB) $(LIB_OBJS) $(EXE_OBJS)

