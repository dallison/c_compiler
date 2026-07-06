//
//  constraint_serialize.h
//  c_compiler
//
//  (De)serialization for the C++20 concepts/constraints graph: ConstraintExpr,
//  RequiresExpr, Requirement and Concept.  These objects are uniquely owned by
//  their parent (a TemplateParameter, a FunctionInfo requires-clause, or a
//  concept Symbol), so they are written inline as length-delimited sub-messages
//  rather than as pooled, handle-referenced objects.  Cross-references they hold
//  to AST nodes, Symbols and TypeRecords are still written as pool handles.
//
//  The write helpers emit nothing when the object is NULL, so absence on read
//  naturally decodes back to NULL.  The read helpers consume the length-
//  delimited payload at the current buffer position (the field tag having
//  already been read by the caller).
//

#ifndef constraint_serialize_h
#define constraint_serialize_h

#include "serialize.h"
#include "wireformat.h"

struct ConstraintExpr;
struct Concept;

// ConstraintExpr, written under `field` only when `c != NULL`.
void SerialWriteConstraint(SerializeContext* ctx, WireBuffer* buf, int field,
                           struct ConstraintExpr* c);
struct ConstraintExpr* SerialReadConstraint(DeserializeContext* ctx,
                                            WireBuffer* in);

// Concept, written under `field` only when `c != NULL`.
void SerialWriteConcept(SerializeContext* ctx, WireBuffer* buf, int field,
                        struct Concept* c);
struct Concept* SerialReadConcept(DeserializeContext* ctx, WireBuffer* in);

#endif /* constraint_serialize_h */
