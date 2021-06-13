CC = davecc
AS = davecc
ASFLAGS = -target 6502 -c
CFLAGS = -target 6502

EXE = kernel

OBJS = main.o

$(EXE): $(OBJS)
	$(CC) -origin 0x1000 -static $(OBJS) -o $(EXE)
	scp $(EXE) pi@192.168.86.52:6502root

clean:
	rm -f  $(OBJS) $(EXE)
