// EEPROM programmer for Raspberry Pi
// David Allison
// July 2019

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPi.h>

#define DEFAULT_EEPROM_SIZE 16*1024

#define RCLK 12         // Register clock (put value in register)
#define SRCLK 16        // Shift one bit into register
#define SER 20          // Bit 0 to shift in
#define SRCLR 21        // Clear register.

// Data values.
#define DATA0 25
#define DATA1 24
#define DATA2 23
#define DATA3 18
#define DATA4 26
#define DATA5 19
#define DATA6 13
#define DATA7 6

#define WE 5            // Write enable (active low).
#define OE 4            // Output enable (active low).

// EEPROM page write mode is 64 bytes at a time.
#define PAGE_SIZE 64

// Bit to GPIO pin for data bus.
static int data_pins[8] = {
  DATA0, DATA1, DATA2, DATA3, DATA4, DATA5, DATA6, DATA7,
};

void InitPins() {
  pinMode(RCLK, OUTPUT);
  pullUpDnControl(RCLK, PUD_DOWN);

  pinMode(SRCLK, OUTPUT);
  pullUpDnControl(SRCLK, PUD_DOWN);

  pinMode(SER, OUTPUT);
  pullUpDnControl(SER, PUD_DOWN);

  pinMode(SRCLR, OUTPUT);
  pullUpDnControl(SRCLR, PUD_UP);

  // Write Enable is active low so pull high
  pinMode(WE, OUTPUT);
  pullUpDnControl(WE, PUD_UP);

  // Output Enable is active low so pull high
  pinMode(OE, OUTPUT);
  pullUpDnControl(OE, PUD_UP);
}

void SetWriteMode() {
  for (int i = 0; i < 8; i++) {
    pinMode(data_pins[i], OUTPUT);
    pullUpDnControl(data_pins[i], PUD_DOWN);
  }
}

void SetReadMode() {
  for (int i = 0; i < 8; i++) {
    pinMode(data_pins[i], INPUT);
    pullUpDnControl(data_pins[i], PUD_DOWN);
  }
}

void Reset() {
  digitalWrite(SRCLR, 0);
  digitalWrite(SRCLR, 1);

  digitalWrite(WE, 1);
  digitalWrite(OE, 1);
}

void SetAddress(int addr) {
  Reset();
  digitalWrite(RCLK, 0);                  // RCLK inactive.
  digitalWrite(SRCLK, 0);                 // SRCLK inactive.

  // Shift register shifts left so start at MSB.
  for (int i = 15; i >= 0; i--) {
    digitalWrite(SER, (addr >> i) & 1);
    digitalWrite(SRCLK, 1);                 // Pulse SRCLK active.
    digitalWrite(SRCLK, 0);                 // SRCLK inactive.
  }
  digitalWrite(RCLK, 1);                 // Pulse RCLK active.
  digitalWrite(RCLK, 0);                 // RCLK inactive.
}

void SetData(int v) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(data_pins[i], (v >> i) & 1);
  }
}

int ReadData() {
  int byte = 0;
  for (int i = 0; i < 8; i++) {
    int bit = digitalRead(data_pins[i]);
    byte |= bit << i;
  }
  return byte;
}

// SetWriteMode must have been called.
void WriteByte(int address, int data) {
  SetAddress(address);
  SetData(data);

  // Pulse write enable low.
  digitalWrite(WE, 0);
  digitalWrite(WE, 1);
}

// SetReadMode must have been called.
int ReadByte(int address) {
  SetAddress(address);

  // Pulse Output enable Low.
  digitalWrite(OE, 0);
  int v = ReadData();
  digitalWrite(OE, 1);
  return v;
}

bool VerifyPage(int address, unsigned char* data, size_t size) {
  SetReadMode();
  for (size_t i = 0; i < size; i++) {
    int addr = address + i;
    unsigned char v = ReadByte(addr);
    if (v != data[i]) {
      return false;
    }
  }
  return true;
}

void Poll(int address, int expected_data) {
  // Read IO7.
  pinMode(data_pins[7], INPUT);

  // The max write cycle time is 10ms.  Each loop will
  // delay 100us, so the max loop count is 10000/100 = 100.
  // However that doesn't seem to be long enough so we multiply
  // it by 10.
  for (int i = 0; i < 1000; i++) {
    // OE low.
    digitalWrite(OE, 0);
    int bit = digitalRead(data_pins[7]);
    digitalWrite(OE, 1);
    if (bit == expected_data) {
       return;
    }
    usleep(10000);
  }
  fprintf(stderr, "\nWrite failed\n");
  abort();
}

void WritePage(int address, unsigned char* data, size_t size) {
  // Size is PAGE_SIZE max.
  // Page bytes are written with a max of 150us between
  // them.
  SetWriteMode();
  for (int i = 0; i < size; i++) {
    int addr = address + i;
    SetAddress(addr);

    // Write pulse.
    digitalWrite(WE, 0);
    SetData(data[i]);
    digitalWrite(WE, 1);
  }

  // Now poll for the correct value on I/O7.
  // Read the last address and expect I/O7 to be
  // the value of the top bit of the last byte
  // written.
  int last_addr = address + size - 1;
  int expected_bit = data[size-1] >> 7;
  Poll(last_addr, expected_bit);
}


void ReadEEPROM(const char* filename, int size) {
  printf("Reading to %s\n", filename);
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open file %s\n", filename);
    exit(1);
  }
  SetReadMode();
  for (int addr = 0; addr < size; addr++) {
    printf("Reading 0x%04x: ", addr);
    int byte = ReadByte(addr);
    fputc(byte, fp);
    printf("%02x\r", byte);
    fflush(stdout);
  }
  printf("\n0x%x bytes read\n", size);
  fclose(fp);
}

void WriteEEPROM(const char* filename) {
  printf("Writing from %s\n", filename);
  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open file %s\n", filename);
    exit(1);
  }
  unsigned char buffer[PAGE_SIZE];
  int addr = 0;
  for (;;) {
    size_t len = fread(buffer, 1, sizeof(buffer), fp);
    if (len == 0) {
      break;
    }
    bool verified = false;
    for (int i = 0; i < 100; i++) {
      printf("Writing 0x%04x\r", addr);
      fflush(stdout);
      WritePage(addr, buffer, sizeof(buffer));
      if (VerifyPage(addr, buffer, sizeof(buffer))) {
         verified = true;
         break;
      }
    }
    if (!verified) {
      fprintf(stderr, "Failed to write page %04x\n", addr);
      abort();
    }
    addr += len;
  }
  printf("\n0x%x bytes written\n", addr);
  fclose(fp);
}

void EraseEEPROM(int size) {
  unsigned char buffer[PAGE_SIZE];
  memset(buffer, 0, sizeof(buffer));
  for (int addr = 0; addr < size;) {
    bool verified = false;
    for (int i = 0; i < 100; i++) {
      printf("Erasing 0x%04x\r", addr);
      fflush(stdout);
      WritePage(addr, buffer, sizeof(buffer));
      if (VerifyPage(addr, buffer, sizeof(buffer))) {
         verified = true;
         break;
      }
    }
    if (!verified) {
      fprintf(stderr, "Failed to write page %04x\n", addr);
      abort();
    }
    addr += sizeof(buffer);
  }
  printf("\nErased\n");
}

void TestProgrammer(int size) {
  for (int i = 0; i < size; i++) {
    SetAddress(i);
    usleep(10000);
  }
}

void Usage() {
  fprintf(stderr, "usage: eeprom [-r] [-e] [-r] [-w] [-B addr value] [-b addr] [-s K] [filename]\n");
  exit(1);
}

int main(int argc, char** argv) {
  enum Action {
    kErase,
    kRead,
    kWrite,
    kWriteByte,
    kReadByte,
    kTest,
  };

  enum Action action;
  const char* filename = NULL;
  int addr, value;
  int size = DEFAULT_EEPROM_SIZE;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
        case 'e':
          action = kErase;
          break;
        case 'r':
          action = kRead;
          break;
        case 'w':
          action = kWrite;
          break;
        case 'B':
          action = kWriteByte;
          if (i >= argc-2) {
            Usage();
          }
          addr = strtoll(argv[i+1], NULL, 16);
          value = strtoll(argv[i+2], NULL, 16);
          i += 2;
          break;
        case 'b':
          action = kReadByte;
          if (i >= argc-1) {
            Usage();
          }
          addr = strtoll(argv[i+1], NULL, 16);
          i++;
          break;
        case 's':
          if (i >= argc-1) {
            Usage();
          }
          size = strtoll(argv[i+1], NULL, 10);
          size *= 1024;     // Convert to K.
          i++;
          break;
        case 't':
          action = kTest;
          break;
        default:
          Usage();
          break;
      }
    } else {
      if (filename == NULL) {
        filename = argv[i];
      } else {
        Usage();
      }
    }
  }
  if (filename == NULL) {
    filename = "eeprom.bin";
  }
  wiringPiSetupGpio();            // Use GPIO numbering
  InitPins();

  switch (action) {
    case kErase:
       EraseEEPROM(size);
       break;
    case kRead:
       ReadEEPROM(filename, size);
       break;
    case kWrite:
       WriteEEPROM(filename);
       break;
    case kWriteByte:
       SetWriteMode();
       WriteByte(addr, value);
       break;
    case kReadByte:
       SetReadMode();
       printf("%04x  %02x\n", addr, ReadByte(addr));
       break;
    case kTest:
       TestProgrammer(size);
       break;
  }
}

