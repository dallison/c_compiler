//
//  module_identity.h
//  c_compiler
//
//  C++20 module attachment, linkage, and cross-TU identity for symbols.
//

#ifndef module_identity_h
#define module_identity_h

#include <stdbool.h>

#include "dstring.h"
#include "symbol.h"

// Returns true when `id` names a module purview owner (module name non-empty).
bool ModuleIdentityIsNamed(const String* module_name);

// Formats `module_name` and optional `partition` as "name" or "name:partition".
void ModuleIdentityFormat(const String* module_name, const String* partition,
                          String* out);

// Computes and stores linkage plus owning-module fields on a new declaration.
void SymbolAttachModuleContext(Symbol* sym, Storage storage);

// True when two symbols share the same owning module name and partition.
bool SymbolSameOwningModule(const Symbol* a, const Symbol* b);

// True when a redeclaration/definition merge is permitted across module/import
// boundaries (exported import, implementation-to-interface binding, etc.).
bool SymbolCompatibleModuleRedeclaration(const Symbol* existing,
                                         const Symbol* incoming);

// Records the module through which an importer gained name visibility.
void SymbolSetImportProvenance(Symbol* sym, const char* source_module);

// Finds the first overload-compatible symbol in `head` that may merge with
// `incoming` under module identity rules.
Symbol* SymbolFindModuleCompatibleOverload(Symbol* head, Symbol* incoming);

// Appends a stable module-local mangling prefix for module-linkage entities.
void AppendCXXModuleIdentityMangling(String* out, const Symbol* symbol);

#endif /* module_identity_h */
