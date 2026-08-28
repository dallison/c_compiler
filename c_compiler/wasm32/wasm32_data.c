//
//  wasm32_data.c
//  c_compiler
//

#include "wasm32_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "target_generator.h"
#include "type_compare.h"
#include "wasm32_codegen.h"
#include "wasm32_machine.h"

// One placed object.  Literals are keyed by id and variables by name; only
// one of the two fields is meaningful for any given entry.
typedef struct {
  int literal_id;
  char* name;
  uint32_t address;
} Wasm32DataAddress;

static Wasm32DataAddress* NewAddress(int literal_id, const char* name,
                                     uint32_t address) {
  Wasm32DataAddress* entry = malloc(sizeof(Wasm32DataAddress));
  entry->literal_id = literal_id;
  entry->name = name == NULL ? NULL : strdup(name);
  entry->address = address;
  return entry;
}

static uint32_t AlignUp(uint32_t value, uint32_t alignment) {
  if (alignment < 1) {
    alignment = 1;
  }
  return (value + alignment - 1) & ~(alignment - 1);
}

// Extend the segment with zeros so that byte 'length' exists.  Anything not
// written by an initializer stays zero, which is what both .bss and the gaps
// between initializers need.
static void Reserve(Buffer* bytes, size_t length) {
  while (bytes->length < length) {
    BufferAppendByte(bytes, 0);
  }
}

static void PutBytes(Buffer* bytes, size_t offset, const void* data,
                     size_t length) {
  Reserve(bytes, offset + length);
  memcpy(bytes->value + offset, data, length);
}

static void PutInteger(Buffer* bytes, size_t offset, uint64_t value,
                       int width) {
  uint8_t encoded[8];
  for (int i = 0; i < width; i++) {
    encoded[i] = (uint8_t)(value >> (8 * i));
  }
  PutBytes(bytes, offset, encoded, (size_t)width);
}

// Size a literal occupies, including the terminator that makes it a valid C
// string.
static size_t LiteralSize(Literal* literal) {
  switch (literal->type) {
    case kLiteralString:
      return ((StringLiteral*)literal)->value.length + 1;
    case kLiteralWideString: {
      StringLiteral* string = (StringLiteral*)literal;
      return string->value.length + (size_t)string->element_size;
    }
    case kLiteralBuffer:
      return ((BufferLiteral*)literal)->value.length;
  }
  return 0;
}

static uint32_t LiteralAlignment(Literal* literal) {
  switch (literal->type) {
    case kLiteralString:
      return 1;
    case kLiteralWideString:
      return (uint32_t)((StringLiteral*)literal)->element_size;
    case kLiteralBuffer:
      // A buffer literal is the image of an aggregate whose alignment is not
      // recorded here, so assume the strictest the target has.
      return 16;
  }
  return 1;
}

static const char* VariableName(Symbol* symbol, char* buf, size_t length) {
  return TargetSymbolName(symbol, buf, length);
}

uint32_t Wasm32LiteralAddress(Wasm32DataLayout* layout, int literal_id) {
  for (size_t i = 0; i < layout->literals.length; i++) {
    Wasm32DataAddress* entry = layout->literals.value.p[i];
    if (entry->literal_id == literal_id) {
      return entry->address;
    }
  }
  fprintf(stderr, "wasm32: string literal %d has no address.\n", literal_id);
  layout->failed = true;
  return 0;
}

int Wasm32FunctionIndex(const char* name) {
  for (size_t i = 0; i < compiler->functions.length; i++) {
    Wasm32Generator* wasm = compiler->functions.value.p[i];
    if (strcmp(wasm->base.function_name.value, name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

uint32_t Wasm32SymbolAddress(Wasm32DataLayout* layout, const char* name) {
  for (size_t i = 0; i < layout->symbols.length; i++) {
    Wasm32DataAddress* entry = layout->symbols.value.p[i];
    if (strcmp(entry->name, name) == 0) {
      return entry->address;
    }
  }
  fprintf(stderr,
          "wasm32: '%s' is not defined in this translation unit.  Data "
          "shared between objects needs the wasm linker.\n",
          name);
  layout->failed = true;
  return 0;
}

// Pass one: give every literal and variable an address, so that pass two can
// resolve initializers that point at any of them regardless of order.
static void AssignAddresses(Wasm32DataLayout* layout, uint32_t* next) {
  char buf[256];

  for (size_t i = 0; i < compiler->literals.length; i++) {
    Literal* literal = compiler->literals.value.p[i];
    if (literal->disabled) {
      continue;
    }
    *next = AlignUp(*next, LiteralAlignment(literal));
    VectorAppend(&layout->literals, NewAddress(literal->id, NULL, *next));
    *next += (uint32_t)LiteralSize(literal);
  }

  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (var->is_tls) {
      fprintf(stderr,
              "wasm32: thread-local '%s' is not supported.\n",
              VariableName(var->symbol, buf, sizeof(buf)));
      layout->failed = true;
      continue;
    }
    *next = AlignUp(*next, (uint32_t)var->alignment);
    VectorAppend(&layout->symbols,
                 NewAddress(-1, VariableName(var->symbol, buf, sizeof(buf)),
                            *next));
    *next += (uint32_t)var->size;
  }

  // Uninitialized variables go last so that the data segment written below
  // ends at the last byte anything actually initializes.  Linear memory
  // starts out zeroed, so reserving the space is all these need.
  for (size_t i = 0; i < compiler->uninitialized_static_variables.length;
       i++) {
    UninitializedStaticVariable* var =
        compiler->uninitialized_static_variables.value.p[i];
    if (var->is_tls) {
      fprintf(stderr, "wasm32: thread-local '%s' is not supported.\n",
              VariableName(var->symbol, buf, sizeof(buf)));
      layout->failed = true;
      continue;
    }
    *next = AlignUp(*next, (uint32_t)var->alignment);
    VectorAppend(&layout->symbols,
                 NewAddress(-1, VariableName(var->symbol, buf, sizeof(buf)),
                            *next));
    *next += (uint32_t)var->size;
  }
}

static void WriteLiteralBytes(Wasm32DataLayout* layout, Literal* literal) {
  size_t offset = Wasm32LiteralAddress(layout, literal->id) - layout->start;
  switch (literal->type) {
    case kLiteralString:
    case kLiteralWideString: {
      StringLiteral* string = (StringLiteral*)literal;
      PutBytes(&layout->bytes, offset, string->value.value,
               string->value.length);
      // The terminator is already zero; just make sure the space exists.
      Reserve(&layout->bytes, offset + LiteralSize(literal));
      break;
    }
    case kLiteralBuffer: {
      BufferLiteral* buffer = (BufferLiteral*)literal;
      PutBytes(&layout->bytes, offset, buffer->value.value,
               buffer->value.length);
      break;
    }
  }
}

static void WriteVariableBytes(Wasm32DataLayout* layout,
                               InitializedStaticVariable* var) {
  char buf[256];
  size_t base =
      Wasm32SymbolAddress(layout, VariableName(var->symbol, buf, sizeof(buf))) -
      layout->start;

  for (size_t i = 0; i < var->initializers.length; i++) {
    Initializer* init = var->initializers.value.p[i];
    size_t at = base + (size_t)init->offset;
    switch (init->type) {
      case kInitTypeByte:
        PutInteger(&layout->bytes, at, init->value.byte, 1);
        break;
      case kInitTypeHalf:
        PutInteger(&layout->bytes, at, init->value.half, 2);
        break;
      case kInitTypeWord:
        PutInteger(&layout->bytes, at, init->value.word, 4);
        break;
      case kInitTypeLong:
        PutInteger(&layout->bytes, at, init->value._long, 8);
        break;
      case kInitTypeSymbol: {
        const char* name =
            TargetSymbolName(init->value.symbol, buf, sizeof(buf));
        int64_t value;
        if (TypeIsFunction(init->value.symbol->type)) {
          // A function pointer is a slot in the function table, not an
          // address, and offsetting one is meaningless.
          int index = Wasm32FunctionIndex(name);
          if (index < 0) {
            fprintf(stderr,
                    "wasm32: '%s' has no table slot, because this "
                    "translation unit does not define it.\n",
                    name);
            layout->failed = true;
          }
          value = index + 1;
        } else {
          value = (int64_t)Wasm32SymbolAddress(layout, name) +
                  init->symbol_addend;
        }
        PutInteger(&layout->bytes, at, (uint64_t)value, 4);
        break;
      }
      case kInitTypeString:
        PutInteger(&layout->bytes, at,
                   Wasm32LiteralAddress(layout, init->value.literal_id), 4);
        break;
      case kInitTypeMemory:
        PutBytes(&layout->bytes, at, init->value.memory.value,
                 init->value.memory.length);
        break;
    }
  }

  // The initializers may stop short of the object's size; the rest is zero
  // but still has to be part of the segment if anything follows it.
  Reserve(&layout->bytes, base + var->size);
}

bool Wasm32BuildDataLayout(Wasm32DataLayout* layout) {
  BufferInit(&layout->bytes);
  VectorInit(&layout->literals);
  VectorInit(&layout->symbols);
  layout->start = WASM32_DATA_START;
  layout->failed = false;

  uint32_t next = layout->start;
  AssignAddresses(layout, &next);

  for (size_t i = 0; i < compiler->literals.length; i++) {
    Literal* literal = compiler->literals.value.p[i];
    if (!literal->disabled) {
      WriteLiteralBytes(layout, literal);
    }
  }
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (!var->is_tls) {
      WriteVariableBytes(layout, var);
    }
  }

  // The shadow stack sits above the data, and the heap above that.  Both are
  // 16-byte aligned because the stack allocator assumes it can hand out
  // maximally aligned frames.
  layout->stack_top = AlignUp(next, 16) + WASM32_STACK_SIZE;
  layout->heap_start = layout->stack_top;
  return !layout->failed;
}

void Wasm32DataLayoutDestruct(Wasm32DataLayout* layout) {
  for (size_t i = 0; i < layout->literals.length; i++) {
    Wasm32DataAddress* entry = layout->literals.value.p[i];
    free(entry->name);
    free(entry);
  }
  for (size_t i = 0; i < layout->symbols.length; i++) {
    Wasm32DataAddress* entry = layout->symbols.value.p[i];
    free(entry->name);
    free(entry);
  }
  VectorDestruct(&layout->literals);
  VectorDestruct(&layout->symbols);
  BufferDestruct(&layout->bytes);
}
