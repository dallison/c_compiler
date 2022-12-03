CC = clang

CFLAGS = -I../c_compiler \
	-I../c_compiler/6502 \
	-I../6502_interpreter \
	-I../c_compiler/Loader \
	-I../c_compiler/ELF \
	-I../c_compiler/Loader/Architecture

OBJS = \
  main.o 

LIBRARIES =  \
	-L../c_compiler -lutils\
	-L../c_compiler/ELF -lelf \
	-L../c_compiler/Loader -lloader \
	-L../6502_interpreter -l6502 

6502dasm: $(OBJS)
	$(CC) -o $@  $(OBJS) $(LIBRARIES)

clean:
	rm $(OBJS) 6502dasm

