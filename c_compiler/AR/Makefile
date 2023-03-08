
CC = clang
CFLAGS =  \
	-I.. \
	-I../ELF \
	-I../6502 \
	$(OPT_DEBUG) $(ADD_FLAGS)

AR_OBJS = \
	ar.o

LIB = libar.a

$(LIB): $(AR_OBJS)
	$(AR) -r $@ $(AR_OBJS) 

clean:
	rm -f $(AR_OBJS) $(LIB)

