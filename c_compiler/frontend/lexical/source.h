//
//  source.h
//  c_compiler
//
//  Created by David Allison on 11/18/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "dstring.h"
#include "vector.h"

#ifndef source_h
#define source_h

// We need to keep track of all locations (filename and line number) in
// the programs being compiled.  This is because we need to report errors
// as accurately as possible
//
// NOTE: Vectors contain a dynamic array of void pointers.  Pointers are
// always big enough to hold an integer, and line numbers are 32 bits in
// length (could be shorter).
typedef struct {
  String name;   // Name of file
  Vector lines;  // All line numbers with code on them.
  bool is_system_header;  // True if any location was created from -isystem.
} File;

uint32_t NewFile(const char* filename);
void FileDestruct(File* file);
void ClearAllFiles(void);

// Encoded file, line and offset information for a location in the
// program.  This is a 64 bit value with the following fields:

// +----------+-------------+------------------+------------------------+
// | start    |  length     |   file index     |    line index          |
// +----------+-------------+------------------+------------------------+
//    16 bits     13 bits       16 bits               20 bits
//    65536        8192         65536                1048576

// The fields are:
//    line index: index into the lines vector inside the File struct.
//    file index: index into all_files vector
//    length: length of token
//    start: index into line of start of token
//
// Recording the location like this allows us to keep track of every token in
// the program being compiled.  This would allow for precise error location
// markers and also open the possibility of using the software as the basis for
// a syntax-based code rewriting (refactoring tools, reformatters, etc.)

typedef uint64_t SourceLocation;

#define LOC_LINE_MASK 0xfffffLL
#define LOC_LINE_LENGTH 20LL
#define LOC_LINE_SHIFT 0LL
#define LOC_FILE_MASK 0xffffLL
#define LOC_FILE_LENGTH 16LL
#define LOC_FILE_SHIFT 20LL
#define LOC_LENGTH_MASK 0x1fffLL
#define LOC_LENGTH_LENGTH 13LL
#define LOC_LENGTH_SHIFT (LOC_LINE_LENGTH + LOC_FILE_LENGTH)
#define LOC_START_MASK 0xffffLL
#define LOC_START_SHIFT (LOC_LENGTH_SHIFT + LOC_LENGTH_LENGTH)

#define MAX_TOKEN_POS LOC_START_MASK
#define MAX_TOKEN_LENGTH LOC_LENGTH_MASK
#define MAX_LINE_INDEX (LOC_LINE_MASK - 2)  // -2 for special meanings.
#define MAX_FILE_INDEX LOC_FILE_MASK

typedef enum {
  kSourceFromFile,    // Input comes from a file.
  kSourceFromString,  // Input comes from a string.
} SourceDevice;

// A Source provides the source code from somewhere.  It can be
// from a file or from a string.
typedef struct Source {
  String filename;      // Filename if from disk.
  String original;      // The original filename, if changed.
  int lineno;           // Line number (starting at 1).
  uint32_t file_index;  // Index of file.
  struct Source* prev;  // Previous Source in stack.
  SourceDevice device;  // Where the input comes from.
  bool at_start;        // No source bytes have been consumed yet.
  bool reached_eof;     // A read has reached the end of this source.
  union {
    FILE* file;  // File to read, if from file.
    struct {
      String* string;  // String to read from, if from memory.
      size_t index;    // Current location in string.
    } string;
  } from;
  size_t path_index;  // Index into search path.
  bool is_system_header;  // Set by -isystem resolution or system_header pragma.
} Source;

SourceLocation NewSourceLocation(Source* sou, int lineno, size_t start,
                                 size_t end);
void DecodeSourceLocation(SourceLocation location, const char** filename,
                          int* lineno, int* start, int* end);
// True when `location` was recorded in `filename` (exact or directory suffix).
bool SourceLocationIsFile(SourceLocation location, const char* filename);

void SourceLocationNumbers(SourceLocation location, int* fileno, int* lineno,
                           int* colno);

#define SOURCE_LOCATION_MISSING ((SourceLocation)0xfffffffffffffffeLL)
#define SOURCE_LOCATION_COMMAND_LINE ((SourceLocation)0xffffffffffffffffLL)

// Creates a new source in heap memory.
Source* NewSourceFromFile(const char* filename, FILE* in);
Source* NewSourceFromString(const char* filenname, String* str);
void SourceMarkSystemHeader(Source* source);
bool SourceIsSystemHeader(const Source* source);
bool SourceLocationIsSystemHeader(SourceLocation location);
void SourceRewind(Source* src);

int SourceGetChar(Source* src);
void SourceReadLine(Source* src, String* line);

// Destroys a source but does not free the memory.
void SourceDestruct(Source* src);
void SourceDelete(Source* source);

// Is the source at end of file?
bool SourceEof(Source* ctx);

void SourcePrintLocation(SourceLocation location);
void SourceTraverseFiles(void* data,
                         void (*func)(int index, File* file, void* data));

#endif /* source_h */
