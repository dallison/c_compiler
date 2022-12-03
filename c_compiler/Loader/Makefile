LIB = libloader.so
default: $(LIB)

CC = davecc
CFLAGS = -target risc-v -fPIC \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/Loader \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/Loader/Architecture \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler \
	-I/Users/dallison/Google\ Drive/c_compiler/libc/include \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/ELF \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/6502 \
    -Xkeep-asm

ARCH_OBJS = \
	Architecture/loader_arch_riscv.o \
	Architecture/loader_arch_pcode.o \
	Architecture/loader_arch_6502.o \

LINKER_OBJS = \
	loader.o \
	loader_dynamic.o \
	$(ARCH_OBJS)

LDFLAGS = -L/Users/dallison/Google\ Drive/c_compiler/libc -lc \
	-rpath /Users/dallison/Google\ Drive/c_compiler/libc

$(LIB): $(LINKER_OBJS)
	$(CC) -o $@  -shared $(LINKER_OBJS) $(LDFLAGS)

clean:
	rm $(LINKER_OBJS) $(LIB)
