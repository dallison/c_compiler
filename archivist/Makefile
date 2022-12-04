CFLAGS = -I../c_compiler \
	-I../c_compiler/ELF \
 	-I../c_compiler/6502 \
	-I../Linker \
	-I../c_compiler/Loader \
	-I../c_compiler/Loader/Architecture \
	-I../risc_v_interpreter \
	-I../6502_interpreter \
	-I../c_compiler/AR \
	$(OPT_DEBUG)

LDFLAGS = -L../c_compiler/AR -lar \
	-L../risc_v_interpreter -lrisc_v \
	-L../6502_interpreter -l6502 \
	-L../c_compiler/Loader -lloader \
	-L../c_compiler -lutils \
	-L../Linker -llinker \
	-L../c_compiler/ELF -lelf \
	-lm 

OBJS = \
	main.o

EXE = archivist

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean:
	$(RM) -f $(OBJS) $(EXE)

