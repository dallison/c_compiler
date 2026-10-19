//
//  asm_module.h
//  c_compiler
//
//  Target-neutral retained assembly operation stream.
//

#ifndef asm_module_h
#define asm_module_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "asm_object.h"
#include "dstring.h"
#include "vector.h"

struct Assembler;

typedef enum {
  kAsmExprConstant,
  kAsmExprSymbol,
  kAsmExprSymbolDifference,
} AsmExprKind;

typedef struct {
  AsmExprKind kind;
  int64_t addend;
  bool force_relocation;
  String symbol;
  String subtract_symbol;
} AsmExpr;

void AsmExprInitConstant(AsmExpr* expr, int64_t value);
void AsmExprInitSymbol(AsmExpr* expr, const char* symbol, int64_t addend);
void AsmExprInitDifference(AsmExpr* expr, const char* symbol,
                           const char* subtract_symbol, int64_t addend);
void AsmExprForceRelocation(AsmExpr* expr);
void AsmExprDestruct(AsmExpr* expr);

// An AsmExpr as retained by a module: the symbol names are interned in the
// module's name pool rather than owned.
typedef struct {
  AsmExprKind kind;
  bool force_relocation;
  int64_t addend;
  const char* symbol;
  const char* subtract_symbol;
} AsmModuleExpr;

// Set of the distinct names a module refers to.  Mangled names are long and
// each is referenced from many operations, so they are stored once.
typedef struct {
  const char** slots;  // Open-addressing table, `capacity` entries.
  size_t capacity;     // Zero or a power of two.
  size_t count;
  Vector blocks;       // Storage the names are carved from.
  char* free_space;
  size_t free_length;
} AsmNamePool;

typedef struct AsmModuleTargetOps {
  void (*destroy_instruction)(void* instruction);
  void (*emit_instruction)(struct Assembler* assembler,
                           const void* instruction);
  bool (*write_instruction)(FILE* out, const void* instruction);
  void (*assemble_text)(struct Assembler* assembler, String* mnemonic);
} AsmModuleTargetOps;

typedef enum {
  kAsmModuleOpSection,
  kAsmModuleOpLabel,
  kAsmModuleOpSymbol,
  kAsmModuleOpSymbolSize,
  kAsmModuleOpInteger,
  kAsmModuleOpBytes,
  kAsmModuleOpString,
  kAsmModuleOpFill,
  kAsmModuleOpAlign,
  kAsmModuleOpUleb128,
  kAsmModuleOpSleb128,
  kAsmModuleOpFile,
  kAsmModuleOpLocation,
  kAsmModuleOpComment,
  kAsmModuleOpInstruction,
  kAsmModuleOpText,
} AsmModuleOpKind;

typedef struct {
  AsmModuleOpKind kind;
  union {
    struct {
      const char* name;
      int32_t type;
      int32_t flags;
      int32_t alignment;
    } section;
    struct {
      const char* name;
    } label;
    struct {
      const char* name;
      AssemblerSymbolType type;
      AssemblerSymbolBinding binding;
      int32_t size;
      int32_t alignment;
      bool defined;
      bool exported;
      bool common;
    } symbol;
    struct {
      const char* name;
      AsmModuleExpr value;
    } symbol_size;
    struct {
      int width;
      AsmModuleExpr value;
    } integer;
    struct {
      uint8_t* value;
      size_t length;
    } bytes;
    struct {
      String value;
      bool zero_terminate;
    } string;
    struct {
      int64_t size;
      int fill;
    } fill;
    struct {
      int alignment;
    } align;
    struct {
      AsmModuleExpr value;
    } leb128;
    struct {
      int index;
      const char* name;
    } file;
    struct {
      int file;
      int line;
      int column;
    } location;
    struct {
      String text;
    } comment;
    struct {
      void* value;
    } instruction;
    struct {
      String name;
      String value;
    } text;
  } u;
} AsmModuleOp;

typedef struct AsmModule {
  Vector operations;
  const AsmModuleTargetOps* target;
  bool failed;
  // False when the module is only assembled and never written as text; then
  // comments and the symbolic spellings of instructions are not recorded.
  bool keep_text;
  AsmNamePool names;
} AsmModule;

void AsmModuleInit(AsmModule* module, const AsmModuleTargetOps* target);
void AsmModuleDestruct(AsmModule* module);

// Returns the module's copy of `name`, which lives until AsmModuleDestruct.
// Equal names return the same pointer.  Returns NULL if out of memory.
const char* AsmModuleInternName(AsmModule* module, const char* name);

// Looks up a name returned by AsmModuleInternName in `assembler`'s symbol
// table.  The name remembers the symbol it found until the table is cleared.
AssemblerSymbol* AsmModuleFindSymbol(struct Assembler* assembler,
                                     const char* interned_name);

void AsmModuleSection(AsmModule* module, const char* name, int32_t type,
                      int32_t flags, int32_t alignment);
void AsmModuleLabel(AsmModule* module, const char* name);
void AsmModuleSymbol(AsmModule* module, const char* name,
                     AssemblerSymbolType type, AssemblerSymbolBinding binding,
                     int32_t size, int32_t alignment, bool defined,
                     bool exported, bool common);
void AsmModuleSymbolSize(AsmModule* module, const char* name,
                         const AsmExpr* value);
void AsmModuleInteger(AsmModule* module, int width, const AsmExpr* value);
void AsmModuleBytes(AsmModule* module, const void* bytes, size_t length);
void AsmModuleString(AsmModule* module, const char* value, bool zero_terminate);
void AsmModuleFill(AsmModule* module, int64_t size, int fill);
void AsmModuleAlign(AsmModule* module, int alignment);
void AsmModuleUleb128(AsmModule* module, const AsmExpr* value);
void AsmModuleSleb128(AsmModule* module, const AsmExpr* value);
void AsmModuleFile(AsmModule* module, int index, const char* name);
void AsmModuleLocation(AsmModule* module, int file, int line, int column);
void AsmModuleComment(AsmModule* module, const char* text);
void AsmModuleInstruction(AsmModule* module, void* instruction);
void AsmModuleText(AsmModule* module, const char* name, const char* text,
                   size_t length);

// Replay the retained operations for the assembler's current pass.  Symbols
// it creates borrow the module's interned names, so `module` must outlive
// `assembler`'s symbols.
void AsmModuleEmit(AsmModule* module, struct Assembler* assembler);

// Write canonical assembly source. Comments are emitted only by this path.
bool AsmModuleWriteText(const AsmModule* module, FILE* out);

#endif /* asm_module_h */
