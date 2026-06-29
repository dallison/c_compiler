//
//  rtti.h
//  c_compiler
//
//  C++ run-time type information (RTTI) support: emission of std::type_info
//  objects describing class (and other) types, used by the vtable RTTI slot,
//  the typeid operator and the dynamic_cast runtime.
//

#ifndef rtti_h
#define rtti_h

#include "symbol.h"

struct TypeRecord;

// Returns (creating on first use) the static symbol that names the
// std::type_info object describing `type`.  The first call for a given type
// emits the type_info object, its mangled-name string and -- for class types
// -- its direct base-info array as weak globals, recursing so every referenced
// base type_info also exists.  Results are de-duplicated per translation unit
// so identical types share a single object (giving typeid pointer-identity).
// Returns NULL if no type_info can be produced for the type.
Symbol* RttiGetTypeInfoSymbol(struct TypeRecord* type);

#endif /* rtti_h */
