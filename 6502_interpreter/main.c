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
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "-debug") == 0) {
        debug = true;
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
  
  _6502Interpreter interpreter;
  Loader loader;
  
  _6502InterpreterInit(&interpreter, debug, cycle_accurate);
  
  // The environment variable LD_BIND_NOW tells the dynamic loader to
  // replace the GOT entries for functions with the function address
  // at load time rather than delaying the resolution to the first
  // call.  It needs to be set to a non-empty string.
  char* bind_now = getenv("LD_BIND_NOW");
  bool lazy = bind_now == NULL || bind_now[0] == '\0';
  
  // The LD_TRACE_LOADED_OBJECTS variable shows the loaded objects
  // and doesn't run the program.
  char* ld_trace = getenv("LD_TRACE_LOADED_OBJECTS");
  if (ld_trace != NULL && ld_trace[0] != '\0') {
    print_libraries_only = true;
  }
  
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
  
  if (print_libraries_only) {
    exit(0);
  }
  
 
  if (disassemble_only) {
    _6502InterpreterDisassemble(&interpreter, &loader);
  } else {
    // Run the code at its entry address.
    _6502InterpreterRun(&interpreter, &loader, loader.main_address, argc, argv);
  }
  
  if (extract) {
    FILE* fp = fopen(extract_filename, "w");
    _6502InterpreterExtract(&interpreter, &loader, fp);
    fclose(fp);
    printf("Extracted to %s\n", extract_filename);
  }
  _6502InterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
}

