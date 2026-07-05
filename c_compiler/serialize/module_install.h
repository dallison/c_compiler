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

// Merges the *exported* symbols, tags, and namespaces of a loaded module into
// `compiler`'s global symbol/tag tables and namespace tree.  Non-exported
// entities remain reachable through the object graph (for type references) but
// are not injected into name lookup.
//
// The `LoadedModule` (its owned_buffers and object graph) must stay alive for
// the remainder of the compile, since the installed symbols point into it.
// Requires an initialized `compiler` global.  Returns false if there is no
// compiler or global namespace to install into.
//
// Duplicate-name policy for this phase: an entity whose name already exists in
// the destination scope is skipped (the existing declaration wins); real
// redefinition diagnostics and overload merging are left for a later phase.
bool ModuleInstallLoaded(LoadedModule* m);

#endif /* module_install_h */
