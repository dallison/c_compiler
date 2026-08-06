//
//  main.c
//  aarch64_interpreter
//

#include "aarch64_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void Usage(const char* prog) {
  fprintf(stderr,
          "usage: %s [-i | -n] [-d] [-r] program [args...]\n"
          "  -i, --interpret   Execute with software interpreter\n"
          "  -n, --native      Execute on local CPU (default on AArch64 hosts)\n"
          "  -d                Trace instructions\n"
          "  -r                Trace register changes\n",
          prog);
  exit(2);
}

int main(int argc, char** argv) {
  const char* program = NULL;
  AARCH64ExecutionMode mode = AARCH64DefaultExecutionMode();
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
      mode = kAARCH64ModeInterpret;
      mode_set = true;
    } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--native") == 0) {
      mode = kAARCH64ModeNative;
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

  AARCH64Runtime runtime;
  if (!AARCH64RuntimeInit(&runtime, program, mode, trace_registers,
                          trace_instructions)) {
    fprintf(stderr, "Error loading %s\n", program);
    return 1;
  }

  int result = AARCH64RuntimeRun(&runtime, argc, argv, program_arg_offset);
  AARCH64RuntimeDestruct(&runtime);
  return result;
}
