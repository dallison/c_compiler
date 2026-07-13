//
//  main.c
//  6502_interpreter
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <stdio.h>
#include "loader.h"
#include "loader_arch_6502.h"
#include "6502_interpreter.h"
#include <stdlib.h>
#include <string.h>

extern bool print_libraries_only;

static void Usage() {
  fprintf(stderr, "usage: 6502 [-d] [-x outfile] filename\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  const char* file = NULL;
  int program_arg_offset = 1;
  bool disassemble_only = false;
  bool extract = false;
  const char* extract_filename = NULL;
  bool debug = false;
  bool cycle_accurate = false;
  bool trace = false;
  const char* rom_filename = NULL;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "-debug") == 0) {
        debug = true;
      } else if (strcmp(argv[i], "-trace") == 0) {
          trace = true;
      } else if (strcmp(argv[i], "-cycle") == 0) {
        cycle_accurate = true;
      } else if (argv[i][1] == 'd') {
        disassemble_only = true;
      } else if (argv[i][1] == 'x') {
          extract = true;
        if (i == argc - 1) {
          Usage();
        }
        extract_filename = argv[++i];
      } else if (strcmp(argv[i], "-rom") == 0) {
        if (i == argc - 1) {
          Usage();
        }
        rom_filename = argv[++i];
      } else {
        Usage();
      }
    } else {
      if (file == NULL) {
        file = argv[i];
        program_arg_offset = i+1;
      }
    }
  }
  if (file == NULL) {
    fprintf(stderr, "usage: need a file to execute\n");
    exit(1);
  }
  String filename;
  StringInit(&filename, file);
  
  W65C02Interpreter interpreter;
  Loader loader;
  
  if (rom_filename == NULL) {
    fprintf(stderr, "No ROM filename provided, please pass -rom <introm.exe>\n");
    exit(1);
  }
  W65C02InterpreterInit(&interpreter, debug, cycle_accurate, trace, rom_filename);
  
  // The LD_TRACE_LOADED_OBJECTS variable shows the loaded objects
  // and doesn't run the program.
  char* ld_trace = getenv("LD_TRACE_LOADED_OBJECTS");
  if (ld_trace != NULL && ld_trace[0] != '\0') {
    print_libraries_only = true;
  }
  
  // Initialize a 6502 architecture.
  LoaderArchitecture arch;
  W65C02LoaderArchitectureInit(&arch);
  
  // Initialize the loader from the given exe file.
  bool ok = LoaderInitFromFile(&loader, &filename, 0,
                               &arch,
                               NULL,
                               ".");
  if (!ok) {
    printf("Error Loading %s\n", filename.value);
    exit(1);
  }
  
  if (print_libraries_only) {
    exit(0);
  }
  
 
  int result = 0;
  if (disassemble_only) {
    W65C02InterpreterDisassemble(&interpreter, &loader);
  } else {
    // The 6502 interpreter runs the guest inside a 64K memory array at the
    // ELF linked addresses, so the entry point must be the guest-linked entry
    // (e_entry).  The shared loader rewrites loader.main_address to a host
    // runtime pointer for ignore_vaddr architectures, which is meaningless for
    // the in-array 6502, so use the original ELF entry directly.
    uint64_t entry = loader.elf_file != NULL ? loader.elf_file->header->entry
                                             : loader.main_address;
    result = W65C02InterpreterRun(&interpreter, &loader, entry, argc, argv,
                                  program_arg_offset);
  }
  
  if (extract) {
    FILE* fp = fopen(extract_filename, "w");
    W65C02InterpreterExtract(&interpreter, &loader, fp);
    fclose(fp);
    printf("Extracted to %s\n", extract_filename);
  }
  W65C02InterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
  return result;
}

