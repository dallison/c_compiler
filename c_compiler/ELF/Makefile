CC = davecc
CFLAGS = -target risc-v -fPIC -I/Users/dallison/Google\ Drive/c_compiler/c_compiler -Xkeep-asm

OBJS = \
  elf_reader.o \
  elf_writer.o

LDFLAGS = -L/Users/dallison/Google\ Drive/c_compiler/libc -lc \
		-L/Users/dallison/Google\ Drive/c_compiler/c_compiler -lutils \
	-rpath /Users/dallison/Google\ Drive/c_compiler/libc:/Users/dallison/Google\ Drive/c_compiler/c_compiler

LIB = libelf.so

$(LIB): $(OBJS)
	$(CC) -o $@  -shared $(OBJS) $(LDFLAGS)

clean:
	rm $(OBJS) $(LIB)
