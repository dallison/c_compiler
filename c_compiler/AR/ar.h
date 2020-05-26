//
//  ar.h
//
//  Created by David Allison on 1/27/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef ar_h
#define ar_h

#include <time.h>
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
  int owner;
  int group;
  int mode;
  time_t timestamp;
  void* contents;
  bool deleted;
  bool delete_contents;
  size_t extended_filename_offset;
} ARFile;

ARFile* NewARFile(const char* filename);
void ARFileDelete(ARFile* file);
ARFile* ARFileCopyFromArchive(ARFile* file, FILE* fp);

typedef struct {
  String name;
  ARFile* file;
} ARSymbol;

ARSymbol* NewARSymbol(const char* name);
void ARSymbolDelete(ARSymbol* sym);

typedef struct {
  String filename;
  Vector files;                   // Vector of ARFile*.
  Map file_offsets;               // Offset vs ARFile*.
  int64_t extended_filenames_offset;  // Offset to "//"
  ARFile* symbol_table_file;      // Symbol table file.
  HashTable symbol_table;         // Symbol table (name vs ARSymbol*)
} ARArchive;

typedef struct {
  String filename;
  Vector files;                   // Vector of ARFile*.
  Vector symbols;                 // Vector of ARSymbol*.
  Vector long_filenames;
  size_t next_long_filename_offset;
  Map file_offsets;               // Offset vs ARFile*.
  int num_symbols;
  size_t symbol_table_length;
} ARArchiveBuilder;

ARArchive* NewARArchive(const char* filename);
void ARArchiveInit(ARArchive* archive, const char* filename);
void ARArchiveDestruct(ARArchive* archive);
void ARArchiveDelete(ARArchive* archive);

bool ARArchiveOpen(ARArchive* archive, FILE* fp);
void ARArchivePrintSymbolTable(ARArchive* archive);

ARSymbol* ARArchiveFindSymbol(ARArchive* archive, const char* name);

// Archive creator.
ARArchiveBuilder* NewARArchiveBuilder(const char* filename);
void ARArchiveBuilderInit(ARArchiveBuilder* archive, const char* filename);
void ARArchiveBuilderDestruct(ARArchiveBuilder* archive);
void ARArchiveBuilderDelete(ARArchiveBuilder* archive);

void ARArchiveBuilderCopyArchive(ARArchiveBuilder* to, ARArchive* from, FILE* fp);

ARFile* ARArchiveBuilderAddFile(ARArchiveBuilder* archive, const char* filename, size_t size,
                                int owner, int group, int mode, int64_t timestamp, void* contents);
void ARArchiveBuilderAddExisingFile(ARArchiveBuilder* archive, ARFile* file);

bool ARArchiveBuilderWrite(ARArchiveBuilder* archive);
ARSymbol* ARArchiveBuilderAddSymbol(ARArchiveBuilder* archive, ARFile* file,
                               const char* symbol_name);

#endif /* ar_h */
