//
//  main.c
//  risc_v_interpreter
//
//  Created by David Allison on 4/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stdio.h>
#include "loader.h"
#include "loader_arch_riscv.h"
#include "risc_v_interpreter.h"
#include "risc_v_debugger.h"
#include <stdlib.h>

extern bool print_libraries_only;

int main(int argc, char * argv[]) {
  String filename = {0};
  bool trace_regs = false;
  bool trace_instructions = false;
  bool enter_debugger = false;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
        case 'd':
          trace_instructions = true;
          break;
        case 'g':
          enter_debugger = true;
          break;
        case 'r':
          trace_regs = true;
          break;
        default:
          fprintf(stderr, "unsupported flag -%d\n", argv[i][1]);
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
  
  RISCVInterpreter interpreter;
  Loader loader;
  
  int32_t loader_flags = trace_instructions ? LOADER_MAP_SYMTAB : 0;

  // The environment variable LD_BIND_NOW tells the dynamic loader to
  // replace the GOT entries for functions with the function address
  // at load time rather than delaying the resolution to the first
  // call.  It needs to be set to a non-empty string.
  char* bind_now = getenv("LD_BIND_NOW");
  if (bind_now == NULL || bind_now[0] == '\0') {
    loader_flags |= LOADER_LAZY_RESOLVE;
  }
  // The LD_TRACE_LOADED_OBJECTS variable shows the loaded objects
  // and doesn't run the program.
  char* ld_trace = getenv("LD_TRACE_LOADED_OBJECTS");
  if (ld_trace != NULL && ld_trace[0] != '\0') {
    print_libraries_only = true;
  }
  
  if (enter_debugger) {
    loader_flags |= LOADER_WRITEABLE_TEXT;
  }
  
  // Initialize a RISC-V architecture.
  LoaderArchitecture arch;
  RISCVLoaderArchitectureInit(&arch);
  
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
  RISCVInterpreterInit(&interpreter, &loader,
                       loader.main_address, argc, argv,
                       trace_regs, trace_instructions);
  if (!RISCVGuestRunInitArrays(&loader, &interpreter)) {
    RISCVInterpreterDestruct(&interpreter);
    LoaderDestruct(&loader);
    exit(1);
  }
  int result;
  if (enter_debugger) {
    RISCVDebugger debugger;
    RISCVDebuggerInit(&debugger, &interpreter, loader.main_address);
    RISCVDebuggerRun(&debugger);
    result = interpreter.exit_code;
  } else {
    result = RISCVInterpreterRun(&interpreter);
  }
  if (!RISCVGuestRunProgramShutdown(&loader, &interpreter)) {
    result = 1;
  }
  
  RISCVInterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
  return result;
}
