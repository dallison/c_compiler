//
//  type_layout.h
//  c_compiler
//

#ifndef type_layout_h
#define type_layout_h

#include "type_defs.h"

// Applies layout-affecting attributes (packed, aligned) from an Attribute
// vector to a struct.  Must be called before the struct is laid out (or
// followed by a re-layout for the trailing/typedef form).
void StructApplyLayoutAttributes(struct Struct* str, Vector* attrs);
// Propagates packed/aligned attributes from a (typedef) symbol onto its
// struct/union type and re-lays out the struct.  No-op if the symbol carries
// no layout attribute or its type is not a struct/union.
void TypeApplyStructAttributesFromSymbol(Symbol* sym);

#endif /* type_layout_h */
