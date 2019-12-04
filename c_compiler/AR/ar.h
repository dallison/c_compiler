//
//  ar.h
//
//  Created by David Allison on 1/27/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef ar_h
#define ar_h

#include <stdio.h>
#include <stdbool.h>
#include "vector.h"
#include "dstring.h"
#include "hashtable.h"
#include "map.h"

// UNIX archive file format.  This uses the System V (and GNU) format
// as opposed to the BSD one.  Mac OS uses the BSD format which differs
// in the way it handles long filenames (>15 bytes).  The symbol
// table is also handled differently and the BSD symbol table is
// architecture specific and unspecified.  The System V format is easier
// and the symbol table is simply a list of symbol names and the offsets
// of the file they are in.
//
// Linux and other UNIX variants that use ELF files use the System V format
// so that's what I chose for this implementation.

#define AR_MAGIC "!<arch>\n"
#define AR_FILE_END "\x60\x0a"

// Header before every file in the archive.  This is fixed in size and all
// fields contain ASCII strings, padded to the right with spaces.  There is
// special handling for filenames > 15 bytes long.
typedef struct {
  char filename[16];    // Offset 0.
  char timestamp[12];   // Offset 16.
  char owner[6];        // Offset 28.
  char group[6];        // Offset 34.
  char mode[8];         // Offset 40.
  char size[10];        // Offset 48
  char end[2];          // Offset 58: 2 chars: 0x60 0x0a
} ARFileHeader;

typedef struct {
  String filename;
  int64_t file_offset;
  int64_t size;
} ARFile;

ARFile* NewARFile(void);
void ARFileDelete(ARFile* file);

typedef struct {
  String name;
  ARFile* file;
} ARSymbol;

ARSymbol* NewARSymbol(void);
void ARSymbolDelete(ARSymbol* sym);

typedef struct {
  String filename;
  Vector files;                   // Vector of ARFile*.
  Map file_offsets;               // Offset vs ARFile*.
  int64_t extended_filenames_offset;  // Offset to "//"
  ARFile* symbol_table_file;      // Symbol table file.
  HashTable symbol_table;         // Symbol table (name vs ARSymbol*)
} ARArchive;

ARArchive* NewARArchive(const char* filename);
void ARArchiveInit(ARArchive* archive, const char* filename);
void ARArchiveDestruct(ARArchive* archive);
void ARArchiveDelete(ARArchive* archive);

bool ARArchiveOpen(ARArchive* archive, FILE* fp);

ARSymbol* ARArchiveFindSymbol(ARArchive* archive, const char* name);

#endif /* ar_h */
