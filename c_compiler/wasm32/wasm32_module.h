//
//  wasm32_module.h
//  c_compiler
//
//  Serialization of lowered functions into the WebAssembly binary format.
//

#ifndef wasm32_module_h
#define wasm32_module_h

#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"
#include "string.h"
#include "wasm32_codegen.h"
#include "wasm32_data.h"

// LEB128 encoders.  The padded form always occupies five bytes so that a
// value patched at link time can never change the size of the code around
// it; every relocatable index must go through it.
void WasmWriteULEB128(Buffer* buf, uint64_t value);
void WasmWriteSLEB128(Buffer* buf, int64_t value);
void WasmWritePaddedU32(Buffer* buf, uint32_t value);
void WasmPatchPaddedU32(Buffer* buf, size_t offset, uint32_t value);

// Encode one lowered function's body: the local declarations followed by the
// instruction sequence, without the trailing size prefix.  The layout
// resolves references to literals and static variables into addresses.
void Wasm32EncodeFunctionBody(Wasm32Generator* wasm, Wasm32DataLayout* layout,
                              Buffer* out);

// Index of a signature in the module's type section, adding it if it is not
// there yet.  Indirect calls need one at lowering time, before the writer
// has seen any function, so the table lives for the whole module.
int Wasm32InternSignature(Wasm32Signature* signature);

// Write every function the compiler has lowered as a complete module.
bool Wasm32WriteModule(String* filename);

#endif /* wasm32_module_h */
