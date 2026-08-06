//
//  main.c
//  x86_64_interpreter
//

#include "x86_64_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void Usage(const char* prog) {
  fprintf(stderr,
          "usage: %s [-i | -n] [-d] [-r] program [args...]\n"
          "  -i, --interpret   Execute with software interpreter\n"
          "  -n, --native      Execute on local CPU (default on x86_64 hosts)\n"
          "  -d                Trace instructions\n"
          "  -r                Trace register changes\n",
          prog);
  exit(2);
}

int main(int argc, char** argv) {
  const char* program = NULL;
  X86_64ExecutionMode mode = X86_64DefaultExecutionMode();
  bool mode_set = false;
  bool trace_instructions = false;
  bool trace_registers = false;
  int program_arg_offset = 0;

  for (int i = 1; i < argc; i++) {
    if (program == NULL && argv[i][0] != '-') {
      program = argv[i];
      program_arg_offset = i;
      continue;
    }
    if (program != NULL) {
      break;
    }
    if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interpret") == 0) {
      mode = kX86_64ModeInterpret;
      mode_set = true;
    } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--native") == 0) {
      mode = kX86_64ModeNative;
      mode_set = true;
    } else if (strcmp(argv[i], "-d") == 0) {
      trace_instructions = true;
    } else if (strcmp(argv[i], "-r") == 0) {
      trace_registers = true;
    } else {
      Usage(argv[0]);
    }
  }

  if (program == NULL) {
    Usage(argv[0]);
  }
  (void)mode_set;

  X86_64Runtime runtime;
  if (!X86_64RuntimeInit(&runtime, program, mode, trace_registers,
                         trace_instructions)) {
    fprintf(stderr, "Error loading %s\n", program);
    return 1;
  }

  int result = X86_64RuntimeRun(&runtime, argc, argv, program_arg_offset);
  X86_64RuntimeDestruct(&runtime);
  return result;
}
