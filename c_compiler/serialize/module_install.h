//
//  module_install.h
//  c_compiler
//
//  Installs a loaded C++20 module's exported names into the live compiler's
//  global symbol tables and namespace tree, so that a subsequent parse (of an
//  importing translation unit) can look them up.  This bridges the serialize
//  library (which owns ModuleLoad / LoadedModule) and the compiler's symbol
//  tables; it lives in the serialize library because that already depends on
//  the compiler.
//

#ifndef module_install_h
#define module_install_h

#include <stdbool.h>

#include "module_archive.h"
#include "vector.h"

// Tracks symbols and namespace scaffolding inserted into the live compiler
// during one install attempt so a failure can roll back visible names.
typedef struct {
  Vector global_symbols;       // Symbol* inserted into the global hash table.
  Vector global_tags;          // Symbol* inserted into the global tag table.
  Vector namespaced_symbols;   // Symbol* inserted into a namespace symbol table.
  Vector namespaced_symbol_scopes;  // Namespace* parallel to namespaced_symbols.
  Vector namespaced_tags;        // Symbol* inserted into a namespace tag table.
  Vector namespaced_tag_scopes;  // Namespace* parallel to namespaced_tags.
  Vector created_namespaces;   // Namespace* shells created in the compiler tree.
  Vector alias_names;          // String copies of alias names installed.
  Vector alias_targets;        // Namespace* alias targets (not owned).
  Vector alias_scopes;         // Namespace* where each alias was inserted.
  Vector appended_overloads;   // Symbol* merged onto an existing overload chain.
  Vector overload_heads;       // Symbol* chain head parallel to appended_overloads.
} ModuleInstallRecord;

// Last installation failure message; empty when no failure was recorded.
const char* ModuleInstallLastError(void);

void ModuleInstallRecordInit(ModuleInstallRecord* record);
void ModuleInstallRecordDestruct(ModuleInstallRecord* record);

// Removes every name recorded by a successful/partial install without deleting
// the underlying heap Symbol objects (those remain owned by the LoadedModule
// graph until LoadedModuleReleaseGraph).
void ModuleInstallRecordRollback(ModuleInstallRecord* record);

// Merges exported names into the compiler tables.  When `record` is non-NULL,
// every insertion is tracked so ModuleInstallRecordRollback can make the
// operation atomic on failure.  Returns false if installation cannot complete.
bool ModuleInstallLoadedTracked(LoadedModule* m, ModuleInstallRecord* record);

// Untracked install wrapper kept for existing unit tests.
bool ModuleInstallLoaded(LoadedModule* m);

#endif /* module_install_h */
