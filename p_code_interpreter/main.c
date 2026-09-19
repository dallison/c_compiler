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
  int program_index = -1;
  for (int i = 1; i < argc; i++) {
    if (program_index < 0 && argv[i][0] == '-') {
      switch (argv[i][1]) {
        case 'd':
          trace_instructions = true;
          break;
        default:
          fprintf(stderr, "unsupported flag -%c\n", argv[i][1]);
          exit(2);
      }
    } else {
      if (program_index < 0) {
        program_index = i;
        StringSet(&filename, argv[i]);
      }
    }
  }

  if (program_index < 0) {
    fprintf(stderr, "usage: %s [-d] program [args...]\n", argv[0]);
    exit(2);
  }
  int program_argc = argc - program_index;
  char** program_argv = &argv[program_index];

  PCodeInterpreter interpreter;
  Loader loader;
  
  PCodeInterpreterInit(&interpreter);
  PCodeInterpreterSetDisassemble(trace_instructions);
  
  // Lazy PLT binding leaves cross-DSO calls unresolved.  Bind eagerly,
  // matching x86_64 and AArch64.
  int32_t loader_flags = 0;
  
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

  PCodeInterpreterInit(&interpreter);
  PCodeInterpreterSetDisassemble(trace_instructions);
  interpreter.stack = malloc(P_CODE_STACK_SIZE);
  if (interpreter.stack == NULL) {
    LoaderDestruct(&loader);
    exit(1);
  }
  interpreter.iregs[PCODE_SP_REG] =
      (int64_t)(interpreter.stack + P_CODE_STACK_SIZE);
  interpreter.loader = &loader;
  if (!PCodeGuestRunInitArrays(&loader, &interpreter)) {
    PCodeInterpreterDestruct(&interpreter);
    LoaderDestruct(&loader);
    exit(1);
  }

  int result = PCodeInterpreterRun(&interpreter, &loader, loader.main_address,
                                   program_argc, program_argv);
  if (!PCodeGuestRunProgramShutdown(&loader, &interpreter)) {
    result = 1;
  }

  PCodeInterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
  return result;
}
