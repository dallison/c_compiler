//
//  wasm32_object.h
//  c_compiler
//
//  The relocatable object form of a wasm module.
//
//  A wasm object is a valid module that leaves every reference the linker
//  will have to change encoded as a five-byte LEB128, so that patching one
//  cannot move the bytes around it.  Two custom sections say what those
//  references are: 'linking' carries the symbol table and describes the data
//  segments, and 'reloc.CODE' and 'reloc.DATA' list the places to patch.
//
//  This is the format LLVM and wasm-ld use, so wasm-objdump can read what we
//  produce, which is worth more than a leaner format of our own would be.
//

#ifndef wasm32_object_h
#define wasm32_object_h

#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"
#include "dstring.h"
#include "vector.h"

// Section ids.
#define WASM_SECTION_CUSTOM 0
#define WASM_SECTION_TYPE 1
#define WASM_SECTION_IMPORT 2
#define WASM_SECTION_FUNCTION 3
#define WASM_SECTION_TABLE 4
#define WASM_SECTION_MEMORY 5
#define WASM_SECTION_GLOBAL 6
#define WASM_SECTION_EXPORT 7
#define WASM_SECTION_START 8
#define WASM_SECTION_ELEMENT 9
#define WASM_SECTION_CODE 10
#define WASM_SECTION_DATA 11

#define WASM_EXTERN_FUNC 0
#define WASM_EXTERN_TABLE 1
#define WASM_EXTERN_MEMORY 2
#define WASM_EXTERN_GLOBAL 3

#define WASM_FUNCTYPE 0x60

// Subsections of the 'linking' section.
#define WASM_SEGMENT_INFO 5
#define WASM_INIT_FUNCS 6
#define WASM_COMDAT_INFO 7
#define WASM_SYMBOL_TABLE 8

#define WASM_LINKING_VERSION 2

// Symbol kinds.
#define WASM_SYMBOL_FUNCTION 0
#define WASM_SYMBOL_DATA 1
#define WASM_SYMBOL_GLOBAL 2
#define WASM_SYMBOL_SECTION 3
#define WASM_SYMBOL_TAG 4
#define WASM_SYMBOL_TABLE_KIND 5

// Symbol flags.
#define WASM_SYM_BINDING_WEAK 0x01
#define WASM_SYM_BINDING_LOCAL 0x02
#define WASM_SYM_VISIBILITY_HIDDEN 0x04
#define WASM_SYM_UNDEFINED 0x10
#define WASM_SYM_EXPORTED 0x20
#define WASM_SYM_EXPLICIT_NAME 0x40
#define WASM_SYM_NO_STRIP 0x80

// Relocation types.  The suffix says how the value is encoded at the patch
// site: LEB and SLEB are five-byte padded LEB128s, I32 is four raw bytes.
#define R_WASM_FUNCTION_INDEX_LEB 0
#define R_WASM_TABLE_INDEX_SLEB 1
#define R_WASM_TABLE_INDEX_I32 2
#define R_WASM_MEMORY_ADDR_LEB 3
#define R_WASM_MEMORY_ADDR_SLEB 4
#define R_WASM_MEMORY_ADDR_I32 5
#define R_WASM_TYPE_INDEX_LEB 6
#define R_WASM_GLOBAL_INDEX_LEB 7

// The names the objects agree on for the three things the linker provides.
#define WASM32_MEMORY_IMPORT "__linear_memory"
#define WASM32_TABLE_IMPORT "__indirect_function_table"
#define WASM32_STACK_POINTER "__stack_pointer"
#define WASM32_IMPORT_MODULE "env"

// A function whose name starts with this prefix is a call into the host
// rather than something the program defines, and the host knows it by the
// rest of the name.  Wasm has no inline assembler to spell such a call out
// in, and an attribute would have to be understood by the whole front end,
// so the name carries the meaning instead.
#define WASM32_WASI_MODULE "wasi_snapshot_preview1"
#define WASM32_WASI_PREFIX "__wasi_"

// The field name the host knows a symbol by, or NULL if the symbol is an
// ordinary one that the link has to find a definition for.
const char* Wasm32WasiFieldName(const char* name);

// Addresses only the linker knows, which it supplies to whatever names them.
// The heap is what is left of linear memory once the static data and the
// shadow stack have had their share.
#define WASM32_DATA_END "__data_end"
#define WASM32_HEAP_BASE "__heap_base"
#define WASM32_HEAP_END "__heap_end"

// No signature has been worked out for an import yet.
#define WASM32_NO_TYPE 0xFFFFFFFFu

// One entry in an object's symbol table.  A symbol is either a function, a
// piece of data, or a global; which of the trailing fields mean anything
// depends on the kind and on whether it is defined here.
typedef struct {
  uint8_t kind;
  uint32_t flags;
  char* name;

  // Defined function: its index in this object's function index space, which
  // counts imports first.  Undefined function: the import that stands in for
  // it, which also lives in that space.
  uint32_t index;

  // Undefined function: the signature the import is declared with, taken
  // from a call site.  The definition's own signature wins once the linker
  // has found it, so this only has to make the object validate on its own.
  uint32_t type_index;

  // Defined data: where the bytes are.
  uint32_t segment;
  uint32_t offset;
  uint32_t size;

  // Filled in by the linker.
  int32_t address;      // Data: final linear memory address.
  int32_t final_index;  // Function: final index; global: final global index.
  int32_t table_slot;   // Function: table slot, or -1 if never taken.
  void* owner;          // Wasm32ObjectFile* that defines it.
  void* definition;     // The defining Wasm32Symbol, once resolved.
} Wasm32Symbol;

// A place in a section's payload that names a symbol rather than a value.
typedef struct {
  uint8_t type;
  uint32_t offset;  // From the start of the section payload.
  uint32_t index;   // Symbol index, or type index for R_WASM_TYPE_INDEX_LEB.
  int32_t addend;
} Wasm32Reloc;

// A run of initialized bytes that the linker places as a unit.  One data
// symbol owns each segment, which is what lets the linker drop or reorder
// them.  A .bss segment carries no bytes but still reserves 'size'.
typedef struct {
  char* name;
  uint32_t alignment;  // log2 of the required alignment.
  uint32_t flags;
  Buffer bytes;
  uint32_t size;    // Bytes to reserve; >= bytes.length for .bss.
  int32_t address;  // Filled in by the linker.
  Vector relocs;    // Wasm32Reloc* at offsets within 'bytes'.
} Wasm32Segment;

// An object, either being built by the compiler or read back by the linker.
//
// Relocations hang off the function or the segment they patch rather than
// off the section, so that placing a function or a segment somewhere new is
// just a matter of adding to the offsets its own relocations carry.
typedef struct Wasm32ObjectFile {
  String filename;

  Vector types;      // Buffer* of encoded functypes.
  Vector symbols;    // Wasm32Symbol*
  Vector segments;   // Wasm32Segment*
  Vector functions;  // Wasm32Function* defined here.

  // Imported function symbols, in index-space order; a defined function's
  // index is its position in 'functions' plus this length.
  Vector imported_functions;  // Wasm32Symbol*

  bool is_live;  // Archive members are only linked in once needed.
} Wasm32ObjectFile;

// One defined function: its signature and its encoded body, minus the size
// prefix the code section adds.
typedef struct {
  char* name;
  uint32_t type_index;  // Into the owning object's type vector.
  Buffer body;
  Wasm32Symbol* symbol;
  Vector relocs;  // Wasm32Reloc* with body-relative offsets.
} Wasm32Function;

// LEB128 encoders.  The padded form always occupies five bytes so that a
// value patched at link time can never change the size of the code around
// it; every relocatable index must go through it.
void WasmWriteULEB128(Buffer* buf, uint64_t value);
void WasmWriteSLEB128(Buffer* buf, int64_t value);
void WasmWritePaddedU32(Buffer* buf, uint32_t value);
void WasmPatchPaddedU32(Buffer* buf, size_t offset, uint32_t value);
void WasmPatchU32(Buffer* buf, size_t offset, uint32_t value);

void WasmWriteName(Buffer* buf, const char* name);
void WasmWriteSection(Buffer* out, int id, Buffer* body);
void WasmWriteCustomSection(Buffer* out, const char* name, Buffer* body);

// Object construction.
void Wasm32ObjectFileInit(Wasm32ObjectFile* object, const char* filename);
void Wasm32ObjectFileDestruct(Wasm32ObjectFile* object);

// Find a symbol by kind and name, or add an undefined one.  Everything the
// encoder references goes through this, so that a name used before it is
// defined still lands on one symbol.
Wasm32Symbol* Wasm32ObjectSymbol(Wasm32ObjectFile* object, uint8_t kind,
                                 const char* name);
Wasm32Symbol* Wasm32ObjectFindSymbol(Wasm32ObjectFile* object, uint8_t kind,
                                     const char* name);
int Wasm32ObjectSymbolIndex(Wasm32ObjectFile* object, Wasm32Symbol* symbol);

// Index of an encoded signature in the object's type vector, adding it if it
// is not there yet.
int Wasm32ObjectInternType(Wasm32ObjectFile* object, Buffer* encoding);

Wasm32Reloc* Wasm32NewReloc(uint8_t type, uint32_t offset, uint32_t index,
                            int32_t addend);

// Serialize a fully built object.
bool Wasm32WriteObjectFile(Wasm32ObjectFile* object, String* filename);

// Parse an object back in.  'bytes' stays owned by the caller and must
// outlive the object.  Returns false, having explained why, if the bytes are
// not a wasm object this linker understands.
bool Wasm32ReadObjectFile(Wasm32ObjectFile* object, const char* filename,
                          const uint8_t* bytes, size_t length);

#endif /* wasm32_object_h */
