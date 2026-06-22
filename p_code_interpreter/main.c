//
//  main.c
//  p_code_interpreter
//
//  Created by David Allison on 1/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stdio.h>
#include "loader.h"
#include "loader_arch_pcode.h"
#include "p_code_interpreter.h"
#include <stdlib.h>

extern bool print_libraries_only;

int main(int argc, char *argv[]) {
  String filename = {0};
  bool trace_instructions = false;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
        case 'd':
          trace_instructions = true;
          break;
        default:
          fprintf(stderr, "unsupported flag -%c\n", argv[i][1]);
          exit(2);
      }
    } else {
      if (filename.length != 0) {
        fprintf(stderr, "Only one file to interpret please\n");
        exit(1);
      }
      StringSet(&filename, argv[i]);
    }
  }

  if (filename.length == 0) {
    fprintf(stderr, "usage: %s [-d] <program>\n", argv[0]);
    exit(2);
  }

  PCodeInterpreter interpreter;
  Loader loader;
  
  PCodeInterpreterInit(&interpreter);
  PCodeInterpreterSetDisassemble(trace_instructions);
  
  // The environment variable LD_BIND_NOW tells the dynamic loader to
  // replace the GOT entries for functions with the function address
  // at load time rather than delaying the resolution to the first
  // call.  It needs to be set to a non-empty string.
  char* bind_now = getenv("LD_BIND_NOW");
  int32_t loader_flags = 0;
  if (bind_now == NULL || bind_now[0] == '\0') {
    loader_flags |= LOADER_LAZY_RESOLVE;
  }
  
  // The LD_TRACE_LOADED_OBJECTS variable shows the loaded objects
  // and doesn't run the program.
  char* ld_trace = getenv("LD_TRACE_LOADED_OBJECTS");
  if (ld_trace != NULL && ld_trace[0] != '\0') {
    print_libraries_only = true;
  }
  
  // Initialize a PCode architecture.
  LoaderArchitecture arch;
  PCodeLoaderArchitectureInit(&arch);
  
  // Initialize the loader from the given exe file.
  bool ok = LoaderInitFromFile(&loader, &filename, loader_flags,
                               &arch,
                               &interpreter.symbol_resolver_code,
                               ".");
  if (!ok) {
    printf("Error Loading %s\n", filename.value);
    exit(1);
  }

  if (print_libraries_only) {
    exit(0);
  }
  // Run the code at its entry address.
  PCodeInterpreterRun(&interpreter, &loader, loader.main_address, argc, argv);

  PCodeInterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
}
