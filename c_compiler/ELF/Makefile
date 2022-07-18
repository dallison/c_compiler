CFLAGS = -I.. -fPIC
LDFLAGS = -L.. -lutils

OBJS = elf_reader.o elf_writer.o

LIB = libcelf.so

$(LIB) : $(OBJS)
	$(CC) -o $(LIB) $(OBJS) $(LDFLAGS) -shared

clean:
	$(RM) -f $(OBJS) $(LIB)


