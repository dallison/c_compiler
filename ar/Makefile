CFLAGS = -I../c_compiler -I../c_compiler/AR
LDFLAGS = -L../c_compiler -lutils -L../c_compiler/AR -lar -Wl,-rpath ../c_compiler -Wl,-rpath ../c_compiler/AR
OBJS = main.o

EXE = archivist

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean:
	$(RM) -f $(OBJS) $(EXE)

