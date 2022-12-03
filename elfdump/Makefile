CFLAGS = -I../c_compiler -I../c_compiler/ELF -I../c_compiler/6502 -I../Linker -I../c_compiler/Loader -I../c_compiler/Loader/Architecture -I../risc_v_interpreter -I../6502_interpreter
LDFLAGS = -L../c_compiler -lutils -lcompiler -L../Linker -llinker -L../c_compiler/Loader -lloader -L../c_compiler/ELF -lelf -L../risc_v_interpreter -lrisc_v -L../6502_interpreter -l6502

OBJS = \
	main.o

EXE = elfdump

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean:
	$(RM) -f $(OBJS) $(EXE)

