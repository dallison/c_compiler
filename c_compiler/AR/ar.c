//
//  ar.c
//
//  Created by David Allison on 1/27/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "ar.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

//
// Symbol table.  This is a hash table of Vectors.  The Vectors
// contain ARSymbol pointers.
//

ARSymbol* NewARSymbol(const char* name) {
  ARSymbol* sym = malloc(sizeof(ARSymbol));
  StringInit(&sym->name, name);
  sym->file = NULL;
  return sym;
}

void ARSymbolDestruct(ARSymbol* sym) {
  StringDestruct(&sym->name);
}

void ARSymbolDelete(ARSymbol* sym) {
  ARSymbolDestruct(sym);
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
  uint32_t hash = 5381;
  while (*name != '\0') {
    hash = (hash << 5) + hash + *name++;
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
    ARSymbol* sym = bucket->value.p[i];
    if (StringEqual(&sym->name, (char*)value)) {
      return sym;
    }
  }
  return NULL;
}

static void DeleteSymbolList(void* entry, void* data) {
  Vector* bucket = (Vector*)entry;
  for (size_t i = 0; i < bucket->length; i++) {
    ARSymbol* sym = bucket->value.p[i];
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
    ARSymbol* symbol = bucket->value.p[i];
    printf("%-30s %-20s @0x%llx\n", symbol->name.value,
           symbol->file->filename.value,
           symbol->file->file_offset);
  }
}

void ARArchivePrintSymbolTable(ARArchive* archive) {
  HashTableTraverse(&archive->symbol_table, PrintSymbolList, NULL);
}

void ARArchiveInit(ARArchive* archive, const char* filename) {
  StringInit(&archive->filename, filename);
  VectorInit(&archive->files);
  archive->extended_filenames_offset = 0;
  archive->symbol_table_file = NULL;
  HashTableInit(&archive->symbol_table, "symbol-table", 1009,
                HashSymbol, InsertInHashTable, FindInHashTable);
  MapInitForInt64Keys(&archive->file_offsets);
}

ARArchive* NewARArchive(const char* filename) {
  ARArchive* archive = malloc(sizeof(ARArchive));
  ARArchiveInit(archive, filename);
  return archive;
}

void ARArchiveDestruct(ARArchive* archive) {
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFileDelete(archive->files.value.p[i]);
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
// There are the same number of file offsets as symbol names the
// indexes of each file offset corresponds to the same index in the
// symbol names.
static void ReadSymbolTable(ARArchive* archive, FILE* fp) {
  // Move to start of symbol table file.
  fseek(fp, archive->symbol_table_file->file_offset, SEEK_SET);
  
  // Number of symbols to read.
  int32_t num_entries = ReadBigEndianInt(fp);
  
  // Temporary vector to hold symbols we build.  We hold them
  // here and then insert them into the symbol table after they
  // are all read.  We have to do it this way because the information
  // for each symbol is split into two parts.
  Vector symbols = {0};
  
  // Read the file offsets and create the symbols.  Insert them
  // in the temporary vector.
  for (int32_t i = 0; i < num_entries; i++) {
    int64_t file_offset = ReadBigEndianInt(fp);
    ARSymbol* symbol = NewARSymbol("");
    symbol->file = MapFindInt64Key(&archive->file_offsets, file_offset);
    if (symbol->file == NULL) {
      fprintf(stderr, "Corrupted symbol table\n");
      exit(1);
    }
    VectorAppend(&symbols, symbol);
  }
  
  // Now read all the symbol names and set them in the corresponding ARSymbol
  // object.
  for (int32_t i = 0; i < num_entries; i++) {
    ARSymbol* symbol = symbols.value.p[i];
    ReadSymbolName(&symbol->name, fp);
  }
  
  // We have all the symbols, insert them into the symbol table.
  for (size_t i = 0; i < symbols.length; i++) {
    HashTableInsert(&archive->symbol_table, symbols.value.p[i]);
  }
  
  // We're done with this vector.  All the symbols are now owned
  // by the symbol table.
  VectorDestruct(&symbols);
}

static bool ReadFileHeaders(ARArchive* archive, FILE* fp) {
  while (!feof(fp)) {
    int64_t file_start = ftell(fp);
    ARFileHeader header;
    size_t n = fread(&header, 1, sizeof(header), fp);
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
    ARFile* file = NewARFile("");
    
    // The slash character terminates filenames but is also used
    // as special names when at the start of the filename.
    // If it is '//' then the file contains the extended filenames.
    // If it is '/' then the file is the symbol table.
    // If it is '/xxx' where xxx is a number it is a reference to an extended
    // filename in the '//' file.
    ReadFilename(&file->filename, header.filename);
    file->size = ReadInteger(header.size, sizeof(header.size));
    file->owner = (int)ReadInteger(header.owner, sizeof(header.owner));
    file->group = (int)ReadInteger(header.group, sizeof(header.group));
    file->mode = (int)ReadInteger(header.mode, sizeof(header.mode));
    file->timestamp = ReadInteger(header.timestamp, sizeof(header.timestamp));
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
    MapKeyValue kv;
    kv.key.w = file_start;
    kv.value.p = file;
    MapInsert(&archive->file_offsets, kv);
    
    // Skip to next file header.
    // This is 2-byte aligned.
    int64_t aligned_size = (file->size + 1) & ~1;
    fseek(fp, aligned_size, SEEK_CUR);
  }
  return true;
}

static void ReadExtendedFilenames(ARArchive* archive, FILE* fp) {
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFile* file = archive->files.value.p[i];
    
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
}

bool ARArchiveOpen(ARArchive* archive, FILE* fp) {
  // Read and verify archive header.
  char header[8];
  ssize_t n = fread(header, 1, sizeof(header), fp);
  if (n != sizeof(header) ||
      memcmp(header, AR_MAGIC, sizeof(header)) != 0) {
    return false;
  }
  
  // Header OK, now read all the file headers.
  bool ok = ReadFileHeaders(archive, fp);
  if (!ok) {
    return ok;
  }
  
  // Now get the extended filenames for any files whose name is longer
  // than will fit in the file header.  These are in a special file with the
  // name "//".
  ReadExtendedFilenames(archive, fp);
  
  // Any symbol table?  If so, read it.
  if (archive->symbol_table_file != NULL) {
    ReadSymbolTable(archive, fp);
    // PrintSymbolTable(&archive->symbol_table);
  }
  return true;
}

ARFile* NewARFile(const char* filename) {
  ARFile* file = malloc(sizeof(ARFile));
  StringInit(&file->filename, filename);
  file->file_offset = 0;
  file->size = 0;
  file->owner = 0;
  file->group = 0;
  file->mode = 0;
  file->timestamp = 0;
  file->contents = NULL;
  file->deleted = false;
  file->delete_contents = false;
  return file;
}

void ARFileDestruct(ARFile* file) {
  StringDestruct(&file->filename);
  if (file->delete_contents) {
    free(file->contents);
  }
}

void ARFileDelete(ARFile* file) {
  ARFileDestruct(file);
  free(file);
}

ARFile* ARFileCopyFromArchive(ARFile* file, FILE* fp) {
  ARFile* new_file = NewARFile(file->filename.value);
  new_file->size = file->size;
  new_file->owner = file->owner;
  new_file->group = file->group;
  new_file->mode = file->mode;
  new_file->timestamp = file->timestamp;
  
  // Allocate space for contents and populate it from the original archive
  // file.
  new_file->contents = malloc(new_file->size);
  fseek(fp, file->file_offset, SEEK_SET);
  fread(new_file->contents, 1, new_file->size, fp);
  new_file->delete_contents = true;
  return new_file;
}

ARSymbol* ARArchiveFindSymbol(ARArchive* archive, const char* name) {
  return HashTableSearch(&archive->symbol_table, (void*)name);
}


ARArchiveBuilder* NewARArchiveBuilder(const char* filename) {
  ARArchiveBuilder* archive = malloc(sizeof(ARArchiveBuilder));
  ARArchiveBuilderInit(archive, filename);
  return archive;
}

void ARArchiveBuilderInit(ARArchiveBuilder* archive, const char* filename) {
  StringInit(&archive->filename, filename);
  VectorInit(&archive->files);
  VectorInit(&archive->symbols);
  VectorInit(&archive->long_filenames);
  archive->next_long_filename_offset = 0;
  archive->num_symbols = 0;
  archive->symbol_table_length = 0;
  MapInitForInt64Keys(&archive->file_offsets);
}

void ARArchiveBuilderDestruct(ARArchiveBuilder* archive) {
  StringDestruct(&archive->filename);
  VectorDestructWithContents(&archive->files, (VectorElementDestructor)ARFileDestruct);
  VectorDestructWithContents(&archive->symbols, (VectorElementDestructor)ARSymbolDestruct);
  VectorDestruct(&archive->long_filenames);
  MapDestruct(&archive->file_offsets);
}

void ARArchiveBuilderDelete(ARArchiveBuilder* archive) {
  ARArchiveBuilderDestruct(archive);
  free(archive);
}

ARFile* ARArchiveBuilderAddFile(ARArchiveBuilder* archive,
                                const char* filename,
                                size_t size,
                                int owner,
                                int group,
                                int mode,
                                int64_t timestamp,
                                void* contents) {
  ARFile* file = NewARFile(filename);
  file->size = size;
  file->owner = owner;
  file->group = group;
  file->mode = mode;
  file->timestamp = timestamp;
  file->contents = contents;
  VectorAppend(&archive->files, file);
  return file;
}

void ARArchiveBuilderAddExisingFile(ARArchiveBuilder* archive, ARFile* file) {
  VectorAppend(&archive->files, file);
  // Add the offset to the map of offsets vs file.  This is used by
  // the symbol table to find the file for each symbol.
  MapKeyValue kv;
  kv.key.w = file->file_offset;
  kv.value.p = file;
  MapInsert(&archive->file_offsets, kv);
}

ARSymbol* ARArchiveBuilderAddSymbol(ARArchiveBuilder* archive, ARFile* file,
                               const char* symbol_name) {
  ARSymbol* symbol = NewARSymbol(symbol_name);
  symbol->file = file;
  VectorAppend(&archive->symbols, symbol);
  return symbol;
}

static void WriteBigEndianInt(int32_t v, FILE *fp) {
  for (int i = 3; i >= 0; i--) {
    fputc((v >> (i * 8)) & 0xff, fp);
  }
}

static void WriteSymbolName(String* name, FILE* fp) {
  for (size_t i = 0; i < name->length; i++) {
    fputc(name->value[i], fp);
  }
  fputc('\0', fp);
}

static void AlignFileOffset(FILE* fp) {
  off_t current_offset = ftell(fp);
  if ((current_offset & 1) == 1) {
    fputc(0, fp);
  }
}

static void WriteLeftString(char* dest, const char* src, size_t destlen) {
  while (*src != '\0') {
    *dest++ = *src++;
    destlen--;
  }
  while (destlen-- > 0) {
    *dest++ = ' ';
  }
}

static void WriteLeftInt(char* dest, size_t src, size_t destlen) {
  size_t n = snprintf(dest, destlen, "%zd", src);
  dest += n;
  destlen -= n;
  while (destlen-- > 0) {
    *dest++ = ' ';
  }
}

static void WriteLeftFilename(char* dest, const char* src, size_t destlen) {
  bool terminate = *src != '/';
  while (*src != '\0') {
    *dest++ = *src++;
    destlen--;
  }
  if (terminate) {
    *dest++ = '/';
    destlen--;
  }
  while (destlen-- > 0) {
    *dest++ = ' ';
  }
}

static void BuildFileHeader(ARFileHeader* header, const char* filename,
                            int owner, int group, int mode, int64_t timestamp,
                            size_t size) {
  if (filename != NULL) {
    WriteLeftFilename(header->filename, filename, sizeof(header->filename));
  }
  WriteLeftInt(header->size, size, sizeof(header->size));
  WriteLeftInt(header->owner, owner, sizeof(header->owner));
  WriteLeftInt(header->group, group, sizeof(header->group));
  WriteLeftInt(header->mode, mode, sizeof(header->mode));
  WriteLeftInt(header->timestamp, timestamp, sizeof(header->timestamp));
  header->end[0] = AR_FILE_END[0];
  header->end[1] = AR_FILE_END[1];
}



// Write the file, updating file->file_offset.
static void WriteFile(ARArchiveBuilder* archive, ARFile* file, FILE* fp) {
  // Record header offset.  This will be used by the symbol table to
  // refer to the file.
  file->file_offset = ftell(fp);
  
  ARFileHeader header;
  if (file->filename.length > 15) {
    // Need extended name.
    // Form filename with form "/xxx" where "xxx" if the offset into
    // the extended filenames file.
    char tmp[16];
    snprintf(tmp,
             sizeof(tmp),
             "/%zd", file->extended_filename_offset);
    WriteLeftString(header.filename, tmp, sizeof(header.filename));
  } else {
    WriteLeftFilename(header.filename,
                      file->filename.value, sizeof(header.filename));
  }
  BuildFileHeader(&header, NULL, file->owner,
                  file->group, file->mode, file->timestamp, file->size);
  
  // Write header.
  fwrite(&header, 1, sizeof(header), fp);
  
  // Write contents.
  fwrite(file->contents, 1, file->size, fp);
  
  // Align to 2 bytes.
  AlignFileOffset(fp);
}

static void WriteExtendedFilenames(ARArchiveBuilder* archive, FILE* fp) {
  if (archive->long_filenames.length == 0) {
    return;
  }
  ARFileHeader header;
  BuildFileHeader(&header, "//", 0, 0, 0777, time(NULL), archive->next_long_filename_offset);
  
  // Write header.
  fwrite(&header, 1, sizeof(header), fp);
  
  // Write extended filenames.  Each one is terminated by a newline.
  for (size_t i = 0; i < archive->long_filenames.length; i++) {
    String* filename = archive->long_filenames.value.p[i];
    fwrite(filename->value, 1, filename->length, fp);
    fputc('\n', fp);
  }
  
  // Align to 2 bytes.
  AlignFileOffset(fp);
}

// Count symbols and calculate file length;
static void PrepareSymbolTable(ARArchiveBuilder* archive) {
  archive->symbol_table_length = 4;
  for (size_t i = 0; i < archive->symbols.length; i++) {
    ARSymbol* symbol = (ARSymbol*)archive->symbols.value.p[i];
    if (!symbol->file->deleted) {
      archive->num_symbols++;
      archive->symbol_table_length += symbol->name.length + 1 + 4;
    }
  }
}

// For each file whose name is longer than 15 chars, assign an
// offset into the "//" file.  The filename for this file will
// be "/xxx" where xxx is the offset into the extended filenames
// file, which is terminated by newline.
static void PrepareExtendedFilenames(ARArchiveBuilder* archive) {
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFile* file = (ARFile*)archive->files.value.p[i];
    if (file->filename.length > 15) {
      // Need extended name.
      file->extended_filename_offset = archive->next_long_filename_offset;
      archive->next_long_filename_offset += file->filename.length + 1;
      VectorAppend(&archive->long_filenames, &file->filename);
    }
  }
}

static void WriteSymbolTable(ARArchiveBuilder* archive, FILE* fp) {
  if (archive->symbols.length == 0) {
    return;
  }
  
  ARFileHeader header;
  BuildFileHeader(&header, "/", 0, 0, 0777, time(NULL), archive->symbol_table_length);
  
  // Write header.
  fwrite(&header, 1, sizeof(header), fp);
  
  // Number of symbols.
  WriteBigEndianInt(archive->num_symbols, fp);
  
  // Symbol file offsets.
  for (size_t i = 0; i < archive->symbols.length; i++) {
    ARSymbol* symbol = (ARSymbol*)archive->symbols.value.p[i];
    if (symbol->file->deleted) {
      continue;
    }
    WriteBigEndianInt((int)symbol->file->file_offset, fp);
  }
  
  // Symbol names.
  for (size_t i = 0; i < archive->symbols.length; i++) {
    ARSymbol* symbol = (ARSymbol*)archive->symbols.value.p[i];
    if (symbol->file->deleted) {
      continue;
    }
    WriteSymbolName(&symbol->name, fp);
  }
  
  // Align to 2 bytes.
  AlignFileOffset(fp);
}
  
inline static off_t Align2(off_t offset) {
  return (offset + 1) & ~1;
}

bool ARArchiveBuilderWrite(ARArchiveBuilder* archive) {
  FILE* fp = fopen(archive->filename.value, "w");
  if (fp == NULL) {
    return false;
  }
  // File header,
  fwrite(AR_MAGIC, 1, 8, fp);

  // For compatibility with Linux and other systems, place the symbol
  // table at the beginning of the archive.  It appears that Linux
  // expects it to be there and some tools don't work properly if it's
  // not.
  PrepareSymbolTable(archive);
  
  // It appears that the extended filenames file needs to be second
  // in the archive too.
  PrepareExtendedFilenames(archive);
  
  // Set file offsets.  First file offset is after symbol table file and
  // the extended filenames file.
  off_t offset = Align2(8 + sizeof(ARFileHeader) + archive->symbol_table_length);

  // Next file is the extended filenames file, which may be absent.
  if (archive->long_filenames.length > 0) {
    offset += Align2(sizeof(ARFileHeader) + archive->next_long_filename_offset);
  }
  
  // Calculate the file offsets.
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFile* file = (ARFile*)archive->files.value.p[i];
    file->file_offset = offset;
    offset += Align2(file->size + sizeof(ARFileHeader));
  }
  WriteSymbolTable(archive, fp);

  // Extended filenames file.
  WriteExtendedFilenames(archive, fp);

  // Now write files.
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFile* file = (ARFile*)archive->files.value.p[i];
    if (file->deleted) {
      continue;
    }
    WriteFile(archive, file, fp);
  }
  
  
  fclose(fp);
  return true;
}

typedef struct {
  ARArchiveBuilder* archive;
  Map file_map;
} SymbolTableCopier;

static void CopySymbolTable(void* entry, void* data) {
  Vector* bucket = (Vector*)entry;
  SymbolTableCopier* copier = data;
  for (size_t i = 0; i < bucket->length; i++) {
    ARSymbol* sym = bucket->value.p[i];
    if (sym->file->deleted) {
      continue;
    }
    MapKeyType key = {.p = sym->file};
    void* new_file = MapFind(&copier->file_map, key);
    assert(new_file != NULL);
    ARSymbol* new_sym = malloc(sizeof(ARSymbol));
    new_sym->file = new_file;
    StringInit(&new_sym->name, sym->name.value);
    VectorAppend(&copier->archive->symbols, new_sym);
  }
}

void ARArchiveBuilderCopyArchive(ARArchiveBuilder* to, ARArchive* from, FILE* fp) {
  // Map of original ARFile to new ARFile.  Symbols in the original
  // archive contain references to the old ARFile and the will need
  // to be redirected to the new ARFile.
  SymbolTableCopier copier;
  copier.archive = to;
  MapInitForPointerKeys(&copier.file_map);
  
  for (size_t i = 0; i < from->files.length; i++) {
    ARFile* from_file = from->files.value.p[i];
    if (StringEqual(&from_file->filename, "/")) {
      // Don't copy symbol table.
      continue;
    }
    if (StringEqual(&from_file->filename, "//")) {
       // Don't copy extended filenames file.
       continue;
     }
    ARFile* to_file = ARFileCopyFromArchive(from_file, fp);
    ARArchiveBuilderAddExisingFile(to, to_file);
    
    // Add mapping from from_file to to_file for symbol conversion.
    MapKeyValue kv;
    kv.key.p = from_file;
    kv.value.p = to_file;
    MapInsert(&copier.file_map, kv);
  }
  
  HashTableTraverse(&from->symbol_table, CopySymbolTable, &copier);
  MapDestruct(&copier.file_map);
}
