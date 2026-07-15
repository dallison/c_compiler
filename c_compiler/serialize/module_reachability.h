//
//  module_reachability.h
//  c_compiler
//

#ifndef module_reachability_h
#define module_reachability_h

#include <stdbool.h>

#include "vector.h"

typedef struct {
  Vector exported_roots;  // Symbol*, borrowed from the producer Compiler.
  char error[256];
} ModuleReachability;

void ModuleReachabilityInit(ModuleReachability* reachability);
void ModuleReachabilityDestruct(ModuleReachability* reachability);

// Collects the importer-visible declaration roots.  ModuleWrite closes the
// hidden semantic graph from these roots through types, constraints and ASTs.
bool ModuleReachabilityBuild(ModuleReachability* reachability,
                             bool header_unit);

#endif /* module_reachability_h */
