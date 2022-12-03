LIB = libloader_linux.a
default: $(LIB)

CC = clang
CFLAGS = -I. \
	-IArchitecture \
	-I.. \
	-I../ELF \
	-I../6502 \

ARCH_OBJS = \
	Architecture/loader_arch_riscv.o \
	Architecture/loader_arch_pcode.o \
	Architecture/loader_arch_6502.o \

LINKER_OBJS = \
	loader.o \
	loader_dynamic.o \
	$(ARCH_OBJS)

$(LIB): $(LINKER_OBJS)
	$(AR) -r $@  $(LINKER_OBJS)

clean:
	rm -f $(LINKER_OBJS) $(LIB)

