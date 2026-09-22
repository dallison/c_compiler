//
//  assembler.h
//  c_compiler
//
//  Created by David Allison on 1/1/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

// This is a general assembler.  It is intended that instruction-set specific
// assemblers be derived from this.  This handles things are common to all
// assemblers, such as directives, symbols, sections, debug output, etc.
//
// It generates ELF64 output from a text input file.

#ifndef assembler_h
#define assembler_h

#include <stdarg.h>
#include <stdio.h>
#include "asm_object.h"
#include "buffer.h"
#include "dstring.h"
#include "hashtable.h"
#include "lex.h"
#include "map.h"
#include "preprocessor.h"
#include "syntax.h"
#include "binary_tree.h"
#include "vector.h"

typedef struct Assembler {
  AsmObject object;
  Preprocessor preprocessor;
  Lex lex;
  Syntax syntax;
  FILE* out;
  Map directives;
  int num_errors;
  bool parsing_layout_expression;
  AssemblerSymbol* (*define_label)(struct Assembler*, String*);
  // GNU `N:` / `Nb` / `Nf` local labels.  Counts reset each assembly pass.
  int* local_label_count;
  size_t local_label_cap;
} Assembler;

struct AsmModule;

typedef void (*AssemblerRecordedEmitter)(Assembler* assembler, void* context);

typedef enum {
  kAssemblerRecordedText,
  kAssemblerRecordedEmitter,
} AssemblerRecordedInputKind;

typedef struct {
  AssemblerRecordedInputKind kind;
  const char* name;
  union {
    String* text;
    struct {
      AssemblerRecordedEmitter emit;
      void* context;
    } emitter;
  } value;
} AssemblerRecordedInput;

static inline AsmObject* AssemblerGetObject(Assembler* assembler) {
  return &assembler->object;
}

bool AssemblerInit(Assembler* assembler, int16_t elf_machine_type,
                   uint16_t elf_flags, int* reloc_types, String* infile,
                   String* outfile);
bool AssemblerInitFromString(Assembler* assembler, int16_t elf_machine_type,
                             uint16_t elf_flags, int* reloc_types,
                             const char* name, String* input,
                             String* outfile);
void AssemblerDestruct(Assembler* assembler);
AssemblerSymbol* AssemblerFindSymbol(Assembler* assembler, const char* name);
void AssemblerInsertSymbol(Assembler* assembler, AssemblerSymbol* sym);
void AssemblerTrackOrphanSymbol(Assembler* assembler, AssemblerSymbol* sym);
void AssemblerReset(Assembler* assembler, bool clear_symbols);
void AssemblerClearSymbols(Assembler* assembler);

int AssemblerAddSection(Assembler* assembler, String* name, int32_t type,
                        int32_t flags, int32_t alignment);
int AssemblerFindSection(Assembler* assembler, String* name);
void AssemblerSetSectionSize(Assembler* assembler, size_t index, size_t size);

void AssemblerAddRelocation(Assembler* assembler, AssemblerRelocation* reloc);
void AssemblerAddRelocationForSymbol(Assembler* assembler,
                                     AssemblerSymbol* symbol, int32_t type,
                                     int32_t section, int32_t offset,
                                     int32_t addend);

void AssemblerRun(Assembler* assembler, void (*run_func)(Assembler*, String*));
void AssemblerAssembleInput(Assembler* assembler, const char* name, String* input,
                            void (*run_func)(Assembler*, String*));
void AssemblerRunRecordedOperations(Assembler* assembler, Vector* inputs,
                                    void (*run_func)(Assembler*, String*));
void AssemblerRunModule(Assembler* assembler, struct AsmModule* module);
int AssemblerRelocTypeForWord(Assembler* assembler);

void AssemblerEmitWord(Assembler* assembler, int section, int32_t word);
void AssemblerEmitByte(Assembler* assembler, int section, uint8_t byte);
void AssemblerEmitHalf(Assembler* assembler, int section, uint16_t half);
void AssemblerEmitLong(Assembler* assembler, int section, uint64_t l);
int64_t AssemblerEvaluateKnownExpression(Assembler* assembler, bool* known);
int64_t AssemblerEvaluateExpression(Assembler* assembler);
double AssemblerGetDoubleConst(Assembler* assembler);

void AssemblerError(Assembler* assembler, const char* format, ...);
void AssemblerErrorAtLocation(Assembler* assembler, SourceLocation location,
                              const char* format, ...);
void AssemblerWarning(Assembler* assembler, const char* warn,
                      const char* format, ...);

int64_t AssemblerCurrentAddress(Assembler* assembler);
void AssemblerExtractSymbolSuffix(String* symbol, String* name, String* suffix);

void AssemblerNoteNumericLocalLabel(Assembler* assembler, int number);
AssemblerSymbol* AssemblerLookupNumericLocalLabel(Assembler* assembler,
                                                 int number, bool backward);

#endif /* assembler_h */
