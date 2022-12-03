CC = clang

CFLAGS = -I../c_compiler \
	-I../c_compiler/6502 \
	-I../6502_interpreter \
	-I../c_compiler/Loader \
	-I../c_compiler/ELF \
	-I../c_compiler/Loader/Architecture

OBJS = \
  main.o \
  ../6502_interpreter/6502_disassembler.o \

LIBRARIES =  \
	-L../c_compiler -lutils\
	-L../c_compiler/ELF -lelf \
	-L../c_compiler/Loader -lloader 

RPATH = \
        -rpath \$$ORIGIN/../c_compiler:\$$ORIGIN/../c_compiler/ELF:\$$ORIGIN/../c_compiler/AR:\$$ORIGIN/../c_compiler/Loader:\$$ORIGIN/../Linker


6502dasm: $(OBJS)
	$(CC) -o $@  $(OBJS) $(LIBRARIES) $(RPATH)

clean:
	rm $(OBJS) 6502dasm

