CFLAGS = -I.. -fPIC
LDFLAGS = -L.. -lutils

OBJS = ar.o

LIB = libar.so

$(LIB) : $(OBJS)
	$(CC) -o $(LIB) $(OBJS) $(LDFLAGS) -shared

clean:
	$(RM) -f $(OBJS) $(LIB)


