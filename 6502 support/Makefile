CC = davecc
AS = davecc
ASFLAGS = -target 65c02 -c
CFLAGS = -target 65c02 
LDFLAGS = -Wl,-trom -target 65c02 -static 

EXE = 6502rom.exe
BIN = 6502rom.bin
DIS = 6502rom.dis

OBJS = bootrom/6502rom.o acia.o comms.o console.o device.o hexdump.o server.o utils.o romsyscalls.o
TESTOBJS = bootrom/6502test.o
TESTEXE = 6502test.exe
TESTBIN = 6502test.bin
TESTDIS = 6502test.dis

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(EXE)
	6502 -d -x $(BIN) $(EXE) >$(DIS)
	dd if=/dev/zero of=/tmp/zeroes count=16384 bs=1
	cat $(BIN) /tmp/zeroes >$(BIN).tmp
	mv $(BIN).tmp $(BIN)
	#scp 6502rom.bin  pi@192.168.86.52:

clean:
	rm -f  $(OBJS) $(BIN) $(EXE) $(DIS)

test: $(TESTBIN)

$(TESTBIN) : $(TESTOBJS)
	$(CC) $(LDFLAGS) $(TESTOBJS) -o $(TESTEXE)
	6502 -d -x $(TESTBIN) $(TESTEXE) >$(TESTDIS)
	dd if=/dev/zero of=/tmp/zeroes count=16384 bs=1
	cat $(TESTBIN) /tmp/zeroes >$(TESTBIN).tmp
	mv $(TESTBIN).tmp $(TESTBIN)
