//
//  disassembler_tool.c
//  c_compiler
//

#include "disassembler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void Usage(FILE* fp, const char* tool_name, DAsmArchitecture arch) {
  fprintf(fp,
          "usage: %s [--help] [--all] filename [addr [length]]\n"
          "\n"
          "Disassemble executable sections from a %s ELF file.\n"
          "\n"
          "Arguments:\n"
          "  filename       ELF object or executable to disassemble\n"
          "  addr           Optional start address (decimal or 0x-prefixed)\n"
          "  length         Optional number of bytes to disassemble\n"
          "\n"
          "Options:\n"
          "  -h, --help     Show this help message\n"
          "  --all          Print section names before disassembly\n",
          tool_name, DAsmArchitectureName(arch));
}

int DAsmToolMain(int argc, const char** argv, DAsmArchitecture arch,
                 const char* tool_name) {
  DAsmOptions options = {
      .arch = arch,
      .print_section_names = false,
  };
  const char* filename = NULL;
  int positional = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ||
        strcmp(argv[i], "-help") == 0) {
      Usage(stdout, tool_name, arch);
      return 0;
    }
    if (strcmp(argv[i], "--all") == 0) {
      options.print_section_names = true;
      continue;
    }
    if (argv[i][0] == '-') {
      fprintf(stderr, "%s: unknown option '%s'\n\n", tool_name, argv[i]);
      Usage(stderr, tool_name, arch);
      return 1;
    }
    switch (positional) {
      case 0:
        filename = argv[i];
        break;
      case 1:
        options.start_address = (uint64_t)strtoull(argv[i], NULL, 0);
        options.has_start_address = true;
        break;
      case 2:
        options.length = (uint64_t)strtoull(argv[i], NULL, 0);
        options.has_length = true;
        break;
      default:
        fprintf(stderr, "%s: too many arguments: '%s'\n\n", tool_name,
                argv[i]);
        Usage(stderr, tool_name, arch);
        return 1;
    }
    positional++;
  }
  if (filename == NULL) {
    fprintf(stderr, "%s: missing input file\n\n", tool_name);
    Usage(stderr, tool_name, arch);
    return 1;
  }
  return DAsmDisassembleFile(filename, &options, stdout) ? 0 : 1;
}

static void WasmUsage(FILE* fp, const char* tool_name) {
  fprintf(fp,
          "usage: %s [--help] [--all] filename [addr [length]]\n"
          "\n"
          "Disassemble functions from a WebAssembly module or object.\n"
          "\n"
          "Arguments:\n"
          "  filename       wasm object or linked module\n"
          "  addr           Optional start file offset (decimal or 0x-prefixed)\n"
          "  length         Optional number of bytes to disassemble\n"
          "\n"
          "Options:\n"
          "  -h, --help     Show this help message\n"
          "  --all          Print the code section name before disassembly\n",
          tool_name);
}

int DAsmWasmToolMain(int argc, const char** argv, const char* tool_name) {
  DAsmOptions options = {0};
  const char* filename = NULL;
  int positional = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ||
        strcmp(argv[i], "-help") == 0) {
      WasmUsage(stdout, tool_name);
      return 0;
    }
    if (strcmp(argv[i], "--all") == 0) {
      options.print_section_names = true;
      continue;
    }
    if (argv[i][0] == '-') {
      fprintf(stderr, "%s: unknown option '%s'\n\n", tool_name, argv[i]);
      WasmUsage(stderr, tool_name);
      return 1;
    }
    switch (positional) {
      case 0:
        filename = argv[i];
        break;
      case 1:
        options.start_address = (uint64_t)strtoull(argv[i], NULL, 0);
        options.has_start_address = true;
        break;
      case 2:
        options.length = (uint64_t)strtoull(argv[i], NULL, 0);
        options.has_length = true;
        break;
      default:
        fprintf(stderr, "%s: too many arguments: '%s'\n\n", tool_name,
                argv[i]);
        WasmUsage(stderr, tool_name);
        return 1;
    }
    positional++;
  }
  if (filename == NULL) {
    fprintf(stderr, "%s: missing input file\n\n", tool_name);
    WasmUsage(stderr, tool_name);
    return 1;
  }
  return DAsmDisassembleWasmFile(filename, &options, stdout) ? 0 : 1;
}
