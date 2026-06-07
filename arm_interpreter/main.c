//
//  main.c
//  arm_interpreter
//
//  Created by David Allison on 4/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stdio.h>
#include "loader.h"
#include "loader_arch_arm.h"
#include "arm_interpreter.h"
#include "arm_debugger.h"
#include <stdlib.h>

extern bool print_libraries_only;

int main(int argc, char * argv[]) {
  String filename = {0};
  bool trace_regs = false;
  bool trace_instructions = false;
  bool enter_debugger = false;
  // Index in argv of the guest program's name.  Everything from this index
  // onward (the program plus any of its own arguments) becomes the guest's
  // argc/argv.
  int program_index = -1;
  for (int i = 1; i < argc; i++) {
    if (program_index < 0 && argv[i][0] == '-') {
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
        case 'i':
          // The ARM backend only has a software interpreter; accept -i
          // (and --interpret) for parity with the other interpreters.
          break;
        default:
          fprintf(stderr, "unsupported flag -%d\n", argv[i][1]);
          exit(2);
      }
    } else {
      // First non-flag argument is the program; remaining arguments belong to
      // the guest program, not the interpreter.
      if (program_index < 0) {
        program_index = i;
        StringSet(&filename, argv[i]);
      }
    }
  }

  // The guest program sees argv starting at its own name.
  int program_argc = program_index < 0 ? 0 : argc - program_index;
  char** program_argv = program_index < 0 ? NULL : &argv[program_index];
  
  ARMInterpreter interpreter;
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
  ARMLoaderArchitectureInit(&arch);
  
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
  ARMInterpreterInit(&interpreter, &loader,
                       loader.main_address, program_argc, program_argv,
                       trace_regs, trace_instructions);
  if (enter_debugger) {
    ARMDebugger debugger;
    ARMDebuggerInit(&debugger, &interpreter, loader.main_address);
    ARMDebuggerRun(&debugger);
  } else {
    // Run the code at its entry address.
    ARMInterpreterCycle(&interpreter);
  }
  
  ARMInterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
}
