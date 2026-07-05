//
//  module_archive.h
//  c_compiler
//
//  Container format for C++20 module interface files (".dcm").  A module file
//  is a System V AR archive (see c_compiler/AR/ar.h) whose members are each an
//  independent protobuf-wire-format blob:
//
//    MODULE      -- header: magic, format version, module name, target triple,
//                   compiler version, flags, and the exported root symbol
//                   handles.
//    STRINGS     -- the interned string pool.
//    TYPES       -- TypeRecord pool.
//    SYMBOLS     -- Symbol pool.
//    STRUCTS     -- Struct pool.
//    ENUMS       -- Enum pool.
//    MEMBERS     -- StructMember pool.
//    NAMESPACES  -- Namespace pool.
//    AST         -- ASTNode pool (inline/template/default-argument bodies).
//    FIELDMETA   -- (optional) dumped field-descriptor tables, for debugging
//                   and schema evolution.
//
//  Named members keep the format extensible (new sections can be added without
//  breaking existing readers) and make compression trivially addable later
//  (compress a member payload and set a per-member flag).
//
//  Loading requires an initialized `compiler` global because deserialized
//  objects are allocated from the compiler's arenas (NewSymbol/NewTypeRecord/
//  ...).  Language-level module/import/export semantics and BMI discovery are
//  out of scope for this phase; see the plan.
//

#ifndef module_archive_h
#define module_archive_h

#include <stdbool.h>
#include <stdint.h>

#include "dstring.h"
#include "serialize.h"
#include "vector.h"

// 8-byte magic written at the start of the MODULE member.
#define MODULE_MAGIC "DCCMOD\x01\x00"
#define MODULE_MAGIC_LEN 8

// Bumped whenever the on-wire schema changes incompatibly.  Readers reject a
// mismatching version (strict-match policy to start).
#define MODULE_FORMAT_VERSION 1u

// Parameters describing the module to write.
typedef struct {
  const char* module_name;    // Logical module name (may be NULL/empty).
  const char* target_triple;  // Target description (may be NULL/empty).
  const char* compiler_version;  // Producer version string (may be NULL).
  uint32_t flags;             // Reserved for future use (0 for now).
  Vector* root_symbols;       // Vector of Symbol* to export (may be NULL).
  Vector* root_namespaces;    // Vector of Namespace* to export (may be NULL).
} ModuleWriteRequest;

// Serializes the object graph reachable from req->root_symbols and writes it to
// `path` as a ".dcm" AR archive.  Returns false on any I/O or serialization
// error.  Requires an initialized `compiler` global.
bool ModuleWrite(const char* path, const ModuleWriteRequest* req);

// The result of loading a module.  Keeps the DeserializeContext alive because
// the loaded objects reference its string pool (via interned copies) and
// resolution vectors.  Call LoadedModuleDestruct when done.
typedef struct {
  String module_name;
  String target_triple;
  String compiler_version;
  uint32_t format_version;
  uint32_t flags;

  // Exported roots (Vector of Symbol*), pointing into the loaded graph.
  Vector root_symbols;

  // Exported namespace roots (Vector of Namespace*), pointing into the graph.
  Vector root_namespaces;

  // Owns the loaded object pools; kept for the lifetime of the loaded symbols.
  DeserializeContext ctx;

  // Raw member content blobs (void*), kept alive because the deserialized
  // objects and ctx hold pointers into them.  Freed by LoadedModuleDestruct.
  Vector owned_buffers;
} LoadedModule;

// Reads a ".dcm" archive from `path`, deserializes the full graph, and fills
// `out`.  Returns false on any I/O, format, or deserialization error (in which
// case `out` is left destructed).  Requires an initialized `compiler` global.
bool ModuleLoad(const char* path, LoadedModule* out);

// Releases resources held by a LoadedModule.  The deserialized compiler objects
// themselves are arena-owned and are not freed here.
void LoadedModuleDestruct(LoadedModule* out);

#endif /* module_archive_h */
