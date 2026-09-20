//
//  macho_object.h
//  c_compiler
//

#ifndef macho_object_h
#define macho_object_h

#include <stdbool.h>
#include <stdio.h>

struct AsmObject;

bool AsmObjectWriteMachO(struct AsmObject* object, FILE* out);

#endif /* macho_object_h */
