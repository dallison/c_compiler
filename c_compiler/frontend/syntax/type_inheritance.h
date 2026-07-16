//
//  type_inheritance.h
//  c_compiler
//

#ifndef type_inheritance_h
#define type_inheritance_h

#include "type_core.h"

bool StructHasVirtualBases(Struct* str);

// Whether destroying an object of `type` (a class, or array of class objects)
// runs any non-trivial destructor.  Computed structurally so it is correct for
// template instantiations whose cached triviality flag is stale.
bool TypeHasNonTrivialDestructor(TypeRecord* type);
Symbol* StructFindVBTableSymbol(Struct* complete, Struct* source,
                                int source_offset);
Symbol* StructFindVTableSymbol(Struct* complete, Struct* source,
                               int source_offset);

#endif /* type_inheritance_h */
