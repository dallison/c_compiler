//
//  main.c
//  6502map
//
//  Created by David Allison on 9/5/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

// Multiplexed IO chip selects.  We have 8 chips we can select with the
// bottom 3 bits of the mapping ROM.
// The next 5 bits select the RAM bank.
#define ROM 0
#define RAM 1
#define ACIA1 2
#define ACIA2 3
#define VIA 4
#define REGS 5

#define VECTORS 0xfd00  // to 0xfdff

static void WriteMap(int chip, int ram_bank, int start_addr, int end_addr, FILE* fp) {
  const int kSpan = 64;
  for (int i = start_addr; i < end_addr; i += kSpan) {
    int value = (chip & 0x7) | (ram_bank << 3);
    fputc(value, fp);
  }
}

// Mapped IO.
// There are 4 devices, each occupying 64 bytes:
// ACIA 6850 #1 (serial) - 0xfe00
// ACIA 6850 #2 (serial) - 0xfe40
// 6522 VIA              - 0xfe80
// Mapped IO registers   - 0xfec0

static void WriteMappedIO(int ram_bank, FILE* fp) {
  WriteMap(ACIA1, 0, 0xfe00, 0xfe40, fp);   // ACIA1: 0xfe00..0xfe3f
  WriteMap(ACIA2, 0, 0xfe40, 0xfe80, fp);   // ACIA2: 0xfe40..0xfe7f
  WriteMap(VIA, 0, 0xfe80, 0xfec0, fp);     // VIA: 0xfe80..0xfebf
  WriteMap(REGS, 0, 0xfec0, 0xff00, fp);    // REGS: 0xfec0..0xfeff
}

static void WriteVectorArea(FILE *fp) {
  WriteMap(RAM, 0, 0xfd00, 0xfe00, fp);    // VECTORS: 0xfd00..0xfdff
}

// Kernel bank (0).
static void WriteKernelBank(FILE* fp) {
  WriteMap(RAM, 0, 0, 0xc000, fp);          // RAM: 0...0xbfff
  WriteMap(ROM, 0, 0xc000, 0xfd00, fp);     // ROM: 0xc000..0xfdff
  WriteVectorArea(fp);
  WriteMappedIO(0, fp);                     // IO: 0xfe00..0xfeff
  WriteMap(ROM, 0, 0xff00, 0xffff, fp);     // ROM: 0xff00..0xffff
}

int main(int argc, const char * argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: 6502map filename\n");
    exit(1);
  }
  FILE* fp = fopen(argv[1], "w");
  if (fp == NULL) {
    fprintf(stderr, "Unable to open %s\n", argv[1]);
    exit(1);
  }
  WriteKernelBank(fp);
  fclose(fp);
}
