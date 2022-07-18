CFLAGS = -I../c_compiler -I../c_compiler/AR -I../c_compiler/ELF -I../c_compiler/6502 -I../Linker
LDFLAGS = -L../c_compiler -lutils -lcompiler -L../Linker -llink -L../c_compiler/Loader -lloader -L../c_compiler/AR -L../c_compiler/ELF -lcelf -lar \
	  -Wl,-rpath $(PWD)/../c_compiler -Wl,-rpath $(PWD)/../c_compiler/AR \
	  -Wl,-rpath $(PWD)/../c_compiler/ELF \
	  -Wl,-rpath $(PWD)/../Linker -Wl,-rpath $(PWD)/../c_compiler/Loader
OBJS = main.o

EXE = davecc

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean:
	$(RM) -f $(OBJS) $(EXE)

