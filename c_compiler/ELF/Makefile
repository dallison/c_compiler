CC = clang
CFLAGS =  -I..

OBJS = \
  elf_reader.o \
  elf_writer.o

LDFLAGS = -L.. -lutils \

LIB = libelf.a

$(LIB): $(OBJS)
	$(AR) -r $@  $(OBJS)

clean:
	rm -f $(OBJS) $(LIB)

