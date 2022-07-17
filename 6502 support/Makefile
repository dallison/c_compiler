CC = davecc
AS = davecc
ASFLAGS = -target 6502 -c
CFLAGS = -target 6502 
LDFLAGS = -Wl,-trom -target 6503 -static 

EXE = 6502rom.exe
BIN = 6502rom.bin
DIS = 6502rom.dis

OBJS = bootrom/6502rom.o acia.o comms.o console.o device.o hexdump.o server.o utils.o

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(EXE)
	6502 -d -x $(BIN) $(EXE) >$(DIS)
	dd if=/dev/zero of=/tmp/zeroes count=16384 bs=1
	cat 6502rom.bin /tmp/zeroes >6502rom.bin.tmp
	mv 6502rom.bin.tmp 6502rom.bin
	#scp 6502rom.bin  pi@192.168.86.52:

clean:
	rm -f  $(OBJS) $(BIN) $(EXE) $(DIS)
