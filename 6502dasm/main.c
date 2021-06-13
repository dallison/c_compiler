//
//  main.c
//  6502dasm
//
//  Created by David Allison on 10/6/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "6502_disassembler.h"
#include "loader.h"
#include "loader_arch_6502.h"

static void usage() {
  fprintf(stderr, "usage: 6502dasm filename [addr [length]]\n");
  exit(1);
}

int main(int argc, const char * argv[]) {
  if (argc < 2) {
    usage();
  }
  
  String filename;
  StringInit(&filename, argv[1]);
  int start_addr_range = 0;
  int length = 64*1024;
  for (int i = 2; i < argc; i++) {
    if (start_addr_range == 0) {
      start_addr_range = (int)strtoll(argv[i], NULL, 0);
    } else if (length == 64*1024) {
      length = atoi(argv[i]);
    } else {
      usage();
    }
  }
  int end_addr_range = start_addr_range + length - 1;
  
  Loader loader;
  
  // Initialize a 6502 architecture.
  LoaderArchitecture arch;
  _6502LoaderArchitectureInit(&arch);
  
  // Initialize the loader from the given exe file.
  bool ok = LoaderInitFromFile(&loader, &filename, 0,
                               &arch,
                               NULL,
                               ".");
  if (!ok) {
    printf("Error Loading %s\n", filename.value);
    exit(1);
  }
  
  for (size_t i = 1; i < loader.regions.length; i++) {
    Region* region = loader.regions.value.p[i];
    for (size_t section_index = 0; section_index < region->sections.length; section_index++) {
      ELFReaderSection* section = region->sections.value.p[section_index];
      if ((section->header->flags & SHF(execinstr)) != 0) {
        void* addr = (void*)((char*)region->address + section->header->offset);
        void* end_addr = addr + section->header->size;
        uint16_t pc = section->header->addr;
        if (pc > end_addr_range) {
          continue;
        }
        while (addr <= end_addr) {
          if (pc >= start_addr_range && pc <= end_addr_range) {
            void* new_addr = Disassemble6502Instruction(pc, addr, stdout);
            pc += new_addr - addr;
            addr = new_addr;
          } else {
            pc++;
            addr++;
          }
        }
      }
    }
  }
}
