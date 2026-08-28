//
//  wasm32_emitter.h
//  c_compiler
//
//  Textual listing of lowered wasm functions.  The binary module writer is
//  the source of truth for what actually runs; this exists for -S and for
//  reading what the backend produced.
//

#ifndef wasm32_emitter_h
#define wasm32_emitter_h

#include <stdio.h>

#include "wasm32_codegen.h"

void Wasm32PrintFunction(Wasm32Generator* wasm, FILE* fp);

#endif /* wasm32_emitter_h */
