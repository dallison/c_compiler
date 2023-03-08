CFLAGS = -I../c_compiler \
	-I../c_compiler/ELF \
	-I../c_compiler/6502 \
	-I../Linker \
	-I../c_compiler/Loader \
	-I../c_compiler/Loader/Architecture \
	$(OPT_DEBUG) $(ADD_FLAGS)

LDFLAGS = -L../Linker -llinker \
	-L../c_compiler/Loader -lloader \
	-L../c_compiler -lcompiler -lutils \
	-L../c_compiler/ELF -lelf

OBJS = \
	main.o

EXE = 6502asm

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean:
	$(RM) -f $(OBJS) $(EXE)

