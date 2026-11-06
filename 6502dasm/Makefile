CC = clang

CFLAGS = -I../c_compiler \
	-I../c_compiler/disassembler \
	-I../c_compiler/ELF \
	$(OPT_DEBUG) $(ADD_FLAGS)

OBJS = \
  main.o 

LIBRARIES =  \
	-L../c_compiler -ldisassembler \
	-L../c_compiler/ELF -lelf \
	-L../c_compiler -lutils

6502dasm: $(OBJS)
	$(CC) -o $@  $(OBJS) $(LIBRARIES)

clean:
	rm -f $(OBJS) 6502dasm
