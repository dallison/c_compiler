//
//  main.c
//  ar
//
//  Created by David Allison on 1/27/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stdio.h>
#include "ar.h"
#include <stdlib.h>

typedef enum {
  kCommandNone,
  kCommandType,
  kCommandReplace,
  kCommandAddSymbolTable,
} Command;

static Command ParseCommand(const char* p) {
  switch (*p) {
    case 't':
      return kCommandType;
    case 'r':
      return kCommandReplace;
    case 's':
      return kCommandAddSymbolTable;
    default:
      fprintf(stderr, "Unknown ar command -%c\n", *p);
      exit(1);
  }
}

static void ShowArchive(ARArchive* archive, const char* filename) {
  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "ar: Unable to open archive file %s\n", filename);
    exit(1);
  }
  bool ok = ARArchiveOpen(archive, fp);
  if (!ok) {
    exit(1);
  }
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFile* file = archive->files.value.p[i];
    if (file->filename.value.p[0] == '/') {
      // Filenames beginning with / are special files in the archive.
      continue;
    }
    printf("%s\n", file->filename.value);
  }
  fclose(fp);
}

int main(int argc, const char * argv[]) {
  Command command = kCommandNone;
  
  const char* archive_filename = NULL;
  Vector files;
  VectorInit(&files);
  
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      const char* p = &argv[i][1];
      command = ParseCommand(p);
    } else {
      if (command == kCommandNone) {
        // No command with - given, allow next string to be command
        command = ParseCommand(argv[i]);
      } else if (archive_filename == NULL) {
        archive_filename = argv[i];
      } else {
        VectorAppend(&files, (char*)argv[i]);
      }
    }
  }
  if (archive_filename == NULL) {
    fprintf(stderr, "ar: Missing archive filename\n");
    exit(1);
  }

  ARArchive archive;
  ARArchiveInit(&archive), archive_filename;
  
  switch (command) {
    case kCommandAddSymbolTable:
      break;
    case kCommandNone:
      break;
    case kCommandType:
      ShowArchive(&archive, archive_filename);
      break;
    case kCommandReplace:
      break;
  }
}
