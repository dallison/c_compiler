//
//  wasm32_module.h
//  c_compiler
//
//  Serialization of lowered functions into a relocatable wasm object.
//

#ifndef wasm32_module_h
#define wasm32_module_h

#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"
#include "dstring.h"
#include "wasm32_codegen.h"
#include "wasm32_object.h"

// The object being built for this translation unit.  Lowering needs it
// before any function is finished, because an indirect call has to intern
// its signature the moment it is lowered, so the object outlives any one
// function and is reset when the module is written.
Wasm32ObjectFile* Wasm32CurrentObject(void);
void Wasm32ResetCurrentObject(void);

// Index of a signature in the object's type section, adding it if it is not
// there yet.
int Wasm32InternSignature(Wasm32Signature* signature);

// Encode one lowered function's body: the local declarations followed by the
// instruction sequence, without the trailing size prefix.  Anything the
// linker will have to resolve is written as a five-byte placeholder and
// appended to 'relocs' with an offset relative to the start of 'out'.
void Wasm32EncodeFunctionBody(Wasm32Generator* wasm, Wasm32ObjectFile* object,
                              Buffer* out, Vector* relocs);

// Write every function the compiler has lowered as a relocatable object.
bool Wasm32WriteObject(String* filename);

#endif /* wasm32_module_h */
