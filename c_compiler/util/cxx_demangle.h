//
//  cxx_demangle.h
//  c_compiler
//
//  Demangler for DaveCC's Itanium-style C++ asm names (_Z…).
//

#ifndef cxx_demangle_h
#define cxx_demangle_h

#include <stdbool.h>
#include <stddef.h>

#include "dstring.h"

// True if `name` looks like a DaveCC C++ mangled symbol (_Z / __Z / _ZTS…).
bool CXXIsMangledName(const char* name);

// Demangle `mangled` into `out`.  Returns true on success.  On failure `out`
// is left empty and the caller should keep the mangled spelling.
bool CXXDemangle(const char* mangled, String* out);

// If demangle is requested and `name` demangles, copy the demangled form into
// `buf` and return `buf`.  Otherwise return `name` unchanged.
const char* CXXFormatSymbol(const char* name, bool demangle, char* buf,
                            size_t buf_size);

#endif /* cxx_demangle_h */
