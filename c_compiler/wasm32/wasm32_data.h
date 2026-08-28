//
//  wasm32_data.h
//  c_compiler
//
//  String literals and static variables as relocatable data segments.
//

#ifndef wasm32_data_h
#define wasm32_data_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wasm32_object.h"

// Turn everything the compiler collected that needs an address into data
// symbols and the segments holding their bytes.  One segment per symbol, so
// that the linker is free to place them independently.  Returns false,
// having explained why, if something cannot be represented.
bool Wasm32BuildDataSegments(Wasm32ObjectFile* object);

// The name of the data symbol standing for a string literal.  Literals have
// no source-level name, so one is invented from the id; the leading .L
// marks it as local, which keeps two objects' literals from colliding.
const char* Wasm32LiteralSymbolName(int literal_id, char* buf, size_t size);

#endif /* wasm32_data_h */
