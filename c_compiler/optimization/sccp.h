//
//  sccp.h
//  c_compiler
//

#ifndef sccp_h
#define sccp_h

#include "codegen.h"

typedef struct {
  size_t constants_folded;
  size_t phis_folded;
  size_t branches_simplified;
} SCCPStats;

bool SparseConditionalConstantPropagation(Generator* gen, SCCPStats* stats);

#endif /* sccp_h */
