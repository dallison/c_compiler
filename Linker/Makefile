LIB = liblinker.a
default: $(LIB)

CC = clang
CFLAGS = \
    -I. \
	-IArchitecture \
	-I../c_compiler \
	-I../c_compiler/Loader \
	-I../c_compiler/ELF \
	-I../c_compiler/6502 \
	-I../c_compiler/AR \

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
	linker_config.o \
	$(ARCH_OBJS)

$(LIB): $(LINKER_OBJS)
	$(AR) -r $@  $(LINKER_OBJS)

clean:
	rm $(LINKER_OBJS) $(LIB)
