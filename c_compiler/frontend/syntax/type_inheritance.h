//
//  type_inheritance.h
//  c_compiler
//

#ifndef type_inheritance_h
#define type_inheritance_h

#include "type_core.h"

bool StructHasVirtualBases(Struct* str);
Symbol* StructFindVBTableSymbol(Struct* complete, Struct* source,
                                int source_offset);
Symbol* StructFindVTableSymbol(Struct* complete, Struct* source,
                               int source_offset);

#endif /* type_inheritance_h */
