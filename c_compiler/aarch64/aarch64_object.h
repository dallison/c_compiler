#ifndef aarch64_object_h
#define aarch64_object_h

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "aarch64_assembler.h"
#include "compiler.h"
#include "dstring.h"
#include "vector.h"

struct AARCH64ObjectModule {
  AARCH64Assembler assembler;
  Vector chunks;
  char* pending_ptr;
  size_t pending_size;
  FILE* pending_stream;
  String object_file;
  String src_file;
  bool pic;
  int fragment_id;
  bool owns_assembler;
  bool failed;
};

bool AARCH64ObjectModuleInit(AARCH64ObjectModule* module, String* src_file,
                             String* object_file, bool pic);
FILE* AARCH64ObjectModuleAssemblyStream(AARCH64ObjectModule* module);

bool AARCH64ObjectModuleAppendFunction(AARCH64ObjectModule* module,
                                      AARCH64Generator* generator);
void AARCH64ObjectModuleEndFunction(AARCH64ObjectModule* module);
bool AARCH64AssembleFragment(AARCH64ObjectModule* module, const char* name,
                             const char* text, size_t length);

bool AARCH64ObjectModuleFinalize(AARCH64ObjectModule* module);
void AARCH64ObjectModuleDestruct(AARCH64ObjectModule* module);

String* AARCH64EmitObjectFile(Compiler* compiler, Vector* options);

#endif /* aarch64_object_h */
