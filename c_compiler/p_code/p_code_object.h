#ifndef P_CODE_OBJECT_H
#define P_CODE_OBJECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dstring.h"
#include "p_code_codegen.h"
#include "vector.h"

typedef enum {
  kPCodeObjectText,
  kPCodeObjectROData,
  kPCodeObjectExceptionTable,
} PCodeObjectSection;

typedef enum {
  kPCodeFixupBranch,
  kPCodeFixupAbsolute,
  kPCodeFixupAddress,
  kPCodeFixupPCRelative,
  kPCodeFixupCall,
  kPCodeFixupJump,
  kPCodeFixupData64,
  kPCodeFixupData32,
} PCodeFixupKind;

typedef struct {
  String name;
  PCodeObjectSection section;
  size_t offset;
  bool defined;
  bool weak;
} PCodeObjectSymbol;

typedef struct {
  String symbol_name;
  PCodeObjectSection section;
  size_t offset;
  int64_t addend;
  PCodeFixupKind kind;
} PCodeObjectFixup;

typedef struct {
  String text;
  String rodata;
  String exception_table;
  Vector symbols;
  Vector fixups;
} PCodeObject;

void PCodeObjectInit(PCodeObject* object);
void PCodeObjectDestruct(PCodeObject* object);
bool PCodeObjectCopy(PCodeObject* dest, const PCodeObject* source);
bool PCodeObjectAppend(PCodeObject* dest, const PCodeObject* source,
                       const char** reason);
bool PCodeObjectBuildFunction(PCodeObject* object, PCodeGenerator* pcode,
                              const char** reason);
PCodeObjectSymbol* PCodeObjectFindSymbol(const PCodeObject* object,
                                         const char* name);
String* PCodeObjectSectionData(PCodeObject* object,
                               PCodeObjectSection section);
const String* PCodeObjectSectionDataConst(const PCodeObject* object,
                                          PCodeObjectSection section);

#endif
