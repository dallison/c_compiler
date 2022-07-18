CFLAGS = -I../c_compiler -I../c_compiler/ELF -I../c_compiler/6502 -I../Linker -I../c_compiler/Loader -I../c_compiler/Loader/Architecture
LDFLAGS = -L../c_compiler -lutils -lcompiler -L../Linker -llink -L../c_compiler/Loader -lloader -L../c_compiler/ELF -lcelf \
	  -Wl,-rpath $(PWD)/../c_compiler \
	  -Wl,-rpath $(PWD)/../c_compiler/ELF \
	  -Wl,-rpath $(PWD)/../Linker -Wl,-rpath $(PWD)/../c_compiler/Loader
OBJS = \
	6502_debugger.o \
	6502_devices.o \
	6502_disassembler.o \
	6502_interpreter.o \
	main.o

EXE = 6502

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean:
	$(RM) -f $(OBJS) $(EXE)

