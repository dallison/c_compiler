//
//  main.c
//  linker
//
//  Created by David Allison on 1/9/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stdio.h>
#include "linker.h"
#include <string.h>
#include "linker_dynamic.h"

// Does the string 's' end with 'suffix'?
static bool EndsWith(const char* s, const char* suffix) {
  return strstr(s, suffix) == s + (strlen(s) - strlen(suffix));
}

int main(int argc, const char * argv[]) {
  Linker linker;
  LinkerInit(&linker);
  
  // Output file defaults to a.out but can be set using -o.
  String output_filename;
  StringInit(&output_filename, "a.out");
  
  Vector object_files;
  VectorInit(&object_files);
  
  // Process all input args and flags.
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
        case 'L': {
          // Library search path entry.
          String* path = NewString(&argv[i][2]);
          LinkerAddLibrarySearchDir(&linker, path);
          break;
        case 'l': {
          // Library using search path.
          bool found = LinkerAddLibrary(&linker, &argv[i][2]);
          if (!found) {
            fprintf(stderr, "Unable to find library %s\n", &argv[i][2]);
          }
          break;
        }
        case 'o':
          i++;
          if (i >= argc) {
            fprintf(stderr, "-o needs an output filename\n");
            break;
          }
          StringSet(&output_filename, argv[i]);
          break;
        default: {
          bool option_ok = false;
          if (strcmp(argv[i], "-shared") == 0) {
            linker.dso = true;
            option_ok = true;
          }
          if (!option_ok) {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
          }
          break;
          }
        }
      }
    } else {
      // Static libraries end in ".a".
      if (EndsWith(argv[i], ".a")) {
        LinkerAddStaticLibrary(&linker, argv[i]);
      } else if (EndsWith(argv[i], ".o")) {
        // Object files end in ".o".
        String* filename = NewString(argv[i]);
        VectorAppend(&object_files, filename);
      } if (EndsWith(argv[i], ".so")) {
        LinkerAddDynamicLibrary(&linker, argv[i]);
      } else {
        fprintf(stderr, "Unknown file type %s\n", argv[i]);
      }
    }
  }
  
  if (linker.dso) {
    // We are building a DSO, build the dynamic section.
    linker.dynamic_section = NewDynamicSection();
    DynamicSectionInventSymbols(&linker, linker.dynamic_section);
  }
  
  for (size_t i = 0; i < object_files.length; i++) {
    String* filename = object_files.value[i];
    bool ok = LinkerReadObjectFile(&linker, filename);
    if (!ok) {
      printf("Failed to read object file %s", filename->value);
    }
    StringDestruct(filename);
  }
  VectorDestruct(&object_files);
  
  // Link all the files together.
  LinkerLinkAllFiles(&linker);
  
  // Open the output file and write the ELF file.
  FILE* fp = fopen(output_filename.value, "w");
  LinkerWriteOutput(&linker, fp);
  fclose(fp);
  
  // We're done.
  StringDestruct(&output_filename);
  LinkerDestruct(&linker);
}
