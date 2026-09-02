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
      String name;
      int32_t type;
      int32_t flags;
      int32_t alignment;
    } section;
    struct {
      String name;
    } label;
    struct {
      String name;
      AssemblerSymbolType type;
      AssemblerSymbolBinding binding;
      int32_t size;
      int32_t alignment;
      bool defined;
      bool exported;
      bool common;
    } symbol;
    struct {
      String name;
      AsmExpr value;
    } symbol_size;
    struct {
      int width;
      AsmExpr value;
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
      AsmExpr value;
    } leb128;
    struct {
      int index;
      String name;
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
} AsmModule;

void AsmModuleInit(AsmModule* module, const AsmModuleTargetOps* target);
void AsmModuleDestruct(AsmModule* module);

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

// Replay the retained operations for the assembler's current pass.
void AsmModuleEmit(AsmModule* module, struct Assembler* assembler);

// Write canonical assembly source. Comments are emitted only by this path.
bool AsmModuleWriteText(const AsmModule* module, FILE* out);

#endif /* asm_module_h */
