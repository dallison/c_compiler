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
  String filename;
  StringInit(&filename, argv[1]);

  Interpreter interpreter;
  Loader loader;
  
  InterpreterInit(&interpreter);
  
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
  
  // Initialize a PCode architecture.
  LoaderArchitecture arch;
  PCodeLoaderArchitectureInit(&arch);
  
  // Initialize the loader from the given exe file.
  bool ok = LoaderInitFromFile(&loader, &filename, lazy,
                               &arch,
                               &interpreter.symbol_resolver_code);
  if (!ok) {
    printf("Error Loading %s\n", filename.value);
    exit(1);
  }

  if (print_libraries_only) {
    exit(0);
  }
  // Run the code at its entry address.
  InterpreterRun(&interpreter, &loader, loader.main_address, argc, argv);

  InterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
}
