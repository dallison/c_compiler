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
#include "risc_v_process.h"
#include <stdlib.h>

extern bool print_libraries_only;

int main(int argc, char * argv[]) {
  String filename = {0};
  bool trace_regs = false;
  bool trace_instructions = false;
  bool enter_debugger = false;
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
        default:
          fprintf(stderr, "unsupported flag -%d\n", argv[i][1]);
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
    fprintf(stderr, "usage: %s [-d] [-g] [-r] program [args...]\n", argv[0]);
    exit(2);
  }
  int program_argc = argc - program_index;
  char** program_argv = &argv[program_index];
  
  RISCVInterpreter interpreter;
  Loader loader;
  
  int32_t loader_flags = trace_instructions ? LOADER_MAP_SYMTAB : 0;

  // Lazy PLT binding leaves cross-DSO calls unresolved on the RISC-V
  // interpreter.  Bind eagerly, matching x86_64 and AArch64.
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
                       loader.main_address, program_argc, program_argv,
                       trace_regs, trace_instructions);
  RISCVProcessRuntime process;
  if (!RISCVProcessRuntimeInit(&process, &loader)) {
    RISCVInterpreterDestruct(&interpreter);
    LoaderDestruct(&loader);
    exit(1);
  }
  if (RISCVProcessAttachMainThread(&process, &interpreter) == NULL) {
    RISCVProcessRuntimeDestruct(&process);
    RISCVInterpreterDestruct(&interpreter);
    LoaderDestruct(&loader);
    exit(1);
  }
  if (!RISCVGuestRunInitArrays(&loader, &interpreter)) {
    RISCVProcessRuntimeDestruct(&process);
    RISCVInterpreterDestruct(&interpreter);
    LoaderDestruct(&loader);
    exit(1);
  }
  RISCVInterpreterPrepareMain(&interpreter, loader.main_address, program_argc,
                              program_argv);
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
  
  RISCVProcessRuntimeDestruct(&process);
  RISCVInterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
  return result;
}
