CFLAGS = -I$(PWD) -I$(PWD)/Architecture -I../c_compiler -I../c_compiler/ELF -I../c_compiler/Loader -I../c_compiler/AR -I../c_compiler/6502 -fPIC
LDFLAGS = -L../c_compiler -L../c_compiler/AR -L../c_compiler/Loader -lutils -lloader -Wl,-rpath $(PWD)/../c_compiler/Loader

OBJS = \
	linker.o\
	linker_config.o\
	linker_dynamic.o\
	linker_file.o\
	linker_main.o\
	linker_reloc.o\
	linker_symbols.o \
	Architecture/linker_arch_6502.o \
	Architecture/linker_arch_pcode.o \
	Architecture/linker_arch_riscv.o

LIB = liblink.so

$(LIB) : $(OBJS)
	$(CC) -o $(LIB) $(OBJS) $(LDFLAGS) -shared

clean:
	$(RM) -f $(OBJS) $(LIB)


