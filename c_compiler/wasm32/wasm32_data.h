//
//  wasm32_data.h
//  c_compiler
//
//  Placement of string literals and static variables in linear memory.
//

#ifndef wasm32_data_h
#define wasm32_data_h

#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"
#include "vector.h"

// Where everything the module needs at a fixed address ended up.  Built once
// per module, before any function body is encoded, because a body that takes
// the address of a static has to encode that address as an immediate.
typedef struct {
  Buffer bytes;         // Contents of the one active data segment.
  uint32_t start;       // Address the segment loads at.
  uint32_t stack_top;   // Initial value of the shadow stack pointer.
  uint32_t heap_start;  // First address above everything reserved here.
  Vector literals;      // Wasm32DataAddress* for each literal, by id.
  Vector symbols;       // Wasm32DataAddress* for each static variable.
  bool failed;
} Wasm32DataLayout;

// Lay out every literal and static variable the compiler collected.  Returns
// false, having explained why, if something cannot be represented.
bool Wasm32BuildDataLayout(Wasm32DataLayout* layout);

void Wasm32DataLayoutDestruct(Wasm32DataLayout* layout);

// Address of a literal by id, or of a static variable by name.  Both report
// an error and return 0 when the name is not one this module defines.
uint32_t Wasm32LiteralAddress(Wasm32DataLayout* layout, int literal_id);
uint32_t Wasm32SymbolAddress(Wasm32DataLayout* layout, const char* name);

// Index of a function this module defines, or -1.  Its slot in the function
// table is one past this, because slot 0 is reserved for the null pointer.
int Wasm32FunctionIndex(const char* name);

#endif /* wasm32_data_h */
