//
//  alias.h
//  c_compiler
//

#ifndef alias_h
#define alias_h

#include "codegen.h"

#include <stdint.h>

struct LoopInfo;

#define IR_MEMORY_OFFSET_UNKNOWN INT64_MIN

typedef enum {
  kIRMemoryUnknown,
  kIRMemoryLocal,
  kIRMemoryArgument,
  kIRMemoryTemporary,
  kIRMemoryStatic,
  kIRMemoryExtern,
} IRMemoryRootKind;

typedef struct {
  Symbol* base;
  IRMemoryRootKind kind;
  int64_t offset;
  uint32_t size;
  bool escaped;
  bool volatile_access;
  bool atomic_access;
} IRMemoryLocation;

typedef enum {
  kIRNoAlias,
  kIRMayAlias,
  kIRMustAlias,
} IRAliasResult;

bool IRAliasDecode(IRNode* inst, IRMemoryLocation* location);
IRAliasResult IRAliasClassify(const IRMemoryLocation* lhs,
                              const IRMemoryLocation* rhs);
bool IRAliasInstMayClobber(const IRMemoryLocation* location, IRNode* inst);
bool IRAliasLoopMayClobber(Generator* gen, const struct LoopInfo* loop,
                           const IRMemoryLocation* location);

#endif /* alias_h */
