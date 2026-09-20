# C++20 Module File Format (`.dcm`)

This document describes the on-disk format of C++20 module interface files
(`.dcm`) produced by `ModuleWrite` and consumed by `ModuleLoad`
(`c_compiler/serialize/module_archive.{c,h}`). Inspect a file with
`moduledump` ([tools.md](tools.md)).

## 1. Big picture

A module interface file is a **System V `ar` archive** whose members are
independent **protobuf-wire-format** blobs. The design has two orthogonal
layers:

- **Container layer** — a plain `ar` archive (`c_compiler/AR/ar.h`). Each
  logical section is a named archive member. This makes the format
  self-describing, extensible (new members can be added without breaking old
  readers), and trivially compressible later (compress a member's payload and
  set a per-member flag).
- **Payload layer** — every member is encoded with hand-rolled protobuf wire
  primitives (varint / zigzag / tags / fixed / length-delimited). There are no
  `.proto` files and protoc is not involved; field numbers are assigned by hand
  and documented with `// @wire N` comments next to each struct field.

The file represents a **post-semantic, typed object graph** (symbols, types,
structs, enums, namespaces, and AST bodies), not source text.

## 2. Archive members

Defined in `module_archive.h` and `kPoolMembers` in `module_archive.c`:

| Member       | Contents |
|--------------|----------|
| `MODULE`     | Header: magic, format version, module name, target triple, compiler version, flags, and the exported root handles. |
| `STRINGS`    | The interned string pool. |
| `TYPES`      | `TypeRecord` pool. |
| `SYMBOLS`    | `Symbol` pool. |
| `STRUCTS`    | `Struct` pool. |
| `ENUMS`      | `Enum` pool. |
| `MEMBERS`    | `StructMember` pool. |
| `NAMESPACES` | `Namespace` pool. |
| `AST`        | `ASTNode` pool (function / template / default-argument bodies). |
| `FIELDMETA`  | *(optional)* dumped field-descriptor tables, for debugging and schema evolution. |

A dummy archive symbol `__dcm_module__` pointing at the `MODULE` member is always
emitted so the `ar` writer produces a symbol table and its precomputed file
offsets stay consistent.

## 3. The `MODULE` header member

The header begins with an 8-byte magic `"DCCMOD\x01\x00"`
(`MODULE_MAGIC`, `MODULE_MAGIC_LEN == 8`), followed by protobuf fields:

| Field | Number | Wire type | Meaning |
|-------|--------|-----------|---------|
| `format_version`   | 1 | varint            | Schema version (currently `1`). |
| `module_name`      | 2 | length-delimited  | Logical module name (may be empty). |
| `target_triple`    | 3 | length-delimited  | Target description (may be empty). |
| `compiler_version` | 4 | length-delimited  | Producer version string. |
| `flags`            | 5 | varint            | Module-unit kind (`primary interface`, `interface partition`, `internal partition`, or `header unit`). |
| `root_count`       | 6 | varint            | Advisory count of exported root symbols. |
| `root_handle`      | 7 | varint (repeated) | Pool handle of each exported root symbol. |
| `ns_root_count`    | 8 | varint            | Advisory count of exported root namespaces. |
| `ns_root_handle`   | 9 | varint (repeated) | Pool handle of each exported root namespace. |
| `dependency`       | 10 | length-delimited (repeated) | Direct logical module dependencies. |
| `reexport`         | 11 | length-delimited (repeated) | Direct dependencies made visible by `export import`. |
| `header_macro`     | 12 | length-delimited (repeated) | Macro definitions exported by a header unit. |

`format_version` is validated with a **strict-match** policy: a mismatch against
`MODULE_FORMAT_VERSION` is rejected. The `*_count` fields are advisory; the
authoritative roots come from the repeated `*_handle` fields.

## 4. Pool and string stream framing

Both the string pool and each object pool are a simple length-delimited stream:

- **Object pool:** `<count varint>` followed by `count` length-delimited
  records, each record being one object's serialized bytes.
- **String pool:** `<count varint>` followed by `count` length-delimited raw
  byte strings. Strings are **not** NUL-terminated on the wire; the loader
  appends a NUL when copying them into memory.

## 5. Wire primitives

Ported from protobuf (`wireformat.h`): base-128 **varints**, **zigzag** encoding
for signed integers, **tags** = `(field_number << 3) | wire_type`, and the
following wire types:

| Wire type            | Value | Notes |
|----------------------|-------|-------|
| `kWireVarint`        | 0     | varint-encoded integers / bools / enums |
| `kWireFixed64`       | 1     | fixed 8-byte |
| `kWireLengthDelimited` | 2   | strings, bytes, nested sub-messages |
| `kWireStartGroup`    | 3     | unsupported (present for completeness) |
| `kWireEndGroup`      | 4     | unsupported |
| `kWireFixed32`       | 5     | fixed 4-byte |

Errors are **sticky**: once a read or write fails, the buffer's `error` flag
latches and all later operations become no-ops, so callers can check the flag
once per batch. Unknown field numbers are skipped via `WireSkip`, which is what
enables forward-compatible schema evolution.

## 6. Object-graph model: handles and two passes

The graph is cyclic and shared, so it cannot be inlined recursively. Instead
(`serialize.h`):

- Every distinct object of an interned **kind** (`kSerialKindType`, `Symbol`,
  `Struct`, `Enum`, `StructMember`, `Namespace`, `AST`, plus `String`) is
  assigned a stable integer **handle** in a per-kind pool. Handle `0` denotes
  NULL; real handles are 1-based pool indices.
- Cross-references between objects are written as handles, not inline copies.
  This breaks cycles and de-duplicates shared objects (for example, the shared
  global namespace round-trips to a single object).

### Writing (`SerializeContext`)

1. Callers intern the root objects.
2. `SerializeContextDrain` walks each pool, serializing every object into its
   own byte record. Serializing an object may intern further objects, which are
   appended to the pools and processed in turn until the graph is closed.
3. `module_archive` emits each pool as a length-prefixed record stream.

### Reading (`DeserializeContext`, two-pass)

1. **Pass 1** (`SerializeReadPool`) reads each pool, allocating one empty object
   per handle (via the kind's `alloc`) and remembering its serialized bytes.
2. **Pass 2** (`DeserializeContextResolve`) fills each object's fields,
   resolving handle references to the pointers allocated in pass 1. Two passes
   are required so references to not-yet-read objects (including cycles) can be
   resolved.

Each kind registers a vtable `{write, alloc, read, name}`, and the framework
drives the generic drain / allocate / resolve loops.

## 7. Per-object encoding

Each object is a protobuf-style message with its own field-number space
(documented in the `*_serialize.c` files and mirrored by `// @wire N` comments in
the headers). Two reference styles are used:

- **Pooled references** (`SWriteRef` / `SReadRef`) — a varint handle into
  another pool. Used for the `Symbol` ↔ `Type` ↔ `Struct` ↔ `AST` ↔ `Namespace`
  edges.
- **Inline sub-messages** — uniquely-owned children written as a
  length-delimited nested message with their own local field numbers. Examples:
  `TemplateParameter`, `TemplateArgument`, `ArrayInfo`, `FunctionInfo`,
  `CXXBaseSpecifier`, `Attribute`, `VariableTemplate`, and the entire concepts
  graph (`ConstraintExpr`, `RequiresExpr`, `Requirement`, `Concept`) in
  `constraint_serialize.c`. Their inner pointers to Symbols / Types / AST nodes
  are still written as pooled handles.

Strings are always written as string-pool handles (`SWriteStringPtr` for a
`String*`, `SWriteStringVal` for an embedded `String`).

Codegen-only and transient fields (reference counts, `codegen_info`, DIEs,
`usage_info`, symbol-table maps) are deliberately **not** serialized; they are
recomputed on load. Namespace symbol/tag tables are stored as flat handle lists
and rebuilt with `NamespaceInsertSymbol` / `NamespaceInsertTag`.

### Symbol module-identity fields (format v1)

Added in milestone 5 without bumping `MODULE_FORMAT_VERSION`. Old archives omit
these fields; readers default linkage to external and leave module strings empty.

| Field | Number | Wire type | Meaning |
|-------|--------|-----------|---------|
| `cxx_linkage` | 48 | varint | `0` external, `1` internal, `2` module linkage. |
| `owning_module_name` | 49 | length-delimited | Named-module purview owner (may be empty). |
| `owning_module_partition` | 50 | length-delimited | Owning partition name (may be empty). |
| `import_source_module` | 51 | length-delimited | Import provenance stamped on install (usually empty on write). |

## 8. Loading requirements and lifetime

`ModuleLoad` requires an initialized `compiler` global, because deserialized
objects use compiler allocation and type registries. The returned `LoadedModule`
keeps the `DeserializeContext` and raw member byte buffers alive while imported
names are installed. `LoadedModuleReleaseGraph` detaches and destroys the loaded
graph after importer symbols have been uninstalled; `LoadedModuleDestruct`
then releases metadata, macro definitions, and member buffers.

## 9. Extensibility

- **New sections:** add an `ar` member; old readers ignore members they do not
  look for.
- **New fields:** assign a fresh `@wire` number; old readers skip unknown
  numbers.
- **Compression:** add a per-member flag and compress the payload (the hook is
  intentionally left open).
- **Incompatible schema break:** bump `MODULE_FORMAT_VERSION`; readers reject
  version mismatches today.

## 10. Producing and inspecting a module

The public driver can produce the object and module artifact together:

```
# Compile a module interface to coordinated object and .dcm outputs:
davecc -std=c++20 -target x86_64 -c \
  -fmodule-output foo.dcm foo.cppm -o foo.o

# Import a prebuilt module during a normal compile:
davecc -std=c++20 -target x86_64 -fprebuilt-module-path=. -c main.cpp -o main.o
```

`-Xemit-module` and `-Xload-module` remain available as low-level diagnostics.
See `docs/cxx20_modules.md` for dependency scanning, header units, partitions,
and Bazel rules.

Because the container is a standard `ar` archive, the member list can also be
inspected with ordinary archive tooling (e.g. `ar t foo.dcm`).

## 11. Related source files

- `c_compiler/serialize/module_archive.{c,h}` — container + `MODULE` header.
- `c_compiler/serialize/serialize.{c,h}` — graph framework, pools, two-pass I/O.
- `c_compiler/serialize/serialize_common.{c,h}` — shared ref/string/vector helpers.
- `c_compiler/serialize/wireformat.{c,h}` — protobuf wire primitives.
- `c_compiler/serialize/type_serialize.c` — `TypeRecord`, `Struct`, `Enum`, `StructMember`, and inline sub-objects.
- `c_compiler/serialize/symbol_serialize.c` — `Symbol`, `Namespace`, `VariableTemplate`.
- `c_compiler/serialize/constraint_serialize.{c,h}` — C++20 concepts / constraints graph.
- `c_compiler/serialize/ast_serialize.c` — `ASTNode` pool.
