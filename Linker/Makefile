LIB = liblinker.so
default: $(LIB)

CC = davecc
CFLAGS = -target risc-v -fPIC -O2 \
	-I/Users/dallison/Google\ Drive/c_compiler/Linker \
	-I/Users/dallison/Google\ Drive/c_compiler/Linker/Architecture \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/Loader \
	-I/Users/dallison/Google\ Drive/c_compiler/libc/include \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/ELF \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/6502 \
	-I/Users/dallison/Google\ Drive/c_compiler/c_compiler/AR \
    -Xkeep-asm

ARCH_OBJS = \
	Architecture/linker_arch_riscv.o \
	Architecture/linker_arch_pcode.o \
	Architecture/linker_arch_6502.o \

LINKER_OBJS = \
	linker.o \
	linker_main.o \
	linker_file.o \
	linker_symbols.o \
	linker_reloc.o \
	linker_dynamic.o \
	$(ARCH_OBJS)

LDFLAGS = -L/Users/dallison/Google\ Drive/c_compiler/libc -lc \
	-rpath /Users/dallison/Google\ Drive/c_compiler/libc

$(LIB): $(LINKER_OBJS)
	$(CC) -o $@  -shared $(LINKER_OBJS) $(LDFLAGS)

clean:
	rm $(LINKER_OBJS) $(LIB)
