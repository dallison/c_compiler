CFLAGS = -I../c_compiler \
	-I../c_compiler/ELF \
	-I../c_compiler/6502 \
	-I../Linker \
	-I../c_compiler/Loader \
	-I../c_compiler/Loader/Architecture \
	$(OPT_DEBUG)

LDFLAGS = -L../c_compiler/Loader -lloader \
	-L../c_compiler -lutils \
	-L../Linker -llinker \
	-L../c_compiler/ELF -lelf 

LIB_OBJS = \
	6502_debugger.o \
	6502_devices.o \
	6502_disassembler.o \
	6502_interpreter.o \

EXE_OBJS = \
	main.o

LIB = lib6502.a

EXE = 6502
default: $(EXE) 

$(LIB) : $(LIB_OBJS)
	$(AR) -r $@ $(LIB_OBJS)

$(EXE) : $(EXE_OBJS) $(LIB)
	$(CC) -o $(EXE) $(EXE_OBJS) $(LIB_OBJS) $(LDFLAGS)

clean:
	$(RM) -f $(EXE_OBJS) $(LIB_OBJS) $(EXE) $(LIB)

