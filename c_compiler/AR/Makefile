
CC = clang
CFLAGS =  \
	-I.. \
	-I../ELF \
	-I../6502 \

AR_OBJS = \
	ar.o

libar_linux.a: $(AR_OBJS)
	$(AR) -r $@ $(AR_OBJS) 

clean:
	rm -f $(AR_OBJS) libar_linux.a

