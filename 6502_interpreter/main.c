//
//  main.c
//  6502_interpreter
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include <limits.h>
#include <stdio.h>
#include "loader.h"
#include "loader_arch_6502.h"
#include "6502_interpreter.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern bool print_libraries_only;

static void Usage() {
  fprintf(stderr, "usage: 6502 [-d] [-x outfile] [-rom file] filename\n");
  exit(1);
}

static bool PathIsFile(const char* path) {
  struct stat st;
  return path != NULL && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static void SetPathDirectory(String* out, const char* path) {
  const char* slash = strrchr(path, '/');
  if (slash == NULL) {
    StringSet(out, ".");
  } else if (slash == path) {
    StringSet(out, "/");
  } else {
    StringInitFromSegment(out, path, (size_t)(slash - path));
  }
}

static bool SetInvocationPath(String* out, const char* argv0) {
  if (strchr(argv0, '/') != NULL) {
    if (argv0[0] == '/') {
      StringSet(out, argv0);
      return true;
    }
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
      return false;
    }
    StringPrintf(out, "%s/%s", cwd, argv0);
    return true;
  }

  const char* path_env = getenv("PATH");
  if (path_env == NULL) {
    return false;
  }
  char* paths = strdup(path_env);
  if (paths == NULL) {
    return false;
  }
  bool found = false;
  char* save = NULL;
  for (char* dir = strtok_r(paths, ":", &save); dir != NULL;
       dir = strtok_r(NULL, ":", &save)) {
    String candidate = {0};
    StringPrintf(&candidate, "%s/%s", dir, argv0);
    if (access(candidate.value, X_OK) == 0) {
      StringSetString(out, &candidate);
      found = true;
      StringDestruct(&candidate);
      break;
    }
    StringDestruct(&candidate);
  }
  free(paths);
  return found;
}

static bool TryROMPath(String* out, const char* base, const char* relative) {
  if (base == NULL || base[0] == '\0') {
    return false;
  }
  String candidate = {0};
  if (relative == NULL || relative[0] == '\0') {
    StringInit(&candidate, base);
  } else {
    StringPrintf(&candidate, "%s/%s", base, relative);
  }
  bool found = false;
  if (PathIsFile(candidate.value)) {
    char resolved[PATH_MAX];
    if (realpath(candidate.value, resolved) != NULL) {
      StringSet(out, resolved);
      found = true;
    }
  }
  StringDestruct(&candidate);
  return found;
}

static bool FindDefaultROM(String* rom, const char* argv0) {
  if (TryROMPath(rom, getenv("DAVECC_6502_ROM"), NULL)) {
    return true;
  }

  const char* root = getenv("DAVECC_ROOT");
  const char* root_relatives[] = {
      "bazel-bin/6502_support/6502rom.exe",
      "lib/davecc/6502rom.exe",
      "share/davecc/6502rom.exe",
  };
  for (size_t i = 0; i < sizeof(root_relatives) / sizeof(root_relatives[0]);
       i++) {
    if (TryROMPath(rom, root, root_relatives[i])) {
      return true;
    }
  }

  String invocation = {0};
  String invocation_dir = {0};
  String executable_dir = {0};
  if (SetInvocationPath(&invocation, argv0)) {
    SetPathDirectory(&invocation_dir, invocation.value);
    char resolved[PATH_MAX];
    if (realpath(invocation.value, resolved) != NULL) {
      SetPathDirectory(&executable_dir, resolved);
    }
  }
  StringDestruct(&invocation);

  String* roots[] = {&invocation_dir, &executable_dir};
  const char* relatives[] = {
      "6502_support/6502rom.exe",
      "6502rom.exe",
      "../lib/davecc/6502rom.exe",
      "../share/davecc/6502rom.exe",
  };
  bool found = false;
  for (size_t i = 0; !found && i < sizeof(roots) / sizeof(roots[0]); i++) {
    for (size_t j = 0; j < sizeof(relatives) / sizeof(relatives[0]); j++) {
      if (TryROMPath(rom, roots[i]->value, relatives[j])) {
        found = true;
        break;
      }
    }
  }
  StringDestruct(&invocation_dir);
  StringDestruct(&executable_dir);
  if (found) {
    return true;
  }
  return TryROMPath(rom, ".", "bazel-bin/6502_support/6502rom.exe");
}

int main(int argc, char *argv[]) {
  const char* file = NULL;
  int program_arg_offset = 1;
  bool disassemble_only = false;
  bool extract = false;
  const char* extract_filename = NULL;
  bool debug = false;
  bool cycle_accurate = false;
  bool trace = false;
  const char* rom_filename = NULL;
  for (int i = 1; i < argc; i++) {
    // Once the executable is known, every remaining token belongs to the
    // guest, including arguments such as "-d" that resemble interpreter
    // options.
    if (file != NULL) {
      continue;
    }
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "-debug") == 0) {
        debug = true;
      } else if (strcmp(argv[i], "-trace") == 0) {
          trace = true;
      } else if (strcmp(argv[i], "-cycle") == 0) {
        cycle_accurate = true;
      } else if (argv[i][1] == 'd') {
        disassemble_only = true;
      } else if (argv[i][1] == 'x') {
          extract = true;
        if (i == argc - 1) {
          Usage();
        }
        extract_filename = argv[++i];
      } else if (strcmp(argv[i], "-rom") == 0) {
        if (i == argc - 1) {
          Usage();
        }
        rom_filename = argv[++i];
      } else {
        Usage();
      }
    } else {
      if (file == NULL) {
        file = argv[i];
        program_arg_offset = i+1;
      }
    }
  }
  if (file == NULL) {
    fprintf(stderr, "usage: need a file to execute\n");
    exit(1);
  }
  String filename;
  StringInit(&filename, file);
  
  W65C02Interpreter interpreter;
  Loader loader;
  String default_rom = {0};
  
  if (rom_filename == NULL) {
    if (!FindDefaultROM(&default_rom, argv[0])) {
      fprintf(stderr,
              "Unable to find the 65C02 support ROM; pass -rom <file>, set "
              "DAVECC_6502_ROM or DAVECC_ROOT, or build "
              "//:support_rom_65c02\n");
      exit(1);
    }
    rom_filename = default_rom.value;
  }
  W65C02InterpreterInit(&interpreter, debug, cycle_accurate, trace, rom_filename);
  StringDestruct(&default_rom);
  
  // The LD_TRACE_LOADED_OBJECTS variable shows the loaded objects
  // and doesn't run the program.
  char* ld_trace = getenv("LD_TRACE_LOADED_OBJECTS");
  if (ld_trace != NULL && ld_trace[0] != '\0') {
    print_libraries_only = true;
  }
  
  // Initialize a 6502 architecture.
  LoaderArchitecture arch;
  W65C02LoaderArchitectureInit(&arch);
  
  // Initialize the loader from the given exe file.
  bool ok = LoaderInitFromFile(&loader, &filename, 0,
                               &arch,
                               NULL,
                               ".");
  if (!ok) {
    printf("Error Loading %s\n", filename.value);
    exit(1);
  }
  
  if (print_libraries_only) {
    exit(0);
  }
  
 
  int result = 0;
  if (disassemble_only) {
    W65C02InterpreterDisassemble(&interpreter, &loader);
  } else {
    // The 6502 interpreter runs the guest inside a 64K memory array at the
    // ELF linked addresses, so the entry point must be the guest-linked entry
    // (e_entry).  The shared loader rewrites loader.main_address to a host
    // runtime pointer for ignore_vaddr architectures, which is meaningless for
    // the in-array 6502, so use the original ELF entry directly.
    uint64_t entry = loader.elf_file != NULL ? loader.elf_file->header->entry
                                             : loader.main_address;
    result = W65C02InterpreterRun(&interpreter, &loader, entry, argc, argv,
                                  program_arg_offset);
  }
  
  if (extract) {
    FILE* fp = fopen(extract_filename, "w");
    W65C02InterpreterExtract(&interpreter, &loader, fp);
    fclose(fp);
    printf("Extracted to %s\n", extract_filename);
  }
  W65C02InterpreterDestruct(&interpreter);
  LoaderDestruct(&loader);
  return result;
}

