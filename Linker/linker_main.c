//
//  linker_main.c
//  linkerlibrary
//
//  Created by David Allison on 6/27/19.
//  Copyright © 2019 David Allison. All rights reserved.
//
#include <errno.h>
#include <unistd.h>

#include "linker_main.h"
#include <string.h>
#include "linker_dynamic.h"
#include <stdlib.h>


String* Link(int argc, char** argv) {
  Linker linker;
  LinkerInit(&linker);
  
  bool output_set = false;
  
  Vector object_files = {0};
  Vector static_library_names = {0};
  Vector dynamic_library_names = {0};
  Vector library_search_dirs = {0};
  Vector library_searches = {0};

  // Process all input args and flags.
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      // Check for whole-word arguments first.
      bool option_ok = false;
      if (strcmp(argv[i], "-shared") == 0) {
        linker.building_dso = true;
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
        VectorAppend(&static_library_names, (void*)argv[i]);
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
      printf("Failed to read object file %s", filename->value);
    }
    StringDestruct(filename);
  }
  
  if (linker.elf_machine_type == 0) {
    fprintf(stderr, "No files to link\n");
    return NULL;
  }
  
  LinkerInitArchitecture(&linker);
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
  for (size_t i = 0; i < dynamic_library_names.length; i++) {
    LinkerAddDynamicLibrary(&linker, dynamic_library_names.value.p[i]);
  }
  
  // Done with temporary vectors.
  VectorDestruct(&object_files);
  VectorDestruct(&static_library_names);
  VectorDestruct(&dynamic_library_names);
  VectorDestruct(&library_search_dirs);
  VectorDestruct(&library_searches);

  if (num_errors != 0) {
    return NULL;
  }
  if (!linker.fully_static) {
    DynamicLinkerInventSymbols(&linker, linker.dynamic_linker);
    DynamicLinkerGatherDynamicRelocations(&linker);
  }
  
  // Link all the files together.
  LinkerLinkAllFiles(&linker);
  
  // Open the output file and write the ELF file.
  String* output = NewString(linker.output_filename.value);
  FILE* fp = fopen(linker.output_filename.value, "w");
  if (fp == NULL) {
    fprintf(stderr, "Can't open output file %s: %s\n", linker.output_filename.value, strerror(errno));
    return NULL;
  }
  LinkerWriteOutput(&linker, fp);
  fclose(fp);
  
  // We're done.
  LinkerDestruct(&linker);
  return output;
}
