//
//  constraint_serialize.c
//  c_compiler
//
//  Serialization of the C++20 concepts/constraints graph.  See
//  constraint_serialize.h for the overall design (inline sub-messages that
//  still reference pooled AST/Symbol/Type objects by handle).
//
//  Field numbers below are local to each sub-message.  They are stable and
//  documented here; new fields must take fresh numbers so older modules remain
//  readable (unknown numbers are skipped).
//

#include "constraint_serialize.h"

#include <stdlib.h>

#include "ast.h"
#include "concepts.h"
#include "serialize_common.h"
#include "symbol.h"
#include "type.h"
#include "vector.h"

//
// ConstraintExpr sub-message field numbers.
//
enum {
  kCon_kind = 1,
  kCon_location = 2,
  kCon_atomic_expr = 3,        // kConstraintAtomic: ASTNode ref.
  kCon_left = 4,               // conjunction/disjunction: ConstraintExpr sub.
  kCon_right = 5,              // conjunction/disjunction: ConstraintExpr sub.
  kCon_concept_symbol = 6,     // kConstraintConceptId: Symbol ref.
  kCon_concept_arguments = 7,  // kConstraintConceptId: TemplateArgument vector.
  kCon_requires = 8,           // kConstraintRequires: RequiresExpr sub.
};

//
// RequiresExpr sub-message field numbers.
//
enum {
  kReq_location = 1,
  kReq_parameters = 2,    // Symbol ref vector (local parameters).
  kReq_requirements = 3,  // length-delimited [count][Requirement sub-msgs].
};

//
// Requirement sub-message field numbers.
//
enum {
  kRmt_kind = 1,
  kRmt_location = 2,
  kRmt_expr = 3,                     // simple/compound: ASTNode ref.
  kRmt_type = 4,                     // type requirement: TypeRecord ref.
  kRmt_is_noexcept = 5,             // compound.
  kRmt_return_type_constraint = 6,   // compound: ConstraintExpr sub.
  kRmt_nested = 7,                   // nested: ConstraintExpr sub.
};

//
// Concept sub-message field numbers.
//
enum {
  kCpt_name = 1,
  kCpt_template_parameters = 2,  // TemplateParameter vector.
  kCpt_constraint = 3,           // ConstraintExpr sub.
  kCpt_location = 4,
};

// Forward declarations for the recursive graph.
static void WriteRequiresInto(SerializeContext* ctx, WireBuffer* out,
                              RequiresExpr* re);
static RequiresExpr* ReadRequiresFrom(DeserializeContext* ctx, WireBuffer* in);

// ---------------------------------------------------------------------------
// ConstraintExpr.
// ---------------------------------------------------------------------------
static void WriteConstraintInto(SerializeContext* ctx, WireBuffer* out,
                                ConstraintExpr* c) {
  WireWriteInt32(out, kCon_kind, (int32_t)c->kind);
  WireWriteUint64(out, kCon_location, (uint64_t)c->location);
  switch (c->kind) {
    case kConstraintAtomic:
      SWriteRef(ctx, out, kCon_atomic_expr, kSerialKindAST, c->as.atomic.expr);
      break;
    case kConstraintConjunction:
    case kConstraintDisjunction:
      SerialWriteConstraint(ctx, out, kCon_left, c->as.binary.left);
      SerialWriteConstraint(ctx, out, kCon_right, c->as.binary.right);
      break;
    case kConstraintConceptId:
      SWriteRef(ctx, out, kCon_concept_symbol, kSerialKindSymbol,
                c->as.concept_id.concept_symbol);
      if (c->as.concept_id.arguments != NULL) {
        SerialWriteTemplateArgumentVector(ctx, out, kCon_concept_arguments,
                                          c->as.concept_id.arguments);
      }
      break;
    case kConstraintRequires:
      if (c->as.requires_.requires_expr != NULL) {
        WireBuffer sub;
        WireBufferInitOwned(&sub, 32);
        WriteRequiresInto(ctx, &sub, c->as.requires_.requires_expr);
        WireWriteBytes(out, kCon_requires, WireBufferData(&sub),
                       WireBufferSize(&sub));
        WireBufferDestruct(&sub);
      }
      break;
  }
}

void SerialWriteConstraint(SerializeContext* ctx, WireBuffer* buf, int field,
                           ConstraintExpr* c) {
  if (c == NULL) {
    return;
  }
  WireBuffer sub;
  WireBufferInitOwned(&sub, 32);
  WriteConstraintInto(ctx, &sub, c);
  WireWriteBytes(buf, field, WireBufferData(&sub), WireBufferSize(&sub));
  WireBufferDestruct(&sub);
}

static ConstraintExpr* ReadConstraintFrom(DeserializeContext* ctx,
                                          WireBuffer* in) {
  int32_t kind = kConstraintAtomic;
  SourceLocation location = 0;
  ASTNode* atomic_expr = NULL;
  ConstraintExpr* left = NULL;
  ConstraintExpr* right = NULL;
  Symbol* concept_symbol = NULL;
  Vector* concept_arguments = NULL;
  RequiresExpr* requires_expr = NULL;

  while (!WireBufferEof(in) && !WireBufferHasError(in)) {
    int field;
    WireType wt;
    if (!WireReadTag(in, &field, &wt)) {
      break;
    }
    switch (field) {
      case kCon_kind:
        WireReadInt32(in, &kind);
        break;
      case kCon_location: {
        uint64_t v;
        WireReadUint64(in, &v);
        location = (SourceLocation)v;
        break;
      }
      case kCon_atomic_expr:
        atomic_expr = (ASTNode*)SReadRef(ctx, in, kSerialKindAST);
        break;
      case kCon_left:
        left = SerialReadConstraint(ctx, in);
        break;
      case kCon_right:
        right = SerialReadConstraint(ctx, in);
        break;
      case kCon_concept_symbol:
        concept_symbol = (Symbol*)SReadRef(ctx, in, kSerialKindSymbol);
        break;
      case kCon_concept_arguments:
        concept_arguments = SerialReadTemplateArgumentVector(ctx, in);
        break;
      case kCon_requires: {
        const void* data;
        size_t dlen;
        if (WireReadBytes(in, &data, &dlen)) {
          WireBuffer sub;
          WireBufferInitReader(&sub, data, dlen);
          requires_expr = ReadRequiresFrom(ctx, &sub);
        }
        break;
      }
      default:
        WireSkip(in, wt);
        break;
    }
  }

  switch ((ConstraintExprKind)kind) {
    case kConstraintAtomic:
      return NewAtomicConstraint(atomic_expr, location);
    case kConstraintConjunction:
      return NewConjunctionConstraint(left, right, location);
    case kConstraintDisjunction:
      return NewDisjunctionConstraint(left, right, location);
    case kConstraintConceptId:
      return NewConceptIdConstraint(concept_symbol, concept_arguments,
                                    location);
    case kConstraintRequires:
      return NewRequiresConstraint(requires_expr, location);
  }
  // Unknown kind (from a newer producer): keep the graph well-formed.
  return NewAtomicConstraint(atomic_expr, location);
}

ConstraintExpr* SerialReadConstraint(DeserializeContext* ctx, WireBuffer* in) {
  const void* data;
  size_t len;
  if (!WireReadBytes(in, &data, &len)) {
    return NULL;
  }
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);
  return ReadConstraintFrom(ctx, &sub);
}

// ---------------------------------------------------------------------------
// Requirement (inline, as an element of a RequiresExpr requirement list).
// ---------------------------------------------------------------------------
static void WriteRequirementInto(SerializeContext* ctx, WireBuffer* out,
                                 Requirement* r) {
  WireWriteInt32(out, kRmt_kind, (int32_t)r->kind);
  WireWriteUint64(out, kRmt_location, (uint64_t)r->location);
  SWriteRef(ctx, out, kRmt_expr, kSerialKindAST, r->expr);
  SWriteRef(ctx, out, kRmt_type, kSerialKindType, r->type);
  WireWriteBool(out, kRmt_is_noexcept, r->is_noexcept);
  SerialWriteConstraint(ctx, out, kRmt_return_type_constraint,
                        r->return_type_constraint);
  SerialWriteConstraint(ctx, out, kRmt_nested, r->nested);
}

static Requirement* ReadRequirementFrom(DeserializeContext* ctx,
                                        WireBuffer* in) {
  int32_t kind = kRequirementSimple;
  SourceLocation location = 0;
  ASTNode* expr = NULL;
  TypeRecord* type = NULL;
  bool is_noexcept = false;
  ConstraintExpr* return_type_constraint = NULL;
  ConstraintExpr* nested = NULL;

  while (!WireBufferEof(in) && !WireBufferHasError(in)) {
    int field;
    WireType wt;
    if (!WireReadTag(in, &field, &wt)) {
      break;
    }
    switch (field) {
      case kRmt_kind:
        WireReadInt32(in, &kind);
        break;
      case kRmt_location: {
        uint64_t v;
        WireReadUint64(in, &v);
        location = (SourceLocation)v;
        break;
      }
      case kRmt_expr:
        expr = (ASTNode*)SReadRef(ctx, in, kSerialKindAST);
        break;
      case kRmt_type:
        type = (TypeRecord*)SReadRef(ctx, in, kSerialKindType);
        break;
      case kRmt_is_noexcept:
        WireReadBool(in, &is_noexcept);
        break;
      case kRmt_return_type_constraint:
        return_type_constraint = SerialReadConstraint(ctx, in);
        break;
      case kRmt_nested:
        nested = SerialReadConstraint(ctx, in);
        break;
      default:
        WireSkip(in, wt);
        break;
    }
  }

  switch ((RequirementKind)kind) {
    case kRequirementSimple:
      return NewSimpleRequirement(expr, location);
    case kRequirementType:
      return NewTypeRequirement(type, location);
    case kRequirementCompound:
      return NewCompoundRequirement(expr, is_noexcept, return_type_constraint,
                                    location);
    case kRequirementNested:
      return NewNestedRequirement(nested, location);
  }
  return NewSimpleRequirement(expr, location);
}

// ---------------------------------------------------------------------------
// RequiresExpr.
// ---------------------------------------------------------------------------
static void WriteRequirementVector(SerializeContext* ctx, WireBuffer* buf,
                                   int field, Vector* v) {
  WireBuffer tmp;
  WireBufferInitOwned(&tmp, 16);
  size_t length = v == NULL ? 0 : v->length;
  WireWriteRawVarint(&tmp, (uint64_t)length);
  for (size_t i = 0; i < length; i++) {
    WireBuffer elem;
    WireBufferInitOwned(&elem, 16);
    WriteRequirementInto(ctx, &elem, (Requirement*)VectorGet(v, i));
    WireWriteRawVarint(&tmp, (uint64_t)WireBufferSize(&elem));
    WireWriteRaw(&tmp, WireBufferData(&elem), WireBufferSize(&elem));
    WireBufferDestruct(&elem);
  }
  WireWriteBytes(buf, field, WireBufferData(&tmp), WireBufferSize(&tmp));
  WireBufferDestruct(&tmp);
}

static Vector* ReadRequirementVector(DeserializeContext* ctx, WireBuffer* in) {
  const void* data;
  size_t len;
  if (!WireReadBytes(in, &data, &len)) {
    return NULL;
  }
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);
  uint64_t count;
  if (!WireReadRawVarint(&sub, &count)) {
    return NULL;
  }
  Vector* out = NewVector();
  for (uint64_t i = 0; i < count; i++) {
    const void* elem;
    size_t elen;
    if (!WireReadBytes(&sub, &elem, &elen)) {
      break;
    }
    WireBuffer er;
    WireBufferInitReader(&er, elem, elen);
    VectorAppend(out, ReadRequirementFrom(ctx, &er));
  }
  return out;
}

static void WriteRequiresInto(SerializeContext* ctx, WireBuffer* out,
                              RequiresExpr* re) {
  WireWriteUint64(out, kReq_location, (uint64_t)re->location);
  if (re->parameters != NULL) {
    SWriteRefVector(ctx, out, kReq_parameters, kSerialKindSymbol,
                    re->parameters);
  }
  if (re->requirements != NULL) {
    WriteRequirementVector(ctx, out, kReq_requirements, re->requirements);
  }
}

static RequiresExpr* ReadRequiresFrom(DeserializeContext* ctx, WireBuffer* in) {
  SourceLocation location = 0;
  Vector* parameters = NULL;
  Vector* requirements = NULL;

  while (!WireBufferEof(in) && !WireBufferHasError(in)) {
    int field;
    WireType wt;
    if (!WireReadTag(in, &field, &wt)) {
      break;
    }
    switch (field) {
      case kReq_location: {
        uint64_t v;
        WireReadUint64(in, &v);
        location = (SourceLocation)v;
        break;
      }
      case kReq_parameters:
        parameters = NewVector();
        SReadRefVector(ctx, in, kSerialKindSymbol, parameters);
        break;
      case kReq_requirements:
        requirements = ReadRequirementVector(ctx, in);
        break;
      default:
        WireSkip(in, wt);
        break;
    }
  }
  return NewRequiresExpr(parameters, requirements, location);
}

// ---------------------------------------------------------------------------
// Concept.
// ---------------------------------------------------------------------------
void SerialWriteConcept(SerializeContext* ctx, WireBuffer* buf, int field,
                        Concept* c) {
  if (c == NULL) {
    return;
  }
  WireBuffer sub;
  WireBufferInitOwned(&sub, 64);
  SWriteStringVal(ctx, &sub, kCpt_name, &c->name);
  if (c->template_parameters != NULL) {
    SerialWriteTemplateParameterVector(ctx, &sub, kCpt_template_parameters,
                                       c->template_parameters);
  }
  SerialWriteConstraint(ctx, &sub, kCpt_constraint, c->constraint);
  WireWriteUint64(&sub, kCpt_location, (uint64_t)c->location);
  WireWriteBytes(buf, field, WireBufferData(&sub), WireBufferSize(&sub));
  WireBufferDestruct(&sub);
}

Concept* SerialReadConcept(DeserializeContext* ctx, WireBuffer* in) {
  const void* data;
  size_t len;
  if (!WireReadBytes(in, &data, &len)) {
    return NULL;
  }
  WireBuffer sub;
  WireBufferInitReader(&sub, data, len);

  String name;
  StringInit(&name, "");
  Vector* template_parameters = NULL;
  ConstraintExpr* constraint = NULL;
  SourceLocation location = 0;

  while (!WireBufferEof(&sub) && !WireBufferHasError(&sub)) {
    int field;
    WireType wt;
    if (!WireReadTag(&sub, &field, &wt)) {
      break;
    }
    switch (field) {
      case kCpt_name:
        SReadStringVal(ctx, &sub, &name);
        break;
      case kCpt_template_parameters:
        template_parameters = NewVector();
        SerialReadTemplateParameterVector(ctx, &sub, template_parameters);
        break;
      case kCpt_constraint:
        constraint = SerialReadConstraint(ctx, &sub);
        break;
      case kCpt_location: {
        uint64_t v;
        WireReadUint64(&sub, &v);
        location = (SourceLocation)v;
        break;
      }
      default:
        WireSkip(&sub, wt);
        break;
    }
  }

  Concept* c = NewConcept(name.value, template_parameters, constraint, location);
  StringDestruct(&name);
  return c;
}
