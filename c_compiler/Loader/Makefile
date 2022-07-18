CFLAGS = -I.. -I../ELF -I$(PWD) -I$(PWD)/Architecture -fPIC
LDFLAGS = -L.. -lutils

OBJS = loader.o loader_dynamic.o \
	Architecture/loader_arch_6502.o \
	Architecture/loader_arch_pcode.o \
	Architecture/loader_arch_riscv.o

LIB = libloader.so

$(LIB) : $(OBJS)
	$(CC) -o $(LIB) $(OBJS) $(LDFLAGS) -shared

clean:
	$(RM) -f $(OBJS) $(LIB)


