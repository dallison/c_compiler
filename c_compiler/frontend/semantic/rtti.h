//
//  rtti.h
//  c_compiler
//

#ifndef rtti_h
#define rtti_h

#include "symbol.h"

struct TypeRecord;

typedef enum {
  kRttiABIItanium,
  kRttiABIDaveCC,
} RttiABI;

RttiABI RttiTargetABI(void);
bool RttiUsesItaniumABI(void);
Symbol* RttiGetTypeInfoSymbol(struct TypeRecord* type);

#endif /* rtti_h */
