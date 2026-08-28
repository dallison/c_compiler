//
//  wasm32_link.h
//  c_compiler
//
//  Linking wasm objects into a module an engine can run.
//

#ifndef wasm32_link_h
#define wasm32_link_h

#include "dstring.h"

// Link the objects and archives named on the command line into one module.
// Accepts the subset of the linker's argument surface that means anything
// here: -o, -L, -l, -e, and the input files.  Returns the name of the file
// written, or NULL if the link failed, having explained why.
String* Wasm32Link(int argc, char** argv);

// Whether this argument list is destined for the wasm linker.  The driver
// asks before it decides which linker to run.
bool Wasm32IsTargetName(const char* name);

#endif /* wasm32_link_h */
