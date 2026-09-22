//
//  std_header_suggestion.h
//  c_compiler
//
//  Suggest a standard header (or `import std;`) for missing library names.
//

#ifndef std_header_suggestion_h
#define std_header_suggestion_h

#include "syntax.h"

// After an unknown-symbol or unknown-type diagnostic, emit a note pointing at
// the standard header (and, in C++20 or later, `import std;`) that provides
// the name, when it is a known library symbol.
void ReportStdHeaderSuggestion(Syntax* syntax, FullyQualifiedIdentifier* name);
void ReportStdHeaderSuggestionForName(Syntax* syntax, const char* spelling);

#endif /* std_header_suggestion_h */
