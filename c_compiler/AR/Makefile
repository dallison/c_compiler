
CC = davecc
CFLAGS = -target risc-v -fPIC \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler \
	-I/Users/dallison/Google\ Drive/c_compiler/libc/include \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/ELF \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/6502 \
    -Xkeep-asm

AR_OBJS = \
	ar.o

LDFLAGS = -L/Users/dallison/Google\ Drive/c_compiler/libc -lc \
	-rpath /Users/dallison/Google\ Drive/c_compiler/libc

libar.so: $(AR_OBJS)
	$(CC) -o $@  -shared $(AR_OBJS) $(LDFLAGS)

clean:
	rm $(AR_OBJS) libar.so

