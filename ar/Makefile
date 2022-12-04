CFLAGS = -I../c_compiler \
	-I../c_compiler/AR \
	$(OPT_DEBUG)

LDFLAGS = -L../c_compiler -lutils \
	-L../c_compiler/AR -lar 

OBJS = main.o

EXE = archivist

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

clean:
	$(RM) -f $(OBJS) $(EXE)

