//
//  ar.c
//
//  Created by David Allison on 1/27/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "ar.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

//
// Symbol table.  This is a hash table of Vectors.  The Vectors
// contain ARSymbol pointers.
//

ARSymbol* NewARSymbol(void) {
  ARSymbol* sym = malloc(sizeof(ARSymbol));
  StringInit(&sym->name, "");
  sym->file = NULL;
  return sym;
}

void ARSymbolDelete(ARSymbol* sym) {
  StringDestruct(&sym->name);
  free(sym);
}

static size_t HashSymbol(void* value, HashTable* table, HashMode mode) {
  const char* name;
  switch (mode) {
    case kHashInsert:
      // For insertion we have a pointer a ARSymbol.
      name = ((ARSymbol*)value)->name.value;
      break;
    case kHashSearch:
      // For search we have pointer to the name.
      // to find.
      name = (const char*)value;
      break;
  }
  size_t hash = 0;
  for (size_t i = 0; name[i] != '\0'; i++) {
    hash = (hash << 1) ^ name[i];
  }
  return hash;
}

static bool InsertInHashTable(void* entry, void* value, void** parent) {
  if (entry == NULL) {
    entry = NewVector();
    *parent = entry;
  }
  Vector* bucket = (Vector*)entry;
  VectorAppend(bucket, value);
  return true;
}

static void* FindInHashTable(void* entry, void* value) {
  if (entry == NULL) {
    return NULL;
  }
  Vector* bucket = (Vector*)entry;
  for (size_t i = 0; i < bucket->length; i++) {
    ARSymbol* sym = bucket->value[i];
    if (StringEqual(&sym->name, (char*)value)) {
      return sym;
    }
  }
  return NULL;
}

static void DeleteSymbolList(void* entry, void* data) {
  Vector* bucket = (Vector*)entry;
  for (size_t i = 0; i < bucket->length; i++) {
    ARSymbol* sym = bucket->value[i];
    ARSymbolDelete(sym);
  }
  VectorDelete(bucket);
}

static void ClearSymbolTable(HashTable* table) {
  HashTableTraverse(table, DeleteSymbolList, NULL);
}

static void PrintSymbolList(void* entry, void* data) {
  Vector* bucket = entry;
  for (size_t i = 0; i < bucket->length; i++) {
    ARSymbol* symbol = bucket->value[i];
    printf("%-30s %-20s @0x%llx\n", symbol->name.value,
           symbol->file->filename.value,
           symbol->file->file_offset);
  }
}

static void PrintSymbolTable(HashTable* table) {
  HashTableTraverse(table, PrintSymbolList, NULL);
}

static int CompareFileOffset(const void* a, const void* b) {
  const MapKeyValue* v1 = a;
  const MapKeyValue* v2 = b;
  return (int)((int64_t)v1->key - (int64_t)v2->key);
}

void ARArchiveInit(ARArchive* archive, const char* filename) {
  StringInit(&archive->filename, filename);
  VectorInit(&archive->files);
  archive->extended_filenames_offset = 0;
  archive->symbol_table_file = NULL;
  HashTableInit(&archive->symbol_table, "symbol-table", 1009,
                HashSymbol, InsertInHashTable, FindInHashTable);
  MapInit(&archive->file_offsets, CompareFileOffset);
}

ARArchive* NewARArchive(const char* filename) {
  ARArchive* archive = malloc(sizeof(ARArchive));
  ARArchiveInit(archive, filename);
  return archive;
}

void ARArchiveDestruct(ARArchive* archive) {
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFileDelete(archive->files.value[i]);
  }
  VectorDestruct(&archive->files);
  ClearSymbolTable(&archive->symbol_table);
  MapDestruct(&archive->file_offsets);
}

void ARArchiveDelete(ARArchive* archive) {
  ARArchiveDestruct(archive);
  free(archive);
}


static void ReadFilename(String* out, const char* in) {
  // Allow // and / as filenames.  Also allow /xxx where
  // xxx is terminated by a space.
  if (*in == '/') {
    if (in[1] == '/') {
      StringSet(out, "//");
    } else if (in[1] == ' ') {
      StringSet(out, "/");
    } else {
      // Allow /xxx.  This is terminated by a space.
      for (;;) {
        char ch = *in++;
        if (ch == ' ') {
          break;
        }
        StringAppendChar(out, ch);
      }
    }
    return;
  }
  for (;;) {
    char ch = *in++;
    if (ch == '/') {
      break;
    }
    StringAppendChar(out, ch);
  }
}

static int64_t ReadInteger(const char* in, int max_length) {
  int64_t value = 0;
  int length = 0;
  while (length < max_length) {
    int ch = *in++;
    if (ch == ' ') {
      break;
    }
    value = value * 10 + ch - '0';
    length++;
  }
  return value;
}

// Read an integer from the file in big endian format.
static int32_t ReadBigEndianInt(FILE *fp) {
  int32_t v = 0;
  for (int i = 0; i < 4; i++) {
    v = v << 8 | (fgetc(fp) & 0xff);
  }
  return v;
}

// Read a zero terminated symbol name from the file into the
// string 'name'.
static void ReadSymbolName(String* name, FILE* fp) {
  while (!feof(fp)) {
    int ch = fgetc(fp);
    if (ch == EOF) {
      break;
    }
    if (ch == '\0') {
      break;
    }
    StringAppendChar(name, ch);
  }
 }

// The System V symbol table format is:
// 1. Number of entries (big endian 32 bit)
// 2. Sequence of file offsets (big endian 32 bit)
// 3. Sequence of symbol names (character strings, zero terminated).
//
// There are the same number of file offsets as symbol names the the
// indexes of each file offset corresponds to the same index in the
// symbol names.
static void ReadSymbolTable(ARArchive* archive, FILE* fp) {
  // Move to start of symbol table file.
  fseek(fp, archive->symbol_table_file->file_offset, SEEK_SET);
  
  // Number of symbols to read.
  int32_t num_entries = ReadBigEndianInt(fp);
  
  // Temporary vector to hold symbols we build.  We hold them
  // here and then insert them into the symbol table after thay
  // are all read.  We have to do it this way because the information
  // for each symbol is split into two parts.
  Vector symbols;
  VectorInit(&symbols);
  
  // Read the file offsets and create the symbols.  Insert them
  // in the temporary vector.
  for (int32_t i = 0; i < num_entries; i++) {
    int64_t file_offset = ReadBigEndianInt(fp);
    ARSymbol* symbol = NewARSymbol();
    symbol->file = MapFind(&archive->file_offsets, (void*)file_offset);
    VectorAppend(&symbols, symbol);
  }
  
  // Now read all the symbol names and set them in the corresponding ARSymbol
  // object.
  for (int32_t i = 0; i < num_entries; i++) {
    ARSymbol* symbol = symbols.value[i];
    ReadSymbolName(&symbol->name, fp);
  }
  
  // We have all the symbols, insert them into the symbol table.
  for (size_t i = 0; i < symbols.length; i++) {
    HashTableInsert(&archive->symbol_table, symbols.value[i]);
  }
  
  // We're done with this vector.  All the symbols are now owned
  // by the symbol table.
  VectorDestruct(&symbols);
}


bool ARArchiveOpen(ARArchive* archive, FILE* fp) {
  // Read and verify archive header.
  char header[8];
  fread(header, 8, 1, fp);
  if (memcmp(header, AR_MAGIC, 8) != 0) {
    return false;
  }
  
  // Header OK, now read all the file headers.
  while (!feof(fp)) {
    int64_t file_start = ftell(fp);
    ARFileHeader header;
    size_t n = fread(&header, sizeof(header), 1, fp);
    if (n < 1) {
      break;
    }
    
    // Verify the file header has the appropriate end marker.
    if (memcmp(header.end, AR_FILE_END, 2) != 0) {
      // Corrupted archive
      fprintf(stderr, "ar: Corrupted archive; invalid file header\n");
      return false;
    }
    
    // All looks good, create the file.
    ARFile* file = NewARFile();
    
    // The slash character terminates filenames but is also used
    // as special names when at the start of the filename.
    // If it is '//' then the file contains the extended filenames.
    // If it is '/' then the file is the symbol table.
    // If it is '/xxx' where xxx is a number it is a reference to an extended
    // filename in the '//' file.
    ReadFilename(&file->filename, header.filename);
    file->size = ReadInteger(header.size, sizeof(header.size));
    file->file_offset = ftell(fp);
    
    if (StringEqual(&file->filename, "//")) {
      // Extended filenames file.
      archive->extended_filenames_offset = file->file_offset;
    }
    if (StringEqual(&file->filename, "/")) {
      archive->symbol_table_file = file;
    }
    
    // Add file to the archive.
    VectorAppend(&archive->files, file);
    
    // Add the offset to the map of offsets vs file.  This is used by
    // the symbol table to find the file for each symbol.
    MapInsert(&archive->file_offsets, (void*)file_start, file);
    
    // Skip to next file header.
    // This is 2-byte aligned.
    int64_t aligned_size = (file->size + 1) & ~1;
    fseek(fp, aligned_size, SEEK_CUR);
  }
  
  // Now get the extended filenames for any files whose name is longer
  // than will fit in the file header.  These are in a special file with the
  // name "//".
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFile* file = archive->files.value[i];
    
    // An extended filename is encoded as a slash followed by an integer.
    // The integer is the offset into the extended file names file.
    if (file->filename.value[0] == '/' && isdigit(file->filename.value[1])) {
      int64_t offset = 0;
      for (size_t j = 1; j < file->filename.length; j++) {
        offset = offset * 10 + file->filename.value[j] - '0';
      }
      
      // Go to file name in extended file names file.
      fseek(fp, archive->extended_filenames_offset + offset, SEEK_SET);
      
      // Replace the placeholder filename.
      StringClear(&file->filename);
      
      // Read the extended name.  This is terminated by a / or newline.
      for (;;) {
        int ch = fgetc(fp);
        if (ch == EOF) {
          break;
        }
        if (ch == '/' || ch == '\n') {
          break;
        }
        StringAppendChar(&file->filename, ch);
      }
    }
  }
  
  // Any symbol table?  If so, read it.
  if (archive->symbol_table_file != NULL) {
    ReadSymbolTable(archive, fp);
    PrintSymbolTable(&archive->symbol_table);
  }
  return true;
}

ARFile* NewARFile(void) {
  ARFile* file = malloc(sizeof(ARFile));
  StringInit(&file->filename, "");
  file->file_offset = 0;
  file->size = 0;
  return file;
}

void ARFileDelete(ARFile* file) {
  StringDestruct(&file->filename);
  free(file);
}

ARSymbol* ARArchiveFindSymbol(ARArchive* archive, const char* name) {
  return HashTableSearch(&archive->symbol_table, (void*)name);
}


