#include <stdio.h>
#include <stdlib.h>
#include "bpf_interpreter.h"
#include "dstring.h"
#include "loader.h"
#include "loader_arch_bpf.h"

int main(int argc, char** argv) {
  bool trace = false;
  int program_index = -1;
  for (int i = 1; i < argc; i++) {
    if (program_index < 0 && argv[i][0] == '-') {
      if (argv[i][1] == 'd') {
        trace = true;
      } else {
        fprintf(stderr, "unsupported flag %s\n", argv[i]);
        return 2;
      }
    } else if (program_index < 0) {
      program_index = i;
    }
  }
  if (program_index < 0) {
    fprintf(stderr, "usage: %s [-d] program\n", argv[0]);
    return 2;
  }

  String filename = {0};
  StringSet(&filename, argv[program_index]);
  BPFInterpreter interpreter;
  BPFInterpreterInit(&interpreter);
  interpreter.trace = trace;

  Loader loader;
  LoaderArchitecture arch;
  BPFLoaderArchitectureInit(&arch);
  bool ok = LoaderInitFromFile(&loader, &filename, 0, &arch,
                               &interpreter.symbol_resolver_code, ".");
  if (!ok) {
    fprintf(stderr, "Error loading %s\n", filename.value);
    BPFInterpreterDestruct(&interpreter);
    return 1;
  }

  int result = BPFInterpreterRun(&interpreter, &loader, loader.main_address);
  BPFInterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
  StringDestruct(&filename);
  return result;
}
