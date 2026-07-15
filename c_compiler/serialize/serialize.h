//
//  serialize.h
//  c_compiler
//
//  Graph-aware serialization framework for the C++20 module system.  The AST,
//  type, symbol and namespace records form a shared, cyclic object graph, so
//  they cannot be serialized by naive recursive inlining.  Instead every
//  distinct object of an interned kind is assigned a stable integer "handle"
//  and stored in a flat per-kind pool; cross-references are written as handles
//  rather than inline copies.  This breaks cycles and de-duplicates shared
//  objects.
//
//  Writing (SerializeContext):
//    1. Callers intern their root objects (SerializeIntern / SerializeInternString).
//    2. SerializeContextDrain walks each pool, serializing every object into its
//       own byte record.  Serializing an object may intern further objects,
//       which are appended to the pools and processed in turn until the graph is
//       closed.
//    3. module_archive emits each pool as a length-prefixed record stream.
//
//  Reading (DeserializeContext):
//    1. Pass 1 (SerializeReadPool) reads each pool, allocating one empty object
//       per handle and remembering its serialized bytes.
//    2. Pass 2 (SerializeContextResolve) fills each object's fields, resolving
//       handle references to the pointers allocated in pass 1.  Two passes are
//       required so references to not-yet-read objects (including cycles) can be
//       resolved.
//
//  Handle 0 always denotes NULL.  Real handles are 1-based pool indices.
//

#ifndef serialize_h
#define serialize_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dstring.h"
#include "map.h"
#include "vector.h"
#include "wireformat.h"

// Interned object kinds.  Each has its own pool and handle space.
typedef enum {
  kSerialKindString = 0,
  kSerialKindType,
  kSerialKindSymbol,
  kSerialKindStruct,
  kSerialKindEnum,
  kSerialKindStructMember,
  kSerialKindNamespace,
  kSerialKindAST,
  kSerialKindCount,
} SerialKind;

typedef uint32_t SerialHandle;
#define kSerialNullHandle ((SerialHandle)0)

// A field-metadata descriptor.  Every serializable field of an object records
// its wire number and name here so the schema can be dumped (FIELDMETA) and so
// fields can be added later without breaking existing modules (unknown numbers
// are skipped on read).
typedef struct {
  int number;
  const char* name;
} WireFieldDesc;

//
// Write side.
//
typedef struct SerializeContext {
  // pointer -> handle (int64), for every interned kind except strings.
  Map handle_maps[kSerialKindCount];
  // handle-1 -> object pointer, in interning order.  May grow during drain.
  Vector objects[kSerialKindCount];
  // handle-1 -> WireBuffer* holding the serialized bytes (filled by drain).
  Vector serialized[kSerialKindCount];
  // Number of objects already serialized per kind (drain progress cursor).
  size_t processed[kSerialKindCount];

  // String interning: content -> handle, plus the interned copies.
  Map string_map;      // char* content -> handle (int64).
  Vector string_pool;  // handle-1 -> char* (owned copy, NUL terminated).
  Vector string_lens;  // handle-1 -> length (as int64) of the interned string.

  // Module archives filter namespace lookup tables to exported names.  Hidden
  // dependencies are still interned through direct graph references.
  bool writing_module_interface;
  bool writing_internal_partition;
  bool error;
} SerializeContext;

void SerializeContextInit(SerializeContext* ctx);
void SerializeContextDestruct(SerializeContext* ctx);

// Interns a string by content (de-duplicated).  Returns 0 for a NULL pointer.
SerialHandle SerializeInternStringN(SerializeContext* ctx, const char* str,
                                    size_t len);
SerialHandle SerializeInternString(SerializeContext* ctx, String* s);

// Interns an object of `kind`, returning its handle (0 if ptr is NULL).  On
// first encounter the object is appended to the pool and will be serialized by
// SerializeContextDrain.
SerialHandle SerializeIntern(SerializeContext* ctx, SerialKind kind, void* ptr);

// Serializes every interned object (closing the graph).  Returns false on error.
bool SerializeContextDrain(SerializeContext* ctx);

//
// Read side.
//
typedef struct DeserializeContext {
  // handle-1 -> object pointer (allocated in pass 1).
  Vector objects[kSerialKindCount];
  // handle-1 -> serialized bytes for pass 2: alternating {ptr,len} kept in two
  // parallel vectors.
  Vector blob_ptrs[kSerialKindCount];
  Vector blob_lens[kSerialKindCount];

  // String pool: handle-1 -> char* (owned copy).
  Vector string_pool;
  Vector string_lens;

  bool error;
} DeserializeContext;

void DeserializeContextInit(DeserializeContext* ctx);
void DeserializeContextDestruct(DeserializeContext* ctx);

// Looks up a previously-allocated object pointer by handle (NULL for handle 0).
void* DeserializeResolve(DeserializeContext* ctx, SerialKind kind,
                         SerialHandle handle);
// Returns the interned string bytes for a handle (NULL for handle 0).  *len is
// set to the string length when provided.
const char* DeserializeResolveString(DeserializeContext* ctx,
                                      SerialHandle handle, size_t* len);

//
// Per-kind vtable.  The type/symbol/ast serializers register their functions so
// the framework can drive the generic drain/allocate/resolve loops.
//   write:   serialize obj's fields into buf (may intern more objects).
//   alloc:   allocate an object for pass 1; returns the pointer.  The object's
//            serialized bytes are provided so kinds with a struct discriminant
//            (e.g. AST nodes keyed by opcode) can peek at it to pick the right
//            concrete type.  The object need not be fully populated here.
//   read:    fill obj's fields from buf, resolving handles (pass 2).
//
typedef bool (*SerialWriteFn)(SerializeContext* ctx, WireBuffer* buf, void* obj);
typedef void* (*SerialAllocFn)(DeserializeContext* ctx, const void* blob,
                               size_t len);
typedef bool (*SerialReadFn)(DeserializeContext* ctx, WireBuffer* buf,
                             void* obj);

typedef struct {
  SerialWriteFn write;
  SerialAllocFn alloc;
  SerialReadFn read;
  const char* name;  // Human-readable kind name (for FIELDMETA / debugging).
} SerialKindVtable;

// Registers the vtable for a kind.  Safe to call more than once (idempotent).
void SerializeRegisterKind(SerialKind kind, const SerialKindVtable* vtable);
const SerialKindVtable* SerializeGetKindVtable(SerialKind kind);
// Registers all built-in kinds (implemented across the *_serialize.c files).
void SerializeRegisterAllKinds(void);

// Registers a field-descriptor table for a kind (for FIELDMETA emission).
void SerializeRegisterFields(SerialKind kind, const WireFieldDesc* fields,
                             size_t count);
const WireFieldDesc* SerializeGetFields(SerialKind kind, size_t* count);

//
// Pool I/O.  A pool record stream is: <count varint> then `count`
// length-delimited object records (raw serialized bytes per object).
//
// Writes the drained pool for `kind` into `out` (a length-delimited stream).
bool SerializeWritePool(SerializeContext* ctx, SerialKind kind, WireBuffer* out);
// Writes the string pool into `out`.
bool SerializeWriteStringPool(SerializeContext* ctx, WireBuffer* out);

// Pass 1: reads a pool stream for `kind`, allocating objects and recording their
// blobs for pass 2.
bool SerializeReadPool(DeserializeContext* ctx, SerialKind kind, WireBuffer* in);
// Reads the string pool.
bool SerializeReadStringPool(DeserializeContext* ctx, WireBuffer* in);
// Pass 2: resolves (fills) every object in every pool.
bool DeserializeContextResolve(DeserializeContext* ctx);

#endif /* serialize_h */
