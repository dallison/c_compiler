//
//  main.c
//  archvist
//
//  Created by David Allison on 2/27/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

// An ar-like command line utility.  Not exactly the same as ar.
// Members are ELF objects, or wasm objects for the wasm32 target, which are
// modules rather than ELF and so carry their symbols somewhere else.
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "ar.h"
#include "elf_reader.h"
#include "vector.h"
#include "dstring.h"
#include "set.h"
#include "wasm32_object.h"

typedef enum  {
  kNone = 0,
  kType = 1,        // Show files in archive.
  kReplace = 2,     // Replace or append files.
  kDelete = 4,      // Delete files.
  kExtract = 8,     // Extract files
  kSymbols = 16,    // Show symbols (line nm -s on Linux)
  kEndCommands = 32,
  
  // Modifiers below.
  kCreate = 128,        // Don't warn if archive is created.
  kUpdate = 256,        // Update only if newer.
  kVerbose = 512,       // Verbose output, like ar.
} Command;

static bool CommandCompatible(Command old, Command new) {
  if (old == kNone) {
    return true;
  }
  if (new == old) {
    return true;
  }
  int mask = kEndCommands - 1;  // Don't include modifiers.
  return ((old | new) & mask) == old;
}

static Command ParseCommand(const char* s) {
  Command command = kNone;
  Command new;
  while (*s != '\0') {
    switch (*s) {
      case 't':
        new = kType;
        break;
      case 'u':
        new = kUpdate;
        break;
      case 'r':
        new = kReplace;
        break;
      case 'd':
        new = kDelete;
        break;
      case 'x':
         new = kExtract;
         break;
      case 's':
         new = kSymbols;
         break;
      case 'c':
        new = kCreate;
        break;
      case 'v':
        new = kVerbose;
        break;
      default:
        fprintf(stderr, "Unknown command letter '%c'\n", *s);
        exit(1);
    }
    if (!CommandCompatible(command, new)) {
      fprintf(stderr, "Ambiguous command\n");
      exit(1);
    }
    command |= new;
    s++;
  }
  return command;
}

static void AddELFSymbols(ARArchiveBuilder* builder, ELFReaderFile* elf, ARFile* file) {
  Vector symbol_tables;
  VectorInit(&symbol_tables);
  ELFReaderFileFindSectionsByType(elf, SHT(symtab), &symbol_tables);
  
  for (size_t sect = 0; sect < symbol_tables.length; sect++) {
    ELFReaderSection* symtab = symbol_tables.value.p[sect];
    if (symtab->header->link >= elf->sections.length) {
      fprintf(stderr, "archivist: corrupt symbol table link value in %s\n", elf->filename.value);
      continue;
    }
    ELFReaderSection* strtab = elf->sections.value.p[symtab->header->link];
    // Symbol entries are decoded through the format ops so that both ELF32 and
    // ELF64 object files are handled.  The on-disk stride is the format's
    // symbol size; section contents point into the mapped file.
    size_t sym_size = elf->ops->symbol_size;
    size_t num_symbols = sym_size ? (symtab->header->size / sym_size) : 0;
    const char* symbol_addr = (const char*)symtab->contents;

    // Now read the symbols and add them to the symbol tables in the file.
    for (size_t i = 0; i < num_symbols; i++) {
      ELFSymbol elf_sym;
      elf->ops->ReadSymbol(&elf_sym, symbol_addr);
      int64_t binding = ELF_ST_BIND(elf_sym.info);
      if (elf_sym.name > strtab->header->size) {
        fprintf(stderr, "archivist: corrupt symbol name in %s\n", elf->filename.value);
        symbol_addr += sym_size;
        continue;
      }
      const char* sym_name = (const char*)strtab->contents + elf_sym.name;
      if (sym_name[0] == '\0') {
        // Don't insert empty symbol.
        symbol_addr += sym_size;
        continue;
      }
      bool is_local_symbol = binding == STB(local);
      bool is_defined = elf_sym.shndx != 0;
      if (!is_local_symbol && is_defined) {
        ARArchiveBuilderAddSymbol(builder, file, sym_name);
      }
      symbol_addr += sym_size;
    }
  }
  VectorDestruct(&symbol_tables);
}

// One file on its way into the archive.  Reading it is what decides whether
// it is ELF or wasm, and the two differ only in where the names it defines
// are written down.
typedef struct {
  String filename;
  struct stat file_stat;
  void* bytes;
  size_t size;
  ELFReaderFile* elf;        // Set for an ELF member.
  Wasm32ObjectFile* object;  // Set for a wasm member.
} ArchiveInput;

// A wasm object's symbol table lives in its 'linking' section, so the names
// worth indexing are the defined ones that are not file-local, exactly as
// for ELF.
static void AddWasmSymbols(ARArchiveBuilder* builder, Wasm32ObjectFile* object,
                           ARFile* file) {
  for (size_t i = 0; i < object->symbols.length; i++) {
    Wasm32Symbol* symbol = object->symbols.value.p[i];
    bool is_local = (symbol->flags & WASM_SYM_BINDING_LOCAL) != 0;
    bool is_defined = (symbol->flags & WASM_SYM_UNDEFINED) == 0;
    if (!is_local && is_defined && symbol->name[0] != '\0') {
      ARArchiveBuilderAddSymbol(builder, file, symbol->name);
    }
  }
}

static void ReplaceFile(ARArchiveBuilder* builder, ArchiveInput* input,
                        Command command) {
  // See if the file exists in the archive.
  ARFile* file = NULL;
  for (size_t i = 0; i < builder->files.length; i++) {
    ARFile* f = builder->files.value.p[i];
    if (StringEqualString(&input->filename, &f->filename)) {
      file = f;
      break;
    }
  }
  // File timestamp is in seconds since epoch.  On Mac OS the timestamps
  // in the file are in nanoseconds using struct timespec.
#if defined(__APPLE__)
  int64_t timestamp = input->file_stat.st_mtimespec.tv_sec;
#elif defined(__linux__)
  int64_t timestamp = input->file_stat.st_mtime;
#else
#error "Unknown OS"
#endif
  if (file != NULL) {
    // File exists in archive, check if we need to replace it.
    if ((command & kUpdate) != 0) {
      if (timestamp < file->timestamp) {
        // New file is older, don't replace.
        return;
      }
    }
    file->deleted = true;
  }
  if ((command & kVerbose) != 0) {
    printf("r - %s\n", input->filename.value);
  }
  file = ARArchiveBuilderAddFile(builder, input->filename.value, input->size,
                                 input->file_stat.st_uid,
                                 input->file_stat.st_gid,
                                 input->file_stat.st_mode, timestamp,
                                 input->bytes);
  if (input->elf != NULL) {
    AddELFSymbols(builder, input->elf, file);
  } else {
    AddWasmSymbols(builder, input->object, file);
  }
}

// Try the file as a wasm object.  The bytes have to stay alive as long as
// the archive builder holds them, so they are handed over rather than freed.
static bool ReadWasmInput(ArchiveInput* input) {
  FILE* fp = fopen(input->filename.value, "rb");
  if (fp == NULL) {
    return false;
  }
  if (fstat(fileno(fp), &input->file_stat) != 0) {
    fclose(fp);
    return false;
  }
  size_t size = (size_t)input->file_stat.st_size;
  uint8_t* bytes = malloc(size == 0 ? 1 : size);
  bool ok = fread(bytes, 1, size, fp) == size;
  fclose(fp);
  if (!ok || size < 8 || memcmp(bytes, "\0asm", 4) != 0) {
    free(bytes);
    return false;
  }
  input->object = malloc(sizeof(Wasm32ObjectFile));
  if (!Wasm32ReadObjectFile(input->object, input->filename.value, bytes,
                            size)) {
    Wasm32ObjectFileDestruct(input->object);
    free(input->object);
    input->object = NULL;
    free(bytes);
    return false;
  }
  input->bytes = bytes;
  input->size = size;
  return true;
}

static void ArchiveInputDelete(ArchiveInput* input) {
  StringDestruct(&input->filename);
  if (input->elf != NULL) {
    ELFReaderFileDestruct(input->elf);
    free(input->elf);
  }
  if (input->object != NULL) {
    Wasm32ObjectFileDestruct(input->object);
    free(input->object);
    free(input->bytes);
  }
  free(input);
}

static void ReplaceFiles(String* archive_name, Vector* filenames, Command command) {
  Vector inputs = {0};
  Vector known_files = {0};
  for (size_t i = 0; i < filenames->length; i++) {
    // Remove duplicate file.
    bool dup = false;
    for (size_t j = 0; j < known_files.length; j++) {
      if (strcmp(filenames->value.p[i], known_files.value.p[j]) == 0) {
        dup = true;
        break;
      }
    }
    if (dup) {
      continue;
    }
    VectorAppend(&known_files, filenames->value.p[i]);

    ArchiveInput* input = calloc(1, sizeof(ArchiveInput));
    StringInit(&input->filename, filenames->value.p[i]);

    ELFReaderFile* elf = NewELFReaderFile(&input->filename);
    if (ELFReaderFileRead(elf, 0, 0)) {
      input->elf = elf;
      input->file_stat = elf->file_stat;
      input->bytes = (void*)elf->base;
      input->size = (size_t)elf->file_stat.st_size;
      VectorAppend(&inputs, input);
      continue;
    }
    ELFReaderFileDestruct(elf);
    free(elf);

    if (ReadWasmInput(input)) {
      VectorAppend(&inputs, input);
      continue;
    }
    fprintf(stderr, "%s is neither an ELF file nor a wasm object\n",
            (const char*)filenames->value.p[i]);
    ArchiveInputDelete(input);
  }
  VectorDestruct(&known_files);
  
  ARArchiveBuilder builder;
  String temp_name = {0};
  
  struct stat st;
  int e = stat(archive_name->value, &st);
  if (e == 0) {
    // File exists, open the archive and copy to the builder.
    ARArchive archive;
    ARArchiveInit(&archive, archive_name->value);
    FILE* fp = fopen(archive_name->value, "r");
    if (fp == NULL || !ARArchiveOpen(&archive, fp)) {
      fprintf(stderr, "Failed to read archive file %s\n", archive_name->value);
      return;
    }
    StringPrintf(&temp_name, "%s.tmp", archive_name->value);
    ARArchiveBuilderInit(&builder, temp_name.value);
    ARArchiveBuilderCopyArchive(&builder, &archive, fp);
    fclose(fp);
  } else {
    if ((command & kCreate) == 0) {
      printf("archivist: creating %s\n", archive_name->value);
    }
    ARArchiveBuilderInit(&builder, archive_name->value);
  }
  
  for (size_t i = 0; i < inputs.length; i++) {
    ReplaceFile(&builder, inputs.value.p[i], command);
  }
  
  ARArchiveBuilderWrite(&builder);
  
  // Was the file written to a temp file?  If so, remove old one and
  // rename.
  if (temp_name.length != 0) {
    remove(archive_name->value);
    rename(temp_name.value, archive_name->value);
    StringDestruct(&temp_name);
  }
  for (size_t i = 0; i < inputs.length; i++) {
    ArchiveInputDelete(inputs.value.p[i]);
  }
  VectorDestruct(&inputs);
}

// rw-rw-r-- 1000/1000   1904 Jan 27 08:58 2018 long_file_name_program.o
static void ShowFileDetails(ARFile* file) {
  char mode[10];
  const char* mode_bits[2] = {"---------", "rwxrwxrwx"};
  for (int i = 8; i >= 0; i--) {
    int bit = (file->mode >> i) & 1;
    mode[8-i] = mode_bits[bit][8-i];
  }
  mode[9] = '\0';
  struct tm* filetime = localtime(&file->timestamp);
  char date[256];
  strftime(date, sizeof(date), "%c", filetime);
  
  printf("%s %d/%d %zd %s %s\n", mode,
         file->owner, file->group,
         (size_t)file->size,
         date,
         file->filename.value);
}

void ShowArchiveContents(String* archive_name, Vector* filenames, Command command) {
  FILE* fp = fopen(archive_name->value, "r");
  if (fp == NULL) {
    fprintf(stderr, "archivist: unable to open archive %s: %s\n", archive_name->value, strerror(errno));
    exit(1);
  }
  ARArchive archive;
  ARArchiveInit(&archive, archive_name->value);
  if (ARArchiveOpen(&archive, fp)) {
    for (size_t i = 0; i < archive.files.length; i++) {
      ARFile* file = archive.files.value.p[i];
      if (file->filename.value[0] == '/') {
        continue;
      }
      if ((command & kVerbose) != 0) {
        ShowFileDetails(file);
      } else {
        printf("%s\n", file->filename.value);
      }
    }
  } else {
    fprintf(stderr, "archivist: unable to read archive %s\n", archive_name->value);
  }
  fclose(fp);
}

static void DeleteFile(ARArchiveBuilder* builder, const char* filename, bool verbose) {
  for (size_t i = 0; i < builder->files.length; i++) {
    ARFile* f = builder->files.value.p[i];
    if (StringEqual(&f->filename, filename)) {
      f->deleted = true;
      if (verbose) {
        printf("d - %s\n", filename);
      }
      break;
    }
  }
  if (verbose) {
    fprintf(stderr, "archivist: no such member %s\n", filename);
  }
}

static void DeleteFiles(String* archive_name, Vector* filenames, Command command) {
  ARArchiveBuilder builder;
  String temp_name = {0};
  
  struct stat st;
  int e = stat(archive_name->value, &st);
  if (e == 0) {
    // File exists, open the archive and copy to the builder.
    ARArchive archive;
    ARArchiveInit(&archive, archive_name->value);
    FILE* fp = fopen(archive_name->value, "r");
    if (fp == NULL || !ARArchiveOpen(&archive, fp)) {
      fprintf(stderr, "Failed to read archive file %s\n", archive_name->value);
      return;
    }
    StringPrintf(&temp_name, "%s.tmp", archive_name->value);
    ARArchiveBuilderInit(&builder, temp_name.value);
    ARArchiveBuilderCopyArchive(&builder, &archive, fp);
    fclose(fp);
  } else {
    printf("archivist: creating %s\n", archive_name->value);
    ARArchiveBuilderInit(&builder, archive_name->value);
  }
  
  for (size_t i = 0; i < filenames->length; i++) {
    DeleteFile(&builder, filenames->value.p[i], command & kVerbose);
  }
  
  ARArchiveBuilderWrite(&builder);
  
  // Was the file written to a temp file?  If so, remove old one and
  // rename.
  if (temp_name.length != 0) {
    remove(archive_name->value);
    rename(temp_name.value, archive_name->value);
    StringDestruct(&temp_name);
  }
}

static void ExtractFile(ARArchive* archive, const char* filename, bool verbose, FILE* fp) {
  for (size_t i = 0; i < archive->files.length; i++) {
    ARFile* file = archive->files.value.p[i];
    if (StringEqual(&file->filename, filename)) {
      FILE* outfp = fopen(filename, "w");
      if (outfp == NULL) {
        fprintf(stderr, "archivist: failed to write file %s: %s\n", filename, strerror(errno));
        return;
      }
      if (verbose) {
        printf("x - %s\n", filename);
      }
      char buf[4096];
      fseek(fp, (int)file->file_offset, SEEK_SET);
      off_t offset = ftell(fp);
      if (offset == -1) {
        fprintf(stderr, "archivist: failed to seek to offset in archive file %s: %s\n", filename, strerror(errno));
        break;
      }
      size_t remaining = file->size;
      while (remaining > 0) {
        size_t bytes_to_read = remaining;
        if (bytes_to_read > sizeof(buf)) {
          bytes_to_read = sizeof(buf);
        }
        size_t n = fread(buf, 1, bytes_to_read, fp);
        if (n == 0) {
          fprintf(stderr, "archivist: failed to read archive file %s: %s\n", filename, strerror(errno));
          break;
        }
        fwrite(buf, 1, n, outfp);
        remaining -= n;
      }
      fclose(outfp);
    }
  }
}

static void ExtractFiles(String* archive_name, Vector* filenames, Command command) {
  struct stat st;
  int e = stat(archive_name->value, &st);
  if (e == -1) {
    fprintf(stderr, "archivist: %s: %s\n", archive_name->value, strerror(errno));
    return;
  }
  ARArchive archive;
  ARArchiveInit(&archive, archive_name->value);
  FILE* fp = fopen(archive_name->value, "r");
  if (fp == NULL || !ARArchiveOpen(&archive, fp)) {
    fprintf(stderr, "Failed to read archive file %s\n", archive_name->value);
    return;
  }
  for (size_t i = 0; i < filenames->length; i++) {
    ExtractFile(&archive, filenames->value.p[i], command & kVerbose, fp);
  }
  fclose(fp);
}

static void ShowSymbolTable(String* archive_name) {
  struct stat st;
  int e = stat(archive_name->value, &st);
  if (e == -1) {
    fprintf(stderr, "archivist: %s: %s\n", archive_name->value, strerror(errno));
    return;
  }
  ARArchive archive;
  ARArchiveInit(&archive, archive_name->value);
  FILE* fp = fopen(archive_name->value, "r");
  if (fp == NULL || !ARArchiveOpen(&archive, fp)) {
    fprintf(stderr, "Failed to read archive file %s\n", archive_name->value);
    return;
  }
  if (archive.symbol_table_file != NULL) {
    ARArchivePrintSymbolTable(&archive);
  }
  fclose(fp);
}

int main(int argc, char* argv[]) {
  Vector files = {0};     // Contains char* from argv.
  String archive = {0};

  Command command = kNone;
  for (int i = 1; i < argc; i++) {
    if (command == kNone) {
      command = ParseCommand(argv[i]);
    } else if (archive.length == 0) {
      StringInit(&archive, argv[i]);
    } else {
      VectorAppend(&files, argv[i]);
    }
  }
  
  if (archive.length == 0) {
    fprintf(stderr, "archivist: specify archive file\n");
    exit(1);
  }
  
  switch (command & (kEndCommands - 1)) {
    case kReplace:
      ReplaceFiles(&archive, &files, command);
      break;
    case kType:
      ShowArchiveContents(&archive, &files, command);
      break;
    case kDelete:
      DeleteFiles(&archive, &files, command);
      break;
    case kExtract:
      ExtractFiles(&archive, &files, command);
      break;
    case kSymbols:
      ShowSymbolTable(&archive);
      break;
    case kNone:
      fprintf(stderr, "archivist: no command\n");
      exit(1);
  }
  VectorDestruct(&files);
  StringDestruct(&archive);
}
