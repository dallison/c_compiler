#ifndef aarch64_object_h
#define aarch64_object_h

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "aarch64_assembler.h"
#include "asm_module.h"
#include "compiler.h"
#include "dstring.h"
#include "vector.h"

typedef struct AARCH64ObjectModule {
  AARCH64Assembler assembler;
  AsmModule program;
  String object_file;
  String src_file;
  bool pic;
  bool owns_assembler;
  bool failed;
} AARCH64ObjectModule;

// Starts a module with an empty program that functions can be emitted into
// before the module is opened.
void AARCH64ObjectModuleStart(AARCH64ObjectModule* module, bool keep_text);
// Opens the assembler and puts the preamble ahead of anything the program
// already holds.  On failure the module still needs
// AARCH64ObjectModuleDestruct.
bool AARCH64ObjectModuleOpen(AARCH64ObjectModule* module, String* src_file,
                             String* object_file, bool pic);
bool AARCH64ObjectModuleFinalize(AARCH64ObjectModule* module);
bool AARCH64ObjectModuleWriteAssembly(AARCH64ObjectModule* module, FILE* out);
void AARCH64ObjectModuleDestruct(AARCH64ObjectModule* module);

String* AARCH64EmitObjectFile(Compiler* compiler, Vector* options);
String* AARCH64EmitProgramFile(Compiler* compiler, Vector* options,
                               bool assembly_only);
bool AARCH64StreamFunction(Compiler* compiler, void* code);

#endif /* aarch64_object_h */
