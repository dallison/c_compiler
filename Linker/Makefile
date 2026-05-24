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
	-I../c_compiler/aarch64 \
	-I../c_compiler/arm \
	-I../c_compiler/p_code \
	-I../c_compiler/risc_v \
	-I../c_compiler/x86_64 \
	-I../c_compiler/AR \
	$(OPT_DEBUG) $(ADD_FLAGS)

ARCH_OBJS = \
	Architecture/linker_arch_riscv.o \
	Architecture/linker_arch_pcode.o \
	Architecture/linker_arch_6502.o \
	Architecture/linker_arch_aarch64.o \
	Architecture/linker_arch_arm.o \
	Architecture/linker_arch_x86_64.o \

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
	rm -f $(LINKER_OBJS) $(LIB)
