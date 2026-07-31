//
//  serialize_common.h
//  c_compiler
//
//  Shared helpers used by the per-object serializers (type/symbol/ast).  These
//  wrap the low-level wire format with the graph interning conventions:
//  references become varint handles, strings become string-pool handles, and
//  vectors of references/strings become length-delimited sub-blobs.
//
//  The read helpers assume the field tag has already been consumed and the
//  payload (a varint or length-delimited blob) is next in the buffer.
//

#ifndef serialize_common_h
#define serialize_common_h

#include "dstring.h"
#include "serialize.h"
#include "vector.h"
#include "wireformat.h"

// --- Writing ---

// Writes a handle to an interned object (0 for NULL).
void SWriteRef(SerializeContext* ctx, WireBuffer* buf, int field,
               SerialKind kind, void* ptr);

// Writes a String* (or NULL) as a string-pool handle.
void SWriteStringPtr(SerializeContext* ctx, WireBuffer* buf, int field,
                     String* s);

// Writes an embedded String value as a string-pool handle.
void SWriteStringVal(SerializeContext* ctx, WireBuffer* buf, int field,
                     String* s);

// Writes a Vector of interned-object pointers as a length-delimited blob.
void SWriteRefVector(SerializeContext* ctx, WireBuffer* buf, int field,
                     SerialKind kind, Vector* v);

// Writes a Vector of String* as a length-delimited blob.
void SWriteStringVector(SerializeContext* ctx, WireBuffer* buf, int field,
                        Vector* v);

// --- Reading ---

// Reads a varint handle and resolves it to a pointer of the given kind.
void* SReadRef(DeserializeContext* ctx, WireBuffer* buf, SerialKind kind);

// Reads a string handle into a newly allocated String* (NULL for handle 0).
String* SReadStringPtr(DeserializeContext* ctx, WireBuffer* buf);

// Reads a string handle into an (already-initialized) embedded String.
void SReadStringVal(DeserializeContext* ctx, WireBuffer* buf, String* dest);

// Reads a length-delimited blob of handles into `out` (must be initialized),
// resolving each to a pointer of the given kind and appending it.
void SReadRefVector(DeserializeContext* ctx, WireBuffer* buf, SerialKind kind,
                    Vector* out);

// Reads a length-delimited blob of string handles into `out` (must be
// initialized), appending a newly allocated String* per entry.
void SReadStringVector(DeserializeContext* ctx, WireBuffer* buf, Vector* out);

// TemplateParameter vector (de)serialization, implemented in type_serialize.c
// and shared with symbol_serialize.c (variable templates).  `v` / `out` are
// embedded Vectors of TemplateParameter*.
void SerialWriteTemplateParameterVector(SerializeContext* ctx, WireBuffer* buf,
                                        int field, Vector* v);
void SerialReadTemplateParameterVector(DeserializeContext* ctx, WireBuffer* buf,
                                       Vector* out);

// TemplateArgument vector (de)serialization, implemented in type_serialize.c
// and shared with constraint_serialize.c (concept-id arguments).  The write
// side takes a `Vector*` of TemplateArgument*; the read side allocates and
// returns a fresh heap Vector* (NULL on failure/absence).
void SerialWriteTemplateArgumentVector(SerializeContext* ctx, WireBuffer* buf,
                                       int field, Vector* v);
Vector* SerialReadTemplateArgumentVector(DeserializeContext* ctx,
                                         WireBuffer* in);

// ClassTemplatePartialSpecialization vector (de)serialization, implemented in
// type_serialize.c and shared with symbol_serialize.c for variable templates.
void SerialWritePartialSpecializationVector(SerializeContext* ctx,
                                            WireBuffer* out, int field,
                                            Vector* specializations);
void SerialReadPartialSpecializationVector(DeserializeContext* ctx,
                                           WireBuffer* in, Vector* out);

#endif /* serialize_common_h */
