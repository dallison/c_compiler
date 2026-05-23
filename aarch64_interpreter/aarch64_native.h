//
//  aarch64_native.h
//  aarch64_interpreter
//

#ifndef aarch64_native_h
#define aarch64_native_h

#include <stdint.h>

#include "loader.h"

// True when -n was requested but guest code must run under the interpreter
// (dynamic executables on hosts where native cross-DSO calls are unreliable).
bool AARCH64NativeNeedsInterpreter(const Loader* loader);

int AARCH64NativeRun(Loader* loader, uint64_t entry_address, int argc,
                     char** argv);

#endif /* aarch64_native_h */
