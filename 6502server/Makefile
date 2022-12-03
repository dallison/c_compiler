CFLAGS = -std=c99
OBJS = main.o \
    file_loader.o\
    fs.o\
    comms.o\
    packet.o\
    dstring.o \
    vector.o \
    hashtable.o \
    map.o \
    list.o \
    binary_tree.o \
        set.o \
        bitset.o \
    buffer.o \
	elf_reader.o \
	elf_writer.o \
	loader.o \
	loader_dynamic.o \
	loader_arch_6502.o \


EXE = 6502server

$(EXE): $(OBJS)
	$(CC) -o $(EXE) $(OBJS)

