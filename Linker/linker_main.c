//
//  linker_main.c
//  linkerlibrary
//
//  Created by David Allison on 6/27/19.
//  Copyright © 2019 David Allison. All rights reserved.
//
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "linker_main.h"
#include <string.h>
#include "linker_dynamic.h"
#include <stdlib.h>

// This is the default config for when the user doesn't specify one.
// If you add another architecture add it here too.
// The following architectures and layouts are supported by default:
// 1. 6502:
//    a: rom - 16K ROM at 0xc000
//    b: program - loadable program at 0x800
// 2. RISC-V:
//    a: program - loadable program with code at 0x400000000 and data
//                 at 0x410000000.
// 3. PCODE:
//    a: program - loadable program with code at 0x400000000 and data
//                 at 0x410000000.
// 4. AArch64:
//    a: program - loadable program with code at 0x400000000 and data
//                 at 0x410000000.
const char default_config[] =
"  .set false 0\n"
"  .set true 1\n"
"  .set text 1\n"
"  .set data 2\n"
"  .set dynamic 3\n"
"  .set interp 4\n"
"\n"
"  # ELF machine type\n"
"  .set M_6502 6502\n"
"  .set M_RISC_V 243\n"
"  .set M_PCODE 6500\n"
"  .set M_AARCH64 183\n"
"  .set M_ARM 40\n"
"  .set M_X86 3\n"
"  .set M_X86_64 62\n"
"\n"
"  layout {\n"
"    machine: M_6502\n"
"    type: \"rom\"\n"
"\n"
"    segment {\n"
"      type: text\n"
"      alignment: 1\n"
"      region {\n"
"        name: \"text\"\n"
"        section: \".text\"\n"
"        section: \".rodata\"\n"
"        section: \".davecc_stacktrace\"\n"
"        start_addr: 0xc000\n"
"        size: 0x3f00\n"
"      }\n"
"      region {\n"
"        name: \"boot\"\n"
"        section: \".boot\"\n"
"        start_addr: 0xff00\n"
"        size: 0xfa\n"
"      }\n"
"      region {\n"
"        name: \"hwvectors\"\n"
"        section: \".hwvectors\"\n"
"        start_addr: 0xfffa\n"
"        size: 6\n"
"        alignment: 1\n"
"      }\n"
"    }\n"
"  }\n"
"\n"
"  layout {\n"
"    machine: M_6502\n"
"    type: \"program\"\n"
"   \n"
"    segment {\n"
"      type: text\n"
"      alignment: 1\n"
"      region {\n"
"        name: \"text\"\n"
"        section: \".text\"\n"
"        section: \".rodata\"\n"
"        section: \".davecc_stacktrace\"\n"
"        start_addr: 0x800\n"
"      }\n"
"    }\n"
"    segment {\n"
"      alignment: 1\n"
"      type: data\n"
"      name: \"data\"\n"
"      region {\n"
"        name: \"data\"\n"
"        section: \".data\"\n"
"        section: \".preinit_array\"\n"
"        section: \".init_array\"\n"
"        section: \".fini_array\"\n"
"      }\n"
"      region {\n"
"        name: \"bss\"\n"
"        section: \".bss\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: interp\n"
"      region {\n"
"        name: \"interp\"\n"
"      }\n"
"    }\n"
"  }\n"
"\n"
"  layout {\n"
"    machine: M_RISC_V\n"
"    type: \"program\"\n"
"\n"
"    segment {\n"
"      type: text\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"text\"\n"
"        section: \".text\"\n"
"        section: \".rodata\"\n"
"        section: \".davecc_stacktrace\"\n"
"        section: \".eh_frame\"\n"
"        section: \".gcc_except_table\"\n"
"        start_addr: 0x400000000\n"
"        falign: 1\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: data\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"data\"\n"
"        section: \".got\"\n"
"        section: \".got.plt\"\n"
"        section: \".data\"\n"
"        section: \".preinit_array\"\n"
"        section: \".init_array\"\n"
"        section: \".fini_array\"\n"
"        start_addr: 0x410000000\n"
  "      falign: 1\n"
"      }\n"
"      region {\n"
"        name: \"bss\"\n"
"        section: \".bss\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: dynamic\n"
"      region {\n"
"        name: \"dynamic\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: interp\n"
"      region {\n"
"        name: \"interp\"\n"
"      }\n"
"    }\n"
"  }\n"
"\n"
"  layout {\n"
"    machine: M_PCODE\n"
"    type: \"program\"\n"
"\n"
"    segment {\n"
"      type: text\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"text\"\n"
"        section: \".text\"\n"
"        section: \".rodata\"\n"
"        section: \".davecc_stacktrace\"\n"
"        section: \".eh_frame\"\n"
"        section: \".gcc_except_table\"\n"
"        section: \".davecc_except_table\"\n"
"        start_addr: 0x400000000\n"
"        falign: 1\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: data\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"data\"\n"
"        section: \".got\"\n"
"        section: \".got.plt\"\n"
"        section: \".data\"\n"
"        section: \".preinit_array\"\n"
"        section: \".init_array\"\n"
"        section: \".fini_array\"\n"
"        start_addr: 0x410000000\n"
"        falign: 1\n"
"      }\n"
"      region {\n"
"        name: \"bss\"\n"
"        section: \".bss\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: dynamic\n"
"      region {\n"
"        name: \"dynamic\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: interp\n"
"      region {\n"
"        name: \"interp\"\n"
"      }\n"
"    }\n"
"  }\n"
"\n"
"  layout {\n"
"    machine: M_AARCH64\n"
"    type: \"program\"\n"
"\n"
"    segment {\n"
"      type: text\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"text\"\n"
"        section: \".text\"\n"
"        section: \".rodata\"\n"
"        section: \".davecc_stacktrace\"\n"
"        section: \".eh_frame\"\n"
"        section: \".gcc_except_table\"\n"
"        start_addr: 0x400000000\n"
"        falign: 1\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: data\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"data\"\n"
"        section: \".got\"\n"
"        section: \".got.plt\"\n"
"        section: \".data\"\n"
"        section: \".preinit_array\"\n"
"        section: \".init_array\"\n"
"        section: \".fini_array\"\n"
"        start_addr: 0x410000000\n"
"        falign: 1\n"
"      }\n"
"      region {\n"
"        name: \"bss\"\n"
"        section: \".bss\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: dynamic\n"
"      region {\n"
"        name: \"dynamic\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: interp\n"
"      region {\n"
"        name: \"interp\"\n"
"      }\n"
"    }\n"
"  }\n"
"\n"
"  layout {\n"
"    machine: M_ARM\n"
"    type: \"program\"\n"
"\n"
"    segment {\n"
"      type: text\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"text\"\n"
"        section: \".ARM.exidx\"\n"
"        section: \".text\"\n"
"        section: \".rodata\"\n"
"        section: \".davecc_stacktrace\"\n"
"        section: \".gcc_except_table\"\n"
"        section: \".ARM.exidx\"\n"
"        section: \".ARM.extab\"\n"
"        section: \".ARM.extab\"\n"
"        start_addr: 0x40000000\n"
"        falign: 1\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: data\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"data\"\n"
"        section: \".got\"\n"
"        section: \".got.plt\"\n"
"        section: \".data\"\n"
"        section: \".preinit_array\"\n"
"        section: \".init_array\"\n"
"        section: \".fini_array\"\n"
"        start_addr: 0x41000000\n"
"        falign: 1\n"
"      }\n"
"      region {\n"
"        name: \"bss\"\n"
"        section: \".bss\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: dynamic\n"
"      region {\n"
"        name: \"dynamic\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: interp\n"
"      region {\n"
"        name: \"interp\"\n"
"      }\n"
"    }\n"
"  }\n"
"\n"
"  layout {\n"
"    machine: M_X86_64\n"
"    type: \"program\"\n"
"\n"
"    segment {\n"
"      type: text\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"text\"\n"
"        section: \".text\"\n"
"        section: \".rodata\"\n"
"        section: \".davecc_stacktrace\"\n"
"        section: \".eh_frame\"\n"
"        section: \".gcc_except_table\"\n"
"        start_addr: 0x400000000\n"
"        falign: 1\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: data\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"data\"\n"
"        section: \".got\"\n"
"        section: \".got.plt\"\n"
"        section: \".data\"\n"
"        section: \".preinit_array\"\n"
"        section: \".init_array\"\n"
"        section: \".fini_array\"\n"
"        start_addr: 0x410000000\n"
"        falign: 1\n"
"      }\n"
"      region {\n"
"        name: \"bss\"\n"
"        section: \".bss\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: dynamic\n"
"      region {\n"
"        name: \"dynamic\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: interp\n"
"      region {\n"
"        name: \"interp\"\n"
"      }\n"
"    }\n"
"  }\n"
"\n"
"  layout {\n"
"    machine: M_X86\n"
"    type: \"program\"\n"
"\n"
"    segment {\n"
"      type: text\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"text\"\n"
"        section: \".text\"\n"
"        section: \".rodata\"\n"
"        section: \".davecc_stacktrace\"\n"
"        section: \".eh_frame\"\n"
"        section: \".gcc_except_table\"\n"
"        start_addr: 0x08048000\n"
"        falign: 1\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: data\n"
"      alignment: 0x1000\n"
"      region {\n"
"        name: \"data\"\n"
"        section: \".got\"\n"
"        section: \".got.plt\"\n"
"        section: \".data\"\n"
"        section: \".preinit_array\"\n"
"        section: \".init_array\"\n"
"        section: \".fini_array\"\n"
"        start_addr: 0x09000000\n"
"        falign: 1\n"
"      }\n"
"      region {\n"
"        name: \"bss\"\n"
"        section: \".bss\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: dynamic\n"
"      region {\n"
"        name: \"dynamic\"\n"
"      }\n"
"    }\n"
"    segment {\n"
"      type: interp\n"
"      region {\n"
"        name: \"interp\"\n"
"      }\n"
"    }\n"
"  }\n"
"\n";

String* Link(int argc, char** argv) {
  Linker linker;
  LinkerInit(&linker);
  
  bool output_set = false;
  
  Vector object_files = {0};
  Vector static_library_names = {0};
  Vector whole_static_library_names = {0};
  Vector dynamic_library_names = {0};
  Vector library_search_dirs = {0};
  Vector library_searches = {0};
  const char* config_file = NULL;     // -T config-file
  const char* layout_type_name = "program";  // -t layout-type
  bool delete_config = false;
  bool whole_archive = false;
  
  // Process all input args and flags.
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      // Check for whole-word arguments first.
      bool option_ok = false;
      if (strcmp(argv[i], "-shared") == 0) {
        linker.building_dso = true;
        option_ok = true;
      } else if (strcmp(argv[i], "-bind-now") == 0) {
        linker.bind_now = true;
        option_ok = true;
      } else if (strcmp(argv[i], "-defer-init") == 0) {
        linker.defer_program_init = true;
        option_ok = true;
      } else if (strcmp(argv[i], "-Xsymbol-tables") == 0) {
          linker.print_symbol_tables = true;
          option_ok = true;
      } else if (strcmp(argv[i], "-Xrelocations") == 0) {
            linker.print_relocations = true;
            option_ok = true;
      } else if (strcmp(argv[i], "-Xsections") == 0) {
              linker.print_sections = true;
              option_ok = true;
      } else if (strcmp(argv[i], "-static") == 0) {
        linker.fully_static = true;
        option_ok = true;
      } else if (strcmp(argv[i], "-dynamic") == 0) {
        linker.fully_static = false;
        option_ok = true;
      } else if (strcmp(argv[i], "-whole-archive") == 0) {
        whole_archive = true;
        option_ok = true;
      } else if (strcmp(argv[i], "-no-whole-archive") == 0) {
        whole_archive = false;
        option_ok = true;
      } else if (strcmp(argv[i], "-rpath") == 0) {
        i++;
        if (i >= argc) {
          fprintf(stderr, "-rpath needs a path\n");
          break;
        }
        VectorAppend(&linker.rpath, NewString(argv[i]));
        option_ok = true;
        } else if (strcmp(argv[i], "-chdir") == 0) {
          i++;
          if (i >= argc) {
            fprintf(stderr, "-chdir needs a dir\n");
            break;
          }
          chdir(argv[i]);
          option_ok = true;
      } else if (strcmp(argv[i], "-origin") == 0) {
        i++;
        if (i >= argc) {
          fprintf(stderr, "-origin needs a value\n");
          break;
        }
        linker.origin = strtoll(argv[i], NULL, 0);
        option_ok = true;
      } else if (strcmp(argv[i], "-e") == 0) {
        i++;
        if (i >= argc) {
          fprintf(stderr, "-e needs a value\n");
          break;
        }
        StringSet(&linker.entry_symbol, argv[i]);
        option_ok = true;
      }
      if (!option_ok) {
        // Not a word argument, check of letter args.
        switch (argv[i][1]) {
          case 'L': {
            // Library search path entry.
            String* path = NewString(&argv[i][2]);
            VectorAppend(&library_search_dirs, path);
            break;
          case 'l': {
            // Library using search path.
            String* lib = NewString(&argv[i][2]);
            VectorAppend(&library_searches, lib);
            break;
          }
          case 'o':
            i++;
            if (i >= argc) {
              fprintf(stderr, "-o needs an output filename\n");
              break;
            }
            StringSet(&linker.output_filename, argv[i]);
            output_set = true;
            break;
          case 'I':
            // Set interpreter.
            StringSet(&linker.interpreter, &argv[i][2]);
            break;
          case 'T':
            // Linker config file.
            config_file = &argv[i][2];
            break;
          case 't':
            // Layout type.
            layout_type_name = &argv[i][2];
            break;
          default:
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            break;
          }
        }
      }
    } else {
      String arg;
      StringInit(&arg, argv[i]);
      // Static libraries end in ".a".
      if (StringEndsWith(&arg, ".a")) {
        VectorAppend(whole_archive ? &whole_static_library_names
                                  : &static_library_names,
                     (void*)argv[i]);
      } else if (StringEndsWith(&arg, ".o")) {
        // Object files end in ".o".
        String* filename = NewString(argv[i]);
        VectorAppend(&object_files, filename);
      } else if (StringEndsWith(&arg, ".so")) {
        VectorAppend(&dynamic_library_names, (void*)argv[i]);
      } else {
        fprintf(stderr, "Unknown file type %s\n", argv[i]);
      }
      StringDestruct(&arg);
    }
  }
  
  if (!output_set) {
    if (linker.building_dso) {
      // If we are building a shared object find the
      // first .o file and set the output to the same
      // name with the extension .so
      for (size_t i = 0; i < object_files.length; i++) {
        String* filename = object_files.value.p[i];
        if (StringEndsWith(filename, ".o")) {
          StringSubstring(filename, 0,
                          filename->length - 2, &linker.output_filename);
          StringAppend(&linker.output_filename, ".so");
          break;
        }
      }
    }
  }
  
  // Read all the object files.
  for (size_t i = 0; i < object_files.length; i++) {
    String* filename = object_files.value.p[i];
    bool ok = LinkerReadObjectFile(&linker, filename);
    if (!ok) {
      fprintf(stderr, "Failed to read object file %s\n", filename->value);
    }
    StringDestruct(filename);
  }
  
  if (linker.elf_machine_type == 0) {
    fprintf(stderr, "No files to link\n");
    return NULL;
  }
  
  // No config specified, copy default to a temp file.
  if (config_file == NULL) {
    delete_config = true;
    char name[256];
    snprintf(name, sizeof(name), "/tmp/link.XXXXXX");
    int fd = mkstemp(name);
    if (fd < 0) {
      fprintf(stderr, "Cannot open tempfile %s for config\n", name);
      exit(1);
    }
    FILE* fp = fdopen(fd, "w");
    if (fp == NULL) {
      fprintf(stderr, "Cannot open tempfile %s for config\n", name);
      exit(1);
    }
    size_t len = strlen(default_config);
    fwrite(default_config, 1, len, fp);
    fclose(fp);
    config_file = name;
  }
  
  LinkerInitArchitecture(&linker);
  LinkerInitConfigLayout(&linker, config_file, layout_type_name);
  LinkerInitDynamic(&linker);
  
  for (size_t i = 0; i < library_search_dirs.length; i++) {
    LinkerAddLibrarySearchDir(&linker, library_search_dirs.value.p[i]);
  }

  int num_errors = 0;
  for (size_t i = 0; i < library_searches.length; i++) {
    String* lib = library_searches.value.p[i];
    bool found = LinkerAddLibrary(&linker, lib->value);
    if (!found) {
      fprintf(stderr, "Unable to find library %s\n", &argv[i][2]);
      num_errors++;
    }
    StringDelete(lib);
  }

  // Add all static and dynamic libraries.
  for (size_t i = 0; i < static_library_names.length; i++) {
    LinkerAddStaticLibrary(&linker, static_library_names.value.p[i]);
  }
  for (size_t i = 0; i < whole_static_library_names.length; i++) {
    LinkerAddWholeStaticLibrary(
        &linker, whole_static_library_names.value.p[i]);
  }
  for (size_t i = 0; i < dynamic_library_names.length; i++) {
    LinkerAddDynamicLibrary(&linker, dynamic_library_names.value.p[i]);
  }
  
  // Done with temporary vectors.
  VectorDestruct(&object_files);
  VectorDestruct(&static_library_names);
  VectorDestruct(&whole_static_library_names);
  VectorDestruct(&dynamic_library_names);
  VectorDestruct(&library_search_dirs);
  VectorDestruct(&library_searches);
  if (num_errors != 0) {
    if (delete_config) {
      remove(config_file);
    }
    return NULL;
  }
  if (!linker.fully_static) {
    DynamicLinkerInventSymbols(&linker, linker.dynamic_linker);
  }
  
  // Link all the files together.
  LinkerLinkAllFiles(&linker);
  
  if (linker.num_errors != 0) {
    LinkerDestruct(&linker);
    if (delete_config) {
       remove(config_file);
    }
    return NULL;
  }
  // Open the output file and write the ELF file.
  String* output = NewString(linker.output_filename.value);
  FILE* fp = fopen(linker.output_filename.value, "w");
  if (fp == NULL) {
    fprintf(stderr, "Can't open output file %s: %s\n",
            linker.output_filename.value, strerror(errno));
    if (delete_config) {
       remove(config_file);
     }
    return NULL;
  }
  int ok = LinkerWriteOutput(&linker, fp);
  fclose(fp);
  if (ok && !linker.building_dso &&
      chmod(linker.output_filename.value, 0755) != 0) {
    fprintf(stderr, "Can't make output file executable %s: %s\n",
            linker.output_filename.value, strerror(errno));
    ok = 0;
  }
  
  // We're done.
  LinkerDestruct(&linker);
  if (delete_config) {
     remove(config_file);
  }
  return ok ? output : NULL;
}
