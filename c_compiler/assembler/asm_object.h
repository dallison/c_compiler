//
//  asm_object.h
//  c_compiler
//
//  Target-neutral programmatic assembler object builder: sections, symbols,
//  relocations, byte emission, layout passes, and ELF finalization.
//

#ifndef asm_object_h
#define asm_object_h

#include <stdbool.h>
#include <stdio.h>
#include "buffer.h"
#include "binary_tree.h"
#include "dstring.h"
#include "dwarf.h"
#include "elf.h"
#include "elf_writer.h"
#include "hashtable.h"
#include "vector.h"

struct Assembler;

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
  SYM_BIND(weak),
} AssemblerSymbolBinding;

typedef struct AssemblerSymbol {
  BinaryTreeNode header;
  String name;
  AssemblerSymbolType type;
  AssemblerSymbolBinding binding;
  int64_t value;
  int32_t size;
  bool defined;
  int32_t section;
  int32_t index;
  bool exported;
  bool is_label;
  bool is_forward_declared;
  int32_t alignment;
  bool is_constant;
} AssemblerSymbol;

AssemblerSymbol* NewAssemblerSymbol(const char* name, int32_t section,
                                    AssemblerSymbolType type,
                                    AssemblerSymbolBinding binding,
                                    int64_t value);
void AssemblerSymbolDelete(AssemblerSymbol* sym);

typedef struct AssemblerSection {
  String* name;
  ELFWriterSectionContents contents;
  uint64_t address;
  int32_t flags;
  int32_t type;
  int32_t alignment;
} AssemblerSection;

AssemblerSection* NewAssemblerSection(String* name, int32_t type, int32_t flags,
                                      int32_t alignment);
void AssemblerSectionDestruct(AssemblerSection* section);
void AssemblerSectionDelete(AssemblerSection* section);
void AssemblerSectionAlign(AssemblerSection* section, int alignment);

typedef struct AssemblerRelocation {
  AssemblerSymbol* symbol;
  int32_t type;
  int32_t section;
  int32_t offset;
  int32_t addend;
} AssemblerRelocation;

AssemblerRelocation* NewAssemblerRelocation(AssemblerSymbol* sym, int32_t type,
                                            int32_t section, int32_t offset,
                                            int32_t addend);
void AssemblerRelocationDestruct(AssemblerRelocation* reloc);
void AssemblerRelocationDelete(AssemblerRelocation* reloc);

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

#define ASM_OBJECT_FINAL_PASS 3

typedef enum {
  kAsmOpAlignSection,
  kAsmOpSpace,
  kAsmOpEmitUleb128,
  kAsmOpEmitSleb128,
} AsmOperationKind;

typedef struct AsmOperation {
  AsmOperationKind kind;
  int32_t section;
  union {
    struct {
      int alignment;
    } align;
    struct {
      int64_t size;
      int fill;
    } space;
    struct {
      int64_t value;
    } uleb128;
    struct {
      int64_t value;
    } sleb128;
  } u;
} AsmOperation;

typedef struct AsmObject {
  Vector sections;
  Vector relocations;
  HashTable symbol_table;
  Vector orphan_symbols;
  Vector operations;
  int pass;
  bool allow_layout_pass_skip;
  bool requires_layout_pass;
  int32_t current_section;
  uint16_t elf_machine_type;
  uint16_t elf_flags;
  bool is_64_bit;
  bool is_little_endian;
  int* reloc_types;
  bool pic;
  bool absolute;
  Dwarf dwarf;
  String filename;
  bool recording_operations;
} AsmObject;

void AsmObjectInit(AsmObject* object, int16_t elf_machine_type,
                   uint16_t elf_flags, int* reloc_types);
void AsmObjectDestruct(AsmObject* object);
void AsmObjectReset(AsmObject* object, bool clear_symbols);
void AsmObjectClearSymbols(AsmObject* object);

AssemblerSymbol* AsmObjectFindSymbol(AsmObject* object, const char* name);
void AsmObjectInsertSymbol(AsmObject* object, AssemblerSymbol* sym);
void AsmObjectTrackOrphanSymbol(AsmObject* object, AssemblerSymbol* sym);

// Section creation takes ownership of name.
int AsmObjectAddSection(AsmObject* object, String* name, int32_t type,
                        int32_t flags, int32_t alignment);
int AsmObjectFindSection(AsmObject* object, String* name);
// Ensure takes ownership of name whether or not it creates a section.
int AsmObjectEnsureSection(AsmObject* object, String* name, int32_t type,
                           int32_t flags, int32_t alignment);
void AsmObjectSwitchSection(AsmObject* object, int section);
void AsmObjectSetSectionSize(AsmObject* object, size_t index, size_t size);
void AsmObjectSetCurrentSection(AsmObject* object, int32_t section);
int64_t AsmObjectCurrentAddress(AsmObject* object);

void AsmObjectEmitWord(AsmObject* object, int section, int32_t word);
void AsmObjectEmitByte(AsmObject* object, int section, uint8_t byte);
void AsmObjectEmitHalf(AsmObject* object, int section, uint16_t half);
void AsmObjectEmitLong(AsmObject* object, int section, uint64_t l);
void AsmObjectEmitFill(AsmObject* object, int section, int64_t num_bytes,
                        int fill);
void AsmObjectEmitUleb128(AsmObject* object, int section, uint64_t value);
void AsmObjectEmitSleb128(AsmObject* object, int section, int64_t value);
void AsmObjectAlignCurrentSection(AsmObject* object, int alignment);

void AsmObjectAddRelocation(AsmObject* object, AssemblerRelocation* reloc);
void AsmObjectAddRelocationForSymbol(AsmObject* object, AssemblerSymbol* symbol,
                                     int32_t type, int32_t section,
                                     int32_t offset, int32_t addend);

AssemblerSymbol* AsmObjectDefineLabel(AsmObject* object, struct Assembler* assembler,
                                      String* spelling);

void AsmObjectBeginRecording(AsmObject* object);
void AsmObjectEndRecording(AsmObject* object);
void AsmObjectRecordOperation(AsmObject* object, const AsmOperation* op);
void AsmObjectReplayOperations(AsmObject* object, struct Assembler* assembler);

void AsmObjectWriteELF(AsmObject* object, FILE* out);

#endif /* asm_object_h */
