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

bool AARCH64ObjectModuleInit(AARCH64ObjectModule* module, String* src_file,
                             String* object_file, bool pic);
bool AARCH64ObjectModuleFinalize(AARCH64ObjectModule* module);
bool AARCH64ObjectModuleWriteAssembly(AARCH64ObjectModule* module, FILE* out);
void AARCH64ObjectModuleDestruct(AARCH64ObjectModule* module);

String* AARCH64EmitObjectFile(Compiler* compiler, Vector* options);
String* AARCH64EmitProgramFile(Compiler* compiler, Vector* options,
                               bool assembly_only);

#endif /* aarch64_object_h */
