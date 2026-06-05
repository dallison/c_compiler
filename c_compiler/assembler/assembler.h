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
#include "buffer.h"
#include "dstring.h"
#include "dwarf.h"
#include "elf.h"
#include "elf_writer.h"
#include "hashtable.h"
#include "lex.h"
#include "map.h"
#include "preprocessor.h"
#include "syntax.h"
#include "binary_tree.h"

typedef struct {
  int32_t inst_address;
  void* label;
} AssemblerFixup;

#define SYM_TYPE(type) kAssemblerSymbolType_##type
typedef enum {
  SYM_TYPE(none),
  SYM_TYPE(func),
  SYM_TYPE(object),
  SYM_TYPE(common),
  SYM_TYPE(tls),
} AssemblerSymbolType;

#define SYM_BIND(b) kAssemblerSymbolBinding_##b
typedef enum {
  SYM_BIND(global),
  SYM_BIND(local),
} AssemblerSymbolBinding;

typedef struct AssemblerSymbol {
  BinaryTreeNode header;
  String name;
  AssemblerSymbolType type;
  AssemblerSymbolBinding binding;
  int64_t value;
  int32_t size;
  bool defined;
  int32_t section;  // Section number.
  int32_t index;    // Symbol index.
  bool exported;    // Symbol is to be exported to object file.
  bool is_label;    // Is a section-local label.
  bool is_forward_declared;
  int32_t alignment;
  bool is_constant;   // Not subject to relocation.
} AssemblerSymbol;

AssemblerSymbol* NewAssemblerSymbol(const char* name, int32_t section,
                                    AssemblerSymbolType type,
                                    AssemblerSymbolBinding binding,
                                    int64_t value);
void AssemblerSymbolDelete(AssemblerSymbol* sym);

// A section is a named data buffer.  The code and data are in separate
// sections.
typedef struct AssemblerSection {
  String* name;
  ELFWriterSectionContents contents;
  uint64_t address;
  int32_t flags;
  int32_t type;
  int32_t alignment;  // Alignment for section (power of 2).
} AssemblerSection;

// This takes ownership of the name string.
AssemblerSection* NewAssemblerSection(String* name, int32_t type, int32_t flags,
                                      int32_t alignment);
void AssemblerSectionDestruct(AssemblerSection* section);
void AssemblerSectionDelete(AssemblerSection* section);
void AssemblerSectionAlign(AssemblerSection* section, int alignment);

typedef struct AssemblerRelocation {
  AssemblerSymbol* symbol;  // Symbol to use to relocate.
  int32_t type;             // Relocation type.
  int32_t section;          // Section index that it is applied to.
  int32_t offset;           // Offset into section.
  int32_t addend;           // Value to add to symbol.
} AssemblerRelocation;

AssemblerRelocation* NewAssemblerRelocation(AssemblerSymbol* sym, int32_t type,
                                            int32_t section, int32_t offset, int32_t addend);
void AssemblerRelocationDestruct(AssemblerRelocation* reloc);
void AssemblerRelocationDelete(AssemblerRelocation* reloc);

// Types for relocations set by the target architecture.  They are used
// as indexes into an array of integers containing the actual relocation
// values, set by the architecture.
typedef enum {
  kRelocSet16,
  kRelocSet32,
  kRelocSet64,
  kRelocAdd16,
  kRelocAdd32,
  kRelocAdd64,
  kRelocSub16,
  kRelocSub32,
  kRelocSub64,
  kNumRelocTypes,
} RelocationType;

typedef struct Assembler {
  Preprocessor preprocessor;
  Lex lex;                    // Lexical analyzer.
  Syntax syntax;              // Syntax analyzer.
  String filename;            // Input filename.
  FILE* out;                  // Output file (open for write).
  Map directives;             // Assembler directives.
  Vector sections;            // Vector of AssemblerSection*.
  Vector relocations;         // Vector of AssemberRelocation*
  HashTable symbol_table;     // Symbol table.
  int pass;                   // Pass number (1 or 2).
  int num_errors;             // Number of errors.
  int32_t current_section;    // Current section index.
  uint16_t elf_machine_type;  // ELF machine.
  uint16_t elf_flags;         // ELF flags.
  int* reloc_types;           // Relocation types.
  bool pic;                   // Position Independent Code.
  Dwarf dwarf;                // Debugging information.
  bool absolute;              // All symbols are absolute.
  // Function to define a label.  This can be overridden by architecture
  // specific assemblers to handle branches and labels.
  AssemblerSymbol* (*define_label)(struct Assembler*, String*);
} Assembler;

bool AssemblerInit(Assembler* assembler, int16_t elf_machine_type,
                   uint16_t elf_flags, int* reloc_types, String* infile,
                   String* outfile);
void AssemblerDestruct(Assembler* assembler);
AssemblerSymbol* AssemblerFindSymbol(Assembler* assembler, const char* name);
void AssemblerInsertSymbol(Assembler* assembler, AssemblerSymbol* sym);
void AssemblerReset(Assembler* assembler, bool clear_symbols);
void AssemblerClearSymbols(Assembler* assembler);

int AssemblerAddSection(Assembler* assembler, String* name, int32_t type,
                        int32_t flags, int32_t alignment);
int AssemblerFindSection(Assembler* assembler, String* name);
void AssemblerSetSectionSize(Assembler* assembler, size_t index, size_t size);

void AssemblerAddRelocation(Assembler* assembler, AssemblerRelocation* reloc);

void AssemblerRun(Assembler* assembler, void (*run_func)(Assembler*, String*));

void AssemblerEmitWord(Assembler* assembler, int section, int32_t word);
void AssemblerEmitByte(Assembler* assembler, int section, uint8_t byte);
void AssemblerEmitHalf(Assembler* assembler, int section, uint16_t half);
void AssemblerEmitLong(Assembler* assembler, int section, uint64_t l);
int64_t AssemblerEvaluateKnownExpression(Assembler* assembler, bool* known);
int64_t AssemblerEvaluateExpression(Assembler* assembler);
double AssemblerGetDoubleConst(Assembler* assembler);

void AssemblerError(Assembler* assembler, const char* format, ...);
void AssemblerErrorAtLocation(Assembler* assembler, SourceLocation location, const char* format, ...);
void AssemblerWarning(Assembler* assembler, const char* warn,
                      const char* format, ...);

int64_t AssemblerCurrentAddress(Assembler* assembler);
void AssemblerExtractSymbolSuffix(String* symbol, String* name, String* suffix);

#endif /* assembler_h */
