//
//  wasm32_data.c
//  c_compiler
//

#include "wasm32_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_emitter.h"
#include "compiler.h"
#include "target_generator.h"
#include "type_compare.h"
#include "wasm32_machine.h"

const char* Wasm32LiteralSymbolName(int literal_id, char* buf, size_t size) {
  snprintf(buf, size, ".L.str.%d", literal_id);
  return buf;
}

static uint32_t AlignmentLog2(uint32_t alignment) {
  uint32_t log = 0;
  while ((1u << log) < alignment && log < 31) {
    log++;
  }
  return log;
}

// Extend the segment with zeros so that byte 'length' exists.  Anything an
// initializer does not write stays zero, which is what both .bss and the
// gaps between initializers need.
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

// Add a segment and the one data symbol that owns it.  'prefix' names the
// kind of data, which is what tells the linker where to place it and, for
// .bss, that its bytes need not reach the output.
static Wasm32Segment* AddSegment(Wasm32ObjectFile* object, const char* prefix,
                                 const char* name, uint32_t alignment,
                                 uint32_t size, bool is_global, bool is_weak,
                                 Wasm32Symbol** out_symbol) {
  Wasm32Segment* segment = malloc(sizeof(Wasm32Segment));
  size_t length = strlen(prefix) + strlen(name) + 2;
  segment->name = malloc(length);
  snprintf(segment->name, length, "%s.%s", prefix, name);
  segment->alignment = AlignmentLog2(alignment);
  segment->flags = 0;
  segment->size = size;
  segment->address = 0;
  BufferInit(&segment->bytes);
  VectorInit(&segment->relocs);
  VectorAppend(&object->segments, segment);

  Wasm32Symbol* symbol = Wasm32ObjectSymbol(object, WASM_SYMBOL_DATA, name);
  symbol->flags = 0;
  if (is_weak) {
    symbol->flags |= WASM_SYM_BINDING_WEAK;
  } else if (!is_global) {
    symbol->flags |= WASM_SYM_BINDING_LOCAL;
  }
  symbol->segment = (uint32_t)(object->segments.length - 1);
  symbol->offset = 0;
  symbol->size = size;
  *out_symbol = symbol;
  return segment;
}

static const char* VariableName(Symbol* symbol, char* buf, size_t length) {
  return TargetSymbolName(symbol, buf, length);
}

static void WriteLiteralBytes(Literal* literal, Wasm32Segment* segment) {
  switch (literal->type) {
    case kLiteralString:
    case kLiteralWideString: {
      StringLiteral* string = (StringLiteral*)literal;
      PutBytes(&segment->bytes, 0, string->value.value, string->value.length);
      // The terminator is already zero; just make sure the space exists.
      Reserve(&segment->bytes, LiteralSize(literal));
      break;
    }
    case kLiteralBuffer: {
      BufferLiteral* buffer = (BufferLiteral*)literal;
      PutBytes(&segment->bytes, 0, buffer->value.value, buffer->value.length);
      break;
    }
  }
}

// A word in the data that names something rather than holding a value.  The
// bytes stay zero and a relocation says what belongs there; the addend
// carries any offset from the symbol, because a relocation names a symbol
// and nothing finer.
static void AddDataReloc(Wasm32ObjectFile* object, Wasm32Segment* segment,
                         size_t offset, uint8_t type, Wasm32Symbol* symbol,
                         int32_t addend) {
  Reserve(&segment->bytes, offset + 4);
  VectorAppend(&segment->relocs,
               Wasm32NewReloc(type, (uint32_t)offset,
                              (uint32_t)Wasm32ObjectSymbolIndex(object, symbol),
                              addend));
}

static void WriteVariableBytes(Wasm32ObjectFile* object,
                               InitializedStaticVariable* var,
                               Wasm32Segment* segment) {
  char buf[256];

  for (size_t i = 0; i < var->initializers.length; i++) {
    Initializer* init = var->initializers.value.p[i];
    size_t at = (size_t)init->offset;
    switch (init->type) {
      case kInitTypeByte:
        PutInteger(&segment->bytes, at, init->value.byte, 1);
        break;
      case kInitTypeHalf:
        PutInteger(&segment->bytes, at, init->value.half, 2);
        break;
      case kInitTypeWord:
        PutInteger(&segment->bytes, at, init->value.word, 4);
        break;
      case kInitTypeLong:
        PutInteger(&segment->bytes, at, init->value._long, 8);
        break;
      case kInitTypeSymbol: {
        const char* name =
            TargetSymbolName(init->value.symbol, buf, sizeof(buf));
        if (TypeIsFunction(init->value.symbol->type)) {
          // A function pointer is a slot in the table, not an address, and
          // offsetting one is meaningless.
          Wasm32Symbol* target =
              Wasm32ObjectSymbol(object, WASM_SYMBOL_FUNCTION, name);
          AddDataReloc(object, segment, at, R_WASM_TABLE_INDEX_I32, target, 0);
        } else {
          Wasm32Symbol* target =
              Wasm32ObjectSymbol(object, WASM_SYMBOL_DATA, name);
          AddDataReloc(object, segment, at, R_WASM_MEMORY_ADDR_I32, target,
                       (int32_t)init->symbol_addend);
        }
        break;
      }
      case kInitTypeString: {
        Wasm32Symbol* target = Wasm32ObjectSymbol(
            object, WASM_SYMBOL_DATA,
            Wasm32LiteralSymbolName(init->value.literal_id, buf, sizeof(buf)));
        AddDataReloc(object, segment, at, R_WASM_MEMORY_ADDR_I32, target, 0);
        break;
      }
      case kInitTypeMemory:
        PutBytes(&segment->bytes, at, init->value.memory.value,
                 init->value.memory.length);
        break;
    }
  }

  // The initializers may stop short of the object's size; the rest is zero
  // but still belongs to the segment.
  Reserve(&segment->bytes, var->size);
}

// The constructors and destructors this translation unit contributes, as an
// array of function pointers for libc to walk.  A wasm function pointer is a
// table slot rather than an address, so each entry is a relocation the
// linker fills in once it has assigned the slots.
static void BuildInitFiniArray(Wasm32ObjectFile* object, Vector* functions,
                               const char* section, bool is_fini) {
  Vector ordered;
  VectorInit(&ordered);
  CollectInitFiniArrayFunctions(functions, is_fini, &ordered);
  if (ordered.length == 0) {
    VectorDestruct(&ordered);
    return;
  }

  char buf[256];
  Wasm32Symbol* owner;
  Wasm32Segment* segment =
      AddSegment(object, section, "entries", 4,
                 (uint32_t)(ordered.length * 4), /*is_global=*/false,
                 /*is_weak=*/false, &owner);
  for (size_t i = 0; i < ordered.length; i++) {
    Symbol* function = ordered.value.p[i];
    Wasm32Symbol* target = Wasm32ObjectSymbol(
        object, WASM_SYMBOL_FUNCTION,
        TargetSymbolName(function, buf, sizeof(buf)));
    AddDataReloc(object, segment, i * 4, R_WASM_TABLE_INDEX_I32, target, 0);
  }
  Reserve(&segment->bytes, ordered.length * 4);
  VectorDestruct(&ordered);
}

bool Wasm32BuildDataSegments(Wasm32ObjectFile* object) {
  char buf[256];
  bool ok = true;

  // Every symbol has to exist before any initializer is written, so that an
  // initializer naming a static declared later still finds one symbol.
  for (size_t i = 0; i < compiler->literals.length; i++) {
    Literal* literal = compiler->literals.value.p[i];
    if (literal->disabled) {
      continue;
    }
    Wasm32Symbol* symbol;
    Wasm32Segment* segment = AddSegment(
        object, ".rodata",
        Wasm32LiteralSymbolName(literal->id, buf, sizeof(buf)),
        LiteralAlignment(literal), (uint32_t)LiteralSize(literal),
        /*is_global=*/false, /*is_weak=*/false, &symbol);
    WriteLiteralBytes(literal, segment);
  }

  // One entry per initialized static, in step with the compiler's list, so
  // that the second pass can find the segment a variable was given.
  Vector variable_segments;
  VectorInit(&variable_segments);
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    if (var->is_tls) {
      fprintf(stderr, "wasm32: thread-local '%s' is not supported.\n",
              VariableName(var->symbol, buf, sizeof(buf)));
      ok = false;
      VectorAppend(&variable_segments, NULL);
      continue;
    }
    Wasm32Symbol* symbol;
    Wasm32Segment* segment =
        AddSegment(object, ".data", VariableName(var->symbol, buf, sizeof(buf)),
                   (uint32_t)var->alignment, (uint32_t)var->size,
                   var->is_global, var->is_weak, &symbol);
    VectorAppend(&variable_segments, segment);
  }

  for (size_t i = 0; i < compiler->uninitialized_static_variables.length; i++) {
    UninitializedStaticVariable* var =
        compiler->uninitialized_static_variables.value.p[i];
    if (var->is_tls) {
      fprintf(stderr, "wasm32: thread-local '%s' is not supported.\n",
              VariableName(var->symbol, buf, sizeof(buf)));
      ok = false;
      continue;
    }
    // A tentative definition is only a definition if nothing else in the
    // translation unit gives the variable a value, and the compiler lists it
    // here either way.  Taking it now would put the name on a run of zeroes
    // and lose the initializer written above.
    const char* name = VariableName(var->symbol, buf, sizeof(buf));
    Wasm32Symbol* existing =
        Wasm32ObjectFindSymbol(object, WASM_SYMBOL_DATA, name);
    if (existing != NULL && (existing->flags & WASM_SYM_UNDEFINED) == 0) {
      continue;
    }
    Wasm32Symbol* symbol;
    Wasm32Segment* segment =
        AddSegment(object, ".bss", name, (uint32_t)var->alignment,
                   (uint32_t)var->size, var->is_global, var->is_weak, &symbol);
    // The bytes are all zero, and the linker leaves them out of the module
    // because linear memory starts out zeroed; they are here so that the
    // object stays a self-consistent module.
    Reserve(&segment->bytes, var->size);
  }

  BuildInitFiniArray(object, CXXInitArrayFunctionsVector(), ".init_array",
                     /*is_fini=*/false);
  BuildInitFiniArray(object, CXXFiniArrayFunctionsVector(), ".fini_array",
                     /*is_fini=*/true);

  // Second pass, now that every data symbol exists.
  for (size_t i = 0; i < compiler->initialized_static_variables.length; i++) {
    InitializedStaticVariable* var =
        compiler->initialized_static_variables.value.p[i];
    Wasm32Segment* segment = variable_segments.value.p[i];
    if (segment != NULL) {
      WriteVariableBytes(object, var, segment);
    }
  }

  VectorDestruct(&variable_segments);
  return ok;
}
