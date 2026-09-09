//
//  typo_correction.h
//  c_compiler
//
//  Close-match suggestions for misspelled identifiers ("did you mean ...?").
//

#ifndef typo_correction_h
#define typo_correction_h

#include "syntax.h"

struct Struct;

// Unique nearby identifier visible from `syntax`, or NULL if none is close
// enough.  Distance uses the same (len+2)/3 cap as clang.
const char* TypoCorrectionFindVisibleName(Syntax* syntax, const char* typo);

// Unique nearby member of `str` (including bases), or NULL.
const char* TypoCorrectionFindMemberName(struct Struct* str, const char* typo);

// Suggest a correction for an unresolved qualified or unqualified name.
const char* TypoCorrectionFindForIdentifier(Syntax* syntax,
                                            FullyQualifiedIdentifier* name);

// Emits "No such symbol ...", with a did-you-mean clause when one is found.
void TypoCorrectionErrorUnknownSymbol(Syntax* syntax,
                                      FullyQualifiedIdentifier* name);

#endif /* typo_correction_h */
