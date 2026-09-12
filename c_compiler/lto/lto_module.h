//
//  lto_module.h
//  c_compiler
//
//  In-memory IR module for bitcode-style LTO.  Archive I/O lives in
//  serialize/lto_archive.h so :compiler does not depend on :serialize.
//

#ifndef lto_module_h
#define lto_module_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "vector.h"

struct Symbol;
struct TypeRecord;
struct Generator;
struct Compiler;

typedef struct LTOFunction {
  struct Symbol* symbol;
  struct TypeRecord* type;
  List code;
  int ir_node_count;
  bool received_inline;
} LTOFunction;

typedef struct LTOModule {
  Vector functions;  // LTOFunction*
  Vector initialized_static_variables;  // InitializedStaticVariable*
  Vector uninitialized_static_variables;  // UninitializedStaticVariable*
  Vector literals;  // Literal*
  Vector init_array;  // Symbol*
  Vector fini_array;  // Symbol*
  int next_literal_id;
  char* tu_id;
  bool owns_globals;
} LTOModule;

LTOModule* LTOModuleCreate(void);
void LTOModuleDestruct(LTOModule* module);
void LTOModuleDelete(LTOModule* module);

LTOFunction* LTOModuleAddFunction(LTOModule* module, struct Generator* gen);
LTOFunction* LTOModuleFindFunction(LTOModule* module, const char* asm_name);
const char* LTOSymbolAsmName(struct Symbol* symbol);

void LTOModuleCaptureCompilerState(LTOModule* module, struct Compiler* compiler);
void LTOModuleRenameInternalSymbols(LTOModule* module, const char* tu_id);
void LTOMakeTUId(const char* filename, char* out /* at least 17 bytes */);

bool LTOModuleMerge(LTOModule* dest, LTOModule* src);
int LTOInlineModule(LTOModule* module);
int LTODevirtualizeModule(LTOModule* module, bool whole_program);
bool LTOCodegenModule(struct Compiler* compiler, LTOModule* module,
                      bool whole_program, struct Vector* preserve_asm_names);
void LTOModuleInstallIntoCompiler(struct Compiler* compiler, LTOModule* module);

#endif /* lto_module_h */
