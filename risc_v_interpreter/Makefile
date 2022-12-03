CC = davecc
CFLAGS = -target risc-v -fPIC -I/Users/dallison/Google\ Drive/c_compiler/c_compiler -Xkeep-asm

OBJS = \
  main.o \
  risc_v_disassembler.o \
  risc_v_interpreter.o

LDFLAGS = -L/Users/dallison/Google\ Drive/c_compiler/libc -lc \
		-L/Users/dallison/Google\ Drive/c_compiler/c_compiler -lutils \
	-rpath /Users/dallison/Google\ Drive/c_compiler/libc:/Users/dallison/Google\ Drive/c_compiler/c_compiler

libutils.so: $(OBJS)
	$(CC) -o $@  -shared $(OBJS) $(LDFLAGS)

clean:
	rm $(OBJS) libutils.so

