//
//  x86_64_native.h
//  x86_64_interpreter
//

#ifndef x86_64_native_h
#define x86_64_native_h

#include <stdint.h>

#include "loader.h"

bool X86_64NativeNeedsInterpreter(const Loader* loader);

int X86_64NativeRun(Loader* loader, uint64_t entry_address, int argc,
                    char** argv);

#endif /* x86_64_native_h */
