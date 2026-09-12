//
//  source.c
//  c_compiler
//
//  Created by David Allison on 11/18/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "source.h"

#include <stdlib.h>
#include <string.h>
#include "map.h"

static Vector all_files;
static Map file_map;
static bool file_map_initialized = false;

uint32_t NewFile(const char* filename) {
  File* file = malloc(sizeof(File));
  StringInit(&file->name, filename);
  VectorInit(&file->lines);
  file->is_system_header = false;
  size_t curr_length = all_files.length;
  VectorAppend(&all_files, file);
  return (uint32_t)curr_length;
}

// The file_map is a mapping from filename to file index + 1.  We can't
// store the value 0 in there because MapFind returns a NULL for the
// key not found, so we add 1 to the index.
static uint32_t FindFile(const char* filename) {
  if (!file_map_initialized) {
    // Lazy init of file_map.
    MapInitForCharPointerKeys(&file_map);
    file_map_initialized = true;
  }
  // Find the index (+1) in the file map corresponding to the filename.
  void* index = MapFindPointerKey(&file_map, (void*)filename);
  if (index != NULL) {
    return (uint32_t)(((uint64_t)index) - 1);
  }

  // No found, create a new file in the all_files vector and add its
  // index (+1) into the file_map.
  uint32_t file_index = NewFile(filename);
  File* file = all_files.value.p[file_index];
  MapKeyValue kv;
  kv.key.p = file->name.value;
  kv.value.w = file_index + 1;
  MapInsert(&file_map, kv);
  return file_index;
}

void FileDestruct(File* file) {
  StringDestruct(&file->name);
  VectorDestruct(&file->lines);
}

void ClearAllFiles() {
  for (size_t i = 0; i < all_files.length; i++) {
    FileDestruct((File*)all_files.value.p[i]);
    free(all_files.value.p[i]);
  }
  VectorDestruct(&all_files);
  // The file_map keys are borrowed pointers into the File names just freed, so
  // only the map's own storage is released here.
  if (file_map_initialized) {
    MapDestruct(&file_map);
    file_map_initialized = false;
  }
}

static void ResetFiles() {
  for (size_t i = 0; i < all_files.length; i++) {
    FileDestruct((File*)all_files.value.p[i]);
    free(all_files.value.p[i]);
  }
  VectorClear(&all_files);
  // Empty (but keep) the file map so its backing storage is reused.  Leaving
  // file_map_initialized set avoids a re-init that would null the values
  // pointer and orphan the existing allocation.
  MapClear(&file_map);
}

SourceLocation NewSourceLocation(Source* source, int lineno, size_t start,
                                 size_t end) {
  if (source->file_index == -1) {
    source->file_index = FindFile(source->filename.value);
  }
  uint32_t file_index = source->file_index;
  if (start > MAX_TOKEN_POS || file_index > MAX_FILE_INDEX) {
    return SOURCE_LOCATION_MISSING;
  }
  File* file = (File*)all_files.value.p[file_index];
  if (source->is_system_header) {
    file->is_system_header = true;
  }
  size_t line_index = file->lines.length;     // One greater than index.
  int64_t length = end - start;
  if (line_index > MAX_LINE_INDEX || length > MAX_TOKEN_LENGTH) {
    return SOURCE_LOCATION_MISSING;
  }
  // Only append to vector if line number has changed.
  if (file->lines.length == 0 ||
      file->lines.value.w[file->lines.length-1] != lineno) {
    VectorAppend(&file->lines, (void*)((int64_t)lineno));
  } else {
    line_index--;     // One too far since we didn't append to vector.
  }
  SourceLocation r = ((int64_t)file_index << LOC_FILE_SHIFT) |
         ((int64_t)line_index << LOC_LINE_SHIFT) |
         ((int64_t)start << LOC_START_SHIFT) | (length << LOC_LENGTH_SHIFT);
  return r;
}

void DecodeSourceLocation(SourceLocation location, const char** filename,
                          int* lineno, int* start, int* end) {
  *end = *start = *lineno = 0;
  if (location == SOURCE_LOCATION_COMMAND_LINE) {
    *filename = "command-line";
    return;
  }
  if (location == SOURCE_LOCATION_MISSING) {
    *filename = "<unknown>";
    return;
  }
  uint32_t file_index = (location >> LOC_FILE_SHIFT) & LOC_FILE_MASK;
  uint32_t line_index = (location >> LOC_LINE_SHIFT) & LOC_LINE_MASK;
  uint32_t length = (location >> LOC_LENGTH_SHIFT) & LOC_LENGTH_MASK;
  *start = (location >> LOC_START_SHIFT) & LOC_START_MASK;
  if (file_index < all_files.length) {
    File* file = (File*)all_files.value.p[file_index];
    *filename = file->name.value;
    if (line_index < file->lines.length) {
      *lineno = (int)file->lines.value.w[line_index];
    }
    *end = *start + length;
  } else {
    *filename = "<unknown>";
  }
}

bool SourceLocationIsFile(SourceLocation location, const char* filename) {
  if (filename == NULL || location == SOURCE_LOCATION_COMMAND_LINE ||
      location == SOURCE_LOCATION_MISSING) {
    return false;
  }
  const char* loc_file = NULL;
  int lineno = 0;
  int start = 0;
  int end = 0;
  DecodeSourceLocation(location, &loc_file, &lineno, &start, &end);
  if (loc_file == NULL) {
    return false;
  }
  if (strcmp(loc_file, filename) == 0) {
    return true;
  }
  size_t loc_len = strlen(loc_file);
  size_t file_len = strlen(filename);
  if (loc_len > file_len && loc_file[loc_len - file_len - 1] == '/' &&
      strcmp(loc_file + loc_len - file_len, filename) == 0) {
    return true;
  }
  if (file_len > loc_len && filename[file_len - loc_len - 1] == '/' &&
      strcmp(filename + file_len - loc_len, loc_file) == 0) {
    return true;
  }
  return false;
}

void SourceLocationNumbers(SourceLocation location, int* fileno, int* lineno,
                           int* colno) {
  *fileno = *lineno = *colno = 0;
  if (location == SOURCE_LOCATION_COMMAND_LINE ||
      location == SOURCE_LOCATION_MISSING) {
    return;
  }
  uint32_t file_index = (location >> LOC_FILE_SHIFT) & LOC_FILE_MASK;
  uint32_t line_index = (location >> LOC_LINE_SHIFT) & LOC_LINE_MASK;
  *colno = (location >> LOC_START_SHIFT) & LOC_START_MASK;
  if (file_index < all_files.length) {
    File* file = (File*)all_files.value.p[file_index];
    if (line_index < file->lines.length) {
      *lineno = (int)file->lines.value.w[line_index];
    }
    *fileno = file_index;
  }
}

Source* NewSourceFromFile(const char* filename, FILE* in) {
  Source* src = malloc(sizeof(Source));
  StringInit(&src->filename, filename);
  StringInit(&src->original, filename);
  src->lineno = 0;
  src->from.file = in;
  src->device = kSourceFromFile;
  src->at_start = true;
  src->reached_eof = false;
  src->file_index = -1;
  src->prev = NULL;
  src->path_index = 0;
  src->is_system_header = false;
  return src;
}

Source* NewSourceFromString(const char* filename, String* str) {
  Source* src = malloc(sizeof(Source));
  StringInit(&src->filename, filename);
  StringInit(&src->original, filename);
  src->lineno = 0;
  src->from.string.string = str;
  src->from.string.index = 0;
  src->device = kSourceFromString;
  src->at_start = true;
  src->reached_eof = false;
  src->file_index = -1;
  src->prev = NULL;
  src->path_index = 0;
  src->is_system_header = false;
  return src;
}

void SourceMarkSystemHeader(Source* source) {
  if (source != NULL) {
    source->is_system_header = true;
  }
}

bool SourceIsSystemHeader(const Source* source) {
  return source != NULL && source->is_system_header;
}

bool SourceLocationIsSystemHeader(SourceLocation location) {
  if (location == SOURCE_LOCATION_COMMAND_LINE ||
      location == SOURCE_LOCATION_MISSING) {
    return false;
  }
  uint32_t file_index = (location >> LOC_FILE_SHIFT) & LOC_FILE_MASK;
  if (file_index >= all_files.length) {
    return false;
  }
  File* file = (File*)all_files.value.p[file_index];
  return file != NULL && file->is_system_header;
}

void SourceDestruct(Source* src) {
  switch (src->device) {
    case kSourceFromFile:
      if (src->from.file != NULL) {
        fclose(src->from.file);
      }
      break;
    case kSourceFromString:
      StringDestruct(src->from.string.string);
      free(src->from.string.string);
      break;
  }
  StringDestruct(&src->filename);
  StringDestruct(&src->original);
}

void SourceDelete(Source* src) {
  SourceDestruct(src);
  free(src);
}

void SourceRewind(Source* src) {
  switch (src->device) {
    case kSourceFromFile:
      if (src->from.file != NULL) {
        rewind(src->from.file);
      }
      break;
    case kSourceFromString:
      src->from.string.index = 0;
      break;
  }
  src->lineno = 0;
  src->at_start = true;
  src->reached_eof = false;
  src->file_index = -1;
}

void SourceResetFiles(Source* src) {
  src->file_index = -1;
  ResetFiles();
}

// Has end of file been reached?
bool SourceEof(Source* src) {
  switch (src->device) {
    case kSourceFromFile:
      return src->from.file == NULL || src->reached_eof;
    case kSourceFromString:
      // From a string, check current index against length.
      return src->from.string.index > src->from.string.string->length;
  }
}

int SourceGetChar(Source* src) {
  switch (src->device) {
    case kSourceFromFile:
      if (src->from.file == NULL) {
        return EOF;
      }
      // Sources are consumed by one compiler thread, so avoid stdio's lock on
      // every byte. The FILE still provides block buffering underneath.
      int ch = getc_unlocked(src->from.file);
      if (ch == EOF) {
        src->reached_eof = true;
      }
      return ch;
      break;
    case kSourceFromString:
      if (src->from.string.index > src->from.string.string->length) {
        return EOF;
      }
      return src->from.string.string->value[src->from.string.index++];
      break;
  }
}


// Read a line from the source into the 'line'.  This replaces trigraphs and
// appends lines ending in backslash.
void SourceReadLine(Source* src, String* line) {
  while (!SourceEof(src)) {
    String newline = {0};
    for (;;) {
      int ch = SourceGetChar(src);
      if (ch == EOF) {
        break;
      }
      if (ch == '\n') {
        break;
      }
      StringAppendChar(&newline, ch);
    }
    src->lineno++;

    size_t start = 0;
    if (src->at_start && newline.length >= 3 &&
        (unsigned char)newline.value[0] == 0xef &&
        (unsigned char)newline.value[1] == 0xbb &&
        (unsigned char)newline.value[2] == 0xbf) {
      start = 3;
    }
    src->at_start = false;

    // Replace trigraphs in newline, generating line.
    for (size_t i = start; i < newline.length; i++) {
      char c = newline.value[i];

      if (c == '?' && i < newline.length - 2 && newline.value[i + 1] == '?') {
        switch (newline.value[i + 2]) {
          case '=':
            c = '#';
            i += 2;
            break;
          case '/':
            c = '\\';
            i += 2;
            break;
          case '\'':
            c = '^';
            i += 2;
            break;
          case '(':
            c = '[';
            i += 2;
            break;
          case ')':
            c = ']';
            i += 2;
            break;
          case '!':
            c = '|';
            i += 2;
            break;
          case '<':
            c = '{';
            i += 2;
            break;
          case '>':
            c = '}';
            i += 2;
            break;
          case '-':
            c = '~';
            i += 2;
            break;
        }
      }

      StringAppendChar(line, c);  // Add char to line.
    }

    // Translation phase 2 deletes a trailing backslash, any intervening
    // non-newline whitespace, and the newline.  Deletion is significant:
    // replacing the splice with a space prevents tokens and universal
    // character names from being formed across physical source lines.
    if (line->length > 0) {
      size_t splice = line->length;
      while (splice > 0 &&
             (line->value[splice - 1] == ' ' ||
              line->value[splice - 1] == '\t' ||
              line->value[splice - 1] == '\v' ||
              line->value[splice - 1] == '\f' ||
              line->value[splice - 1] == '\r')) {
        splice--;
      }
      if (splice > 0 && line->value[splice - 1] == '\\') {
        StringErase(line, splice - 1, line->length - (splice - 1));
        StringDestruct(&newline);
        continue;
      }
    }
    StringDestruct(&newline);
    break;
  }
}

void SourcePrintLocation(SourceLocation location) {
  const char* filename;
  int lineno;
  int start;
  int end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  printf("%s:%d\n", filename, lineno);
}

void SourceTraverseFiles(void* data,
                         void (*func)(int index, File* file, void* data)) {
  for (size_t i = 0; i < all_files.length; i++) {
    func((int)i, all_files.value.p[i], data);
  }
}

size_t SourceFileCount(void) { return all_files.length; }

File* SourceFileAt(size_t index) {
  if (index >= all_files.length) {
    return NULL;
  }
  return (File*)all_files.value.p[index];
}

uint32_t SourceImportFile(const char* filename, bool is_system_header,
                          const int64_t* lines, size_t nlines) {
  File* file = malloc(sizeof(File));
  StringInit(&file->name, filename != NULL ? filename : "");
  VectorInit(&file->lines);
  file->is_system_header = is_system_header;
  for (size_t i = 0; i < nlines; i++) {
    VectorAppend(&file->lines, (void*)(intptr_t)lines[i]);
  }
  uint32_t index = (uint32_t)all_files.length;
  VectorAppend(&all_files, file);
  return index;
}

SourceLocation SourceRemapLocationFile(SourceLocation location,
                                       uint32_t new_file_index) {
  if (location == SOURCE_LOCATION_COMMAND_LINE ||
      location == SOURCE_LOCATION_MISSING) {
    return location;
  }
  uint32_t line_index = (location >> LOC_LINE_SHIFT) & LOC_LINE_MASK;
  uint32_t length = (location >> LOC_LENGTH_SHIFT) & LOC_LENGTH_MASK;
  uint32_t start = (location >> LOC_START_SHIFT) & LOC_START_MASK;
  return ((SourceLocation)new_file_index << LOC_FILE_SHIFT) |
         ((SourceLocation)line_index << LOC_LINE_SHIFT) |
         ((SourceLocation)start << LOC_START_SHIFT) |
         ((SourceLocation)length << LOC_LENGTH_SHIFT);
}
