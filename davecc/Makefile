CFLAGS = -I../c_compiler \
	-I../c_compiler/ELF \
	-I../c_compiler/AR \
	-I../c_compiler/6502 \
	-I../c_compiler/aarch64 \
	-I../c_compiler/arm \
	-I../c_compiler/p_code \
	-I../c_compiler/risc_v \
	-I../Linker \
	-I../c_compiler/Loader \
	-I../c_compiler/Loader/Architecture \
	$(OPT_DEBUG) $(ADD_FLAGS)

LDFLAGS = -L../Linker -llinker \
	-L../c_compiler -lcompiler -lutils \
	-L../c_compiler/Loader -lloader \
	-L../c_compiler/ELF -lelf \
	-L../c_compiler/AR -lar

OBJS = \
	main.o

EXE = davecc

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean:
	$(RM) -f $(OBJS) $(EXE)

