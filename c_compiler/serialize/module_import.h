//
//  module_import.h
//  c_compiler
//
//  Per-translation-unit C++20 module import state: path resolution, archive
//  validation, deduplicated loading/installation, and explicit teardown of
//  LoadedModule graphs.  One store is attached to each translation-unit compile
//  via CompilerSetImportState and released from CompilerDestruct after IR
//  teardown and before the AST arena and lookup tables are torn down.
//

#ifndef module_import_h
#define module_import_h

#include <stdbool.h>

#include "vector.h"

typedef struct TranslationUnitImportState TranslationUnitImportState;

// Creates import state for one translation unit.  Search paths are borrowed
// from `options` (-fprebuilt-module-path) for the store's lifetime.
TranslationUnitImportState* TranslationUnitImportStateCreate(Vector* options);

// Frees the store shell after TranslationUnitImportStateRelease has dropped any
// loaded graphs.  Safe to call after CompilerDelete.
void TranslationUnitImportStateDelete(TranslationUnitImportState* state);

// Detaches imported names from compiler lookup tables and releases every
// heap-owned deserialized Symbol/Namespace graph owned by this store.  Must run
// after generated-function IR teardown and while declaration ASTs are gone, but
// before ASTArenaRelease and DeleteGlobalNamespace.
void TranslationUnitImportStateRelease(TranslationUnitImportState* state);

// Resolves, loads, validates, and installs `module_name` into the active
// `compiler` global.  Repeated imports of the same module are idempotent.
// Returns false on resolution, format, validation, installation, or cycle errors.
bool TranslationUnitImportStateImport(TranslationUnitImportState* state,
                                      const char* module_name);

// Human-readable detail for the most recent failed import on this store, or
// NULL when no detail is available.
const char* TranslationUnitImportStateLastError(
    TranslationUnitImportState* state);

// Test-only helper: mark a module entry as loading without loading it.
void TranslationUnitImportStateMarkLoadingForTest(
    TranslationUnitImportState* state, const char* module_name);

// Validates archive header metadata against the requested import name and the
// active compiler target.  On failure, writes a message into `err` when non-NULL.
bool ModuleValidateLoadedForImport(const char* requested_module_name,
                                   const char* requested_target,
                                   const char* archive_module_name,
                                   const char* archive_target, char* err,
                                   size_t err_len);

#endif /* module_import_h */
