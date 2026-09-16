//
//  main.c
//  Hosted ESP32 LX6 interpreter
//

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader.h"
#include "loader_arch_xtensa.h"
#include "xtensa_interpreter.h"

int main(int argc, char** argv) {
  bool trace = false;
  int program_index = 1;
  if (argc > 1 && strcmp(argv[1], "-d") == 0) {
    trace = true;
    program_index++;
  }
  if (program_index >= argc) {
    fprintf(stderr, "usage: %s [-d] program\n", argv[0]);
    return 2;
  }

  String filename = {0};
  StringInit(&filename, argv[program_index]);
  LoaderArchitecture architecture;
  XtensaLoaderArchitectureInit(&architecture);
  Loader loader;
  if (!LoaderInitFromFile(&loader, &filename,
                          trace ? LOADER_MAP_SYMTAB : 0, &architecture, NULL,
                          ".")) {
    StringDestruct(&filename);
    return 1;
  }

  uint64_t linked_entry;
  if (!LoaderRuntimeAddressToLinked(&loader, loader.main_address,
                                    &linked_entry) ||
      linked_entry > UINT32_MAX) {
    fprintf(stderr, "invalid ESP32 entry address 0x%" PRIx64 "\n",
            loader.main_address);
    LoaderDestruct(&loader);
    StringDestruct(&filename);
    return 1;
  }

  XtensaInterpreter interpreter;
  if (!XtensaInterpreterInit(&interpreter, &loader, (uint32_t)linked_entry,
                             trace)) {
    LoaderDestruct(&loader);
    StringDestruct(&filename);
    return 1;
  }
  int result = XtensaInterpreterRun(&interpreter);
  XtensaInterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
  StringDestruct(&filename);
  return result;
}
