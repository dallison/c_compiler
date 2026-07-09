//
//  type.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <assert.h>
#include "concepts.h"
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "compiler.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"

static int next_type_id = 0;

static ASTNode* IdentityCloneNode(ASTNode* node, void* data);

//
// TypeRecord arena allocator.
//
// TypeRecords are reference counted and shared widely (AST nodes, symbols, IR,
// target instructions) with ownership that is hard to balance precisely, so a
// stray ref leaves the 96-byte struct leaked at exit.  Like the AST, every
// TypeRecord struct is allocated from a bump arena and the whole arena is freed
// in one shot at CompilerDestruct (TypeRecordArenaRelease).  Reference counting
// still drives release of each record's *owned* auxiliary resources (function
// prototype symbols, struct/enum info, VLA size AST); only the struct itself is
// no longer individually free()d.
typedef struct TypeArenaBlock {
  struct TypeArenaBlock* next;
  size_t used;
  size_t capacity;
  char data[];
} TypeArenaBlock;

#define TYPE_ARENA_BLOCK_SIZE (256 * 1024)

static TypeArenaBlock* type_arena = NULL;

static TypeArenaBlock* NewTypeArenaBlock(size_t capacity) {
  TypeArenaBlock* block = malloc(capacity + sizeof(TypeArenaBlock));
  block->next = NULL;
  block->used = 0;
  block->capacity = capacity;
  return block;
}

static TypeRecord* TypeArenaAlloc(void) {
  size_t aligned = (sizeof(TypeRecord) + 15) & ~(size_t)15;
  if (type_arena == NULL || type_arena->used + aligned > type_arena->capacity) {
    TypeArenaBlock* block = NewTypeArenaBlock(TYPE_ARENA_BLOCK_SIZE);
    block->next = type_arena;
    type_arena = block;
  }
  void* p = type_arena->data + type_arena->used;
  type_arena->used += aligned;
  return (TypeRecord*)p;
}

// Free every TypeRecord struct.  Call only after all type-referencing data
// structures have been torn down (their TypeRecordDelete calls have released
// the auxiliary resources); the struct memory becomes invalid afterwards.
void TypeRecordArenaRelease(void) {
  TypeArenaBlock* block = type_arena;
  while (block != NULL) {
    TypeArenaBlock* next = block->next;
    free(block);
    block = next;
  }
  type_arena = NULL;
}

// Registries of every Struct and Enum created.  Struct infos can form
// reference cycles (a member of type 'struct S *' inside 'struct S' takes a
// reference on S's own struct_info), so refcount-driven freeing in
// TypeRecordDelete never reaches zero for them.  Instead, every struct and enum
// info is freed in one bulk pass at end of compilation, after all
// type-referencing structures have been torn down.
static Vector struct_registry;
static bool struct_registry_initialized = false;
static Vector enum_registry;
static bool enum_registry_initialized = false;

static void StructTeardownMembers(Struct* s);

// Free every Struct and Enum info in one pass.  Must be called after the AST,
// symbol tables and tags are gone but before TypeRecordArenaRelease, because
// tearing down a struct's members deletes member symbols which decref
// TypeRecords (and may decref other struct/enum infos).
void StructRegistryRelease(void) {
  // Phase 1: tear down every struct's members.  This deletes member symbols,
  // whose types may decref OTHER struct/enum infos.  Every info is still
  // allocated at this point (none are freed below), so those decrements are
  // safe.  TypeRecordDelete never frees struct/enum infos itself.
  for (size_t i = 0; i < struct_registry.length; i++) {
    StructTeardownMembers((Struct*)struct_registry.value.p[i]);
  }
  // Phase 2: now that no more decrements will occur, free the info structs.
  for (size_t i = 0; i < struct_registry.length; i++) {
    free(struct_registry.value.p[i]);
  }
  for (size_t i = 0; i < enum_registry.length; i++) {
    EnumDelete((Enum*)enum_registry.value.p[i]);
  }
  if (struct_registry_initialized) {
    VectorDestruct(&struct_registry);
    struct_registry_initialized = false;
  }
  if (enum_registry_initialized) {
    VectorDestruct(&enum_registry);
    enum_registry_initialized = false;
  }
}

static void Trap(TypeRecord* r) {
  if (r->id == 1234) {
    printf("");
  }
}

// The size of a pointer depends on the machine architecture.
int SizeofBool(void) {
  return compiler->bool_size;
}

int SizeofShort(void) {
  return compiler->short_size;
}

int SizeofChar(void) {
  return 1;
}

int SizeofVoid(void) {
  return 1;
}


int SizeofPointer(void) {
  return compiler->pointer_size;
}

int SizeofInt(void) {
  return compiler->int_size;
}

int SizeofLong(void) {
  return compiler->long_size;
}

int SizeofLongLong(void) {
  return compiler->long_long_size;
}

int SizeofFloat(void) {
  return compiler->float_size;
}

int SizeofDouble(void) {
  return compiler->double_size;
}

// Mapping of type to its size in bytes.
static struct {
  Type type;
  int (*func)(void);
} type_sizes[] = {
  {kTypeChar, SizeofChar},
  {kTypeBool, SizeofBool},
  {kTypeShort, SizeofShort},
  {kTypeLong, SizeofLong},
  {kTypeLongLong, SizeofLongLong},
  {kTypeFloat, SizeofFloat},
  {kTypeDouble, SizeofDouble},
  {kTypeLongDouble, SizeofDouble},
  {kTypeInt, SizeofInt},
  {kTypeVoid, SizeofVoid},
  {kTypeImplicit, 0},
};


// What is the size in bytes of the given type?  Uses the mapping
// above.  Returns the size or zero if the type isn't known.
int SizeofType(Type type) {
  for (int i = 0; type_sizes[i].type != kTypeImplicit; i++) {
    if ((type & type_sizes[i].type) != 0) {
       return type_sizes[i].func();
    }
  }
  if ((type & kTypeEnum) != 0) {
    // Enums are variable in size.
    if ((type & kTypeChar) != 0) {
      return SizeofChar();
    }
    return SizeofInt();
  }
  return SizeofPointer();
}



TypeRecord* NewTypeRecord(Type type, Qualifiers quals) {
  // If a plain unsigned or signed is found, this implies an int.
  if (type == kTypeUnsigned || type == kTypeSigned) {
    type |= kTypeInt;
  }
  TypeRecord* record = TypeArenaAlloc();
  record->id = next_type_id++;
  record->type = type;
  record->qualifiers = quals;
  record->size = 0;
  record->template_parameter_index = -1;
  record->dependent_member_name = NULL;
  record->template_origin = NULL;
  record->template_arguments = NULL;
  record->refs = 0;
  record->next = NULL;
  record->declarator = kDeclPrimitive;
  memset(&record->info, 0, sizeof(record->info));
  Trap(record);
  return record;
}

// Deletes a TypeRecord with regard to the reference count.  The
// reference count is decremented and if it goes to zero the
// record can be deleted.  When deleting it, the record pointed
// to by the 'next' field is first deleted (using the same function)
// and then the memory is freed.
void TypeRecordDelete(TypeRecord* record) {
  if (record == NULL) {
    return;
  }
  TypeRecordDecRef(record);
  if (record->refs == 0) {
    if (record->next != NULL) {
      TypeRecordDelete(record->next);
      record->next = NULL;
    }
    if (record->template_arguments != NULL) {
      VectorDeleteWithContents(record->template_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      record->template_arguments = NULL;
    }
    if (record->dependent_member_name != NULL) {
      StringDelete(record->dependent_member_name);
      record->dependent_member_name = NULL;
    }
    // Delete type-specific info if refs goes to zero.
    if (TypeIsStructOrUnion(record)) {
      // Struct infos are not freed here: they can form reference cycles, so
      // they are all freed in one pass by StructRegistryRelease at end of
      // compilation.  Keep the count balanced for any code that reads it.
      record->info.struct_info->refs--;
    } else if (TypeIsEnum(record)) {
      // Enum infos are freed in bulk by StructRegistryRelease (alongside
      // structs) so that decrements during the bulk teardown never touch an
      // already-freed info.  Keep the count balanced for any reader.
      record->info.enum_info->refs--;
    } else if (TypeIsFunction(record)) {
      TypeRecordDelete(record->info.function.coroutine_promise_type);
      record->info.function.coroutine_promise_type = NULL;
      TypeRecordDelete(record->info.function.coroutine_frame_type);
      record->info.function.coroutine_frame_type = NULL;
      VectorDestructWithContents(&record->info.function.prototype,
                                (VectorElementDestructor)SymbolDestruct, /*free_element=*/true);
      VectorDestructWithContents(&record->info.function.template_parameters,
                                 (VectorElementDestructor)TemplateParameterDelete,
                                 /*free_element=*/false);
      VectorDestructWithContents(&record->info.function.template_instantiations,
                                 (VectorElementDestructor)SymbolDelete,
                                 /*free_element=*/false);
      ConstraintExprDelete(record->info.function.associated_constraint);
      record->info.function.associated_constraint = NULL;
    } else if (TypeIsVLA(record)) {
      // Delete the AST containing the size.
      ASTNodeDelete(record->info.array.size.vla.size);
    }
    // The struct itself lives in the type arena and is reclaimed wholesale by
    // TypeRecordArenaRelease; only its owned auxiliary resources are freed here.
  }
}

int TypeRecordAlignment(TypeRecord* record) {
  if (compiler->alignment == 1) {
    // No alignment necessary for this target.
    return 1;
  }
  switch (record->declarator) {
    case kDeclArray:
      return TypeRecordAlignment(record->next);
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return SizeofPointer();
    case kDeclFunction:
      return SizeofPointer();
      break;
    case kDeclPrimitive:
      if (TypeIsStructOrUnion(record)) {
        int a = record->info.struct_info->alignment;
        return a > 0 ? a : 1;
      }
      return SizeofType(record->type);
  }
}

void TypeRecordIncRef(TypeRecord* record) {
  if (record == NULL) {
    return;
  }
  record->refs++;
}

void TypeRecordDecRef(TypeRecord* record) {
  if (record == NULL) {
    return;
  }
  record->refs--;
}

TypeRecord* TypeRecordCalculateSize(TypeRecord* record) {
  if (record == NULL) {
    return NULL;
  }
  TypeRecordCalculateSize(record->next);
  if (record->size == 0) {
    switch (record->declarator) {
      case kDeclArray:
        if (!record->info.array.is_vla) {
          record->size = record->info.array.size.fixed * record->next->size;
        }
        break;
      case kDeclPointer:
      case kDeclReference:
      case kDeclRValueReference:
        record->size = SizeofPointer();
        break;
      case kDeclFunction:
        record->size = SizeofPointer();
        break;
      case kDeclPrimitive:
        if (TypeIsStructOrUnion(record)) {
          if (record->info.struct_info != NULL) {
            record->size = record->info.struct_info->size;
          }
        } else {
          record->size = SizeofType(record->type);
        }
        break;
    }
  }
  return record;
}

TypeRecord* NewTypeRecordWithSize(Type type, Qualifiers quals) {
  return TypeRecordCalculateSize(NewTypeRecord(type, quals));
}

// Join two type records through the next field.  This increments
// the reference count on the one pointed to.
void TypeRecordChain(TypeRecord* from, TypeRecord* to) {
  TypeRecordIncRef(to);
  from->next = to;
}

static ASTNode* CloneVLAExpr(ASTNode* node, void* data) {
  return node;
}

/* Deep-copy a template parameter declaration (name, kind, pack flag, and any
 * default type/value), including its placeholder type record. */
static TemplateParameter* TemplateParameterCopy(TemplateParameter* param) {
  if (param == NULL) {
    return NULL;
  }
  TemplateParameter* copy = malloc(sizeof(TemplateParameter));
  StringInit(&copy->name, param->name.value);
  copy->kind = param->kind;
  copy->is_parameter_pack = param->is_parameter_pack;
  copy->type = param->type != NULL ? TypeRecordCopy(param->type) : NULL;
  copy->default_type =
      param->default_type != NULL ? TypeRecordCopy(param->default_type) : NULL;
  copy->has_default_int = param->has_default_int;
  copy->default_int_value = param->default_int_value;
  copy->default_template_parameter_index =
      param->default_template_parameter_index;
  // Constraint cloning is introduced with full associated-constraint semantics.
  copy->associated_constraint = NULL;
  copy->index = param->index;
  return copy;
}

/* Deep-copy a template argument (a concrete type/value, an unresolved parameter
 * reference, or a pack of arguments) used when instantiating templates. */
static TemplateArgument* TemplateArgumentCopy(TemplateArgument* arg) {
  if (arg == NULL) {
    return NULL;
  }
  TemplateArgument* copy = malloc(sizeof(TemplateArgument));
  copy->kind = arg->kind;
  copy->is_pack_expansion = arg->is_pack_expansion;
  copy->type = arg->type != NULL ? TypeRecordCopy(arg->type) : NULL;
  copy->int_value = arg->int_value;
  copy->template_parameter_index = arg->template_parameter_index;
  copy->pack_arguments = TemplateArgumentVectorCopy(arg->pack_arguments);
  // The dependent expression is arena-owned and only read (cloned) at
  // re-evaluation, so the pointer may be shared across copies.
  copy->dependent_expr = arg->dependent_expr;
  copy->location = arg->location;
  return copy;
}

TemplateArgument* NewTypeTemplateArgument(TypeRecord* type) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->template_parameter_index = -1;
  arg->type = type != NULL ? TypeRecordCopy(type) : NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

/* Deep-copy a vector of template arguments (NULL-safe). */
Vector* TemplateArgumentVectorCopy(Vector* args) {
  if (args == NULL) {
    return NULL;
  }
  Vector* copy = NewVector();
  for (size_t i = 0; i < args->length; i++) {
    VectorAppend(copy, TemplateArgumentCopy(args->value.p[i]));
  }
  return copy;
}

// Copy a type record and chain it to its existing next,
// incrementing the ref count.
TypeRecord* TypeRecordCopy(TypeRecord* record) {
  TypeRecord* r = TypeArenaAlloc();
  memcpy(r, record, sizeof(TypeRecord));
  r->id = next_type_id;
  r->refs = 0;  // No refs to this yet.
  r->dependent_member_name = record->dependent_member_name != NULL
      ? NewString(record->dependent_member_name->value)
      : NULL;
  if (r->next != NULL) {
    TypeRecordIncRef(r->next);  // Another ref to next.
  }
  r->template_arguments = TemplateArgumentVectorCopy(record->template_arguments);
  if (TypeIsFunction(record)) {
    VectorInit(&r->info.function.prototype);
    for (size_t i = 0; i < record->info.function.prototype.length; i++) {
      Symbol* formal = record->info.function.prototype.value.p[i];
      if (formal == NULL) {
        VectorAppend(&r->info.function.prototype, NULL);
        continue;
      }
      Symbol* clone = NewSymbol(formal->name.value, formal->type,
                                formal->storage);
      clone->flags = formal->flags;
      clone->location = formal->location;
      clone->alignment = formal->alignment;
      clone->namespace_ = formal->namespace_;
      clone->value = formal->value;
      clone->default_argument =
          ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
      VectorAppend(&r->info.function.prototype, clone);
    }
    VectorInit(&r->info.function.template_parameters);
    for (size_t i = 0; i < record->info.function.template_parameters.length; i++) {
      VectorAppend(&r->info.function.template_parameters,
                   TemplateParameterCopy(
                       record->info.function.template_parameters.value.p[i]));
    }
    VectorInit(&r->info.function.template_instantiations);
    // Constraint cloning is introduced with full associated-constraint
    // semantics; avoid sharing ownership across function type copies.
    r->info.function.associated_constraint = NULL;
    TypeRecordIncRef(r->info.function.coroutine_promise_type);
    TypeRecordIncRef(r->info.function.coroutine_frame_type);
  }
  // Increment ref counts for type-specific objects.
  if (TypeIsStructOrUnion(record)) {
    record->info.struct_info->refs++;
  } else if (TypeIsEnum(record)) {
    record->info.enum_info->refs++;
  } else if (TypeIsVLA(record)) {
    // Copy the expression AST.
    r->info.array.size.vla.size =
      ASTNodeClone(record->info.array.size.vla.size, CloneVLAExpr, NULL, NULL);
  }
  return r;
}

//
// Type creation functions.
//

TypeRecord* NewPointerTypeRecord(Qualifiers quals) {
  TypeRecord* t = NewTypeRecord(kTypeImplicit, quals);
  t->declarator = kDeclPointer;
  t->size = compiler->pointer_size;
  return t;
}

TypeRecord* NewReferenceTypeRecord(Qualifiers quals, bool rvalue) {
  TypeRecord* t = NewTypeRecord(kTypeImplicit, quals);
  t->declarator = rvalue ? kDeclRValueReference : kDeclReference;
  t->size = compiler->pointer_size;
  return t;
}

TypeRecord* NewPointerTo(Qualifiers quals, TypeRecord* type) {
  TypeRecord* ptr = NewPointerTypeRecord(quals);
  TypeRecordChain(ptr, type);
  return ptr;
}

Symbol* NewCXXThisSymbol(Struct* owner, bool is_const_member,
                         SourceLocation location) {
  TypeRecord* class_type =
      NewTypeRecord(kTypeStruct, is_const_member ? kQualConst : kQualPlain);
  class_type->info.struct_info = owner;
  TypeRecord* this_type = NewPointerTo(kQualPlain, class_type);
  Symbol* this_symbol = NewSymbol("this", this_type, STO(implicit));
  this_symbol->flags.is_argument = true;
  this_symbol->flags.invented = true;
  this_symbol->flags.is_defined = true;
  this_symbol->value.arg_number = 0;
  this_symbol->location = location;
  return this_symbol;
}

static Symbol* NewCXXCompleteObjectSymbol(SourceLocation location) {
  Symbol* symbol =
      NewSymbol("__complete_object",
                NewTypeRecordWithSize(kTypeInt, kQualPlain), STO(implicit));
  symbol->flags.is_argument = true;
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = location;
  return symbol;
}

void TypeRecordAddCXXThisParameter(TypeRecord* func, Struct* owner,
                                   SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) || owner == NULL ||
      func->info.function.cxx_member_owner != NULL) {
    return;
  }
  Symbol* this_symbol =
      NewCXXThisSymbol(owner, func->info.function.is_const_member, location);
  if (func->info.function.prototype.length == 0) {
    VectorAppend(&func->info.function.prototype, this_symbol);
  } else {
    VectorInsertBefore(&func->info.function.prototype, 0, this_symbol);
  }
  if ((func->info.function.is_constructor ||
       func->info.function.is_destructor) &&
      StructHasVirtualBases(owner)) {
    Symbol* complete_object = NewCXXCompleteObjectSymbol(location);
    if (func->info.function.prototype.length <= 1) {
      VectorAppend(&func->info.function.prototype, complete_object);
    } else {
      VectorInsertBefore(&func->info.function.prototype, 1, complete_object);
    }
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    formal->value.arg_number = (int32_t)i;
  }
  func->info.function.cxx_member_owner = owner;
}

TypeRecord* NewArrayTypeRecord(Qualifiers quals, bool is_static) {
  TypeRecord* t = NewTypeRecord(kTypeImplicit, quals);
  t->declarator = kDeclArray;
  t->info.array.size.fixed = 0;
  t->info.array.is_flexible = false;
  t->info.array.is_static = is_static;
  t->info.array.size.vla.size = NULL;
  t->info.array.size.vla.codegen_info = NULL;
  t->info.array.is_vla = false;
  t->info.array.is_placeholder_vla = false;
  t->info.array.template_parameter_index = -1;
  t->size = 0;  // Don't know yet.
  return t;
}

TypeRecord* NewBasicArrayTypeRecord(Qualifiers quals, int size, bool is_flexible) {
  TypeRecord* t = NewArrayTypeRecord(quals, false);
  t->info.array.size.fixed = size;
  t->info.array.is_flexible = is_flexible;
  return t;
}

TypeRecord* NewFunctionTypeRecord() {
  TypeRecord* t = NewTypeRecord(kTypeImplicit, kQualPlain);
  t->declarator = kDeclFunction;
  t->size = 0;
  t->info.function.symbol = NULL;
  t->info.function.varargs = false;
  t->info.function.unknown_args = false;
  t->info.function.definition = false;
  t->info.function.is_constructor = false;
  t->info.function.is_destructor = false;
  t->info.function.is_const_member = false;
  t->info.function.ref_qualifier = kCXXRefQualifierNone;
  t->info.function.is_explicit = false;
  t->info.function.is_explicit_conversion = false;
  t->info.function.is_virtual = false;
  t->info.function.is_override = false;
  t->info.function.is_final = false;
  t->info.function.is_pure_virtual = false;
  t->info.function.is_defaulted = false;
  t->info.function.is_deleted = false;
  t->info.function.cxx_special_member_kind = kCXXSpecialMemberNone;
  t->info.function.is_user_declared = false;
  t->info.function.is_user_provided = false;
  t->info.function.is_explicitly_defaulted = false;
  t->info.function.is_explicitly_deleted = false;
  t->info.function.is_implicitly_declared = false;
  t->info.function.is_implicitly_deleted = false;
  t->info.function.is_trivial_special_member = false;
  t->info.function.is_constexpr_eligible = false;
  t->info.function.is_noexcept_eligible = true;
  t->info.function.is_noexcept = false;
  t->info.function.is_auto_return_deduced = false;
  t->info.function.is_deduction_guide = false;
  t->info.function.is_coroutine = false;
  t->info.function.coroutine_promise_type = NULL;
  t->info.function.coroutine_frame_type = NULL;
  t->info.function.coroutine_suspend_count = 0;
  t->info.function.virtual_index = -1;
  t->info.function.cxx_member_owner = NULL;
  t->info.function.template_origin = NULL;
  t->info.function.template_parameter_count = 0;
  t->info.function.template_parameter_base = 0;
  t->info.function.old_style = false;
  t->info.function.is_inline = false;
  t->info.function.is_constexpr = false;
  t->info.function.is_consteval = false;
  t->info.function.body = NULL;
  VectorInit(&t->info.function.prototype);
  VectorInit(&t->info.function.template_parameters);
  VectorInit(&t->info.function.template_instantiations);
  t->info.function.associated_constraint = NULL;
  t->info.function.explicit_condition = NULL;
  return t;
}

TypeRecord* NewSizeTypeRecord() {
  // size_t must match the headers' typedef (`unsigned long` on LP64 targets,
  // `unsigned int` on 32-bit) so that an explicitly declared `operator new`
  // / `operator delete` (e.g. from <new>) mangles identically to the implicit
  // allocation function the compiler synthesises for `new`/`delete`
  // expressions.  On LP64 this yields the Itanium ABI names `_Znwm` / `_Znam`.
  if (compiler->pointer_size == 8) {
    return NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain);
  }
  return NewTypeRecordWithSize(kTypeInt | kTypeUnsigned, kQualPlain);
}

StructMember* NewStructMember(Symbol* symbol) {
  StructMember* mem = malloc(sizeof(StructMember));
  mem->symbol = symbol;
  mem->default_initializer = NULL;
  mem->byte_offset = 0;
  mem->bit_offset = 0;
  mem->bit_size = 0;
  mem->cxx_vcall_offset = 0;
  mem->is_anon = false;
  mem->is_static = false;
  mem->is_mutable = false;
  mem->is_member_function = false;
  mem->is_using_declaration = false;
  mem->access = kAccessPublic;
  mem->overload_next = NULL;
  return mem;
}

/* Free a template parameter and its owned type records. */
void TemplateParameterDelete(TemplateParameter* param) {
  if (param == NULL) {
    return;
  }
  StringDestruct(&param->name);
  TypeRecordDelete(param->type);
  TypeRecordDelete(param->default_type);
  ConstraintExprDelete(param->associated_constraint);
  free(param);
}

/* Free a template argument, recursively freeing any pack elements. */
void TemplateArgumentDelete(TemplateArgument* arg) {
  if (arg == NULL) {
    return;
  }
  TypeRecordDelete(arg->type);
  if (arg->pack_arguments != NULL) {
    VectorDeleteWithContents(arg->pack_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  free(arg);
}

/* Create a record describing a class-template partial specialization: its own
 * template parameters and the argument pattern (e.g. `<T, T*>`) that incoming
 * instantiation arguments are matched against. */
static ClassTemplatePartialSpecialization* NewClassTemplatePartialSpecialization(
    Symbol* tag_symbol, Vector* template_parameters, Vector* pattern_arguments) {
  ClassTemplatePartialSpecialization* partial =
      malloc(sizeof(ClassTemplatePartialSpecialization));
  partial->tag_symbol = tag_symbol;
  VectorInit(&partial->template_parameters);
  VectorInit(&partial->pattern_arguments);
  for (size_t i = 0; template_parameters != NULL &&
                     i < template_parameters->length; i++) {
    VectorAppend(&partial->template_parameters,
                 TemplateParameterCopy(template_parameters->value.p[i]));
  }
  for (size_t i = 0; pattern_arguments != NULL &&
                     i < pattern_arguments->length; i++) {
    VectorAppend(&partial->pattern_arguments,
                 TemplateArgumentCopy(pattern_arguments->value.p[i]));
  }
  return partial;
}

/* Free a partial-specialization record and its parameter/pattern vectors. */
static void ClassTemplatePartialSpecializationDelete(
    ClassTemplatePartialSpecialization* partial) {
  if (partial == NULL) {
    return;
  }
  VectorDestructWithContents(&partial->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(&partial->pattern_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  free(partial);
}

static CXXBaseSpecifier* NewCXXBaseSpecifier(TypeRecord* type,
                                             CXXAccess access,
                                             bool is_virtual) {
  CXXBaseSpecifier* base = malloc(sizeof(CXXBaseSpecifier));
  base->type = type;
  TypeRecordIncRef(type);
  base->access = access;
  base->byte_offset = 0;
  base->is_virtual = is_virtual;
  base->is_pack_expansion = false;
  return base;
}

static void CXXBaseSpecifierDelete(CXXBaseSpecifier* base) {
  TypeRecordDelete(base->type);
  free(base);
}

static void CXXMemberUsingDeclarationDelete(CXXMemberUsingDeclaration* decl) {
  if (decl == NULL) {
    return;
  }
  TypeRecordDelete(decl->base_type);
  StringDestruct(&decl->member_name);
  free(decl);
}

static void LayoutCXXBaseSpecifiers(Struct* str);
static void CollectCXXVirtualBases(Struct* str);
static void CopyCXXBaseVirtualMembers(Struct* str);

static CXXVirtualBaseInfo* NewCXXVirtualBaseInfo(TypeRecord* type,
                                                CXXAccess access,
                                                int vbtable_index) {
  CXXVirtualBaseInfo* base = malloc(sizeof(CXXVirtualBaseInfo));
  base->type = type;
  TypeRecordIncRef(type);
  base->access = access;
  base->byte_offset = 0;
  base->vbtable_index = vbtable_index;
  return base;
}

static void CXXVirtualBaseInfoDelete(CXXVirtualBaseInfo* base) {
  TypeRecordDelete(base->type);
  free(base);
}

static void CXXVBTableInfoDelete(CXXVBTableInfo* info) {
  free(info);
}

static void CXXVTableInfoDelete(CXXVTableInfo* info) {
  free(info);
}

void StructMemberDelete(StructMember* member) {
  if (member->default_initializer != NULL) {
    ASTNodeDelete(member->default_initializer);
  }
  SymbolDelete(member->symbol);
  free(member);
}

bool StructMemberIsBitField(StructMember* member) {
  return member->bit_size > 0;
}

static int CompareStructMember(const void* a, const void* b) {
  const MapKeyValue* key1 = (const MapKeyValue*)a;
  const MapKeyValue* key2 = (const MapKeyValue*)b;
  return StringCompareString(key1->key.p, key2->key.p);
}

Struct* NewStruct(bool is_union) {
  Struct* s = malloc(sizeof(Struct));
  s->refs = 1;
  s->tag_symbol = NULL;
  VectorInit(&s->bases);
  VectorInit(&s->friend_classes);
  VectorInit(&s->friend_functions);
  VectorInit(&s->member_using_declarations);
  VectorInit(&s->virtual_bases);
  VectorInit(&s->members);
  VectorInit(&s->virtual_members);
  VectorInit(&s->template_parameters);
  VectorInit(&s->partial_specializations);
  VectorInit(&s->deduction_guides);
  s->vptr_member = NULL;
  s->vtable_symbol = NULL;
  s->vbptr_member = NULL;
  s->vbtable_symbol = NULL;
  VectorInit(&s->vtable_symbols);
  VectorInit(&s->vbtable_symbols);
  MapInit(&s->symbol_table, CompareStructMember);
  MapInitForCharPointerKeys(&s->symbol_name_table);
  s->is_union = is_union;
  s->is_class = false;
  s->is_final = false;
  s->is_template = false;
  s->is_aggregate = CompilerIsCXX();
  s->cxx_special_members_complete = false;
  s->vtables_registered = false;
  s->template_parameter_count = 0;
  s->next_offset = 0;
  s->current_offset = 0;
  s->size = 0;
  s->non_virtual_size = 0;
  s->alignment = 1;
  s->packed = false;
  s->is_abstract = false;
  s->explicit_alignment = 0;
  s->pack = 0;
  s->next_bit_pos = 65;
  // Track every struct so it can be freed in bulk at end of compilation; see
  // StructRegistryRelease (struct infos can form reference cycles).
  if (!struct_registry_initialized) {
    VectorInit(&struct_registry);
    struct_registry_initialized = true;
  }
  VectorAppend(&struct_registry, s);
  return s;
}

// Tear down a struct's members and member tables, but do NOT free the Struct
// itself (see StructRegistryRelease's two-phase teardown).
static void StructTeardownMembers(Struct* s) {
  VectorDestructWithContents(&s->bases,
                             (VectorElementDestructor)CXXBaseSpecifierDelete,
                             /*free_element=*/false);
  // Friend lists hold borrowed references (the structs and function symbols are
  // owned elsewhere), so just release the backing storage.
  VectorDestruct(&s->friend_classes);
  VectorDestruct(&s->friend_functions);
  VectorDestructWithContents(
      &s->member_using_declarations,
      (VectorElementDestructor)CXXMemberUsingDeclarationDelete,
      /*free_element=*/false);
  VectorDestructWithContents(&s->virtual_bases,
                             (VectorElementDestructor)CXXVirtualBaseInfoDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(&s->members,
                             (VectorElementDestructor)StructMemberDelete, /*free_element=*/false);
  VectorDestruct(&s->virtual_members);
  VectorDestructWithContents(&s->vtable_symbols,
                             (VectorElementDestructor)CXXVTableInfoDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(&s->vbtable_symbols,
                             (VectorElementDestructor)CXXVBTableInfoDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(&s->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(
      &s->partial_specializations,
      (VectorElementDestructor)ClassTemplatePartialSpecializationDelete,
      /*free_element=*/false);
  VectorDestructWithContents(&s->deduction_guides,
                             (VectorElementDestructor)SymbolDestruct,
                             /*free_element=*/true);
  MapDestruct(&s->symbol_table);
  MapDestruct(&s->symbol_name_table);
}

void StructDelete(Struct* s) {
  StructTeardownMembers(s);
  free(s);
}

void StructAddFriendClass(Struct* str, Struct* friend_class) {
  if (str == NULL || friend_class == NULL) {
    return;
  }
  for (size_t i = 0; i < str->friend_classes.length; i++) {
    if (str->friend_classes.value.p[i] == friend_class) {
      return;
    }
  }
  VectorAppend(&str->friend_classes, friend_class);
}

void StructAddFriendFunction(Struct* str, Symbol* friend_function) {
  if (str == NULL || friend_function == NULL) {
    return;
  }
  for (size_t i = 0; i < str->friend_functions.length; i++) {
    if (str->friend_functions.value.p[i] == friend_function) {
      return;
    }
  }
  VectorAppend(&str->friend_functions, friend_function);
}

Symbol* NewEnumConstant(const char* name, int value) {
  TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualConst);
  Symbol* c = NewSymbol(name, int_type, STO(implicit));
  c->value.ivalue = value;
  c->flags.value_set = true;
  return c;
}

Symbol* NewScopedEnumConstant(const char* name, int value,
                              TypeRecord* enum_type) {
  TypeRecord* type = TypeRecordCopy(enum_type);
  type->qualifiers |= kQualConst;
  Symbol* c = NewSymbol(name, type, STO(implicit));
  c->value.ivalue = value;
  c->flags.value_set = true;
  return c;
}

static bool CXXExpressionNamesNonTypeTemplateParameter(ASTNode* node,
                                                       int* index) {
  if (!CompilerIsCXX() || node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL || !id->symbol->flags.is_template_parameter ||
      id->symbol->flags.is_template_type_parameter ||
      id->symbol->template_parameter_index < 0) {
    return false;
  }
  if (index != NULL) {
    *index = id->symbol->template_parameter_index;
  }
  return true;
}

Enum* NewEnum() {
  Enum* e = malloc(sizeof(Enum));
  e->refs = 1;
  e->tag_symbol = NULL;
  VectorInit(&e->constants);
  e->next_value = 0;
  e->is_scoped = false;
  e->has_fixed_underlying = false;
  e->fixed_underlying_type = 0;
  e->fixed_underlying_size = 0;
  // Track every enum so it can be freed in bulk by StructRegistryRelease; this
  // keeps composite-info teardown uniform and cycle/UAF-safe.
  if (!enum_registry_initialized) {
    VectorInit(&enum_registry);
    enum_registry_initialized = true;
  }
  VectorAppend(&enum_registry, e);
  return e;
}

void EnumDelete(Enum* e) {
  VectorDestruct(&e->constants);
  free(e);
}

Symbol* EnumFindConstant(Enum* e, String* name) {
  if (e == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < e->constants.length; i++) {
    Symbol* constant = e->constants.value.p[i];
    if (StringEqualString(&constant->name, name)) {
      return constant;
    }
  }
  return NULL;
}

// Mapping of type to its name.
static struct {
  Type type;
  const char* name;
} type_names[] = {
    {kTypeChar, "char"},          {kTypeShort, "short"},
    {kTypeLong, "long"},          {kTypeLongLong, "long long"},
    {kTypeInt, "int"},            {kTypeFloat, "float"},
    {kTypeDouble, "double"},      {kTypeLongDouble, "long double"},
    {kTypeStruct, "struct"},      {kTypeUnion, "union"},
    {kTypeVoid, "void"},          {kTypeBool, "bool"},
    {kTypeEnum, "enum"},          {kTypeNullPointer, "std::nullptr_t"},
    {kTypeImplicit, ""},
};

// Converts a type to a string and appends it to result.
static void TypeToString(Type type, String* result) {
  if ((type & kTypeSigned) != 0) {
    StringAppend(result, "signed ");
  }
  if ((type & kTypeUnsigned) != 0) {
    StringAppend(result, "unsigned ");
  }
  for (int i = 0; type_names[i].type != kTypeImplicit; i++) {
    if ((type & type_names[i].type) != 0) {
      StringAppend(result, type_names[i].name);
      StringAppend(result, " ");
    }
  }
}

// Converts qualifiers to string and appends them to result.
static void QualifiersToString(Qualifiers quals, String* result) {
  if ((quals & kQualConst) != 0) {
    StringAppend(result, "const ");
  }
  if ((quals & kQualVolatile) != 0) {
    StringAppend(result, "volatile ");
  }
  if ((quals & kQualRestrict) != 0) {
    StringAppend(result, "restrict ");
  }
}

// Prints a type record to standard output.
// This is very verbose output, intended for debugging to display
// all the information needed.  The 'with_function_body' parameter
// says whether the function body (the statements) are also printed.
void TypeRecordPrintDetails(TypeRecord* record, bool with_function_body, FILE* fp) {
  String str = {0};
  bool print_newline = false;

  while (record != NULL) {
    if (record->declarator == kDeclPointer) {
      fprintf(fp, "pointer to ");
    } else if (record->declarator == kDeclReference) {
      fprintf(fp, "reference to ");
    } else if (record->declarator == kDeclRValueReference) {
      fprintf(fp, "rvalue reference to ");
    }
    QualifiersToString(record->qualifiers, &str);
    TypeToString(record->type, &str);
    fprintf(fp, "%s", str.value);

    if (record->declarator == kDeclArray) {
      if (record->info.array.is_vla) {
        fprintf(fp, "variable length array ");
      } else {
        fprintf(fp, "array of size %d ", record->info.array.size.fixed);
      }
    } else if (record->declarator == kDeclFunction) {
      fprintf(fp, "function (");
      const char* sep = "";
      size_t nformals = record->info.function.prototype.length;
      for (size_t i = 0; i < nformals; i++) {
        Symbol* formal = (Symbol*)record->info.function.prototype.value.p[i];
        fprintf(fp, "%s", sep);
        sep = ",";
        SymbolPrint(formal, fp);
      }
      if (record->info.function.varargs) {
        fprintf(fp, "%s...", sep);
      }
      if (with_function_body) {
        fprintf(fp, ") {");
        CompoundStatementASTNode* body = (CompoundStatementASTNode*)
              record->info.function.body;
        size_t num_statements = body->statements->length;
        if (num_statements > 0) {
          fprintf(fp, "\n");
          print_newline = true;
        }
        for (size_t i = 0; i < num_statements; i++) {
          ASTNode* stmt = (ASTNode*)body->statements->value.p[i];
          ASTNodePrint(stmt, 2, fp);
        }
        fprintf(fp, "} returning ");
      } else {
        fprintf(fp, ") returning ");
      }
    }
    record = record->next;
  }
  if (print_newline) {
    fprintf(fp, "\n");
  }
  StringDestruct(&str);
}

// Basic type record printer without function body.  Also pretty verbose.
void TypeRecordPrint(TypeRecord* record, FILE* fp) {
  TypeRecordPrintDetails(record, false, fp);
}

// Convert a type record to a string in C syntax.  Not verbose.
void TypeRecordToString(TypeRecord* type, String* result) {
  if (type == NULL) {
    StringAppend(result, "<invalid>");
    return;
  }
  switch (type->declarator) {
    case kDeclPrimitive:
      if (TypeIsUnknown(type) && type->template_parameter_index >= 0) {
        QualifiersToString(type->qualifiers, result);
        StringPrintf(result, "$T%d", type->template_parameter_index);
        if (type->dependent_member_name != NULL) {
          StringAppend(result, "::");
          StringAppendString(result, type->dependent_member_name);
        }
        break;
      }
      QualifiersToString(type->qualifiers, result);
      TypeToString(type->type, result);
      if (TypeIsStructOrUnion(type)) {
        StringAppendString(result, type->info.struct_info->tag_name);
      } else if (TypeIsEnum(type)) {
        StringAppendString(result, type->info.enum_info->tag_name);
      }
      break;

    case kDeclPointer:
      TypeRecordToString(type->next, result);
      if (type->next != NULL &&
          (type->next->declarator == kDeclArray ||
           type->next->declarator == kDeclFunction)) {
        StringAppend(result, "(*");
        QualifiersToString(type->qualifiers, result);
        StringAppendChar(result, ')');
      } else {
        StringAppendChar(result, '*');
        QualifiersToString(type->qualifiers, result);
      }
      break;

    case kDeclReference:
    case kDeclRValueReference:
      TypeRecordToString(type->next, result);
      StringAppend(result,
                   type->declarator == kDeclRValueReference ? "&&" : "&");
      break;

    case kDeclArray:
      TypeRecordToString(type->next, result);
      StringPrintf(result, "[%d]", type->info.array.size.fixed);
      break;

    case kDeclFunction: {
      TypeRecordToString(type->next, result);
      StringAppendChar(result, '(');
      const char* sep = "";
      size_t nformals = type->info.function.prototype.length;
      for (size_t i = 0; i < nformals; i++) {
        Symbol* formal = (Symbol*)type->info.function.prototype.value.p[i];
        StringAppend(result, sep);
        TypeRecordToString(formal->type, result);
        sep = ",";
      }
      StringAppendChar(result, ')');
      break;
    }
  }
}

static bool FunctionFormalIsImplicitThis(TypeRecord* func, size_t index) {
  if (func == NULL || index >= func->info.function.prototype.length) {
    return false;
  }
  Symbol* formal = func->info.function.prototype.value.p[index];
  return formal != NULL && StringEqual(&formal->name, "this");
}

static void TrimTrailingSpaces(String* out) {
  while (out->length > 0 && out->value[out->length - 1] == ' ') {
    out->value[--out->length] = '\0';
  }
}

static void AppendFunctionDisplayName(TypeRecord* func, String* out) {
  Symbol* symbol = func != NULL ? func->info.function.symbol : NULL;
  if (symbol == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  Namespace* ns = symbol->namespace_;
  if (ns == NULL && owner != NULL && owner->tag_symbol != NULL) {
    ns = owner->tag_symbol->namespace_;
  }
  if (ns != NULL && ns != compiler->global_namespace &&
      ns->qualified_name.length != 0) {
    StringAppendString(out, &ns->qualified_name);
    StringAppend(out, "::");
  }
  if (owner != NULL && owner->tag_name != NULL) {
    StringAppendString(out, owner->tag_name);
    StringAppend(out, "::");
  }
  if (func->info.function.is_constructor && owner != NULL &&
      owner->tag_name != NULL) {
    StringAppendString(out, owner->tag_name);
  } else if (func->info.function.is_destructor && owner != NULL &&
             owner->tag_name != NULL) {
    StringAppendChar(out, '~');
    StringAppendString(out, owner->tag_name);
  } else {
    StringAppendString(out, &symbol->name);
  }
}

static void AppendFunctionParameterList(TypeRecord* func, String* out) {
  StringAppendChar(out, '(');
  const char* sep = "";
  bool wrote_parameter = false;
  size_t start = FunctionFormalIsImplicitThis(func, 0) ? 1 : 0;
  for (size_t i = start; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal == NULL) {
      continue;
    }
    StringAppend(out, sep);
    TypeRecordToString(formal->type, out);
    sep = ", ";
    wrote_parameter = true;
  }
  if (!wrote_parameter) {
    StringAppend(out, "void");
  }
  StringAppendChar(out, ')');
}

void TypeRecordFunctionPrettyName(TypeRecord* func, String* result) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.symbol == NULL) {
    return;
  }
  if (!func->info.function.is_constructor &&
      !func->info.function.is_destructor) {
    TypeRecordToString(func->next, result);
    TrimTrailingSpaces(result);
    StringAppendChar(result, ' ');
  }
  AppendFunctionDisplayName(func, result);
  AppendFunctionParameterList(func, result);
  if (func->info.function.is_const_member) {
    StringAppend(result, " const");
  }
  if (func->info.function.ref_qualifier == kCXXRefQualifierLValue) {
    StringAppend(result, " &");
  } else if (func->info.function.ref_qualifier == kCXXRefQualifierRValue) {
    StringAppend(result, " &&");
  }
}

void SymbolFunctionPrettyName(Symbol* symbol, String* result) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    if (symbol != NULL) {
      StringAppendString(result, &symbol->name);
    }
    return;
  }
  TypeRecordFunctionPrettyName(symbol->type, result);
}

void SymbolFunctionDiagnosticSuffix(Symbol* symbol, String* result) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return;
  }
  String pretty;
  StringInit(&pretty, NULL);
  SymbolFunctionPrettyName(symbol, &pretty);
  if (pretty.length != 0 && !StringEqualString(&pretty, &symbol->name)) {
    StringAppend(result, " (");
    StringAppendString(result, &pretty);
    StringAppendChar(result, ')');
  }
  StringDestruct(&pretty);
}

void SymbolFunctionDiagnosticName(Symbol* symbol, String* result) {
  if (symbol == NULL) {
    StringAppend(result, "<unknown>");
    return;
  }
  StringAppendString(result, &symbol->name);
  SymbolFunctionDiagnosticSuffix(symbol, result);
}

static void TypeRecordToTemplateKeyString(TypeRecord* type, String* result) {
  switch (type->declarator) {
    case kDeclPrimitive:
      if (TypeIsUnknown(type) && type->template_parameter_index >= 0) {
        QualifiersToString(type->qualifiers, result);
        StringPrintf(result, "$T%d", type->template_parameter_index);
        if (type->dependent_member_name != NULL) {
          StringAppend(result, "::");
          StringAppendString(result, type->dependent_member_name);
        }
        break;
      }
      QualifiersToString(type->qualifiers, result);
      TypeToString(type->type, result);
      if (TypeIsStructOrUnion(type)) {
        Struct* str = type->info.struct_info;
        if (str != NULL && str->tag_name != NULL) {
          if (str->tag_symbol != NULL && str->tag_symbol->namespace_ != NULL &&
              str->tag_symbol->namespace_ != compiler->global_namespace) {
            StringAppendString(result,
                               &str->tag_symbol->namespace_->qualified_name);
            StringAppend(result, "::");
          }
          if (str->tag_name->value[0] != '<') {
            StringAppendString(result, str->tag_name);
          }
          StringPrintf(result, "$S%p", (void*)str);
        }
      } else if (TypeIsEnum(type)) {
        Enum* e = type->info.enum_info;
        if (e != NULL && e->tag_name != NULL) {
          StringAppendString(result, e->tag_name);
        }
      }
      break;

    case kDeclPointer:
      TypeRecordToTemplateKeyString(type->next, result);
      StringAppendChar(result, '*');
      QualifiersToString(type->qualifiers, result);
      break;

    case kDeclReference:
    case kDeclRValueReference:
      TypeRecordToTemplateKeyString(type->next, result);
      StringAppend(result,
                   type->declarator == kDeclRValueReference ? "&&" : "&");
      break;

    case kDeclArray:
      TypeRecordToTemplateKeyString(type->next, result);
      StringPrintf(result, "[%d]", type->info.array.size.fixed);
      break;

    case kDeclFunction:
      TypeRecordToString(type, result);
      break;
  }
}

//
// The type parser: a recursive descent parser for the C language's complex
// type system.
//
void TypeParserInit(TypeParser* parser, Lex* lex, struct Syntax* syntax,
                    Storage storage, ParserContext context) {
  parser->lex = lex;
  parser->syntax = syntax;
  parser->storage = storage;
  VectorInit(&parser->stack);
  parser->symbol = NULL;
  parser->found_void = false;
  parser->dimension_count = 0;
  parser->is_inline = false;
  parser->is_constexpr = false;
  parser->is_consteval = false;
  parser->is_constinit = false;
  parser->declarator_is_parameter_pack = false;
  parser->context = context;
  parser->cxx_member_owner = NULL;
  parser->template_substitution_source = NULL;
  parser->template_substitution_target = NULL;
  parser->cxx_member_definition = NULL;
  parser->declarator_template_arguments = NULL;
  parser->template_substitution_failed = false;
}

void TypeParserReset(TypeParser* parser) {
  parser->symbol = NULL;
  parser->storage = STO(implicit);
  parser->found_void = false;
  parser->dimension_count = 0;
  parser->is_consteval = false;
  parser->is_constinit = false;
  parser->declarator_is_parameter_pack = false;
  parser->cxx_member_owner = NULL;
  parser->template_substitution_source = NULL;
  parser->template_substitution_target = NULL;
  parser->cxx_member_definition = NULL;
  if (parser->declarator_template_arguments != NULL) {
    VectorDeleteWithContents(parser->declarator_template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    parser->declarator_template_arguments = NULL;
  }
  VectorDestruct(&parser->stack);
  VectorInit(&parser->stack);
}

void TypeParserDestruct(TypeParser* parser) {
  // Only frees the stack's backing array.  Any TypeRecords still referenced by
  // the stack are owned elsewhere (the combined result type) and must not be
  // freed here.
  if (parser->declarator_template_arguments != NULL) {
    VectorDeleteWithContents(parser->declarator_template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    parser->declarator_template_arguments = NULL;
  }
  VectorDestruct(&parser->stack);
}

// Mapping for token vs type for parsing a type specifier.
static struct {
  Token token;
  Type type;
} type_map[] = {
    {TOK(char), kTypeChar},         {TOK(int), kTypeInt},
    {TOK(short), kTypeShort},       {TOK(long), kTypeLong},
    {TOK(float), kTypeFloat},       {TOK(double), kTypeDouble},
    {TOK(class), kTypeStruct},      {TOK(struct), kTypeStruct},
    {TOK(union), kTypeUnion},
    {TOK(enum), kTypeEnum},         {TOK(void), kTypeVoid},
    {TOK(bool), kTypeBool},         {TOK(signed), kTypeSigned},
    {TOK(unsigned), kTypeUnsigned}, {TOK(auto), kTypeAuto},
    // In C++ `wchar_t` is a distinct keyword, but this implementation defines
    // it to its underlying integer type (matching `__WCHAR_TYPE__` and the C
    // `typedef int wchar_t`), so a `wchar_t` type-specifier behaves like `int`.
    {TOK(wchar_t), kTypeInt},
    {TOK(bad), kTypeImplicit},
};

static TypeRecord* NewDecltypeReference(TypeRecord* expr_type, bool rvalue) {
  TypeRecord* base = TypeIsReference(expr_type) ? expr_type->next : expr_type;
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
  TypeRecordChain(ref, base);
  ref->type = base->type;
  TypeRecordCalculateSize(ref);
  return ref;
}

static TypeRecord* ParseCXXDecltypeSpecifier(TypeParser* parser) {
  LexNextToken(parser->lex);
  SyntaxNeedBracket(parser->syntax, TOK(lparen), TC(type));

  bool parenthesized_expression = LexLookingAt(parser->lex, TOK(lparen));
  bool unparenthesized_identifier =
      !parenthesized_expression &&
      (LexLookingAt(parser->lex, TOK(identifier)) ||
       LexLookingAt(parser->lex, TOK(coloncolon)));
  ASTNode* expr = SyntaxParseSingleExpression(parser->syntax, TC(closebra));
  Symbol* declared_symbol = NULL;
  if (unparenthesized_identifier && expr != NULL &&
      expr->op == AST_OP(identifier)) {
    declared_symbol = ((IdentifierASTNode*)expr)->symbol;
  }
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(type));

  TypeRecord* result = NULL;
  if (declared_symbol != NULL) {
    result = TypeRecordCopy(declared_symbol->type);
  } else {
    expr = AnalyzeExpression(expr);
    if (expr == NULL || expr->type == NULL) {
      SyntaxError(parser->syntax, "Invalid expression in decltype");
      result = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    } else if (expr->value_category == kValueCategoryLvalue) {
      result = NewDecltypeReference(expr->type, false);
    } else if (expr->value_category == kValueCategoryXvalue) {
      result = NewDecltypeReference(expr->type, true);
    } else {
      result = TypeRecordCopy(expr->type);
    }
  }
  if (expr != NULL) {
    ASTNodeDelete(expr);
  }
  if (result == NULL) {
    SyntaxError(parser->syntax, "Invalid decltype specifier");
    result = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  return result;
}

static void AlignNextOffset(Struct* str, TypeRecord* type);
static void AlignNextOffsetForSymbol(Struct* str, Symbol* symbol);
static void AddStructMember(TypeParser* parser, Struct* str,
                            StructMember* member);
static void AppendStructMemberOverload(TypeParser* parser, Struct* str,
                                       StructMember* first,
                                       StructMember* member);
static void ParseCXXPureSpecifier(TypeParser* parser, TypeRecord* func);
static void CXXFinalizeSpecialMemberMetadata(Symbol* symbol, Struct* owner,
                                             bool user_declared);
static void ComputeCXXAggregateStatus(Struct* str);
static void AddImplicitCXXSpecialMembers(TypeParser* parser, Struct* str,
                                         Symbol* tag);
static void AddImplicitCXXDestructorIfNeeded(TypeParser* parser, Struct* str,
                                             Symbol* tag);
static void AddImplicitCXXDeductionGuides(Struct* str, Symbol* tag);
static void UpdateStructSize(Struct* str, TypeRecord* member_type,
                             bool is_union);
static void FinalizeStructAlignment(Struct* str);
static TypeRecord* InstantiateSimpleClassTemplate(TypeParser* parser,
                                                  Symbol* templ,
                                                  Vector* args);
static TypeRecord* InstantiateAliasClassTemplate(TypeParser* parser,
                                                 Symbol* alias,
                                                 Vector* args);
static bool CXXAliasTemplatePatternNamesClassTemplate(Symbol* alias);
static bool TemplateArgumentPatternVectorEqual(Vector* left, Vector* right);
static void AddClassTemplatePartialSpecialization(TypeParser* parser,
                                                  Symbol* primary,
                                                  Symbol* partial_tag,
                                                  Vector* pattern_args);

static Vector* CompleteFunctionTemplateArguments(TypeParser* parser,
                                                 TypeRecord* func,
                                                 Vector* args,
                                                 bool emit_error);
static TypeRecord* SubstituteTemplateParameters(TypeParser* parser,
                                                TypeRecord* type,
                                                Vector* args);
static bool TypeIsTemplateParameterPlaceholder(TypeRecord* type, int* index);
static bool CurrentTemplateParameterIsPack(Syntax* syntax, int index);
bool TypeContainsTemplateParameter(TypeRecord* type);
static TemplateArgument* NewEmptyPackTemplateArgument(
    TemplateParameterKind kind);
static int FindTemplateParameterPackIndex(Vector* template_parameters);
static void AppendSubstitutedFormalParameter(TypeParser* parser, Vector* out,
                                             Symbol* formal, Vector* args,
                                             int rebase_base);
static TypeRecord* SubstituteTemplateParametersForPackElement(
    TypeParser* parser, TypeRecord* type, Vector* args, int pack_index,
    size_t element_index);
static void RebaseTemplateParameterIndices(TypeRecord* type, int base);
static bool StructContainsTemplateParameter(Struct* str);
static TypeRecord* SubstituteNestedStructTemplateParameters(TypeParser* parser,
                                                            TypeRecord* type,
                                                            Vector* args);
typedef struct {
  Symbol* symbol;
  Symbol* template_definition;
  Struct* substitution_source;
} PendingMemberBody;
static StructMember* InstantiateTemplateMemberFunction(TypeParser* parser,
                                                       Struct* owner,
                                                       StructMember* member,
                                                       Vector* args,
                                                       Vector* pending);
static void CloneInstantiatedMemberFunctionBody(TypeParser* parser,
                                                Struct* owner, Symbol* symbol,
                                                Symbol* template_definition,
                                                Struct* substitution_source,
                                                Vector* args);
static int CXXBaseOffsetForMember(Struct* str, StructMember* member);
static bool CanOverloadStructMember(StructMember* existing,
                                    StructMember* member);
static CXXMemberUsingDeclaration* NewCXXMemberUsingDeclaration(
    TypeRecord* base_type, const char* member_name, CXXAccess access,
    SourceLocation location, bool is_pack_expansion);
static void ImportCXXMemberUsingDeclaration(TypeParser* parser, Struct* owner,
                                            TypeRecord* base_type,
                                            const char* member_name,
                                            CXXAccess access,
                                            SourceLocation location);

static ASTNode* IdentityCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static ASTNode* CloneCXXDefaultMemberInitializer(ASTNode* initializer) {
  return ASTNodeClone(initializer, IdentityCloneNode, NULL, NULL);
}

static bool StructMemberIsNestedType(StructMember* member) {
  return member != NULL && member->symbol != NULL &&
         StorageIs(member->symbol->storage, STO(typedef));
}

static bool StructHasMemberFunction(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && member->is_member_function) {
      return true;
    }
  }
  return false;
}

static bool TypeIsNamedCXXNestedType(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL) {
    return false;
  }
  if (TypeIsStructOrUnion(type)) {
    return type->info.struct_info != NULL &&
           type->info.struct_info->tag_symbol != NULL &&
           !type->info.struct_info->tag_symbol->flags.invented;
  }
  if (TypeIsEnum(type)) {
    return type->info.enum_info != NULL &&
           type->info.enum_info->tag_symbol != NULL &&
           !type->info.enum_info->tag_symbol->flags.invented;
  }
  return false;
}

static void AddCXXNestedTypeMember(TypeParser* parser, Struct* owner,
                                   TypeRecord* type, CXXAccess access) {
  Symbol* tag = NULL;
  if (TypeIsStructOrUnion(type)) {
    tag = type->info.struct_info != NULL
        ? type->info.struct_info->tag_symbol
        : NULL;
  } else if (TypeIsEnum(type)) {
    tag = type->info.enum_info != NULL
        ? type->info.enum_info->tag_symbol
        : NULL;
  }
  if (tag == NULL) {
    return;
  }
  if (FindStructMember(owner, &tag->name) != NULL) {
    SyntaxError(parser->syntax, "Duplicate nested type %s", tag->name.value);
    return;
  }

  Symbol* alias = NewSymbol(tag->name.value, TypeRecordCopy(type), STO(typedef));
  alias->location = tag->location;
  StructMember* member = NewStructMember(alias);
  member->access = access;
  AddStructMember(parser, owner, member);
}

static void AddCXXUnscopedEnumConstantMembers(TypeParser* parser,
                                              Struct* owner,
                                              TypeRecord* type,
                                              CXXAccess access) {
  if (!CompilerIsCXX() || owner == NULL || type == NULL ||
      !TypeIsEnum(type) || type->info.enum_info == NULL ||
      type->info.enum_info->is_scoped) {
    return;
  }
  Enum* e = type->info.enum_info;
  for (size_t i = 0; i < e->constants.length; i++) {
    Symbol* constant = e->constants.value.p[i];
    if (constant == NULL) {
      continue;
    }
    if (FindStructMember(owner, &constant->name) != NULL) {
      SyntaxError(parser->syntax, "Duplicate enum constant %s",
                  constant->name.value);
      continue;
    }
    StructMember* member = NewStructMember(SymbolClone(constant));
    member->access = access;
    member->is_static = true;
    AddStructMember(parser, owner, member);

    // Make the constant visible by unqualified name within the class body so
    // later member declarations (e.g. array bounds) and inline member bodies
    // can use it, matching C++ class scope rules.
    Symbol* scope_constant = SymbolClone(constant);
    if (!SyntaxAddSymbol(parser->syntax, scope_constant)) {
      SymbolDelete(scope_constant);
    }
  }
}

static void AddCXXNestedAliasMember(TypeParser* parser, Struct* owner,
                                    const char* name, TypeRecord* type,
                                    CXXAccess access,
                                    SourceLocation location) {
  if (!CompilerIsCXX() || owner == NULL || name == NULL || type == NULL) {
    return;
  }
  if (FindStructMemberByName(owner, name) != NULL) {
    SyntaxError(parser->syntax, "Duplicate nested type %s", name);
    return;
  }

  Symbol* alias = NewSymbol(name, type, STO(typedef));
  alias->location = location;
  StructMember* member = NewStructMember(alias);
  member->access = access;
  AddStructMember(parser, owner, member);

  // Also make the alias visible by unqualified name within the class body so
  // later member declarations and inline member bodies can use it as a type.
  // The class-body symbol scope opened in ParseStructBody owns the lookup; the
  // injected symbol is tracked in all_local_symbols and freed at end of
  // compilation, so closing that scope does not free it.
  Symbol* scope_alias = NewSymbol(name, type, STO(typedef));
  scope_alias->location = location;
  if (!SyntaxAddSymbol(parser->syntax, scope_alias)) {
    SymbolDelete(scope_alias);
  }
}

static void ParseCXXMemberUsingAlias(TypeParser* parser, Struct* owner,
                                     CXXAccess access,
                                     SourceLocation location) {
  if (!LexLookingAt(parser->lex, TOK(identifier))) {
    SyntaxError(parser->syntax, "Expected alias name after using");
    SyntaxRecover(parser->syntax, TC(semicolon));
    return;
  }
  String alias_name;
  StringInit(&alias_name, parser->lex->spelling.value);
  LexNextToken(parser->lex);
  if (!LexMatch(parser->lex, TOK(equal))) {
    SyntaxError(parser->syntax, "Expected = in using alias declaration");
    StringDestruct(&alias_name);
    SyntaxRecover(parser->syntax, TC(semicolon));
    return;
  }
  TypeRecord* type = TypeParserParseType(parser, true);
  Symbol* parsed = type != NULL ? TypeParserParseDeclarator(parser, type) : NULL;
  TypeRecord* alias_type = parsed != NULL ? parsed->type : type;
  AddCXXNestedAliasMember(parser, owner, alias_name.value, alias_type, access,
                          location);
  if (parsed != NULL) {
    SymbolDelete(parsed);
  }
  StringDestruct(&alias_name);
}

static bool CXXMemberUsingLooksLikeAlias(TypeParser* parser) {
  if (!LexLookingAt(parser->lex, TOK(identifier))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  LexNextToken(parser->lex);
  bool is_alias = LexLookingAt(parser->lex, TOK(equal));
  LexCheckpointRestore(parser->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return is_alias;
}

static void ParseCXXMemberUsingDeclaration(TypeParser* parser, Struct* owner,
                                           CXXAccess access,
                                           SourceLocation location) {
  if (CXXMemberUsingLooksLikeAlias(parser)) {
    ParseCXXMemberUsingAlias(parser, owner, access, location);
    return;
  }

  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(parser->syntax, &name,
                                                         TC(decl))) {
    SyntaxError(parser->syntax, "Expected qualified name after using");
    FullyQualifiedIdentifierDestruct(&name);
    SyntaxRecover(parser->syntax, TC(semicolon));
    return;
  }
  bool is_pack_expansion = LexMatch(parser->lex, TOK(ellipsis));
  if (!name.is_qualified || name.components.length != 2) {
    SyntaxError(parser->syntax,
                "Member using declaration requires Base::member");
    FullyQualifiedIdentifierDestruct(&name);
    return;
  }

  String* base_name = name.components.value.p[0];
  String* member_name = name.components.value.p[1];
  Symbol* base_symbol = SyntaxFindSymbol(parser->syntax, base_name);
  if (base_symbol == NULL) {
    base_symbol = SyntaxFindTag(parser->syntax, base_name);
  }
  if (base_symbol == NULL || base_symbol->type == NULL) {
    SyntaxError(parser->syntax, "No such base class %s", base_name->value);
    FullyQualifiedIdentifierDestruct(&name);
    return;
  }

  TypeRecord* base_type = TypeRecordCopy(base_symbol->type);
  int placeholder_index = -1;
  bool is_template_parameter_base =
      TypeIsTemplateParameterPlaceholder(base_type, &placeholder_index);
  if (is_pack_expansion &&
      !CurrentTemplateParameterIsPack(parser->syntax, placeholder_index)) {
    SyntaxError(parser->syntax,
                "member using pack expansion requires a template parameter pack");
    TypeRecordDelete(base_type);
    FullyQualifiedIdentifierDestruct(&name);
    return;
  }

  bool is_dependent = is_template_parameter_base ||
                      TypeContainsTemplateParameter(base_type);
  if (is_dependent || parser->syntax->parsing_template_declaration) {
    VectorAppend(&owner->member_using_declarations,
                 NewCXXMemberUsingDeclaration(base_type, member_name->value,
                                              access, location,
                                              is_pack_expansion));
  } else {
    ImportCXXMemberUsingDeclaration(parser, owner, base_type,
                                    member_name->value, access, location);
  }
  TypeRecordDelete(base_type);
  FullyQualifiedIdentifierDestruct(&name);
}

static void ParseCXXMemberTypedef(TypeParser* parser, Struct* owner,
                                  CXXAccess access,
                                  SourceLocation location) {
  Storage old_storage = parser->storage;
  parser->storage = STO(typedef);
  TypeRecord* type = TypeParserParseType(parser, true);
  while (!LexEof(parser->lex)) {
    Symbol* alias = TypeParserParseDeclarator(parser, type);
    if (alias == NULL) {
      SyntaxError(parser->syntax, "Invalid typedef member");
      break;
    }
    AddCXXNestedAliasMember(parser, owner, alias->name.value, alias->type,
                            access, alias->location != 0 ? alias->location
                                                         : location);
    SymbolDelete(alias);
    if (!LexMatch(parser->lex, TOK(comma))) {
      break;
    }
  }
  parser->storage = old_storage;
}

static bool TypeIsTemplateParameterPlaceholder(TypeRecord* type, int* index) {
  if (type == NULL || type->declarator != kDeclPrimitive ||
      type->template_parameter_index < 0) {
    return false;
  }
  if (index != NULL) {
    *index = type->template_parameter_index;
  }
  return true;
}

static bool CurrentTemplateParameterIsPack(Syntax* syntax, int index) {
  if (syntax == NULL || syntax->current_template_parameters == NULL ||
      index < 0) {
    return false;
  }
  for (size_t i = 0; i < syntax->current_template_parameters->length; i++) {
    TemplateParameter* param = syntax->current_template_parameters->value.p[i];
    if (param != NULL && param->index == index) {
      return param->is_parameter_pack;
    }
  }
  return false;
}

static CXXMemberUsingDeclaration* NewCXXMemberUsingDeclaration(
    TypeRecord* base_type, const char* member_name, CXXAccess access,
    SourceLocation location, bool is_pack_expansion) {
  CXXMemberUsingDeclaration* decl = malloc(sizeof(CXXMemberUsingDeclaration));
  decl->base_type = TypeRecordCopy(base_type);
  StringInit(&decl->member_name, member_name);
  decl->access = access;
  decl->location = location;
  decl->is_pack_expansion = is_pack_expansion;
  return decl;
}

static bool StructHasBaseStruct(Struct* str, Struct* target, int* offset) {
  if (str == NULL || target == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (base_struct == target) {
      if (offset != NULL) {
        *offset = base->byte_offset;
      }
      return true;
    }
    int nested_offset = 0;
    if (StructHasBaseStruct(base_struct, target, &nested_offset)) {
      if (offset != NULL) {
        *offset = base->byte_offset + nested_offset;
      }
      return true;
    }
  }
  return false;
}

static StructMember* CloneCXXMemberUsingMember(StructMember* member,
                                               CXXAccess access,
                                               int byte_offset) {
  if (member == NULL || member->symbol == NULL) {
    return NULL;
  }
  StructMember* clone = NewStructMember(SymbolClone(member->symbol));
  clone->byte_offset = byte_offset;
  clone->bit_offset = member->bit_offset;
  clone->bit_size = member->bit_size;
  clone->index = member->index;
  clone->cxx_vcall_offset = member->cxx_vcall_offset;
  clone->is_anon = member->is_anon;
  clone->is_static = member->is_static;
  clone->is_mutable = member->is_mutable;
  clone->is_member_function = member->is_member_function;
  clone->is_using_declaration = true;
  clone->access = access;
  return clone;
}

static void AddCXXMemberUsingFunction(TypeParser* parser, Struct* owner,
                                      StructMember* member) {
  if (member == NULL) {
    return;
  }
  StructMember* existing =
      MapFindPointerKey(&owner->symbol_table, &member->symbol->name);
  if (existing != NULL) {
    if (!CanOverloadStructMember(existing, member)) {
      String member_name;
      StringInit(&member_name, NULL);
      SymbolFunctionDiagnosticName(member->symbol, &member_name);
      SyntaxError(parser->syntax, "Duplicate class member %s",
                  member_name.value);
      StringDestruct(&member_name);
      StructMemberDelete(member);
      return;
    }
    if (FindStructMemberOverload(existing, member->symbol->type) != NULL) {
      String member_name;
      StringInit(&member_name, NULL);
      SymbolFunctionDiagnosticName(member->symbol, &member_name);
      SyntaxError(parser->syntax, "Duplicate class member %s",
                  member_name.value);
      StringDestruct(&member_name);
      StructMemberDelete(member);
      return;
    }
    AppendStructMemberOverload(parser, owner, existing, member);
  } else {
    AddStructMember(parser, owner, member);
  }
}

static void ImportCXXMemberUsingDeclaration(TypeParser* parser, Struct* owner,
                                            TypeRecord* base_type,
                                            const char* member_name,
                                            CXXAccess access,
                                            SourceLocation location) {
  if (owner == NULL || base_type == NULL || member_name == NULL) {
    return;
  }
  if (!TypeIsStructOrUnion(base_type) || base_type->info.struct_info == NULL) {
    SyntaxError(parser->syntax,
                "member using declaration requires a class base");
    return;
  }
  Struct* base_struct = base_type->info.struct_info;
  int base_offset = 0;
  if (!StructHasBaseStruct(owner, base_struct, &base_offset)) {
    SyntaxError(parser->syntax,
                "using declaration base is not a base class");
    return;
  }
  CXXAccess ignored_access = kAccessPublic;
  Struct* ignored_owner = NULL;
  int member_offset = 0;
  StructMember* first = FindStructMemberWithAccessAndOffsetByName(
      base_struct, member_name, &ignored_access, &ignored_owner,
      &member_offset);
  if (first == NULL) {
    SyntaxError(parser->syntax, "No such base class member %s", member_name);
    return;
  }
  if (ignored_access == kAccessPrivate) {
    SyntaxError(parser->syntax, "%s is a private member of %s", member_name,
                ignored_owner != NULL && ignored_owner->tag_name != NULL
                    ? ignored_owner->tag_name->value
                    : "<anonymous>");
    return;
  }
  bool imported = false;
  for (StructMember* member = first; member != NULL;
       member = member->overload_next) {
    int byte_offset = base_offset + member_offset;
    if (member->is_member_function) {
      byte_offset = base_offset + CXXBaseOffsetForMember(base_struct, member);
    }
    StructMember* clone =
        CloneCXXMemberUsingMember(member, access, byte_offset);
    if (clone != NULL) {
      clone->symbol->location = location;
      AddCXXMemberUsingFunction(parser, owner, clone);
      imported = true;
    }
  }
  if (!imported) {
    SyntaxError(parser->syntax, "No such base class member %s", member_name);
  }
}

static void ApplyCXXMemberUsingDeclaration(TypeParser* parser, Struct* owner,
                                           CXXMemberUsingDeclaration* decl,
                                           Vector* args) {
  if (decl == NULL || decl->base_type == NULL) {
    return;
  }
  int pack_index = -1;
  if (decl->is_pack_expansion &&
      TypeIsTemplateParameterPlaceholder(decl->base_type, &pack_index) &&
      pack_index >= 0 && args != NULL && (size_t)pack_index < args->length) {
    TemplateArgument* pack = args->value.p[pack_index];
    if (pack != NULL && pack->pack_arguments != NULL) {
      for (size_t i = 0; i < pack->pack_arguments->length; i++) {
        TemplateArgument* element = pack->pack_arguments->value.p[i];
        if (element == NULL || element->kind != kTemplateParameterType ||
            element->type == NULL) {
          SyntaxError(parser->syntax,
                      "member using pack expansion requires class types");
          continue;
        }
        ImportCXXMemberUsingDeclaration(parser, owner, element->type,
                                        decl->member_name.value, decl->access,
                                        decl->location);
      }
      return;
    }
  }
  TypeRecord* base_type =
      args != NULL ? SubstituteTemplateParameters(parser, decl->base_type, args)
                   : TypeRecordCopy(decl->base_type);
  ImportCXXMemberUsingDeclaration(parser, owner, base_type,
                                  decl->member_name.value, decl->access,
                                  decl->location);
  TypeRecordDelete(base_type);
}

static void ApplyCXXMemberUsingDeclarations(TypeParser* parser, Struct* owner,
                                            Struct* template_struct,
                                            Vector* args) {
  if (!CompilerIsCXX() || owner == NULL || template_struct == NULL) {
    return;
  }
  for (size_t i = 0; i < template_struct->member_using_declarations.length;
       i++) {
    ApplyCXXMemberUsingDeclaration(
        parser, owner, template_struct->member_using_declarations.value.p[i],
        args);
  }
}

static bool RecordPackExpansionIndex(int candidate, Vector* args,
                                     int* pack_index, size_t* pack_length) {
  if (candidate < 0 || args == NULL || (size_t)candidate >= args->length) {
    return false;
  }
  TemplateArgument* arg = args->value.p[candidate];
  if (arg == NULL || arg->pack_arguments == NULL) {
    return false;
  }
  if (*pack_index >= 0 && *pack_index != candidate) {
    return *pack_length == arg->pack_arguments->length;
  }
  *pack_index = candidate;
  *pack_length = arg->pack_arguments->length;
  return true;
}

static bool FindPackExpansionInTemplateArgument(TemplateArgument* arg,
                                                Vector* args,
                                                int* pack_index,
                                                size_t* pack_length);

static bool FindPackExpansionInType(TypeRecord* type, Vector* args,
                                    int* pack_index, size_t* pack_length) {
  bool found = false;
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    int index = -1;
    if (TypeIsTemplateParameterPlaceholder(t, &index) &&
        RecordPackExpansionIndex(index, args, pack_index, pack_length)) {
      found = true;
    }
    if (t->declarator == kDeclArray &&
        RecordPackExpansionIndex(t->info.array.template_parameter_index, args,
                                 pack_index, pack_length)) {
      found = true;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        if (FindPackExpansionInTemplateArgument(t->template_arguments->value.p[i],
                                                args, pack_index,
                                                pack_length)) {
          found = true;
        }
      }
    }
    if (TypeIsFunction(t)) {
      for (size_t i = 0; i < t->info.function.prototype.length; i++) {
        Symbol* formal = t->info.function.prototype.value.p[i];
        if (formal != NULL &&
            FindPackExpansionInType(formal->type, args, pack_index,
                                    pack_length)) {
          found = true;
        }
      }
    }
  }
  return found;
}

static bool FindPackExpansionInTemplateArgument(TemplateArgument* arg,
                                                Vector* args,
                                                int* pack_index,
                                                size_t* pack_length) {
  if (arg == NULL) {
    return false;
  }
  if (arg->kind == kTemplateParameterNonType &&
      RecordPackExpansionIndex(arg->template_parameter_index, args, pack_index,
                               pack_length)) {
    return true;
  }
  if (arg->kind == kTemplateParameterType &&
      RecordPackExpansionIndex(arg->template_parameter_index, args, pack_index,
                               pack_length)) {
    return true;
  }
  if (arg->pack_arguments != NULL) {
    bool found = false;
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      if (FindPackExpansionInTemplateArgument(arg->pack_arguments->value.p[i],
                                              args, pack_index, pack_length)) {
        found = true;
      }
    }
    return found;
  }
  return FindPackExpansionInType(arg->type, args, pack_index, pack_length);
}

/* Copy an argument vector but replace the pack at `pack_index` with a single
 * concrete `element`. Used to substitute one element at a time while expanding
 * a pack-dependent pattern. */
static Vector* TemplateArgumentVectorCopyWithPackElement(Vector* args,
                                                         int pack_index,
                                                         TemplateArgument* element) {
  Vector* copy = TemplateArgumentVectorCopy(args);
  if (copy == NULL || pack_index < 0 || (size_t)pack_index >= copy->length) {
    return copy;
  }
  TemplateArgumentDelete(copy->value.p[pack_index]);
  VectorSet(copy, (size_t)pack_index, TemplateArgumentCopy(element));
  return copy;
}

// Re-evaluate a value-dependent non-type template-argument expression (stored
// unevaluated at parse time, e.g. `!is_integral<It>::value`) against concrete
// template arguments; defined after the template-body clone machinery it relies
// on.
static bool TryFoldDependentTemplateArgument(TypeParser* parser, ASTNode* expr,
                                             Vector* args, int64_t* out);
static ASTNode* CloneDependentExpressionWithArgs(TypeParser* parser,
                                                 ASTNode* expr, Vector* args);
static bool TemplateArgumentVectorContainsTemplateParameter(Vector* args);

/* Produce a concrete copy of one template argument `arg` by resolving any
 * template-parameter references against the actual arguments `args`. Type
 * arguments get their referenced type substituted; non-type arguments get the
 * actual integer value. Returns a freshly allocated argument. */
static TemplateArgument* NewSubstitutedTemplateArgument(TypeParser* parser,
                                                       TemplateArgument* arg,
                                                       Vector* args) {
  TemplateArgument* concrete = malloc(sizeof(TemplateArgument));
  concrete->kind = arg->kind;
  concrete->is_pack_expansion = false;
  concrete->type = NULL;
  concrete->int_value = arg->int_value;
  concrete->template_parameter_index = arg->template_parameter_index;
  concrete->pack_arguments = NULL;
  concrete->dependent_expr = NULL;
  concrete->location = arg->location;
  // A value-dependent non-type argument (e.g. an `enable_if` SFINAE condition):
  // try to fold it now that some parameters are concrete.  If it folds, the
  // argument becomes an ordinary integer; otherwise keep the expression so a
  // later, more-concrete substitution can complete it.
  if (arg->kind == kTemplateParameterNonType && arg->dependent_expr != NULL) {
    int64_t folded = 0;
    if (TryFoldDependentTemplateArgument(parser, arg->dependent_expr, args,
                                         &folded)) {
      concrete->int_value = folded;
      concrete->template_parameter_index = -1;
    } else {
      // Cannot fold yet (e.g. `Target - I` where the member template's own
      // `Target` is still unknown but the enclosing class's `I` is concrete).
      // Bake the now-known parameters into a partially substituted copy so the
      // remaining parameter is the only one left symbolic; keeping the raw,
      // unsubstituted expression would leave the concrete parameter's index
      // dangling and later collide with the member template's own parameters
      // after rebasing.
      ASTNode* partial =
          CloneDependentExpressionWithArgs(parser, arg->dependent_expr, args);
      concrete->dependent_expr =
          partial != NULL ? partial : arg->dependent_expr;
    }
    return concrete;
  }
  if (arg->kind == kTemplateParameterType && arg->type != NULL) {
    concrete->type = SubstituteTemplateParameters(parser, arg->type, args);
    concrete->template_parameter_index = -1;
  } else if (arg->kind == kTemplateParameterType &&
             arg->template_parameter_index >= 0 &&
             (size_t)arg->template_parameter_index < args->length) {
    TemplateArgument* actual = args->value.p[arg->template_parameter_index];
    if (actual != NULL && actual->kind == kTemplateParameterType) {
      if (actual->pack_arguments != NULL) {
        TemplateArgumentDelete(concrete);
        return TemplateArgumentCopy(actual);
      }
      if (actual->type != NULL) {
        concrete->type = TypeRecordCopy(actual->type);
        concrete->template_parameter_index = -1;
      } else {
        concrete->template_parameter_index = actual->template_parameter_index;
      }
    }
  } else if (arg->kind == kTemplateParameterNonType &&
             arg->template_parameter_index >= 0 &&
             (size_t)arg->template_parameter_index < args->length) {
    TemplateArgument* actual = args->value.p[arg->template_parameter_index];
    if (actual != NULL && actual->kind == kTemplateParameterNonType &&
        actual->pack_arguments == NULL) {
      concrete->int_value = actual->int_value;
      concrete->template_parameter_index = actual->template_parameter_index;
    }
  }
  return concrete;
}

/* If `symbol`'s value depends on a non-type template parameter, resolve it
 * against `args`: either bind a concrete integer value or remap to another
 * parameter index (when the actual is itself still parameter-dependent). */
static void SubstituteDependentSymbolValue(Symbol* symbol, Vector* args) {
  if (symbol == NULL ||
      symbol->dependent_value_template_parameter_index < 0) {
    return;
  }
  int index = symbol->dependent_value_template_parameter_index;
  if (args == NULL || (size_t)index >= args->length) {
    return;
  }
  TemplateArgument* arg = args->value.p[index];
  if (arg == NULL || arg->kind != kTemplateParameterNonType ||
      arg->pack_arguments != NULL) {
    return;
  }
  if (arg->template_parameter_index >= 0) {
    symbol->dependent_value_template_parameter_index =
        arg->template_parameter_index;
    symbol->flags.value_set = false;
    return;
  }
  symbol->value.ivalue = arg->int_value;
  symbol->flags.value_set = true;
  symbol->dependent_value_template_parameter_index = -1;
}

/* Diagnose an ill-formed pack expansion (a `...` whose pattern contains no
 * expandable parameter pack, e.g. `decltype(T())...` where `T` is not a pack).
 *
 * The offending pattern comes from the template *definition* and is
 * re-substituted for every member that mentions it and every time a member is
 * (re)analyzed; a type alias multiplies this further, since each use expands the
 * alias into a fresh copy of the pattern node.  A naive `SyntaxError` therefore
 * fires many times for a single source construct.  Recover by reporting at most
 * once per distinct diagnosis site: we key on both the pattern node's identity
 * (catches re-substitution of the same node) and the reported source location
 * (catches distinct copies produced at the same instantiation point).  A node /
 * location is only remembered once its diagnostic actually reaches the user
 * (i.e. it was not swallowed by a speculative SFINAE trap or a suppression
 * scope), so a genuine later error is still reported. */
static void ReportPackExpansionRequiresPack(TypeParser* parser,
                                            TemplateArgument* arg) {
  static Set reported_nodes;
  static Set reported_locations;
  static bool reported_init = false;
  if (!reported_init) {
    SetInitForPointers(&reported_nodes);
    SetInitForIntegers(&reported_locations);
    reported_init = true;
  }
  // Prefer the location recorded on the pattern node (where the construct was
  // written); fall back to the lexer's current position only when it is
  // unavailable (e.g. a synthesized argument).
  const char* filename;
  int lineno, start, end;
  DecodeSourceLocation(arg->location, &filename, &lineno, &start, &end);
  bool have_recorded_location =
      arg->location != SOURCE_LOCATION_MISSING && lineno > 0;
  // The location key must match the point we will actually report at, so that
  // copies of one pattern (produced e.g. by expanding a type alias at each use)
  // collapse to a single diagnosis while genuinely distinct occurrences stay
  // separate.  Fold the filename pointer and line together for a stable key.
  const char* key_file = have_recorded_location
                             ? filename
                             : parser->syntax->lex->source->filename.value;
  int key_line = have_recorded_location
                     ? lineno
                     : parser->syntax->lex->source->lineno;
  intptr_t location_key =
      (intptr_t)((((uintptr_t)key_file) << 20) ^ (uintptr_t)key_line);
  if (SetContains(&reported_nodes, arg) ||
      SetContains(&reported_locations, (void*)location_key)) {
    return;
  }
  int before = NumErrors();
  if (have_recorded_location) {
    SyntaxErrorAtLocation(
        parser->syntax, arg->location,
        "template argument pack expansion requires a parameter pack");
  } else {
    SyntaxError(parser->syntax,
                "template argument pack expansion requires a parameter pack");
  }
  if (NumErrors() > before) {
    SetInsert(&reported_nodes, arg);
    SetInsert(&reported_locations, (void*)location_key);
  }
}

/* Substitute one template argument and append the result(s) to `out`. A normal
 * argument appends a single substituted argument; a pack-expansion argument
 * (e.g. `Ts...`) expands into one substituted argument per pack element. */
static void AppendSubstitutedTemplateArgument(TypeParser* parser, Vector* out,
                                              TemplateArgument* arg,
                                              Vector* args) {
  if (arg == NULL) {
    return;
  }
  int index = -1;
  if (arg->is_pack_expansion) {
    TemplateArgument* pack = NULL;
    if (arg->kind == kTemplateParameterType &&
        TypeIsTemplateParameterPlaceholder(arg->type, &index) &&
        index >= 0 && (size_t)index < args->length) {
      pack = args->value.p[index];
    } else if (arg->kind == kTemplateParameterNonType &&
               arg->template_parameter_index >= 0 &&
               (size_t)arg->template_parameter_index < args->length) {
      pack = args->value.p[arg->template_parameter_index];
    }
    if (pack != NULL && pack->pack_arguments != NULL) {
      for (size_t i = 0; i < pack->pack_arguments->length; i++) {
        VectorAppend(out, TemplateArgumentCopy(pack->pack_arguments->value.p[i]));
      }
      return;
    }
    int pattern_pack_index = -1;
    size_t pattern_pack_length = 0;
    if (arg->kind == kTemplateParameterType &&
        FindPackExpansionInType(arg->type, args, &pattern_pack_index,
                                &pattern_pack_length) &&
        pattern_pack_index >= 0 && (size_t)pattern_pack_index < args->length) {
      TemplateArgument* pattern_pack = args->value.p[pattern_pack_index];
      if (pattern_pack != NULL && pattern_pack->pack_arguments != NULL) {
        for (size_t i = 0; i < pattern_pack->pack_arguments->length; i++) {
          Vector* element_args = TemplateArgumentVectorCopyWithPackElement(
              args, pattern_pack_index, pattern_pack->pack_arguments->value.p[i]);
          VectorAppend(out,
                       NewSubstitutedTemplateArgument(parser, arg, element_args));
          VectorDeleteWithContents(
              element_args, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
        }
        return;
      }
    }
    ReportPackExpansionRequiresPack(parser, arg);
    return;
  }

  // An already-bound parameter pack (its elements are populated but it is not
  // itself a pack expansion, e.g. the `Types...` binding carried by a concrete
  // `variant<int, char>`): re-substitute each element and keep the result as a
  // single pack argument.  Without this the pack -- whose own `type` is NULL --
  // would fall into the `type == NULL` early-return below and be dropped,
  // corrupting an already-concrete instantiation into an empty one when its
  // type is re-substituted in an unrelated context.
  if (arg->pack_arguments != NULL) {
    TemplateArgument* pack = malloc(sizeof(TemplateArgument));
    pack->kind = arg->kind;
    pack->is_pack_expansion = false;
    pack->type = NULL;
    pack->int_value = arg->int_value;
    pack->template_parameter_index = -1;
    pack->pack_arguments = NewVector();
    pack->dependent_expr = NULL;
    pack->location = arg->location;
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      AppendSubstitutedTemplateArgument(parser, pack->pack_arguments,
                                        arg->pack_arguments->value.p[i], args);
    }
    VectorAppend(out, pack);
    return;
  }

  if (arg->kind == kTemplateParameterType && arg->type == NULL) {
    return;
  }

  VectorAppend(out, NewSubstitutedTemplateArgument(parser, arg, args));
}

/* Substitute every argument in `template_args` against the actuals `args`,
 * expanding pack expansions, to yield a fully concrete argument vector. */
static Vector* SubstituteTemplateArgumentVectorForTypes(TypeParser* parser,
                                                        Vector* template_args,
                                                        Vector* args) {
  if (template_args == NULL) {
    return NULL;
  }
  Vector* concrete_args = NewVector();
  for (size_t i = 0; i < template_args->length; i++) {
    AppendSubstitutedTemplateArgument(parser, concrete_args,
                                      template_args->value.p[i], args);
  }
  return concrete_args;
}

/* True if a function type has a parameter declared as a pack (variadic
 * template parameter), e.g. `void f(Ts... args)`. */
static bool FunctionTypeHasParameterPack(TypeRecord* type) {
  if (type == NULL || !TypeIsFunction(type)) {
    return false;
  }
  for (size_t i = 0; i < type->info.function.prototype.length; i++) {
    Symbol* formal = type->info.function.prototype.value.p[i];
    if (formal != NULL && formal->flags.is_parameter_pack) {
      return true;
    }
  }
  return false;
}

/* Core type substitution: produce a concrete copy of `type` with every
 * template-parameter reference resolved against the actual arguments `args`.
 * Handles, in order:
 *   - rewriting a self-referential class type to the instantiation in progress;
 *   - dependent member typedefs (`T::member`);
 *   - nested class-template / alias-template instantiations;
 *   - bare type parameters (`T`) and non-type array bounds (`T[N]`);
 *   - structs that still contain parameters, pointer/reference chains (with
 *     reference collapsing), and function prototypes (expanding parameter
 *     packs).
 * Returns a newly allocated, size-calculated type record. */
static TypeRecord* SubstituteTemplateParameters(TypeParser* parser,
                                                TypeRecord* type,
                                                Vector* args) {
  if (type == NULL) {
    return NULL;
  }
  /* Self-reference: a class template referring to its own type substitutes to
   * the concrete instantiation currently being produced. */
  if (CompilerIsCXX() && parser != NULL &&
      parser->template_substitution_source != NULL &&
      parser->template_substitution_target != NULL &&
      type->declarator == kDeclPrimitive && TypeIsStructOrUnion(type) &&
      type->info.struct_info == parser->template_substitution_source) {
    TypeRecord* subst = TypeRecordCopy(type);
    subst->info.struct_info = parser->template_substitution_target;
    if (parser->template_substitution_target->tag_symbol != NULL &&
        parser->template_substitution_target->tag_symbol->type != NULL) {
      if (subst->template_arguments != NULL) {
        VectorDeleteWithContents(subst->template_arguments,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
      }
      subst->template_origin =
          parser->template_substitution_target->tag_symbol->type->template_origin;
      subst->template_arguments =
          TemplateArgumentVectorCopy(parser->template_substitution_target
                                         ->tag_symbol->type->template_arguments);
    }
    return TypeRecordCalculateSize(subst);
  }
  /* Dependent member typedef like `T::value_type`: resolve `T` then look the
   * named typedef up inside the resulting struct. */
  if (type->declarator == kDeclPrimitive &&
      type->template_parameter_index >= 0 &&
      type->dependent_member_name != NULL) {
    int index = type->template_parameter_index;
    if (index < 0 || (size_t)index >= args->length) {
      return TypeRecordCopy(type);
    }
    TemplateArgument* arg = args->value.p[index];
    if (arg == NULL || arg->kind != kTemplateParameterType ||
        arg->type == NULL || !TypeIsStructOrUnion(arg->type) ||
        arg->type->info.struct_info == NULL) {
      return TypeRecordCopy(type);
    }
    StructMember* member =
        FindStructMember(arg->type->info.struct_info,
                         type->dependent_member_name);
    if (member == NULL || member->symbol == NULL ||
        !StorageIs(member->symbol->storage, STO(typedef))) {
      // A missing member typedef on a concrete instantiation is a substitution
      // failure in the immediate context (SFINAE); flag it for callers.
      if (parser != NULL &&
          !StructContainsTemplateParameter(arg->type->info.struct_info)) {
        parser->template_substitution_failed = true;
      }
      return TypeRecordCopy(type);
    }
    TypeRecord* subst = TypeRecordCopy(member->symbol->type);
    subst->qualifiers |= type->qualifiers;
    return TypeRecordCalculateSize(subst);
  }
  Symbol* type_template_origin = type->template_origin;
  Vector* type_template_args = type->template_arguments;
  if (TypeIsStructOrUnion(type) &&
      type->info.struct_info != NULL &&
      type->info.struct_info->tag_symbol != NULL &&
      type->info.struct_info->tag_symbol->type != NULL) {
    TypeRecord* tag_type = type->info.struct_info->tag_symbol->type;
    if (tag_type->template_origin != NULL &&
        (type_template_origin == NULL ||
         tag_type->template_origin == type_template_origin)) {
      type_template_origin = tag_type->template_origin;
      if (type_template_args == NULL) {
        type_template_args = tag_type->template_arguments;
      }
    } else if (type_template_origin == NULL && type_template_args != NULL &&
               type->info.struct_info->is_template &&
               type->info.struct_info->tag_symbol->flags.is_template) {
      type_template_origin = type->info.struct_info->tag_symbol;
    }
  }
  /* A reference to another template (e.g. `Wrapper<T>`): substitute its
   * arguments, then either expand an alias template inline or instantiate the
   * concrete class template. */
  if (type_template_origin != NULL && type_template_args != NULL) {
    Vector* concrete_args =
        SubstituteTemplateArgumentVectorForTypes(parser,
                                                type_template_args, args);
    // If the substituted arguments still mention a template parameter (an outer
    // parameter has not been supplied yet), instantiating now would wrongly pick
    // the primary template.  Keep this as a dependent template-id so its member
    // type or value (`is_integral<It>::value`) is resolved only once every
    // referenced parameter is concrete.
    if (TemplateArgumentVectorContainsTemplateParameter(concrete_args)) {
      TypeRecord* deferred = TypeRecordCopy(type);
      if (deferred->template_arguments != NULL) {
        VectorDeleteWithContents(
            deferred->template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      deferred->template_arguments = concrete_args;
      deferred->qualifiers |= type->qualifiers;
      return deferred;
    }
    if (CompilerIsCXX() && type_template_origin->flags.is_template &&
        StorageIs(type_template_origin->storage, STO(typedef)) &&
        !TypeIsStructOrUnion(type_template_origin->type) &&
        !CXXAliasTemplatePatternNamesClassTemplate(type_template_origin)) {
      TypeRecord* subst =
          SubstituteTemplateParameters(parser, type_template_origin->type,
                                       concrete_args);
      subst->qualifiers |= type->qualifiers;
      VectorDeleteWithContents(concrete_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return TypeRecordCalculateSize(subst);
    }
    TypeRecord* subst =
        InstantiateSimpleClassTemplate(parser, type_template_origin,
                                       concrete_args);
    if (subst == NULL) {
      VectorDeleteWithContents(concrete_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return TypeRecordCopy(type);
    }
    subst->qualifiers |= type->qualifiers;
    VectorDeleteWithContents(concrete_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    if (type->dependent_member_name != NULL &&
        TypeIsStructOrUnion(subst) && subst->info.struct_info != NULL) {
      StructMember* member =
          FindStructMember(subst->info.struct_info,
                           type->dependent_member_name);
      if (member != NULL && member->symbol != NULL &&
          StorageIs(member->symbol->storage, STO(typedef))) {
        TypeRecord* member_type = TypeRecordCopy(member->symbol->type);
        member_type->qualifiers |= type->qualifiers;
        TypeRecordDelete(subst);
        return TypeRecordCalculateSize(member_type);
      }
      // The dependent member typedef (e.g. `enable_if<false, T>::type`) does not
      // exist in this concrete instantiation.  That is a substitution failure in
      // the immediate context; record it so an SFINAE-sensitive caller discards
      // this candidate rather than proceeding with an ill-formed type.
      if (parser != NULL &&
          !StructContainsTemplateParameter(subst->info.struct_info)) {
        parser->template_substitution_failed = true;
      }
    }
    return TypeRecordCalculateSize(subst);
  }
  /* A bare type parameter `T`: replace with the actual type argument, carrying
   * over any cv-qualifiers from the parameter use. */
  if (type->declarator == kDeclPrimitive &&
      type->template_parameter_index >= 0) {
    int index = type->template_parameter_index;
    if (index < 0 || (size_t)index >= args->length) {
      return TypeRecordCopy(type);
    }
    TemplateArgument* arg = args->value.p[index];
    if (arg == NULL || arg->kind != kTemplateParameterType ||
        arg->type == NULL) {
      return TypeRecordCopy(type);
    }
    TypeRecord* subst = TypeRecordCopy(arg->type);
    subst->qualifiers |= type->qualifiers;
    return subst;
  }
  /* A struct that still embeds parameters (e.g. a nested member type): recurse
   * into its members. */
  if (CompilerIsCXX() && TypeIsStructOrUnion(type) &&
      StructContainsTemplateParameter(type->info.struct_info)) {
    if (type->info.struct_info != NULL &&
        type->info.struct_info->is_template &&
        type_template_origin == NULL && type_template_args == NULL) {
      return TypeRecordCopy(type);
    }
    return SubstituteNestedStructTemplateParameters(parser, type, args);
  }

  /* Otherwise: copy and substitute compound pieces (array bounds, the
   * pointed-to/element type, and function prototypes). */
  TypeRecord* copy = TypeRecordCopy(type);
  if (copy->declarator == kDeclArray &&
      type->info.array.template_parameter_index >= 0) {
    int index = type->info.array.template_parameter_index;
    if ((size_t)index < args->length) {
      TemplateArgument* arg = args->value.p[index];
      if (arg != NULL && arg->kind == kTemplateParameterNonType) {
        if (arg->template_parameter_index >= 0) {
          copy->info.array.template_parameter_index =
              arg->template_parameter_index;
        } else {
          copy->info.array.size.fixed = (int)arg->int_value;
          copy->info.array.template_parameter_index = -1;
          copy->size = 0;
        }
      }
    }
  }
  if (copy->next != NULL) {
    TypeRecord* original_next = type->next;
    TypeRecordIncRef(original_next);
    TypeRecordDelete(copy->next);
    copy->next = SubstituteTemplateParameters(parser, original_next, args);
    TypeRecordDelete(original_next);
    if (copy->next != NULL) {
      /* Reference collapsing: T& & -> T&, T&& && -> T&&, otherwise -> T&. */
      bool collapsed_reference = false;
      if (TypeIsReference(copy) && TypeIsReference(copy->next)) {
        TypeRecord* nested = copy->next;
        TypeRecord* collapsed_next = nested->next;
        bool rvalue = copy->declarator == kDeclRValueReference &&
                      nested->declarator == kDeclRValueReference;
        TypeRecordIncRef(collapsed_next);
        TypeRecordDelete(copy->next);
        copy->next = collapsed_next;
        copy->declarator = rvalue ? kDeclRValueReference : kDeclReference;
        collapsed_reference = true;
      }
      /* Pointer-to-reference is not a valid C++ type.  It arises when a
       * by-reference lambda capture field is `U*` and `U` substitutes to a
       * reference (e.g. capturing an `auto&&` parameter whose deduced `T` is
       * `int&`).  Form a pointer to the referent instead. */
      if (!collapsed_reference && TypeIsPointer(copy) &&
          TypeIsReference(copy->next)) {
        TypeRecord* nested = copy->next;
        TypeRecord* referent = nested->next;
        TypeRecordIncRef(referent);
        TypeRecordDelete(copy->next);
        copy->next = referent;
        collapsed_reference = true;
      }
      if (!collapsed_reference) {
        TypeRecordIncRef(copy->next);
      }
      copy->type = copy->next->type;
    }
  }
  if (TypeIsFunction(copy)) {
    /* Rebuild the parameter list. With a parameter pack, each pack expands into
     * zero or more concrete formals; otherwise substitute each formal in place. */
    if (FunctionTypeHasParameterPack(type)) {
      VectorDestructWithContents(&copy->info.function.prototype,
                                 (VectorElementDestructor)SymbolDestruct,
                                 /*free_element=*/true);
      VectorInit(&copy->info.function.prototype);
      for (size_t i = 0; i < type->info.function.prototype.length; i++) {
        Symbol* original = type->info.function.prototype.value.p[i];
        if (original == NULL || original->type == NULL) {
          continue;
        }
        AppendSubstitutedFormalParameter(parser, &copy->info.function.prototype,
                                         original, args, 0);
      }
      for (size_t i = 0; i < copy->info.function.prototype.length; i++) {
        Symbol* formal = copy->info.function.prototype.value.p[i];
        formal->value.arg_number = (int32_t)i;
      }
    } else {
      for (size_t i = 0; i < copy->info.function.prototype.length; i++) {
        Symbol* formal = copy->info.function.prototype.value.p[i];
        Symbol* original = type->info.function.prototype.value.p[i];
        if (formal == NULL || original == NULL || original->type == NULL) {
          continue;
        }
        TypeRecord* formal_type =
            SubstituteTemplateParameters(parser, original->type, args);
        SymbolSetType(formal, formal_type);
        TypeRecordDelete(formal_type);
      }
    }
  }
  return TypeRecordCalculateSize(copy);
}

/* Append a clone of formal parameter `formal` with the already-substituted
 * `formal_type` to the prototype `out`, optionally rebasing any remaining
 * template-parameter indices in the type by `rebase_base`. */
static void AppendFormalClone(Vector* out, Symbol* formal,
                              TypeRecord* formal_type,
                              int rebase_base) {
  if (formal == NULL || formal_type == NULL) {
    return;
  }
  if (rebase_base > 0) {
    RebaseTemplateParameterIndices(formal_type, rebase_base);
  }
  Symbol* clone = NewSymbol(formal->name.value, formal_type, formal->storage);
  clone->flags = formal->flags;
  clone->flags.is_argument = true;
  clone->flags.is_parameter_pack = false;
  clone->location = formal->location;
  clone->default_argument =
      ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
  VectorAppend(out, clone);
}

/* Substitute one formal parameter against `args` and append the result(s) to
 * the prototype `out`. A pack parameter (`Ts... xs`) expands into one formal
 * per pack element; a non-pack parameter substitutes to a single formal. */
static void AppendSubstitutedFormalParameter(TypeParser* parser, Vector* out,
                                             Symbol* formal, Vector* args,
                                             int rebase_base) {
  int index = -1;
  if (formal != NULL && formal->flags.is_parameter_pack &&
      TypeIsTemplateParameterPlaceholder(formal->type, &index) &&
      index >= 0 && (size_t)index < args->length) {
    TemplateArgument* pack = args->value.p[index];
    if (pack != NULL && pack->pack_arguments != NULL) {
      for (size_t i = 0; i < pack->pack_arguments->length; i++) {
        TemplateArgument* element = pack->pack_arguments->value.p[i];
        if (element == NULL || element->kind != kTemplateParameterType ||
            element->type == NULL) {
          continue;
        }
        TypeRecord* formal_type = TypeRecordCopy(element->type);
        AppendFormalClone(out, formal, formal_type, rebase_base);
      }
      return;
    }
  }
  int pattern_pack_index = -1;
  size_t pattern_pack_length = 0;
  if (formal != NULL && formal->flags.is_parameter_pack &&
      FindPackExpansionInType(formal->type, args, &pattern_pack_index,
                              &pattern_pack_length) &&
      pattern_pack_index >= 0 && (size_t)pattern_pack_index < args->length) {
    TemplateArgument* pack = args->value.p[pattern_pack_index];
    if (pack != NULL && pack->pack_arguments != NULL) {
      for (size_t i = 0; i < pattern_pack_length; i++) {
        TypeRecord* formal_type =
            SubstituteTemplateParametersForPackElement(
                parser, formal->type, args, pattern_pack_index, i);
        AppendFormalClone(out, formal, formal_type, rebase_base);
      }
      return;
    }
  }

  TypeRecord* formal_type =
      SubstituteTemplateParameters(parser, formal->type, args);
  size_t before = out->length;
  AppendFormalClone(out, formal, formal_type, rebase_base);
  if (formal != NULL && formal->flags.is_parameter_pack && out->length > before) {
    // Reached only when the pack could not be expanded here because its
    // argument is not among `args` (e.g. instantiating an enclosing class
    // template whose member function template still has an unexpanded pack).
    // AppendFormalClone clears is_parameter_pack for expanded elements, but this
    // formal is still a pack and must stay one so the later member-template
    // instantiation can recognize and deduce it.
    Symbol* appended = out->value.p[out->length - 1];
    appended->flags.is_parameter_pack = true;
  }
}

/* Build the synthetic name for the `index`-th element of a lambda capture pack,
 * e.g. base "xs" -> "xs$pack0". */
static void LambdaCapturePackElementName(String* name, const char* base,
                                         size_t index) {
  StringPrintf(name, "%s$pack%zu", base, index);
}

/* True if `name` is a synthetic element name (base + "$packN") of capture pack
 * `base`. */
static bool LambdaCapturePackElementMatches(const char* name, const char* base) {
  if (name == NULL || base == NULL) {
    return false;
  }
  size_t base_len = strlen(base);
  if (strncmp(name, base, base_len) != 0) {
    return false;
  }
  const char* suffix = name + base_len;
  if (strncmp(suffix, "$pack", 5) != 0) {
    return false;
  }
  suffix += 5;
  if (*suffix == '\0') {
    return false;
  }
  while (*suffix != '\0') {
    if (*suffix < '0' || *suffix > '9') {
      return false;
    }
    suffix++;
  }
  return true;
}

/* Return the first template-parameter index referenced anywhere in `type`
 * (the type chain or its template arguments), or -1 if none. */
static int FirstTemplateParameterIndexInType(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index >= 0) {
      return t->template_parameter_index;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        TemplateArgument* arg = t->template_arguments->value.p[i];
        int nested = FirstTemplateParameterIndexInType(
            arg != NULL ? arg->type : NULL);
        if (nested >= 0) {
          return nested;
        }
      }
    }
  }
  return -1;
}

/* Substitute `type` as if the pack at `pack_index` were the single element
 * `element_index` of that pack. Used to expand a pack-dependent pattern into
 * one type per element. */
static TypeRecord* SubstituteTemplateParametersForPackElement(
    TypeParser* parser, TypeRecord* type, Vector* args, int pack_index,
    size_t element_index) {
  if (args == NULL || pack_index < 0 || (size_t)pack_index >= args->length) {
    return SubstituteTemplateParameters(parser, type, args);
  }
  TemplateArgument* pack = args->value.p[pack_index];
  if (pack == NULL || pack->pack_arguments == NULL ||
      element_index >= pack->pack_arguments->length) {
    return SubstituteTemplateParameters(parser, type, args);
  }
  Vector element_args;
  VectorInit(&element_args);
  for (size_t i = 0; i < args->length; i++) {
    VectorAppend(&element_args,
                 i == (size_t)pack_index ? pack->pack_arguments->value.p[element_index]
                                          : args->value.p[i]);
  }
  TypeRecord* result = SubstituteTemplateParameters(parser, type,
                                                    &element_args);
  VectorDestruct(&element_args);
  return result;
}

/* Substitute a struct/union type that still contains template parameters by
 * rebuilding it member-by-member: each non-pack data member is substituted and
 * re-laid-out, member packs expand into one synthetically-named field per pack
 * element, and member functions are instantiated last (deferred so the layout
 * is finalized first). Returns a copy whose struct_info is the new struct. */
static TypeRecord* SubstituteNestedStructTemplateParameters(TypeParser* parser,
                                                            TypeRecord* type,
                                                            Vector* args) {
  Struct* from = type->info.struct_info;
  TypeRecord* copy = TypeRecordCopy(type);
  Struct* str = NewStruct(from->is_union);
  if (StructHasMemberFunction(from)) {
    String synthetic_name;
    StringInit(&synthetic_name, NULL);
    StringPrintf(&synthetic_name, "%s$S%p",
                 from->tag_name != NULL ? from->tag_name->value : "<anon>",
                 (void*)str);
    Symbol* tag = NewSymbol(synthetic_name.value, copy, STO(implicit));
    tag->flags.is_defined = true;
    // Preserve invented-ness so nested lambda closures rebuilt during
    // template substitution are still recognized as lambda closures (their
    // operator() bodies must be cloned, not left lazy against the template).
    if (from->tag_symbol != NULL) {
      tag->flags.invented = from->tag_symbol->flags.invented;
    }
    str->tag_name = &tag->name;
    str->tag_symbol = tag;
    StringDestruct(&synthetic_name);
  } else {
    str->tag_name = from->tag_name;
    str->tag_symbol = from->tag_symbol;
  }
  str->is_class = from->is_class;
  str->is_template = from->is_template;
  str->is_aggregate = from->is_aggregate;
  str->cxx_special_members_complete = from->cxx_special_members_complete;
  str->packed = from->packed;
  str->explicit_alignment = from->explicit_alignment;
  str->pack = from->pack;
  for (size_t i = 0; i < from->friend_classes.length; i++) {
    StructAddFriendClass(str, from->friend_classes.value.p[i]);
  }
  for (size_t i = 0; i < from->friend_functions.length; i++) {
    StructAddFriendFunction(str, from->friend_functions.value.p[i]);
  }
  copy->info.struct_info = str;
  copy->size = 0;

  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* template_base = from->bases.value.p[i];
    TypeRecord* base_type =
        SubstituteTemplateParameters(parser, template_base->type, args);
    if (!TypeIsStructOrUnion(base_type)) {
      SyntaxError(parser->syntax, "base class must be a class or struct type");
      TypeRecordDelete(base_type);
      continue;
    }
    TypeRecordCalculateSize(base_type);
    VectorAppend(&str->bases,
                 NewCXXBaseSpecifier(base_type, template_base->access,
                                     template_base->is_virtual));
    TypeRecordDelete(base_type);
  }
  CollectCXXVirtualBases(str);
  CopyCXXBaseVirtualMembers(str);
  LayoutCXXBaseSpecifiers(str);

  Vector deferred_member_functions;
  VectorInit(&deferred_member_functions);
  for (size_t i = 0; i < from->members.length; i++) {
    StructMember* member = from->members.value.p[i];
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    if (member->symbol->flags.is_parameter_pack && !member->is_static &&
        !member->is_member_function && !member->is_using_declaration &&
        !StructMemberIsNestedType(member)) {
      int pack_index = FirstTemplateParameterIndexInType(member->symbol->type);
      TemplateArgument* pack =
          pack_index >= 0 && args != NULL && (size_t)pack_index < args->length
              ? args->value.p[pack_index]
              : NULL;
      if (pack != NULL && pack->pack_arguments != NULL) {
        for (size_t j = 0; j < pack->pack_arguments->length; j++) {
          TypeRecord* member_type = SubstituteTemplateParametersForPackElement(
              parser, member->symbol->type, args, pack_index, j);
          TypeRecordCalculateSize(member_type);
          String field_name;
          StringInit(&field_name, NULL);
          LambdaCapturePackElementName(&field_name,
                                       member->symbol->name.value, j);
          Symbol* member_symbol =
              NewSymbol(field_name.value, member_type, member->symbol->storage);
          StringDestruct(&field_name);
          member_symbol->location = member->symbol->location;
          member_symbol->flags = member->symbol->flags;
          member_symbol->flags.is_parameter_pack = false;
          member_symbol->value = member->symbol->value;
          member_symbol->dependent_value_template_parameter_index =
              member->symbol->dependent_value_template_parameter_index;
          SubstituteDependentSymbolValue(member_symbol, args);

          StructMember* instantiated = NewStructMember(member_symbol);
          instantiated->access = member->access;
          instantiated->is_anon = member->is_anon;
          instantiated->is_static = member->is_static;
          instantiated->is_mutable = member->is_mutable;
          instantiated->is_member_function = member->is_member_function;
          instantiated->is_using_declaration = member->is_using_declaration;
          instantiated->bit_size = member->bit_size;
          instantiated->bit_offset = member->bit_offset;
          instantiated->cxx_vcall_offset = member->cxx_vcall_offset;
          AlignNextOffset(str, member_type);
          instantiated->byte_offset = str->next_offset;
          instantiated->index = str->members.length;
          AddStructMember(parser, str, instantiated);
          UpdateStructSize(str, member_type, str->is_union);
        }
        continue;
      }
    }
    if (member->is_member_function) {
      VectorAppend(&deferred_member_functions, member);
      continue;
    }
    TypeRecord* member_type =
        SubstituteTemplateParameters(parser, member->symbol->type, args);
    TypeRecordCalculateSize(member_type);
    Symbol* member_symbol =
        NewSymbol(member->symbol->name.value, member_type, member->symbol->storage);
    member_symbol->location = member->symbol->location;
    member_symbol->flags = member->symbol->flags;
    member_symbol->value = member->symbol->value;
    member_symbol->dependent_value_template_parameter_index =
        member->symbol->dependent_value_template_parameter_index;
    SubstituteDependentSymbolValue(member_symbol, args);

    StructMember* instantiated = NewStructMember(member_symbol);
    instantiated->default_initializer =
        CloneCXXDefaultMemberInitializer(member->default_initializer);
    instantiated->access = member->access;
    instantiated->is_anon = member->is_anon;
    instantiated->is_static = member->is_static;
    instantiated->is_mutable = member->is_mutable;
    instantiated->is_member_function = member->is_member_function;
    instantiated->is_using_declaration = member->is_using_declaration;
    instantiated->bit_size = member->bit_size;
    instantiated->bit_offset = member->bit_offset;
    instantiated->cxx_vcall_offset = member->cxx_vcall_offset;

    if (!instantiated->is_static && !instantiated->is_member_function &&
        !instantiated->is_using_declaration &&
        !StructMemberIsNestedType(instantiated)) {
      AlignNextOffset(str, member_type);
      instantiated->byte_offset = str->next_offset;
      instantiated->index = str->members.length;
      AddStructMember(parser, str, instantiated);
      UpdateStructSize(str, member_type, str->is_union);
    } else {
      instantiated->byte_offset = member->byte_offset;
      instantiated->index = str->members.length;
      AddStructMember(parser, str, instantiated);
    }
  }
  Vector pending_member_bodies;
  VectorInit(&pending_member_bodies);
  for (size_t i = 0; i < deferred_member_functions.length; i++) {
    StructMember* member = deferred_member_functions.value.p[i];
    StructMember* instantiated = InstantiateTemplateMemberFunction(
        parser, str, member, args, &pending_member_bodies);
    StructMember* existing =
        MapFindPointerKey(&str->symbol_table, &instantiated->symbol->name);
    if (existing != NULL) {
      AppendStructMemberOverload(parser, str, existing, instantiated);
    } else {
      AddStructMember(parser, str, instantiated);
    }
  }
  VectorDestruct(&deferred_member_functions);
  for (size_t i = 0; i < pending_member_bodies.length; i++) {
    PendingMemberBody* pmb = pending_member_bodies.value.p[i];
    CloneInstantiatedMemberFunctionBody(parser, str, pmb->symbol,
                                        pmb->template_definition,
                                        pmb->substitution_source, args);
    free(pmb);
  }
  VectorDestruct(&pending_member_bodies);
  FinalizeStructAlignment(str);
  return TypeRecordCalculateSize(copy);
}

bool TypeContainsTemplateParameter(TypeRecord* type);

static void RebaseTemplateArgumentParameterIndices(TemplateArgument* arg,
                                                   int base);
static bool DependentExpressionContainsTemplateParameter(ASTNode* expr);
static ASTNode* CloneAndRebaseDependentExpression(ASTNode* expr, int base);

/* Shift every template-parameter index in `type` down by `base`. Used when a
 * member/inner template's parameters follow the enclosing template's
 * parameters and must be renumbered to start from zero. */
/* Rebase placeholder indices along a type spine (pointers/refs/arrays/args)
 * without descending into struct member lists.  Used when adjusting capture
 * field types so we do not walk into a captured closure's own members (which
 * would corrupt an unrelated lambda's operator() template parameters). */
static void RebaseTemplateParameterIndicesSpine(TypeRecord* type, int base) {
  if (type == NULL || base <= 0) {
    return;
  }
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index >= base) {
      t->template_parameter_index -= base;
    }
    if (t->declarator == kDeclArray &&
        t->info.array.template_parameter_index >= base) {
      t->info.array.template_parameter_index -= base;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        RebaseTemplateArgumentParameterIndices(t->template_arguments->value.p[i],
                                               base);
      }
    }
  }
}

static void RebaseTemplateParameterIndices(TypeRecord* type, int base) {
  if (type == NULL || base <= 0) {
    return;
  }
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index >= base) {
      t->template_parameter_index -= base;
    }
    if (t->declarator == kDeclArray &&
        t->info.array.template_parameter_index >= base) {
      t->info.array.template_parameter_index -= base;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        RebaseTemplateArgumentParameterIndices(t->template_arguments->value.p[i],
                                               base);
      }
    }
    // Nested lambda closures embed placeholder capture-field types numbered
    // relative to an enclosing template.  After substituting the enclosing
    // arguments, those own-parameter placeholders must be renumbered too or
    // later instantiation still sees them as enclosing-relative.
    // Only walk non-function data members, and only along each field's type
    // spine: descending into a pointed-to/referenced struct would corrupt
    // captured visitor closures' own template parameters.
    if (TypeIsStructOrUnion(t) && t->info.struct_info != NULL) {
      Struct* str = t->info.struct_info;
      for (size_t i = 0; i < str->members.length; i++) {
        StructMember* member = str->members.value.p[i];
        if (member == NULL || member->symbol == NULL ||
            member->is_member_function || member->is_static ||
            member->is_using_declaration || StructMemberIsNestedType(member)) {
          continue;
        }
        RebaseTemplateParameterIndicesSpine(member->symbol->type, base);
      }
    }
  }
}

/* Rebase (see RebaseTemplateParameterIndices) the parameter indices inside a
 * template argument and its referenced type. */
static void RebaseTemplateArgumentParameterIndices(TemplateArgument* arg,
                                                   int base) {
  if (arg == NULL || base <= 0) {
    return;
  }
  if (arg->template_parameter_index >= base) {
    arg->template_parameter_index -= base;
  }
  RebaseTemplateParameterIndices(arg->type, base);
  arg->dependent_expr =
      CloneAndRebaseDependentExpression(arg->dependent_expr, base);
}

/* A generic lambda written inside another template numbers its invented `auto`
 * parameters after the enclosing template's parameters and records that offset
 * as the call operator's template-parameter base (see NewLambdaCallOperator).
 * A closure that captures template-dependent state is rebuilt per enclosing
 * instantiation, and that rebuild rebases its operator to a 0-based standalone
 * template.  A closure that captures nothing template-dependent, however, is
 * genuinely identical across every instantiation of the enclosing template and
 * is therefore never rebuilt, so its operator keeps the enclosing-relative
 * numbering forever and neither deduction nor substitution (both of which
 * assume a 0-based own-parameter list) can instantiate it.
 *
 * Rebase such an operator to a 0-based standalone template in place, once: the
 * closure does not depend on the enclosing template, so this is the numbering
 * it should have had all along.  Dependent-capture closures are left untouched
 * (base > 0) so the rebuild path can still expand them safely. */
void TypeRebaseNonDependentLambdaCallOperator(Struct* closure, Symbol* op) {
  if (closure == NULL || op == NULL || op->type == NULL ||
      !TypeIsFunction(op->type)) {
    return;
  }
  TypeRecord* func = op->type;
  int base = func->info.function.template_parameter_base;
  if (base <= 0 || StructContainsTemplateParameter(closure)) {
    return;
  }
  RebaseTemplateParameterIndices(func->next, base);
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL) {
      RebaseTemplateParameterIndices(formal->type, base);
    }
  }
  for (size_t i = 0; i < func->info.function.template_parameters.length; i++) {
    TemplateParameter* param =
        func->info.function.template_parameters.value.p[i];
    if (param != NULL && param->index >= base) {
      param->index -= base;
    }
  }
  func->info.function.template_parameter_base = 0;
}

typedef struct {
  int base;
} RebaseDependentExpressionData;

static void RebaseDependentExpressionVisitor(ASTNode* node, void* data,
                                             int child_id, VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return;
  }
  RebaseDependentExpressionData* rebase = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  Symbol* old = id->symbol;
  if (old == NULL) {
    return;
  }
  bool needs_copy = old->template_parameter_index >= rebase->base ||
                    old->dependent_value_template_parameter_index >=
                        rebase->base ||
                    TypeContainsTemplateParameter(old->type) ||
                    (id->template_arguments != NULL &&
                     TemplateArgumentVectorContainsTemplateParameter(
                         id->template_arguments));
  if (!needs_copy) {
    return;
  }
  TypeRecord* type =
      old->type != NULL ? TypeRecordCopy(old->type) : NULL;
  RebaseTemplateParameterIndices(type, rebase->base);
  Symbol* copy = NewSymbol(old->name.value, type, old->storage);
  copy->namespace_ = old->namespace_;
  copy->flags = old->flags;
  copy->alignment = old->alignment;
  copy->template_parameter_index = old->template_parameter_index;
  if (copy->template_parameter_index >= rebase->base) {
    copy->template_parameter_index -= rebase->base;
  }
  copy->dependent_value_template_parameter_index =
      old->dependent_value_template_parameter_index;
  if (copy->dependent_value_template_parameter_index >= rebase->base) {
    copy->dependent_value_template_parameter_index -= rebase->base;
  }
  copy->location = old->location;
  copy->value = old->value;
  copy->stack_offset = old->stack_offset;
  copy->alias_target = old->alias_target;
  id->symbol = copy;
  ASTNodeSetType(node, copy->type);
}

static ASTNode* CloneAndRebaseDependentExpression(ASTNode* expr, int base) {
  if (expr == NULL || base <= 0) {
    return expr;
  }
  ASTNode* clone = ASTNodeClone(expr, IdentityCloneNode, NULL, NULL);
  RebaseDependentExpressionData rebase = {.base = base};
  ASTNodeVisit(clone, RebaseDependentExpressionVisitor, 0, &rebase);
  return clone;
}

/* Copy `from`'s function template-parameter list onto `to`, rebasing all
 * indices by `rebase_base` (used when cloning a member function template whose
 * parameters trail the enclosing class template's parameters). */
static void CopyFunctionTemplateParameters(TypeRecord* to, TypeRecord* from,
                                           int rebase_base) {
  if (to == NULL || from == NULL || !TypeIsFunction(to) ||
      !TypeIsFunction(from)) {
    return;
  }
  VectorDestructWithContents(&to->info.function.template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&to->info.function.template_parameters);
  for (size_t i = 0; i < from->info.function.template_parameters.length; i++) {
    TemplateParameter* original =
        from->info.function.template_parameters.value.p[i];
    if (original == NULL) {
      continue;
    }
    TemplateParameter* param =
        TemplateParameterCopy(original);
    RebaseTemplateParameterIndices(param->type, rebase_base);
    RebaseTemplateParameterIndices(param->default_type, rebase_base);
    if (param->default_template_parameter_index >= rebase_base) {
      param->default_template_parameter_index -= rebase_base;
    }
    if (param->index >= rebase_base) {
      param->index -= rebase_base;
    }
    VectorAppend(&to->info.function.template_parameters, param);
  }
}

static bool TemplateArgumentVectorContainsTemplateParameter(Vector* args);

static void DependentExpressionContainsParameterVisitor(ASTNode* node,
                                                       void* data,
                                                       int child_id,
                                                       VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL) {
    return;
  }
  if ((id->symbol->flags.is_template_parameter &&
       id->symbol->template_parameter_index >= 0) ||
      id->symbol->dependent_value_template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(id->symbol->type) ||
      TemplateArgumentVectorContainsTemplateParameter(id->template_arguments)) {
    *(bool*)data = true;
  }
}

static bool DependentExpressionContainsTemplateParameter(ASTNode* expr) {
  bool found = false;
  ASTNodeVisit(expr, DependentExpressionContainsParameterVisitor, 0, &found);
  return found;
}

/* True if a template argument is still dependent: it references a template
 * parameter directly, contains a dependent pack element, or names a dependent
 * type. */
static bool TemplateArgumentContainsTemplateParameter(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->kind == kTemplateParameterNonType &&
      arg->template_parameter_index >= 0) {
    return true;
  }
  if (arg->pack_arguments != NULL &&
      TemplateArgumentVectorContainsTemplateParameter(arg->pack_arguments)) {
    return true;
  }
  if (DependentExpressionContainsTemplateParameter(arg->dependent_expr)) {
    return true;
  }
  return TypeContainsTemplateParameter(arg->type);
}

/* True if any argument in the vector is still dependent (see above). */
static bool TemplateArgumentVectorContainsTemplateParameter(Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    if (TemplateArgumentContainsTemplateParameter(args->value.p[i])) {
      return true;
    }
  }
  return false;
}

/* True if `type` still mentions an unresolved template parameter anywhere: as a
 * bare parameter, an array bound, a template argument, or a function parameter
 * type. Used to decide whether a type is dependent. */
bool TypeContainsTemplateParameter(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsUnknown(t) && t->template_parameter_index >= 0) {
      return true;
    }
    if (t->dependent_member_name != NULL &&
        (t->template_parameter_index >= 0 || t->template_origin != NULL)) {
      return true;
    }
    if (t->declarator == kDeclArray &&
        t->info.array.template_parameter_index >= 0) {
      return true;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        if (TemplateArgumentContainsTemplateParameter(
                t->template_arguments->value.p[i])) {
          return true;
        }
      }
    }
    if (TypeIsFunction(t)) {
      if (TypeContainsTemplateParameter(t->next)) {
        return true;
      }
      for (size_t i = 0; i < t->info.function.prototype.length; i++) {
        Symbol* formal = t->info.function.prototype.value.p[i];
        if (formal != NULL && TypeContainsTemplateParameter(formal->type)) {
          return true;
        }
      }
    }
  }
  return false;
}

/* Build the mangled instantiation name for a template, e.g. `Foo<int,3>`.
 * Type args are encoded via their template-key string; non-type args by value
 * (or `$N<idx>` if still parameter-dependent); packs are bracketed `[...]`.
 * This string is the lookup key for caching/finding instantiations. */
static void AppendTemplateInstantiationName(String* name, Symbol* templ,
                                            Vector* args) {
  StringSet(name, templ->name.value);
  StringAppendChar(name, '<');
  for (size_t i = 0; i < args->length; i++) {
    if (i != 0) {
      StringAppend(name, ",");
    }
    String arg_name;
    StringInit(&arg_name, NULL);
    TemplateArgument* arg = args->value.p[i];
    if (arg->pack_arguments != NULL) {
      StringAppendChar(&arg_name, '[');
      for (size_t j = 0; j < arg->pack_arguments->length; j++) {
        if (j != 0) {
          StringAppend(&arg_name, ",");
        }
        String element_name;
        StringInit(&element_name, NULL);
        TemplateArgument* element = arg->pack_arguments->value.p[j];
        if (element->kind == kTemplateParameterType) {
          TypeRecordToTemplateKeyString(element->type, &element_name);
        } else if (element->template_parameter_index >= 0) {
          StringPrintf(&element_name, "$N%d", element->template_parameter_index);
        } else {
          char buffer[32];
          snprintf(buffer, sizeof(buffer), "%lld", element->int_value);
          StringAppend(&element_name, buffer);
        }
        StringAppendString(&arg_name, &element_name);
        StringDestruct(&element_name);
      }
      StringAppendChar(&arg_name, ']');
    } else if (arg->kind == kTemplateParameterType) {
      TypeRecordToTemplateKeyString(arg->type, &arg_name);
    } else {
      if (arg->template_parameter_index >= 0) {
        StringPrintf(&arg_name, "$N%d", arg->template_parameter_index);
      } else {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lld", arg->int_value);
        StringAppend(&arg_name, buffer);
      }
    }
    StringAppendString(name, &arg_name);
    StringDestruct(&arg_name);
  }
  StringAppendChar(name, '>');
}

/* Find an already-created instantiation tag named `name` for template `templ`,
 * or NULL. */
static Symbol* FindTemplateInstantiationTag(TypeParser* parser, Symbol* templ,
                                            String* name) {
  (void)parser;
  /* Look up the instantiation tag in exactly the template's own namespace.
   * Using a scope-walking lookup (SyntaxFindTag) here is wrong: two templates
   * with the same name in different namespaces (e.g. ::Foo<int> and
   * ns::Foo<int>) produce identical instantiation-name strings, so falling back
   * to enclosing/global scopes would alias the nested instantiation to a
   * previously created global one, corrupting its template_origin. This mirrors
   * how AddTemplateInstantiationTag inserts the tag. */
  if (templ->namespace_ != NULL &&
      templ->namespace_ != compiler->global_namespace) {
    return NamespaceFindTag(templ->namespace_, name);
  }
  return FindGlobalTag(name);
}

/* Register a freshly created instantiation tag in the template's own namespace
 * (temporarily switching the parser's tag scope so the tag is not added to some
 * unrelated local/current scope). Mirrors FindTemplateInstantiationTag. */
static bool AddTemplateInstantiationTag(TypeParser* parser, Symbol* templ,
                                        Symbol* tag) {
  LocalSymbolTable* saved_tag_stack = parser->syntax->local_tag_stack;
  Namespace* saved_namespace = parser->syntax->current_namespace;
  parser->syntax->local_tag_stack = NULL;
  parser->syntax->current_namespace =
      templ->namespace_ != NULL ? templ->namespace_ : compiler->global_namespace;
  bool added = SyntaxAddTag(parser->syntax, tag);
  parser->syntax->local_tag_stack = saved_tag_stack;
  parser->syntax->current_namespace = saved_namespace;
  return added;
}

/* Reject (with a diagnostic) class-template instantiations that use member
 * kinds the instantiation machinery does not yet handle: anonymous members,
 * bit-fields, and virtual/pure-virtual member functions. */
static bool ClassTemplateInstantiationMembersSupported(TypeParser* parser,
                                                       Struct* str) {
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (StructMemberIsNestedType(member)) {
      continue;
    }
    if (member->is_anon || StructMemberIsBitField(member)) {
      SyntaxError(parser->syntax,
                  "Class template instantiation is not supported yet");
      return false;
    }
    if (member->is_member_function &&
        (member->symbol == NULL || member->symbol->type == NULL ||
         !TypeIsFunction(member->symbol->type) ||
         member->symbol->type->info.function.is_virtual ||
         member->symbol->type->info.function.is_pure_virtual)) {
      SyntaxError(parser->syntax,
                  "Class template instantiation is not supported yet");
      return false;
    }
  }
  return true;
}

/* Build the concrete function type for a member function of an instantiated
 * class template: copy all of `from`'s function attributes (cv/ref/noexcept,
 * special-member kind, coroutine info, etc.), substitute the return type and
 * each parameter type against `args` (expanding parameter packs), re-add the
 * implicit `this` parameter against the instantiated `owner`, and renumber
 * argument slots. */
static TypeRecord* InstantiateMemberFunctionType(TypeParser* parser,
                                                 Struct* owner,
                                                 bool is_static_member,
                                                 TypeRecord* from,
                                                 Vector* args,
                                                 SourceLocation location) {
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.varargs = from->info.function.varargs;
  func->info.function.unknown_args = from->info.function.unknown_args;
  func->info.function.definition = false;
  func->info.function.is_inline = from->info.function.is_inline;
  func->info.function.is_constexpr = from->info.function.is_constexpr;
  func->info.function.is_consteval = from->info.function.is_consteval;
  func->info.function.is_constructor = from->info.function.is_constructor;
  func->info.function.is_destructor = from->info.function.is_destructor;
  func->info.function.is_const_member = from->info.function.is_const_member;
  func->info.function.ref_qualifier = from->info.function.ref_qualifier;
  func->info.function.is_explicit = from->info.function.is_explicit;
  func->info.function.is_explicit_conversion =
      from->info.function.is_explicit_conversion;
  func->info.function.explicit_condition = NULL;
  // A value-dependent `explicit(cond)` was deferred at parse time; substitute
  // the concrete template arguments and constant-fold it now so this
  // instantiation gets the correct explicit-ness (C++20 [dcl.fct.spec]).
  if (from->info.function.explicit_condition != NULL) {
    int64_t explicit_value = 0;
    if (TryFoldDependentTemplateArgument(
            parser, from->info.function.explicit_condition, args,
            &explicit_value)) {
      func->info.function.is_explicit = explicit_value != 0;
      func->info.function.is_explicit_conversion =
          from->info.function.is_explicit_conversion && explicit_value != 0;
    }
  }
  func->info.function.is_final = from->info.function.is_final;
  func->info.function.is_defaulted = from->info.function.is_defaulted;
  func->info.function.is_deleted = from->info.function.is_deleted;
  func->info.function.cxx_special_member_kind =
      from->info.function.cxx_special_member_kind;
  func->info.function.is_user_declared = from->info.function.is_user_declared;
  func->info.function.is_user_provided = from->info.function.is_user_provided;
  func->info.function.is_explicitly_defaulted =
      from->info.function.is_explicitly_defaulted;
  func->info.function.is_explicitly_deleted =
      from->info.function.is_explicitly_deleted;
  func->info.function.is_implicitly_declared =
      from->info.function.is_implicitly_declared;
  func->info.function.is_implicitly_deleted =
      from->info.function.is_implicitly_deleted;
  func->info.function.is_trivial_special_member =
      from->info.function.is_trivial_special_member;
  func->info.function.is_constexpr_eligible =
      from->info.function.is_constexpr_eligible;
  func->info.function.is_noexcept_eligible =
      from->info.function.is_noexcept_eligible;
  func->info.function.is_noexcept = from->info.function.is_noexcept;
  func->info.function.is_auto_return_deduced =
      from->info.function.is_auto_return_deduced;
  func->info.function.is_coroutine = from->info.function.is_coroutine;
  func->info.function.coroutine_promise_type =
      from->info.function.coroutine_promise_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_promise_type)
          : NULL;
  func->info.function.coroutine_frame_type =
      from->info.function.coroutine_frame_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_frame_type)
          : NULL;
  func->info.function.coroutine_suspend_count =
      from->info.function.coroutine_suspend_count;
  func->info.function.template_parameter_count =
      from->info.function.template_parameter_count;
  func->info.function.template_parameter_base = 0;
  int member_template_base = from->info.function.template_parameter_base;
  CopyFunctionTemplateParameters(func, from, member_template_base);
  // A member function template's own parameters are numbered at/after
  // `member_template_base`.  Only enclosing-template arguments (indices
  // below that base) may be substituted here; the member's own placeholders
  // must survive for later deduction.  When the member has already been
  // rebased to a standalone template (base == 0) and is rebuilt again —
  // e.g. a nested generic lambda whose closure is substituted while cloning
  // an outer generic-lambda body — the enclosing args would otherwise
  // consume the member's own `auto` placeholders at index 0.
  Vector enclosing_only_args;
  Vector* subst_args = args;
  bool use_enclosing_only = false;
  if (from->info.function.template_parameter_count > 0) {
    VectorInit(&enclosing_only_args);
    use_enclosing_only = true;
    for (size_t i = 0;
         args != NULL && (int)i < member_template_base && i < args->length;
         i++) {
      VectorAppend(&enclosing_only_args, args->value.p[i]);
    }
    subst_args = &enclosing_only_args;
  }
  TypeRecord* return_type =
      SubstituteTemplateParameters(parser, from->next, subst_args);
  RebaseTemplateParameterIndices(return_type, member_template_base);
  TypeRecordChain(func, return_type);

  if (is_static_member) {
    func->info.function.cxx_member_owner = owner;
  } else {
    TypeRecordAddCXXThisParameter(func, owner, location);
  }
  size_t first_formal =
      from->info.function.cxx_member_owner != NULL && !is_static_member ? 1 : 0;
  for (size_t i = first_formal; i < from->info.function.prototype.length; i++) {
    Symbol* formal = from->info.function.prototype.value.p[i];
    if (formal != NULL && formal->flags.is_parameter_pack) {
      AppendSubstitutedFormalParameter(
          parser, &func->info.function.prototype, formal, subst_args,
          member_template_base);
      continue;
    }
    TypeRecord* formal_type =
        SubstituteTemplateParameters(parser, formal->type, subst_args);
    RebaseTemplateParameterIndices(formal_type, member_template_base);
    Symbol* clone = NewSymbol(formal->name.value, formal_type, formal->storage);
    clone->flags = formal->flags;
    clone->flags.is_argument = true;
    clone->location = formal->location;
    clone->default_argument =
        ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
    VectorAppend(&func->info.function.prototype, clone);
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    formal->value.arg_number = (int32_t)i;
  }
  if (use_enclosing_only) {
    // Shallow: elements are borrowed from `args`, not owned here.
    VectorDestruct(&enclosing_only_args);
  }
  (void)parser;
  return func;
}

/* State threaded through the recursive clone of a template function body when
 * instantiating it. Carries the template arguments to substitute and maps from
 * the original local/parameter symbols to their freshly cloned counterparts.
 *   symbol_map      - original symbol -> cloned symbol (locals, parameters).
 *   pack_symbol_map - pack symbol -> Vector of per-element cloned symbols,
 *                     used when expanding a parameter pack inside the body.
 *   args            - the concrete template arguments for this instantiation.
 *   to_func         - the function type being produced (for `this`/member info).
 *   rebase_template_parameter_base - offset for renumbering nested template
 *                     parameter indices (member templates). */
typedef struct {
  Map symbol_map;
  Map pack_symbol_map;
  TypeParser* parser;
  Vector* args;
  TypeRecord* to_func;
  int rebase_template_parameter_base;
  // The generic template's owning class and the instantiated owning class, used
  // to rewrite uses of the injected-class-name (e.g. a functional-cast
  // `ClassName(args)` constructing a temporary of the current specialization)
  // from the primary template to this instantiation.  NULL for non-member
  // function templates.
  struct Struct* from_owner;
  struct Struct* to_owner;
} TemplateFunctionBodyClone;

static void DeleteMappedVector(MapKeyValue* kv) {
  VectorDelete(kv->value.p);
}

static ASTNode* CloneTemplateFunctionBodyNode(ASTNode* node, void* data);
static Vector* SubstituteTemplateArgumentVector(TypeParser* parser,
                                                Vector* template_args,
                                                Vector* args,
                                                int rebase_base);

/* Re-evaluate a value-dependent non-type template-argument expression (stored
 * unevaluated at parse time, e.g. `!is_integral<It>::value`) against the concrete
 * template arguments `args`.  The expression is cloned with template-parameter
 * substitution (which resolves dependent qualified names like `X<T>::value`
 * against the substituted scope) and then constant-folded.  On success writes the
 * folded value to *out and returns true; returns false when the expression is
 * still value-dependent (some referenced parameter is not yet concrete). */
/* Clone a value-dependent template-argument expression, substituting the
 * concrete actuals `args` into any template parameters they supply.  Parameters
 * not covered by `args` (e.g. a member function template's own parameter while
 * only its enclosing class's arguments are known) are left symbolic so a later,
 * more-concrete substitution can finish them.  The returned node is a fresh,
 * unanalyzed AST (arena-owned identifiers); the caller owns it. */
static ASTNode* CloneDependentExpressionWithArgs(TypeParser* parser,
                                                 ASTNode* expr, Vector* args) {
  if (expr == NULL || args == NULL) {
    return NULL;
  }
  TemplateFunctionBodyClone clone;
  MapInitForPointerKeys(&clone.symbol_map);
  MapInitForPointerKeys(&clone.pack_symbol_map);
  clone.parser = parser;
  clone.args = args;
  clone.to_func = NULL;
  clone.rebase_template_parameter_base = 0;
  clone.from_owner = NULL;
  clone.to_owner = NULL;
  ASTNode* cloned =
      ASTNodeClone(expr, CloneTemplateFunctionBodyNode, &clone, NULL);
  MapDestructWithContents(&clone.pack_symbol_map, DeleteMappedVector);
  MapDestruct(&clone.symbol_map);
  return cloned;
}

static bool TryFoldDependentTemplateArgument(TypeParser* parser, ASTNode* expr,
                                             Vector* args, int64_t* out) {
  if (expr == NULL || args == NULL) {
    return false;
  }
  ASTNode* cloned = CloneDependentExpressionWithArgs(parser, expr, args);
  if (cloned == NULL) {
    return false;
  }
  // A dependent qualified name that survived cloning (its scope is still
  // dependent) must not be diagnosed here: this is a speculative fold, and an
  // unresolved name simply means the value stays dependent for now.
  DiagnosticSuppressBegin();
  cloned = AnalyzeExpression(cloned);
  bool ok = EvaluateIntegerExpression(cloned, out);
  DiagnosticSuppressEnd();
  ASTNodeDelete(cloned);
  return ok;
}

/* Instantiate a C++ variable template's initializer against concrete template
 * arguments `args` (a Vector of TemplateArgument*, positional by parameter
 * index) and constant-fold it to an integer.  Returns true and writes the value
 * to *out on success.  Used for constant-valued variable templates such as
 * `variant_size_v<T>`. */
bool TypeInstantiateVariableTemplateConstant(Syntax* syntax,
                                             Symbol* var_template, Vector* args,
                                             int64_t* out) {
  if (var_template == NULL || var_template->variable_template == NULL ||
      var_template->variable_template->initializer == NULL) {
    return false;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  bool ok = TryFoldDependentTemplateArgument(
      &parser, var_template->variable_template->initializer, args, out);
  TypeParserDestruct(&parser);
  return ok;
}

/* Instantiate the *type* of a C++ variable template against concrete template
 * arguments `args`, e.g. `in_place_index<1>` -> `in_place_index_t<1>`.  Used for
 * variable templates whose value is a class-type object (a tag such as
 * `std::in_place_index`) rather than a folded constant.  Returns a freshly
 * allocated concrete type, or NULL if the template has no type. */
TypeRecord* TypeInstantiateVariableTemplateType(Syntax* syntax,
                                                Symbol* var_template,
                                                Vector* args) {
  if (var_template == NULL || var_template->variable_template == NULL ||
      var_template->type == NULL) {
    return NULL;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* concrete =
      SubstituteTemplateParameters(&parser, var_template->type, args);
  TypeParserDestruct(&parser);
  return concrete;
}

ASTNode* TypeSubstituteTemplateExpression(Syntax* syntax, ASTNode* expr,
                                          Vector* args,
                                          SourceLocation location) {
  if (syntax == NULL || expr == NULL) {
    return NULL;
  }
  if (args == NULL) {
    return ASTNodeClone(expr, IdentityCloneNode, NULL, NULL);
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TemplateFunctionBodyClone clone;
  MapInitForPointerKeys(&clone.symbol_map);
  MapInitForPointerKeys(&clone.pack_symbol_map);
  clone.parser = &parser;
  clone.args = args;
  clone.to_func = NULL;
  clone.rebase_template_parameter_base = 0;
  clone.from_owner = NULL;
  clone.to_owner = NULL;
  ASTNode* cloned = ASTNodeClone(expr, CloneTemplateFunctionBodyNode,
                                 &clone, NULL);
  MapDestructWithContents(&clone.pack_symbol_map, DeleteMappedVector);
  MapDestruct(&clone.symbol_map);
  TypeParserDestruct(&parser);
  if (cloned != NULL) {
    cloned->location = location;
  }
  return cloned;
}

TypeRecord* TypeSubstituteTemplateType(Syntax* syntax, TypeRecord* type,
                                       Vector* args) {
  if (type == NULL) {
    return NULL;
  }
  if (syntax == NULL || args == NULL) {
    return TypeRecordCopy(type);
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* result = SubstituteTemplateParameters(&parser, type, args);
  TypeParserDestruct(&parser);
  return result;
}

Vector* TypeSubstituteTemplateArgumentVector(Syntax* syntax,
                                             Vector* template_args,
                                             Vector* args) {
  if (template_args == NULL) {
    return NULL;
  }
  if (args == NULL) {
    return TemplateArgumentVectorCopy(template_args);
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  Vector* result =
      SubstituteTemplateArgumentVector(&parser, template_args, args, 0);
  TypeParserDestruct(&parser);
  return result;
}

/* Return the constructor name (== the class tag name) for a struct type, or
 * NULL if `type` is not a named C++ class. */
static const char* CXXConstructorNameForRecord(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  return type->info.struct_info->tag_name->value;
}

/* True if `type` is a class type that declares a constructor member (named
 * after its tag).  Used to decide whether value-initialization of a resolved
 * template-parameter type must run a default constructor (class with a
 * constructor) or zero-initialize the storage (scalar or aggregate class). */
static bool CXXRecordHasConstructorMember(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return false;
  }
  StructMember* ctor = FindStructMember(type->info.struct_info,
                                        type->info.struct_info->tag_name);
  return ctor != NULL && ctor->is_member_function && ctor->symbol != NULL &&
         TypeIsFunction(ctor->symbol->type) &&
         ctor->symbol->type->info.function.is_constructor;
}

/* When a cloned member access (`obj.field`/`obj->field`) has no resolved type
 * yet, infer the member's type by looking the field name up in the enclosing
 * instantiated class (the function's member owner). */
static TypeRecord* InferClonedMemberReceiverType(TemplateFunctionBodyClone* clone,
                                                ASTNode* receiver) {
  if (clone == NULL || clone->to_func == NULL ||
      clone->to_func->info.function.cxx_member_owner == NULL ||
      receiver == NULL ||
      (receiver->op != AST_OP(dot) && receiver->op != AST_OP(arrow))) {
    return NULL;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)receiver;
  if (member_access->right == NULL ||
      member_access->right->op != AST_OP(string)) {
    return NULL;
  }
  ConstantASTNode* member_name = (ConstantASTNode*)member_access->right;
  if (member_name->value.string == NULL) {
    return NULL;
  }
  StructMember* member = FindStructMember(
      clone->to_func->info.function.cxx_member_owner, member_name->value.string);
  if (member == NULL || member->symbol == NULL) {
    return NULL;
  }
  return member->symbol->type;
}

/* Fix up a cloned `receiver.member(...)` call where `member` still carries the
 * generic template's constructor name. After substitution the receiver's class
 * is concrete, so rewrite the member name to that class's constructor name and
 * rebind it to the instantiated member. */
static void RewriteClonedConstructorMemberCall(TemplateFunctionBodyClone* clone,
                                               ASTNode* node) {
  if (node->op != AST_OP(call)) {
    return;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL ||
      (call->left->op != AST_OP(dot) && call->left->op != AST_OP(arrow))) {
    return;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)call->left;
  if (member_access->left == NULL || member_access->right == NULL ||
      member_access->right->op != AST_OP(string)) {
    return;
  }
  TypeRecord* receiver_type = member_access->left->type;
  if (receiver_type == NULL) {
    receiver_type = InferClonedMemberReceiverType(clone, member_access->left);
  }
  const char* constructor_name = CXXConstructorNameForRecord(receiver_type);
  if (constructor_name == NULL) {
    return;
  }
  ConstantASTNode* member_name = (ConstantASTNode*)member_access->right;
  if (member_name->value.string == NULL ||
      StringEqual(member_name->value.string, constructor_name)) {
    return;
  }
  if (TypeIsStructOrUnion(receiver_type) &&
      receiver_type->info.struct_info != NULL) {
    StructMember* existing = FindStructMember(receiver_type->info.struct_info,
                                              member_name->value.string);
    if (existing != NULL) {
      return;
    }
  }
  if (receiver_type->template_origin == NULL ||
      strcmp(member_name->value.string->value,
             receiver_type->template_origin->name.value) != 0) {
    return;
  }
  StringSet(member_name->value.string, constructor_name);
}

/* Visitor: rewrite an identifier referencing an original symbol to point at
 * its cloned replacement (from `symbol_map`) and refresh its node type. */
static void RewriteTemplateBodyIdentifierVisitor(ASTNode* node, void* data,
                                                 int child_id,
                                                 VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node->op != AST_OP(identifier)) {
    return;
  }
  Map* symbol_map = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  Symbol* replacement = MapFindPointerKey(symbol_map, id->symbol);
  if (replacement != NULL) {
    id->symbol = replacement;
    ASTNodeSetType(node, replacement->type);
  }
}

/* Rewrite all identifiers in `node` to their cloned symbols via `symbol_map`. */
static void RewriteTemplateBodyIdentifiers(ASTNode* node, Map* symbol_map) {
  ASTNodeVisit(node, RewriteTemplateBodyIdentifierVisitor, 0, symbol_map);
}

/* Clone the symbol declared by a local `vardecl` in a template body: substitute
 * its type against the instantiation args, create a replacement symbol, record
 * the original->clone mapping, and point the declaration (and its initializer's
 * identifiers) at the new symbol. */
/* The owning C++ class of a member function type, whether static (recorded in
 * cxx_member_owner) or non-static (reached through the implicit `this`
 * parameter's pointee).  Returns NULL for non-member functions. */
static struct Struct* CloneFunctionMemberOwner(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return NULL;
  }
  FunctionInfo* info = &func->info.function;
  if (info->cxx_member_owner != NULL) {
    return info->cxx_member_owner;
  }
  if (info->prototype.length > 0) {
    Symbol* this_sym = info->prototype.value.p[0];
    if (this_sym != NULL && this_sym->type != NULL &&
        StringEqual(&this_sym->name, "this") && this_sym->type->next != NULL &&
        TypeIsStructOrUnion(this_sym->type->next) &&
        this_sym->type->next->info.struct_info != NULL) {
      return this_sym->type->next->info.struct_info;
    }
  }
  return NULL;
}

static void AddOwnerMemberSymbolMappings(TemplateFunctionBodyClone* clone) {
  if (clone == NULL || clone->from_owner == NULL || clone->to_owner == NULL ||
      clone->from_owner == clone->to_owner) {
    return;
  }
  for (size_t i = 0; i < clone->from_owner->members.length; i++) {
    StructMember* from_member = clone->from_owner->members.value.p[i];
    if (from_member == NULL || from_member->symbol == NULL ||
        from_member->is_member_function) {
      continue;
    }
    StructMember* to_member =
        FindStructMemberByName(clone->to_owner, from_member->symbol->name.value);
    if (to_member == NULL || to_member->symbol == NULL) {
      continue;
    }
    MapKeyValue kv;
    kv.key.p = from_member->symbol;
    kv.value.p = to_member->symbol;
    MapInsert(&clone->symbol_map, kv);
  }
}

// True if any node in `type`'s declarator chain (the type itself, or a
// pointed-to / referenced / element type) is the class `str`.  Used to detect
// uses of the injected-class-name -- e.g. `static_cast<ClassName&&>(...)` or a
// local `ClassName tmp(...)` -- whose type names the *generic* primary-template
// struct directly and so contains no template *parameter*; such types must
// still be remapped to the current instantiation during member-body cloning,
// otherwise a cast to the generic self-type forces a spurious temporary built
// with the generic (unemitted) copy constructor.
static bool TypeChainReferencesStruct(TypeRecord* type, struct Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->info.struct_info == str) {
      return true;
    }
  }
  return false;
}

static void CloneTemplateLocalDeclarationSymbol(TemplateFunctionBodyClone* clone,
                                                ASTNode* node) {
  if (node->op != AST_OP(vardecl)) {
    return;
  }
  VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
  Symbol* old_symbol = decl->symbol;
  if (old_symbol == NULL || old_symbol->flags.is_argument) {
    return;
  }
  Symbol* existing = MapFindPointerKey(&clone->symbol_map, old_symbol);
  if (existing != NULL) {
    decl->symbol = existing;
    RewriteTemplateBodyIdentifiers(decl->initializer, &clone->symbol_map);
    return;
  }

  TypeRecord* type =
      SubstituteTemplateParameters(clone->parser, old_symbol->type, clone->args);
  RebaseTemplateParameterIndices(type, clone->rebase_template_parameter_base);
  Symbol* replacement =
      NewSymbol(old_symbol->name.value, type, old_symbol->storage);
  replacement->flags = old_symbol->flags;
  replacement->location = old_symbol->location;
  replacement->alignment = old_symbol->alignment;
  replacement->namespace_ = old_symbol->namespace_;
  replacement->value = old_symbol->value;

  MapKeyValue kv;
  kv.key.p = old_symbol;
  kv.value.p = replacement;
  MapInsert(&clone->symbol_map, kv);
  decl->symbol = replacement;
  ASTNodeSetType(node, replacement->type);
  RewriteTemplateBodyIdentifiers(decl->initializer, &clone->symbol_map);
}

/* Clone a compiler temporary whose type was template-dependent: substitute its
 * type and register the original->clone mapping (memoized in symbol_map). */
static Symbol* CloneTemplateDependentTemporarySymbol(
    TemplateFunctionBodyClone* clone, Symbol* old_symbol) {
  if (old_symbol == NULL || !old_symbol->flags.is_temp ||
      old_symbol->type == NULL) {
    return NULL;
  }
  Symbol* existing = MapFindPointerKey(&clone->symbol_map, old_symbol);
  if (existing != NULL) {
    return existing;
  }
  TypeRecord* type =
      SubstituteTemplateParameters(clone->parser, old_symbol->type, clone->args);
  RebaseTemplateParameterIndices(type, clone->rebase_template_parameter_base);
  Symbol* replacement =
      NewSymbol(old_symbol->name.value, type, old_symbol->storage);
  replacement->flags = old_symbol->flags;
  replacement->location = old_symbol->location;
  replacement->alignment = old_symbol->alignment;
  replacement->namespace_ = old_symbol->namespace_;
  replacement->value = old_symbol->value;

  MapKeyValue kv;
  kv.key.p = old_symbol;
  kv.value.p = replacement;
  MapInsert(&clone->symbol_map, kv);
  return replacement;
}

/* Like SubstituteTemplateArgumentVectorForTypes but additionally rebases the
 * parameter indices of the produced arguments by `rebase_base` (for nested /
 * member template instantiations). */
static Vector* SubstituteTemplateArgumentVector(TypeParser* parser,
                                                Vector* template_args,
                                                Vector* args,
                                                int rebase_base) {
  if (template_args == NULL) {
    return NULL;
  }
  Vector* concrete_args = NewVector();
  for (size_t i = 0; i < template_args->length; i++) {
    size_t start = concrete_args->length;
    AppendSubstitutedTemplateArgument(parser, concrete_args,
                                      template_args->value.p[i], args);
    for (size_t j = start; j < concrete_args->length; j++) {
      RebaseTemplateArgumentParameterIndices(concrete_args->value.p[j],
                                             rebase_base);
    }
  }
  return concrete_args;
}

/* True if `node` is a pack-expansion of a single parameter-pack identifier,
 * i.e. the `xs` in `xs...`. */
static bool IsIdentifierPackExpansion(ASTNode* node) {
  if (node == NULL || (node->flags & kASTPackExpansion) == 0 ||
      node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  return id->symbol != NULL && id->symbol->flags.is_parameter_pack;
}

typedef struct {
  TemplateFunctionBodyClone* clone;
  Symbol* symbol;
  bool multiple_packs;
} PackExpansionExpressionSearch;

static void FindPackExpansionExpressionSymbol(ASTNode* node, void* data,
                                              int child_id,
                                              VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  PackExpansionExpressionSearch* search = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL ||
      MapFindPointerKey(&search->clone->pack_symbol_map, id->symbol) == NULL) {
    return;
  }
  if (search->symbol != NULL && search->symbol != id->symbol) {
    search->multiple_packs = true;
    return;
  }
  search->symbol = id->symbol;
}

/* Find the single parameter-pack symbol referenced inside a pack-expansion
 * pattern `node`. Sets `*multiple_packs` if more than one distinct pack appears
 * (which the simple expansion path cannot handle). */
static Symbol* PackExpansionExpressionSymbol(TemplateFunctionBodyClone* clone,
                                             ASTNode* node,
                                             bool* multiple_packs) {
  PackExpansionExpressionSearch search = {0};
  search.clone = clone;
  ASTNodeVisit(node, FindPackExpansionExpressionSymbol, 0, &search);
  if (multiple_packs != NULL) {
    *multiple_packs = search.multiple_packs;
  }
  return search.symbol;
}

typedef struct {
  TemplateFunctionBodyClone* clone;
  bool found;
} UnresolvedPackSearch;

static void FindUnresolvedPackIdentifier(ASTNode* node, void* data,
                                         int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  UnresolvedPackSearch* search = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol != NULL && id->symbol->flags.is_parameter_pack &&
      MapFindPointerKey(&search->clone->pack_symbol_map, id->symbol) == NULL) {
    search->found = true;
  }
}

/* True if `node` (a pack-expansion pattern) still references a parameter pack
 * that this clone does not resolve: its pack symbol is not in the clone's
 * pack_symbol_map, so it belongs to a not-yet-instantiated nested/member
 * template (e.g. a variadic member template's pack while its enclosing class
 * template is being instantiated). Such a pattern must retain its
 * kASTPackExpansion marker for the later member-template instantiation. */
static bool ClonePatternReferencesUnresolvedPack(TemplateFunctionBodyClone* clone,
                                                 ASTNode* node) {
  UnresolvedPackSearch search;
  search.clone = clone;
  search.found = false;
  ASTNodeVisit(node, FindUnresolvedPackIdentifier, 0, &search);
  return search.found;
}

/* If `node` is (optionally cast/unary-wrapped) member access
 * `recv.name`/`recv->name` naming a captured field, return that access node;
 * otherwise NULL. Used to expand lambda capture packs stored as synthetic
 * struct members.  Patterns like `static_cast<T&&>(xs)...` must peel the cast
 * so the captured pack field can still be found and expanded. */
static ASTNode* LambdaCapturePackMemberAccess(ASTNode* node) {
  while (node != NULL) {
    if (node->op == AST_OP(cast)) {
      node = ((CastASTNode*)node)->expr;
      continue;
    }
    if (node->op == AST_OP(contents) || node->op == AST_OP(address) ||
        node->op == AST_OP(plus) || node->op == AST_OP(minus) ||
        node->op == AST_OP(not)) {
      node = ((UnaryASTNode*)node)->sub;
      continue;
    }
    break;
  }
  if (node == NULL || (node->op != AST_OP(arrow) &&
                       node->op != AST_OP(dot))) {
    return NULL;
  }
  BinaryASTNode* access = (BinaryASTNode*)node;
  if (access->right == NULL ||
      (access->right->op != AST_OP(string) &&
       access->right->op != AST_OP(structmember))) {
    return NULL;
  }
  return node;
}

/* Return the field name of a lambda-capture-pack member access, or NULL. */
static const char* LambdaCapturePackMemberName(ASTNode* node) {
  ASTNode* access_node = LambdaCapturePackMemberAccess(node);
  if (access_node == NULL) {
    return NULL;
  }
  BinaryASTNode* access = (BinaryASTNode*)access_node;
  if (access->right->op == AST_OP(string)) {
    ConstantASTNode* name = (ConstantASTNode*)access->right;
    return name->value.string != NULL ? name->value.string->value : NULL;
  }
  StructMemberASTNode* member = (StructMemberASTNode*)access->right;
  return member->member != NULL && member->member->symbol != NULL
             ? member->member->symbol->name.value
             : NULL;
}

/* Return the closure struct on the receiver side of a lambda-capture-pack
 * member access (looking through a pointer), or NULL.  When the access has not
 * yet been typed (common while cloning a body before re-analysis), fall back to
 * `fallback_owner` -- typically the call operator's cxx_member_owner, i.e. the
 * closure whose pack field was expanded into synthetic `$packN` elements. */
static Struct* LambdaCapturePackReceiverStruct(ASTNode* node,
                                               Struct* fallback_owner) {
  ASTNode* access_node = LambdaCapturePackMemberAccess(node);
  if (access_node == NULL) {
    return NULL;
  }
  BinaryASTNode* access = (BinaryASTNode*)access_node;
  TypeRecord* receiver_type = access->left != NULL ? access->left->type : NULL;
  if (receiver_type != NULL && TypeIsPointer(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (receiver_type != NULL && TypeIsStructOrUnion(receiver_type)) {
    return receiver_type->info.struct_info;
  }
  if (access->left != NULL && access->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)access->left;
    if (id->symbol != NULL && id->symbol->type != NULL) {
      TypeRecord* sym_type = id->symbol->type;
      if (TypeIsPointer(sym_type)) {
        sym_type = sym_type->next;
      }
      if (sym_type != NULL && TypeIsStructOrUnion(sym_type)) {
        return sym_type->info.struct_info;
      }
    }
  }
  return fallback_owner;
}

/* Return the synthetic per-element fields (`base$pack0`, `base$pack1`, ...) of
 * a captured pack member, in declaration order, for expanding `field...`. */
static Vector* LambdaCapturePackFieldReplacements(ASTNode* node,
                                                  Struct* fallback_owner) {
  const char* base_name = LambdaCapturePackMemberName(node);
  Struct* receiver = LambdaCapturePackReceiverStruct(node, fallback_owner);
  if (base_name == NULL || receiver == NULL) {
    return NULL;
  }
  Vector* replacements = NewVector();
  for (size_t i = 0; i < receiver->members.length; i++) {
    StructMember* member = receiver->members.value.p[i];
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    if (LambdaCapturePackElementMatches(member->symbol->name.value,
                                        base_name)) {
      VectorAppend(replacements, member);
    }
  }
  return replacements;
}

/* Determine how many elements a captured pack field expands to, from the
 * length of the corresponding template argument pack. */
static bool LambdaCapturePackFieldArgumentLength(ASTNode* node, Vector* args,
                                                 size_t* length,
                                                 Struct* fallback_owner) {
  const char* base_name = LambdaCapturePackMemberName(node);
  Struct* receiver = LambdaCapturePackReceiverStruct(node, fallback_owner);
  if (base_name == NULL || receiver == NULL || length == NULL) {
    return false;
  }
  String name;
  StringInit(&name, base_name);
  StructMember* member = FindStructMember(receiver, &name);
  StringDestruct(&name);
  if (member == NULL || member->symbol == NULL ||
      !member->symbol->flags.is_parameter_pack) {
    return false;
  }
  int pack_index = FirstTemplateParameterIndexInType(member->symbol->type);
  if (pack_index < 0 || args == NULL || (size_t)pack_index >= args->length) {
    return false;
  }
  TemplateArgument* pack = args->value.p[pack_index];
  if (pack == NULL || pack->pack_arguments == NULL) {
    return false;
  }
  *length = pack->pack_arguments->length;
  return true;
}

/* Parameters for replacing, within one expanded pack element, references to a
 * pack symbol `from` with the element-specific symbol `to` (or pack-mapped
 * symbol at `element_index`). */
typedef struct {
  TemplateFunctionBodyClone* clone;
  Symbol* from;
  Symbol* to;
  size_t element_index;
} ReplacePackIdentifierData;

static void InstantiateClonedFunctionTemplateCall(
    TemplateFunctionBodyClone* clone, ASTNode* node);
static ASTNode* ReanalyzeClonedDependentFunctorCall(
    ASTNode* node, void* data, ASTNodeTransformAction* action);
static bool ASTNodeWithinPackExpansion(ASTNode* node);
static bool ASTNodeWithinUnresolvedPackExpansion(TemplateFunctionBodyClone* clone,
                                                 ASTNode* node);
static bool RebindClonedConstructorCall(VectorASTNode* call,
                                        Vector* overload_snapshots);
static void RestoreSymbolOverloadLinks(Vector* snapshots);

static void InstantiateClonedFunctionTemplateCallVisitor(ASTNode* node,
                                                        void* data,
                                                        int child_id,
                                                        VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  InstantiateClonedFunctionTemplateCall(data, node);
}

/* Visitor that materializes one element of a pack expansion in a cloned body:
 * clears the pack-expansion flag, substitutes any per-identifier template
 * arguments for this element, rewrites pack-symbol references to the element's
 * symbol, and substitutes pack-dependent cast types. Invoked once per element
 * index while expanding `pattern...`. */
static void ReplacePackIdentifierVisitor(ASTNode* node, void* data,
                                         int child_id, VisitorMode mode) {
  (void)child_id;
  if (node != NULL) {
    node->flags &= ~kASTPackExpansion;
  }
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  ReplacePackIdentifierData* replace = data;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (replace->clone != NULL && id->template_arguments != NULL) {
      bool substituted_template_args = false;
      int pack_index = -1;
      size_t pack_length = 0;
      Vector* template_args_for_substitution = id->template_arguments;
      Vector* rebased_template_args = NULL;
      int substitution_rebase_base =
          replace->clone->rebase_template_parameter_base;
      for (size_t i = 0; i < id->template_arguments->length; i++) {
        if (FindPackExpansionInTemplateArgument(
                id->template_arguments->value.p[i], replace->clone->args,
                &pack_index, &pack_length)) {
          break;
        }
      }
      if (pack_index < 0 &&
          replace->clone->rebase_template_parameter_base > 0) {
        rebased_template_args =
            TemplateArgumentVectorCopy(id->template_arguments);
        for (size_t i = 0; i < rebased_template_args->length; i++) {
          RebaseTemplateArgumentParameterIndices(
              rebased_template_args->value.p[i],
              replace->clone->rebase_template_parameter_base);
        }
        for (size_t i = 0; i < rebased_template_args->length; i++) {
          if (FindPackExpansionInTemplateArgument(
                  rebased_template_args->value.p[i], replace->clone->args,
                  &pack_index, &pack_length)) {
            template_args_for_substitution = rebased_template_args;
            substitution_rebase_base = 0;
            break;
          }
        }
      }
      if (pack_index >= 0 && (size_t)pack_index < replace->clone->args->length) {
        TemplateArgument* pack = replace->clone->args->value.p[pack_index];
        if (pack != NULL && pack->pack_arguments != NULL &&
            replace->element_index < pack->pack_arguments->length) {
          Vector* element_args = TemplateArgumentVectorCopyWithPackElement(
              replace->clone->args, pack_index,
              pack->pack_arguments->value.p[replace->element_index]);
          Vector* concrete_args = SubstituteTemplateArgumentVector(
              replace->clone->parser, template_args_for_substitution,
              element_args, substitution_rebase_base);
          VectorDeleteWithContents(
              id->template_arguments,
              (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
          id->template_arguments = concrete_args;
          if (id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
              id->symbol->type->info.function.template_origin != NULL) {
            id->symbol = id->symbol->type->info.function.template_origin;
          }
          VectorDeleteWithContents(
              element_args, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
          substituted_template_args = true;
        }
      }
      if (rebased_template_args != NULL) {
        VectorDeleteWithContents(
            rebased_template_args,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      if (!substituted_template_args) {
        Vector* concrete_args = SubstituteTemplateArgumentVector(
            replace->clone->parser, id->template_arguments,
            replace->clone->args,
            replace->clone->rebase_template_parameter_base);
        VectorDeleteWithContents(
            id->template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        id->template_arguments = concrete_args;
      }
    }
    if (id->symbol == replace->from) {
      id->symbol = replace->to;
      ASTNodeSetType(node, TypeIsReference(replace->to->type)
                               ? replace->to->type->next
                               : replace->to->type);
      node->value_category = kValueCategoryLvalue;
    } else if (replace->clone != NULL && id->symbol != NULL) {
      Vector* replacements = MapFindPointerKey(&replace->clone->pack_symbol_map,
                                               id->symbol);
      if (replacements != NULL &&
          replace->element_index < replacements->length) {
        Symbol* replacement = replacements->value.p[replace->element_index];
        if (replacement != NULL) {
          id->symbol = replacement;
          ASTNodeSetType(node, TypeIsReference(replacement->type)
                                   ? replacement->type->next
                                   : replacement->type);
          node->value_category = kValueCategoryLvalue;
        }
      }
    }
  }
  if (node->op != AST_OP(cast) || replace->clone == NULL) {
    return;
  }
  CastASTNode* cast = (CastASTNode*)node;
  if (!TypeContainsTemplateParameter(cast->cast_type) &&
      !(replace->clone->from_owner != NULL &&
        replace->clone->to_owner != NULL &&
        replace->clone->from_owner != replace->clone->to_owner &&
        TypeChainReferencesStruct(cast->cast_type,
                                  replace->clone->from_owner))) {
    return;
  }
  int pack_index = -1;
  size_t pack_length = 0;
  TypeRecord* cast_type = NULL;
  if (FindPackExpansionInType(cast->cast_type, replace->clone->args,
                              &pack_index, &pack_length)) {
    if (pack_length > 0 && replace->element_index >= pack_length) {
      SyntaxError(replace->clone->parser->syntax,
                  "pack expansion argument packs have different lengths");
      return;
    }
    cast_type = SubstituteTemplateParametersForPackElement(
        replace->clone->parser, cast->cast_type, replace->clone->args,
        pack_index, replace->element_index);
  } else {
    cast_type = SubstituteTemplateParameters(replace->clone->parser,
                                             cast->cast_type,
                                             replace->clone->args);
  }
  RebaseTemplateParameterIndices(
      cast_type, replace->clone->rebase_template_parameter_base);
  TypeRecordCalculateSize(cast_type);
  TypeRecordDelete(cast->cast_type);
  cast->cast_type = cast_type;
  if (TypeIsReference(cast_type)) {
    ASTNodeSetType(node, cast_type->next);
    node->value_category =
        cast_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    ASTNodeSetType(node, cast_type);
  }
}

/* Visitor for the special case of a pack used in a non-expansion context where
 * exactly one element is expected (e.g. a single-element pack): substitute the
 * identifier's template arguments and bind to the concrete instantiation. */
static void ReplaceSingleElementPackIdentifierVisitor(ASTNode* node, void* data,
                                                      int child_id,
                                                      VisitorMode mode) {
  (void)child_id;
  TemplateFunctionBodyClone* clone = data;
  if (mode == kVisitPostChildren && node != NULL) {
    // Drop the pack-expansion marker only once the pack is resolved in this
    // clone.  A pattern still referencing a pack that this clone does not
    // resolve belongs to a not-yet-instantiated nested/member template (e.g. a
    // variadic member template of a class template during class instantiation)
    // and must keep its marker so the later member-template instantiation can
    // expand it.
    if ((node->flags & kASTPackExpansion) == 0 ||
        !ClonePatternReferencesUnresolvedPack(clone, node)) {
      node->flags &= ~kASTPackExpansion;
    }
    return;
  }
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->template_arguments != NULL &&
      !ASTNodeWithinUnresolvedPackExpansion(clone, node)) {
    int pack_index = -1;
    size_t pack_length = 0;
    for (size_t i = 0; i < id->template_arguments->length; i++) {
      if (FindPackExpansionInTemplateArgument(
              id->template_arguments->value.p[i], clone->args, &pack_index,
              &pack_length)) {
        break;
      }
    }
    if (pack_index < 0 && id->template_arguments->length == 1 &&
        id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
        (id->symbol->flags.is_template ||
         id->symbol->type->info.function.template_origin != NULL)) {
      for (size_t i = 0; i < clone->args->length; i++) {
        TemplateArgument* pack = clone->args->value.p[i];
        if (pack != NULL && pack->pack_arguments != NULL &&
            pack->pack_arguments->length == 1) {
          pack_index = (int)i;
          break;
        }
      }
    }
    if (pack_index >= 0 && (size_t)pack_index < clone->args->length) {
      Vector* element_args = NULL;
      TemplateArgument* pack = clone->args->value.p[pack_index];
      if (pack != NULL && pack->pack_arguments != NULL &&
          pack->pack_arguments->length == 1) {
        element_args = TemplateArgumentVectorCopyWithPackElement(
            clone->args, pack_index, pack->pack_arguments->value.p[0]);
      }
      Vector* concrete_args = NULL;
      bool use_single_pack_element =
          id->template_arguments->length == 1 && pack != NULL &&
          pack->pack_arguments != NULL && pack->pack_arguments->length == 1 &&
          id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
          (id->symbol->flags.is_template ||
           id->symbol->type->info.function.template_origin != NULL);
      if (use_single_pack_element) {
        concrete_args = NewVector();
        TemplateArgument* element = pack->pack_arguments->value.p[0];
        if (element != NULL && element->pack_arguments != NULL &&
            element->pack_arguments->length == 1) {
          element = element->pack_arguments->value.p[0];
        }
        VectorAppend(concrete_args, TemplateArgumentCopy(element));
      } else {
        Vector* substitution_args =
            element_args != NULL ? element_args : clone->args;
        concrete_args = SubstituteTemplateArgumentVector(
            clone->parser, id->template_arguments, substitution_args,
            clone->rebase_template_parameter_base);
      }
      VectorDeleteWithContents(id->template_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      id->template_arguments = concrete_args;
      if (id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
          id->symbol->type->info.function.template_origin != NULL) {
        id->symbol = id->symbol->type->info.function.template_origin;
      }
      if (node->parent != NULL && node->parent->op == AST_OP(call)) {
        VectorASTNode* parent_call = (VectorASTNode*)node->parent;
        if (parent_call->left == node) {
          node->parent->flags |= kASTDependentFunctorCall;
          node->parent->flags &= ~kASTAnalyzed;
        }
      }
      if (element_args != NULL) {
        VectorDeleteWithContents(
            element_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
    }
  }
  Vector* replacements =
      id->symbol != NULL ? MapFindPointerKey(&clone->pack_symbol_map,
                                             id->symbol)
                         : NULL;
  if (replacements != NULL && replacements->length == 1) {
    Symbol* replacement = replacements->value.p[0];
    if (replacement != NULL) {
      id->symbol = replacement;
      ASTNodeSetType(node, TypeIsReference(replacement->type)
                               ? replacement->type->next
                               : replacement->type);
      if (TypeIsReference(replacement->type) &&
          replacement->type->declarator == kDeclReference) {
        node->value_category = kValueCategoryLvalue;
      } else if (node->value_category != kValueCategoryXvalue) {
        node->value_category = kValueCategoryLvalue;
      }
    }
  }
}

/* Produce one expanded copy of a pack-expansion `pattern` for element
 * `element_index`: clone the pattern, rewrite pack references to this element,
 * instantiate any nested function-template calls, then reanalyze dependent
 * functor calls. */
/* Deep-copy cast_type on every cast in a pack-expansion pattern clone so each
 * element can substitute its own pack-dependent cast type without mutating the
 * shared TypeRecord that ASTNodeClone only IncRefs. */
static void DetachClonedPackExpansionCastTypes(ASTNode* node, void* data,
                                               int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL || node->op != AST_OP(cast)) {
    return;
  }
  CastASTNode* cast = (CastASTNode*)node;
  if (cast->cast_type == NULL) {
    return;
  }
  TypeRecord* copy = TypeRecordCopy(cast->cast_type);
  TypeRecordDelete(cast->cast_type);
  cast->cast_type = copy;
}

static ASTNode* ClonePackExpansionPattern(TemplateFunctionBodyClone* clone,
                                          ASTNode* pattern, Symbol* from,
                                          Symbol* to, size_t element_index) {
  ASTNode* pattern_clone = ASTNodeClone(pattern, IdentityCloneNode, NULL, NULL);
  ASTNodeVisit(pattern_clone, DetachClonedPackExpansionCastTypes, 0, NULL);
  ReplacePackIdentifierData replace = {0};
  replace.clone = clone;
  replace.from = from;
  replace.to = to;
  replace.element_index = element_index;
  ASTNodeVisit(pattern_clone, ReplacePackIdentifierVisitor, 0, &replace);
  ASTNodeVisit(pattern_clone, InstantiateClonedFunctionTemplateCallVisitor, 0,
               clone);
  pattern_clone = ASTNodeVisitAndTransform(
      pattern_clone, ReanalyzeClonedDependentFunctorCall, NULL);
  return pattern_clone;
}

/* Parameters for rewriting, in one expanded element, a captured-pack member
 * access named `from_name` to the synthetic element field `to`. */
typedef struct {
  TemplateFunctionBodyClone* clone;
  const char* from_name;
  StructMember* to;
  size_t element_index;
} ReplaceLambdaCapturePackFieldData;

/* Visitor: rewrite a `closure.field`/`closure->field` access naming a captured
 * pack to the synthetic per-element field, fixing the member, offset, and node
 * type (and the parent dereference's type). */
static void ReplaceLambdaCapturePackFieldVisitor(ASTNode* node, void* data,
                                                 int child_id,
                                                 VisitorMode mode) {
  (void)child_id;
  if (node != NULL) {
    node->flags &= ~kASTPackExpansion;
  }
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  ReplaceLambdaCapturePackFieldData* replace = data;
  if (node->op == AST_OP(cast) && replace->clone != NULL) {
    ReplacePackIdentifierData cast_replace = {0};
    cast_replace.clone = replace->clone;
    cast_replace.element_index = replace->element_index;
    ReplacePackIdentifierVisitor(node, &cast_replace, child_id, mode);
  }
  ASTNode* access_node = LambdaCapturePackMemberAccess(node);
  if (access_node != node) {
    return;
  }
  BinaryASTNode* access = (BinaryASTNode*)access_node;
  if (access->right->op == AST_OP(string)) {
    ConstantASTNode* name = (ConstantASTNode*)access->right;
    if (name->value.string == NULL ||
        strcmp(name->value.string->value, replace->from_name) != 0) {
      return;
    }
    StringSet(name->value.string, replace->to->symbol->name.value);
  } else if (access->right->op == AST_OP(structmember)) {
    StructMemberASTNode* member = (StructMemberASTNode*)access->right;
    if (member->member == NULL || member->member->symbol == NULL ||
        strcmp(member->member->symbol->name.value, replace->from_name) != 0) {
      return;
    }
    member->member = replace->to;
    member->access = replace->to->access;
    member->byte_offset = replace->to->byte_offset;
    ASTNodeSetType(access->right, replace->to->symbol->type);
  } else {
    return;
  }
  ASTNodeSetType(node, replace->to->symbol->type);
  if (node->parent != NULL && node->parent->op == AST_OP(contents) &&
      TypeIsPointer(replace->to->symbol->type)) {
    ASTNodeSetType(node->parent, replace->to->symbol->type->next);
  }
}

/* Produce one expanded copy of a pattern that references a captured pack field,
 * rewriting `from_name` to the synthetic element field `to`. */
static ASTNode* CloneLambdaCapturePackPattern(TemplateFunctionBodyClone* clone,
                                              ASTNode* pattern,
                                              const char* from_name,
                                              StructMember* to,
                                              size_t element_index) {
  ASTNode* pattern_clone = ASTNodeClone(pattern, IdentityCloneNode, NULL, NULL);
  ASTNodeVisit(pattern_clone, DetachClonedPackExpansionCastTypes, 0, NULL);
  ReplaceLambdaCapturePackFieldData replace = {0};
  replace.clone = clone;
  replace.from_name = from_name;
  replace.to = to;
  replace.element_index = element_index;
  ASTNodeVisit(pattern_clone, ReplaceLambdaCapturePackFieldVisitor, 0, &replace);
  return pattern_clone;
}

/* Expand pack-expansion call arguments (`f(args...)`) in a cloned call node
 * into the concrete sequence of per-element arguments. Handles three forms of
 * pack actuals: a bare pack identifier, an arbitrary pattern containing a pack,
 * and a captured-pack member access. Returns true if the argument list was
 * rewritten. */

static Struct* CloneLambdaClosureOwner(TemplateFunctionBodyClone* clone) {
  if (clone == NULL || clone->to_func == NULL ||
      !TypeIsFunction(clone->to_func)) {
    return NULL;
  }
  return clone->to_func->info.function.cxx_member_owner;
}

static bool ExpandClonedCallPackActuals(TemplateFunctionBodyClone* clone,
                                        ASTNode* node) {
  if (node->op != AST_OP(call)) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  Vector* expanded = NewVector();
  bool changed = false;
  for (size_t i = 0; i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    if (IsIdentifierPackExpansion(actual)) {
      IdentifierASTNode* id = (IdentifierASTNode*)actual;
      Vector* replacements = MapFindPointerKey(&clone->pack_symbol_map,
                                               id->symbol);
      if (replacements != NULL) {
        for (size_t j = 0; j < replacements->length; j++) {
          Symbol* replacement = replacements->value.p[j];
          ASTNode* replacement_id =
              NewIdentifierASTNode(replacement, actual->location);
          replacement_id->parent = node;
          replacement_id->child_id = (int)expanded->length;
          VectorAppend(expanded, replacement_id);
        }
        ASTNodeDelete(actual);
        changed = true;
        continue;
      }
    }
    if ((actual->flags & kASTPackExpansion) != 0) {
      bool multiple_packs = false;
      Symbol* pack_symbol =
          PackExpansionExpressionSymbol(clone, actual, &multiple_packs);
      if (multiple_packs) {
        SyntaxError(clone->parser->syntax,
                    "pack expansion with multiple parameter packs is not supported yet");
      }
      Vector* replacements =
          pack_symbol != NULL
              ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
              : NULL;
      if (replacements != NULL) {
        for (size_t j = 0; j < replacements->length; j++) {
          Symbol* replacement = replacements->value.p[j];
          ASTNode* expanded_actual =
              ClonePackExpansionPattern(clone, actual, pack_symbol,
                                        replacement, j);
          expanded_actual->parent = node;
          expanded_actual->child_id = (int)expanded->length;
          VectorAppend(expanded, expanded_actual);
        }
        ASTNodeDelete(actual);
        changed = true;
        continue;
      }
      Vector* capture_replacements = LambdaCapturePackFieldReplacements(actual, CloneLambdaClosureOwner(clone));
      const char* capture_name = LambdaCapturePackMemberName(actual);
      if (capture_replacements != NULL && capture_name != NULL) {
        for (size_t j = 0; j < capture_replacements->length; j++) {
          StructMember* replacement = capture_replacements->value.p[j];
          ASTNode* expanded_actual =
              CloneLambdaCapturePackPattern(clone, actual, capture_name,
                                            replacement, j);
          expanded_actual->parent = node;
          expanded_actual->child_id = (int)expanded->length;
          VectorAppend(expanded, expanded_actual);
        }
        VectorDelete(capture_replacements);
        ASTNodeDelete(actual);
        changed = true;
        continue;
      }
    }
    Vector* capture_replacements = LambdaCapturePackFieldReplacements(actual, CloneLambdaClosureOwner(clone));
    const char* capture_name = LambdaCapturePackMemberName(actual);
    if (capture_replacements != NULL && capture_name != NULL &&
        capture_replacements->length > 0) {
      for (size_t j = 0; j < capture_replacements->length; j++) {
        StructMember* replacement = capture_replacements->value.p[j];
        ASTNode* expanded_actual =
            CloneLambdaCapturePackPattern(clone, actual, capture_name,
                                          replacement, j);
        expanded_actual->parent = node;
        expanded_actual->child_id = (int)expanded->length;
        VectorAppend(expanded, expanded_actual);
      }
      VectorDelete(capture_replacements);
      ASTNodeDelete(actual);
      changed = true;
      continue;
    }
    if (capture_replacements != NULL) {
      VectorDelete(capture_replacements);
    }
    actual->parent = node;
    actual->child_id = (int)expanded->length;
    VectorAppend(expanded, actual);
  }
  if (changed) {
    VectorDelete(call->children);
    call->children = expanded;
  } else {
    VectorDelete(expanded);
  }
  return changed;
}

/* True if any actual argument of `call` is still an unexpanded pack expansion
 * (so the call cannot yet be resolved to a concrete overload). */
static bool CallActualsStillContainPackExpansion(VectorASTNode* call) {
  for (size_t i = 0; call != NULL && i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    if (actual != NULL && (actual->flags & kASTPackExpansion) != 0) {
      return true;
    }
    Vector* replacements = LambdaCapturePackFieldReplacements(actual, NULL);
    if (replacements != NULL) {
      bool still_contains_pack = replacements->length > 0;
      VectorDelete(replacements);
      if (still_contains_pack) {
        return true;
      }
    }
  }
  return false;
}

/* Set a cloned call node's result type and value category from the resolved
 * function type `func`. A reference return yields an lvalue/xvalue of the
 * referenced type; otherwise a prvalue of the return type. */
static void SetClonedCallReturnType(VectorASTNode* call, TypeRecord* func) {
  if (call == NULL || func == NULL) {
    return;
  }
  if (TypeIsPointer(func)) {
    func = func->next;
  }
  if (!TypeIsFunction(func)) {
    return;
  }
  TypeRecord* return_type = func->next;
  if (return_type == NULL) {
    return;
  }
  if (TypeIsReference(return_type)) {
    ASTNodeSetType((ASTNode*)call, return_type->next);
    call->base.value_category =
        return_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    ASTNodeSetType((ASTNode*)call, return_type);
  }
}

/* In a cloned body, resolve a call whose callee is a function template to the
 * concrete instantiation deduced from the (now concrete) explicit template
 * arguments and actual arguments, updating the callee symbol and result type.
 * Skips calls whose actuals still contain pack expansions. */
static void InstantiateClonedFunctionTemplateCall(
    TemplateFunctionBodyClone* clone, ASTNode* node) {
  if (node->op != AST_OP(call)) {
    return;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(identifier) ||
      CallActualsStillContainPackExpansion(call)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)call->left;
  if (id->template_arguments != NULL && id->symbol != NULL &&
      id->symbol->type != NULL && TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.template_origin != NULL) {
    id->symbol = id->symbol->type->info.function.template_origin;
    ASTNodeSetType(call->left, id->symbol->type);
  }
  if (id->symbol == NULL || !id->symbol->flags.is_template ||
      !TypeIsFunction(id->symbol->type)) {
    return;
  }
  if (TemplateArgumentVectorContainsTemplateParameter(id->template_arguments)) {
    return;
  }
  // An actual argument whose type still contains `auto` has not been re-deduced
  // yet in this instantiation (its declaration is re-analyzed later, at compile
  // time).  A classic case is a local closure variable, `auto l = [..]{..};`,
  // passed to a function template: at this early post-clone pass `l` is still an
  // undeduced placeholder, so deducing the callee now would bind it against the
  // wrong (placeholder) argument type.  Leave the call for the full re-analysis
  // that runs once `auto` is resolved.
  //
  // The same deferral applies to a temporary lambda-expression passed directly
  // (`f([&]{ ... })`): its closure type may already look concrete after nested
  // struct substitution, but its `operator()` body is only fully re-analyzed
  // later.  Instantiating the callee against that temporary now binds a
  // specialization to a half-lowered call operator (e.g. an indirect call of a
  // captured functor) and never recovers.
  if (call->children != NULL) {
    for (size_t i = 0; i < call->children->length; i++) {
      ASTNode* actual = call->children->value.p[i];
      if (actual == NULL) {
        continue;
      }
      if (actual->type != NULL && TypeContainsAuto(actual->type)) {
        return;
      }
      if ((actual->flags & kASTLambdaExpression) != 0) {
        return;
      }
    }
  }
  Symbol* instantiated = NULL;
  if (id->symbol->overload_next != NULL) {
    // The callee names an *overloaded* function template.  During the first
    // (dependent) pass the identifier was bound to whichever overload the
    // lookup happened to return (the head of the set); binding the call to that
    // one and instantiating it would bypass overload resolution entirely.  Now
    // that the actuals are concrete, resolve across the whole overload set so
    // the best-matching overload wins (e.g. tag-dispatch on iterator category).
    instantiated = CXXResolveOverloadedFunctionTemplateCall(
        id->symbol, id->template_arguments, call->children);
  }
  if (instantiated == NULL) {
    instantiated = TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
        clone->parser->syntax, id->symbol, id->template_arguments,
        call->children);
  }
  if (instantiated != NULL && instantiated != id->symbol) {
    id->symbol = instantiated;
    ASTNodeSetType(call->left, instantiated->type);
    SetClonedCallReturnType(call, instantiated->type);
  }
}

/* True if `node` is an expression-initializer wrapping a bare pack-identifier
 * expansion (`xs...`); reports the pack symbol and source location. */
static bool ExpressionInitializerIsIdentifierPackExpansion(ASTNode* node,
                                                           Symbol** symbol,
                                                           SourceLocation* loc) {
  if (node == NULL || node->op != AST_OP(expr_init)) {
    return false;
  }
  ExpressionInitializerASTNode* init = (ExpressionInitializerASTNode*)node;
  if (!IsIdentifierPackExpansion(init->expr)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)init->expr;
  if (symbol != NULL) {
    *symbol = id->symbol;
  }
  if (loc != NULL) {
    *loc = init->expr->location;
  }
  return true;
}

/* Expand pack expansions appearing as elements of a braced initializer
 * (`{xs...}`), including designated-initializer member packs, into the concrete
 * per-element initializers. */
static void ExpandClonedBracedInitializerPackElements(
    TemplateFunctionBodyClone* clone, ASTNode* node) {
  if (node->op != AST_OP(braced_init)) {
    return;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)node;
  Vector* expanded = NewVector();
  bool changed = false;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    Symbol* pack_symbol = NULL;
    SourceLocation location = initializer != NULL ? initializer->location
                                                  : node->location;
    if (ExpressionInitializerIsIdentifierPackExpansion(initializer,
                                                       &pack_symbol,
                                                       &location)) {
      Vector* replacements = MapFindPointerKey(&clone->pack_symbol_map,
                                               pack_symbol);
      if (replacements != NULL) {
        for (size_t j = 0; j < replacements->length; j++) {
          ASTNode* replacement =
              NewIdentifierASTNode(replacements->value.p[j], location);
          ASTNode* expr_init =
              NewExpressionInitializerASTNode(replacement, location);
          expr_init->parent = node;
          expr_init->child_id = (int)expanded->length;
          VectorAppend(expanded, expr_init);
        }
        ASTNodeDelete(initializer);
        changed = true;
        continue;
      }
    }
    if (initializer != NULL && initializer->op == AST_OP(designated_init)) {
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)initializer;
      ASTNode* init = designated->init;
      if (init != NULL && init->op == AST_OP(expr_init)) {
        ExpressionInitializerASTNode* expr_init =
            (ExpressionInitializerASTNode*)init;
        if (expr_init->expr != NULL &&
            (expr_init->expr->flags & kASTPackExpansion) != 0 &&
            designated->designators != NULL &&
            designated->designators->length == 1) {
          Designator* designator = designated->designators->value.p[0];
          const char* member_name =
              designator != NULL &&
                      designator->designator_type == kDesignatorStruct &&
                      !designator->is_resolved_member &&
                      designator->value.struct_member_name != NULL
                  ? designator->value.struct_member_name->value
                  : NULL;
          bool multiple_packs = false;
          pack_symbol = PackExpansionExpressionSymbol(clone, expr_init->expr,
                                                      &multiple_packs);
          if (multiple_packs) {
            SyntaxError(clone->parser->syntax,
                        "pack expansion with multiple parameter packs is not supported yet");
          }
          Vector* replacements =
              pack_symbol != NULL
                  ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
                  : NULL;
          if (member_name != NULL && replacements != NULL) {
            for (size_t j = 0; j < replacements->length; j++) {
              ASTNode* replacement = ClonePackExpansionPattern(
                  clone, expr_init->expr, pack_symbol,
                  replacements->value.p[j], j);
              ASTNode* expanded_init =
                  NewExpressionInitializerASTNode(replacement, location);
              Vector* designators = NewVector();
              String field_name;
              StringInit(&field_name, NULL);
              LambdaCapturePackElementName(&field_name, member_name, j);
              VectorAppend(designators,
                           NewStructDesignator(NewString(field_name.value)));
              StringDestruct(&field_name);
              ASTNode* expanded_designated =
                  NewDesignatedInitializerASTNode(designators, expanded_init,
                                                  location);
              expanded_designated->parent = node;
              expanded_designated->child_id = (int)expanded->length;
              VectorAppend(expanded, expanded_designated);
            }
            ASTNodeDelete(initializer);
            changed = true;
            continue;
          }
        }
      }
    }
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      ExpressionInitializerASTNode* expr_init_node =
          (ExpressionInitializerASTNode*)initializer;
      if (expr_init_node->expr != NULL &&
          (expr_init_node->expr->flags & kASTPackExpansion) != 0) {
        bool multiple_packs = false;
        pack_symbol = PackExpansionExpressionSymbol(clone, expr_init_node->expr,
                                                    &multiple_packs);
        if (multiple_packs) {
          SyntaxError(clone->parser->syntax,
                      "pack expansion with multiple parameter packs is not supported yet");
        }
        Vector* replacements =
            pack_symbol != NULL
                ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
                : NULL;
        if (replacements != NULL) {
          for (size_t j = 0; j < replacements->length; j++) {
            ASTNode* replacement = ClonePackExpansionPattern(
                clone, expr_init_node->expr, pack_symbol,
                replacements->value.p[j], j);
            ASTNode* expanded_init =
                NewExpressionInitializerASTNode(replacement, location);
            expanded_init->parent = node;
            expanded_init->child_id = (int)expanded->length;
            VectorAppend(expanded, expanded_init);
          }
          ASTNodeDelete(initializer);
          changed = true;
          continue;
        }
        Vector* capture_replacements =
            LambdaCapturePackFieldReplacements(expr_init_node->expr, CloneLambdaClosureOwner(clone));
        const char* capture_name =
            LambdaCapturePackMemberName(expr_init_node->expr);
        if (capture_replacements != NULL && capture_name != NULL) {
          for (size_t j = 0; j < capture_replacements->length; j++) {
            ASTNode* replacement = CloneLambdaCapturePackPattern(
                clone, expr_init_node->expr, capture_name,
                capture_replacements->value.p[j], j);
            ASTNode* expanded_init =
                NewExpressionInitializerASTNode(replacement, location);
            expanded_init->parent = node;
            expanded_init->child_id = (int)expanded->length;
            VectorAppend(expanded, expanded_init);
          }
          VectorDelete(capture_replacements);
          ASTNodeDelete(initializer);
          changed = true;
          continue;
        }
      }
    }
    if (initializer != NULL) {
      initializer->parent = node;
      initializer->child_id = (int)expanded->length;
    }
    VectorAppend(expanded, initializer);
  }
  if (changed) {
    VectorDelete(braced->initializers);
    braced->initializers = expanded;
  } else {
    VectorDelete(expanded);
  }
}

/* The identity value for an empty fold expression: `&&` folds to true, `||` to
 * false; any other operator over an empty pack is an error. */
static ASTNode* NewFoldIdentity(ASTOpcode op, SourceLocation location,
                                TypeParser* parser) {
  if (op == AST_OP(logand)) {
    return NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                                 location);
  }
  if (op == AST_OP(logor)) {
    return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                                 location);
  }
  SyntaxError(parser->syntax, "Empty fold expression is not supported for this operator");
  return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                               location);
}

/* Expand a fold expression (`(... op pack)` / `(pack op ...)` / binary folds)
 * in a cloned body into a left- or right-associated chain of the operator over
 * the concrete pack elements, using NewFoldIdentity for the empty case. */
static ASTNode* ExpandClonedFoldExpression(TemplateFunctionBodyClone* clone,
                                           ASTNode* node) {
  if (node == NULL || (node->flags & kASTFoldExpression) == 0 ||
      (node->op != AST_OP(mult) && node->op != AST_OP(plus) &&
       node->op != AST_OP(minus) && node->op != AST_OP(div) &&
       node->op != AST_OP(mod) && node->op != AST_OP(lshift) &&
       node->op != AST_OP(rshift) && node->op != AST_OP(and) &&
       node->op != AST_OP(exor) && node->op != AST_OP(bitor) &&
       node->op != AST_OP(logand) && node->op != AST_OP(logor))) {
    return node;
  }

  BinaryASTNode* fold = (BinaryASTNode*)node;
  bool pack_on_left = (node->flags & kASTFoldPackOnLeft) != 0;
  ASTNode* pack_node = pack_on_left ? fold->left : fold->right;
  ASTNode* seed = pack_on_left ? fold->right : fold->left;
  bool multiple_packs = false;
  Symbol* pack_symbol =
      PackExpansionExpressionSymbol(clone, pack_node, &multiple_packs);
  if (multiple_packs) {
    SyntaxError(clone->parser->syntax,
                "pack expansion with multiple parameter packs is not supported yet");
  }
  Vector* replacements =
      pack_symbol != NULL
          ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
          : NULL;
  const char* capture_name = NULL;
  bool capture_pack = false;
  if (replacements == NULL) {
    replacements = LambdaCapturePackFieldReplacements(pack_node, CloneLambdaClosureOwner(clone));
    capture_name = LambdaCapturePackMemberName(pack_node);
    capture_pack = replacements != NULL && capture_name != NULL;
  }
  if (replacements == NULL) {
    return node;
  }
  if (replacements->length == 0) {
    if (capture_pack) {
      VectorDelete(replacements);
    }
    if (seed != NULL) {
      return seed;
    }
    return NewFoldIdentity(node->op, node->location, clone->parser);
  }

  if (pack_on_left) {
    ASTNode* result = seed != NULL
                          ? seed
                          : capture_pack
                                ? CloneLambdaCapturePackPattern(
                                      clone, pack_node, capture_name,
                                      replacements->value.p[
                                          replacements->length - 1],
                                      replacements->length - 1)
                                : ClonePackExpansionPattern(
                                      clone, pack_node, pack_symbol,
                                      replacements->value.p[
                                          replacements->length - 1],
                                      replacements->length - 1);
    size_t start = seed != NULL ? replacements->length
                                : replacements->length - 1;
    for (size_t i = start; i > 0; i--) {
      ASTNode* left = capture_pack
                          ? CloneLambdaCapturePackPattern(
                                clone, pack_node, capture_name,
                                replacements->value.p[i - 1], i - 1)
                          : ClonePackExpansionPattern(
                                clone, pack_node, pack_symbol,
                                replacements->value.p[i - 1], i - 1);
      result = NewBinaryASTNode(node->op, NULL, node->location, left, result);
    }
    if (capture_pack) {
      VectorDelete(replacements);
    }
    return result;
  }

  ASTNode* result = seed != NULL
                        ? seed
                        : capture_pack
                              ? CloneLambdaCapturePackPattern(
                                    clone, pack_node, capture_name,
                                    replacements->value.p[0], 0)
                              : ClonePackExpansionPattern(
                                    clone, pack_node, pack_symbol,
                                    replacements->value.p[0], 0);
  size_t start = seed != NULL ? 0 : 1;
  for (size_t i = start; i < replacements->length; i++) {
    ASTNode* right = capture_pack
                         ? CloneLambdaCapturePackPattern(
                               clone, pack_node, capture_name,
                               replacements->value.p[i], i)
                         : ClonePackExpansionPattern(
                               clone, pack_node, pack_symbol,
                               replacements->value.p[i], i);
    result = NewBinaryASTNode(node->op, NULL, node->location, result, right);
  }
  if (capture_pack) {
    VectorDelete(replacements);
  }
  return result;
}

/* Build the member-access constructor call `receiver.Tag(actuals)` for a
 * dependent `new` initializer whose type resolved to the class `record`. */
static ASTNode* NewClonedDependentConstructorCall(TypeRecord* record,
                                                  ASTNode* receiver,
                                                  Vector* actuals,
                                                  SourceLocation location) {
  ASTNode* member = NewStringConstantASTNode(
      NewString(record->info.struct_info->tag_name->value), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
}

/* Rewrite a deferred `new` initializer (parsed as an assignment while T was
 * dependent) now that T has resolved to a concrete type.
 *
 *   * `new T(expr)` copy/direct-init becomes a constructor call
 *     `receiver.T(expr)` when T is a class, and stays a scalar assignment
 *     otherwise.
 *   * `new T()` value-init (flagged kASTDependentNewValueInit) becomes a
 *     default constructor call `receiver.T()` when T is a class with a
 *     constructor, an empty-compound-literal zero-init `*receiver = (T){}` when
 *     T is an aggregate class, and keeps its `*receiver = 0` scalar zero-init
 *     otherwise. */
static ASTNode* RewriteClonedDependentNewInitializer(
    TemplateFunctionBodyClone* clone, ASTNode* node) {
  if (node == NULL || node->op != AST_OP(assign) ||
      (node->flags & kASTDependentNewInitializer) == 0 || node->type == NULL) {
    return node;
  }
  BinaryASTNode* assign = (BinaryASTNode*)node;
  if (assign->left == NULL || assign->right == NULL) {
    return node;
  }
  // If the allocated type is still dependent (e.g. a `typename X<Target,T>::type`
  // whose member-template parameter `Target` is not yet concrete during the
  // enclosing class-template instantiation), leave the deferred initializer
  // intact.  Rewriting it now against the unresolved placeholder would bake in a
  // wrong scalar zero-init and lose the dependent-type information the later
  // member-template instantiation needs to resolve the real class type.
  if (TypeContainsTemplateParameter(node->type) ||
      node->type->dependent_member_name != NULL ||
      (node->type->type & kTypeUnknown) != 0) {
    return node;
  }
  bool is_class = TypeIsStructOrUnion(node->type) &&
                  node->type->info.struct_info != NULL &&
                  node->type->info.struct_info->tag_name != NULL;

  if ((node->flags & kASTDependentNewValueInit) != 0) {
    if (is_class && CXXRecordHasConstructorMember(node->type)) {
      // Class with a constructor: value-init runs the default constructor.
      return NewClonedDependentConstructorCall(node->type,
                                               ASTNodeMove(assign->left),
                                               NewVector(), node->location);
    }
    if (is_class) {
      // Aggregate class with no constructor: zero-initialize the object with an
      // empty compound literal `*receiver = (T){}`.
      ASTNode* receiver = ASTNodeMove(assign->left);
      Symbol* storage =
          SyntaxNewTemporary(clone->parser->syntax, node->type);
      ASTNode* literal = NewCompoundLiteralASTNode(
          NewIdentifierASTNode(storage, node->location), node->location,
          NewBracedInitializerASTNode(NewVector(), NULL, node->location));
      return NewBinaryASTNode(AST_OP(assign), node->type, node->location,
                              receiver, literal);
    }
    // Scalar: the placeholder `*receiver = 0` already zero-initializes it.
    node->flags &= ~(kASTDependentNewInitializer | kASTDependentNewValueInit);
    return node;
  }

  if (!is_class) {
    if (assign->right != NULL && assign->right->op == AST_OP(braced_init)) {
      BracedInitializerASTNode* braced =
          (BracedInitializerASTNode*)assign->right;
      bool dependent_initializer = false;
      for (size_t i = 0; i < braced->initializers->length; i++) {
        ASTNode* init = braced->initializers->value.p[i];
        if (init != NULL &&
            ((init->flags & kASTPackExpansion) != 0 ||
             DependentExpressionContainsTemplateParameter(init))) {
          dependent_initializer = true;
          break;
        }
      }
      if (dependent_initializer) {
        return node;
      }
      if (braced->initializers->length > 1) {
        SyntaxError(clone->parser->syntax,
                    "too many initializers for scalar new-expression");
        return node;
      }
      ASTNode* replacement = braced->initializers->length == 1
          ? ASTNodeMove(braced->initializers->value.p[0])
          : NewIntConstantASTNode(0, TypeRecordCopy(node->type),
                                  node->location);
      replacement->parent = node;
      replacement->child_id = 1;
      ASTNodeDelete(assign->right);
      assign->right = replacement;
      return node;
    }
    // `new T(expr)` where T resolved to a scalar keeps its scalar assignment.
    // When the initializer is a parameter-pack expansion (e.g. the allocator's
    // `new (ptr) U(std::forward<Args>(args)...)` with U deduced to a scalar),
    // expand it here against this instantiation's pack: an empty pack
    // value-initializes the scalar (`*receiver = 0`), a single-element pack
    // supplies the value, and more than one element is ill-formed.  Otherwise
    // the unexpanded pack would survive as a stray `*receiver = forward(args)`
    // whose callee template arguments are never substituted.
    if (assign->right != NULL &&
        (assign->right->flags & kASTPackExpansion) != 0) {
      bool multiple_packs = false;
      Symbol* pack_symbol =
          PackExpansionExpressionSymbol(clone, assign->right, &multiple_packs);
      if (multiple_packs) {
        SyntaxError(clone->parser->syntax,
                    "pack expansion with multiple parameter packs is not supported yet");
      }
      Vector* replacements =
          pack_symbol != NULL
              ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
              : NULL;
      if (replacements != NULL) {
        if (replacements->length > 1) {
          SyntaxError(clone->parser->syntax,
                      "too many initializers for scalar new-expression");
          return node;
        }
        ASTNode* pattern = ASTNodeMove(assign->right);
        ASTNode* expanded =
            replacements->length == 1
                ? ClonePackExpansionPattern(clone, pattern, pack_symbol,
                                            replacements->value.p[0], 0)
                : NewIntConstantASTNode(0, TypeRecordCopy(node->type),
                                        node->location);
        ASTNodeDelete(pattern);
        expanded->parent = node;
        expanded->child_id = 1;
        assign->right = expanded;
        node->flags &= ~kASTPackExpansion;
      }
    }
    return node;
  }
  Vector* actuals = NewVector();
  if (assign->right != NULL && assign->right->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced =
        (BracedInitializerASTNode*)assign->right;
    for (size_t i = 0; i < braced->initializers->length; i++) {
      VectorAppend(actuals, ASTNodeMove(braced->initializers->value.p[i]));
    }
    ASTNodeDelete(assign->right);
  } else if (assign->right != NULL &&
             (assign->right->flags & kASTPackExpansion) != 0) {
    // `new (ptr) T(std::forward<Args>(args)...)`: expand the initializer pack
    // against this instantiation before it becomes constructor arguments.  An
    // empty pack value-initializes T (handled below for an aggregate); a
    // non-empty pack supplies one actual per element.  Without this the raw
    // pack-expansion pattern would survive as a single bogus argument whose
    // callee template arguments are never substituted.
    bool multiple_packs = false;
    Symbol* pack_symbol =
        PackExpansionExpressionSymbol(clone, assign->right, &multiple_packs);
    if (multiple_packs) {
      SyntaxError(clone->parser->syntax,
                  "pack expansion with multiple parameter packs is not supported yet");
    }
    Vector* replacements =
        pack_symbol != NULL
            ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
            : NULL;
    if (replacements == NULL) {
      // The pack is not resolvable at this level (e.g. it belongs to an
      // enclosing template): leave the deferred initializer for a later clone.
      VectorDelete(actuals);
      return node;
    }
    ASTNode* pattern = ASTNodeMove(assign->right);
    for (size_t i = 0; i < replacements->length; i++) {
      VectorAppend(actuals,
                   ClonePackExpansionPattern(clone, pattern, pack_symbol,
                                             replacements->value.p[i], i));
    }
    ASTNodeDelete(pattern);
  } else {
    VectorAppend(actuals, ASTNodeMove(assign->right));
  }

  ASTNode* receiver = ASTNodeMove(assign->left);
  if (actuals->length == 0 && !CXXRecordHasConstructorMember(node->type)) {
    // Value-initialize an aggregate class with no constructor by zero-init'ing
    // the object with an empty compound literal `*receiver = (T){}`.
    VectorDelete(actuals);
    Symbol* storage = SyntaxNewTemporary(clone->parser->syntax, node->type);
    ASTNode* literal = NewCompoundLiteralASTNode(
        NewIdentifierASTNode(storage, node->location), node->location,
        NewBracedInitializerASTNode(NewVector(), NULL, node->location));
    return NewBinaryASTNode(AST_OP(assign), node->type, node->location,
                            receiver, literal);
  }
  return NewClonedDependentConstructorCall(node->type, receiver, actuals,
                                           node->location);
}

/* True if `node` or any ancestor is itself a pack expansion (so it will be
 * materialized per-element by the surrounding expansion, not here). */
static bool ASTNodeWithinPackExpansion(ASTNode* node) {
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    if ((current->flags & kASTPackExpansion) != 0) {
      return true;
    }
  }
  return false;
}

/* True if `node` sits inside a pack-expansion pattern (`pattern...`) whose pack
 * is NOT resolved by this clone -- i.e. it belongs to a not-yet-instantiated
 * nested/member template (e.g. `std::forward<Args>(args)...` in a variadic
 * member template while its enclosing class template is being instantiated).
 * Sub-expressions of such a pattern must be left untouched: their per-element
 * expansion happens later, at the member template's own instantiation, so
 * concretizing their template arguments now (against the enclosing class's
 * arguments) would bind the wrong pack. */
static bool ASTNodeWithinUnresolvedPackExpansion(TemplateFunctionBodyClone* clone,
                                                 ASTNode* node) {
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    if ((current->flags & kASTPackExpansion) != 0 &&
        ClonePatternReferencesUnresolvedPack(clone, current)) {
      return true;
    }
  }
  return false;
}

/* Per-node transform applied while cloning a template function body for one
 * instantiation. It turns the dependent generic AST into a concrete AST by:
 *   - evaluating `sizeof...(pack)` to the concrete pack length;
 *   - expanding fold expressions and dependent `delete`;
 *   - substituting identifier template arguments and rebinding symbols;
 *   - substituting cast/declaration types against the instantiation args;
 *   - rewriting deferred dependent `new` initializers and member calls.
 * Returns the (possibly replacement) node for this position. */
static ASTNode* CloneTemplateFunctionBodyNode(ASTNode* node, void* data) {
  TemplateFunctionBodyClone* clone = data;
  if (node->op == AST_OP(ptr_scale)) {
    PtrScaleASTNode* scale = (PtrScaleASTNode*)node;
    ASTNode* expr = scale->expr;
    scale->expr = NULL;
    ASTNodeDelete(node);
    return expr;
  }
  if (node->op == AST_OP(requires_expr)) {
    RequiresExpressionASTNode* requires_node =
        (RequiresExpressionASTNode*)node;
    SourceLocation location = node->location;
    int64_t value = 0;
    bool ok = ConceptsEvaluateConstraintWithArguments(
        requires_node->constraint, clone->args, &value);
    ConstraintExprDelete(requires_node->constraint);
    requires_node->constraint = NULL;
    ASTNodeDelete(node);
    return NewIntConstantASTNode(ok && value != 0 ? 1 : 0,
                                 NewTypeRecordWithSize(kTypeBool, kQualPlain),
                                 location);
  }
  if (node->op == AST_OP(structmember) && clone->from_owner != NULL &&
      clone->to_owner != NULL && clone->from_owner != clone->to_owner) {
    StructMemberASTNode* member_node = (StructMemberASTNode*)node;
    if (member_node->member != NULL && member_node->member->symbol != NULL) {
      StructMember* concrete =
          FindStructMemberByName(clone->to_owner,
                                 member_node->member->symbol->name.value);
      if (concrete != NULL && concrete->symbol != NULL) {
        member_node->member = concrete;
        member_node->access = concrete->access;
        member_node->byte_offset = concrete->byte_offset;
        ASTNodeSetType(node, concrete->symbol->type);
      }
    }
  }
  // Explicit template arguments on an accessed member function template, e.g.
  // `obj.template mfn<I>()`, are held on the StructMemberASTNode rather than on
  // an identifier. Substitute the instantiation's arguments into them (mirroring
  // the identifier handling below) so a dependent explicit argument like `I`
  // becomes concrete and the member call can be resolved during instantiation.
  // Explicit template arguments on an accessed member function template, e.g.
  // `obj.template mfn<I>()`, are held on the member-name node (a StructMember
  // node once resolved, or an as-yet-unanalyzed string constant) rather than on
  // an ordinary identifier. Substitute the instantiation's arguments into them
  // (mirroring the identifier handling below) so a dependent explicit argument
  // like `I` becomes concrete and the member call resolves during instantiation.
  {
    Vector** member_template_args = NULL;
    if (node->op == AST_OP(structmember)) {
      member_template_args = &((StructMemberASTNode*)node)->template_arguments;
    } else if (node->op == AST_OP(string)) {
      member_template_args = &((ConstantASTNode*)node)->template_arguments;
    }
    if (member_template_args != NULL && *member_template_args != NULL) {
      Vector* member_args = *member_template_args;
      bool member_template_args_contain_pack = false;
      int pack_index = -1;
      size_t pack_length = 0;
      for (size_t i = 0; i < member_args->length; i++) {
        if (FindPackExpansionInTemplateArgument(member_args->value.p[i],
                                                clone->args, &pack_index,
                                                &pack_length)) {
          member_template_args_contain_pack = true;
          break;
        }
      }
      if (!member_template_args_contain_pack &&
          !ASTNodeWithinPackExpansion(node)) {
        Vector* concrete_args = SubstituteTemplateArgumentVector(
            clone->parser, member_args, clone->args,
            clone->rebase_template_parameter_base);
        VectorDeleteWithContents(
            member_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        *member_template_args = concrete_args;
      } else if (clone->rebase_template_parameter_base > 0) {
        for (size_t i = 0; i < member_args->length; i++) {
          RebaseTemplateArgumentParameterIndices(
              member_args->value.p[i],
              clone->rebase_template_parameter_base);
        }
      }
    }
  }
  if ((node->op == AST_OP(dot) || node->op == AST_OP(arrow)) &&
      ((BinaryASTNode*)node)->right != NULL &&
      ((BinaryASTNode*)node)->right->op == AST_OP(structmember)) {
    StructMemberASTNode* member_node =
        (StructMemberASTNode*)((BinaryASTNode*)node)->right;
    if (member_node->member != NULL && member_node->member->symbol != NULL) {
      ASTNodeSetType(node, member_node->member->symbol->type);
      if (!member_node->member->is_member_function) {
        node->value_category = kValueCategoryLvalue;
      }
    }
  }
  if (node->op == AST_OP(sizeof)) {
    SizeofASTNode* sizeof_node = (SizeofASTNode*)node;
    if (sizeof_node->is_pack_size && sizeof_node->expr != NULL) {
      if (sizeof_node->expr->op == AST_OP(identifier)) {
        IdentifierASTNode* id = (IdentifierASTNode*)sizeof_node->expr;
        Vector* replacements = MapFindPointerKey(&clone->pack_symbol_map,
                                                 id->symbol);
        if (replacements != NULL) {
          return NewIntConstantASTNode(
              (int64_t)replacements->length, NewSizeTypeRecord(),
              node->location);
        }
        int pack_index =
            id->symbol != NULL ? id->symbol->template_parameter_index : -1;
        if ((pack_index < 0) && id->symbol != NULL) {
          TypeIsTemplateParameterPlaceholder(id->symbol->type, &pack_index);
        }
        if (id->symbol != NULL && id->symbol->flags.is_parameter_pack &&
            pack_index >= 0 && (size_t)pack_index < clone->args->length) {
          TemplateArgument* arg = clone->args->value.p[pack_index];
          if (arg != NULL && arg->pack_arguments != NULL) {
            return NewIntConstantASTNode(
                (int64_t)arg->pack_arguments->length, NewSizeTypeRecord(),
                node->location);
          }
        }
      }
      Vector* capture_replacements =
          LambdaCapturePackFieldReplacements(sizeof_node->expr, CloneLambdaClosureOwner(clone));
      if (capture_replacements != NULL) {
        size_t length = capture_replacements->length;
        VectorDelete(capture_replacements);
        size_t argument_length = 0;
        if (LambdaCapturePackFieldArgumentLength(sizeof_node->expr,
                                                 clone->args,
                                                 &argument_length,
                                                 CloneLambdaClosureOwner(clone))) {
          length = argument_length;
        }
        return NewIntConstantASTNode((int64_t)length, NewSizeTypeRecord(),
                                     node->location);
      }
    }
    // `sizeof(dependent-type)`: substitute the retained operand type and
    // recompute the size for this instantiation.
    if (sizeof_node->type_operand != NULL &&
        TypeContainsTemplateParameter(sizeof_node->type_operand)) {
      TypeRecord* concrete = SubstituteTemplateParameters(
          clone->parser, sizeof_node->type_operand, clone->args);
      RebaseTemplateParameterIndices(concrete,
                                     clone->rebase_template_parameter_base);
      TypeRecordCalculateSize(concrete);
      TypeRecordDelete(sizeof_node->type_operand);
      sizeof_node->type_operand = concrete;
      sizeof_node->base.value.ivalue = concrete->size;
    }
  }
  if ((node->flags & kASTFoldExpression) != 0) {
    return ExpandClonedFoldExpression(clone, node);
  }
  if ((node->flags & kASTDependentDelete) != 0 &&
      node->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->children != NULL && call->children->length == 1) {
      ASTNode* expr = call->children->value.p[0];
      bool is_array_delete = (node->flags & kASTDependentArrayDelete) != 0;
      return NewCXXDeleteExpressionForPointer(clone->parser->syntax, expr,
                                              is_array_delete,
                                              node->location,
                                              /*global_scope=*/false);
    }
  }
  // Injected-class-name as a functional-cast callee: `ClassName(args)` inside a
  // member body of the primary template must construct a temporary of *this*
  // instantiation, not the generic template.  We only rewrite the call's callee
  // identifier (not other uses of the name, e.g. as a type in a cast or a
  // declared object's type, which must stay pure type references); otherwise the
  // tag would be misused as runtime storage.  Children are cloned before this
  // callback runs, so `call->left` is already the cloned callee here.
  if (node->op == AST_OP(call) && clone->from_owner != NULL &&
      clone->to_owner != NULL && clone->from_owner != clone->to_owner &&
      clone->to_owner->tag_symbol != NULL &&
      clone->to_owner->tag_symbol->type != NULL) {
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->left != NULL && call->left->op == AST_OP(identifier)) {
      IdentifierASTNode* callee = (IdentifierASTNode*)call->left;
      if (callee->symbol != NULL && callee->symbol->type != NULL &&
          TypeIsStructOrUnion(callee->symbol->type) &&
          callee->symbol->type->info.struct_info == clone->from_owner) {
        callee->symbol = clone->to_owner->tag_symbol;
        ASTNodeSetType(call->left, clone->to_owner->tag_symbol->type);
      }
    }
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    // Dependent qualified value name like `T::member` or `Trait<T>::member`:
    // substitute the scope placeholder to its concrete class (a bare template
    // parameter or a dependent template specialization), then resolve the named
    // static/enum member against the resulting concrete type.
    if ((node->flags & kASTDependentQualifiedName) != 0 && id->symbol != NULL &&
        id->symbol->type != NULL &&
        id->symbol->type->dependent_member_name != NULL &&
        (id->symbol->type->template_parameter_index >= 0 ||
         id->symbol->type->template_origin != NULL)) {
      TypeRecord* scope = TypeRecordCopy(id->symbol->type);
      StringDelete(scope->dependent_member_name);
      scope->dependent_member_name = NULL;
      TypeRecord* concrete =
          SubstituteTemplateParameters(clone->parser, scope, clone->args);
      RebaseTemplateParameterIndices(concrete,
                                     clone->rebase_template_parameter_base);
      TypeRecordDelete(scope);
      if (concrete != NULL && TypeIsStructOrUnion(concrete) &&
          concrete->info.struct_info != NULL) {
        StructMember* member = FindStructMember(
            concrete->info.struct_info, id->symbol->type->dependent_member_name);
        if (member != NULL && member->symbol != NULL &&
            (member->is_static ||
             StorageIs(member->symbol->storage, STO(typedef)) ||
             member->symbol->flags.value_set)) {
          id->symbol = member->symbol;
          ASTNodeSetType(node, member->symbol->type);
          node->flags &= ~kASTDependentQualifiedName;
          node->value_category = kValueCategoryLvalue;
          TypeRecordDelete(concrete);
          return node;
        }
      }
      // The scope is still dependent (e.g. `__index_of<T, Types...>::value`
      // where the member template's own `T` is unknown but the enclosing class
      // pack `Types` is now concrete).  Bake the partial substitution back into
      // the identifier's scope type -- re-attaching the member name -- so a
      // later, more-concrete instantiation (which supplies `T`) can finish
      // resolving the qualified name.  Without this the enclosing class's
      // arguments would be dropped and the name would stay forever dependent.
      if (concrete != NULL && TypeContainsTemplateParameter(concrete)) {
        concrete->dependent_member_name =
            NewString(id->symbol->type->dependent_member_name->value);
        Symbol* copy = NewSymbol(id->symbol->name.value, concrete,
                                 id->symbol->storage);
        copy->namespace_ = id->symbol->namespace_;
        copy->flags = id->symbol->flags;
        copy->template_parameter_index = id->symbol->template_parameter_index;
        copy->dependent_value_template_parameter_index =
            id->symbol->dependent_value_template_parameter_index;
        id->symbol = copy;
        ASTNodeSetType(node, concrete);
        return node;
      }
      TypeRecordDelete(concrete);
    }
    bool template_args_contain_pack = false;
    if (id->template_arguments != NULL) {
      int pack_index = -1;
      size_t pack_length = 0;
      for (size_t i = 0; i < id->template_arguments->length; i++) {
        if (FindPackExpansionInTemplateArgument(
                id->template_arguments->value.p[i], clone->args, &pack_index,
                &pack_length)) {
          template_args_contain_pack = true;
          break;
        }
      }
    }
    if (id->template_arguments != NULL && !template_args_contain_pack &&
        !ASTNodeWithinPackExpansion(node)) {
      Vector* concrete_args = SubstituteTemplateArgumentVector(
          clone->parser, id->template_arguments, clone->args,
          clone->rebase_template_parameter_base);
      VectorDeleteWithContents(id->template_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      id->template_arguments = concrete_args;
      if (id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
          id->symbol->type->info.function.template_origin != NULL) {
        id->symbol = id->symbol->type->info.function.template_origin;
      }
    } else if (id->template_arguments != NULL &&
               clone->rebase_template_parameter_base > 0) {
      // The full substitution above is deferred when this template-id is inside
      // a pack expansion (e.g. `std::forward<Args>(args)...`), because the pack
      // elements are not known until the pattern is expanded later. However, if
      // this body belongs to a member template of an enclosing template (so the
      // member's own parameters are numbered after the enclosing ones), we must
      // still renumber those explicit template arguments down by the enclosing
      // parameter count now. Otherwise the deferred per-element expansion, which
      // runs against the member's own zero-based arguments, would look the pack
      // up at the wrong (enclosing-offset) index and fail to substitute it.
      for (size_t i = 0; i < id->template_arguments->length; i++) {
        RebaseTemplateArgumentParameterIndices(
            id->template_arguments->value.p[i],
            clone->rebase_template_parameter_base);
      }
    }
    if ((node->flags & kASTPackExpansion) != 0 &&
        id->symbol != NULL && id->symbol->flags.is_parameter_pack &&
        MapFindPointerKey(&clone->pack_symbol_map, id->symbol) != NULL) {
      return node;
    }
    Symbol* replacement = MapFindPointerKey(&clone->symbol_map, id->symbol);
    if (replacement != NULL) {
      id->symbol = replacement;
      ASTNodeSetType(node, TypeIsReference(replacement->type)
                               ? replacement->type->next
                               : replacement->type);
      node->value_category = kValueCategoryLvalue;
      return node;
    }
    replacement = CloneTemplateDependentTemporarySymbol(clone, id->symbol);
    if (replacement != NULL) {
      id->symbol = replacement;
      ASTNodeSetType(node, TypeIsReference(replacement->type)
                               ? replacement->type->next
                               : replacement->type);
      node->value_category = kValueCategoryLvalue;
      return node;
    }
    if (id->symbol != NULL && id->symbol->flags.is_template_parameter &&
        !id->symbol->flags.is_template_type_parameter &&
        id->symbol->template_parameter_index >= 0 &&
        (size_t)id->symbol->template_parameter_index < clone->args->length) {
      TemplateArgument* arg =
          clone->args->value.p[id->symbol->template_parameter_index];
      if (arg != NULL && arg->kind == kTemplateParameterNonType &&
          arg->template_parameter_index < 0) {
        return NewIntConstantASTNode(
            arg->int_value, NewTypeRecordWithSize(kTypeInt, kQualPlain),
            node->location);
      }
    }
    // Deferred reference to a member template's own template parameter (e.g.
    // `Target` inside a member function template of a class template): during
    // the lazy first-pass clone that bakes in the enclosing class arguments,
    // such a parameter is numbered after the enclosing ones and is not among
    // `clone->args`. It must be renumbered down by the enclosing parameter
    // count so the subsequent per-call instantiation (which supplies the
    // member's own zero-based arguments) can substitute it. Copy the symbol so
    // the shared template definition is not mutated.
    if (clone->rebase_template_parameter_base > 0 && id->symbol != NULL &&
        id->symbol->flags.is_template_parameter &&
        (id->symbol->template_parameter_index >=
             clone->rebase_template_parameter_base ||
         id->symbol->dependent_value_template_parameter_index >=
             clone->rebase_template_parameter_base)) {
      Symbol* old = id->symbol;
      TypeRecord* type = old->type != NULL ? TypeRecordCopy(old->type) : NULL;
      RebaseTemplateParameterIndices(type,
                                     clone->rebase_template_parameter_base);
      Symbol* copy = NewSymbol(old->name.value, type, old->storage);
      copy->namespace_ = old->namespace_;
      copy->flags = old->flags;
      copy->alignment = old->alignment;
      copy->template_parameter_index = old->template_parameter_index;
      if (copy->template_parameter_index >=
          clone->rebase_template_parameter_base) {
        copy->template_parameter_index -=
            clone->rebase_template_parameter_base;
      }
      copy->dependent_value_template_parameter_index =
          old->dependent_value_template_parameter_index;
      if (copy->dependent_value_template_parameter_index >=
          clone->rebase_template_parameter_base) {
        copy->dependent_value_template_parameter_index -=
            clone->rebase_template_parameter_base;
      }
      copy->location = old->location;
      copy->value = old->value;
      copy->stack_offset = old->stack_offset;
      copy->alias_target = old->alias_target;
      id->symbol = copy;
      ASTNodeSetType(node, copy->type);
      return node;
    }
    if (id->symbol != NULL && id->symbol->type != NULL &&
        TypeIsFunction(id->symbol->type) &&
        id->symbol->type->info.function.cxx_member_owner != NULL) {
      Struct* owner = id->symbol->type->info.function.cxx_member_owner;
      TypeRecord* owner_type =
          owner->tag_symbol != NULL ? owner->tag_symbol->type : NULL;
      if (owner_type != NULL &&
          (TypeContainsTemplateParameter(owner_type) ||
           StructContainsTemplateParameter(owner))) {
        TypeRecord* concrete_owner =
            SubstituteTemplateParameters(clone->parser, owner_type,
                                         clone->args);
        RebaseTemplateParameterIndices(concrete_owner,
                                       clone->rebase_template_parameter_base);
        if (concrete_owner != NULL && TypeIsStructOrUnion(concrete_owner) &&
            concrete_owner->info.struct_info != NULL) {
          StructMember* member =
              FindStructMember(concrete_owner->info.struct_info,
                               &id->symbol->name);
          if (member != NULL && member->is_static &&
              member->is_member_function && member->symbol != NULL) {
            id->symbol = member->symbol;
            ASTNodeSetType(node, member->symbol->type);
            TypeRecordDelete(concrete_owner);
            return node;
          }
        }
        TypeRecordDelete(concrete_owner);
      }
    }
  }
  bool node_type_substituted = false;
  if (node->op == AST_OP(cast)) {
    CastASTNode* cast = (CastASTNode*)node;
    // A cast whose type still names a pack (e.g. `static_cast<Ts&&>(args)...`)
    // must keep the pack-dependent cast type until ExpandClonedCallPackActuals
    // clones the pattern per element.  Substituting the whole pack here would
    // collapse `Ts` to a single element (or leave a broken type) and poison
    // every expanded copy that shares the cast_type pointer.
    int pack_index = -1;
    size_t pack_length = 0;
    bool pack_dependent_cast =
        FindPackExpansionInType(cast->cast_type, clone->args, &pack_index,
                                &pack_length) &&
        ASTNodeWithinPackExpansion(node);
    if (pack_dependent_cast) {
      // Leave cast_type / node->type pack-dependent for per-element expansion.
      node_type_substituted = true;
    } else if (TypeContainsTemplateParameter(cast->cast_type) ||
               (clone->from_owner != NULL && clone->to_owner != NULL &&
                clone->from_owner != clone->to_owner &&
                TypeChainReferencesStruct(cast->cast_type,
                                          clone->from_owner))) {
      TypeRecord* cast_type =
          SubstituteTemplateParameters(clone->parser, cast->cast_type,
                                       clone->args);
      RebaseTemplateParameterIndices(cast_type,
                                     clone->rebase_template_parameter_base);
      TypeRecordCalculateSize(cast_type);
      TypeRecordDelete(cast->cast_type);
      cast->cast_type = cast_type;
      if (TypeIsReference(cast_type)) {
        ASTNodeSetType(node, cast_type->next);
        node->value_category =
            cast_type->declarator == kDeclRValueReference
                ? kValueCategoryXvalue
                : kValueCategoryLvalue;
      } else {
        ASTNodeSetType(node, cast_type);
      }
      /* The cast block produced the authoritative result type and kept it in
       * sync with cast->cast_type. Re-substituting node->type below would both
       * be redundant and desync node->type from cast->cast_type (yielding a
       * separate, dangling pointee after later type-record churn). */
      node_type_substituted = true;
    }
  }
  if (node->type != NULL && !node_type_substituted) {
    TypeRecord* type =
        SubstituteTemplateParameters(clone->parser, node->type, clone->args);
    RebaseTemplateParameterIndices(type, clone->rebase_template_parameter_base);
    ASTNodeSetType(node, type);
  }
  if (node->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->left != NULL && call->left->type != NULL) {
      SetClonedCallReturnType(call, call->left->type);
    }
  }
  ASTNode* rewritten_new = RewriteClonedDependentNewInitializer(clone, node);
  if (rewritten_new != node) {
    // A dependent `new T(args)` initializer is rewritten into a constructor
    // call here.  When the initializer is a pack expansion, e.g.
    // `new T(std::forward<Args>(args)...)`, the freshly built call still holds
    // the unexpanded pack argument (with its per-element explicit template
    // arguments unsubstituted).  Route it through the ordinary call
    // pack-expansion + instantiation path below rather than returning
    // immediately, which would leave the pack unexpanded and fail to apply the
    // element template arguments to callees like `std::forward<Args>`.
    if (rewritten_new->op == AST_OP(call) &&
        CallActualsStillContainPackExpansion((VectorASTNode*)rewritten_new)) {
      node = rewritten_new;
    } else {
      return rewritten_new;
    }
  }
  bool expanded_call_actuals = ExpandClonedCallPackActuals(clone, node);
  InstantiateClonedFunctionTemplateCall(clone, node);
  if (expanded_call_actuals && node->op == AST_OP(call)) {
    ASTNodeVisit(node, ReplaceSingleElementPackIdentifierVisitor, 0, clone);
    node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
    ASTNodeSetType(node, NULL);
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->left != NULL) {
      call->left->flags &= ~kASTAnalyzed;
    }
    /* This clone callback runs bottom-up, so an enclosing declaration's symbol
     * type may not yet be substituted to its instantiated class. Re-analyzing a
     * member-access call here (e.g. a constructor call whose receiver is the
     * declared object) would resolve overloads against the not-yet-instantiated
     * generic class. Defer such calls to the post-clone reanalysis passes, which
     * run after all declaration types have been substituted. */
    bool is_member_access_call =
        call->left != NULL && (call->left->op == AST_OP(dot) ||
                               call->left->op == AST_OP(arrow));
    if (is_member_access_call) {
      node->flags |= kASTDependentFunctorCall;
      return node;
    }
    return AnalyzeExpression(node);
  }
  ExpandClonedBracedInitializerPackElements(clone, node);
  CloneTemplateLocalDeclarationSymbol(clone, node);
  RewriteClonedConstructorMemberCall(clone, node);
  return node;
}

/* A deferred member-access constructor call may still name the generic class
 * template's constructor (e.g. "Foo") even though its receiver is now the
 * instantiated class ("Foo<int>"), whose constructor member is named after the
 * instantiated tag. Rewrite the member-name string to the instantiated
 * constructor name so the member lookup succeeds during re-analysis. */
static void RewriteDeferredConstructorMemberName(VectorASTNode* call) {
  if (call->left == NULL ||
      (call->left->op != AST_OP(dot) && call->left->op != AST_OP(arrow))) {
    return;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)call->left;
  if (member_access->left == NULL || member_access->right == NULL ||
      member_access->right->op != AST_OP(string)) {
    return;
  }
  TypeRecord* receiver_type = member_access->left->type;
  if (receiver_type != NULL && TypeIsStructOrUnionPointer(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  const char* constructor_name = CXXConstructorNameForRecord(receiver_type);
  if (constructor_name == NULL || receiver_type->info.struct_info == NULL) {
    return;
  }
  ConstantASTNode* member_name = (ConstantASTNode*)member_access->right;
  if (member_name->value.string == NULL ||
      StringEqual(member_name->value.string, constructor_name)) {
    return;
  }
  if (FindStructMember(receiver_type->info.struct_info,
                       member_name->value.string) != NULL) {
    return;
  }
  if (receiver_type->template_origin == NULL ||
      strcmp(member_name->value.string->value,
             receiver_type->template_origin->name.value) != 0) {
    return;
  }
  StringSet(member_name->value.string, constructor_name);
}

static ASTNode* ReanalyzeClonedDependentFunctorCall(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)data;
  if ((node->flags & kASTDependentFunctorCall) == 0 ||
      node->op != AST_OP(call)) {
    return node;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  /* The cloned body may still reference the generic template's constructor
   * symbol. Rebind it to the instantiated class's constructor (derived from
   * the receiver) before re-analysis, while the receiver type is still intact.
   */
  RewriteDeferredConstructorMemberName(call);
  Vector overload_snapshots;
  VectorInit(&overload_snapshots);
  RebindClonedConstructorCall(call, &overload_snapshots);
  node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
  ASTNodeSetType(node, NULL);
  if (call->left != NULL) {
    call->left->flags &= ~kASTAnalyzed;
    ASTNodeSetType(call->left, NULL);
  }
  if (call->children != NULL) {
    for (size_t i = 0; i < call->children->length; i++) {
      ASTNode* actual = call->children->value.p[i];
      if (actual != NULL) {
        actual->flags &= ~kASTAnalyzed;
        ASTNodeSetType(actual, NULL);
      }
    }
  }
  *action = kASTTransformSkipChildren;
  ASTNode* parent = node->parent;
  ASTNode* analyzed = AnalyzeExpression(node);
  RestoreSymbolOverloadLinks(&overload_snapshots);
  if (parent != NULL && parent->op == AST_OP(call)) {
    parent->flags |= kASTDependentFunctorCall;
    parent->flags &= ~kASTAnalyzed;
  }
  return analyzed;
}

/* Saved overload-link state for one symbol so it can be restored after a
 * temporary rewiring (see SaveAndLinkStructMemberOverloadSymbols). */
typedef struct {
  Symbol* symbol;
  Symbol* overload_next;
  bool is_overloaded;
} SymbolOverloadLinkSnapshot;

/* Temporarily rebuild the symbol-level overload chain from a struct member's
 * overload list (member overloads link via StructMember, but overload
 * resolution walks Symbol->overload_next). Each touched symbol's prior link is
 * snapshotted into `snapshots` for later restoration. */
static void SaveAndLinkStructMemberOverloadSymbols(StructMember* first,
                                                   Vector* snapshots) {
  bool overloaded = first != NULL && first->overload_next != NULL;
  for (StructMember* member = first; member != NULL;
       member = member->overload_next) {
    if (member->symbol == NULL) {
      continue;
    }
    SymbolOverloadLinkSnapshot* snapshot = malloc(sizeof(*snapshot));
    snapshot->symbol = member->symbol;
    snapshot->overload_next = member->symbol->overload_next;
    snapshot->is_overloaded = member->symbol->flags.is_overloaded;
    VectorAppend(snapshots, snapshot);
    member->symbol->flags.is_overloaded = overloaded;
    StructMember* next = member->overload_next;
    while (next != NULL && next->symbol == NULL) {
      next = next->overload_next;
    }
    member->symbol->overload_next = next != NULL ? next->symbol : NULL;
  }
}

/* Restore the symbol overload links saved by SaveAndLinkStructMemberOverloadSymbols
 * and free the snapshot vector's contents. */
static void RestoreSymbolOverloadLinks(Vector* snapshots) {
  if (snapshots == NULL) {
    return;
  }
  for (size_t i = 0; i < snapshots->length; i++) {
    SymbolOverloadLinkSnapshot* snapshot = snapshots->value.p[i];
    if (snapshot == NULL || snapshot->symbol == NULL) {
      continue;
    }
    snapshot->symbol->overload_next = snapshot->overload_next;
    snapshot->symbol->flags.is_overloaded = snapshot->is_overloaded;
  }
  VectorDestructWithContents(snapshots, free, /*free_element=*/false);
}

/* Find the head of the overload chain for member `name` in `owner`, preferring
 * a member that actually has overloads linked; falls back to any match. */
static StructMember* FindStructMemberOverloadHead(Struct* owner, String* name) {
  StructMember* fallback = FindStructMember(owner, name);
  if (owner == NULL || name == NULL) {
    return fallback;
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL ||
        !StringEqual(&member->symbol->name, name->value)) {
      continue;
    }
    if (fallback == NULL) {
      fallback = member;
    }
    if (member->overload_next != NULL) {
      return member;
    }
  }
  return fallback;
}

/* If `call` is a constructor call whose callee still points at the generic
 * class template's constructor, rebind it to the instantiated class's
 * constructor overload set. The instantiated class is taken from the receiver
 * (this) argument when available, since that reflects the fully substituted
 * type. The overload links of the chosen set are temporarily made live and
 * must be restored by the caller via RestoreSymbolOverloadLinks. Returns true
 * if a rebind (and link) happened. */
static bool RebindClonedConstructorCall(VectorASTNode* call,
                                        Vector* overload_snapshots) {
  if (call == NULL || call->left == NULL ||
      call->left->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)call->left;
  if (id->symbol == NULL || id->symbol->type == NULL ||
      !TypeIsFunction(id->symbol->type) ||
      !id->symbol->type->info.function.is_constructor) {
    return false;
  }
  Struct* ctor_owner = NULL;
  String* ctor_name = NULL;
  if (call->children != NULL && call->children->length > 0) {
    ASTNode* receiver = call->children->value.p[0];
    TypeRecord* receiver_type = receiver != NULL ? receiver->type : NULL;
    /* Look through an address-of so this still works if the top-level
     * receiver type has already been cleared for re-analysis. */
    if (receiver_type == NULL && receiver != NULL &&
        receiver->op == AST_OP(address)) {
      UnaryASTNode* addr = (UnaryASTNode*)receiver;
      if (addr->sub != NULL && TypeIsStructOrUnion(addr->sub->type)) {
        ctor_owner = addr->sub->type->info.struct_info;
      }
    } else if (receiver_type != NULL &&
               TypeIsStructOrUnionPointer(receiver_type) &&
               receiver_type->next != NULL &&
               TypeIsStructOrUnion(receiver_type->next)) {
      ctor_owner = receiver_type->next->info.struct_info;
    }
    if (ctor_owner != NULL && ctor_owner->tag_name != NULL) {
      ctor_name = ctor_owner->tag_name;
    } else {
      ctor_owner = NULL;
    }
  }
  if (ctor_owner == NULL &&
      id->symbol->type->info.function.cxx_member_owner != NULL) {
    ctor_owner = id->symbol->type->info.function.cxx_member_owner;
    ctor_name = &id->symbol->name;
  }
  if (ctor_owner == NULL &&
      id->symbol->type->info.function.prototype.length > 0) {
    Symbol* this_formal = id->symbol->type->info.function.prototype.value.p[0];
    TypeRecord* this_type = this_formal != NULL ? this_formal->type : NULL;
    if (TypeIsPointer(this_type) && TypeIsStructOrUnion(this_type->next) &&
        this_type->next->info.struct_info != NULL &&
        this_type->next->info.struct_info->tag_name != NULL) {
      ctor_owner = this_type->next->info.struct_info;
      ctor_name = ctor_owner->tag_name;
    }
  }
  if (ctor_owner == NULL || ctor_name == NULL) {
    return false;
  }
  StructMember* member = FindStructMemberOverloadHead(ctor_owner, ctor_name);
  if (member == NULL || member->symbol == NULL) {
    return false;
  }
  SaveAndLinkStructMemberOverloadSymbols(member, overload_snapshots);
  id->symbol = member->symbol;
  ASTNodeSetType(call->left, id->symbol->type);
  return true;
}

/* Post-clone pass: re-resolve already-typed construction calls in the cloned
 * body. Constructor calls may still reference the generic template's overload
 * set, and typedef class functional casts (e.g. `alias(args)`) may still carry
 * a dependent alias type even after the enclosing class is concrete. Keep this
 * narrow so ordinary resolved calls are not disturbed. */
static ASTNode* ReanalyzeClonedResolvedCall(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  TemplateFunctionBodyClone* clone = data;
  if (node == NULL || node->op != AST_OP(call)) {
    return node;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(identifier)) {
    return node;
  }
  if (CallActualsStillContainPackExpansion(call)) {
    return node;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)call->left;
  bool is_typedef_class_construction =
      clone != NULL && id->template_arguments == NULL &&
      id->symbol != NULL && id->symbol->type != NULL &&
      StorageIs(id->symbol->storage, STO(typedef)) &&
      TypeIsStructOrUnion(id->symbol->type);
  if (is_typedef_class_construction) {
    TypeRecord* concrete_type = TypeRecordCopy(id->symbol->type);
    if (TypeContainsTemplateParameter(concrete_type)) {
      TypeRecordDelete(concrete_type);
      concrete_type =
          SubstituteTemplateParameters(clone->parser, id->symbol->type,
                                       clone->args);
      RebaseTemplateParameterIndices(concrete_type,
                                     clone->rebase_template_parameter_base);
    }
    if (!TypeContainsTemplateParameter(concrete_type)) {
      Symbol* concrete = NewSymbol(id->symbol->name.value, concrete_type,
                                   id->symbol->storage);
      concrete->flags = id->symbol->flags;
      concrete->location = id->symbol->location;
      concrete->alignment = id->symbol->alignment;
      concrete->namespace_ = id->symbol->namespace_;
      concrete->value = id->symbol->value;
      concrete->stack_offset = id->symbol->stack_offset;
      id->symbol = concrete;
      ASTNodeSetType(call->left, concrete->type);
      node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
      ASTNodeSetType(node, NULL);
      call->left->flags &= ~kASTAnalyzed;
      if (call->children != NULL) {
        for (size_t i = 0; i < call->children->length; i++) {
          ASTNode* actual = call->children->value.p[i];
          if (actual != NULL) {
            actual->flags &= ~kASTAnalyzed;
          }
        }
      }
      if (action != NULL) {
        *action = kASTTransformSkipChildren;
      }
      ASTNode* saved_parent = node->parent;
      int saved_child_id = node->child_id;
      node->parent = NULL;
      ASTNode* analyzed = AnalyzeExpression(node);
      node->parent = saved_parent;
      node->child_id = saved_child_id;
      return analyzed;
    }
    TypeRecordDelete(concrete_type);
  }
  bool is_constructor =
      id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.is_constructor;
  if (!is_constructor) {
    return node;
  }
  Vector overload_snapshots;
  VectorInit(&overload_snapshots);
  RebindClonedConstructorCall(call, &overload_snapshots);
  node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
  ASTNodeSetType(node, NULL);
  if (call->left != NULL) {
    call->left->flags &= ~kASTAnalyzed;
  }
  if (action != NULL) {
    *action = kASTTransformSkipChildren;
  }
  ASTNode* analyzed = AnalyzeExpression(node);
  RestoreSymbolOverloadLinks(&overload_snapshots);
  return analyzed;
}

/* Clone the body of function template `from` into the concrete instantiation
 * `to`, substituting template arguments `args`. First builds the original->
 * clone symbol map (mapping each generic formal to its instantiated formal, and
 * each parameter pack to the vector of its expanded formals), then clones the
 * AST via CloneTemplateFunctionBodyNode and runs the post-clone reanalysis
 * passes (pack-element rewrites, nested template-call instantiation, and
 * dependent/constructor call re-resolution). Returns the new body. */
static ASTNode* CloneTemplateFunctionBody(TypeParser* parser,
                                          TypeRecord* from,
                                          TypeRecord* to,
                                          Vector* args) {
  if (from == NULL || to == NULL || from->info.function.body == NULL) {
    return NULL;
  }
  TemplateFunctionBodyClone clone;
  MapInitForPointerKeys(&clone.symbol_map);
  MapInitForPointerKeys(&clone.pack_symbol_map);
  clone.parser = parser;
  clone.args = args;
  clone.to_func = to;
  clone.rebase_template_parameter_base =
      from->info.function.template_parameter_base;
  clone.from_owner = CloneFunctionMemberOwner(from);
  clone.to_owner = CloneFunctionMemberOwner(to);
  size_t to_index = 0;
  for (size_t i = 0; i < from->info.function.prototype.length; i++) {
    Symbol* from_formal = from->info.function.prototype.value.p[i];
    if (from_formal == NULL) {
      continue;
    }
    if (from_formal->flags.is_parameter_pack) {
      int pack_index = -1;
      size_t pack_length = 0;
      bool found_pack = FindPackExpansionInType(from_formal->type, args,
                                                &pack_index, &pack_length);
      TemplateArgument* pack =
          found_pack && pack_index >= 0 && (size_t)pack_index < args->length
              ? args->value.p[pack_index]
              : NULL;
      bool expandable = pack != NULL && pack->pack_arguments != NULL;
      if (!expandable) {
        // The pack's argument is not part of this clone's `args`, so it belongs
        // to an outer/nested template that is not being instantiated here (e.g.
        // a member function template's own pack while its enclosing class
        // template is instantiated).  Map it 1:1 to the corresponding still-pack
        // formal in `to` so the pack expansion in the body is preserved verbatim
        // for the later member-template instantiation instead of being expanded
        // to an empty sequence.
        if (to_index < to->info.function.prototype.length) {
          MapKeyValue kv;
          kv.key.p = from_formal;
          kv.value.p = to->info.function.prototype.value.p[to_index++];
          MapInsert(&clone.symbol_map, kv);
        }
        continue;
      }
      if (pack_length == 0) {
        pack_length = pack->pack_arguments->length;
      }
      Vector* replacements = NewVector();
      for (size_t j = 0;
           j < pack_length && to_index < to->info.function.prototype.length;
           j++) {
        VectorAppend(replacements,
                     to->info.function.prototype.value.p[to_index++]);
      }
      MapKeyValue kv;
      kv.key.p = from_formal;
      kv.value.p = replacements;
      MapInsert(&clone.pack_symbol_map, kv);
      continue;
    }
    if (to_index >= to->info.function.prototype.length) {
      break;
    }
    MapKeyValue kv;
    kv.key.p = from_formal;
    kv.value.p = to->info.function.prototype.value.p[to_index++];
    MapInsert(&clone.symbol_map, kv);
  }
  // Make self-type references inside the body (the injected-class-name used as a
  // local variable's type, a `static_cast<T&&>` target, etc.) resolve to *this*
  // instantiation rather than the generic primary template.  The deferred body
  // clone runs long after InstantiateTemplateMemberFunction restored these, so
  // re-establish the source->target struct mapping that
  // SubstituteTemplateParameters consults; otherwise such types stay generic and
  // member calls (e.g. the move ctor/assignment used by `swap`) target an
  // unemitted, generic-mangled symbol.
  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  Struct* saved_access_context = compiler->current_class_access_context;
  if (clone.from_owner != NULL && clone.to_owner != NULL) {
    parser->template_substitution_source = clone.from_owner;
    parser->template_substitution_target = clone.to_owner;
    compiler->current_class_access_context = clone.to_owner;
  }
  AddOwnerMemberSymbolMappings(&clone);
  ASTNode* body = ASTNodeClone(from->info.function.body,
                               CloneTemplateFunctionBodyNode, &clone, NULL);
  body = ASTNodeVisitAndTransform(body, ReanalyzeClonedDependentFunctorCall,
                                  NULL);
  ASTNodeVisit(body, ReplaceSingleElementPackIdentifierVisitor, 0, &clone);
  ASTNodeVisit(body, InstantiateClonedFunctionTemplateCallVisitor, 0, &clone);
  body = ASTNodeVisitAndTransform(body, ReanalyzeClonedDependentFunctorCall,
                                  NULL);
  body = ASTNodeVisitAndTransform(body, ReanalyzeClonedDependentFunctorCall,
                                  NULL);
  body = ASTNodeVisitAndTransform(body, ReanalyzeClonedResolvedCall, &clone);
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
  compiler->current_class_access_context = saved_access_context;
  MapDestructWithContents(&clone.pack_symbol_map, DeleteMappedVector);
  MapDestruct(&clone.symbol_map);
  return body;
}

/* True if a template instantiation with the given mangled asm name is already
 * queued for emission, so it is not instantiated/emitted twice. */
static bool PendingTemplateInstantiationHasAsmName(const char* asm_name) {
  if (asm_name == NULL || *asm_name == '\0') {
    return false;
  }
  for (size_t i = 0; i < compiler->pending_template_instantiations.length; i++) {
    DeclarationListASTNode* decls =
        (DeclarationListASTNode*)compiler->pending_template_instantiations.value.p[i];
    if (decls == NULL || decls->base.op != AST_OP(decl_list) ||
        decls->declarations == NULL) {
      continue;
    }
    for (size_t j = 0; j < decls->declarations->length; j++) {
      VariableDeclarationASTNode* decl =
          (VariableDeclarationASTNode*)decls->declarations->value.p[j];
      if (decl == NULL || decl->symbol == NULL) {
        continue;
      }
      const char* queued_name = decl->symbol->asm_name.value;
      if (queued_name != NULL && strcmp(queued_name, asm_name) == 0) {
        return true;
      }
    }
  }
  return false;
}

/* Instantiate the body of a member function template into `symbol` by cloning
 * `template_definition`'s body with `args`, mark it defined, set inline/weak
 * linkage as appropriate, and queue the instantiation for code emission (unless
 * it is still dependent or already queued). */
static void QueueTemplateMemberFunctionDefinitionImpl(Symbol* symbol,
                                                      Symbol* template_definition,
                                                      TypeParser* parser,
                                                      Vector* args,
                                                      bool allow_lazy) {
  if (symbol == NULL || symbol->type == NULL ||
      template_definition == NULL || template_definition->type == NULL ||
      template_definition->type->info.function.body == NULL) {
    return;
  }
  if (allow_lazy && !symbol->flags.is_template && args != NULL) {
    // A nested non-generic lambda inside a generic lambda / function template
    // still needs its body cloned against the rebuilt closure: capture field
    // types and dependent functor calls (e.g. `vis(x)` on a captured template
    // parameter) are only concrete after this substitution.  The plain lazy
    // path would keep the template-level body and mis-lower those calls.
    bool nested_lambda_operator =
        symbol->name.value != NULL &&
        strcmp(symbol->name.value, "operator()") == 0 &&
        symbol->type->info.function.cxx_member_owner != NULL &&
        symbol->type->info.function.cxx_member_owner->tag_symbol != NULL &&
        symbol->type->info.function.cxx_member_owner->tag_symbol->flags.invented;
    if (!nested_lambda_operator) {
      symbol->value.func_defn = template_definition;
      if (symbol->type->template_arguments == NULL) {
        symbol->type->template_arguments = TemplateArgumentVectorCopy(args);
      }
      return;
    }
  }
  if (PendingTemplateInstantiationHasAsmName(symbol->asm_name.value)) {
    symbol->type->info.function.definition = true;
    symbol->flags.is_defined = true;
    symbol->value.func_defn = symbol;
    return;
  }
  if (allow_lazy && symbol->flags.is_template && args != NULL) {
    symbol->type->info.function.body =
        CloneTemplateFunctionBody(parser, template_definition->type,
                                  symbol->type, args);
    symbol->type->info.function.definition = true;
    symbol->flags.is_defined = true;
    symbol->value.func_defn = symbol;
    return;
  }
  symbol->type->info.function.body =
      CloneTemplateFunctionBody(parser, template_definition->type,
                                symbol->type, args);
  symbol->type->info.function.definition = true;
  symbol->flags.is_defined = true;
  symbol->value.func_defn = symbol;
  if (symbol->type->info.function.is_inline) {
    symbol->flags.is_inline_defn = true;
    if (!StorageIs(symbol->storage, STO(static))) {
      symbol->flags.is_weak = true;
    }
  }
  if (TypeContainsTemplateParameter(symbol->type)) {
    return;
  }
  if (symbol->flags.is_template) {
    VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
    return;
  }
  Vector* declarations = NewVector();
  VectorAppend(declarations,
               NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
  VectorAppend(&compiler->pending_template_instantiations,
               NewDeclarationListASTNode(declarations, symbol->location));
  VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
}

void TypeEnsureTemplateMemberFunctionDefinition(Syntax* syntax, Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(symbol->type) ||
      symbol->flags.is_template ||
      symbol->type->info.function.cxx_member_owner == NULL ||
      symbol->type->info.function.body != NULL ||
      symbol->value.func_defn == NULL ||
      symbol->type->template_arguments == NULL) {
    return;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Symbol* template_definition = symbol->value.func_defn;
  Struct* saved_source = parser.template_substitution_source;
  Struct* saved_target = parser.template_substitution_target;
  TypeRecord* source_owner =
      template_definition->type != NULL &&
              template_definition->type->info.function.cxx_member_owner != NULL &&
              template_definition->type->info.function.cxx_member_owner
                      ->tag_symbol != NULL
          ? template_definition->type->info.function.cxx_member_owner
                ->tag_symbol->type
          : NULL;
  parser.template_substitution_source =
      source_owner != NULL && TypeIsStructOrUnion(source_owner)
          ? source_owner->info.struct_info
          : NULL;
  parser.template_substitution_target =
      symbol->type->info.function.cxx_member_owner;
  QueueTemplateMemberFunctionDefinitionImpl(
      symbol, template_definition, &parser, symbol->type->template_arguments,
      /*allow_lazy=*/false);
  parser.template_substitution_source = saved_source;
  parser.template_substitution_target = saved_target;
  TypeParserDestruct(&parser);
}

/* Build the concrete function type for a function-template instantiation:
 * copy all function attributes from the template type `from`, substitute the
 * return type and each parameter (expanding parameter packs) against `args`,
 * and renumber argument slots. (The body is cloned separately.) */
static TypeRecord* InstantiateFunctionTemplateType(TypeParser* parser,
                                                   TypeRecord* from,
                                                   Vector* args) {
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.varargs = from->info.function.varargs;
  func->info.function.unknown_args = from->info.function.unknown_args;
  func->info.function.definition = false;
  func->info.function.old_style = from->info.function.old_style;
  func->info.function.is_inline = from->info.function.is_inline;
  func->info.function.is_constexpr = from->info.function.is_constexpr;
  func->info.function.is_consteval = from->info.function.is_consteval;
  func->info.function.is_const_member = from->info.function.is_const_member;
  func->info.function.ref_qualifier = from->info.function.ref_qualifier;
  // Preserve constructor/destructor-ness so an instantiated constructor
  // template is still recognized as a constructor (its call is void-typed and
  // must not be treated as a copy-initialization of the object).
  func->info.function.is_constructor = from->info.function.is_constructor;
  func->info.function.is_destructor = from->info.function.is_destructor;
  func->info.function.is_explicit = from->info.function.is_explicit;
  func->info.function.is_explicit_conversion =
      from->info.function.is_explicit_conversion;
  func->info.function.explicit_condition = NULL;
  if (from->info.function.explicit_condition != NULL) {
    int64_t explicit_value = 0;
    if (TryFoldDependentTemplateArgument(
            parser, from->info.function.explicit_condition, args,
            &explicit_value)) {
      func->info.function.is_explicit = explicit_value != 0;
      func->info.function.is_explicit_conversion =
          from->info.function.is_explicit_conversion && explicit_value != 0;
    }
  }
  func->info.function.is_virtual = from->info.function.is_virtual;
  func->info.function.is_override = from->info.function.is_override;
  func->info.function.is_final = from->info.function.is_final;
  func->info.function.is_pure_virtual = from->info.function.is_pure_virtual;
  func->info.function.is_defaulted = from->info.function.is_defaulted;
  func->info.function.is_deleted = from->info.function.is_deleted;
  func->info.function.cxx_special_member_kind =
      from->info.function.cxx_special_member_kind;
  func->info.function.is_user_declared = from->info.function.is_user_declared;
  func->info.function.is_user_provided = from->info.function.is_user_provided;
  func->info.function.is_explicitly_defaulted =
      from->info.function.is_explicitly_defaulted;
  func->info.function.is_explicitly_deleted =
      from->info.function.is_explicitly_deleted;
  func->info.function.is_implicitly_declared =
      from->info.function.is_implicitly_declared;
  func->info.function.is_implicitly_deleted =
      from->info.function.is_implicitly_deleted;
  func->info.function.is_trivial_special_member =
      from->info.function.is_trivial_special_member;
  func->info.function.is_constexpr_eligible =
      from->info.function.is_constexpr_eligible;
  func->info.function.is_noexcept_eligible =
      from->info.function.is_noexcept_eligible;
  func->info.function.is_noexcept = from->info.function.is_noexcept;
  func->info.function.is_auto_return_deduced =
      from->info.function.is_auto_return_deduced;
  func->info.function.is_deduction_guide =
      from->info.function.is_deduction_guide;
  func->info.function.is_coroutine = from->info.function.is_coroutine;
  func->info.function.coroutine_promise_type =
      from->info.function.coroutine_promise_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_promise_type)
          : NULL;
  func->info.function.coroutine_frame_type =
      from->info.function.coroutine_frame_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_frame_type)
          : NULL;
  func->info.function.coroutine_suspend_count =
      from->info.function.coroutine_suspend_count;
  func->info.function.virtual_index = from->info.function.virtual_index;
  func->info.function.cxx_member_owner = from->info.function.cxx_member_owner;
  TypeRecord* return_type = SubstituteTemplateParameters(parser, from->next, args);
  TypeRecordChain(func, return_type);

  for (size_t i = 0; i < from->info.function.prototype.length; i++) {
    Symbol* formal = from->info.function.prototype.value.p[i];
    if (formal != NULL && formal->flags.is_parameter_pack) {
      AppendSubstitutedFormalParameter(parser, &func->info.function.prototype,
                                       formal, args, 0);
      continue;
    }
    TypeRecord* formal_type =
        SubstituteTemplateParameters(parser, formal->type, args);
    Symbol* clone = NewSymbol(formal->name.value, formal_type, formal->storage);
    clone->flags = formal->flags;
    clone->flags.is_argument = true;
    clone->location = formal->location;
    clone->default_argument =
        ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
    VectorAppend(&func->info.function.prototype, clone);
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    formal->value.arg_number = (int32_t)i;
  }
  return func;
}

/* Structural equality of two concrete template arguments (kind, pack contents,
 * type, or non-type value). Used to find an existing matching instantiation. */
static bool DependentTemplateArgExprEqual(ASTNode* a, ASTNode* b);

static bool TemplateArgumentEqual(TemplateArgument* left,
                                  TemplateArgument* right) {
  if (left == NULL || right == NULL || left->kind != right->kind) {
    return left == right;
  }
  if (left->pack_arguments != NULL || right->pack_arguments != NULL) {
    if (left->pack_arguments == NULL || right->pack_arguments == NULL ||
        left->pack_arguments->length != right->pack_arguments->length) {
      return false;
    }
    for (size_t i = 0; i < left->pack_arguments->length; i++) {
      if (!TemplateArgumentEqual(left->pack_arguments->value.p[i],
                                 right->pack_arguments->value.p[i])) {
        return false;
      }
    }
    return true;
  }
  if (left->kind == kTemplateParameterType) {
    return TypeEqual(left->type, right->type);
  }
  // Value-dependent non-type arguments (e.g. two `enable_if` SFINAE conditions)
  // are distinguished by comparing their stored expressions structurally, so
  // distinct overloads are not mistaken for redefinitions.
  if (left->dependent_expr != NULL || right->dependent_expr != NULL) {
    return DependentTemplateArgExprEqual(left->dependent_expr,
                                         right->dependent_expr);
  }
  return left->int_value == right->int_value &&
         left->template_parameter_index == right->template_parameter_index;
}

/* Element-wise equality of two concrete template argument vectors. */
static bool TemplateArgumentVectorEqual(Vector* left, Vector* right) {
  if (left == NULL || right == NULL || left->length != right->length) {
    return left == right;
  }
  for (size_t i = 0; i < left->length; i++) {
    if (!TemplateArgumentEqual(left->value.p[i], right->value.p[i])) {
      return false;
    }
  }
  return true;
}

bool TypeTemplateArgumentVectorEqual(Vector* left, Vector* right) {
  if (TemplateArgumentVectorEqual(left, right)) {
    return true;
  }
  if (left == NULL || right == NULL || left->length != right->length) {
    return false;
  }
  for (size_t i = 0; i < left->length; i++) {
    TemplateArgument* left_arg = left->value.p[i];
    TemplateArgument* right_arg = right->value.p[i];
    if (TemplateArgumentEqual(left_arg, right_arg)) {
      continue;
    }
    if (left_arg != NULL && left_arg->pack_arguments != NULL &&
        left_arg->pack_arguments->length == 1 &&
        TemplateArgumentEqual(left_arg->pack_arguments->value.p[0],
                              right_arg)) {
      continue;
    }
    if (right_arg != NULL && right_arg->pack_arguments != NULL &&
        right_arg->pack_arguments->length == 1 &&
        TemplateArgumentEqual(left_arg,
                              right_arg->pack_arguments->value.p[0])) {
      continue;
    }
    return false;
  }
  return true;
}

/* Structural equality of two value-dependent non-type template-argument
 * expressions (e.g. the conditions of two `enable_if` SFINAE overloads).  Only
 * the node shapes that appear in constant/SFINAE conditions are compared; any
 * other shape is conservatively treated as unequal so that distinct overloads
 * are never merged into one. */
static bool DependentTemplateArgExprEqual(ASTNode* a, ASTNode* b) {
  if (a == b) {
    return true;
  }
  if (a == NULL || b == NULL || a->op != b->op) {
    return false;
  }
  switch (a->op) {
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
      return ((ConstantASTNode*)a)->value.ivalue ==
             ((ConstantASTNode*)b)->value.ivalue;
    case AST_OP(identifier): {
      IdentifierASTNode* ia = (IdentifierASTNode*)a;
      IdentifierASTNode* ib = (IdentifierASTNode*)b;
      Symbol* sa = ia->symbol;
      Symbol* sb = ib->symbol;
      if (sa == sb) {
        return true;
      }
      if (sa == NULL || sb == NULL) {
        return false;
      }
      TypeRecord* ta = sa->type;
      TypeRecord* tb = sb->type;
      // Dependent qualified names (`Trait<Args>::member`): equal iff the same
      // scope template, the same member, and structurally-equal scope arguments.
      if (ta != NULL && tb != NULL && ta->dependent_member_name != NULL &&
          tb->dependent_member_name != NULL) {
        return ta->template_origin == tb->template_origin &&
               ta->template_parameter_index == tb->template_parameter_index &&
               StringEqual(ta->dependent_member_name,
                           tb->dependent_member_name->value) &&
               TemplateArgumentVectorEqual(ta->template_arguments,
                                           tb->template_arguments);
      }
      // Plain non-type template parameter references compare by index.
      if (sa->flags.is_template_parameter && sb->flags.is_template_parameter) {
        return sa->template_parameter_index == sb->template_parameter_index;
      }
      return false;
    }
    case AST_OP(not):
    case AST_OP(onescomp):
    case AST_OP(uminus):
    case AST_OP(uplus):
      return DependentTemplateArgExprEqual(((UnaryASTNode*)a)->sub,
                                           ((UnaryASTNode*)b)->sub);
    case AST_OP(plus):
    case AST_OP(minus):
    case AST_OP(mult):
    case AST_OP(div):
    case AST_OP(mod):
    case AST_OP(lshift):
    case AST_OP(rshifta):
    case AST_OP(rshiftl):
    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq):
    case AST_OP(equal):
    case AST_OP(noteq):
    case AST_OP(and):
    case AST_OP(bitor):
    case AST_OP(exor):
    case AST_OP(logand):
    case AST_OP(logor):
      return DependentTemplateArgExprEqual(((BinaryASTNode*)a)->left,
                                           ((BinaryASTNode*)b)->left) &&
             DependentTemplateArgExprEqual(((BinaryASTNode*)a)->right,
                                           ((BinaryASTNode*)b)->right);
    case AST_OP(cast): {
      CastASTNode* ca = (CastASTNode*)a;
      CastASTNode* cb = (CastASTNode*)b;
      return TypeEqual(ca->cast_type, cb->cast_type) &&
             DependentTemplateArgExprEqual(ca->expr, cb->expr);
    }
    default:
      return false;
  }
}

/* Equality of two type *patterns* (types that may still mention template
 * parameters), comparing parameter indices structurally rather than resolving
 * them. Used to compare partial-specialization / template signatures. */
static bool TemplateTypePatternEqual(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL || left->declarator != right->declarator ||
      left->qualifiers != right->qualifiers) {
    return left == right;
  }
  if (left->template_parameter_index >= 0 ||
      right->template_parameter_index >= 0) {
    return left->template_parameter_index == right->template_parameter_index;
  }
  if (left->type != right->type) {
    return false;
  }
  if (TypeIsStructOrUnion(left)) {
    if (left->info.struct_info != right->info.struct_info) {
      return false;
    }
  } else if (TypeIsEnum(left)) {
    if (left->info.enum_info != right->info.enum_info) {
      return false;
    }
  }
  if (!TemplateArgumentPatternVectorEqual(left->template_arguments,
                                          right->template_arguments)) {
    return false;
  }
  switch (left->declarator) {
    case kDeclArray:
      if (left->info.array.template_parameter_index !=
          right->info.array.template_parameter_index) {
        return false;
      }
      if (left->info.array.template_parameter_index < 0 &&
          left->info.array.size.fixed != right->info.array.size.fixed) {
        return false;
      }
      return TemplateTypePatternEqual(left->next, right->next);
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return TemplateTypePatternEqual(left->next, right->next);
    case kDeclFunction:
      if (!TemplateTypePatternEqual(left->next, right->next) ||
          left->info.function.prototype.length !=
              right->info.function.prototype.length ||
          left->info.function.is_const_member !=
              right->info.function.is_const_member ||
          left->info.function.ref_qualifier !=
              right->info.function.ref_qualifier) {
        return false;
      }
      for (size_t i = 0; i < left->info.function.prototype.length; i++) {
        Symbol* left_formal = left->info.function.prototype.value.p[i];
        Symbol* right_formal = right->info.function.prototype.value.p[i];
        if (left_formal == NULL || right_formal == NULL ||
            !TemplateTypePatternEqual(left_formal->type, right_formal->type)) {
          return left_formal == right_formal;
        }
      }
      return true;
    case kDeclPrimitive:
      return true;
  }
}

/* Equality of two template argument *patterns* (arguments that may still be
 * parameter-dependent), comparing parameter indices structurally. */
static bool TemplateArgumentPatternEqual(TemplateArgument* left,
                                         TemplateArgument* right) {
  if (left == NULL || right == NULL || left->kind != right->kind ||
      left->is_pack_expansion != right->is_pack_expansion) {
    return left == right;
  }
  if (left->pack_arguments != NULL || right->pack_arguments != NULL) {
    if (left->pack_arguments == NULL || right->pack_arguments == NULL ||
        left->pack_arguments->length != right->pack_arguments->length) {
      return false;
    }
    return TemplateArgumentPatternVectorEqual(left->pack_arguments,
                                             right->pack_arguments);
  }
  if (left->kind == kTemplateParameterType) {
    return TemplateTypePatternEqual(left->type, right->type);
  }
  return left->int_value == right->int_value &&
         left->template_parameter_index == right->template_parameter_index;
}

/* Element-wise equality of two template argument pattern vectors. */
static bool TemplateArgumentPatternVectorEqual(Vector* left, Vector* right) {
  if (left == NULL || right == NULL || left->length != right->length) {
    return left == right;
  }
  for (size_t i = 0; i < left->length; i++) {
    if (!TemplateArgumentPatternEqual(left->value.p[i], right->value.p[i])) {
      return false;
    }
  }
  return true;
}

/* True when a cached instantiation `candidate` corresponds to the freshly
 * requested instantiation `type` with the same template `args`. The template
 * arguments already uniquely identify an instantiation, but the cheap identity
 * check compares the whole function type. That fails for a function with a
 * deduced (`auto`) return: the return type is filled in lazily by analyzing the
 * body *after* the instantiation is cached, so a later request for the same
 * instantiation arrives with an as-yet-undeduced `auto` return and would not
 * TypeEqual the cached, now-deduced instantiation. Accept a match that differs
 * only in an auto-deduced return; otherwise a duplicate instantiation is
 * created whose body is never cloned (its asm name is already pending), leaving
 * its return type unresolved. */
static bool FunctionTemplateInstantiationMatches(Symbol* candidate,
                                                 TypeRecord* type,
                                                 Vector* args) {
  if (candidate == NULL || candidate->flags.is_template ||
      candidate->type == NULL || !TypeIsFunction(candidate->type) ||
      !TemplateArgumentVectorEqual(candidate->type->template_arguments, args)) {
    return false;
  }
  if (TypeEqual(candidate->type, type)) {
    return true;
  }
  return TypeIsFunction(type) &&
         (candidate->type->info.function.is_auto_return_deduced ||
          TypeContainsAuto(candidate->type->next) ||
          TypeContainsAuto(type->next));
}

/* Search a function template's instantiation overload chain for one whose type
 * and template arguments match (cache lookup), or NULL. */
static Symbol* FindFunctionTemplateInstantiation(Symbol* templ,
                                                 TypeRecord* type,
                                                 Vector* args) {
  if (templ == NULL || templ->type == NULL || !TypeIsFunction(templ->type)) {
    return NULL;
  }
  for (Symbol* candidate = templ->overload_next; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->type != NULL &&
        candidate->type->info.function.template_origin == templ &&
        FunctionTemplateInstantiationMatches(candidate, type, args)) {
      return candidate;
    }
  }
  Vector* cache = &templ->type->info.function.template_instantiations;
  for (size_t i = 0; i < cache->length; i++) {
    Symbol* candidate = cache->value.p[i];
    if (FunctionTemplateInstantiationMatches(candidate, type, args)) {
      return candidate;
    }
  }
  return NULL;
}

static Symbol* FindFunctionTemplateInstantiationByAsmName(Symbol* templ,
                                                          const char* asm_name) {
  if (asm_name == NULL || *asm_name == '\0') {
    return NULL;
  }
  for (Symbol* candidate = templ; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->flags.is_template && candidate->asm_name.value != NULL &&
        strcmp(candidate->asm_name.value, asm_name) == 0) {
      return candidate;
    }
  }
  // Instantiations are recorded in the template's `template_instantiations`
  // cache (not the overload chain), so they must be searched here too.  The
  // mangled name is the definitive ABI identity of an instantiation: two
  // requests that mangle identically denote the same function even when their
  // deduced signatures fail a structural `TypeEqual` (e.g. a `*this` self-type
  // that lost its template_origin metadata), so reuse the cached one to avoid
  // emitting a duplicate symbol.
  if (templ != NULL && templ->type != NULL && TypeIsFunction(templ->type)) {
    Vector* cache = &templ->type->info.function.template_instantiations;
    for (size_t i = 0; i < cache->length; i++) {
      Symbol* candidate = cache->value.p[i];
      if (candidate != NULL && !candidate->flags.is_template &&
          candidate->asm_name.value != NULL &&
          strcmp(candidate->asm_name.value, asm_name) == 0) {
        return candidate;
      }
    }
  }
  return NULL;
}

/* Append a newly created instantiation to the template's overload chain so it
 * can be found later (and marks both ends as overloaded). */
static void AppendFunctionTemplateInstantiation(Symbol* templ,
                                                Symbol* instantiated) {
  if (templ == NULL || templ->type == NULL || !TypeIsFunction(templ->type) ||
      instantiated == NULL) {
    return;
  }
  instantiated->overload_next = NULL;
  instantiated->flags.is_overloaded = false;
  VectorAppend(&templ->type->info.function.template_instantiations,
               instantiated);
}

/* True if a template argument is still dependent and therefore the
 * instantiation it belongs to cannot be fully realized yet. Differs from
 * TemplateArgumentContainsTemplateParameter in that it inspects every pack
 * element regardless of kind. */
static bool TemplateArgumentContainsTemplateParameterForInstantiation(
    TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(arg->type) ||
      DependentExpressionContainsTemplateParameter(arg->dependent_expr)) {
    return true;
  }
  for (size_t i = 0; arg->pack_arguments != NULL &&
                     i < arg->pack_arguments->length; i++) {
    if (TemplateArgumentContainsTemplateParameterForInstantiation(
            arg->pack_arguments->value.p[i])) {
      return true;
    }
  }
  return false;
}

/* True if any argument in the vector is still dependent (see above). */
static bool TemplateArgumentVectorContainsTemplateParameterForInstantiation(
    Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    if (TemplateArgumentContainsTemplateParameterForInstantiation(
            args->value.p[i])) {
      return true;
    }
  }
  return false;
}

/* Instantiate a function template `templ` with explicit/deduced arguments
 * `args`. Completes defaulted/deduced arguments, builds the concrete function
 * type and body (caching to avoid duplicate instantiations), queues it for
 * emission, and returns the instantiation symbol. Returns `templ` unchanged if
 * arguments are still dependent or the definition is unavailable. */
static Symbol* InstantiateSimpleFunctionTemplate(TypeParser* parser,
                                                 Symbol* templ,
                                                 Vector* args) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type)) {
    return templ;
  }
  if (templ->type->info.function.template_origin != NULL) {
    templ = templ->type->info.function.template_origin;
  }
  Vector* completed_args =
      CompleteFunctionTemplateArguments(parser, templ->type, args,
                                        /*emit_error=*/true);
  if (completed_args == NULL) {
    return templ;
  }
  if (TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }

  Symbol* template_definition = templ;
  if ((template_definition->type == NULL ||
       template_definition->type->info.function.body == NULL) &&
      templ->value.func_defn != NULL &&
      templ->value.func_defn->type != NULL &&
      templ->value.func_defn->type->info.function.body != NULL) {
    template_definition = templ->value.func_defn;
  }
  if (template_definition->type->info.function.body == NULL) {
    SyntaxError(parser->syntax,
                "Function template definition is required for instantiation");
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }
  bool saved_substitution_failed = parser->template_substitution_failed;
  parser->template_substitution_failed = false;
  TypeRecord* func =
      InstantiateFunctionTemplateType(parser, templ->type,
                                      completed_args);
  bool substitution_failed = parser->template_substitution_failed;
  parser->template_substitution_failed = saved_substitution_failed;
  if (substitution_failed) {
    // A dependent type in the signature (e.g. an `enable_if` SFINAE guard) had
    // no valid substitution.  Abandon this instantiation quietly so overload
    // resolution can discard the candidate; do not create or queue a symbol.
    TypeRecordDelete(func);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }
  if (TypeContainsTemplateParameter(func)) {
    SyntaxError(parser->syntax,
                "Function template instantiation is not supported yet");
    TypeRecordDelete(func);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }

  Symbol* existing = FindFunctionTemplateInstantiation(templ, func,
                                                       completed_args);
  if (existing != NULL) {
    TypeRecordDelete(func);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return existing;
  }

  Symbol* symbol = NewSymbol(templ->name.value, func, templ->storage);
  symbol->location = templ->location;
  symbol->namespace_ = templ->namespace_;
  func->info.function.symbol = symbol;
  func->info.function.template_origin = templ;
  func->template_arguments = TemplateArgumentVectorCopy(completed_args);
  SymbolSetCXXMangledAsmName(symbol);
  existing = FindFunctionTemplateInstantiationByAsmName(templ,
                                                       symbol->asm_name.value);
  if (existing != NULL) {
    SymbolDelete(symbol);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return existing;
  }
  if (PendingTemplateInstantiationHasAsmName(symbol->asm_name.value)) {
    symbol->type->info.function.definition = true;
    symbol->flags.is_defined = true;
    AppendFunctionTemplateInstantiation(templ, symbol);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return symbol;
  }
  VectorDestruct(&symbol->attributes);
  AttributeListClone(&symbol->attributes, &templ->attributes);
  if (template_definition->type->info.function.body != NULL) {
    symbol->type->info.function.body =
        CloneTemplateFunctionBody(parser, template_definition->type,
                                  symbol->type, completed_args);
    symbol->type->info.function.definition = true;
    symbol->flags.is_defined = true;
    if (symbol->type->info.function.is_inline) {
      symbol->flags.is_inline_defn = true;
      if (!StorageIs(symbol->storage, STO(static))) {
        symbol->flags.is_weak = true;
      }
    }
    Vector* declarations = NewVector();
    VectorAppend(declarations,
                 NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
    VectorAppend(&compiler->pending_template_instantiations,
                 NewDeclarationListASTNode(declarations, symbol->location));
    VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
  }
  AppendFunctionTemplateInstantiation(templ, symbol);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return symbol;
}

/* Create a type template argument holding a copy of a deduced type. */
static TemplateArgument* NewDeducedTypeTemplateArgument(TypeRecord* type) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = TypeRecordCopy(type);
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

/* Create a non-type template argument holding a deduced integer value. */
static TemplateArgument* NewDeducedNonTypeTemplateArgument(long long value) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = kTemplateParameterNonType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = value;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

/* The deduced type for a parameter `T` from an argument: a top-level cv-stripped
 * copy of the actual type (cv-qualifiers are not deduced into a bare `T`). */
static TypeRecord* FunctionTemplateDeductionActualType(TypeRecord* actual) {
  if (actual == NULL) {
    return NULL;
  }
  TypeRecord* deduced = TypeRecordCopy(actual);
  deduced->qualifiers = kQualPlain;
  return deduced;
}

/* Record a deduced type for template parameter `index`. Explicitly supplied
 * arguments (index < explicit_arg_count) are kept as-is; otherwise set the
 * deduced type, or verify consistency if it was already deduced elsewhere. */
static bool SetDeducedFunctionTemplateTypeArgument(Vector* args,
                                                   size_t explicit_arg_count,
                                                   int index,
                                                   TypeRecord* actual) {
  if (index < 0 || (size_t)index >= args->length) {
    return false;
  }
  if ((size_t)index < explicit_arg_count) {
    return true;
  }

  TypeRecord* deduced = FunctionTemplateDeductionActualType(actual);
  TemplateArgument* existing = args->value.p[index];
  if (existing == NULL) {
    args->value.p[index] = NewDeducedTypeTemplateArgument(deduced);
    TypeRecordDelete(deduced);
    return true;
  }
  bool same = existing->kind == kTemplateParameterType &&
              existing->type != NULL &&
              TypeEqual(existing->type, deduced);
  TypeRecordDelete(deduced);
  return same;
}

static bool DeduceFunctionTemplateStructMembers(Vector* args,
                                                size_t explicit_arg_count,
                                                TypeRecord* formal,
                                                TypeRecord* actual);
static bool DeduceFunctionTemplateTypeArgument(Vector* args,
                                               size_t explicit_arg_count,
                                               TypeRecord* formal,
                                               TypeRecord* actual);

/* Deduce one element of a parameter pack: run normal type deduction for the
 * `formal`/`actual` pair into a scratch copy of `args` (with the pack slot
 * cleared), then append the resulting element type to the pack and merge any
 * other parameters deduced as a side effect, checking consistency. */
static bool AppendDeducedFunctionTemplatePackElement(
    Vector* args, size_t explicit_arg_count, int pack_index, TypeRecord* formal,
    TypeRecord* actual) {
  if (pack_index < 0 || args == NULL || (size_t)pack_index >= args->length) {
    return false;
  }
  TemplateArgument* pack = args->value.p[pack_index];
  if (pack == NULL) {
    pack = NewEmptyPackTemplateArgument(kTemplateParameterType);
    args->value.p[pack_index] = pack;
  }
  if (pack->kind != kTemplateParameterType || pack->pack_arguments == NULL) {
    return false;
  }

  Vector* element_args = TemplateArgumentVectorCopy(args);
  TemplateArgumentDelete(element_args->value.p[pack_index]);
  element_args->value.p[pack_index] = NULL;
  bool ok = DeduceFunctionTemplateTypeArgument(element_args, explicit_arg_count,
                                               formal, actual);
  TemplateArgument* element = ok ? element_args->value.p[pack_index] : NULL;
  ok = ok && element != NULL && element->kind == kTemplateParameterType &&
       element->pack_arguments == NULL;
  if (ok) {
    for (size_t i = 0; i < element_args->length; i++) {
      if (i == (size_t)pack_index) {
        continue;
      }
      TemplateArgument* deduced = element_args->value.p[i];
      TemplateArgument* existing = args->value.p[i];
      if (deduced == NULL) {
        continue;
      }
      if (existing == NULL) {
        args->value.p[i] = TemplateArgumentCopy(deduced);
      } else if (!TemplateArgumentEqual(existing, deduced)) {
        ok = false;
        break;
      }
    }
  }
  if (ok) {
    VectorAppend(pack->pack_arguments, TemplateArgumentCopy(element));
  }
  VectorDeleteWithContents(element_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return ok;
}

/* [temp.deduct.call]: when a function parameter is not a reference, an array
 * argument is replaced by the array-to-pointer result and a function argument
 * by the function-to-pointer result before deduction.  Returns a freshly
 * allocated decayed type with one reference held (release it with
 * TypeRecordDelete), or NULL when no decay applies and the caller should deduce
 * against the argument's own type.  Top-level cv-qualifiers on the decayed
 * pointer are dropped, as the standard requires. */
static TypeRecord* DecayCallArgumentTypeForDeduction(TypeRecord* formal,
                                                     TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return NULL;
  }
  // Only a by-value bare template parameter (`T first`) triggers decay: that is
  // the case whose deduced argument must become a pointer.  When the parameter
  // is a pointer, array, or reference the existing structural matching already
  // deduces correctly (e.g. `T*` vs an array recurses element-wise, and an
  // array parameter `char[N]` in aggregate CTAD must keep matching structurally
  // to deduce N), so decaying there would wrongly defeat deduction.
  bool bare_template_parameter = formal->declarator == kDeclPrimitive &&
                                 TypeIsUnknown(formal) &&
                                 formal->template_parameter_index >= 0;
  if (!bare_template_parameter) {
    return NULL;
  }
  TypeRecord* decayed = NULL;
  if (actual->declarator == kDeclArray) {
    decayed = NewPointerTo(kQualPlain, TypeRecordCopy(actual->next));
  } else if (actual->declarator == kDeclFunction) {
    decayed = NewPointerTo(kQualPlain, TypeRecordCopy(actual));
  }
  if (decayed != NULL) {
    TypeRecordCalculateSize(decayed);
    TypeRecordIncRef(decayed);
  }
  return decayed;
}

/* Deduce a pack element from a call argument expression, applying forwarding-
 * reference rules: a `T&&` pack parameter binding an lvalue deduces `T&`
 * (reference collapsing); otherwise deduce from the argument's type. */
static bool DeduceFunctionTemplatePackCallArgument(
    Vector* args, size_t explicit_arg_count, int pack_index, TypeRecord* formal,
    ASTNode* actual) {
  if (formal == NULL || actual == NULL || actual->type == NULL) {
    return false;
  }
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (TypeIsCXXInitializerList(target) && actual->op != AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(TypeIsReference(actual->type)
                                    ? actual->type->next
                                    : actual->type)) {
    return false;
  }
  if (actual->op == AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(target) &&
      (target == NULL || target->declarator != kDeclArray)) {
    return false;
  }
  int placeholder_index = -1;
  if (formal->declarator == kDeclRValueReference &&
      TypeIsTemplateParameterPlaceholder(formal->next, &placeholder_index) &&
      actual->value_category == kValueCategoryLvalue) {
    TypeRecord* lvalue_ref = NewReferenceTypeRecord(kQualPlain, false);
    TypeRecordChain(lvalue_ref, actual->type);
    lvalue_ref->type = actual->type->type;
    TypeRecordCalculateSize(lvalue_ref);
    if (placeholder_index == pack_index) {
      TemplateArgument* pack = args->value.p[pack_index];
      if (pack == NULL) {
        pack = NewEmptyPackTemplateArgument(kTemplateParameterType);
        args->value.p[pack_index] = pack;
      }
      bool ok = pack->kind == kTemplateParameterType &&
                pack->pack_arguments != NULL;
      if (ok) {
        VectorAppend(pack->pack_arguments,
                     NewDeducedTypeTemplateArgument(lvalue_ref));
      }
      TypeRecordDelete(lvalue_ref);
      return ok;
    }
    bool ok = AppendDeducedFunctionTemplatePackElement(
        args, explicit_arg_count, pack_index, formal, lvalue_ref);
    TypeRecordDelete(lvalue_ref);
    return ok;
  }
  TypeRecord* decayed = DecayCallArgumentTypeForDeduction(formal, actual->type);
  if (decayed != NULL) {
    bool ok = AppendDeducedFunctionTemplatePackElement(
        args, explicit_arg_count, pack_index, formal, decayed);
    TypeRecordDelete(decayed);
    return ok;
  }
  return AppendDeducedFunctionTemplatePackElement(
      args, explicit_arg_count, pack_index, formal, actual->type);
}

/* Record a deduced non-type (integer) argument for parameter `index`, keeping
 * explicit args, or verifying consistency with a prior deduction. */
static bool SetDeducedFunctionTemplateNonTypeArgument(Vector* args,
                                                      size_t explicit_arg_count,
                                                      int index,
                                                      long long value) {
  if (index < 0 || (size_t)index >= args->length) {
    return false;
  }
  if ((size_t)index < explicit_arg_count) {
    return true;
  }
  TemplateArgument* existing = args->value.p[index];
  if (existing == NULL) {
    args->value.p[index] = NewDeducedNonTypeTemplateArgument(value);
    return true;
  }
  return existing->kind == kTemplateParameterNonType &&
         existing->int_value == value;
}

/* Unwrap a braced-initializer element to its underlying expression. */
static ASTNode* CXXBracedInitializerElementExpression(ASTNode* init) {
  if (init == NULL) {
    return NULL;
  }
  if (init->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)init;
    return expr_init->expr;
  }
  return init;
}

/* Deduce template arguments when a `T[N]`/nested-array parameter is matched
 * against a braced initializer: deduce the bound `N` from the element count and
 * the element type from each initializer (recursing for nested braces). */
static bool DeduceFunctionTemplateArrayInitializerArgument(
    Vector* args, size_t explicit_arg_count, TypeRecord* formal,
    ASTNode* actual) {
  if (actual == NULL || actual->op != AST_OP(braced_init) || formal == NULL ||
      formal->declarator != kDeclArray) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  if (formal->info.array.template_parameter_index >= 0 &&
      !SetDeducedFunctionTemplateNonTypeArgument(
          args, explicit_arg_count, formal->info.array.template_parameter_index,
          (long long)braced->initializers->length)) {
    return false;
  }
  if (formal->info.array.template_parameter_index < 0 &&
      !formal->info.array.is_flexible && !formal->info.array.is_vla &&
      formal->info.array.size.fixed < (int)braced->initializers->length) {
    return false;
  }
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init == NULL) {
      return false;
    }
    if (init->op == AST_OP(braced_init)) {
      if (!DeduceFunctionTemplateArrayInitializerArgument(
              args, explicit_arg_count, formal->next, init)) {
        return false;
      }
      continue;
    }
    ASTNode* element = CXXBracedInitializerElementExpression(init);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                            formal->next, element->type)) {
      return false;
    }
  }
  return true;
}

/* Deduce template arguments when a `std::initializer_list<T>` parameter is
 * matched against a braced initializer: deduce `T` from each element. */
static bool DeduceFunctionTemplateInitializerListArgument(
    Vector* args, size_t explicit_arg_count, TypeRecord* formal,
    ASTNode* actual) {
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (actual == NULL || actual->op != AST_OP(braced_init) ||
      !TypeIsCXXInitializerList(target)) {
    return false;
  }
  TypeRecord* element_type = TypeCXXInitializerListElement(target);
  if (element_type == NULL) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* element =
        CXXBracedInitializerElementExpression(braced->initializers->value.p[i]);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                            element_type, element->type)) {
      return false;
    }
  }
  return true;
}

/* Deduce template arguments when both formal and actual are instantiations of
 * the same class template (e.g. formal `Wrapper<T>` vs actual `Wrapper<int>`):
 * match them argument-by-argument, recursing into type arguments. */
/* Match one non-pack formal class-template argument against one actual argument,
 * deducing any function-template parameters it mentions into `args`. */
static bool DeduceFunctionTemplateOneTemplateArgument(Vector* args,
                                                      size_t explicit_arg_count,
                                                      TemplateArgument* formal_arg,
                                                      TemplateArgument* actual_arg) {
  if (formal_arg == NULL || actual_arg == NULL ||
      formal_arg->kind != actual_arg->kind) {
    return false;
  }
  if (formal_arg->kind == kTemplateParameterType) {
    return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                              formal_arg->type,
                                              actual_arg->type);
  }
  if (formal_arg->template_parameter_index >= 0) {
    return SetDeducedFunctionTemplateNonTypeArgument(
        args, explicit_arg_count, formal_arg->template_parameter_index,
        actual_arg->int_value);
  }
  return formal_arg->int_value == actual_arg->int_value;
}

static bool DeduceFunctionTemplateTemplateArguments(Vector* args,
                                                    size_t explicit_arg_count,
                                                    TypeRecord* formal,
                                                    TypeRecord* actual) {
  if (formal == NULL || actual == NULL ||
      formal->template_origin == NULL ||
      formal->template_origin != actual->template_origin ||
      formal->template_arguments == NULL ||
      actual->template_arguments == NULL) {
    return false;
  }
  Vector* formal_args = formal->template_arguments;
  Vector* actual_args = actual->template_arguments;
  size_t actual_index = 0;
  for (size_t i = 0; i < formal_args->length; i++) {
    TemplateArgument* formal_arg = formal_args->value.p[i];
    if (formal_arg == NULL) {
      return false;
    }
    // A formal parameter-pack argument (e.g. `Wrapper<Types...>` matched against
    // `Wrapper<int, char>`): absorb the actual arguments that line up with it,
    // leaving enough for any fixed formal arguments that follow the pack.  Each
    // absorbed actual is deduced as one element of the enclosing function
    // template's pack parameter.
    int pack_index = -1;
    if (formal_arg->is_pack_expansion &&
        formal_arg->kind == kTemplateParameterType &&
        formal_arg->type != NULL &&
        TypeIsTemplateParameterPlaceholder(formal_arg->type, &pack_index) &&
        pack_index >= 0) {
      size_t trailing_formals = formal_args->length - i - 1;
      if (actual_args->length < actual_index + trailing_formals) {
        return false;
      }
      size_t pack_end = actual_args->length - trailing_formals;
      // [temp.deduct]: the same template parameter pack may be named in more
      // than one function parameter, e.g.
      //   operator==(const variant<Types...>&, const variant<Types...>&).
      // Each occurrence must deduce a *consistent* sequence for `Types...`, not
      // concatenate onto it.  `AppendDeducedFunctionTemplatePackElement` only
      // ever appends, so remember how many elements a previous parameter
      // already deduced; if the pack is non-empty here we compare the freshly
      // deduced tail against that prefix and collapse the duplicate instead of
      // doubling the pack.
      TemplateArgument* pack_prev = args->value.p[pack_index];
      size_t pack_prefix =
          (pack_prev != NULL && pack_prev->pack_arguments != NULL)
              ? pack_prev->pack_arguments->length
              : 0;
      for (; actual_index < pack_end; actual_index++) {
        TemplateArgument* actual_arg = actual_args->value.p[actual_index];
        if (actual_arg == NULL) {
          return false;
        }
        // The actual may itself be an already-bundled pack; splice its elements.
        if (actual_arg->pack_arguments != NULL) {
          for (size_t j = 0; j < actual_arg->pack_arguments->length; j++) {
            TemplateArgument* element = actual_arg->pack_arguments->value.p[j];
            if (element == NULL || element->kind != kTemplateParameterType ||
                element->type == NULL ||
                !AppendDeducedFunctionTemplatePackElement(
                    args, explicit_arg_count, pack_index, formal_arg->type,
                    element->type)) {
              return false;
            }
          }
          continue;
        }
        if (actual_arg->kind != kTemplateParameterType ||
            actual_arg->type == NULL ||
            !AppendDeducedFunctionTemplatePackElement(
                args, explicit_arg_count, pack_index, formal_arg->type,
                actual_arg->type)) {
          return false;
        }
      }
      if (pack_prefix > 0) {
        TemplateArgument* pack_now = args->value.p[pack_index];
        if (pack_now == NULL || pack_now->pack_arguments == NULL) {
          return false;
        }
        size_t total = pack_now->pack_arguments->length;
        size_t appended = total - pack_prefix;
        if (appended != pack_prefix) {
          return false;
        }
        for (size_t k = 0; k < appended; k++) {
          if (!TemplateArgumentEqual(
                  pack_now->pack_arguments->value.p[k],
                  pack_now->pack_arguments->value.p[pack_prefix + k])) {
            return false;
          }
        }
        for (size_t k = total; k > pack_prefix; k--) {
          TemplateArgumentDelete(pack_now->pack_arguments->value.p[k - 1]);
          VectorPop(pack_now->pack_arguments);
        }
      }
      continue;
    }
    if (actual_index >= actual_args->length ||
        !DeduceFunctionTemplateOneTemplateArgument(
            args, explicit_arg_count, formal_arg,
            actual_args->value.p[actual_index])) {
      return false;
    }
    actual_index++;
  }
  return actual_index == actual_args->length;
}

/* True if any non-static data member of `str` still has a template-dependent
 * type (so the struct itself is dependent). */
static bool StructContainsTemplateParameter(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && member->symbol != NULL &&
        !member->is_static && !member->is_member_function &&
        !member->is_using_declaration &&
        !StructMemberIsNestedType(member) &&
        TypeContainsTemplateParameter(member->symbol->type)) {
      return true;
    }
  }
  return false;
}

/* Core type-against-type deduction: match a (possibly dependent) parameter type
 * `formal` against a concrete argument type `actual`, recording deduced
 * arguments into `args`. Handles dependent member typedefs, bare parameters
 * `T`, references, arrays `T[N]`, pointers, same-template instantiations, and
 * structural recursion through the type chain. Returns false on a mismatch. */
static bool DeduceFunctionTemplateTypeArgument(Vector* args,
                                               size_t explicit_arg_count,
                                               TypeRecord* formal,
                                               TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return false;
  }
  if (formal->template_origin != NULL &&
      formal->dependent_member_name != NULL) {
    TypeRecord* owner =
        TypeInstantiateClassTemplate(&compiler->syntax, formal->template_origin,
                                     formal->template_arguments);
    bool ok = false;
    if (owner != NULL && TypeIsStructOrUnion(owner) &&
        owner->info.struct_info != NULL) {
      StructMember* member =
          FindStructMember(owner->info.struct_info,
                           formal->dependent_member_name);
      if (member != NULL && member->symbol != NULL &&
          StorageIs(member->symbol->storage, STO(typedef))) {
        ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                member->symbol->type, actual);
      }
    }
    TypeRecordDelete(owner);
    return ok;
  }
  if (formal->declarator == kDeclPrimitive &&
      TypeIsUnknown(formal) && formal->template_parameter_index >= 0) {
    return SetDeducedFunctionTemplateTypeArgument(
        args, explicit_arg_count, formal->template_parameter_index, actual);
  }

  if (TypeIsReference(formal)) {
    return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                              formal->next, actual);
  }

  if (formal->declarator == kDeclArray &&
      formal->info.array.template_parameter_index >= 0) {
    int index = formal->info.array.template_parameter_index;
    if (actual->declarator != kDeclArray || actual->info.array.is_vla ||
        actual->info.array.template_parameter_index >= 0 ||
        !SetDeducedFunctionTemplateNonTypeArgument(
            args, explicit_arg_count, index, actual->info.array.size.fixed)) {
      return false;
    }
  }

  if (formal->declarator != actual->declarator) {
    if (formal->declarator == kDeclPointer &&
        actual->declarator == kDeclArray) {
      return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal->next, actual->next);
    }
    if (formal->declarator == kDeclPointer &&
        actual->declarator == kDeclFunction) {
      return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal->next, actual);
    }
    return false;
  }
  switch (formal->declarator) {
    case kDeclPointer:
    case kDeclArray:
      return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal->next,
                                                actual->next);
    case kDeclReference:
    case kDeclRValueReference:
      return false;
    case kDeclPrimitive:
      if (DeduceFunctionTemplateTemplateArguments(args, explicit_arg_count,
                                                  formal, actual)) {
        return true;
      }
      if (TypeIsStructOrUnion(formal) && TypeIsStructOrUnion(actual) &&
          (TypeContainsTemplateParameter(formal) ||
           StructContainsTemplateParameter(formal->info.struct_info))) {
        return DeduceFunctionTemplateStructMembers(args, explicit_arg_count,
                                                   formal, actual);
      }
      return TypeEqual(formal, actual);
    case kDeclFunction:
      if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                              formal->next, actual->next)) {
        return false;
      }
      if (formal->info.function.prototype.length !=
          actual->info.function.prototype.length) {
        return false;
      }
      for (size_t i = 0; i < formal->info.function.prototype.length; i++) {
        Symbol* formal_arg = formal->info.function.prototype.value.p[i];
        Symbol* actual_arg = actual->info.function.prototype.value.p[i];
        if (formal_arg == NULL || actual_arg == NULL ||
            !DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal_arg->type,
                                                actual_arg->type)) {
          return false;
        }
      }
      return true;
  }
  return false;
}

/* Deduce template arguments for one (non-pack) call argument expression against
 * formal parameter type `formal`, applying the forwarding-reference rule: a
 * `T&&` parameter binding an lvalue deduces `T&` (reference collapsing). */
static bool DeduceFunctionTemplateCallArgument(Vector* args,
                                               size_t explicit_arg_count,
                                               TypeRecord* formal,
                                               ASTNode* actual) {
  if (formal == NULL || actual == NULL || actual->type == NULL) {
    return false;
  }
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  /* An `initializer_list<U>` parameter deduces `U` either from a braced-init
   * argument (`{1, 2, 3}`) or from an argument that is already an
   * `initializer_list<V>` (e.g. forwarding a bound `init` parameter through a
   * second template); reject only arguments that are neither. */
  if (TypeIsCXXInitializerList(target) && actual->op != AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(TypeIsReference(actual->type)
                                    ? actual->type->next
                                    : actual->type)) {
    return false;
  }
  if (actual->op == AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(target) &&
      (target == NULL || target->declarator != kDeclArray)) {
    return false;
  }
  /* A string literal may initialize an array of characters, so a parameter of
   * type `CharT[N]` deduces its bound `N` (and, in aggregate CTAD, its element
   * type) directly from the literal.  The literal itself has a `const`-qualified
   * element type (`const char[M]`), which need not match the (possibly
   * non-const) parameter element, so deduce against a copy of the literal's
   * array type whose element cv-qualifiers are adjusted to the parameter's. */
  if ((actual->op == AST_OP(string) || actual->op == AST_OP(string_wide)) &&
      target != NULL && target->declarator == kDeclArray &&
      target->next != NULL && actual->type != NULL &&
      actual->type->declarator == kDeclArray && actual->type->next != NULL) {
    // Copy the whole array->element chain so the adjustment below never mutates
    // the shared element type of the original string-literal expression.
    TypeRecord* adjusted = TypeRecordCopy(actual->type);
    TypeRecord* element = TypeRecordCopy(actual->type->next);
    element->qualifiers = target->next->qualifiers;
    TypeRecordIncRef(element);
    // `adjusted->next` still aliases the original (shared) element; drop that
    // borrowed reference before repointing at our private copy.
    TypeRecordDelete(adjusted->next);
    adjusted->next = element;
    TypeRecordCalculateSize(adjusted);
    TypeRecordIncRef(adjusted);
    bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                 target, adjusted);
    TypeRecordDelete(adjusted);
    return ok;
  }
  /* [temp.deduct.call]: if the parameter type P contains no template
   * parameters, no deduction is performed from this argument.  The pairing
   * trivially succeeds; whether the argument is convertible to P is decided
   * later during overload resolution.  Requiring an exact match here would
   * wrongly reject calls needing a standard conversion (e.g. int -> long) and
   * corrupt deduction of the other, dependent, parameters. */
  if (!TypeContainsTemplateParameter(formal)) {
    return true;
  }
  /* [temp.deduct.type]: a type nominated by a qualified-id whose
   * nested-name-specifier names a member of a dependent type -- e.g.
   * `typename iterator_traits<It>::difference_type` -- is a non-deduced
   * context.  We still *attempt* deduction from it (this compiler can often
   * recover the parameter from the argument's concrete nested type, an
   * extension relied upon by e.g. `T f(typename Owner<T>::Inner)`), but a
   * *failure* to deduce here must not fail the whole call: the parameter may
   * be deduced from another argument or supplied explicitly (e.g. std::next's
   * defaulted `difference_type` second parameter).  We look through leading
   * references and pointers so that `const X<T>::type&` and `X<T>::type*` are
   * recognized too, but not `T*`/`T&` (ordinary deduced contexts, which lack a
   * dependent member name). */
  bool non_deduced_member = false;
  for (TypeRecord* p = formal; p != NULL; p = p->next) {
    if (p->dependent_member_name != NULL) {
      non_deduced_member = true;
      break;
    }
    if (p->declarator != kDeclReference && p->declarator != kDeclRValueReference &&
        p->declarator != kDeclPointer) {
      break;
    }
  }
  int placeholder_index = -1;
  if (formal->declarator == kDeclRValueReference &&
      TypeIsTemplateParameterPlaceholder(formal->next, &placeholder_index) &&
      actual->value_category == kValueCategoryLvalue) {
    TypeRecord* lvalue_ref = NewReferenceTypeRecord(kQualPlain, false);
    TypeRecordChain(lvalue_ref, actual->type);
    lvalue_ref->type = actual->type->type;
    TypeRecordCalculateSize(lvalue_ref);
    bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                 formal->next, lvalue_ref);
    TypeRecordDelete(lvalue_ref);
    return ok || non_deduced_member;
  }
  TypeRecord* decayed = DecayCallArgumentTypeForDeduction(formal, actual->type);
  if (decayed != NULL) {
    bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                 formal, decayed);
    TypeRecordDelete(decayed);
    return ok || non_deduced_member;
  }
  return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count, formal,
                                            actual->type) ||
         non_deduced_member;
}

/* Deduce template arguments by matching two structurally-identical structs
 * member-by-member (used for aggregate deduction): member layout/kind/names
 * must agree, and each data member's type is deduced recursively. */
static bool DeduceFunctionTemplateStructMembers(Vector* args,
                                                size_t explicit_arg_count,
                                                TypeRecord* formal,
                                                TypeRecord* actual) {
  if (!TypeIsStructOrUnion(formal) || !TypeIsStructOrUnion(actual) ||
      formal->info.struct_info == NULL || actual->info.struct_info == NULL) {
    return false;
  }
  Struct* formal_struct = formal->info.struct_info;
  Struct* actual_struct = actual->info.struct_info;
  if (formal_struct->members.length != actual_struct->members.length) {
    return false;
  }

  for (size_t i = 0; i < formal_struct->members.length; i++) {
    StructMember* formal_member = formal_struct->members.value.p[i];
    StructMember* actual_member = actual_struct->members.value.p[i];
    if (formal_member == NULL || actual_member == NULL ||
        formal_member->symbol == NULL || actual_member->symbol == NULL ||
        formal_member->is_static != actual_member->is_static ||
        formal_member->is_member_function != actual_member->is_member_function ||
        StructMemberIsNestedType(formal_member) !=
            StructMemberIsNestedType(actual_member) ||
        !StringEqualString(&formal_member->symbol->name,
                           &actual_member->symbol->name)) {
      return false;
    }
    if (formal_member->is_member_function || formal_member->is_static) {
      continue;
    }
    if (StructMemberIsNestedType(formal_member)) {
      if (!TypeEqual(formal_member->symbol->type, actual_member->symbol->type)) {
        return false;
      }
      continue;
    }
    if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                            formal_member->symbol->type,
                                            actual_member->symbol->type)) {
      return false;
    }
  }
  return true;
}

/* True if every still-undeduced template parameter has a usable default
 * (type default or default int value), so deduction can be completed. */
static bool FunctionTemplateCanCompleteDeducedArguments(TypeRecord* func,
                                                        Vector* args) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.template_parameters.length != args->length) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    if (args->value.p[i] != NULL) {
      continue;
    }
    TemplateParameter* param = func->info.function.template_parameters.value.p[i];
    if (param->kind == kTemplateParameterType) {
      if (param->default_type == NULL) {
        return false;
      }
    } else if (!param->has_default_int) {
      return false;
    }
  }
  return true;
}

/* Build the initial deduction-argument vector for a function template, one slot
 * per template parameter. Explicitly provided arguments are placed (a trailing
 * parameter pack absorbs all remaining explicit args), and parameters left to
 * be deduced are NULL. Reports how many leading slots are explicit. Returns
 * NULL on an explicit-argument/parameter mismatch. */
static Vector* NewFunctionTemplateDeductionArguments(TypeRecord* func,
                                                     Vector* explicit_args,
                                                     size_t* explicit_arg_count) {
  Vector* args = NewVector();
  size_t explicit_index = 0;
  size_t fixed_explicit_count = 0;
  for (size_t i = 0; i < func->info.function.template_parameters.length; i++) {
    TemplateParameter* param =
        func->info.function.template_parameters.value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      TemplateArgument* pack = NewEmptyPackTemplateArgument(param->kind);
      while (explicit_args != NULL && explicit_index < explicit_args->length) {
        TemplateArgument* explicit_arg = explicit_args->value.p[explicit_index++];
        if (explicit_arg == NULL || explicit_arg->kind != param->kind) {
          TemplateArgumentDelete(pack);
          VectorDeleteWithContents(args,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
          return NULL;
        }
        if (explicit_arg->pack_arguments != NULL) {
          for (size_t j = 0; j < explicit_arg->pack_arguments->length; j++) {
            VectorAppend(pack->pack_arguments,
                         TemplateArgumentCopy(
                             explicit_arg->pack_arguments->value.p[j]));
          }
        } else {
          VectorAppend(pack->pack_arguments, TemplateArgumentCopy(explicit_arg));
        }
      }
      VectorAppend(args, pack);
      continue;
    }

    TemplateArgument* explicit_arg = NULL;
    if (explicit_args != NULL && explicit_index < explicit_args->length) {
      TemplateArgument* source = explicit_args->value.p[explicit_index++];
      explicit_arg = source != NULL && source->pack_arguments != NULL &&
                             source->pack_arguments->length == 1
                         ? TemplateArgumentCopy(source->pack_arguments->value.p[0])
                         : TemplateArgumentCopy(source);
    }
    if (explicit_arg != NULL) {
      fixed_explicit_count = i + 1;
    }
    VectorAppend(args, explicit_arg);
  }
  if (explicit_args != NULL && explicit_index < explicit_args->length) {
    VectorDeleteWithContents(args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  if (explicit_arg_count != NULL) {
    *explicit_arg_count = fixed_explicit_count;
  }
  return args;
}

/* Number of leading fixed formals that must be supplied by call arguments (i.e.
 * those without a default argument), used to validate the call's arity. */
static size_t RequiredFixedFunctionTemplateFormals(TypeRecord* func,
                                                   size_t first_formal_arg,
                                                   size_t fixed_formal_count) {
  size_t required = 0;
  for (size_t i = 0; i < fixed_formal_count; i++) {
    Symbol* formal =
        func->info.function.prototype.value.p[i + first_formal_arg];
    if (formal != NULL && formal->default_argument == NULL) {
      required = i + 1;
    }
  }
  return required;
}

/* Deduce the full template argument vector for a call to function template
 * `templ` from the explicit template arguments and the actual call arguments
 * `actuals` (skipping `first_formal_arg` leading formals, e.g. an implicit
 * `this`). Validates arity (including a trailing parameter pack), deduces each
 * argument, and returns the deduced argument vector or NULL if deduction fails.
 */
static Vector* DeduceSimpleFunctionTemplateArguments(Symbol* templ,
                                                     Vector* explicit_args,
                                                     Vector* actuals,
                                                     size_t first_formal_arg) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type)) {
    return NULL;
  }
  TypeRecord* func =
      templ->type->template_arguments != NULL &&
              templ->type->info.function.cxx_member_owner != NULL
          ? templ->type
          : templ->value.func_defn != NULL && templ->value.func_defn->type != NULL
                ? templ->value.func_defn->type
                : templ->type;
  if (first_formal_arg > func->info.function.prototype.length) {
    return NULL;
  }
  int formal_pack_index = -1;
  for (size_t i = first_formal_arg; i < func->info.function.prototype.length;
       i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL && formal->flags.is_parameter_pack) {
      formal_pack_index = (int)i;
      break;
    }
  }
  size_t fixed_formal_count =
      func->info.function.prototype.length - first_formal_arg;
  if (formal_pack_index >= 0) {
    fixed_formal_count = (size_t)formal_pack_index - first_formal_arg;
  }
  size_t required_formal_count = RequiredFixedFunctionTemplateFormals(
      func, first_formal_arg, fixed_formal_count);
  if (func->info.function.template_parameter_count <= 0 ||
      func->info.function.unknown_args || func->info.function.varargs ||
      (formal_pack_index < 0 &&
       (actuals->length < required_formal_count ||
        actuals->length > fixed_formal_count)) ||
      (formal_pack_index >= 0 && actuals->length < required_formal_count)) {
    return NULL;
  }
  size_t explicit_arg_count = 0;
  Vector* args =
      NewFunctionTemplateDeductionArguments(func, explicit_args,
                                            &explicit_arg_count);
  if (args == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < actuals->length; i++) {
    Symbol* formal = NULL;
    if (formal_pack_index >= 0 && i >= fixed_formal_count) {
      formal = func->info.function.prototype.value.p[formal_pack_index];
      int pack_type_index = -1;
      ASTNode* actual = actuals->value.p[i];
      size_t pack_length = 0;
      if (formal == NULL || actual == NULL || actual->type == NULL ||
          !FindPackExpansionInType(formal->type, args, &pack_type_index,
                                   &pack_length) ||
          !DeduceFunctionTemplatePackCallArgument(
              args, explicit_arg_count, pack_type_index, formal->type,
              actual)) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        return NULL;
      }
      continue;
    }
    formal = func->info.function.prototype.value.p[i + first_formal_arg];
    ASTNode* actual = actuals->value.p[i];
    if (formal == NULL || actual == NULL ||
        !(DeduceFunctionTemplateArrayInitializerArgument(
              args, explicit_arg_count, formal->type, actual) ||
          DeduceFunctionTemplateInitializerListArgument(
              args, explicit_arg_count, formal->type, actual) ||
          DeduceFunctionTemplateCallArgument(args, explicit_arg_count,
                                             formal->type, actual))) {
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
  }
  if (formal_pack_index >= 0) {
    Symbol* formal = func->info.function.prototype.value.p[formal_pack_index];
    int pack_type_index = -1;
    if (formal != NULL &&
        TypeIsTemplateParameterPlaceholder(formal->type, &pack_type_index) &&
        pack_type_index >= 0 && (size_t)pack_type_index < args->length &&
        args->value.p[pack_type_index] == NULL) {
      args->value.p[pack_type_index] =
          NewEmptyPackTemplateArgument(kTemplateParameterType);
    }
  }
  for (size_t i = 0; i < args->length; i++) {
    if (args->value.p[i] == NULL) {
      if (!FunctionTemplateCanCompleteDeducedArguments(func, args)) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        return NULL;
      }
      break;
    }
  }
  return args;
}

/* Public: deduce and instantiate a function template from a call's actuals. */
Symbol* TypeDeduceFunctionTemplateFromCall(Syntax* syntax, Symbol* templ,
                                           Vector* actuals) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, NULL, actuals, 0);
}

/* Public: as above, with caller-supplied explicit template arguments. */
Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
    Syntax* syntax, Symbol* templ, Vector* explicit_args, Vector* actuals) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, explicit_args, actuals, 0);
}

/* Public: as above, skipping `first_formal_arg` leading formals (e.g. implicit
 * `this` for member functions). */
Symbol* TypeDeduceFunctionTemplateFromCallWithOffset(Syntax* syntax,
                                                     Symbol* templ,
                                                     Vector* actuals,
                                                     size_t first_formal_arg) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, NULL, actuals, first_formal_arg);
}

/* Public: deduce template arguments from a call (with explicit args and formal
 * offset) and instantiate the template. Returns `templ` unchanged if deduction
 * fails. */
Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    Syntax* syntax, Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return templ;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, templ->type, args,
                                        /*emit_error=*/true);
  TypeParserDestruct(&parser);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL) {
    return templ;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }
  Symbol* symbol =
      TypeInstantiateFunctionTemplate(syntax, templ, completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return symbol;
}

/* Public: test whether a function template's arguments can be deduced from a
 * call (used for overload viability) without instantiating it. */
bool TypeCanDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return false;
  }
  TypeParser parser;
  TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                 STO(implicit), compiler->syntax.context);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, templ->type, args,
                                        /*emit_error=*/false);
  TypeParserDestruct(&parser);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL) {
    return false;
  }
  bool ok = ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return ok;
}

/* Public: build a non-emitting concrete candidate for overload resolution.
 * This performs deduction, constraint checking, default completion, and
 * signature substitution, but it does not clone a body or enqueue codegen. */
Symbol* TypeCreateFunctionTemplateCandidate(Syntax* syntax, Symbol* templ,
                                            Vector* explicit_args,
                                            Vector* actuals,
                                            size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return NULL;
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, templ->type, args,
                                        /*emit_error=*/false);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL ||
      TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    if (completed_args != NULL) {
      VectorDeleteWithContents(
          completed_args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    TypeParserDestruct(&parser);
    return NULL;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    TypeParserDestruct(&parser);
    return NULL;
  }

  Symbol* template_definition = templ;
  if ((template_definition->type == NULL ||
       template_definition->type->info.function.body == NULL) &&
      templ->value.func_defn != NULL &&
      templ->value.func_defn->type != NULL) {
    template_definition = templ->value.func_defn;
  }
  bool saved_substitution_failed = parser.template_substitution_failed;
  parser.template_substitution_failed = false;
  TypeRecord* func = InstantiateFunctionTemplateType(&parser,
                                                     template_definition->type,
                                                     completed_args);
  bool substitution_failed = parser.template_substitution_failed;
  parser.template_substitution_failed = saved_substitution_failed;
  if (substitution_failed || TypeContainsTemplateParameter(func)) {
    TypeRecordDelete(func);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    TypeParserDestruct(&parser);
    return NULL;
  }

  Symbol* symbol = NewSymbol(templ->name.value, func, templ->storage);
  symbol->location = templ->location;
  symbol->namespace_ = templ->namespace_;
  func->info.function.symbol = symbol;
  func->info.function.template_origin = templ;
  func->template_arguments = TemplateArgumentVectorCopy(completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  TypeParserDestruct(&parser);
  return symbol;
}

/* Public: deduce (but do not instantiate) a function template's argument vector
 * from a call's actuals. */
Vector* TypeDeduceFunctionTemplateArgumentsFromCall(Symbol* templ,
                                                    Vector* actuals,
                                                    size_t first_formal_arg) {
  return DeduceSimpleFunctionTemplateArguments(templ, NULL, actuals,
                                               first_formal_arg);
}

/* Public: register a user-written CTAD deduction guide for a class template. */
void TypeAddCXXDeductionGuide(Symbol* class_template, Symbol* guide) {
  if (class_template == NULL || class_template->type == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL || guide == NULL) {
    return;
  }
  VectorAppend(&class_template->type->info.struct_info->deduction_guides, guide);
}

/* True if `origin` is an alias template whose pattern names another template
 * (so a use of the alias acts as a class-template placeholder for CTAD). */
static bool CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(Symbol* origin) {
  return origin != NULL && StorageIs(origin->storage, STO(typedef)) &&
         origin->type != NULL && origin->type->template_origin != NULL;
}

/* True if `type` is a class-template name used without arguments (a CTAD
 * placeholder), i.e. `Foo x = ...;` where Foo is a class template, including
 * the alias-template form. */
bool TypeIsClassTemplatePlaceholder(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->info.struct_info != NULL && t->info.struct_info->is_template &&
        (t->template_arguments == NULL ||
         (CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(t->template_origin) &&
          TemplateArgumentVectorContainsTemplateParameter(t->template_arguments)))) {
      return true;
    }
  }
  return false;
}

/* Return the class template a CTAD placeholder type refers to (resolving alias
 * templates to the underlying class template), or NULL. */
Symbol* TypeClassTemplatePlaceholderOrigin(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->info.struct_info != NULL && t->info.struct_info->is_template &&
        (t->template_arguments == NULL ||
         (CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(t->template_origin) &&
          TemplateArgumentVectorContainsTemplateParameter(t->template_arguments)))) {
      if (CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(t->template_origin)) {
        return t->template_origin->type->template_origin;
      }
      return t->template_origin;
    }
  }
  return NULL;
}

/* Return the type-chain node that is the CTAD placeholder within `type`. */
static TypeRecord* TypeClassTemplatePlaceholderBase(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->info.struct_info != NULL && t->info.struct_info->is_template &&
        (t->template_arguments == NULL ||
         (CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(t->template_origin) &&
          TemplateArgumentVectorContainsTemplateParameter(t->template_arguments)))) {
      return t;
    }
  }
  return NULL;
}

static void MaxTemplateParameterIndexInArgument(TemplateArgument* arg,
                                                int* max_index);

/* Update `*max_index` with the largest template-parameter index referenced
 * anywhere in `type` (used to size a deduction-binding vector for a pattern). */
static void MaxTemplateParameterIndexInType(TypeRecord* type, int* max_index) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index > *max_index) {
      *max_index = t->template_parameter_index;
    }
    if (t->declarator == kDeclArray &&
        t->info.array.template_parameter_index > *max_index) {
      *max_index = t->info.array.template_parameter_index;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        MaxTemplateParameterIndexInArgument(t->template_arguments->value.p[i],
                                            max_index);
      }
    }
    if (TypeIsFunction(t)) {
      for (size_t i = 0; i < t->info.function.prototype.length; i++) {
        Symbol* formal = t->info.function.prototype.value.p[i];
        if (formal != NULL) {
          MaxTemplateParameterIndexInType(formal->type, max_index);
        }
      }
    }
  }
}

static void MaxTemplateParameterIndexInArgument(TemplateArgument* arg,
                                                int* max_index) {
  if (arg == NULL) {
    return;
  }
  if (arg->kind == kTemplateParameterNonType &&
      arg->template_parameter_index > *max_index) {
    *max_index = arg->template_parameter_index;
  }
  MaxTemplateParameterIndexInType(arg->type, max_index);
}

/* Match a partial-specialization argument `pattern` against an already-deduced
 * argument `deduced`, binding the specialization's own parameters into
 * `bindings`. Non-type params bind/compare values; type params either deduce
 * (if dependent) or require exact type equality. */
static bool TemplateArgumentPatternMatchesDeduced(Vector* bindings,
                                                  TemplateArgument* pattern,
                                                  TemplateArgument* deduced) {
  if (pattern == NULL || deduced == NULL || pattern->kind != deduced->kind) {
    return false;
  }
  if (pattern->kind == kTemplateParameterNonType) {
    if (pattern->template_parameter_index >= 0) {
      return SetDeducedFunctionTemplateNonTypeArgument(
          bindings, 0, pattern->template_parameter_index, deduced->int_value);
    }
    return pattern->int_value == deduced->int_value;
  }
  if (pattern->type == NULL || deduced->type == NULL) {
    return false;
  }
  if (TemplateArgumentContainsTemplateParameter(pattern)) {
    return DeduceFunctionTemplateTypeArgument(bindings, 0, pattern->type,
                                              deduced->type);
  }
  return TypeEqual(pattern->type, deduced->type);
}

/* Bind class-template parameter `index` to a deduced type (or verify a prior
 * binding is consistent) when matching a partial specialization. */
static bool SetDeducedClassTemplateTypeArgument(Vector* bindings, int index,
                                                TypeRecord* actual) {
  if (index < 0 || bindings == NULL || (size_t)index >= bindings->length) {
    return false;
  }
  TemplateArgument* existing = bindings->value.p[index];
  if (existing == NULL) {
    bindings->value.p[index] = NewDeducedTypeTemplateArgument(actual);
    return true;
  }
  return existing->kind == kTemplateParameterType &&
         TypeEqual(existing->type, actual);
}

static TemplateArgument* NewDeducedTypePackTemplateArgument(void) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NewVector();
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static bool SetDeducedClassTemplateTypePackArgument(Vector* bindings, int index,
                                                    Vector* actuals,
                                                    size_t first_actual,
                                                    size_t last_actual) {
  if (index < 0 || bindings == NULL || (size_t)index >= bindings->length ||
      actuals == NULL || first_actual > actuals->length ||
      last_actual > actuals->length || first_actual > last_actual) {
    return false;
  }
  TemplateArgument* pack = NewDeducedTypePackTemplateArgument();
  for (size_t i = first_actual; i < last_actual; i++) {
    TemplateArgument* actual = actuals->value.p[i];
    if (actual == NULL) {
      TemplateArgumentDelete(pack);
      return false;
    }
    // The actual argument may itself be an already-expanded pack bundle (e.g.
    // `box<int, char, long>` stores its variadic arguments as a single pack
    // argument).  Splice that bundle's elements into the deduced pack rather
    // than nesting it, so `sizeof...` and later expansion see the individual
    // types.
    if (actual->pack_arguments != NULL) {
      for (size_t j = 0; j < actual->pack_arguments->length; j++) {
        VectorAppend(pack->pack_arguments,
                     TemplateArgumentCopy(actual->pack_arguments->value.p[j]));
      }
      continue;
    }
    if (actual->kind != kTemplateParameterType) {
      TemplateArgumentDelete(pack);
      return false;
    }
    VectorAppend(pack->pack_arguments, TemplateArgumentCopy(actual));
  }
    TemplateArgument* existing = bindings->value.p[index];
  if (existing == NULL) {
    bindings->value.p[index] = pack;
    return true;
  }
  bool equal = TemplateArgumentEqual(existing, pack);
  TemplateArgumentDelete(pack);
  return equal;
}

static bool ClassTemplateTypePatternMatches(Vector* bindings,
                                            TypeRecord* pattern,
                                            TypeRecord* actual);
static bool ClassTemplateArgumentPatternMatches(Vector* bindings,
                                                TemplateArgument* pattern,
                                                TemplateArgument* actual);

static bool TemplateArgumentIsTypeParameterPackPattern(TemplateArgument* arg,
                                                       int* index) {
  if (index != NULL) {
    *index = -1;
  }
  if (arg == NULL || arg->kind != kTemplateParameterType ||
      arg->type == NULL ||
      !TypeIsTemplateParameterPlaceholder(arg->type, index)) {
    return false;
  }
  return arg->is_pack_expansion;
}

static bool ClassTemplateArgumentPackPatternMatches(Vector* bindings,
                                                    Vector* pattern_args,
                                                    Vector* actual_args) {
  if (pattern_args == NULL || actual_args == NULL) {
    return pattern_args == actual_args;
  }
  size_t actual_index = 0;
  for (size_t i = 0; i < pattern_args->length; i++) {
    TemplateArgument* pattern = pattern_args->value.p[i];
    int pack_index = -1;
    if (TemplateArgumentIsTypeParameterPackPattern(pattern, &pack_index)) {
      return SetDeducedClassTemplateTypePackArgument(
          bindings, pack_index, actual_args, actual_index, actual_args->length);
    }
    if (actual_index >= actual_args->length ||
        !ClassTemplateArgumentPatternMatches(
            bindings, pattern, actual_args->value.p[actual_index])) {
      return false;
    }
    actual_index++;
  }
  return actual_index == actual_args->length;
}

static bool PartialSpecializationBindingsContainNull(Vector* bindings) {
  if (bindings == NULL) {
    return false;
  }
  for (size_t i = 0; i < bindings->length; i++) {
    if (bindings->value.p[i] == NULL) {
      return true;
    }
  }
  return false;
}

/* Match one partial-specialization pattern argument against an actual class
 * template argument, binding the specialization's parameters into `bindings`. */
static bool ClassTemplateArgumentPatternMatches(Vector* bindings,
                                                TemplateArgument* pattern,
                                                TemplateArgument* actual) {
  if (pattern == NULL || actual == NULL || pattern->kind != actual->kind) {
    return false;
  }
  if (pattern->pack_arguments != NULL || actual->pack_arguments != NULL) {
    return pattern->pack_arguments != NULL && actual->pack_arguments != NULL &&
           ClassTemplateArgumentPackPatternMatches(bindings,
                                                   pattern->pack_arguments,
                                                   actual->pack_arguments);
  }
  int placeholder_index = -1;
  if (pattern->kind == kTemplateParameterType && pattern->type != NULL &&
      !TypeIsTemplateParameterPlaceholder(pattern->type, &placeholder_index) &&
      !PartialSpecializationBindingsContainNull(bindings)) {
    TypeParser parser;
    TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                   STO(implicit), compiler->syntax.context);
    TypeRecord* substituted =
        SubstituteTemplateParameters(&parser, pattern->type, bindings);
    TypeParserDestruct(&parser);
    bool ok = ClassTemplateTypePatternMatches(bindings, substituted,
                                              actual->type);
    TypeRecordDelete(substituted);
    return ok;
  }
  if (pattern->kind == kTemplateParameterNonType) {
    if (pattern->template_parameter_index >= 0) {
      return SetDeducedFunctionTemplateNonTypeArgument(
          bindings, 0, pattern->template_parameter_index, actual->int_value);
    }
    return pattern->int_value == actual->int_value;
  }
  return ClassTemplateTypePatternMatches(bindings, pattern->type, actual->type);
}

/* Match a partial-specialization argument-pattern vector against the actual
 * argument vector, accumulating parameter bindings. */
static bool ClassTemplateArgumentVectorPatternMatches(Vector* bindings,
                                                      Vector* pattern_args,
                                                      Vector* actual_args) {
  if (pattern_args == NULL || actual_args == NULL) {
    return pattern_args == actual_args;
  }
  // A template-parameter pack in the pattern (e.g. `box<T...>`) absorbs a
  // variable number of actual arguments, so the pattern and actual argument
  // counts need not match exactly. Locate a pack pattern (at most one) and let
  // it consume the middle, while leading/trailing fixed patterns match 1:1.
  int pack_pattern_pos = -1;
  for (size_t i = 0; i < pattern_args->length; i++) {
    int idx = -1;
    if (TemplateArgumentIsTypeParameterPackPattern(pattern_args->value.p[i],
                                                   &idx)) {
      pack_pattern_pos = (int)i;
      break;
    }
  }
  if (pack_pattern_pos < 0) {
    if (pattern_args->length != actual_args->length) {
      return false;
    }
    for (size_t i = 0; i < pattern_args->length; i++) {
      if (!ClassTemplateArgumentPatternMatches(
              bindings, pattern_args->value.p[i], actual_args->value.p[i])) {
        return false;
      }
    }
    return true;
  }
  size_t leading = (size_t)pack_pattern_pos;
  size_t trailing = pattern_args->length - leading - 1;
  if (actual_args->length < leading + trailing) {
    return false;
  }
  for (size_t i = 0; i < leading; i++) {
    if (!ClassTemplateArgumentPatternMatches(bindings, pattern_args->value.p[i],
                                             actual_args->value.p[i])) {
      return false;
    }
  }
  for (size_t i = 0; i < trailing; i++) {
    if (!ClassTemplateArgumentPatternMatches(
            bindings, pattern_args->value.p[leading + 1 + i],
            actual_args->value.p[actual_args->length - trailing + i])) {
      return false;
    }
  }
  int pack_index = -1;
  TemplateArgumentIsTypeParameterPackPattern(
      pattern_args->value.p[pack_pattern_pos], &pack_index);
  return SetDeducedClassTemplateTypePackArgument(
      bindings, pack_index, actual_args, leading,
      actual_args->length - trailing);
}

/* Match a partial-specialization type pattern (e.g. `T*`, `vector<T>`) against
 * a concrete actual type, binding `T` etc. into `bindings`. A bare parameter
 * placeholder binds the actual; otherwise structure must match exactly while
 * recursing through pointers/references/arrays/template arguments. */
static bool ClassTemplateTypePatternMatches(Vector* bindings,
                                            TypeRecord* pattern,
                                            TypeRecord* actual) {
  if (pattern == NULL || actual == NULL) {
    return pattern == actual;
  }
  int index = -1;
  if (TypeIsTemplateParameterPlaceholder(pattern, &index)) {
    if ((pattern->qualifiers & ~actual->qualifiers) != 0) {
      return false;
    }
    TypeRecord* deduced = TypeRecordCopy(actual);
    deduced->qualifiers &= ~pattern->qualifiers;
    bool ok = SetDeducedClassTemplateTypeArgument(bindings, index, deduced);
    TypeRecordDelete(deduced);
    return ok;
  }
  if (pattern->declarator != actual->declarator ||
      pattern->qualifiers != actual->qualifiers) {
    return false;
  }
  switch (pattern->declarator) {
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return ClassTemplateTypePatternMatches(bindings, pattern->next,
                                             actual->next);
    case kDeclArray:
      if (pattern->info.array.template_parameter_index >= 0) {
        if (actual->info.array.is_vla ||
            !SetDeducedFunctionTemplateNonTypeArgument(
                bindings, 0, pattern->info.array.template_parameter_index,
                actual->info.array.size.fixed)) {
          return false;
        }
      } else if (pattern->info.array.size.fixed !=
                 actual->info.array.size.fixed) {
        return false;
      }
      return ClassTemplateTypePatternMatches(bindings, pattern->next,
                                             actual->next);
    case kDeclFunction:
      if (!ClassTemplateTypePatternMatches(bindings, pattern->next,
                                           actual->next) ||
          pattern->info.function.prototype.length !=
              actual->info.function.prototype.length) {
        return false;
      }
      for (size_t i = 0; i < pattern->info.function.prototype.length; i++) {
        Symbol* pattern_formal = pattern->info.function.prototype.value.p[i];
        Symbol* actual_formal = actual->info.function.prototype.value.p[i];
        if (pattern_formal == NULL || actual_formal == NULL ||
            !ClassTemplateTypePatternMatches(bindings, pattern_formal->type,
                                             actual_formal->type)) {
          return false;
        }
      }
      return true;
    case kDeclPrimitive:
      if (pattern->type != actual->type) {
        return false;
      }
      if (TypeIsStructOrUnion(pattern)) {
        if (pattern->template_origin != actual->template_origin) {
          return false;
        }
        if (!ClassTemplateArgumentVectorPatternMatches(
                bindings, pattern->template_arguments,
                actual->template_arguments)) {
          return false;
        }
        return pattern->info.struct_info == actual->info.struct_info ||
               pattern->template_origin != NULL;
      }
      if (TypeIsEnum(pattern)) {
        return pattern->info.enum_info == actual->info.enum_info;
      }
      return true;
  }
  return false;
}

/* Public: for partial CTAD (e.g. `Foo<int> x = ...`), check that a deduction
 * candidate's class template arguments are consistent with the partially
 * specified placeholder arguments by matching them as patterns. */
bool TypeClassTemplatePlaceholderAcceptsDeduced(TypeRecord* placeholder,
                                                TypeRecord* deduced) {
  TypeRecord* base = TypeClassTemplatePlaceholderBase(placeholder);
  if (base == NULL || base->template_arguments == NULL) {
    return true;
  }
  if (deduced == NULL || !TypeIsStructOrUnion(deduced)) {
    return false;
  }
  Vector* deduced_args = deduced->template_arguments;
  if (deduced_args == NULL && deduced->info.struct_info != NULL &&
      deduced->info.struct_info->tag_symbol != NULL &&
      deduced->info.struct_info->tag_symbol->type != NULL) {
    TypeRecord* tag_type = deduced->info.struct_info->tag_symbol->type;
    deduced_args = tag_type->template_arguments;
  }
  if (deduced_args == NULL ||
      deduced_args->length != base->template_arguments->length) {
    return false;
  }
  int max_index = -1;
  for (size_t i = 0; i < base->template_arguments->length; i++) {
    MaxTemplateParameterIndexInArgument(base->template_arguments->value.p[i],
                                        &max_index);
  }
  Vector bindings;
  VectorInit(&bindings);
  for (int i = 0; i <= max_index; i++) {
    VectorAppend(&bindings, NULL);
  }
  bool ok = true;
  for (size_t i = 0; ok && i < base->template_arguments->length; i++) {
    ok = TemplateArgumentPatternMatchesDeduced(
        &bindings,
        base->template_arguments->value.p[i],
        deduced_args->value.p[i]);
  }
  VectorDestructWithContents(&bindings,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  return ok;
}

/* The largest declared index among a template parameter vector. */
static int TemplateParameterVectorMaxIndex(Vector* params) {
  int max_index = -1;
  for (size_t i = 0; params != NULL && i < params->length; i++) {
    TemplateParameter* param = params->value.p[i];
    if (param != NULL && param->index > max_index) {
      max_index = param->index;
    }
  }
  return max_index;
}

/* Allocate an empty (all-NULL) bindings vector sized to hold one slot per
 * parameter of a partial specialization. */
static Vector* NewPartialSpecializationBindings(
    ClassTemplatePartialSpecialization* partial) {
  int max_index = TemplateParameterVectorMaxIndex(&partial->template_parameters);
  Vector* bindings = NewVector();
  for (int i = 0; i <= max_index; i++) {
    VectorAppend(bindings, NULL);
  }
  return bindings;
}

/* True if every (non-pack) parameter of a partial specialization received a
 * binding during pattern matching, i.e. the specialization fully applies. */
static bool PartialSpecializationBindingsComplete(
    ClassTemplatePartialSpecialization* partial, Vector* bindings) {
  for (size_t i = 0; i < partial->template_parameters.length; i++) {
    TemplateParameter* param = partial->template_parameters.value.p[i];
    if (param == NULL || param->is_parameter_pack) {
      continue;
    }
    if (param->index < 0 || (size_t)param->index >= bindings->length ||
        bindings->value.p[param->index] == NULL) {
      return false;
    }
  }
  return true;
}

/* Heuristic "specificity" score for a type pattern: a bare parameter scores 0,
 * more concrete structure (qualifiers, pointers, arrays, template arguments)
 * scores higher. Used to pick the most specialized partial specialization. */
static int TemplateTypePatternSpecificity(TypeRecord* type) {
  if (type == NULL) {
    return 0;
  }
  int placeholder_index = -1;
  if (TypeIsTemplateParameterPlaceholder(type, &placeholder_index)) {
    return 0;
  }
  int score = 1;
  if (type->qualifiers != kQualPlain) {
    score++;
  }
  switch (type->declarator) {
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return score + 2 + TemplateTypePatternSpecificity(type->next);
    case kDeclArray:
      return score + 2 + TemplateTypePatternSpecificity(type->next);
    case kDeclFunction:
      score += TemplateTypePatternSpecificity(type->next);
      for (size_t i = 0; i < type->info.function.prototype.length; i++) {
        Symbol* formal = type->info.function.prototype.value.p[i];
        if (formal != NULL) {
          score += TemplateTypePatternSpecificity(formal->type);
        }
      }
      return score;
    case kDeclPrimitive:
      if (type->template_arguments != NULL) {
        for (size_t i = 0; i < type->template_arguments->length; i++) {
          TemplateArgument* arg = type->template_arguments->value.p[i];
          if (arg != NULL && arg->kind == kTemplateParameterType) {
            score += TemplateTypePatternSpecificity(arg->type);
          } else if (arg != NULL) {
            score += 2;
          }
        }
      }
      return score;
  }
  return score;
}

static int TemplateArgumentVectorPatternSpecificity(Vector* args);

/* Specificity score for one argument pattern (see TemplateTypePatternSpecificity):
 * a parameter-dependent non-type scores 0, a fixed value scores 2. */
static int TemplateArgumentPatternSpecificity(TemplateArgument* arg) {
  if (arg == NULL) {
    return 0;
  }
  if (arg->pack_arguments != NULL) {
    return TemplateArgumentVectorPatternSpecificity(arg->pack_arguments);
  }
  if (arg->kind == kTemplateParameterType) {
    return TemplateTypePatternSpecificity(arg->type);
  }
  return arg->template_parameter_index >= 0 ? 0 : 2;
}

static void NoteSpecificityTypeParameter(int index, int* score,
                                         int* seen_type_parameters,
                                         size_t* seen_type_parameter_count) {
  for (size_t j = 0; j < *seen_type_parameter_count; j++) {
    if (seen_type_parameters[j] == index) {
      *score += 3;
      return;
    }
  }
  if (*seen_type_parameter_count < 64) {
    seen_type_parameters[(*seen_type_parameter_count)++] = index;
  }
}

static void NoteSpecificityArgumentParameters(TemplateArgument* arg, int* score,
                                              int* seen_type_parameters,
                                              size_t* seen_type_parameter_count) {
  if (arg == NULL) {
    return;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      NoteSpecificityArgumentParameters(arg->pack_arguments->value.p[i], score,
                                        seen_type_parameters,
                                        seen_type_parameter_count);
    }
    return;
  }
  if (arg->kind != kTemplateParameterType || arg->type == NULL ||
      !TypeIsTemplateParameterPlaceholder(arg->type, NULL)) {
    return;
  }
  NoteSpecificityTypeParameter(arg->type->template_parameter_index, score,
                               seen_type_parameters,
                               seen_type_parameter_count);
}

/* Total specificity score across an argument-pattern vector. */
static int TemplateArgumentVectorPatternSpecificity(Vector* args) {
  int score = 0;
  int seen_type_parameters[64];
  size_t seen_type_parameter_count = 0;
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    score += TemplateArgumentPatternSpecificity(arg);
    NoteSpecificityArgumentParameters(arg, &score, seen_type_parameters,
                                      &seen_type_parameter_count);
  }
  return score;
}

/* Try to match a single partial specialization against the actual class
 * template arguments. On success, returns its parameter bindings and a
 * specificity score (for choosing the most specialized one). */
static bool MatchClassTemplatePartialSpecialization(
    ClassTemplatePartialSpecialization* partial, Vector* actual_args,
    Vector** bindings_out, int* score_out) {
  if (partial == NULL || actual_args == NULL ||
      partial->pattern_arguments.length != actual_args->length) {
    return false;
  }
  Vector* bindings = NewPartialSpecializationBindings(partial);
  bool ok = true;
  for (size_t i = 0; ok && i < partial->pattern_arguments.length; i++) {
    ok = ClassTemplateArgumentPatternMatches(
        bindings, partial->pattern_arguments.value.p[i], actual_args->value.p[i]);
  }
  ok = ok && PartialSpecializationBindingsComplete(partial, bindings);
  if (!ok) {
    VectorDeleteWithContents(bindings,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return false;
  }
  *bindings_out = bindings;
  *score_out =
      TemplateArgumentVectorPatternSpecificity(&partial->pattern_arguments);
  return true;
}

/* Count the parameter-pack arguments in a partial specialization's pattern.
 * Used as a partial-ordering tiebreaker: a specialization whose pattern has a
 * trailing pack (e.g. `S<I, T, Rest...>`) is less specialized than one with a
 * fixed argument list (e.g. `S<I, T>`), so when both match an argument list the
 * one with fewer packs is preferred rather than reported as ambiguous. */
static int PartialSpecializationPackCount(
    ClassTemplatePartialSpecialization* partial) {
  int count = 0;
  for (size_t i = 0; i < partial->pattern_arguments.length; i++) {
    TemplateArgument* arg = partial->pattern_arguments.value.p[i];
    if (arg != NULL && arg->is_pack_expansion) {
      count++;
    }
  }
  // A trailing template parameter pack (e.g. `class... Rest`) may match zero
  // actual arguments, in which case it leaves no pack-expansion entry in the
  // pattern.  Count such parameter packs too so that `S<I, T, Rest...>` is
  // still ranked as less specialized than `S<I, T>`.
  for (size_t i = 0; i < partial->template_parameters.length; i++) {
    TemplateParameter* param = partial->template_parameters.value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      count++;
    }
  }
  return count;
}

/* Choose the best-matching partial specialization of class template `primary`
 * for the actual arguments: the one with the highest specificity score. Reports
 * an ambiguity error if two equally-specific specializations match, and returns
 * NULL when none match (the primary template is then used). */
static ClassTemplatePartialSpecialization* SelectClassTemplatePartialSpecialization(
    TypeParser* parser, Symbol* primary, Vector* actual_args,
    Vector** bindings_out) {
  if (primary == NULL || primary->type == NULL ||
      !TypeIsStructOrUnion(primary->type) ||
      primary->type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* primary_struct = primary->type->info.struct_info;
  ClassTemplatePartialSpecialization* best = NULL;
  Vector* best_bindings = NULL;
  int best_score = -1;
  int best_pack_count = 0;
  bool ambiguous = false;
  for (size_t i = 0; i < primary_struct->partial_specializations.length; i++) {
    ClassTemplatePartialSpecialization* partial =
        primary_struct->partial_specializations.value.p[i];
    Vector* bindings = NULL;
    int score = 0;
    if (!MatchClassTemplatePartialSpecialization(partial, actual_args,
                                                &bindings, &score)) {
      continue;
    }
    int pack_count = PartialSpecializationPackCount(partial);
    // Higher specificity wins; on a tie the specialization with fewer trailing
    // parameter packs is more specialized (partial ordering) and is preferred.
    bool better = best == NULL || score > best_score ||
                  (score == best_score && pack_count < best_pack_count);
    bool tied = best != NULL && score == best_score &&
                pack_count == best_pack_count;
    if (better) {
      if (best_bindings != NULL) {
        VectorDeleteWithContents(
            best_bindings, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      best = partial;
      best_bindings = bindings;
      best_score = score;
      best_pack_count = pack_count;
      ambiguous = false;
    } else if (tied) {
      ambiguous = true;
      VectorDeleteWithContents(bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    } else {
      VectorDeleteWithContents(bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
  }
  if (ambiguous) {
    SyntaxError(parser->syntax,
                "Ambiguous class template partial specialization for %s",
                primary->name.value);
    if (best_bindings != NULL) {
      VectorDeleteWithContents(best_bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return NULL;
  }
  if (best != NULL) {
    *bindings_out = best_bindings;
  }
  return best;
}

static bool TypeIsArithmeticType(TypeRecord* type) {
  return TypeIsIntegral(type) || TypeIsFloatingPoint(type);
}

static bool TypeEqualIgnoringTopLevelQualifiers(TypeRecord* left,
                                                TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  TypeRecord* left_plain = TypeRecordCopy(left);
  TypeRecord* right_plain = TypeRecordCopy(right);
  left_plain->qualifiers = kQualPlain;
  right_plain->qualifiers = kQualPlain;
  bool equal = TypeEqual(left_plain, right_plain);
  TypeRecordDelete(left_plain);
  TypeRecordDelete(right_plain);
  return equal;
}

static bool TypeCanAddTopLevelQualifiers(TypeRecord* from, TypeRecord* to) {
  if (!TypeEqualIgnoringTopLevelQualifiers(from, to)) {
    return false;
  }
  return (from->qualifiers & ~to->qualifiers) == 0;
}

/* Rough conversion rank for matching a CTAD deduction-guide parameter `formal`
 * against an argument type `actual` (lower is a better match, -1 = no match).
 * Used to pick the best deduction guide overload. */
static int DeductionGuideConversionRank(TypeRecord* formal,
                                        TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return -1;
  }
  if (TypeIsReference(formal)) {
    TypeRecord* target = formal->next;
    if (TypeEqual(target, actual)) {
      return 0;
    }
    return TypeCanAddTopLevelQualifiers(actual, target) ? 1 : -1;
  }
  if (TypeEqual(formal, actual)) {
    return 0;
  }
  if (TypeEqualIgnoringTopLevelQualifiers(formal, actual)) {
    return 1;
  }
  if (formal->declarator == kDeclPointer &&
      actual->declarator == kDeclArray &&
      TypeCanAddTopLevelQualifiers(actual->next, formal->next)) {
    return 1;
  }
  if (TypeEqualIgnoringSign(formal, actual)) {
    return 1;
  }
  if (TypeIsArithmeticType(formal) && TypeIsArithmeticType(actual)) {
    return 2;
  }
  if (TypeAssignmentCompatible(actual, formal)) {
    return 3;
  }
  return -1;
}

/* True if a braced initializer `{...}` can deduce a guide's
 * `std::initializer_list<T>` parameter: every element must be convertible to T. */
static bool CXXInitializerListBracedInitIsViableForDeduction(ASTNode* actual,
                                                             TypeRecord* formal) {
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (actual == NULL || actual->op != AST_OP(braced_init) ||
      !TypeIsCXXInitializerList(target)) {
    return false;
  }
  TypeRecord* element_type = TypeCXXInitializerListElement(target);
  if (element_type == NULL) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* element =
        CXXBracedInitializerElementExpression(braced->initializers->value.p[i]);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!TypeAssignmentCompatible(element->type, element_type)) {
      return false;
    }
  }
  return true;
}

/* True if a braced initializer can deduce a guide's array parameter `T[N]`:
 * within bounds, every (possibly nested) element convertible to the element. */
static bool CXXArrayBracedInitIsViableForDeduction(ASTNode* actual,
                                                   TypeRecord* formal) {
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (actual == NULL || actual->op != AST_OP(braced_init) || target == NULL ||
      target->declarator != kDeclArray) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  if (!target->info.array.is_flexible && !target->info.array.is_vla &&
      target->info.array.template_parameter_index < 0 &&
      target->info.array.size.fixed < (int)braced->initializers->length) {
    return false;
  }
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init == NULL) {
      return false;
    }
    if (init->op == AST_OP(braced_init)) {
      if (!CXXArrayBracedInitIsViableForDeduction(init, target->next)) {
        return false;
      }
      continue;
    }
    ASTNode* element = CXXBracedInitializerElementExpression(init);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!TypeAssignmentCompatible(element->type, target->next)) {
      return false;
    }
  }
  return true;
}

/* Score a CTAD deduction guide against the constructor call's actual arguments:
 * substitute the guide's parameters with `template_args`, rank each
 * parameter/argument conversion, and sum the ranks (lower total = better).
 * Returns false if the guide is not viable for these arguments. */
static bool ScoreDeductionGuideCall(TypeParser* parser,
                                    TypeRecord* guide_type,
                                    Vector* template_args,
                                    Vector* actuals,
                                    int* score) {
  if (parser == NULL || guide_type == NULL || !TypeIsFunction(guide_type) ||
      actuals == NULL || score == NULL ||
      guide_type->info.function.prototype.length != actuals->length) {
    return false;
  }
  int total = 0;
  for (size_t i = 0; i < actuals->length; i++) {
    Symbol* formal = guide_type->info.function.prototype.value.p[i];
    ASTNode* actual = actuals->value.p[i];
    if (formal == NULL || formal->type == NULL || actual == NULL) {
      return false;
    }
    TypeRecord* formal_type =
        template_args != NULL
            ? SubstituteTemplateParameters(parser, formal->type, template_args)
            : formal->type;
    int rank = actual->op == AST_OP(braced_init)
        ? (CXXArrayBracedInitIsViableForDeduction(actual, formal_type) ||
                   CXXInitializerListBracedInitIsViableForDeduction(actual,
                                                                    formal_type)
               ? 0
               : -1)
        : DeductionGuideConversionRank(formal_type, actual->type);
    if (template_args != NULL) {
      TypeRecordDelete(formal_type);
    }
    if (rank < 0) {
      return false;
    }
    total += rank;
  }
  *score = total;
  return true;
}

static bool CXXDeductionCandidateEqual(TypeRecord* left, TypeRecord* right) {
  if (TypeIsStructOrUnion(left) || TypeIsStructOrUnion(right)) {
    return TypeIsStructOrUnion(left) && TypeIsStructOrUnion(right) &&
           left->info.struct_info == right->info.struct_info;
  }
  return TypeEqual(left, right);
}

static TypeRecord* TypeDeduceClassTemplateFromGuideFiltered(
    Syntax* syntax, Symbol* class_template, TypeRecord* placeholder,
    Vector* actuals, bool allow_explicit, bool* alias_rejected) {
  if (syntax == NULL || class_template == NULL || class_template->type == NULL ||
      !class_template->flags.is_template || actuals == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = class_template->type->info.struct_info;
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* result = NULL;
  int best_score = -1;
  bool ambiguous = false;
  bool best_is_explicit = false;
  for (size_t i = 0; i < str->deduction_guides.length; i++) {
    Symbol* guide = str->deduction_guides.value.p[i];
    if (guide == NULL || guide->type == NULL || !TypeIsFunction(guide->type)) {
      continue;
    }
    TypeRecord* guide_return = NULL;
    int guide_score = 0;
    if (guide->flags.is_template) {
      Vector* args =
          TypeDeduceFunctionTemplateArgumentsFromCall(guide, actuals, 0);
      if (args == NULL) {
        continue;
      }
      if (!ScoreDeductionGuideCall(&parser, guide->type, args, actuals,
                                   &guide_score)) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        continue;
      }
      guide_return = SubstituteTemplateParameters(&parser, guide->type->next,
                                                  args);
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    } else {
      if (!ScoreDeductionGuideCall(&parser, guide->type, NULL, actuals,
                                   &guide_score)) {
        continue;
      }
      guide_return = TypeRecordCopy(guide->type->next);
    }
    if (guide_return == NULL) {
      continue;
    }
    if (guide->type->info.function.is_implicitly_declared) {
      guide_score += 1000;
    }
    if (guide->flags.is_template) {
      guide_score += 10;
    }
    TypeRecord* candidate = NULL;
    if (guide_return->template_origin == class_template &&
        guide_return->template_arguments != NULL) {
      candidate =
          TypeInstantiateClassTemplate(syntax, class_template,
                                       guide_return->template_arguments);
    } else if (guide_return->template_origin == class_template ||
               (TypeIsStructOrUnion(guide_return) &&
                guide_return->info.struct_info != NULL &&
                !guide_return->info.struct_info->is_template)) {
      candidate = TypeRecordCopy(guide_return);
    }
    TypeRecordDelete(guide_return);
    if (candidate == NULL) {
      continue;
    }
    if (placeholder != NULL &&
        !TypeClassTemplatePlaceholderAcceptsDeduced(placeholder, candidate)) {
      if (alias_rejected != NULL) {
        *alias_rejected = true;
      }
      TypeRecordDelete(candidate);
      continue;
    }
    if (result == NULL || guide_score < best_score) {
      if (result != NULL) {
        TypeRecordDelete(result);
      }
      result = candidate;
      best_score = guide_score;
      best_is_explicit = guide->type->info.function.is_explicit;
      ambiguous = false;
    } else if (guide_score == best_score &&
               !CXXDeductionCandidateEqual(result, candidate)) {
      ambiguous = true;
      TypeRecordDelete(candidate);
    } else {
      TypeRecordDelete(candidate);
    }
  }
  TypeParserDestruct(&parser);
  if (ambiguous) {
    SyntaxError(syntax, "Ambiguous class template argument deduction for %s",
                class_template->name.value);
    if (result != NULL) {
      TypeRecordDelete(result);
    }
    return NULL;
  }
  if (result != NULL && best_is_explicit && !allow_explicit) {
    SyntaxError(syntax,
                "Explicit deduction guide cannot be used for copy-initialization");
    TypeRecordDelete(result);
    return NULL;
  }
  return result;
}

TypeRecord* TypeDeduceClassTemplateFromGuide(Syntax* syntax,
                                             Symbol* class_template,
                                             Vector* actuals,
                                             bool allow_explicit) {
  return TypeDeduceClassTemplateFromGuideFiltered(
      syntax, class_template, NULL, actuals, allow_explicit, NULL);
}

TypeRecord* TypeDeduceClassTemplateFromPlaceholder(Syntax* syntax,
                                                   TypeRecord* placeholder,
                                                   Vector* actuals,
                                                   bool allow_explicit,
                                                   bool* alias_rejected) {
  if (alias_rejected != NULL) {
    *alias_rejected = false;
  }
  if (placeholder == NULL || !TypeIsClassTemplatePlaceholder(placeholder)) {
    return NULL;
  }
  Symbol* class_template = TypeClassTemplatePlaceholderOrigin(placeholder);
  TypeRecord* alias_base = TypeClassTemplatePlaceholderBase(placeholder);
  TypeRecord* filter = alias_base != NULL && alias_base->template_arguments != NULL
                           ? placeholder
                           : NULL;
  return TypeDeduceClassTemplateFromGuideFiltered(
      syntax, class_template, filter, actuals, allow_explicit, alias_rejected);
}

/* Set up the source->target struct substitution and clone/queue the body of an
 * instantiated member function.  Factored out so it can run in a second pass,
 * after every member function signature has been added to `owner`. */
static void CloneInstantiatedMemberFunctionBody(TypeParser* parser,
                                                Struct* owner, Symbol* symbol,
                                                Symbol* template_definition,
                                                Struct* substitution_source,
                                                Vector* args) {
  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  parser->template_substitution_source = substitution_source;
  parser->template_substitution_target = owner;
  QueueTemplateMemberFunctionDefinitionImpl(symbol, template_definition, parser,
                                            args, /*allow_lazy=*/true);
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
}

/* Instantiate a member function of a class template into the concrete `owner`.
 * Sets up the source->target struct substitution (so self-type references in
 * the signature/body resolve to the instantiation), builds the concrete
 * function type, names constructors/destructors after the instantiated tag,
 * and mangles the symbol.  The body clone is deferred: the information needed
 * to clone it is appended to `pending` and run after all member signatures are
 * in place (so a body may reference later-declared members).  Returns the new
 * member. */
static StructMember* InstantiateTemplateMemberFunction(TypeParser* parser,
                                                       Struct* owner,
                                                       StructMember* member,
                                                       Vector* args,
                                                       Vector* pending) {
  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  TypeRecord* source_owner =
      member->symbol != NULL && member->symbol->type != NULL
          ? member->symbol->type
                ->info.function.cxx_member_owner != NULL
                ? member->symbol->type->info.function.cxx_member_owner
                      ->tag_symbol != NULL
                      ? member->symbol->type->info.function.cxx_member_owner
                            ->tag_symbol->type
                      : NULL
                : NULL
          : NULL;
  Struct* substitution_source =
      source_owner != NULL && TypeIsStructOrUnion(source_owner)
          ? source_owner->info.struct_info
          : NULL;
  parser->template_substitution_source = substitution_source;
  parser->template_substitution_target = owner;
  TypeRecord* func = InstantiateMemberFunctionType(
      parser, owner, member->is_static, member->symbol->type, args,
      member->symbol->location);
  const char* symbol_name = member->symbol->name.value;
  String destructor_name;
  StringInit(&destructor_name, NULL);
  if (func->info.function.is_constructor && owner->tag_name != NULL) {
    symbol_name = owner->tag_name->value;
  } else if (func->info.function.is_destructor && owner->tag_name != NULL) {
    StringSet(&destructor_name, "~");
    StringAppendString(&destructor_name, owner->tag_name);
    symbol_name = destructor_name.value;
  }
  Symbol* symbol = NewSymbol(symbol_name, func,
                             member->symbol->storage);
  StringDestruct(&destructor_name);
  symbol->location = member->symbol->location;
  symbol->flags.is_template = member->symbol->flags.is_template;
  symbol->type->info.function.template_parameter_count =
      member->symbol->type->info.function.template_parameter_count;
  symbol->type->info.function.template_parameter_base = 0;
  func->info.function.symbol = symbol;
  SymbolSetCXXMangledAsmName(symbol);
  Symbol* template_definition = member->symbol->value.func_defn;
  if (template_definition == NULL &&
      member->symbol->type->info.function.body != NULL) {
    template_definition = member->symbol;
  }
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
  PendingMemberBody* pmb = malloc(sizeof(PendingMemberBody));
  pmb->symbol = symbol;
  pmb->template_definition = template_definition;
  pmb->substitution_source = substitution_source;
  VectorAppend(pending, pmb);
  StructMember* instantiated = NewStructMember(symbol);
  instantiated->is_member_function = true;
  instantiated->is_static = member->is_static;
  instantiated->access = member->access;
  return instantiated;
}

static TemplateArgument* NewDefaultTypeTemplateArgument(TypeRecord* type) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = type;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static TemplateArgument* NewDefaultNonTypeTemplateArgument(
    long long int_value, int template_parameter_index) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = kTemplateParameterNonType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = int_value;
  arg->template_parameter_index = template_parameter_index;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static TemplateArgument* NewEmptyPackTemplateArgument(
    TemplateParameterKind kind) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = kind;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NewVector();
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static int FindTemplateParameterPackIndex(Vector* template_parameters) {
  for (size_t i = 0; template_parameters != NULL &&
                     i < template_parameters->length; i++) {
    TemplateParameter* param = template_parameters->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      return (int)i;
    }
  }
  return -1;
}

/* Produce a full template argument vector with one entry per declared
 * parameter: copy supplied `args`, substitute defaults (which may themselves
 * reference earlier parameters) for any omitted trailing parameters, and gather
 * leftover args into a trailing parameter pack. Emits `error_message` and
 * returns NULL if required arguments are missing. */
static Vector* CompleteTemplateArguments(TypeParser* parser,
                                         Vector* template_parameters,
                                         Vector* args,
                                         const char* error_message,
                                         bool emit_error) {
  if (args == NULL) {
    if (emit_error) {
      SyntaxError(parser->syntax, "%s", error_message);
    }
    return NULL;
  }
  int pack_index = FindTemplateParameterPackIndex(template_parameters);
  if (pack_index < 0 && args->length > template_parameters->length) {
    if (emit_error) {
      SyntaxError(parser->syntax, "Too many template arguments");
    }
    return NULL;
  }
  Vector* completed = NewVector();
  for (size_t i = 0; i < template_parameters->length; i++) {
    TemplateParameter* param = template_parameters->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      TemplateArgument* pack = NewEmptyPackTemplateArgument(param->kind);
      for (size_t j = i; j < args->length; j++) {
        TemplateArgument* arg = args->value.p[j];
        if (arg == NULL || arg->kind != param->kind) {
          if (emit_error) {
            SyntaxError(parser->syntax,
                        param->kind == kTemplateParameterType
                            ? "Template argument must name a type"
                            : "Template non-type argument must be an integer constant expression");
          }
          TemplateArgumentDelete(pack);
          VectorDeleteWithContents(completed,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
          return NULL;
        }
        if (arg->pack_arguments != NULL) {
          for (size_t k = 0; k < arg->pack_arguments->length; k++) {
            TemplateArgument* element = arg->pack_arguments->value.p[k];
            if (element == NULL || element->kind != param->kind) {
              if (emit_error) {
                SyntaxError(parser->syntax,
                            param->kind == kTemplateParameterType
                                ? "Template argument must name a type"
                                : "Template non-type argument must be an integer constant expression");
              }
              TemplateArgumentDelete(pack);
              VectorDeleteWithContents(
                  completed, (VectorElementDestructor)TemplateArgumentDelete,
                  /*free_element=*/false);
              return NULL;
            }
            VectorAppend(pack->pack_arguments, TemplateArgumentCopy(element));
          }
        } else {
          VectorAppend(pack->pack_arguments, TemplateArgumentCopy(arg));
        }
      }
      VectorAppend(completed, pack);
      continue;
    }
    TemplateArgument* arg =
        i < args->length ? TemplateArgumentCopy(args->value.p[i]) : NULL;
    if (arg == NULL) {
      if (param->kind == kTemplateParameterType &&
          param->default_type != NULL) {
        TypeRecord* default_type =
            SubstituteTemplateParameters(parser, param->default_type,
                                         completed);
        arg = NewDefaultTypeTemplateArgument(default_type);
      } else if (param->kind == kTemplateParameterNonType &&
                 param->has_default_int) {
        long long default_int_value = param->default_int_value;
        int default_template_parameter_index =
            param->default_template_parameter_index;
        if (default_template_parameter_index >= 0 &&
            (size_t)default_template_parameter_index < completed->length) {
          TemplateArgument* actual =
              completed->value.p[default_template_parameter_index];
          if (actual->kind == kTemplateParameterNonType) {
            default_int_value = actual->int_value;
            default_template_parameter_index = -1;
          }
        }
        arg = NewDefaultNonTypeTemplateArgument(default_int_value,
                                                default_template_parameter_index);
      } else {
        if (emit_error) {
          SyntaxError(parser->syntax, "%s", error_message);
        }
        VectorDeleteWithContents(completed,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        return NULL;
      }
    }
    if (param->kind != arg->kind) {
      if (emit_error) {
        SyntaxError(parser->syntax,
                    param->kind == kTemplateParameterType
                        ? "Template argument must name a type"
                        : "Template non-type argument must be an integer constant expression");
      }
      TemplateArgumentDelete(arg);
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    VectorAppend(completed, arg);
  }
  return completed;
}

/* Complete a class template's argument list against its parameters (filling in
 * defaults and gathering a trailing pack); see CompleteTemplateArguments. */
static Vector* CompleteClassTemplateArguments(TypeParser* parser,
                                              Struct* template_struct,
                                              Vector* args) {
  return CompleteTemplateArguments(parser, &template_struct->template_parameters,
                                   args,
                                   "Class template instantiation is not supported yet",
                                   /*emit_error=*/true);
}

/* Register a partial specialization (its tag and argument pattern) on the
 * primary class template, rejecting an exact duplicate pattern. */
static void AddClassTemplatePartialSpecialization(TypeParser* parser,
                                                  Symbol* primary,
                                                  Symbol* partial_tag,
                                                  Vector* pattern_args) {
  if (primary == NULL || primary->type == NULL ||
      !TypeIsStructOrUnion(primary->type) ||
      primary->type->info.struct_info == NULL || partial_tag == NULL ||
      pattern_args == NULL) {
    return;
  }
  Struct* primary_struct = primary->type->info.struct_info;
  for (size_t i = 0; i < primary_struct->partial_specializations.length; i++) {
    ClassTemplatePartialSpecialization* existing =
        primary_struct->partial_specializations.value.p[i];
    if (existing != NULL &&
        TemplateArgumentPatternVectorEqual(&existing->pattern_arguments,
                                           pattern_args)) {
      SyntaxError(parser->syntax,
                  "Duplicate class template partial specialization %s",
                  partial_tag->name.value);
      return;
    }
  }
  ClassTemplatePartialSpecialization* partial =
      NewClassTemplatePartialSpecialization(
          partial_tag, parser->syntax->current_template_parameters,
          pattern_args);
  VectorAppend(&primary_struct->partial_specializations, partial);
}

/* Complete a function template's argument list against its parameters (filling
 * defaults / gathering a trailing pack), then clear the "unknown/placeholder"
 * marking on type arguments that resolved to their own parameter position so
 * they read as concrete deduced types. Returns NULL if completion fails. */
static Vector* CompleteFunctionTemplateArguments(TypeParser* parser,
                                                 TypeRecord* func,
                                                 Vector* args,
                                                 bool emit_error) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.template_parameters.length == 0) {
    if (emit_error) {
      SyntaxError(parser->syntax,
                  "Function template instantiation is not supported yet");
    }
    return NULL;
  }
  Vector* completed =
      CompleteTemplateArguments(parser, &func->info.function.template_parameters,
                                args,
                                "Function template instantiation is not supported yet",
                                emit_error);
  if (completed == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < completed->length; i++) {
    TemplateArgument* arg = completed->value.p[i];
    if (arg != NULL && arg->kind == kTemplateParameterType &&
        arg->type != NULL && TypeIsUnknown(arg->type) &&
        arg->type->template_parameter_index == (int)i) {
      arg->type->type &= ~kTypeUnknown;
      arg->type->template_parameter_index = -1;
    }
  }
  return completed;
}

/* Instantiate the deferred friend functions a class template declared, mapping
 * each onto a concrete free function in the template's namespace.  The friend's
 * signature (and inline body, if any) is substituted with the instantiation
 * arguments; the resulting function is registered for overload resolution/ADL,
 * recorded as a friend of `str`, and queued for emission when it carries a body
 * (deduplicated against earlier specializations and existing declarations). */
static void InstantiateTemplateFriendFunctions(TypeParser* parser, Struct* str,
                                               Struct* source_struct,
                                               Vector* args) {
  for (size_t i = 0; i < source_struct->friend_functions.length; i++) {
    Symbol* ftpl = source_struct->friend_functions.value.p[i];
    if (ftpl == NULL || ftpl->type == NULL || !TypeIsFunction(ftpl->type)) {
      if (ftpl != NULL) {
        StructAddFriendFunction(str, ftpl);
      }
      continue;
    }

    Struct* saved_source = parser->template_substitution_source;
    Struct* saved_target = parser->template_substitution_target;
    parser->template_substitution_source = source_struct;
    parser->template_substitution_target = str;

    TypeRecord* func = InstantiateMemberFunctionType(
        parser, /*owner=*/NULL, /*is_static_member=*/true, ftpl->type, args,
        ftpl->location);
    func->info.function.cxx_member_owner = NULL;

    Symbol* sym = NewSymbol(ftpl->name.value, func, ftpl->storage);
    sym->location = ftpl->location;
    sym->namespace_ = ftpl->namespace_;
    func->info.function.symbol = sym;
    SymbolSetCXXMangledAsmName(sym);

    Symbol* in_scope = SyntaxRegisterInstantiatedFriendFunction(
        parser->syntax, ftpl->namespace_, sym);
    StructAddFriendFunction(str, in_scope != NULL ? in_scope : sym);

    bool is_new_symbol = (in_scope == sym);
    if (is_new_symbol && ftpl->type->info.function.body != NULL &&
        !PendingTemplateInstantiationHasAsmName(sym->asm_name.value)) {
      sym->type->info.function.body = CloneTemplateFunctionBody(
          parser, ftpl->type, sym->type, args);
      sym->type->info.function.definition = true;
      sym->flags.is_defined = true;
      if (sym->type->info.function.is_inline) {
        sym->flags.is_inline_defn = true;
        if (!StorageIs(sym->storage, STO(static))) {
          sym->flags.is_weak = true;
        }
      }
      Vector* declarations = NewVector();
      VectorAppend(declarations,
                   NewVariableDeclarationASTNode(sym, NULL, sym->location));
      VectorAppend(&compiler->pending_template_instantiations,
                   NewDeclarationListASTNode(declarations, sym->location));
      VectorAppend(&compiler->declaration_asts,
                   sym->type->info.function.body);
    }

    parser->template_substitution_source = saved_source;
    parser->template_substitution_target = saved_target;
  }
}

/* Instantiate a class template `templ` with arguments `args`, returning the
 * concrete struct/union type (memoized by instantiation name so each unique
 * argument set is built once). Steps: handle alias templates; complete default
 * arguments; reuse an existing instantiation tag if present; select the best
 * partial specialization; create the instantiated struct tag; then instantiate
 * bases and members (expanding base/member packs) and lay out the struct. */
static TypeRecord* InstantiateSimpleClassTemplate(TypeParser* parser,
                                                  Symbol* templ,
                                                  Vector* args) {
  TypeRecord* alias_type = InstantiateAliasClassTemplate(parser, templ, args);
  if (alias_type != NULL) {
    return alias_type;
  }
  if (templ == NULL || templ->type == NULL ||
      !TypeIsStructOrUnion(templ->type) ||
      templ->type->info.struct_info == NULL ||
      !templ->type->info.struct_info->is_template) {
    return TypeRecordCopy(templ->type);
  }
  Struct* template_struct = templ->type->info.struct_info;
  Vector* completed_args =
      CompleteClassTemplateArguments(parser, template_struct, args);
  if (completed_args == NULL) {
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }

  String instantiated_name;
  StringInit(&instantiated_name, NULL);
  AppendTemplateInstantiationName(&instantiated_name, templ, completed_args);
  Symbol* existing =
      FindTemplateInstantiationTag(parser, templ, &instantiated_name);
  if (existing != NULL && existing->type != NULL &&
      TypeIsStructOrUnion(existing->type) &&
      existing->type->info.struct_info != NULL &&
      !existing->type->info.struct_info->is_template) {
    StringDestruct(&instantiated_name);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(existing->type);
  }
  if (existing != NULL) {
    SyntaxError(parser->syntax,
                "Class template instantiation conflicts with existing tag %s",
                instantiated_name.value);
    StringDestruct(&instantiated_name);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(templ->type);
  }
  Vector* partial_args = NULL;
  ClassTemplatePartialSpecialization* partial =
      SelectClassTemplatePartialSpecialization(parser, templ, completed_args,
                                               &partial_args);
  Struct* source_struct = template_struct;
  Vector* source_args = completed_args;
  if (partial != NULL && partial->tag_symbol != NULL &&
      partial->tag_symbol->type != NULL &&
      TypeIsStructOrUnion(partial->tag_symbol->type) &&
      partial->tag_symbol->type->info.struct_info != NULL) {
    source_struct = partial->tag_symbol->type->info.struct_info;
    source_args = partial_args;
  }
  if (!ClassTemplateInstantiationMembersSupported(parser, source_struct)) {
    StringDestruct(&instantiated_name);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(templ->type);
  }

  Struct* str = NewStruct(source_struct->is_union);
  str->is_class = source_struct->is_class;
  str->packed = source_struct->packed;
  str->explicit_alignment = source_struct->explicit_alignment;
  str->pack = source_struct->pack;
  // Carry friend classes from the template to each instantiation verbatim;
  // friend *functions* are instantiated per specialization below, after the
  // members are in place, so their dependent signatures and inline bodies can
  // be substituted with the template arguments.
  for (size_t i = 0; i < source_struct->friend_classes.length; i++) {
    StructAddFriendClass(str, source_struct->friend_classes.value.p[i]);
  }
  TypeRecord* type = NewTypeRecord(source_struct->is_union ? kTypeUnion
                                                           : kTypeStruct,
                                  kQualPlain);
  type->info.struct_info = str;
  type->template_origin = templ;
  type->template_arguments = TemplateArgumentVectorCopy(completed_args);
  Symbol* tag = NewSymbol(instantiated_name.value, type, STO(implicit));
  tag->flags.is_defined = true;
  str->tag_name = &tag->name;
  str->tag_symbol = tag;
  if (!AddTemplateInstantiationTag(parser, templ, tag)) {
    SyntaxError(parser->syntax,
                "Class template instantiation conflicts with existing tag %s",
                instantiated_name.value);
    StringDestruct(&instantiated_name);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(templ->type);
  }

  for (size_t i = 0; i < source_struct->bases.length; i++) {
    CXXBaseSpecifier* template_base = source_struct->bases.value.p[i];
    int pack_index = -1;
    if (template_base->is_pack_expansion &&
        TypeIsTemplateParameterPlaceholder(template_base->type, &pack_index) &&
        pack_index >= 0 && (size_t)pack_index < source_args->length) {
      TemplateArgument* pack = source_args->value.p[pack_index];
      if (pack != NULL && pack->pack_arguments != NULL) {
        for (size_t j = 0; j < pack->pack_arguments->length; j++) {
          TemplateArgument* element = pack->pack_arguments->value.p[j];
          if (element == NULL || element->kind != kTemplateParameterType ||
              element->type == NULL || !TypeIsStructOrUnion(element->type)) {
            SyntaxError(parser->syntax,
                        "Base class pack expansion requires class types");
            continue;
          }
          TypeRecord* base_type = TypeRecordCopy(element->type);
          TypeRecordCalculateSize(base_type);
          VectorAppend(&str->bases,
                       NewCXXBaseSpecifier(base_type, template_base->access,
                                           template_base->is_virtual));
          TypeRecordDelete(base_type);
        }
        continue;
      }
    }
    TypeRecord* base_type =
        SubstituteTemplateParameters(parser, template_base->type,
                                     source_args);
    if (!TypeIsStructOrUnion(base_type)) {
      SyntaxError(parser->syntax, "base class must be a class or struct type");
      TypeRecordDelete(base_type);
      continue;
    }
    TypeRecordCalculateSize(base_type);
    VectorAppend(&str->bases,
                 NewCXXBaseSpecifier(base_type, template_base->access,
                                     template_base->is_virtual));
    TypeRecordDelete(base_type);
  }
  CollectCXXVirtualBases(str);
  CopyCXXBaseVirtualMembers(str);
  LayoutCXXBaseSpecifiers(str);
  ApplyCXXMemberUsingDeclarations(parser, str, source_struct, source_args);

  // Member function bodies are cloned in a second pass, after every member
  // function signature has been added to `str`, so that a member's body may
  // reference other members declared later in the class (e.g. `operator=`
  // calling a later-declared `emplace`).
  Vector pending_member_bodies;
  VectorInit(&pending_member_bodies);
  for (size_t i = 0; i < source_struct->members.length; i++) {
    StructMember* member = source_struct->members.value.p[i];
    if (StructMemberIsNestedType(member)) {
      TypeRecord* nested_type =
          SubstituteTemplateParameters(parser, member->symbol->type,
                                       source_args);
      Symbol* nested_symbol =
          NewSymbol(member->symbol->name.value, nested_type, STO(typedef));
      nested_symbol->location = member->symbol->location;
      StructMember* nested_member = NewStructMember(nested_symbol);
      nested_member->access = member->access;
      AddStructMember(parser, str, nested_member);
      continue;
    }
    if (member->is_member_function) {
      StructMember* instantiated = InstantiateTemplateMemberFunction(
          parser, str, member, source_args, &pending_member_bodies);
      StructMember* existing =
          MapFindPointerKey(&str->symbol_table, &instantiated->symbol->name);
      if (existing != NULL) {
        AppendStructMemberOverload(parser, str, existing, instantiated);
      } else {
        AddStructMember(parser, str, instantiated);
      }
      continue;
    }
    TypeRecord* member_type =
        SubstituteTemplateParameters(parser, member->symbol->type,
                                     source_args);
    TypeRecordCalculateSize(member_type);
    Symbol* member_symbol =
        NewSymbol(member->symbol->name.value, member_type, member->symbol->storage);
    member_symbol->location = member->symbol->location;
    member_symbol->flags = member->symbol->flags;
    member_symbol->value = member->symbol->value;
    member_symbol->dependent_value_template_parameter_index =
        member->symbol->dependent_value_template_parameter_index;
    SubstituteDependentSymbolValue(member_symbol, source_args);
    StructMember* instantiated = NewStructMember(member_symbol);
    instantiated->default_initializer =
        CloneCXXDefaultMemberInitializer(member->default_initializer);
    instantiated->access = member->access;
    instantiated->is_anon = member->is_anon;
    instantiated->is_static = member->is_static;
    instantiated->is_mutable = member->is_mutable;
    instantiated->is_member_function = member->is_member_function;
    instantiated->is_using_declaration = member->is_using_declaration;
    instantiated->bit_size = member->bit_size;
    instantiated->bit_offset = member->bit_offset;
    instantiated->cxx_vcall_offset = member->cxx_vcall_offset;
    if (!instantiated->is_static && !instantiated->is_using_declaration &&
        !StructMemberIsNestedType(instantiated)) {
      AlignNextOffset(str, member_type);
      instantiated->byte_offset = str->next_offset;
    } else {
      instantiated->byte_offset = member->byte_offset;
    }
    instantiated->index = str->members.length;
    AddStructMember(parser, str, instantiated);
    if (!instantiated->is_static && !instantiated->is_using_declaration &&
        !StructMemberIsNestedType(instantiated)) {
      UpdateStructSize(str, member_type, str->is_union);
    }
  }
  FinalizeStructAlignment(str);
  TypeRecordCalculateSize(type);
  ComputeCXXAggregateStatus(str);
  AddImplicitCXXSpecialMembers(parser, str, tag);
  AddImplicitCXXDestructorIfNeeded(parser, str, tag);
  AddImplicitCXXDeductionGuides(str, tag);
  // Second pass: with the class fully formed (all members, layout, and implicit
  // special members in place), clone the deferred member function bodies so a
  // body may reference any other member regardless of declaration order.
  for (size_t i = 0; i < pending_member_bodies.length; i++) {
    PendingMemberBody* pmb = pending_member_bodies.value.p[i];
    CloneInstantiatedMemberFunctionBody(parser, str, pmb->symbol,
                                        pmb->template_definition,
                                        pmb->substitution_source, source_args);
    free(pmb);
  }
  VectorDestruct(&pending_member_bodies);
  InstantiateTemplateFriendFunctions(parser, str, source_struct, source_args);
  StringDestruct(&instantiated_name);
  if (partial_args != NULL) {
    VectorDeleteWithContents(partial_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return TypeRecordCopy(type);
}

/* Public entry point: instantiate class template `templ` with `args`. */
TypeRecord* TypeInstantiateClassTemplate(Syntax* syntax, Symbol* templ,
                                         Vector* args) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* type = InstantiateSimpleClassTemplate(&parser, templ, args);
  TypeParserDestruct(&parser);
  return type;
}

/* See TypeMaterializeClassTemplateSpecialization.  Recurses through
 * pointer/reference spines so a capture field typed `variant<int,long>*` is
 * rewritten to point at the concrete specialization. */
TypeRecord* TypeMaterializeClassTemplateSpecialization(Syntax* syntax,
                                                       TypeRecord* type) {
  if (syntax == NULL || type == NULL || !CompilerIsCXX()) {
    return type;
  }
  if (TypeIsPointer(type) || TypeIsReference(type)) {
    TypeRecord* next =
        TypeMaterializeClassTemplateSpecialization(syntax, type->next);
    if (next == type->next) {
      return type;
    }
    TypeRecord* copy = TypeRecordCopy(type);
    TypeRecordIncRef(next);
    TypeRecordDelete(copy->next);
    copy->next = next;
    copy->type = next != NULL ? next->type : copy->type;
    return TypeRecordCalculateSize(copy);
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return type;
  }
  Struct* str = type->info.struct_info;
  // Concrete specializations are not flagged as templates; only the primary
  // (still carrying unresolved-looking template_arguments) needs materializing.
  if (!str->is_template || str->tag_symbol == NULL) {
    return type;
  }
  Symbol* origin = type->template_origin;
  Vector* args = type->template_arguments;
  if (origin == NULL && str->tag_symbol->type != NULL) {
    origin = str->tag_symbol->type->template_origin;
    if (args == NULL) {
      args = str->tag_symbol->type->template_arguments;
    }
  }
  if (origin == NULL && str->tag_symbol->flags.is_template) {
    origin = str->tag_symbol;
  }
  if (origin == NULL || args == NULL ||
      TemplateArgumentVectorContainsTemplateParameter(args)) {
    return type;
  }
  TypeRecord* concrete = TypeInstantiateClassTemplate(syntax, origin, args);
  if (concrete == NULL) {
    return type;
  }
  concrete->qualifiers |= type->qualifiers;
  return concrete;
}

bool SymbolIsInStdNamespace(Symbol* symbol) {
  return symbol != NULL && symbol->namespace_ != NULL &&
         symbol->namespace_->qualified_name.value != NULL &&
         strcmp(symbol->namespace_->qualified_name.value, "std") == 0;
}

static bool CXXSymbolIsStdInitializerListTemplate(Symbol* symbol) {
  return symbol != NULL && strcmp(symbol->name.value, "initializer_list") == 0 &&
         SymbolIsInStdNamespace(symbol);
}

static bool CXXSymbolNamesStdInitializerList(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  const char* name = symbol->name.value;
  if (name == NULL ||
      (strcmp(name, "initializer_list") != 0 &&
       strncmp(name, "initializer_list<", 17) != 0)) {
    return false;
  }
  return symbol->namespace_ == NULL ||
         strcmp(symbol->namespace_->qualified_name.value, "std") == 0;
}

bool TypeIsCXXInitializerList(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL) {
    return false;
  }
  if (CXXSymbolIsStdInitializerListTemplate(type->template_origin)) {
    return true;
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_symbol == NULL) {
    return false;
  }
  Symbol* tag = type->info.struct_info->tag_symbol;
  if (CXXSymbolNamesStdInitializerList(tag)) {
    return true;
  }
  if (CXXSymbolIsStdInitializerListTemplate(tag->type != NULL
                                                ? tag->type->template_origin
                                                : NULL)) {
    return true;
  }
  return CXXSymbolIsStdInitializerListTemplate(type->template_origin);
}

/* Public: the element type T of a `std::initializer_list<T>` type, or NULL. */
TypeRecord* TypeCXXInitializerListElement(TypeRecord* type) {
  if (!TypeIsCXXInitializerList(type) || type->template_arguments == NULL ||
      type->template_arguments->length != 1) {
    return NULL;
  }
  TemplateArgument* arg = type->template_arguments->value.p[0];
  return arg != NULL && arg->kind == kTemplateParameterType ? arg->type : NULL;
}

/* Public: instantiate `std::initializer_list<element_type>` (used for braced
 * initializer expressions), looking up the std template. Returns NULL if the
 * std type is unavailable. */
TypeRecord* TypeInstantiateCXXInitializerList(Syntax* syntax,
                                              TypeRecord* element_type) {
  if (!CompilerIsCXX() || syntax == NULL || element_type == NULL) {
    return NULL;
  }
  String std_name;
  StringInit(&std_name, "std");
  Namespace* std_ns = NamespaceFindChild(compiler->global_namespace, &std_name);
  StringDestruct(&std_name);
  if (std_ns == NULL) {
    return NULL;
  }
  String initializer_list_name;
  StringInit(&initializer_list_name, "initializer_list");
  Symbol* templ = NamespaceFindSymbol(std_ns, &initializer_list_name);
  StringDestruct(&initializer_list_name);
  if (!CXXSymbolIsStdInitializerListTemplate(templ) || !templ->flags.is_template) {
    return NULL;
  }
  Vector* args = NewVector();
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = TypeRecordCopy(element_type);
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  VectorAppend(args, arg);
  TypeRecord* type = TypeInstantiateClassTemplate(syntax, templ, args);
  VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return type;
}

/* Public: look up a C++20 comparison-category type (e.g. "strong_ordering")
 * declared in namespace std.  Returns the struct TypeRecord, or NULL if the
 * type is not visible (typically because <compare> was not included). */
TypeRecord* TypeFindCXXComparisonCategory(const char* category_name) {
  if (!CompilerIsCXX() || compiler == NULL || category_name == NULL) {
    return NULL;
  }
  String std_name;
  StringInit(&std_name, "std");
  Namespace* std_ns = NamespaceFindChild(compiler->global_namespace, &std_name);
  StringDestruct(&std_name);
  if (std_ns == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, category_name);
  Symbol* tag = NamespaceFindTag(std_ns, &name);
  StringDestruct(&name);
  if (tag == NULL || tag->type == NULL || !TypeIsStructOrUnion(tag->type)) {
    return NULL;
  }
  return tag->type;
}

/* Public entry point: instantiate function template `templ` with `args`. */
Symbol* TypeInstantiateFunctionTemplate(Syntax* syntax, Symbol* templ,
                                        Vector* args) {
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, args)) {
    return templ;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  Symbol* symbol = InstantiateSimpleFunctionTemplate(&parser, templ, args);
  TypeParserDestruct(&parser);
  return symbol;
}

/* True if `alias` is an alias template whose right-hand side is itself a class
 * template specialization (e.g. `using X = vector<T>;`), as opposed to a plain
 * type alias. Such aliases participate in class-template instantiation/CTAD. */
static bool CXXAliasTemplatePatternNamesClassTemplate(Symbol* alias) {
  if (!CompilerIsCXX() || alias == NULL || !alias->flags.is_template ||
      !StorageIs(alias->storage, STO(typedef)) || alias->type == NULL ||
      !TypeIsStructOrUnion(alias->type) ||
      alias->type->template_origin == NULL ||
      alias->type->template_arguments == NULL ||
      alias->type->template_origin->type == NULL ||
      !TypeIsStructOrUnion(alias->type->template_origin->type) ||
      alias->type->template_origin->type->info.struct_info == NULL ||
      !alias->type->template_origin->type->info.struct_info->is_template) {
    return false;
  }
  size_t expected =
      (size_t)alias->type->template_origin->type->info.struct_info
          ->template_parameter_count;
  if (alias->type->template_arguments->length != expected) {
    return false;
  }
  return TemplateArgumentVectorContainsTemplateParameter(
      alias->type->template_arguments);
}

/* Mark `type` as a CTAD placeholder for an alias template: point it at the
 * alias's underlying class template and copy the alias's argument pattern, so
 * deduction runs against the alias rather than the underlying template. */
static void SetCXXAliasTemplatePlaceholderOrigin(Symbol* alias,
                                                 TypeRecord* type) {
  if (!CXXAliasTemplatePatternNamesClassTemplate(alias) || type == NULL) {
    return;
  }
  if (type->template_arguments != NULL) {
    VectorDeleteWithContents(type->template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  type->template_arguments = TemplateArgumentVectorCopy(alias->type->template_arguments);
  type->template_origin = alias;
  type->info.struct_info = alias->type->template_origin->type->info.struct_info;
}

/* True if an alias template argument is a pack expansion (`Ts...`), reporting
 * the referenced pack parameter index and its kind. */
static bool AliasTemplateArgumentIsPackExpansion(TemplateArgument* arg,
                                                 int* pack_index,
                                                 TemplateParameterKind* kind) {
  if (arg == NULL || !arg->is_pack_expansion) {
    return false;
  }
  if (arg->kind == kTemplateParameterType) {
    int index = -1;
    if (TypeIsTemplateParameterPlaceholder(arg->type, &index) && index >= 0) {
      if (pack_index != NULL) {
        *pack_index = index;
      }
      if (kind != NULL) {
        *kind = kTemplateParameterType;
      }
      return true;
    }
  }
  if (arg->kind == kTemplateParameterNonType &&
      arg->template_parameter_index >= 0) {
    if (pack_index != NULL) {
      *pack_index = arg->template_parameter_index;
    }
    if (kind != NULL) {
      *kind = kTemplateParameterNonType;
    }
    return true;
  }
  return false;
}

/* Fill in an alias template's argument list from the supplied `actuals`,
 * applying defaults and gathering a trailing parameter pack, so the count
 * matches the alias's parameters. Returns NULL on an arity mismatch. */
static Vector* CompleteAliasTemplateArguments(Symbol* alias, Vector* actuals) {
  if (!CXXAliasTemplatePatternNamesClassTemplate(alias)) {
    return NULL;
  }
  int max_index = -1;
  int pack_index = -1;
  TemplateParameterKind pack_kind = kTemplateParameterType;
  for (size_t i = 0; i < alias->type->template_arguments->length; i++) {
    TemplateArgument* pattern_arg = alias->type->template_arguments->value.p[i];
    MaxTemplateParameterIndexInArgument(pattern_arg, &max_index);
    int current_pack_index = -1;
    TemplateParameterKind current_pack_kind = kTemplateParameterType;
    if (AliasTemplateArgumentIsPackExpansion(pattern_arg, &current_pack_index,
                                             &current_pack_kind)) {
      if (pack_index >= 0 && pack_index != current_pack_index) {
        return NULL;
      }
      pack_index = current_pack_index;
      pack_kind = current_pack_kind;
    }
  }
  if (max_index < 0) {
    return NULL;
  }
  size_t parameter_count = (size_t)max_index + 1;
  size_t actual_count = actuals != NULL ? actuals->length : 0;
  if (pack_index < 0 && actual_count != parameter_count) {
    return NULL;
  }
  if (pack_index >= 0 && actual_count < (size_t)pack_index) {
    return NULL;
  }

  Vector* completed = NewVector();
  for (size_t i = 0; i < parameter_count; i++) {
    if (pack_index >= 0 && i == (size_t)pack_index) {
      TemplateArgument* pack = NewEmptyPackTemplateArgument(pack_kind);
      for (size_t j = i; j < actual_count; j++) {
        TemplateArgument* actual = actuals->value.p[j];
        if (actual == NULL || actual->kind != pack_kind) {
          TemplateArgumentDelete(pack);
          VectorDeleteWithContents(completed,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
          return NULL;
        }
        if (actual->pack_arguments != NULL) {
          for (size_t k = 0; k < actual->pack_arguments->length; k++) {
            VectorAppend(pack->pack_arguments,
                         TemplateArgumentCopy(
                             actual->pack_arguments->value.p[k]));
          }
        } else {
          VectorAppend(pack->pack_arguments, TemplateArgumentCopy(actual));
        }
      }
      VectorAppend(completed, pack);
      continue;
    }

    if (actuals == NULL || i >= actual_count) {
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    VectorAppend(completed, TemplateArgumentCopy(actuals->value.p[i]));
  }
  return completed;
}

/* If `alias` is an alias template whose pattern names a class template (e.g.
 * `template<class T> using Vec = vector<T>;`), instantiate it by substituting
 * the alias arguments into the underlying template's argument list and
 * instantiating that class template. Returns NULL if `alias` is not such an
 * alias (so the caller falls through to normal class instantiation). */
static TypeRecord* InstantiateAliasClassTemplate(TypeParser* parser,
                                                 Symbol* alias,
                                                 Vector* args) {
  if (!CXXAliasTemplatePatternNamesClassTemplate(alias) ||
      alias->type->template_origin == NULL ||
      alias->type->template_arguments == NULL) {
    return NULL;
  }
  Vector* completed_alias_args = CompleteAliasTemplateArguments(alias, args);
  if (completed_alias_args == NULL) {
    return NULL;
  }
  Vector* underlying_args =
      SubstituteTemplateArgumentVectorForTypes(parser,
                                              alias->type->template_arguments,
                                              completed_alias_args);
  TypeRecord* instantiated =
      InstantiateSimpleClassTemplate(parser, alias->type->template_origin,
                                     underlying_args);
  VectorDeleteWithContents(underlying_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  VectorDeleteWithContents(completed_alias_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return instantiated;
}

/* Public: build a CTAD placeholder type from a class-template (or alias
 * template) symbol, used when a variable is declared with just the template
 * name (`Foo x = ...;`). Returns NULL if `symbol` is not a class template. */
TypeRecord* TypeClassTemplatePlaceholderFromSymbol(Symbol* symbol) {
  if (!CompilerIsCXX() || symbol == NULL || !symbol->flags.is_template ||
      symbol->type == NULL || !TypeIsStructOrUnion(symbol->type)) {
    return NULL;
  }
  TypeRecord* type = TypeRecordCopy(symbol->type);
  if (CXXAliasTemplatePatternNamesClassTemplate(symbol)) {
    SetCXXAliasTemplatePlaceholderOrigin(symbol, type);
  } else if (type->info.struct_info != NULL &&
             type->info.struct_info->is_template) {
    type->template_origin = symbol;
  }
  if (!TypeIsClassTemplatePlaceholder(type)) {
    TypeRecordDelete(type);
    return NULL;
  }
  return type;
}

static bool CurrentClassNameMatchesTypeName(Struct* owner, String* name) {
  if (!CompilerIsCXX() || owner == NULL || owner->tag_name == NULL ||
      name == NULL) {
    return false;
  }
  if (StringEqualString(owner->tag_name, name)) {
    return true;
  }
  const char* template_args = strchr(owner->tag_name->value, '<');
  if (template_args == NULL) {
    return false;
  }
  size_t base_length = (size_t)(template_args - owner->tag_name->value);
  return name->length == base_length &&
         strncmp(name->value, owner->tag_name->value, base_length) == 0;
}

// The class whose members are currently being parsed.  While a class body is
// parsed the owner lives on the TypeParser, but a member function *body* parses
// its local declarations through freshly-initialized TypeParsers that do not
// carry it; recover it from the member function then in flight (which does, via
// cxx_member_owner) so a self-type reference inside the body still resolves.
static Struct* CurrentClassBeingParsed(TypeParser* parser) {
  if (parser != NULL && parser->cxx_member_owner != NULL) {
    return parser->cxx_member_owner;
  }
  // Only consult the current function while genuinely parsing statements inside
  // a function body (block scope): `compiler->current_function` is not cleared
  // between top-level declarations, so at file scope it may still point at the
  // last member function parsed, which would spuriously match a namespace-scope
  // use of that class's name (e.g. in an alias `using A = ThatClass<...>;`).
  if (parser != NULL && parser->context == kParsingBlockScope &&
      compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    return compiler->current_function->info.function.cxx_member_owner;
  }
  return NULL;
}

static TypeRecord* ParseCurrentClassTemplateType(TypeParser* parser,
                                                 String* name) {
  Struct* owner = CurrentClassBeingParsed(parser);
  if (!CurrentClassNameMatchesTypeName(owner, name) ||
      owner->tag_symbol == NULL || owner->tag_symbol->type == NULL ||
      !TypeIsStructOrUnion(owner->tag_symbol->type)) {
    return NULL;
  }
  LexNextToken(parser->lex);
  Vector* args = NULL;
  if (LexLookingAt(parser->lex, TOK(less))) {
    args = SyntaxParseTemplateArgumentList(parser->syntax, TC(decl));
  }
  TypeRecord* type_record = TypeRecordCopy(owner->tag_symbol->type);
  if (args != NULL) {
    // Inside a partial specialization, the owner's tag is the specialization
    // itself (whose struct is not marked is_template), so using it as the
    // template origin would prevent InstantiateSimpleClassTemplate from
    // choosing the right specialization for a *different* argument list (e.g.
    // the recursive member `storage<I + 1, Rest...>` inside
    // `storage<I, T, Rest...>`). Resolve the primary class template by name and
    // use it as the origin so such references instantiate correctly.
    Symbol* template_origin = owner->tag_symbol;
    Symbol* primary = SyntaxFindSymbol(parser->syntax, name);
    if (primary != NULL && primary->flags.is_template &&
        primary->type != NULL && TypeIsStructOrUnion(primary->type) &&
        primary->type->info.struct_info != NULL &&
        primary->type->info.struct_info->is_template) {
      template_origin = primary;
    }
    type_record->template_origin = template_origin;
    type_record->template_arguments = args;
    args = NULL;
  }
  if (args != NULL) {
    VectorDestructWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
  }
  return type_record;
}

// Parse a type-specifier.  This might also be a typedef reference which
// contains a full TypeRecord.
static PartialTypeSpecifier ParseTypeSpecifier(TypeParser* parser, bool allow_typedef) {
  PartialTypeSpecifier result;
  result.error = false;
  
  Type type = kTypeImplicit;
  Qualifiers quals = kQualPlain;
  Lex* lex = parser->lex;
  TypeRecord* type_record = NULL;

  Token tok = lex->current_token;
  bool found = false;
  
  if (parser->found_void) {
    // Special handling for already-consumed void type.  This can
    // happen inside a function prototype.
    type |= kTypeVoid;
    parser->found_void = false;
    found = true;
  } else {
    // Check for known type.
    for (int i = 0; type_map[i].token != TOK(bad); i++) {
      if (type_map[i].token == tok) {
        if (tok == TOK(auto) && !CompilerIsCXX()) {
          break;
        }
        // First check for a typedef name reference.
        LexNextToken(lex);
        
        type |= type_map[i].type;
        found = true;
        break;
      }
    }
  }

  // If we didn't find a known type, look for qualifiers and typedef
  // name.
  if (!found) {
    if (LexMatch(lex, TOK(const))) {
      quals |= kQualConst;
    } else if (LexMatch(lex, TOK(volatile))) {
      quals |= kQualVolatile;
    } else if (LexMatch(lex, TOK(restrict))) {
      quals |= kQualRestrict;
    } else if (CompilerIsCXX() && LexLookingAt(lex, TOK(decltype))) {
      type_record = ParseCXXDecltypeSpecifier(parser);
      type |= type_record->type;
    } else if (CompilerIsCXX() && allow_typedef &&
               LexMatch(lex, TOK(typename))) {
      FullyQualifiedIdentifier typename_name;
      FullyQualifiedIdentifierInit(&typename_name);
      if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
              parser->syntax, &typename_name, TC(decl))) {
        SyntaxError(parser->syntax, "Expected qualified type name after typename");
      } else if (!typename_name.is_qualified) {
        SyntaxError(parser->syntax, "typename requires a qualified type name");
      } else {
        bool handled_dependent_template_member = false;
        if (parser->syntax->current_template_parameters != NULL &&
            typename_name.components.length >= 2 &&
            typename_name.template_arguments.length ==
                typename_name.components.length) {
          size_t base_index = typename_name.components.length - 2;
          Vector* parsed_args =
              typename_name.template_arguments.value.p[base_index];
          if (parsed_args != NULL) {
            FullyQualifiedIdentifier prefix;
            FullyQualifiedIdentifierInit(&prefix);
            prefix.absolute = typename_name.absolute;
            prefix.is_qualified = prefix.absolute || base_index > 0;
            for (size_t i = 0; i <= base_index; i++) {
              String* component = typename_name.components.value.p[i];
              if (prefix.spelling.length != 0 || prefix.absolute) {
                StringAppend(&prefix.spelling, "::");
              }
              StringAppendString(&prefix.spelling, component);
              VectorAppend(&prefix.components, NewString(component->value));
              Vector* component_args = typename_name.template_arguments.value.p[i];
              VectorAppend(&prefix.template_arguments,
                           TemplateArgumentVectorCopy(component_args));
            }
            Symbol* base = SyntaxFindQualifiedSymbol(parser->syntax, &prefix);
            if (base != NULL && base->flags.is_template) {
              String* member_name =
                  typename_name.components.value.p[
                      typename_name.components.length - 1];
            type_record = NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
            type_record->template_origin = base;
            type_record->template_arguments =
                TemplateArgumentVectorCopy(parsed_args);
            type_record->dependent_member_name =
                NewString(member_name->value);
            type |= type_record->type;
            handled_dependent_template_member = true;
            }
            FullyQualifiedIdentifierDestruct(&prefix);
          }
        }
        if (handled_dependent_template_member) {
          // Keep the dependent member lookup for template instantiation.
        } else {
        Symbol* symbol =
            SyntaxFindQualifiedSymbol(parser->syntax, &typename_name);
        if (symbol != NULL && StorageIs(symbol->storage, STO(typedef))) {
          type_record = TypeRecordCopy(symbol->type);
          type |= type_record->type;
        } else if (typename_name.components.length == 2) {
          String* base_name = typename_name.components.value.p[0];
          Symbol* base = SyntaxFindSymbol(parser->syntax, base_name);
          if (base != NULL && base->flags.is_template_parameter &&
              base->flags.is_template_type_parameter &&
              base->template_parameter_index >= 0) {
            String* member_name = typename_name.components.value.p[1];
            type_record =
                NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
            type_record->template_parameter_index =
                base->template_parameter_index;
            type_record->dependent_member_name =
                NewString(member_name->value);
            type |= type_record->type;
          } else {
            SyntaxError(parser->syntax, "Unknown type name %s",
                        typename_name.spelling.value);
          }
        } else {
          SyntaxError(parser->syntax, "Unknown type name %s",
                      typename_name.spelling.value);
        }
        }
      }
      FullyQualifiedIdentifierDestruct(&typename_name);
    } else if (allow_typedef && SyntaxCurrentTokenStartsQualifiedName(parser->syntax)) {
      FullyQualifiedIdentifier typedef_name;
      FullyQualifiedIdentifierInit(&typedef_name);
      SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
          parser->syntax, &typedef_name, TC(decl));
      Symbol* symbol = SyntaxFindQualifiedSymbol(parser->syntax, &typedef_name);
      if (symbol != NULL && StorageIs(symbol->storage, STO(typedef))) {
        Vector* args = NULL;
        if (symbol->flags.is_template) {
          if (typedef_name.template_arguments.length > 0) {
            Vector* parsed_args =
                typedef_name.template_arguments.value.p[
                    typedef_name.template_arguments.length - 1];
            args = TemplateArgumentVectorCopy(parsed_args);
          } else if (LexLookingAt(lex, TOK(less))) {
            args = SyntaxParseTemplateArgumentList(parser->syntax, TC(decl));
          }
        }
        if (symbol->flags.is_template && args != NULL &&
            !parser->syntax->parsing_template_declaration &&
            TypeIsStructOrUnion(symbol->type)) {
          type_record = InstantiateSimpleClassTemplate(parser, symbol, args);
        } else {
          type_record = TypeRecordCopy(symbol->type);
          if (symbol->flags.is_template && args == NULL &&
              !parser->syntax->parsing_template_declaration &&
              TypeIsStructOrUnion(symbol->type)) {
            if (CXXAliasTemplatePatternNamesClassTemplate(symbol)) {
              SetCXXAliasTemplatePlaceholderOrigin(symbol, type_record);
            } else {
              type_record->template_origin = symbol;
            }
          }
          if (symbol->flags.is_template && args != NULL &&
              parser->syntax->current_template_parameters != NULL) {
            type_record->template_origin = symbol;
            type_record->template_arguments = args;
            args = NULL;
          }
        }
        type |= type_record->type;
        if (args != NULL) {
          VectorDestructWithContents(args,
                                     (VectorElementDestructor)TemplateArgumentDelete,
                                     /*free_element=*/false);
        }
      } else {
        SyntaxError(parser->syntax, "Unknown type name %s",
                    typedef_name.spelling.value);
      }
      FullyQualifiedIdentifierDestruct(&typedef_name);
    } else if (allow_typedef && tok == TOK(identifier)) {
      // Identifier.  If this is a known typedef name consume it
      // and keep the type.
      String typedef_name;
      StringInit(&typedef_name, lex->spelling.value);
      Symbol* symbol = SyntaxFindSymbol(parser->syntax, &typedef_name);
      TypeRecord* current_class_type =
          ParseCurrentClassTemplateType(parser, &typedef_name);
      if (current_class_type != NULL) {
        type_record = current_class_type;
        type |= type_record->type;
      } else if (symbol != NULL) {
        // Reference to a typedef?
        if (StorageIs(symbol->storage, STO(typedef))) {
          LexNextToken(lex);
          Vector* args = NULL;
          if (symbol->flags.is_template && LexLookingAt(lex, TOK(less))) {
            args = SyntaxParseTemplateArgumentList(parser->syntax, TC(decl));
          }
          if (symbol->flags.is_template && args != NULL &&
              !parser->syntax->parsing_template_declaration &&
              TypeIsStructOrUnion(symbol->type)) {
            type_record = InstantiateSimpleClassTemplate(parser, symbol, args);
          } else {
            type_record = TypeRecordCopy(symbol->type);
            if (symbol->flags.is_template && args == NULL &&
                !parser->syntax->parsing_template_declaration &&
                TypeIsStructOrUnion(symbol->type)) {
              if (CXXAliasTemplatePatternNamesClassTemplate(symbol)) {
                SetCXXAliasTemplatePlaceholderOrigin(symbol, type_record);
              } else {
                type_record->template_origin = symbol;
              }
            }
            // A template alias used with arguments inside another template
            // declaration is a dependent alias template-id: record the alias as
            // origin and keep the supplied arguments so it is expanded (by
            // substituting the alias pattern) once they become concrete.  This
            // must also apply when the alias's pattern is not itself a
            // class/union (e.g. `using alt_t = typename alt<I, V>::type;`),
            // otherwise the arguments would be dropped and the alias pattern's
            // own parameters would leak into the enclosing template.
            if (symbol->flags.is_template && args != NULL &&
                parser->syntax->parsing_template_declaration) {
              type_record->template_origin = symbol;
              if (type_record->template_arguments != NULL) {
                VectorDeleteWithContents(
                    type_record->template_arguments,
                    (VectorElementDestructor)TemplateArgumentDelete,
                    /*free_element=*/false);
              }
              type_record->template_arguments = args;
              args = NULL;
            }
          }
          type |= type_record->type;
          if (args != NULL) {
            VectorDestructWithContents(args,
                                       (VectorElementDestructor)TemplateArgumentDelete,
                                       /*free_element=*/false);
          }
        }
      } else if (CompilerIsCXX()) {
        Symbol* tag = SyntaxFindTag(parser->syntax, &typedef_name);
        if (tag != NULL && tag->type != NULL &&
            TypeIsStructOrUnion(tag->type)) {
          LexNextToken(lex);
          type_record = TypeRecordCopy(tag->type);
          type |= type_record->type;
        }
      }
      StringDestruct(&typedef_name);
    }
  }

  // Check for struct, union or enum and parse it if necessary.
  if (type_record == NULL &&
      (type & (kTypeStruct | kTypeUnion | kTypeEnum)) != 0) {
    
    Symbol* tag;
    TypeParser composite_parser;
    TypeParserInit(&composite_parser, parser->lex, parser->syntax,
                   STO(implicit), kParsingStructOrUnion);
    
    if ((type & (kTypeStruct | kTypeUnion)) != 0) {
      bool is_union = (type & kTypeUnion) != 0;
      bool is_class = tok == TOK(class);
      tag = TypeParserParseStruct(&composite_parser, is_union, is_class);
    } else {
      tag = TypeParserParseEnum(&composite_parser);
    }
    if (tag != NULL) {
      // We need to copy the type record because it is held in the
      // struct tag and we need to apply our qualifiers to it for this
      // type definition.  For example, this might be:
      //   const struct Foo;
      // and the type record will be the one inside the symbol for 'Foo'
      type_record = TypeRecordCopy(tag->type);
      type |= type_record->type;
      
      if (LexMatch(lex, TOK(const))) {
        quals |= kQualConst;
      } else if (LexMatch(lex, TOK(volatile))) {
        quals |= kQualVolatile;
      } else if (LexMatch(lex, TOK(restrict))) {
        quals |= kQualRestrict;
      }
      type_record->qualifiers |= quals;
    }
    TypeParserDestruct(&composite_parser);
  }

  result.type = type;
  result.quals = quals;
  result.type_record = type_record;
  return result;
}

// This is a list of all the valid type combinations.
// These come from the C99 spec, section 6.7.2.
static Type valid_types[] = {
  kTypeVoid,
  
  kTypeChar,
  kTypeChar | kTypeSigned,
  kTypeChar | kTypeUnsigned,
  
  kTypeShort,
  kTypeShort | kTypeSigned,
  kTypeShort | kTypeInt,
  kTypeShort | kTypeSigned | kTypeInt,
  kTypeShort | kTypeUnsigned,
  kTypeShort | kTypeUnsigned | kTypeInt,
  
  kTypeInt,
  kTypeInt | kTypeSigned,
  kTypeSigned,
  kTypeUnsigned,
  kTypeUnsigned | kTypeInt,
  
  kTypeLong,
  kTypeLong | kTypeInt,
  kTypeLong | kTypeSigned,
  kTypeLong | kTypeSigned | kTypeInt,
  kTypeLong | kTypeUnsigned,
  kTypeLong | kTypeUnsigned | kTypeInt,
  
  kTypeLongLong,
  kTypeLongLong | kTypeSigned,
  kTypeLongLong | kTypeInt,
  kTypeLongLong | kTypeSigned | kTypeInt,
  kTypeLongLong | kTypeUnsigned,
  kTypeLongLong | kTypeUnsigned | kTypeInt,
  
  kTypeFloat,
  
  kTypeDouble,
  kTypeDouble | kTypeLong,
  
  kTypeBool,

  kTypeAuto,
  
  kTypeStruct,
  
  kTypeUnion,
  
  kTypeEnum,
};

#define NUM_VALID_TYPES (sizeof(valid_types)/sizeof(valid_types[0]))

static bool IsValidType(Type t) {
  for (size_t i = 0; i < NUM_VALID_TYPES; i++) {
    if (t == valid_types[i]) {
      return true;
    }
  }
  return false;
}

static bool IsValidQualiferCombo(Qualifiers q1, Qualifiers q2) {
  return (q1 & q2) == 0;
}

static void TypeComboError1(Syntax* syntax, Type t1, Type t2) {
  String error;
  StringInit(&error, "");
  TypeToString(t1, &error);
  if (t2 != kTypeImplicit) {
    StringAppend(&error, "and ");
    TypeToString(t2, &error);
  }
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

static void TypeComboError2(Syntax* syntax, TypeRecord* t1, Type t2) {
  String error;
  StringInit(&error, "defined type ");
  TypeRecordToString(t1, &error);
  StringAppend(&error, "and ");
  TypeToString(t2, &error);
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

static void TypeComboError3(Syntax* syntax, TypeRecord* t1, TypeRecord* t2) {
  String error;
  StringInit(&error, "defined type ");
  TypeRecordToString(t1, &error);
  StringAppend(&error, "and defined type ");
  TypeRecordToString(t2, &error);
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

static void QualifierComboError(Syntax* syntax, Qualifiers q1, Qualifiers q2) {
  String error;
  StringInit(&error, "");
  QualifiersToString(q1, &error);
  StringAppend(&error, "and ");
  QualifiersToString(q2, &error);
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

// Type specifiers can be split into pieces.  For example you could
// have:
//
// int extern unsigned foo;
//
// Where the 'int' and 'unsigned' are split by a storage specifier.
// This function combines two type specifiers if it can and issues
// errors and warnings as necessary.
static PartialTypeSpecifier CombineTypeSpecifiers(Syntax* syntax,
                                                PartialTypeSpecifier* t1,
                                                PartialTypeSpecifier* t2) {
  PartialTypeSpecifier result = {0};
  // Check for a valid type.  You can't combine types that contain the
  // same bits:
  // e.g. short short
  // However, we need to handle 'long long'.
  if ((t1->type & kTypeLong) != 0 && (t2->type & kTypeLong) != 0) {
    t1->type &= ~kTypeLong;
    t1->type |= kTypeLongLong;
    t2->type = kTypeImplicit;
  }
  if (t1->type_record != NULL || t2->type_record != NULL) {
    // Either t1->type_record or t2->type_record is non-NULL. Put the non-NULL
    // one in t1 so qualifier-only combinations like `const T` and `T const`
    // can be handled before the primitive valid-type check sees T's internal
    // unknown marker.
    if (t1->type_record == NULL) {
      PartialTypeSpecifier* tmp = t1;
      t1 = t2;
      t2 = tmp;
    }
    if (t2->type_record != NULL) {
      TypeComboError3(syntax, t1->type_record, t2->type_record);
      result.error = true;
    } else if (t2->type != kTypeImplicit) {
      TypeComboError2(syntax, t1->type_record, t2->type);
      result.error = true;
    }
    if (!IsValidQualiferCombo(t1->quals, t2->quals)) {
      QualifierComboError(syntax, t1->quals, t2->quals);
      result.error = true;
    }
    result.type = t1->type;
    result.quals = t1->quals | t2->quals;
    result.type_record = t1->type_record;
    return result;
  }
  result.type = t1->type | t2->type;
  
  bool type_ok = result.type == kTypeImplicit ||
                 (t1->type & t2->type) == 0;
  if (type_ok && result.type != kTypeImplicit) {
    type_ok = IsValidType(result.type);
  }
  if (!type_ok) {
    TypeComboError1(syntax, t1->type, t2->type);
    result.error = true;
  }
  
  // Convert 'long double' to kTypeLongDouble.
  if ((result.type & (kTypeLong | kTypeDouble)) == (kTypeLong | kTypeDouble)) {
    result.type &= ~(kTypeLong | kTypeDouble);
    result.type |= kTypeLongDouble;
  }
  
  // Can't combine qualifiers if they are the same.
  // e.g. const const
  if (!IsValidQualiferCombo(t1->quals, t2->quals)) {
    QualifierComboError(syntax, t1->quals, t2->quals);
    result.error = true;
  }
  result.quals = t1->quals | t2->quals;
  result.type_record = NULL;

  if (result.error) {
    return result;
  }
  return result;
}

PartialTypeSpecifier TypeParserParseAndCombineTypes(TypeParser* parser,
                                                   PartialTypeSpecifier* prev) {
  PartialTypeSpecifier curr = ParseTypeSpecifier(parser, prev->type == kTypeImplicit);
  if (prev->type == kTypeImplicit && prev->quals == kQualPlain) {
    return curr;
  }
  
  return CombineTypeSpecifiers(parser->syntax, prev, &curr);
}

// Given a ParseTypeSpecifier, build a TypeRecord.
TypeRecord* TypeParserBuildTypeRecord(TypeParser* parser, PartialTypeSpecifier* type) {
  if (type->error) {
    return NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }
  if (type->type_record == NULL) {
    if (type->type == kTypeImplicit) {
      return NULL;
    }
    return NewTypeRecordWithSize(type->type, type->quals);
  } else {
    // Add qualifiers to typedef copy.
    type->type_record->qualifiers |= type->quals;
    return type->type_record;
  }
}

TypeRecord* TypeParserParseType(TypeParser* parser, bool needed) {
  Syntax* syntax = parser->syntax;
  
  PartialTypeSpecifier type_specifier = {
    .type = kTypeImplicit,
    .quals = kQualPlain,
    .type_record = NULL,
    .error = false };

  // Leading __attribute__((...)) specifiers (GCC extension) before the type.
  TypeParserSkipAttributes(parser);

  while (parser->found_void || SyntaxLookingAtType(syntax)) {
    PartialTypeSpecifier new_type_specifier = ParseTypeSpecifier(parser, type_specifier.type == kTypeImplicit);
    if (new_type_specifier.type == kTypeImplicit && new_type_specifier.quals == kQualPlain) {
      break;
    }
    if (type_specifier.type == kTypeImplicit && type_specifier.quals == kQualPlain) {
      type_specifier = new_type_specifier;
    } else {
      type_specifier = CombineTypeSpecifiers(parser->syntax, &type_specifier, &new_type_specifier);
    }
  }
  // No type?
  if (type_specifier.type == kTypeImplicit) {
    if (needed) {
      SyntaxError(parser->syntax, "Type expected");
      SyntaxRecover(parser->syntax, TC(semicolon) | TC(type));
      return NewTypeRecordWithSize(kTypeInt, kQualPlain);
    }
    return NULL;
  }
  return TypeParserBuildTypeRecord(parser, &type_specifier);
}

Symbol* TypeParserParseDeclarator(TypeParser* parser, TypeRecord* base_type) {
  if (base_type == NULL) {
    return NULL;
  }
  
  VectorClear(&parser->stack);
  parser->symbol = NULL;
  parser->base_type = base_type;
  parser->declarator_is_parameter_pack =
      CompilerIsCXX() && LexMatch(parser->lex, TOK(ellipsis));
  TypeParserParsePointer(parser);
  if (CompilerIsCXX() && LexMatch(parser->lex, TOK(ellipsis))) {
    parser->declarator_is_parameter_pack = true;
  }

  // Join all the type records together in reverse order.
  size_t i = parser->stack.length;
  TypeRecord* t = parser->base_type;
  while (i > 0) {
    TypeRecord* record = (TypeRecord*)parser->stack.value.p[i - 1];
    if (TypeIsFunction(record) && record->next != NULL) {
      record->type = record->next->type;
    } else {
      TypeRecordChain(record, t);
      record->type = t->type;
    }
    t = record;
    i--;
  }

  // Calculate the size of t, now that we have the complete chain.
  TypeRecordCalculateSize(t);

  if (parser->symbol != NULL) {
    SymbolSetType(parser->symbol, t);
  } else {
    // Invent a fake symbol.
    parser->symbol = NewSymbol(SyntaxFakeName(parser->syntax), t, STO(auto));
    parser->symbol->flags.invented = true;
  }
  parser->symbol->flags.is_parameter_pack =
      parser->declarator_is_parameter_pack;
  if (TypeIsFunction(parser->symbol->type) &&
      parser->declarator_template_arguments != NULL) {
    parser->symbol->type->template_arguments =
        parser->declarator_template_arguments;
    parser->declarator_template_arguments = NULL;
  }
  return parser->symbol;
}

// Skip any __attribute__((...)) specifiers (a GCC extension) that can appear
// in declarator positions (pointers, parenthesized declarators, type names).
// The attributes are parsed and discarded.  Returns true if at least one was
// seen.
bool TypeParserSkipAttributes(TypeParser* parser) {
  bool any = false;
  while (LexLookingAt(parser->lex, TOK(attribute)) ||
         SyntaxLookingAtCXXAttribute(parser->syntax)) {
    Vector attrs = {0};
    VectorInit(&attrs);
    if (LexMatch(parser->lex, TOK(attribute))) {
      SyntaxParseAttribute(parser->syntax, &attrs);
    } else {
      SyntaxParseCXXAttributes(parser->syntax, &attrs);
    }
    AttributeListDestruct(&attrs);
    any = true;
  }
  return any;
}

static Qualifiers ParseQualifiers(TypeParser* parser) {
  Qualifiers quals = kQualPlain;
  int num_consts = 0;
  int num_volatiles = 0;
  int num_restricts = 0;
  while (!LexEof(parser->lex)) {
    if (LexMatch(parser->lex, TOK(const))) {
      quals |= kQualConst;
      num_consts++;
    } else if (LexMatch(parser->lex, TOK(volatile))) {
      num_volatiles++;
      quals |= kQualVolatile;
    } else if (LexMatch(parser->lex, TOK(restrict))) {
      num_restricts++;
      quals |= kQualRestrict;
    } else if (LexLookingAt(parser->lex, TOK(attribute)) ||
               SyntaxLookingAtCXXAttribute(parser->syntax)) {
      TypeParserSkipAttributes(parser);
    } else {
      break;
    }
  }
  if (num_consts > 1 || num_volatiles > 1 || num_restricts > 1) {
    SyntaxError(parser->syntax, "Invalid pointer qualifier declaration");
  }
  return quals;
}

void TypeParserParsePointer(TypeParser* parser) {
  TypeParserSkipAttributes(parser);
  if (CompilerIsCXX() && LexMatch(parser->lex, TOK(ellipsis))) {
    parser->declarator_is_parameter_pack = true;
    TypeParserParsePointer(parser);
  } else if (LexMatch(parser->lex, TOK(star))) {
    Qualifiers quals = ParseQualifiers(parser);
    TypeParserParsePointer(parser);
    TypeRecord* p = NewPointerTypeRecord(quals);
    VectorAppend(&parser->stack, p);
  } else if (CompilerIsCXX() &&
             (LexLookingAt(parser->lex, TOK(amp)) ||
              LexLookingAt(parser->lex, TOK(ampamp)))) {
    bool rvalue = LexMatch(parser->lex, TOK(ampamp));
    if (!rvalue) {
      LexMatch(parser->lex, TOK(amp));
    }
    TypeParserParsePointer(parser);
    TypeRecord* p = NewReferenceTypeRecord(kQualPlain, rvalue);
    VectorAppend(&parser->stack, p);
  } else {
    TypeParserParseFuncOrArray(parser);
  }
}

// Check that a formal argument name is not already in the list
// of formals.  Returns true if name is OK.
static bool CheckFormalName(Vector* formals, String* name) {
  if (name->length == 0) {
    // Empty name is OK.
    return true;
  }
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = (Symbol*)formals->value.p[i];
    if (StringEqualString(&formal->name, name)) {
      return false;
    }
  }
  return true;
}

static bool TemplateArgumentContainsCurrentParameterPack(TypeParser* parser,
                                                         TemplateArgument* arg);

/* True if `type` references the parameter pack of the template currently being
 * parsed. Used to decide whether a `pattern...` is a valid pack expansion in
 * the enclosing template declaration. */
static bool TypeContainsCurrentParameterPack(TypeParser* parser,
                                             TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    int index = -1;
    if (TypeIsTemplateParameterPlaceholder(t, &index) &&
        CurrentTemplateParameterIsPack(parser->syntax, index)) {
      return true;
    }
    if (t->declarator == kDeclArray &&
        CurrentTemplateParameterIsPack(
            parser->syntax, t->info.array.template_parameter_index)) {
      return true;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        if (TemplateArgumentContainsCurrentParameterPack(
                parser, t->template_arguments->value.p[i])) {
          return true;
        }
      }
    }
    if (TypeIsFunction(t)) {
      for (size_t i = 0; i < t->info.function.prototype.length; i++) {
        Symbol* formal = t->info.function.prototype.value.p[i];
        if (formal != NULL &&
            TypeContainsCurrentParameterPack(parser, formal->type)) {
          return true;
        }
      }
    }
  }
  return false;
}

/* True if a template argument references the parameter pack of the template
 * currently being parsed (see TypeContainsCurrentParameterPack). */
static bool TemplateArgumentContainsCurrentParameterPack(TypeParser* parser,
                                                         TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->kind == kTemplateParameterNonType &&
      CurrentTemplateParameterIsPack(parser->syntax,
                                     arg->template_parameter_index)) {
    return true;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      if (TemplateArgumentContainsCurrentParameterPack(
              parser, arg->pack_arguments->value.p[i])) {
        return true;
      }
    }
  }
  return TypeContainsCurrentParameterPack(parser, arg->type);
}

// Parse a formal argument declaration.  Takes ownership of
// formal.
static void ParseFormalArgument(TypeParser* proto_parser,
                                TypeRecord* func,
                                Symbol* formal,
                                int arg_number) {
  if (formal->flags.is_parameter_pack &&
      !TypeContainsCurrentParameterPack(proto_parser, formal->type)) {
    SyntaxError(proto_parser->syntax,
                "function parameter pack requires a template parameter pack");
  }
  if (CheckFormalName(&func->info.function.prototype, &formal->name)) {
    // Function arguments are pointer to functions.
    if (TypeIsFunction(formal->type)) {
      TypeRecord* func_ptr = NewPointerTypeRecord(kQualPlain);
      TypeRecordChain(func_ptr, formal->type);
      SymbolSetType(formal, func_ptr);
    } else if (TypeIsArray(formal->type)) {
      // For an array, convert the type record to a pointer.
      TypeRecord* ptr = TypeRecordCopy(formal->type);
      ptr->declarator = kDeclPointer;
      ptr->size = compiler->pointer_size;
      SymbolSetType(formal, ptr);
    }
    VectorAppend(&func->info.function.prototype, formal);
    formal->flags.is_defined = true;
    formal->flags.is_argument = true;
    formal->value.arg_number = arg_number;
    if (proto_parser->syntax->local_symbol_stack != NULL) {
      InsertLocalSymbol(proto_parser->syntax->local_symbol_stack,
                      formal);
    }

  } else {
    SyntaxError(proto_parser->syntax, "Duplicate function argument '%s'",
                formal->name.value);
    SymbolDelete(formal);
  }
}

static TemplateArgument* NewTemplateParameterTypeArgumentForType(int index,
                                                                 TypeRecord* type) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->type = type != NULL ? TypeRecordCopy(type) : NULL;
  arg->template_parameter_index = index;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static TemplateParameter* NewAbbreviatedTypeTemplateParameter(TypeParser* parser,
                                                             int index) {
  TemplateParameter* param = malloc(sizeof(TemplateParameter));
  memset(param, 0, sizeof(*param));
  StringInit(&param->name, SyntaxFakeName(parser->syntax));
  param->kind = kTemplateParameterType;
  param->index = index;
  return param;
}

static void AddFunctionAssociatedConstraint(TypeRecord* func,
                                            ConstraintExpr* constraint) {
  if (func == NULL || !TypeIsFunction(func) || constraint == NULL) {
    return;
  }
  ConstraintExpr* current = func->info.function.associated_constraint;
  if (current == NULL) {
    func->info.function.associated_constraint = constraint;
    return;
  }
  func->info.function.associated_constraint =
      NewConjunctionConstraint(current, constraint, constraint->location);
}

static void ParseCXXTrailingRequiresClause(TypeParser* parser,
                                           TypeRecord* func) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20) ||
      !LexLookingAt(parser->lex, TOK(requires))) {
    return;
  }
  SyntaxOpenScope(parser->syntax);
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL && formal->name.length > 0) {
      InsertLocalSymbol(parser->syntax->local_symbol_stack, formal);
    }
  }
  ConstraintExpr* constraint = ConceptsParseRequiresClause(parser->syntax);
  SyntaxCloseScope(parser->syntax);
  AddFunctionAssociatedConstraint(func, constraint);
}

static bool ParseAbbreviatedFunctionParameter(TypeParser* proto_parser,
                                              TypeRecord* func,
                                              int arg_number) {
  Syntax* syntax = proto_parser->syntax;
  Lex* lex = proto_parser->lex;
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20)) {
    return false;
  }

  // A parameter-type-specifier of the form `const auto&` / `volatile auto`
  // begins with cv-qualifiers before the `auto` (or `Concept auto`) placeholder.
  // Consume them speculatively; if no placeholder follows, rewind so the normal
  // (non-abbreviated) parameter path re-parses from the qualifier.
  LexCheckpoint cv_checkpoint;
  LexCheckpointSave(lex, &cv_checkpoint);
  int placeholder_qualifiers = kQualPlain;
  while (true) {
    if (LexLookingAt(lex, TOK(const))) {
      placeholder_qualifiers |= kQualConst;
      LexNextToken(lex);
    } else if (LexLookingAt(lex, TOK(volatile))) {
      placeholder_qualifiers |= kQualVolatile;
      LexNextToken(lex);
    } else {
      break;
    }
  }

  Symbol* concept_symbol = NULL;
  SourceLocation constraint_location = lex->current_token_location;
  Vector* concept_arguments = NULL;
  if (LexLookingAt(lex, TOK(identifier))) {
    String concept_name;
    StringInit(&concept_name, lex->spelling.value);
    Symbol* found = SyntaxFindSymbol(syntax, &concept_name);
    StringDestruct(&concept_name);
    if (found != NULL && found->flags.is_concept) {
      LexCheckpoint checkpoint;
      LexCheckpointSave(lex, &checkpoint);
      LexNextToken(lex);
      if (LexLookingAt(lex, TOK(less))) {
        concept_arguments =
            SyntaxParseTemplateArgumentList(syntax, TC(closebra));
      } else {
        concept_arguments = NewVector();
      }
      if (LexLookingAt(lex, TOK(auto))) {
        concept_symbol = found;
      } else {
        if (concept_arguments != NULL) {
          VectorDeleteWithContents(
              concept_arguments, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
          concept_arguments = NULL;
        }
        LexCheckpointRestore(lex, &checkpoint);
      }
      LexCheckpointDestruct(&checkpoint);
    }
  }

  if (concept_symbol == NULL && !LexLookingAt(lex, TOK(auto))) {
    LexCheckpointRestore(lex, &cv_checkpoint);
    LexCheckpointDestruct(&cv_checkpoint);
    return false;
  }
  LexCheckpointDestruct(&cv_checkpoint);
  LexNextToken(lex);  // auto

  int index = syntax->current_template_parameter_count +
              (int)func->info.function.template_parameters.length;
  TypeRecord* placeholder =
      NewTypeRecordWithSize(kTypeInt | kTypeUnknown, placeholder_qualifiers);
  placeholder->template_parameter_index = index;
  Symbol* formal = TypeParserParseDeclarator(proto_parser, placeholder);
  assert(formal != NULL);
  ParseFormalArgument(proto_parser, func, formal, arg_number);

  VectorAppend(&func->info.function.template_parameters,
               NewAbbreviatedTypeTemplateParameter(proto_parser, index));
  if (concept_symbol != NULL) {
    if (concept_arguments == NULL) {
      concept_arguments = NewVector();
    }
    TemplateArgument* constrained_arg =
        NewTemplateParameterTypeArgumentForType(index, placeholder);
    if (concept_arguments->length == 0) {
      VectorAppend(concept_arguments, constrained_arg);
    } else {
      VectorInsertBefore(concept_arguments, 0, constrained_arg);
    }
    AddFunctionAssociatedConstraint(
        func, NewConceptIdConstraint(concept_symbol, concept_arguments,
                                     constraint_location));
  }
  func->info.function.template_parameter_count =
      (int)func->info.function.template_parameters.length;
  TypeRecordDelete(placeholder);
  return true;
}


// Prototype style.  C still allows old-style K&R code.
typedef enum  {
  kStyleUnknown,
  kStyleOld,
  kStyleNew
} PrototypeStyle;

static PrototypeStyle ParseFunctionParameter(TypeParser* proto_parser,
                                             TypeRecord* func,
                                             PrototypeStyle style,
                                             int arg_number,
                                             bool* seen_default_argument) {
  if (ParseAbbreviatedFunctionParameter(proto_parser, func, arg_number)) {
    if (style == kStyleUnknown) {
      style = kStyleNew;
    }
    if (style == kStyleOld) {
      SyntaxError(proto_parser->syntax,
                  "Cannot mix function prototype with old-style function args");
    }
  } else if (proto_parser->found_void ||
        SyntaxLookingAtType(proto_parser->syntax)) {
    TypeRecord* type = TypeParserParseType(proto_parser, true);
    if (style == kStyleUnknown) {
      style = kStyleNew;
    }
    if (style == kStyleOld) {
      SyntaxError(proto_parser->syntax,
                  "Cannot mix function prototype with old-style function args");
    }

    Symbol* formal = TypeParserParseDeclarator(proto_parser, type);
    assert(formal != NULL);
    ParseFormalArgument(proto_parser, func, formal, arg_number);
    if (CompilerIsCXX() && LexMatch(proto_parser->lex, TOK(equal))) {
      if (formal->flags.is_parameter_pack) {
        SyntaxError(proto_parser->syntax,
                    "function parameter pack cannot have a default argument");
      }
      formal->default_argument =
          SyntaxParseSingleExpression(proto_parser->syntax,
                                      TC(closebra) | TC(exprsep));
      *seen_default_argument = true;
    } else if (*seen_default_argument && !formal->flags.is_parameter_pack) {
      SyntaxError(proto_parser->syntax,
                  "parameter after default argument must have a default argument");
    }
  } else {
    // Possible old-style function decl, identifiers only.
    if (LexLookingAt(proto_parser->lex, TOK(identifier))) {
      if (style == kStyleUnknown) {
        style = kStyleOld;
      }
      if (style == kStyleNew) {
        SyntaxError(proto_parser->syntax, "Type expected for function arg");
        SyntaxRecover(proto_parser->syntax, TC(closebra));
      } else {
        TypeRecord* unknown = NewTypeRecordWithSize(kTypeInt, kQualPlain);
        Symbol* formal = NewSymbol(proto_parser->lex->spelling.value,
                                   unknown, STO(auto));
        LexNextToken(proto_parser->lex);
        ParseFormalArgument(proto_parser, func, formal, arg_number);
      }
    } else {
      SyntaxError(proto_parser->syntax,
                  "Expected type or identifier in function prototype");
      SyntaxRecover(proto_parser->syntax, TC(closebra));
    }
  }
  return style;
}

// Parse a function prototype, old or new style.
static void ParseFunctionPrototype(TypeParser* proto_parser, TypeRecord* func) {
  bool void_args = false;
  FunctionInfo* info = &func->info.function;
  PrototypeStyle style = kStyleUnknown;
  int arg_number = 0;
  bool seen_default_argument = false;
  
  info->old_style = false;

  while (!LexLookingAt(proto_parser->lex, TOK(rparen))) {
    if (LexMatch(proto_parser->lex, TOK(ellipsis))) {
      // ... must be the last argument in the prototype.
      info->varargs = true;
      if (!LexLookingAt(proto_parser->lex, TOK(rparen))) {
        SyntaxError(proto_parser->syntax,
                    "... must be at the end of a function prototype");
        SyntaxRecover(proto_parser->syntax, TC(closebra));
      }
      break;
    }
    // Check for (void).
    if (arg_number == 0 && LexMatch(proto_parser->lex, TOK(void))) {
      proto_parser->found_void = true;
      // Look for close paren; meaning (void).
      if (LexLookingAt(proto_parser->lex, TOK(rparen))) {
        // "void" means that there are no arguments.
        void_args = true;
        break;
      }
    }
    
    // The keyword 'register' is allowed here but we ignore it, except
    // to set the type as new style.
    if (LexMatch(proto_parser->lex, TOK(register))) {
      style = kStyleNew;
    }
    
    // Parse the formal argument's type, if it has one.
    // Otherwise it's a possible old-style function.
    style = ParseFunctionParameter(proto_parser, func, style, arg_number,
                                   &seen_default_argument);
    arg_number++;
    if (!LexMatch(proto_parser->lex, TOK(comma))) {
      break;
    }
  }
  if (style == kStyleOld) {
    info->old_style = true;
  }

  // If we were not told (void) and there are no formal args then the C
  // language says that this is a variable arguments function.
  if (!CompilerIsCXX() && info->prototype.length == 0 && !void_args) {
    info->unknown_args = true;
    SyntaxWarning(proto_parser->syntax, "strict-prototypes",
                  "function declaration without a prototype");
  }
}

// Parse an optional C++ exception specification (`noexcept`, `noexcept(expr)`,
// `throw()` or `throw(types)`) and record whether the function is guaranteed
// non-throwing on `func`.  `noexcept` with no operand and the deprecated empty
// `throw()` mean non-throwing; `noexcept(expr)` evaluates `expr` as a constant
// boolean; a dynamic `throw(types)` specification is treated as throwing.
static void ParseCXXExceptionSpecifier(TypeParser* parser, TypeRecord* func) {
  if (!CompilerIsCXX()) {
    return;
  }
  if (LexMatch(parser->lex, TOK(noexcept))) {
    bool is_noexcept = true;
    if (LexMatch(parser->lex, TOK(lparen))) {
      ASTNode* expr =
          SyntaxParseSingleExpression(parser->syntax, TC(exprsep));
      expr = AnalyzeExpression(expr);
      int64_t value = 1;
      if (!EvaluateIntegerExpression(expr, &value)) {
        // A dependent noexcept(expr) in a template can only be resolved at
        // instantiation time; conservatively treat it as possibly-throwing
        // rather than erroring or enforcing here.
        if (parser->syntax->parsing_template_declaration) {
          value = 0;
        } else {
          SyntaxError(parser->syntax,
                      "noexcept specifier is not a constant expression");
        }
      }
      ASTNodeDelete(expr);
      SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
      is_noexcept = value != 0;
    }
    if (func != NULL) {
      func->info.function.is_noexcept = is_noexcept;
    }
    return;
  }
  if (LexMatch(parser->lex, TOK(throw))) {
    if (LexLookingAt(parser->lex, TOK(lparen))) {
      // `throw()` is the deprecated non-throwing spec; `throw(types)` is a
      // dynamic specification that permits throwing the listed types.
      LexNextToken(parser->lex);  // Consume '('.
      bool is_empty = LexLookingAt(parser->lex, TOK(rparen));
      int depth = 1;
      while (depth > 0 && !LexEof(parser->lex)) {
        if (LexMatch(parser->lex, TOK(lparen))) {
          depth++;
        } else if (LexMatch(parser->lex, TOK(rparen))) {
          depth--;
        } else {
          LexNextToken(parser->lex);
        }
      }
      if (func != NULL) {
        func->info.function.is_noexcept = is_empty;
      }
    } else {
      SyntaxError(parser->syntax, "Expected exception specification");
    }
  }
}

static TypeRecord* ParseCXXTrailingReturnType(TypeParser* parser) {
  TypeParser return_parser;
  TypeParserInit(&return_parser, parser->lex, parser->syntax, STO(auto),
                 kParsingPrototype);
  TypeRecord* return_type = TypeParserParseType(&return_parser, true);
  Symbol* parsed = return_type != NULL
                       ? TypeParserParseDeclarator(&return_parser, return_type)
                       : NULL;
  TypeParserDestruct(&return_parser);
  if (parsed != NULL) {
    TypeRecord* result = TypeRecordCopy(parsed->type);
    SymbolDelete(parsed);
    TypeRecordDelete(return_type);
    return result;
  }
  if (return_type == NULL) {
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  return return_type;
}

static CXXRefQualifier ParseCXXRefQualifier(TypeParser* parser) {
  if (!CompilerIsCXX()) {
    return kCXXRefQualifierNone;
  }
  if (LexMatch(parser->lex, TOK(ampamp))) {
    return kCXXRefQualifierRValue;
  }
  if (LexMatch(parser->lex, TOK(amp))) {
    return kCXXRefQualifierLValue;
  }
  return kCXXRefQualifierNone;
}

static void ParseFunctionDecl(TypeParser* parser) {
  TypeParser proto_parser;
  TypeParserInit(&proto_parser, parser->lex, parser->syntax, STO(auto), kParsingPrototype);
  // Propagate the enclosing class so a parameter type can name the class
  // currently being defined, including via its own template-id (e.g. a member
  // or friend declared as 'f(const Box<T>&)' inside 'template<class T> Box').
  proto_parser.cxx_member_owner = parser->cxx_member_owner;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_inline =
      parser->is_inline ||
      (CompilerIsCXX() && (parser->is_constexpr || parser->is_consteval));
  func->info.function.is_constexpr = parser->is_constexpr;
  func->info.function.is_consteval = parser->is_consteval;
  if (parser->symbol != NULL) {
    func->info.function.symbol = parser->symbol;
  }
  
  ParseFunctionPrototype(&proto_parser, func);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
  func->info.function.is_const_member = LexMatch(parser->lex, TOK(const));
  func->info.function.ref_qualifier = ParseCXXRefQualifier(parser);
  ParseCXXExceptionSpecifier(parser, func);
  if (CompilerIsCXX() && TypeContainsAuto(parser->base_type) &&
      LexMatch(parser->lex, TOK(arrow))) {
    TypeRecord* trailing_return = ParseCXXTrailingReturnType(parser);
    TypeRecordChain(func, trailing_return);
  }
  ParseCXXTrailingRequiresClause(parser, func);
  if (parser->cxx_member_definition != NULL &&
      !parser->cxx_member_definition->is_static) {
    TypeRecordAddCXXThisParameter(
        func, parser->cxx_member_owner, parser->symbol->location);
  } else if (parser->cxx_member_definition != NULL) {
    func->info.function.cxx_member_owner = parser->cxx_member_owner;
  }

  VectorAppend(&parser->stack, func);
  TypeParserDestruct(&proto_parser);
}
 
static void ParseArrayDecl(TypeParser* parser) {
  bool is_static = false;
  Qualifiers quals = kQualPlain;
  // An array decl can have:
  // [static quals ...]
  // [quals static ...];
  if (LexMatch(parser->lex, TOK(static))) {
    is_static = true;
    quals = ParseQualifiers(parser);
  } else {
    quals = ParseQualifiers(parser);
    is_static = LexMatch(parser->lex, TOK(static));
  }

  // These are only allowed inside a function prototype.
  if (parser->context != kParsingPrototype) {
    if (is_static || quals != kQualPlain) {
      SyntaxError(parser->syntax, "static or qualifiers used in array declarator outside function prototype");
    }
  }
  
  parser->dimension_count++;
  TypeRecord* p = NewArrayTypeRecord(quals, is_static);
  VectorAppend(&parser->stack, p);

  bool is_vla = false;
  bool found_star = false;
  SourceLocation location = parser->lex->current_token_location;
  if (LexMatch(parser->lex, TOK(rsquare))) {
    if (parser->dimension_count != 1) {
      SyntaxError(parser->syntax,
             "Array dimension required after first dimension");
    }
    // No size expression present.
    if (parser->context != kParsingPrototype) {
      p->info.array.is_flexible = true;
    }
    return;
  }
  
  // There is something in the [...]
  if (LexMatch(parser->lex, TOK(star))) {
    // Unfortunately * can be a unary operator and part of an expression
    // so we need to look at the next token to see if it's a close square
    // bracket.  We've already consumed the *.
    found_star = true;
  }
  if (found_star && LexLookingAt(parser->lex, TOK(rsquare))) {
    if (parser->context != kParsingPrototype) {
      SyntaxError(parser->syntax, "VLA placeholder '*' is only valid in a function prototype");
    } else {
      is_vla = true;
      p->info.array.is_placeholder_vla = true;
    }
  } else {
    // Size expression is present.  If it's constant we have a
    // regular fixed size array, otherwise it's a VLA.
    ASTNode* size_expr =
        SyntaxParseSingleExpression(parser->syntax, TC(closebra));
    if (found_star) {
      // There was a * before the expression, this means contents.
      size_expr = NewUnaryASTNode(AST_OP(contents), NULL, location, size_expr);
    }
    size_expr = AnalyzeExpression(size_expr);
    bool delete_expr = true;
    int64_t size;
    bool ok = EvaluateIntegerExpression(size_expr, &size);
    if (!ok) {
      if (parser->syntax->parsing_template_declaration &&
          size_expr->op == AST_OP(identifier)) {
        IdentifierASTNode* id = (IdentifierASTNode*)size_expr;
        if (id->symbol != NULL && id->symbol->flags.is_template_parameter &&
            !id->symbol->flags.is_template_type_parameter) {
          p->info.array.template_parameter_index =
              id->symbol->template_parameter_index;
          p->info.array.size.fixed = 0;
          goto parsed_bound;
        }
      }
      // VLA.
      if (!TypeIsIntegral(size_expr->type)) {
        SyntaxError(parser->syntax, "Variable length array size must be integral");
      }
      if (parser->context != kParsingBlockScope && parser->context != kParsingPrototype) {
        SyntaxError(parser->syntax, "Variable length array is only allowed inside a function");
      } else if (StorageIs(parser->storage, STO(extern)|STO(static))) {
        SyntaxError(parser->syntax, "Variable length array cannot be static or extern");
      } else {
        p->info.array.size.vla.size = size_expr;
        delete_expr = false;      // Hold on to expression.
        is_vla = true;
      }
    } else {
      if (size < 0) {
        SyntaxError(parser->syntax, "Array with negative size");
        size = 1;
      }
      // A zero-length array (int r[0]) is a GCC extension, commonly used at
      // the end of a struct like a flexible array member.
      p->info.array.size.fixed = (int)size;
    }
parsed_bound:
    if (delete_expr) {
      ASTNodeDelete(size_expr);
    }
  }
  p->info.array.is_vla = is_vla;
  
  if (!LexMatch(parser->lex, TOK(rsquare))) {
    LexError(parser->lex, "Missing ]");
  }
}

static bool CXXDirectInitializerAfterDeclarator(TypeParser* parser) {
  if (!CompilerIsCXX() || parser->symbol == NULL ||
      parser->stack.length != 0 || !TypeIsStructOrUnion(parser->base_type) ||
      !LexLookingAt(parser->lex, TOK(lparen))) {
    return false;
  }
  if (parser->context == kParsingBlockScope) {
    return true;
  }
  if (parser->context != kParsingFileScope) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  LexNextToken(parser->lex);
  bool direct_initializer =
      !LexLookingAt(parser->lex, TOK(rparen)) &&
      !SyntaxLookingAtType(parser->syntax);
  LexCheckpointRestore(parser->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return direct_initializer;
}

void TypeParserParseFuncOrArray(TypeParser* parser) {
  TypeParserParseBase(parser);
  while (parser->syntax->found_open_paren ||
         LexLookingAt(parser->lex, TOK(lparen)) ||
         LexLookingAt(parser->lex, TOK(lsquare))) {
    if (CXXDirectInitializerAfterDeclarator(parser)) {
      // In `T obj(args);`, the parens are direct initialization of `obj`,
      // not a function declarator. At namespace scope this is limited to
      // constexpr/constinit objects so ordinary declarations keep the old path.
      break;
    }
    // Check for function prototype declaration.
    if (parser->syntax->found_open_paren || LexMatch(parser->lex, TOK(lparen))) {
      parser->syntax->found_open_paren = false;
      ParseFunctionDecl(parser);
    } else if (LexMatch(parser->lex, TOK(lsquare))) {
      ParseArrayDecl(parser);
    }
  }
}


static void ResolveQualifiedMemberDeclarator(TypeParser* parser,
                                             FullyQualifiedIdentifier* name) {
  if (!name->is_qualified || name->components.length < 2) {
    return;
  }

  Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
      parser->syntax, name, name->components.length - 1);
  if (owner == NULL || owner->type == NULL ||
      !TypeIsStructOrUnion(owner->type) ||
      owner->type->info.struct_info == NULL) {
    SyntaxError(parser->syntax, "Qualified declarator %s does not name a class member",
                name->spelling.value);
    return;
  }

  parser->cxx_member_owner = owner->type->info.struct_info;
  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(name));
  parser->cxx_member_definition =
      FindStructMember(parser->cxx_member_owner, &member_name);
  if (parser->cxx_member_definition == NULL) {
    SyntaxError(parser->syntax, "No class member named %s",
                name->spelling.value);
  }
  StringDestruct(&member_name);
}

static bool CXXClassNameMatchesUnqualifiedTemplateName(String* class_name,
                                                       String* spelling);

static bool QualifiedNameIsSpecialMember(Symbol* owner,
                                         FullyQualifiedIdentifier* name,
                                         bool* is_destructor,
                                         String* special_member_name) {
  const char* member_name = FullyQualifiedIdentifierLast(name);
  *is_destructor = member_name[0] == '~';
  const char* source_class_name =
      *is_destructor ? member_name + 1 : member_name;
  String source;
  StringInit(&source, source_class_name);
  bool matches =
      CXXClassNameMatchesUnqualifiedTemplateName(&owner->name, &source);
  StringDestruct(&source);
  if (!matches) {
    return false;
  }
  StringSet(special_member_name, *is_destructor ? "~" : "");
  StringAppendString(special_member_name, &owner->name);
  return true;
}

static void ConversionOperatorName(TypeRecord* type, String* name);
static TypeRecord* ParseCXXConversionType(TypeParser* parser);
static Symbol* NewCXXConversionOperatorSymbol(TypeParser* parser,
                                              Struct* owner,
                                              TypeRecord* return_type,
                                              SourceLocation location,
                                              bool is_virtual);

static void AppendConversionOwnerComponent(FullyQualifiedIdentifier* name,
                                           String* component) {
  if (name->spelling.length != 0 || name->absolute) {
    StringAppend(&name->spelling, "::");
  }
  StringAppendString(&name->spelling, component);
  VectorAppend(&name->components, NewString(component->value));
  VectorAppend(&name->template_arguments, NULL);
}

static bool ParseCXXConversionOperatorOwner(TypeParser* parser,
                                            FullyQualifiedIdentifier* owner_name) {
  if (LexMatch(parser->lex, TOK(coloncolon))) {
    owner_name->absolute = true;
    owner_name->is_qualified = true;
  }
  if (!LexLookingAt(parser->lex, TOK(identifier))) {
    return false;
  }

  while (!LexEof(parser->lex)) {
    String component;
    StringInit(&component, parser->lex->spelling.value);
    LexNextToken(parser->lex);
    AppendConversionOwnerComponent(owner_name, &component);
    StringDestruct(&component);

    if (!LexMatch(parser->lex, TOK(coloncolon))) {
      return false;
    }
    owner_name->is_qualified = true;
    if (LexLookingAt(parser->lex, TOK(operator))) {
      return true;
    }
    if (!LexLookingAt(parser->lex, TOK(identifier))) {
      return false;
    }
  }
  return false;
}

static Symbol* TryParseCXXQualifiedConversionOperatorDeclarator(
    TypeParser* parser) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);

  SourceLocation location = parser->lex->current_token_location;
  FullyQualifiedIdentifier owner_name;
  FullyQualifiedIdentifierInit(&owner_name);
  if (!ParseCXXConversionOperatorOwner(parser, &owner_name) ||
      !LexMatch(parser->lex, TOK(operator))) {
    FullyQualifiedIdentifierDestruct(&owner_name);
    LexCheckpointRestore(parser->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);

  if (!owner_name.absolute && owner_name.components.length == 1) {
    owner_name.is_qualified = false;
  }
  Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
      parser->syntax, &owner_name, owner_name.components.length);
  if (owner == NULL) {
    owner = SyntaxFindQualifiedTag(parser->syntax, &owner_name);
  }
  if (owner == NULL || owner->type == NULL ||
      !TypeIsStructOrUnion(owner->type) ||
      owner->type->info.struct_info == NULL) {
    SyntaxError(parser->syntax,
                "Qualified conversion operator does not name a class member");
    FullyQualifiedIdentifierDestruct(&owner_name);
    return NULL;
  }

  parser->cxx_member_owner = owner->type->info.struct_info;
  TypeRecord* return_type = ParseCXXConversionType(parser);
  String member_name;
  ConversionOperatorName(return_type, &member_name);
  parser->cxx_member_definition =
      FindStructMember(parser->cxx_member_owner, &member_name);
  if (parser->cxx_member_definition == NULL) {
    SyntaxError(parser->syntax, "No class member named %s",
                member_name.value);
    StringDestruct(&member_name);
    FullyQualifiedIdentifierDestruct(&owner_name);
    return NULL;
  }

  if (!LexMatch(parser->lex, TOK(lparen))) {
    SyntaxError(parser->syntax,
                "Expected '(' in conversion operator definition");
    StringDestruct(&member_name);
    FullyQualifiedIdentifierDestruct(&owner_name);
    return NULL;
  }
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));

  Symbol* sym =
      NewCXXConversionOperatorSymbol(parser, parser->cxx_member_owner,
                                     return_type, location,
                                     /*is_virtual=*/false);
  SymbolSetCXXMangledAsmName(sym);
  StringDestruct(&member_name);
  FullyQualifiedIdentifierDestruct(&owner_name);
  return sym;
}

Symbol* TypeParserParseCXXSpecialMemberDeclarator(TypeParser* parser) {
  Symbol* conversion = TryParseCXXQualifiedConversionOperatorDeclarator(parser);
  if (conversion != NULL) {
    return conversion;
  }

  SourceLocation location = parser->lex->current_token_location;
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(parser->syntax, &name,
                                                         TC(decl))) {
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }
  if (!name.is_qualified || name.components.length < 2) {
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
      parser->syntax, &name, name.components.length - 1);
  if (owner == NULL || owner->type == NULL ||
      !TypeIsStructOrUnion(owner->type) ||
      owner->type->info.struct_info == NULL) {
    SyntaxError(parser->syntax, "Qualified declarator %s does not name a class member",
                name.spelling.value);
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  bool is_destructor = false;
  String member_name;
  StringInit(&member_name, NULL);
  if (!QualifiedNameIsSpecialMember(owner, &name, &is_destructor,
                                    &member_name)) {
    StringDestruct(&member_name);
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  parser->cxx_member_owner = owner->type->info.struct_info;
  parser->cxx_member_definition =
      FindStructMember(parser->cxx_member_owner, &member_name);
  if (parser->cxx_member_definition == NULL) {
    SyntaxError(parser->syntax, "No class member named %s", name.spelling.value);
    StringDestruct(&member_name);
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  if (!LexMatch(parser->lex, TOK(lparen))) {
    SyntaxError(parser->syntax, "Expected '(' in special member definition");
    StringDestruct(&member_name);
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  TypeParser proto_parser;
  TypeParserInit(&proto_parser, parser->lex, parser->syntax, STO(auto),
                 kParsingPrototype);
  proto_parser.cxx_member_owner = parser->cxx_member_owner;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constexpr = parser->is_constexpr;
  func->info.function.is_consteval = parser->is_consteval;
  func->info.function.is_constructor = !is_destructor;
  func->info.function.is_destructor = is_destructor;
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  ParseFunctionPrototype(&proto_parser, func);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
  if (LexMatch(parser->lex, TOK(const))) {
    SyntaxError(parser->syntax, "Constructors and destructors cannot be const");
  }
  ParseCXXExceptionSpecifier(parser, func);
  ParseCXXPureSpecifier(parser, func);
  TypeParserDestruct(&proto_parser);
  TypeRecordAddCXXThisParameter(func, parser->cxx_member_owner, location);

  Symbol* sym = NewSymbol(member_name.value, func, STO(implicit));
  sym->location = location;
  func->info.function.symbol = sym;
  CXXFinalizeSpecialMemberMetadata(sym, parser->cxx_member_owner, true);
  SymbolSetCXXMangledAsmName(sym);
  StringDestruct(&member_name);
  FullyQualifiedIdentifierDestruct(&name);
  return sym;
}

void TypeParserParseBase(TypeParser* parser) {
  if (LexMatch(parser->lex, TOK(lparen))) {
    if (SyntaxLookingAtType(parser->syntax) || LexLookingAt(parser->lex, TOK(rparen))) {
      // Open paren followed by a type isn't a parenthesized decl, it's
      // a function prototype.
      parser->syntax->found_open_paren = true;
      return;
    }
    TypeParserParsePointer(parser);
    if (!LexMatch(parser->lex, TOK(rparen))) {
      LexError(parser->lex, "Missing close parenthesis in declaration");
    }
  } else {
    if (LexLookingAt(parser->lex, TOK(identifier)) ||
        LexLookingAt(parser->lex, TOK(operator)) ||
        LexLookingAt(parser->lex, TOK(coloncolon))) {
      SourceLocation location = parser->lex->current_token_location;
      FullyQualifiedIdentifier name;
      FullyQualifiedIdentifierInit(&name);
      bool parsed = CompilerIsCXX()
          ? SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
                parser->syntax, &name, TC(decl))
          : SyntaxParseFullyQualifiedIdentifier(parser->syntax, &name);
      if (!parsed) {
        FullyQualifiedIdentifierDestruct(&name);
        return;
      }
      ResolveQualifiedMemberDeclarator(parser, &name);
      parser->symbol =
          NewSymbol(FullyQualifiedIdentifierLast(&name), parser->base_type,
                    parser->storage);
      parser->symbol->location = location;
      if (name.template_arguments.length > 0) {
        Vector* args =
            name.template_arguments.value.p[name.template_arguments.length - 1];
        parser->declarator_template_arguments = TemplateArgumentVectorCopy(args);
      }
      FullyQualifiedIdentifierDestruct(&name);
    }
  }
}

#if 0
static void PrintStructMember(const MapKeyValue* kv) {
  String* name = kv->key.p;
  printf("%s", name->value);
}
#endif

StructMember* FindStructMember(Struct* str, String* name) {
//  MapPrint(&str->symbol_table, PrintStructMember);
//  printf("\n");
  StructMember* member = MapFindPointerKey(&str->symbol_table, name);
  if (member != NULL) {
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMember(base->type->info.struct_info, name);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

static StructMember* FindDirectStructMemberByName(Struct* str,
                                                  const char* name) {
  if (str == NULL || name == NULL) {
    return NULL;
  }
  return MapFindPointerKey(&str->symbol_name_table, (void*)name);
}

StructMember* FindStructMemberByName(Struct* str, const char* name) {
  if (str == NULL || name == NULL) {
    return NULL;
  }
  StructMember* member = FindDirectStructMemberByName(str, name);
  if (member != NULL) {
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberByName(base->type->info.struct_info, name);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

static CXXAccess CombineInheritedAccess(CXXAccess base_access,
                                        CXXAccess member_access) {
  if (member_access == kAccessPrivate || base_access == kAccessPrivate) {
    return kAccessPrivate;
  }
  if (member_access == kAccessProtected || base_access == kAccessProtected) {
    return kAccessProtected;
  }
  return kAccessPublic;
}

static StructMember* FindStructMemberWithAccessByNameFromBase(
    Struct* str, const char* name, CXXAccess inherited, int inherited_offset,
    CXXAccess* access, Struct** owner, int* byte_offset) {
  if (str == NULL || name == NULL) {
    return NULL;
  }
  StructMember* member = FindDirectStructMemberByName(str, name);
  if (member != NULL) {
    if (access != NULL) {
      *access = CombineInheritedAccess(inherited, member->access);
    }
    if (owner != NULL) {
      *owner = str;
    }
    if (byte_offset != NULL) {
      *byte_offset = inherited_offset + member->byte_offset;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberWithAccessByNameFromBase(
          base->type->info.struct_info, name,
          CombineInheritedAccess(inherited, base->access),
          inherited_offset + base->byte_offset, access, owner, byte_offset);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

static StructMember* FindStructMemberWithAccessFromBase(Struct* str,
                                                        String* name,
                                                        CXXAccess inherited,
                                                        int inherited_offset,
                                                        CXXAccess* access,
                                                        Struct** owner,
                                                        int* byte_offset) {
  StructMember* member = MapFindPointerKey(&str->symbol_table, name);
  if (member != NULL) {
    if (access != NULL) {
      *access = CombineInheritedAccess(inherited, member->access);
    }
    if (owner != NULL) {
      *owner = str;
    }
    if (byte_offset != NULL) {
      *byte_offset = inherited_offset + member->byte_offset;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberWithAccessFromBase(
          base->type->info.struct_info, name,
          CombineInheritedAccess(inherited, base->access),
          inherited_offset + base->byte_offset, access, owner, byte_offset);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

StructMember* FindStructMemberWithAccess(Struct* str, String* name,
                                         CXXAccess* access,
                                         Struct** owner) {
  return FindStructMemberWithAccessAndOffset(str, name, access, owner, NULL);
}

StructMember* FindStructMemberWithAccessByName(Struct* str, const char* name,
                                               CXXAccess* access,
                                               Struct** owner) {
  return FindStructMemberWithAccessAndOffsetByName(str, name, access, owner,
                                                   NULL);
}

StructMember* FindStructMemberWithAccessAndOffset(Struct* str, String* name,
                                                  CXXAccess* access,
                                                  Struct** owner,
                                                  int* byte_offset) {
  StructMember* member = MapFindPointerKey(&str->symbol_table, name);
  if (member != NULL) {
    if (access != NULL) {
      *access = member->access;
    }
    if (owner != NULL) {
      *owner = str;
    }
    if (byte_offset != NULL) {
      *byte_offset = member->byte_offset;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberWithAccessFromBase(
          base->type->info.struct_info, name, base->access, base->byte_offset,
          access, owner, byte_offset);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

StructMember* FindStructMemberWithAccessAndOffsetByName(
    Struct* str, const char* name, CXXAccess* access, Struct** owner,
    int* byte_offset) {
  if (str == NULL || name == NULL) {
    return NULL;
  }
  StructMember* member = FindDirectStructMemberByName(str, name);
  if (member != NULL) {
    if (access != NULL) {
      *access = member->access;
    }
    if (owner != NULL) {
      *owner = str;
    }
    if (byte_offset != NULL) {
      *byte_offset = member->byte_offset;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberWithAccessByNameFromBase(
          base->type->info.struct_info, name, base->access, base->byte_offset,
          access, owner, byte_offset);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

StructMember* FindStructMemberOverload(StructMember* first, TypeRecord* type) {
  for (StructMember* overload = first; overload != NULL;
       overload = overload->overload_next) {
    bool overload_is_template =
        overload->symbol != NULL && overload->symbol->flags.is_template;
    bool type_is_template =
        TypeIsFunction(type) && type->info.function.template_parameter_count > 0;
    if (overload_is_template != type_is_template) {
      continue;
    }
    if (TypeEqual(overload->symbol->type, type)) {
      return overload;
    }
  }
  return NULL;
}

static bool CheckStructMember(Struct* str, String* name) {
  return MapFindPointerKey(&str->symbol_table, name) == NULL;
}

static void StructInsertMemberIntoTables(Struct* str, StructMember* member) {
  if (str == NULL || member == NULL || member->symbol == NULL) {
    return;
  }
  MapKeyValue kv;
  kv.key.p = &member->symbol->name;
  kv.value.p = member;
  MapInsert(&str->symbol_table, kv);
  kv.key.p = member->symbol->name.value;
  MapInsert(&str->symbol_name_table, kv);
}

static int EffectiveMemberAlignment(Struct* str, TypeRecord* type,
                                    int explicit_alignment) {
  // A packed struct places members on byte boundaries with no padding, so the
  // effective member alignment is 1.
  int alignment = str->packed ? 1 : TypeRecordAlignment(type);
  if (!str->packed && explicit_alignment > alignment) {
    alignment = explicit_alignment;
  }
  // #pragma pack(n) caps the effective alignment of each member at n bytes.
  if (str->pack > 0 && alignment > str->pack) {
    alignment = str->pack;
  }
  return alignment > 0 ? alignment : 1;
}

static void AlignNextOffsetWithAlignment(Struct* str, TypeRecord* type,
                                         int explicit_alignment) {
  int alignment = EffectiveMemberAlignment(str, type, explicit_alignment);
  if (alignment > str->alignment) {
    str->alignment = alignment;
  }
  str->next_offset = (str->next_offset + (alignment - 1)) & ~(alignment - 1);
  str->next_bit_pos = 65;
  str->current_offset = str->next_offset;
}

static void AlignNextOffset(Struct* str, TypeRecord* type) {
  AlignNextOffsetWithAlignment(str, type, 0);
}

static void AlignNextOffsetForSymbol(Struct* str, Symbol* symbol) {
  AlignNextOffsetWithAlignment(str, symbol->type, symbol->alignment);
}

// Applies the final struct alignment/size rounding, honoring an explicit
// aligned(N) override.
static void FinalizeStructAlignment(Struct* str) {
  int align = str->alignment > 0 ? str->alignment : 1;
  if (CompilerIsCXX() && !str->is_union && str->size == 0) {
    str->size = 1;
    str->alignment = align;
  }
  if (str->explicit_alignment > align) {
    align = str->explicit_alignment;
    str->alignment = align;
  }
  str->size = (str->size + (align - 1)) & ~(align - 1);
}

void StructApplyLayoutAttributes(Struct* str, Vector* attrs) {
  if (AttributeListHas(attrs, "packed")) {
    str->packed = true;
  }
  Attribute* aligned = AttributeListFind(attrs, "aligned");
  if (aligned != NULL) {
    long n = 0;
    int a;
    if (AttributeArgInt(aligned, 0, &n) && n > 0) {
      a = (int)n;
    } else {
      // aligned with no argument requests the target's maximum alignment.
      a = compiler->alignment;
    }
    if (a > str->explicit_alignment) {
      str->explicit_alignment = a;
    }
  }
}

static CXXAccess ParseBaseAccess(TypeParser* parser, bool is_class) {
  if (LexMatch(parser->lex, TOK(public))) {
    return kAccessPublic;
  }
  if (LexMatch(parser->lex, TOK(protected))) {
    return kAccessProtected;
  }
  if (LexMatch(parser->lex, TOK(private))) {
    return kAccessPrivate;
  }
  return is_class ? kAccessPrivate : kAccessPublic;
}

// Parses an optional C++ class-virt-specifier ('final') that may appear after
// the class-head-name.  Because 'final' is only a contextual keyword, it is
// treated as the specifier only when it is immediately followed by '{' or ':'
// (a class definition or base-clause); otherwise it is left for the caller to
// interpret as an ordinary identifier (e.g. a declarator named 'final').
static bool ParseCXXClassFinalSpecifier(TypeParser* parser) {
  if (!CompilerIsCXX() || !LexLookingAt(parser->lex, TOK(identifier)) ||
      !StringEqual(&parser->lex->spelling, "final")) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  LexNextToken(parser->lex);
  if (LexLookingAt(parser->lex, TOK(lbrace)) ||
      LexLookingAt(parser->lex, TOK(colon))) {
    LexCheckpointDestruct(&checkpoint);
    return true;
  }
  LexCheckpointRestore(parser->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return false;
}

static void ParseCXXBaseSpecifiers(TypeParser* parser, Vector* bases,
                                   bool is_union, bool is_class) {
  if (!CompilerIsCXX() || is_union || !LexMatch(parser->lex, TOK(colon))) {
    return;
  }
  do {
    bool is_virtual = false;
    if (LexMatch(parser->lex, TOK(virtual))) {
      is_virtual = true;
    }
    CXXAccess access = ParseBaseAccess(parser, is_class);
    if (LexMatch(parser->lex, TOK(virtual))) {
      is_virtual = true;
    }
    TypeRecord* base_type = TypeParserParseType(parser, true);
    bool is_pack_expansion = CompilerIsCXX() &&
                             LexMatch(parser->lex, TOK(ellipsis));
    int placeholder_index = -1;
    bool is_template_parameter_base =
        TypeIsTemplateParameterPlaceholder(base_type, &placeholder_index);
    if (is_pack_expansion &&
        !CurrentTemplateParameterIsPack(parser->syntax, placeholder_index)) {
      SyntaxError(parser->syntax,
                  "base class pack expansion requires a template parameter pack");
      TypeRecordDelete(base_type);
      continue;
    }
    if (base_type == NULL ||
        (!is_template_parameter_base &&
         (!TypeIsStructOrUnion(base_type) ||
          base_type->info.struct_info == NULL))) {
      SyntaxError(parser->syntax, "base class must be a class or struct type");
      TypeRecordDelete(base_type);
      continue;
    }
    if (!is_template_parameter_base && base_type->info.struct_info->is_final) {
      const char* base_name =
          base_type->info.struct_info->tag_name != NULL
              ? base_type->info.struct_info->tag_name->value
              : "<anonymous>";
      SyntaxError(parser->syntax, "cannot derive from final base class %s",
                  base_name);
    }
    if (!is_template_parameter_base) {
      TypeRecordCalculateSize(base_type);
    }
    CXXBaseSpecifier* base =
        NewCXXBaseSpecifier(base_type, access, is_virtual);
    base->is_pack_expansion = is_pack_expansion;
    VectorAppend(bases, base);
    // `base_type` was returned with refs==0 (a fresh, unowned copy) and
    // NewCXXBaseSpecifier took the single owning reference for the base
    // specifier.  Do NOT release it here: doing so would drop the count back to
    // zero and free the record while the base specifier still points at it.
    // That is harmless for bases whose distinguishing state lives on a
    // registered struct, but for a deferred dependent template-id base (e.g.
    // `integral_constant<size_t, N>` parsed inside a class template) the
    // template arguments live only on this record, so freeing it silently
    // dropped them and broke later instantiation of the derived template.
  } while (LexMatch(parser->lex, TOK(comma)));
}

static void LayoutCXXBaseSpecifiers(Struct* str) {
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->is_virtual) {
      continue;
    }
    if (base->type == NULL || !TypeIsStructOrUnion(base->type)) {
      continue;
    }
    AlignNextOffset(str, base->type);
    base->byte_offset = str->next_offset;
    int base_size = base->type->size;
    if (TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL &&
        base->type->info.struct_info->non_virtual_size > 0) {
      base_size = base->type->info.struct_info->non_virtual_size;
    }
    if (!str->is_union) {
      str->next_offset += base_size;
      str->size = str->next_offset;
    } else if (base->type->size > str->size) {
      str->size = base->type->size;
    }
  }
}

static bool SameStructType(TypeRecord* left, TypeRecord* right) {
  return left != NULL && right != NULL && TypeIsStructOrUnion(left) &&
         TypeIsStructOrUnion(right) && left->info.struct_info != NULL &&
         left->info.struct_info == right->info.struct_info;
}

static CXXVirtualBaseInfo* FindCXXVirtualBaseInfo(Struct* str,
                                                  TypeRecord* type) {
  if (str == NULL || type == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
    if (SameStructType(base->type, type)) {
      return base;
    }
  }
  return NULL;
}

static CXXVirtualBaseInfo* AddCXXVirtualBaseInfo(Struct* str,
                                                 TypeRecord* type,
                                                 CXXAccess access) {
  CXXVirtualBaseInfo* existing = FindCXXVirtualBaseInfo(str, type);
  if (existing != NULL) {
    if (access == kAccessPrivate || existing->access == kAccessPrivate) {
      existing->access = kAccessPrivate;
    } else if (access == kAccessProtected ||
               existing->access == kAccessProtected) {
      existing->access = kAccessProtected;
    }
    return existing;
  }
  CXXVirtualBaseInfo* base =
      NewCXXVirtualBaseInfo(type, access, (int)str->virtual_bases.length);
  VectorAppend(&str->virtual_bases, base);
  return base;
}

static void CollectCXXVirtualBasesFromBase(Struct* str,
                                           CXXBaseSpecifier* base) {
  if (str == NULL || base == NULL || base->type == NULL ||
      !TypeIsStructOrUnion(base->type) ||
      base->type->info.struct_info == NULL) {
    return;
  }
  Struct* base_struct = base->type->info.struct_info;
  if (base->is_virtual) {
    AddCXXVirtualBaseInfo(str, base->type, base->access);
  }
  for (size_t i = 0; i < base_struct->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* inherited = base_struct->virtual_bases.value.p[i];
    AddCXXVirtualBaseInfo(str, inherited->type, inherited->access);
  }
}

static void CollectCXXVirtualBases(Struct* str) {
  if (!CompilerIsCXX() || str == NULL) {
    return;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CollectCXXVirtualBasesFromBase(str, str->bases.value.p[i]);
  }
}

bool StructHasVirtualBases(Struct* str) {
  return str != NULL && str->virtual_bases.length > 0;
}

static void CopyCXXBaseVirtualMembers(Struct* str) {
  if (!CompilerIsCXX() || str->virtual_members.length != 0) {
    return;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    for (size_t j = 0; j < base_struct->virtual_members.length; j++) {
      VectorAppend(&str->virtual_members, base_struct->virtual_members.value.p[j]);
    }
  }
}

static bool StructHasPolymorphicBase(Struct* str) {
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL &&
        base->type->info.struct_info->virtual_members.length > 0) {
      return true;
    }
  }
  return false;
}

static TypeRecord* NewCXXVTableEntryType(void) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  return NewPointerTo(kQualPlain, void_type);
}

static bool CXXMemberFunctionSignaturesMatch(TypeRecord* a, TypeRecord* b);

static TypeRecord* NewCXXVPtrType(void) {
  TypeRecord* entry_type = NewCXXVTableEntryType();
  return NewPointerTo(kQualPlain, entry_type);
}

static TypeRecord* NewCXXVBPtrType(void) {
  TypeRecord* entry_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  return NewPointerTo(kQualPlain, entry_type);
}

static void AddCXXVPtrMember(TypeParser* parser, Struct* str) {
  if (!CompilerIsCXX() || str->vptr_member != NULL ||
      str->virtual_members.length == 0 || StructHasPolymorphicBase(str)) {
    return;
  }
  int ptr_size = SizeofPointer();
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    base->byte_offset += ptr_size;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member->is_static || member->is_member_function ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    member->byte_offset += ptr_size;
  }
  str->next_offset += ptr_size;
  str->current_offset += ptr_size;
  str->size += ptr_size;
  if (ptr_size > str->alignment) {
    str->alignment = ptr_size;
  }

  Symbol* symbol =
      NewSymbol("__vptr", NewCXXVPtrType(), STO(implicit));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = parser->lex->current_token_location;
  StructMember* member = NewStructMember(symbol);
  member->access = kAccessPublic;
  member->byte_offset = 0;
  str->vptr_member = member;
  VectorAppend(&str->members, member);
  StructInsertMemberIntoTables(str, member);
}

static void AddCXXVBPtrMember(TypeParser* parser, Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->vbptr_member != NULL ||
      str->virtual_bases.length == 0) {
    return;
  }
  Symbol* symbol =
      NewSymbol("__vbptr", NewCXXVBPtrType(), STO(implicit));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = parser->lex->current_token_location;
  StructMember* member = NewStructMember(symbol);
  member->access = kAccessPublic;
  AlignNextOffset(str, symbol->type);
  member->byte_offset = str->next_offset;
  member->index = str->members.length;
  UpdateStructSize(str, symbol->type, str->is_union);
  str->vbptr_member = member;
  VectorAppend(&str->members, member);
  StructInsertMemberIntoTables(str, member);
}

static void LayoutCXXVirtualBaseSpecifiers(Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->virtual_bases.length == 0) {
    return;
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
    AlignNextOffset(str, base->type);
    base->byte_offset = str->next_offset;
    if (!str->is_union) {
      str->next_offset += base->type->size;
      str->size = str->next_offset;
    } else if (base->type->size > str->size) {
      str->size = base->type->size;
    }
  }
}

static TypeRecord* NewCXXVTableType(size_t slots) {
  TypeRecord* entry_type = NewCXXVTableEntryType();
  TypeRecord* array_type =
      NewBasicArrayTypeRecord(kQualPlain, (int)slots, false);
  TypeRecordChain(array_type, entry_type);
  TypeRecordCalculateSize(array_type);
  return array_type;
}

static bool CXXVirtualNamesCompatible(StructMember* candidate,
                                      StructMember* base_member) {
  if (candidate == NULL || candidate->symbol == NULL ||
      candidate->symbol->type == NULL || base_member == NULL ||
      base_member->symbol == NULL || base_member->symbol->type == NULL) {
    return false;
  }
  TypeRecord* candidate_func = candidate->symbol->type;
  TypeRecord* base_func = base_member->symbol->type;
  if (candidate_func->info.function.is_destructor &&
      base_func->info.function.is_destructor) {
    return true;
  }
  return StringEqualString(&candidate->symbol->name, &base_member->symbol->name);
}

// Search a class and (depth-first, most-derived first) its bases for a virtual
// member function that overrides `base_member`.  Returns NULL if none is
// declared anywhere in the hierarchy rooted at `str`.
static StructMember* FindCXXOverriderInHierarchy(Struct* str,
                                                 StructMember* base_member) {
  if (str == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* candidate = str->members.value.p[i];
    if (candidate == NULL || !candidate->is_member_function ||
        candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !candidate->symbol->type->info.function.is_virtual ||
        !CXXVirtualNamesCompatible(candidate, base_member)) {
      continue;
    }
    if (CXXMemberFunctionSignaturesMatch(candidate->symbol->type,
                                         base_member->symbol->type)) {
      return candidate;
    }
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    StructMember* found =
        FindCXXOverriderInHierarchy(base->type->info.struct_info, base_member);
    if (found != NULL) {
      return found;
    }
  }
  return NULL;
}

static StructMember* FindCXXFinalOverrider(Struct* complete,
                                           StructMember* base_member) {
  if (complete == NULL || base_member == NULL || base_member->symbol == NULL ||
      base_member->symbol->type == NULL) {
    return base_member;
  }
  // Walk the whole complete-object hierarchy, not just its directly declared
  // members: an intermediate base (e.g. `B` in `A <- B <- C`) may carry the
  // final overrider that a grandbase-source subobject vtable must point at.
  StructMember* overrider = FindCXXOverriderInHierarchy(complete, base_member);
  return overrider != NULL ? overrider : base_member;
}

static Symbol* RegisterCXXThisAdjustorThunk(TypeParser* parser, Symbol* target,
                                            int adjustment) {
  if (adjustment == 0 || target == NULL) {
    return target;
  }
  for (size_t i = 0; i < compiler->cxx_this_adjustor_thunks.length; i++) {
    CXXThisAdjustorThunk* thunk = compiler->cxx_this_adjustor_thunks.value.p[i];
    if (thunk->target == target && thunk->this_adjustment == adjustment) {
      return thunk->thunk;
    }
  }
  String name;
  StringInit(&name, "__davecc_this_adjustor_");
  char suffix[64];
  snprintf(suffix, sizeof(suffix), "%zu_%d_",
           compiler->cxx_this_adjustor_thunks.length,
           adjustment < 0 ? -adjustment : adjustment);
  StringAppend(&name, suffix);
  StringAppendString(&name, &target->name);
  for (size_t i = 0; i < name.length; i++) {
    char ch = name.value[i];
    if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
          (ch >= '0' && ch <= '9') || ch == '_')) {
      name.value[i] = '_';
    }
  }
  Symbol* thunk_symbol =
      NewSymbol(name.value, TypeRecordCopy(target->type), STO(static));
  thunk_symbol->flags.invented = true;
  thunk_symbol->flags.is_defined = true;
  thunk_symbol->location = parser->lex->current_token_location;
  SyntaxAddSymbol(parser->syntax, thunk_symbol);
  StringDestruct(&name);

  CXXThisAdjustorThunk* thunk = malloc(sizeof(CXXThisAdjustorThunk));
  thunk->thunk = thunk_symbol;
  thunk->target = target;
  thunk->this_adjustment = adjustment;
  VectorAppend(&compiler->cxx_this_adjustor_thunks, thunk);
  return thunk_symbol;
}

static TypeRecord* NewCXXVBTableType(size_t slots) {
  TypeRecord* entry_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  TypeRecord* array_type =
      NewBasicArrayTypeRecord(kQualPlain, (int)slots, false);
  TypeRecordChain(array_type, entry_type);
  TypeRecordCalculateSize(array_type);
  return array_type;
}

static void UpdateCXXAbstractStatus(Struct* str) {
  str->is_abstract = false;
  if (!CompilerIsCXX()) {
    return;
  }
  for (size_t i = 0; i < str->virtual_members.length; i++) {
    StructMember* member = str->virtual_members.value.p[i];
    if (member != NULL && member->symbol != NULL &&
        TypeIsFunction(member->symbol->type) &&
        member->symbol->type->info.function.is_pure_virtual) {
      str->is_abstract = true;
      return;
    }
  }
}

static Symbol* RegisterCXXVTableForSubobject(TypeParser* parser,
                                             Struct* complete,
                                             Struct* source,
                                             int source_offset) {
  if (!CompilerIsCXX() || complete == NULL || source == NULL ||
      source->virtual_members.length == 0 || complete->tag_name == NULL ||
      source->tag_name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < complete->vtable_symbols.length; i++) {
    CXXVTableInfo* info = complete->vtable_symbols.value.p[i];
    if (info->source == source && info->source_offset == source_offset) {
      return info->symbol;
    }
  }

  String name;
  StringInit(&name, "__davecc_vtbl_");
  StringAppendString(&name, complete->tag_name);
  if (!(complete == source && source_offset == 0)) {
    StringAppend(&name, "_");
    StringAppendString(&name, source->tag_name);
    StringAppend(&name, "_");
    char offset_suffix[32];
    snprintf(offset_suffix, sizeof(offset_suffix), "%d", source_offset);
    StringAppend(&name, offset_suffix);
  }
  // The vtable carries two RTTI header entries (offset_to_top and a pointer to
  // the complete object's type_info) immediately before the function pointers.
  // __vptr still points at the first function pointer (entry index 2) so that
  // virtual_index-based dispatch is unchanged.
  Symbol* symbol = NewSymbol(name.value,
                             NewCXXVTableType(source->virtual_members.length + 2),
                             STO(static));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = parser->lex->current_token_location;
  SyntaxAddSymbol(parser->syntax, symbol);
  if (complete == source && source_offset == 0) {
    complete->vtable_symbol = symbol;
  }
  StringDestruct(&name);

  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = symbol;
  var->is_global = false;
  var->is_weak = false;
  var->size = symbol->type->size;
  var->alignment = TypeRecordAlignment(symbol->type->next);
  VectorInit(&var->initializers);
  var->is_tls = false;
  var->is_local = false;

  int ptr_size = SizeofPointer();
  // Header entry 0: offset_to_top -- the byte distance from this subobject's
  // vptr back to the most-derived object (0 for the primary table).
  Initializer* ott = malloc(sizeof(Initializer));
  ott->offset = 0;
  switch (ptr_size) {
    case 8:
      ott->type = kInitTypeLong;
      ott->value._long = (uint64_t)(int64_t)(-source_offset);
      break;
    case 2:
      ott->type = kInitTypeHalf;
      ott->value.half = (uint16_t)(-source_offset);
      break;
    default:
      ott->type = kInitTypeWord;
      ott->value.word = (uint32_t)(-source_offset);
      break;
  }
  VectorAppend(&var->initializers, ott);
  // Header entry 1: pointer to the complete object's type_info.
  TypeRecord* complete_type = NewTypeRecord(kTypeStruct, kQualPlain);
  complete_type->info.struct_info = complete;
  TypeRecordCalculateSize(complete_type);
  Symbol* type_info = RttiGetTypeInfoSymbol(complete_type);
  Initializer* ti = malloc(sizeof(Initializer));
  ti->offset = (int32_t)ptr_size;
  if (type_info != NULL) {
    ti->type = kInitTypeSymbol;
    ti->value.symbol = type_info;
  } else {
    ti->type = (ptr_size == 8) ? kInitTypeLong : kInitTypeWord;
    ti->value._long = 0;
  }
  VectorAppend(&var->initializers, ti);

  for (size_t i = 0; i < source->virtual_members.length; i++) {
    StructMember* member = source->virtual_members.value.p[i];
    member = FindCXXFinalOverrider(complete, member);
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    Initializer* init = malloc(sizeof(Initializer));
    init->offset = (int32_t)((i + 2) * SizeofPointer());
    if (member->symbol->type->info.function.is_pure_virtual) {
      if (SizeofPointer() == 8) {
        init->type = kInitTypeLong;
        init->value._long = 0;
      } else {
        init->type = kInitTypeWord;
        init->value.word = 0;
      }
    } else {
      init->type = kInitTypeSymbol;
      int adjustment = member->symbol->type->info.function.cxx_member_owner ==
                               complete
                           ? -source_offset
                           : 0;
      init->value.symbol =
          RegisterCXXThisAdjustorThunk(parser, member->symbol, adjustment);
    }
    VectorAppend(&var->initializers, init);
  }
  VectorAppend(&compiler->initialized_static_variables, var);
  CXXVTableInfo* info = malloc(sizeof(CXXVTableInfo));
  info->source = source;
  info->source_offset = source_offset;
  info->symbol = symbol;
  VectorAppend(&complete->vtable_symbols, info);
  return symbol;
}

static void RegisterCXXVSubobjectTables(TypeParser* parser,
                                        Struct* complete,
                                        Struct* source,
                                        int source_offset) {
  if (source == NULL) {
    return;
  }
  if (source->virtual_members.length > 0) {
    RegisterCXXVTableForSubobject(parser, complete, source, source_offset);
  }
  for (size_t i = 0; i < source->bases.length; i++) {
    CXXBaseSpecifier* base = source->bases.value.p[i];
    if (base->is_virtual || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    RegisterCXXVSubobjectTables(parser, complete, base->type->info.struct_info,
                                source_offset + base->byte_offset);
  }
}

static void RegisterCXXVTable(TypeParser* parser, Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->vtable_symbol != NULL ||
      str->virtual_members.length == 0 || str->tag_name == NULL) {
    return;
  }
  RegisterCXXVSubobjectTables(parser, str, str, 0);
}

static Symbol* RegisterCXXVBTableForSubobject(TypeParser* parser,
                                              Struct* complete,
                                              Struct* source,
                                              int source_offset) {
  if (!CompilerIsCXX() || complete == NULL || source == NULL ||
      source->virtual_bases.length == 0 || complete->tag_name == NULL ||
      source->tag_name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < complete->vbtable_symbols.length; i++) {
    CXXVBTableInfo* info = complete->vbtable_symbols.value.p[i];
    if (info->source == source && info->source_offset == source_offset) {
      return info->symbol;
    }
  }
  String name;
  StringInit(&name, "__davecc_vbtbl_");
  StringAppendString(&name, complete->tag_name);
  StringAppend(&name, "_");
  StringAppendString(&name, source->tag_name);
  StringAppend(&name, "_");
  char offset_suffix[32];
  snprintf(offset_suffix, sizeof(offset_suffix), "%d", source_offset);
  StringAppend(&name, offset_suffix);
  Symbol* symbol = NewSymbol(name.value,
                             NewCXXVBTableType(source->virtual_bases.length),
                             STO(static));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = parser->lex->current_token_location;
  SyntaxAddSymbol(parser->syntax, symbol);
  if (complete == source && source_offset == 0) {
    complete->vbtable_symbol = symbol;
  }
  StringDestruct(&name);

  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = symbol;
  var->is_global = false;
  var->is_weak = false;
  var->size = symbol->type->size;
  var->alignment = TypeRecordAlignment(symbol->type->next);
  VectorInit(&var->initializers);
  var->is_tls = false;
  var->is_local = false;
  for (size_t i = 0; i < source->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* source_base = source->virtual_bases.value.p[i];
    CXXVirtualBaseInfo* complete_base =
        FindCXXVirtualBaseInfo(complete, source_base->type);
    int offset = complete_base != NULL
                     ? complete_base->byte_offset - source_offset
                     : source_base->byte_offset;
    Initializer* init = malloc(sizeof(Initializer));
    init->offset = (int32_t)(i * (size_t)SizeofType(kTypeInt));
    init->type = kInitTypeWord;
    init->value.word = (uint32_t)offset;
    VectorAppend(&var->initializers, init);
  }
  VectorAppend(&compiler->initialized_static_variables, var);

  CXXVBTableInfo* info = malloc(sizeof(CXXVBTableInfo));
  info->source = source;
  info->source_offset = source_offset;
  info->symbol = symbol;
  VectorAppend(&complete->vbtable_symbols, info);
  return symbol;
}

static void RegisterCXXVBSubobjectTables(TypeParser* parser,
                                         Struct* complete,
                                         Struct* source,
                                         int source_offset) {
  if (source == NULL) {
    return;
  }
  if (source->virtual_bases.length > 0) {
    RegisterCXXVBTableForSubobject(parser, complete, source, source_offset);
  }
  for (size_t i = 0; i < source->bases.length; i++) {
    CXXBaseSpecifier* base = source->bases.value.p[i];
    if (base->is_virtual || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    RegisterCXXVBSubobjectTables(parser, complete,
                                 base->type->info.struct_info,
                                 source_offset + base->byte_offset);
  }
}

static void RegisterCXXVBTables(TypeParser* parser, Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->virtual_bases.length == 0 ||
      str->tag_name == NULL) {
    return;
  }
  RegisterCXXVBSubobjectTables(parser, str, str, 0);
}

Symbol* StructFindVBTableSymbol(Struct* complete, Struct* source,
                                int source_offset) {
  if (complete == NULL || source == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < complete->vbtable_symbols.length; i++) {
    CXXVBTableInfo* info = complete->vbtable_symbols.value.p[i];
    if (info->source == source && info->source_offset == source_offset) {
      return info->symbol;
    }
  }
  return NULL;
}

Symbol* StructFindVTableSymbol(Struct* complete, Struct* source,
                               int source_offset) {
  if (complete == NULL || source == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < complete->vtable_symbols.length; i++) {
    CXXVTableInfo* info = complete->vtable_symbols.value.p[i];
    if (info->source == source && info->source_offset == source_offset) {
      return info->symbol;
    }
  }
  return NULL;
}

// Parse a bitfield.  We are just after the : in the member definition.
// We will parse a constant integer expression and calculate the bit position,
// byte position and bit width for the current member, moving on to the next
// word if the it doesn't fit.
static void ParseBitField(TypeParser* parser, bool is_union, Struct* str,
                          Symbol* member_symbol, StructMember* member) {
  char error[256];
  ASTNode* width_node =
      SyntaxParseSingleExpression(parser->syntax, TC(semicolon));
  if (width_node == NULL) {
    snprintf(error, sizeof(error), "constant expression needed");
    goto error;
  }
  int64_t bit_width = 0;
  if (!EvaluateIntegerExpression(width_node, &bit_width)) {
    ASTNodeDelete(width_node);
    snprintf(error, sizeof(error), "constant expression needed");
    goto error;
  }
  ASTNodeDelete(width_node);
  if (!TypeIsIntegral(member_symbol->type)) {
    snprintf(error, sizeof(error),
             "only integer types can be used for bitfields");
    goto error;
  }
  int word_width = member_symbol->type->size * 8;
  if (bit_width <= 0 || bit_width > word_width) {
    snprintf(error, sizeof(error),
             "width of %" PRId64 " is out of bounds for type of size %d bits",
             bit_width, word_width);
    goto error;
  }
  int width = (int)bit_width;
  if (str->next_bit_pos + width > word_width) {
    // No room in current word for bit field (or first bit field).  We align
    // to the next boundary based on the bitfield type and add a new word
    // (of the appropriate type) to the struct.
    AlignNextOffsetForSymbol(
        str, member_symbol);  // Will set current_offset and next_offset.
    member->byte_offset = str->next_offset;
    member->index = str->members.length - 1;
    str->next_bit_pos = 0;
    if (!is_union) {
      str->next_offset += member_symbol->type->size;
      str->size = str->next_offset;
    } else {
      if (member_symbol->type->size > str->size) {
        str->size = member_symbol->type->size;
      }
    }
  } else {
    // There is room in the current word for the bitfield.
    member->byte_offset = str->current_offset;
  }
  member->bit_size = width;
  member->bit_offset = str->next_bit_pos;
  if (!is_union) {
    str->next_bit_pos += width;
  }
  return;

error:
  // If we get here we have an error.
  SyntaxError(parser->syntax, "Invalid bitfield; %s", error);
}

static void UpdateStructSize(Struct* str, TypeRecord* member_type, bool is_union) {
   // Update the struct offset and size based on the
   // anonymous member.
   if (!is_union) {
     str->next_offset += member_type->size;
     str->size = str->next_offset;
   } else {
     // The size of a union is the maximum size of its members.
     if (member_type->size > str->size) {
       str->size = member_type->size;
     }
   }
}

static bool CXXMemberFunctionSignaturesMatch(TypeRecord* a, TypeRecord* b) {
  if (!TypeIsFunction(a) || !TypeIsFunction(b) ||
      !TypeEqual(a->next, b->next) ||
      a->info.function.is_const_member != b->info.function.is_const_member ||
      a->info.function.ref_qualifier != b->info.function.ref_qualifier) {
    return false;
  }
  size_t a_first = a->info.function.cxx_member_owner != NULL ? 1 : 0;
  size_t b_first = b->info.function.cxx_member_owner != NULL ? 1 : 0;
  if (a->info.function.prototype.length - a_first !=
      b->info.function.prototype.length - b_first) {
    return false;
  }
  for (size_t i = 0; i < a->info.function.prototype.length - a_first; i++) {
    Symbol* a_arg = a->info.function.prototype.value.p[i + a_first];
    Symbol* b_arg = b->info.function.prototype.value.p[i + b_first];
    if (!TypeEqual(a_arg->type, b_arg->type)) {
      return false;
    }
  }
  return true;
}

static bool LexMatchContextualIdentifier(Lex* lex, const char* name) {
  if (!LexLookingAt(lex, TOK(identifier)) ||
      !StringEqual(&lex->spelling, name)) {
    return false;
  }
  LexNextToken(lex);
  return true;
}

static void ParseCXXVirtSpecifiers(TypeParser* parser, TypeRecord* func) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func)) {
    return;
  }
  bool progress = true;
  while (progress) {
    progress = false;
    if (LexMatchContextualIdentifier(parser->lex, "override")) {
      if (func->info.function.is_override) {
        SyntaxError(parser->syntax, "duplicate override specifier");
      }
      func->info.function.is_override = true;
      progress = true;
    } else if (LexMatchContextualIdentifier(parser->lex, "final")) {
      if (func->info.function.is_final) {
        SyntaxError(parser->syntax, "duplicate final specifier");
      }
      func->info.function.is_final = true;
      progress = true;
    }
  }
}

static void ParseCXXPureSpecifier(TypeParser* parser, TypeRecord* func) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func) ||
      !LexMatch(parser->lex, TOK(equal))) {
    return;
  }
  if (LexMatch(parser->lex, TOK(default))) {
    func->info.function.is_defaulted = true;
    func->info.function.is_explicitly_defaulted = true;
    func->info.function.is_constexpr_eligible = true;
    func->info.function.is_inline = true;
    return;
  }
  if (LexMatch(parser->lex, TOK(delete))) {
    func->info.function.is_deleted = true;
    func->info.function.is_explicitly_deleted = true;
    return;
  }
  if (!LexLookingAt(parser->lex, TOK(number)) || parser->lex->number != 0) {
    SyntaxError(parser->syntax,
                "function specifier must be '= 0', '= default', or '= delete'");
    if (!LexLookingAt(parser->lex, TOK(semicolon)) &&
        !LexLookingAt(parser->lex, TOK(rbrace))) {
      LexNextToken(parser->lex);
    }
    return;
  }
  LexNextToken(parser->lex);
  if (!func->info.function.is_virtual) {
    SyntaxError(parser->syntax, "pure specifier requires a virtual function");
  }
  func->info.function.is_pure_virtual = true;
}

static StructMember* FindCXXBaseVirtualOverride(Struct* str,
                                                StructMember* member) {
  if (str == NULL || member == NULL || member->symbol == NULL ||
      !member->is_member_function) {
    return NULL;
  }
  TypeRecord* func = member->symbol->type;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    String destructor_name = {0};
    StructMember* base_member = NULL;
    if (func->info.function.is_destructor) {
      StringInit(&destructor_name, "~");
      StringAppendString(&destructor_name,
                         base->type->info.struct_info->tag_name);
      base_member = FindStructMember(base->type->info.struct_info,
                                     &destructor_name);
    } else {
      base_member =
          FindStructMember(base->type->info.struct_info, &member->symbol->name);
    }
    for (StructMember* candidate = base_member; candidate != NULL;
         candidate = candidate->overload_next) {
      if (candidate->is_member_function &&
          candidate->symbol->type->info.function.is_virtual &&
          CXXMemberFunctionSignaturesMatch(member->symbol->type,
                                           candidate->symbol->type)) {
        if (destructor_name.value != NULL) {
          StringDestruct(&destructor_name);
        }
        return candidate;
      }
    }
    if (destructor_name.value != NULL) {
      StringDestruct(&destructor_name);
    }
  }
  return NULL;
}

static int CXXBaseOffsetForMember(Struct* str, StructMember* member) {
  if (str == NULL || member == NULL || member->symbol == NULL ||
      member->symbol->type == NULL ||
      member->symbol->type->info.function.cxx_member_owner == NULL) {
    return 0;
  }
  Struct* owner = member->symbol->type->info.function.cxx_member_owner;
  if (owner == str) {
    return 0;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (base_struct == owner) {
      return base->byte_offset;
    }
    int nested_offset = CXXBaseOffsetForMember(base_struct, member);
    if (nested_offset != 0 || base_struct == owner) {
      return base->byte_offset + nested_offset;
    }
  }
  return 0;
}

static void RegisterCXXVirtualMember(TypeParser* parser, Struct* str,
                                     StructMember* member) {
  if (!CompilerIsCXX() || str == NULL || member == NULL ||
      !member->is_member_function || member->is_static ||
      member->symbol == NULL || !TypeIsFunction(member->symbol->type)) {
    return;
  }
  TypeRecord* func = member->symbol->type;
  StructMember* override = FindCXXBaseVirtualOverride(str, member);
  if (func->info.function.is_override && override == NULL) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(member->symbol, &suffix);
    SyntaxError(parser->syntax, "%s marked override but does not override%s",
                member->symbol->name.value, suffix.value);
    StringDestruct(&suffix);
  }
  if (override != NULL) {
    if (override->symbol->type->info.function.is_final) {
      String suffix;
      StringInit(&suffix, NULL);
      SymbolFunctionDiagnosticSuffix(member->symbol, &suffix);
      SyntaxError(parser->syntax, "%s overrides final function%s",
                  member->symbol->name.value, suffix.value);
      StringDestruct(&suffix);
    }
    func->info.function.is_virtual = true;
    func->info.function.virtual_index =
        override->symbol->type->info.function.virtual_index;
    member->cxx_vcall_offset = CXXBaseOffsetForMember(str, override);
  }
  if (func->info.function.is_final && !func->info.function.is_virtual) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(member->symbol, &suffix);
    SyntaxError(parser->syntax, "%s marked final but is not virtual%s",
                member->symbol->name.value, suffix.value);
    StringDestruct(&suffix);
  }
  if (func->info.function.is_pure_virtual && !func->info.function.is_virtual) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(member->symbol, &suffix);
    SyntaxError(parser->syntax, "%s is pure but is not virtual%s",
                member->symbol->name.value, suffix.value);
    StringDestruct(&suffix);
  }
  if (!func->info.function.is_virtual) {
    return;
  }
  if (func->info.function.virtual_index < 0) {
    func->info.function.virtual_index = (int)str->virtual_members.length;
    VectorAppend(&str->virtual_members, member);
    return;
  }
  if (member->cxx_vcall_offset != 0) {
    return;
  }
  size_t index = (size_t)func->info.function.virtual_index;
  while (str->virtual_members.length <= index) {
    VectorAppend(&str->virtual_members, NULL);
  }
  VectorSet(&str->virtual_members, index, member);
}

static void AddStructMember(TypeParser* parser, Struct* str,
                            StructMember* member) {
  if (member->is_member_function) {
    RegisterCXXVirtualMember(parser, str, member);
    SymbolSetCXXMangledAsmName(member->symbol);
  } else if (member->is_static && member->symbol != NULL) {
    SymbolSetCXXDataAsmName(member->symbol, str);
  }
  VectorAppend(&str->members, member);
  StructInsertMemberIntoTables(str, member);
}

void StructAddSyntheticMember(Struct* str, StructMember* member) {
  if (member->is_member_function) {
    SymbolSetCXXMangledAsmName(member->symbol);
  } else if (!member->is_static) {
    AlignNextOffset(str, member->symbol->type);
    member->byte_offset = str->next_offset;
  }
  member->index = str->members.length;
  VectorAppend(&str->members, member);
  StructInsertMemberIntoTables(str, member);
  if (!member->is_static && !member->is_member_function) {
    UpdateStructSize(str, member->symbol->type, str->is_union);
  }
}

static bool CanOverloadStructMember(StructMember* existing,
                                    StructMember* member) {
  return CompilerIsCXX() && existing != NULL && member != NULL &&
         existing->is_member_function && member->is_member_function;
}

static bool SymbolIsCXXAllocationFunction(Symbol* symbol) {
  if (!CompilerIsCXX() || symbol == NULL) {
    return false;
  }
  return strcmp(symbol->name.value, "operator new") == 0 ||
         strcmp(symbol->name.value, "operator new[]") == 0 ||
         strcmp(symbol->name.value, "operator delete") == 0 ||
         strcmp(symbol->name.value, "operator delete[]") == 0;
}

static bool ParseCXXExplicitSpecifier(TypeParser* parser, bool* saw_explicit) {
  *saw_explicit = false;
  if (!CompilerIsCXX() || !LexMatch(parser->lex, TOK(explicit))) {
    return false;
  }
  *saw_explicit = true;
  if (!LexMatch(parser->lex, TOK(lparen))) {
    return true;
  }
  ASTNode* expr =
      SyntaxParseExpression(parser->syntax, TC(closebra) | TC(exprsep));
  // A value-dependent condition (e.g. `explicit(sizeof(T) > 4)`) is detected on
  // the parsed (pre-analysis) tree, before constant-folding could collapse it
  // to the placeholder's size.  The unanalyzed condition is stashed and
  // re-folded per instantiation (see ParseCXXExplicitDeclarationSpecifier and
  // InstantiateMemberFunctionType); mirrors static_assert deferral.
  if (parser->syntax->parsing_template_declaration &&
      ExpressionIsTemplateDependent(expr)) {
    SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(decl));
    parser->syntax->pending_explicit_condition = expr;
    return true;
  }
  expr = AnalyzeExpression(expr);
  int64_t value = 0;
  bool ok = EvaluateIntegerExpression(expr, &value);
  if (!ok) {
    SyntaxError(parser->syntax,
                "explicit specifier must be a constant expression");
  }
  ASTNodeDelete(expr);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(decl));
  return ok && value != 0;
}

static void SetStructMemberOverloadAsmName(Struct* str,
                                           StructMember* first,
                                           StructMember* overload) {
  (void)str;
  (void)first;
  SymbolSetCXXMangledAsmName(overload->symbol);
}

static void AppendStructMemberOverload(TypeParser* parser, Struct* str,
                                       StructMember* first,
                                       StructMember* member) {
  VectorAppend(&str->members, member);
  StructMember* tail = first;
  while (tail->overload_next != NULL) {
    tail = tail->overload_next;
  }
  tail->overload_next = member;
  first->symbol->flags.is_overloaded = true;
  member->symbol->flags.is_overloaded = true;
  RegisterCXXVirtualMember(parser, str, member);
  SetStructMemberOverloadAsmName(str, first, first);
  SetStructMemberOverloadAsmName(str, first, member);
  (void)parser;
}

static bool SkipInlineMemberFunctionBody(TypeParser* parser) {
  if (!LexMatch(parser->lex, TOK(lbrace))) {
    return false;
  }
  int brace_count = 1;
  while (brace_count > 0 && !LexEof(parser->lex)) {
    if (LexLookingAt(parser->lex, TOK(lbrace))) {
      brace_count++;
    } else if (LexLookingAt(parser->lex, TOK(rbrace))) {
      brace_count--;
    }
    LexNextToken(parser->lex);
  }
  return true;
}

static void AddInlineFunctionScopeSymbols(Syntax* syntax, TypeRecord* func) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = formals->value.p[i];
    InsertLocalSymbol(syntax->local_symbol_stack, formal);
  }
}

static void QueueInlineMemberFunctionDefinition(Symbol* symbol) {
  Vector* declarations = NewVector();
  VectorAppend(declarations,
               NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
  VectorAppend(&compiler->pending_template_instantiations,
               NewDeclarationListASTNode(declarations, symbol->location));
}

static bool TypeNeedsCXXCompleteObjectArgument(TypeRecord* type) {
  return TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
         StructHasVirtualBases(type->info.struct_info);
}

static void CXXPrependCompleteObjectArgument(TypeRecord* type, Vector* actuals,
                                             SourceLocation location) {
  if (actuals == NULL || !TypeNeedsCXXCompleteObjectArgument(type)) {
    return;
  }
  ASTNode* arg =
      NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  if (actuals->length == 0) {
    VectorAppend(actuals, arg);
  } else {
    VectorInsertBefore(actuals, 0, arg);
  }
}

static StructMember* FindCXXDestructorForObjectType(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL || type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(type->info.struct_info, &destructor_name);
  StringDestruct(&destructor_name);
  if (destructor == NULL || !destructor->is_member_function ||
      destructor->symbol == NULL || destructor->symbol->type == NULL ||
      !destructor->symbol->type->info.function.is_destructor) {
    return NULL;
  }
  return destructor;
}

static TypeRecord* CXXDestructibleElementType(TypeRecord* type) {
  if (TypeIsFixedArray(type) && type->next != NULL &&
      FindCXXDestructorForObjectType(type->next) != NULL) {
    return type->next;
  }
  if (FindCXXDestructorForObjectType(type) != NULL) {
    return type;
  }
  return NULL;
}

static ASTNode* NewCXXMemberReceiver(TypeRecord* func, StructMember* member,
                                     SourceLocation location) {
  if (func == NULL || func->info.function.prototype.length == 0 ||
      member == NULL || member->symbol == NULL) {
    return NULL;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  ASTNode* this_node = NewIdentifierASTNode(this_symbol, location);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(member->symbol->name.value), NULL,
                               location);
  return NewBinaryASTNode(AST_OP(arrow), NULL, location, this_node,
                          member_name);
}

static ASTNode* NewCXXMemberDestructorCall(TypeRecord* func,
                                           TypeRecord* object_type,
                                           ASTNode* receiver,
                                           SourceLocation location) {
  if (func == NULL || object_type == NULL ||
      object_type->info.struct_info == NULL ||
      object_type->info.struct_info->tag_name == NULL) {
    ASTNodeDelete(receiver);
    return NULL;
  }
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(object_type, actuals, location);
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, object_type->info.struct_info->tag_name);
  ASTNode* destructor =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL, location);
  StringDestruct(&destructor_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, destructor);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  return NewExpressionStatementASTNode(call, location);
}

static void AppendCXXMemberDestructorCalls(TypeRecord* func, Vector* body,
                                           SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_destructor ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = owner->members.length; i > 0; i--) {
    StructMember* member = owner->members.value.p[i - 1];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || StructMemberIsNestedType(member)) {
      continue;
    }
    TypeRecord* member_type = member->symbol->type;
    TypeRecord* object_type = CXXDestructibleElementType(member_type);
    if (object_type == NULL) {
      continue;
    }
    if (TypeIsFixedArray(member_type)) {
      for (size_t index = member_type->info.array.size.fixed; index > 0;
           index--) {
        ASTNode* receiver = NewCXXMemberReceiver(func, member, location);
        ASTNode* index_node = NewIntConstantASTNode(
            (int64_t)index - 1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
            location);
        receiver = NewBinaryASTNode(AST_OP(subscript), NULL, location,
                                    receiver, index_node);
        ASTNode* call = NewCXXMemberDestructorCall(func, object_type, receiver,
                                                   location);
        if (call != NULL) {
          VectorAppend(body, call);
        }
      }
    } else {
      ASTNode* receiver = NewCXXMemberReceiver(func, member, location);
      ASTNode* call = NewCXXMemberDestructorCall(func, object_type, receiver,
                                                 location);
      if (call != NULL) {
        VectorAppend(body, call);
      }
    }
  }
}

static bool ParseInlineMemberFunctionBody(TypeParser* parser,
                                          Symbol* member_symbol) {
  Syntax* syntax = parser->syntax;
  ParserContext old_context = syntax->context;
  syntax->context = kParsingBlockScope;
  SyntaxOpenScope(syntax);
  AddInlineFunctionScopeSymbols(syntax, member_symbol->type);

  CXXConstructorInitList cxx_initializers;
  SyntaxCXXConstructorInitListInit(&cxx_initializers);
  SyntaxParseCXXConstructorInitializerList(syntax, member_symbol->type,
                                           &cxx_initializers);
  if (!LexMatch(parser->lex, TOK(lbrace))) {
    SyntaxCloseScope(syntax);
    syntax->context = old_context;
    SyntaxCXXConstructorInitListDestruct(&cxx_initializers);
    return false;
  }

  member_symbol->flags.is_defined = true;
  member_symbol->flags.is_inline_defn = true;
  if (!StorageIs(member_symbol->storage, STO(static))) {
    member_symbol->flags.is_weak = true;
  }
  member_symbol->type->info.function.is_inline = true;
  member_symbol->type->info.function.definition = true;
  member_symbol->type->info.function.is_user_provided = true;
  member_symbol->value.func_defn = member_symbol;

  TypeRecord* old_current_function = compiler->current_function;
  compiler->current_function = member_symbol->type;
  Vector* body = NewVector();
  bool seen_statement = false;
  while (!LexLookingAt(parser->lex, TOK(rbrace)) && !LexEof(parser->lex)) {
    ASTNode* stmt;
    if (SyntaxLookingAtDeclaration(syntax)) {
      if (seen_statement) {
        SyntaxWarning(syntax, "declaration-after-statement",
                      "declaration after statement");
      }
      stmt = SyntaxParseLocalDeclaration(syntax);
    } else {
      seen_statement = true;
      stmt = SyntaxParseStatement(syntax, TC(semicolon));
    }
    if (stmt != NULL) {
      VectorAppend(body, stmt);
    }
  }

  SyntaxInsertCXXConstructorPreamble(syntax, member_symbol->type, body,
                                     &cxx_initializers,
                                     member_symbol->location);
  AppendCXXMemberDestructorCalls(member_symbol->type, body,
                                 member_symbol->location);
  AppendCXXBaseDestructorCalls(syntax, member_symbol->type, body,
                               member_symbol->location);
  SyntaxCloseScope(syntax);
  syntax->context = old_context;
  compiler->current_function = old_current_function;
  member_symbol->type->info.function.body =
      NewCompoundStatementASTNode(body, parser->lex->current_token_location);
  VectorAppend(&compiler->declaration_asts,
               member_symbol->type->info.function.body);
  SyntaxNeedBracket(syntax, TOK(rbrace), TC(decl));

  if (!syntax->parsing_template_declaration) {
    QueueInlineMemberFunctionDefinition(member_symbol);
  }
  LexMatch(parser->lex, TOK(semicolon));
  SyntaxCXXConstructorInitListDestruct(&cxx_initializers);
  return true;
}

static bool CXXFunctionNeedsMemberwiseCopy(TypeRecord* func);
static Symbol* CXXSourceObjectParameter(TypeRecord* func);
static void AppendCXXMemberwiseAssignments(TypeRecord* func, Vector* body,
                                           Struct* owner,
                                           SourceLocation location);
static void AppendCXXAssignmentReturnThis(TypeRecord* func, Vector* body,
                                          SourceLocation location);
static ASTNode* NewCXXSourceMemberReceiver(Symbol* source, StructMember* member,
                                           SourceLocation location);

static bool CXXReferenceTargetsStruct(TypeRecord* ref, Struct* owner) {
  return TypeIsReference(ref) && ref->next != NULL &&
         TypeIsStructOrUnion(ref->next) &&
         ref->next->info.struct_info == owner;
}

static bool CXXFunctionHasOnlyImplicitObjectParameters(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return false;
  }
  size_t implicit_count = 0;
  if (func->info.function.cxx_member_owner != NULL) {
    implicit_count++;
  }
  if ((func->info.function.is_constructor ||
       func->info.function.is_destructor) &&
      func->info.function.cxx_member_owner != NULL &&
      StructHasVirtualBases(func->info.function.cxx_member_owner)) {
    implicit_count++;
  }
  return func->info.function.prototype.length == implicit_count;
}

static CXXSpecialMemberKind CXXDetermineSpecialMemberKind(Symbol* symbol,
                                                         Struct* owner) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      owner == NULL) {
    return kCXXSpecialMemberNone;
  }
  TypeRecord* func = symbol->type;
  if (func->info.function.is_destructor) {
    return kCXXSpecialMemberDestructor;
  }
  if (func->info.function.is_constructor) {
    if (CXXFunctionHasOnlyImplicitObjectParameters(func)) {
      return kCXXSpecialMemberDefaultConstructor;
    }
    Symbol* source = CXXSourceObjectParameter(func);
    if (source != NULL && CXXReferenceTargetsStruct(source->type, owner)) {
      return source->type->declarator == kDeclRValueReference
                 ? kCXXSpecialMemberMoveConstructor
                 : kCXXSpecialMemberCopyConstructor;
    }
    return kCXXSpecialMemberNone;
  }
  if (strcmp(symbol->name.value, "operator=") != 0 ||
      func->info.function.prototype.length < 2) {
    return kCXXSpecialMemberNone;
  }
  Symbol* source = CXXSourceObjectParameter(func);
  if (source == NULL || !CXXReferenceTargetsStruct(source->type, owner)) {
    return kCXXSpecialMemberNone;
  }
  return source->type->declarator == kDeclRValueReference
             ? kCXXSpecialMemberMoveAssignment
             : kCXXSpecialMemberCopyAssignment;
}

static void CXXFinalizeSpecialMemberMetadata(Symbol* symbol, Struct* owner,
                                             bool user_declared) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return;
  }
  TypeRecord* func = symbol->type;
  func->info.function.cxx_special_member_kind =
      CXXDetermineSpecialMemberKind(symbol, owner);
  if (user_declared) {
    func->info.function.is_user_declared = true;
  }
  if (func->info.function.is_defaulted) {
    func->info.function.is_explicitly_defaulted =
        !func->info.function.is_implicitly_declared;
    func->info.function.is_constexpr_eligible = true;
  }
  if (func->info.function.is_deleted) {
    func->info.function.is_explicitly_deleted =
        !func->info.function.is_implicitly_declared;
    func->info.function.is_implicitly_deleted =
        func->info.function.is_implicitly_declared;
  }
}

static bool CXXFunctionIsThreeWayComparison(TypeRecord* func) {
  return func != NULL && TypeIsFunction(func) &&
         func->info.function.symbol != NULL &&
         strcmp(func->info.function.symbol->name.value, "operator<=>") == 0;
}

static bool CXXFunctionIsEqualityComparison(TypeRecord* func) {
  return func != NULL && TypeIsFunction(func) &&
         func->info.function.symbol != NULL &&
         strcmp(func->info.function.symbol->name.value, "operator==") == 0;
}

// Synthesize the body of a defaulted `operator==`: return the conjunction of
// memberwise `==` comparisons (`true` for an empty class).
static void AppendCXXEqualityComparison(Symbol* member_symbol, Vector* body,
                                        SourceLocation location) {
  TypeRecord* func = member_symbol->type;
  Struct* owner = func->info.function.cxx_member_owner;
  Symbol* source = CXXSourceObjectParameter(func);
  if (owner == NULL || source == NULL) {
    return;
  }
  ASTNode* result = NULL;
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function) {
      continue;
    }
    ASTNode* lhs = NewCXXMemberReceiver(func, member, location);
    ASTNode* rhs = NewCXXSourceMemberReceiver(source, member, location);
    ASTNode* eq = NewBinaryASTNode(AST_OP(equal), NULL, location, lhs, rhs);
    result = result == NULL
                 ? eq
                 : NewBinaryASTNode(AST_OP(logand), NULL, location, result, eq);
  }
  if (result == NULL) {
    result = NewIntConstantASTNode(
        1, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  VectorAppend(body,
               NewCombinedStatementASTNode(AST_OP(return), result, NULL,
                                           location));
}

// Locate a named static constant (e.g. `less`) of a comparison-category type.
static Symbol* CXXComparisonCategoryConstant(TypeRecord* category,
                                             const char* name) {
  if (category == NULL || category->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = category->info.struct_info;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* m = str->members.value.p[i];
    if (m != NULL && m->symbol != NULL && m->is_static &&
        strcmp(m->symbol->name.value, name) == 0) {
      return m->symbol;
    }
  }
  return NULL;
}

// Build `return <category>::<name>;`, or NULL if the constant is missing.
static ASTNode* NewCXXReturnCategoryConstant(TypeRecord* category,
                                             const char* name,
                                             SourceLocation location) {
  Symbol* constant = CXXComparisonCategoryConstant(category, name);
  if (constant == NULL) {
    return NULL;
  }
  ASTNode* value = NewIdentifierASTNode(constant, location);
  return NewCombinedStatementASTNode(AST_OP(return), value, NULL, location);
}

// Only partial_ordering carries an `unordered` result.
static bool CXXCategoryIsPartial(TypeRecord* category) {
  return CXXComparisonCategoryConstant(category, "unordered") != NULL;
}

// Deduce the return category for a defaulted `auto operator<=>`: partial if any
// data member is floating point, otherwise strong.  (weak_ordering is never
// deduced - a documented simplification.)
static TypeRecord* CXXDeduceComparisonCategory(Struct* owner) {
  bool has_floating = false;
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* m = owner->members.value.p[i];
    if (m == NULL || m->symbol == NULL || m->is_static ||
        m->is_member_function) {
      continue;
    }
    if (TypeIsFloatingPoint(m->symbol->type)) {
      has_floating = true;
    }
  }
  return TypeFindCXXComparisonCategory(has_floating ? "partial_ordering"
                                                    : "strong_ordering");
}

// Synthesize the body of a defaulted `operator<=>`: compare each non-static
// data member in declaration order and return the first non-equivalent result,
// otherwise `equivalent`/`equal`.  Each member subobject is compared with
// `<=>` (built-in or overloaded) and the comparison's sign is mapped onto the
// function's return category, so heterogeneous member categories still yield a
// value of the single deduced/declared category.
static void AppendCXXThreeWayComparisons(TypeParser* parser,
                                         Symbol* member_symbol, Vector* body,
                                         SourceLocation location) {
  TypeRecord* func = member_symbol->type;
  Struct* owner = func->info.function.cxx_member_owner;
  Symbol* source = CXXSourceObjectParameter(func);
  if (owner == NULL || source == NULL) {
    return;
  }
  TypeRecord* category = func->next;
  if (category == NULL || (category->type & kTypeAuto) != 0) {
    TypeRecord* deduced = CXXDeduceComparisonCategory(owner);
    if (deduced == NULL) {
      SyntaxError(parser->syntax,
                  "Defaulted 'operator<=>' requires <compare> to be included");
      return;
    }
    category = TypeRecordCalculateSize(TypeRecordCopy(deduced));
    func->next = category;
  }
  bool is_partial = CXXCategoryIsPartial(category);

  struct {
    ASTOpcode op;
    const char* constant;
  } arms[3] = {
      {AST_OP(less), "less"},
      {AST_OP(greater), "greater"},
      {AST_OP(noteq), "unordered"},
  };
  int arm_count = is_partial ? 3 : 2;

  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function) {
      continue;
    }
    for (int a = 0; a < arm_count; a++) {
      ASTNode* ret =
          NewCXXReturnCategoryConstant(category, arms[a].constant, location);
      if (ret == NULL) {
        continue;
      }
      ASTNode* lhs = NewCXXMemberReceiver(func, member, location);
      ASTNode* rhs = NewCXXSourceMemberReceiver(source, member, location);
      ASTNode* cmp =
          NewBinaryASTNode(AST_OP(spaceship), NULL, location, lhs, rhs);
      ASTNode* zero = NewIntConstantASTNode(
          0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
      ASTNode* cond = NewBinaryASTNode(arms[a].op, NULL, location, cmp, zero);
      VectorAppend(body,
                   NewIfStatementASTNode(cond, ret, NULL, false, location));
    }
  }
  ASTNode* equal = NewCXXReturnCategoryConstant(
      category, is_partial ? "equivalent" : "equal", location);
  if (equal == NULL) {
    equal = NewCXXReturnCategoryConstant(category, "equivalent", location);
  }
  if (equal != NULL) {
    VectorAppend(body, equal);
  }
}

static void SynthesizeDefaultedMemberFunctionBody(TypeParser* parser,
                                                  Symbol* member_symbol) {
  if (parser == NULL || member_symbol == NULL || member_symbol->type == NULL ||
      !TypeIsFunction(member_symbol->type) ||
      member_symbol->type->info.function.is_deleted) {
    return;
  }
  member_symbol->flags.is_defined = true;
  member_symbol->flags.is_inline_defn = true;
  if (!StorageIs(member_symbol->storage, STO(static))) {
    member_symbol->flags.is_weak = true;
  }
  member_symbol->value.func_defn = member_symbol;
  member_symbol->type->info.function.is_inline = true;
  member_symbol->type->info.function.definition = true;

  if (CXXFunctionIsThreeWayComparison(member_symbol->type) ||
      CXXFunctionIsEqualityComparison(member_symbol->type)) {
    // A defaulted comparison operator is implicitly constexpr when it satisfies
    // the requirements for a constexpr function ([class.compare.default]).  Mark
    // it so its synthesized body can participate in constant evaluation; if a
    // member subobject's comparison turns out not to be constant, the evaluator
    // simply fails the fold.
    member_symbol->type->info.function.is_constexpr = true;
    Vector* comparison_body = NewVector();
    if (CXXFunctionIsThreeWayComparison(member_symbol->type)) {
      AppendCXXThreeWayComparisons(parser, member_symbol, comparison_body,
                                   member_symbol->location);
    } else {
      AppendCXXEqualityComparison(member_symbol, comparison_body,
                                  member_symbol->location);
    }
    member_symbol->type->info.function.body =
        NewCompoundStatementASTNode(comparison_body, member_symbol->location);
    VectorAppend(&compiler->declaration_asts,
                 member_symbol->type->info.function.body);
    if (!parser->syntax->parsing_template_declaration) {
      QueueInlineMemberFunctionDefinition(member_symbol);
    }
    return;
  }

  Vector* body = NewVector();
  CXXConstructorInitList cxx_initializers;
  SyntaxCXXConstructorInitListInit(&cxx_initializers);
  SyntaxInsertCXXConstructorPreamble(parser->syntax, member_symbol->type, body,
                                     &cxx_initializers,
                                     member_symbol->location);
  if (CXXFunctionNeedsMemberwiseCopy(member_symbol->type)) {
    AppendCXXMemberwiseAssignments(
        member_symbol->type, body,
        member_symbol->type->info.function.cxx_member_owner,
        member_symbol->location);
    AppendCXXAssignmentReturnThis(member_symbol->type, body,
                                  member_symbol->location);
  }
  AppendCXXMemberDestructorCalls(member_symbol->type, body,
                                 member_symbol->location);
  AppendCXXBaseDestructorCalls(parser->syntax, member_symbol->type, body,
                               member_symbol->location);
  SyntaxCXXConstructorInitListDestruct(&cxx_initializers);

  member_symbol->type->info.function.body =
      NewCompoundStatementASTNode(body, member_symbol->location);
  VectorAppend(&compiler->declaration_asts,
               member_symbol->type->info.function.body);
  if (!parser->syntax->parsing_template_declaration) {
    QueueInlineMemberFunctionDefinition(member_symbol);
  }
}

static Symbol* CXXSourceObjectParameter(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.prototype.length < 2) {
    return NULL;
  }
  return func->info.function.prototype.value.p[func->info.function.prototype.length - 1];
}

static ASTNode* NewCXXSourceMemberReceiver(Symbol* source,
                                           StructMember* member,
                                           SourceLocation location) {
  if (source == NULL || member == NULL || member->symbol == NULL) {
    return NULL;
  }
  ASTNode* source_node = NewIdentifierASTNode(source, location);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(member->symbol->name.value), NULL,
                               location);
  return NewBinaryASTNode(AST_OP(dot), NULL, location, source_node, member_name);
}

static void AppendCXXMemberwiseAssignments(TypeRecord* func, Vector* body,
                                           Struct* owner,
                                           SourceLocation location) {
  if (func == NULL || body == NULL || owner == NULL) {
    return;
  }
  Symbol* source = CXXSourceObjectParameter(func);
  if (source == NULL) {
    return;
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function) {
      continue;
    }
    TypeRecord* member_type = member->symbol->type;
    if (TypeIsFixedArray(member_type)) {
      for (size_t index = 0; index < member_type->info.array.size.fixed;
           index++) {
        ASTNode* target = NewCXXMemberReceiver(func, member, location);
        ASTNode* value = NewCXXSourceMemberReceiver(source, member, location);
        ASTNode* index_node = NewIntConstantASTNode(
            (int64_t)index, NewTypeRecordWithSize(kTypeInt, kQualPlain),
            location);
        ASTNode* value_index = NewIntConstantASTNode(
            (int64_t)index, NewTypeRecordWithSize(kTypeInt, kQualPlain),
            location);
        target = NewBinaryASTNode(AST_OP(subscript), NULL, location, target,
                                  index_node);
        value = NewBinaryASTNode(AST_OP(subscript), NULL, location, value,
                                 value_index);
        ASTNode* assign = NewBinaryASTNode(AST_OP(assign), member_type->next,
                                           location, target, value);
        VectorAppend(body, NewExpressionStatementASTNode(assign, location));
      }
    } else {
      ASTNode* target = NewCXXMemberReceiver(func, member, location);
      ASTNode* value = NewCXXSourceMemberReceiver(source, member, location);
      ASTNode* assign = NewBinaryASTNode(AST_OP(assign), member_type, location,
                                         target, value);
      VectorAppend(body, NewExpressionStatementASTNode(assign, location));
    }
  }
}

static bool CXXFunctionIsAssignmentOperator(TypeRecord* func) {
  return func != NULL && TypeIsFunction(func) && func->info.function.symbol != NULL &&
         strcmp(func->info.function.symbol->name.value, "operator=") == 0;
}

static bool CXXFunctionNeedsMemberwiseCopy(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL ||
      func->info.function.is_destructor) {
    return false;
  }
  switch (func->info.function.cxx_special_member_kind) {
    case kCXXSpecialMemberCopyConstructor:
    case kCXXSpecialMemberMoveConstructor:
    case kCXXSpecialMemberCopyAssignment:
    case kCXXSpecialMemberMoveAssignment:
      return true;
    default:
      return false;
  }
}

static void AppendCXXAssignmentReturnThis(TypeRecord* func, Vector* body,
                                          SourceLocation location) {
  if (!CXXFunctionIsAssignmentOperator(func) || func->next == NULL ||
      TypeIsVoid(func->next) || func->info.function.prototype.length == 0) {
    return;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  ASTNode* this_node = NewIdentifierASTNode(this_symbol, location);
  TypeRecord* object_type =
      TypeIsPointer(this_symbol->type) ? this_symbol->type->next : NULL;
  ASTNode* object = NewUnaryASTNode(AST_OP(contents), object_type, location,
                                    this_node);
  VectorAppend(body, NewCombinedStatementASTNode(AST_OP(return), object, NULL,
                                                 location));
}

static bool CXXClassNameMatchesUnqualifiedTemplateName(String* class_name,
                                                       String* spelling) {
  if (StringEqualString(spelling, class_name)) {
    return true;
  }
  const char* template_args = strchr(class_name->value, '<');
  if (template_args == NULL) {
    return false;
  }
  size_t base_length = (size_t)(template_args - class_name->value);
  return spelling->length == base_length &&
         strncmp(spelling->value, class_name->value, base_length) == 0;
}

static bool ParseClassSpecialMember(TypeParser* parser, Struct* str,
                                    String* class_name, CXXAccess access,
                                    bool is_virtual, bool is_constexpr,
                                    bool is_consteval, bool is_explicit,
                                    bool is_member_template,
                                    Vector* member_template_parameters,
                                    int member_template_parameter_base) {
  if (!CompilerIsCXX() || class_name->length == 0) {
    return false;
  }

  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  bool is_destructor = LexMatch(parser->lex, TOK(tilde));
  if (!LexLookingAt(parser->lex, TOK(identifier)) ||
      !CXXClassNameMatchesUnqualifiedTemplateName(class_name,
                                                  &parser->lex->spelling)) {
    if (is_destructor) {
      SyntaxError(parser->syntax, "Expected class name after '~'");
      LexCheckpointDestruct(&checkpoint);
      return true;
    }
    LexCheckpointRestore(parser->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return false;
  }

  SourceLocation location = parser->lex->current_token_location;
  LexNextToken(parser->lex);
  if (!LexMatch(parser->lex, TOK(lparen))) {
    if (is_destructor) {
      SyntaxError(parser->syntax, "Expected '(' in destructor declaration");
      LexCheckpointDestruct(&checkpoint);
      return true;
    }
    LexCheckpointRestore(parser->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return false;
  }
  LexCheckpointDestruct(&checkpoint);

  TypeParser proto_parser;
  TypeParserInit(&proto_parser, parser->lex, parser->syntax, STO(auto),
                 kParsingPrototype);
  proto_parser.cxx_member_owner = str;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constexpr = is_constexpr;
  func->info.function.is_consteval = is_consteval;
  func->info.function.is_inline = true;
  func->info.function.is_constructor = !is_destructor;
  func->info.function.is_destructor = is_destructor;
  // Only a constructor may be declared explicit; a destructor never converts.
  func->info.function.is_explicit = is_explicit && !is_destructor;
  if (!is_destructor && parser->syntax->pending_explicit_condition != NULL) {
    func->info.function.explicit_condition =
        parser->syntax->pending_explicit_condition;
    parser->syntax->pending_explicit_condition = NULL;
  }
  if (is_virtual && !is_destructor) {
    SyntaxError(parser->syntax, "Constructors cannot be virtual");
  }
  func->info.function.is_virtual = is_virtual && is_destructor;
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  ParseFunctionPrototype(&proto_parser, func);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
  if (LexMatch(parser->lex, TOK(const))) {
    SyntaxError(parser->syntax, "Constructors and destructors cannot be const");
  }
  ParseCXXExceptionSpecifier(parser, func);
  ParseCXXVirtSpecifiers(parser, func);
  ParseCXXPureSpecifier(parser, func);
  TypeParserDestruct(&proto_parser);
  TypeRecordAddCXXThisParameter(func, str, location);

  String member_name;
  StringInit(&member_name, is_destructor ? "~" : "");
  StringAppendString(&member_name, class_name);
  Symbol* member_symbol = NewSymbol(member_name.value, func, STO(implicit));
  member_symbol->location = location;
  func->info.function.symbol = member_symbol;
  if (is_member_template) {
    if (is_destructor) {
      // A destructor is never a template ([class.dtor]); a template following
      // `~Class` is ill-formed.
      SyntaxError(parser->syntax, "A destructor cannot be a template");
    } else {
      // Record the constructor as a member function template so overload
      // resolution deduces its parameters at the call site, exactly like an
      // ordinary member function template (e.g. `emplace`).
      member_symbol->flags.is_template = true;
      func->info.function.template_parameter_count =
          (int)member_template_parameters->length;
      func->info.function.template_parameter_base =
          member_template_parameter_base;
      VectorDestructWithContents(
          &func->info.function.template_parameters,
          (VectorElementDestructor)TemplateParameterDelete,
          /*free_element=*/false);
      VectorInit(&func->info.function.template_parameters);
      for (size_t i = 0; i < member_template_parameters->length; i++) {
        VectorAppend(&func->info.function.template_parameters,
                     member_template_parameters->value.p[i]);
      }
      // Ownership of the parameter entries has moved into the function type;
      // clear the source so the caller's cleanup frees only the vector.
      member_template_parameters->length = 0;
    }
  }
  CXXFinalizeSpecialMemberMetadata(member_symbol, str, true);
  StructMember* member = NewStructMember(member_symbol);
  member->is_member_function = true;
  member->access = access;
  StructMember* existing = MapFindPointerKey(&str->symbol_table, &member_name);
  if (existing != NULL) {
    if (!CanOverloadStructMember(existing, member)) {
      String diagnostic_name;
      StringInit(&diagnostic_name, NULL);
      SymbolFunctionDiagnosticName(member_symbol, &diagnostic_name);
      SyntaxError(parser->syntax, "Duplicate class member %s",
                  diagnostic_name.value);
      StringDestruct(&diagnostic_name);
      StructMemberDelete(member);
      StringDestruct(&member_name);
      SkipInlineMemberFunctionBody(parser);
      return true;
    }
    if (FindStructMemberOverload(existing, member_symbol->type) != NULL) {
      String diagnostic_name;
      StringInit(&diagnostic_name, NULL);
      SymbolFunctionDiagnosticName(member_symbol, &diagnostic_name);
      SyntaxError(parser->syntax, "Duplicate class member %s",
                  diagnostic_name.value);
      StringDestruct(&diagnostic_name);
      StructMemberDelete(member);
      StringDestruct(&member_name);
      SkipInlineMemberFunctionBody(parser);
      return true;
    }
    AppendStructMemberOverload(parser, str, existing, member);
  } else {
    AddStructMember(parser, str, member);
  }
  StringDestruct(&member_name);
  if (func->info.function.is_defaulted) {
    SynthesizeDefaultedMemberFunctionBody(parser, member_symbol);
  } else if (!func->info.function.is_deleted) {
    ParseInlineMemberFunctionBody(parser, member_symbol);
  }
  return true;
}

static void ConversionOperatorName(TypeRecord* type, String* name) {
  String type_name;
  StringInit(&type_name, "");
  TypeRecordToString(type, &type_name);
  StringInit(name, "operator ");
  for (size_t i = 0; i < type_name.length; i++) {
    char ch = type_name.value[i];
    if (ch == '*') {
      StringAppend(name, " pointer");
    } else if (ch == '&') {
      if (i + 1 < type_name.length && type_name.value[i + 1] == '&') {
        StringAppend(name, " rvalue_reference");
        i++;
      } else {
        StringAppend(name, " reference");
      }
    } else {
      StringAppendChar(name, ch);
    }
  }
  while (name->length > 0 && name->value[name->length - 1] == ' ') {
    name->value[name->length - 1] = '\0';
    name->length--;
  }
  StringDestruct(&type_name);
}

static TypeRecord* ParseCXXConversionType(TypeParser* parser) {
  TypeParser return_parser;
  TypeParserInit(&return_parser, parser->lex, parser->syntax, STO(auto),
                 kParsingPrototype);
  TypeRecord* return_type = TypeParserParseType(&return_parser, true);
  TypeParserDestruct(&return_parser);
  if (return_type == NULL) {
    return_type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }

  Vector wrappers;
  VectorInit(&wrappers);
  while (!LexEof(parser->lex)) {
    if (LexMatch(parser->lex, TOK(star))) {
      VectorAppend(&wrappers, NewPointerTypeRecord(ParseQualifiers(parser)));
    } else if (LexLookingAt(parser->lex, TOK(amp)) ||
               LexLookingAt(parser->lex, TOK(ampamp))) {
      bool rvalue = LexMatch(parser->lex, TOK(ampamp));
      if (!rvalue) {
        LexMatch(parser->lex, TOK(amp));
      }
      VectorAppend(&wrappers, NewReferenceTypeRecord(kQualPlain, rvalue));
    } else {
      break;
    }
  }

  TypeRecord* result = return_type;
  for (size_t i = wrappers.length; i > 0; i--) {
    TypeRecord* wrapper = wrappers.value.p[i - 1];
    TypeRecordChain(wrapper, result);
    wrapper->type = result->type;
    result = wrapper;
  }
  VectorDestruct(&wrappers);
  TypeRecordCalculateSize(result);
  return result;
}

bool SyntaxParseMemberOperatorName(Syntax* syntax, String* name) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(operator))) {
    return false;
  }
  // Distinguish a conversion-function-id (`operator <type-id>`) from an
  // operator-function-id (`operator+`, `operator()`, `operator[]`, ...) by
  // peeking at the token following `operator`: only a conversion names a type.
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool is_conversion = SyntaxLookingAtType(syntax);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);

  if (!is_conversion) {
    return SyntaxParseOperatorFunctionName(syntax, name);
  }

  // `operator <type-id>`: reuse the declaration-time conversion-type parser and
  // name builder so the produced lookup key matches the registered member name.
  LexNextToken(syntax->lex);  // consume `operator`
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = ParseCXXConversionType(&parser);
  ConversionOperatorName(type, name);
  TypeParserDestruct(&parser);
  return true;
}

static Symbol* NewCXXConversionOperatorSymbol(TypeParser* parser,
                                              Struct* owner,
                                              TypeRecord* return_type,
                                              SourceLocation location,
                                              bool is_virtual) {
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constexpr = parser->is_constexpr;
  func->info.function.is_consteval = parser->is_consteval;
  func->info.function.is_virtual = is_virtual;
  TypeRecordChain(func, return_type);
  func->info.function.is_const_member = LexMatch(parser->lex, TOK(const));
  func->info.function.ref_qualifier = ParseCXXRefQualifier(parser);
  ParseCXXExceptionSpecifier(parser, func);
  TypeRecordAddCXXThisParameter(func, owner, location);

  String member_name;
  ConversionOperatorName(return_type, &member_name);
  Symbol* member_symbol = NewSymbol(member_name.value, func, STO(implicit));
  member_symbol->location = location;
  func->info.function.symbol = member_symbol;
  StringDestruct(&member_name);
  return member_symbol;
}

static bool ParseCXXConversionOperatorMember(TypeParser* parser, Struct* str,
                                             CXXAccess access,
                                             bool is_virtual,
                                             bool is_explicit) {
  if (!CompilerIsCXX() || !LexLookingAt(parser->lex, TOK(operator))) {
    return false;
  }

  SourceLocation location = parser->lex->current_token_location;
  LexNextToken(parser->lex);

  TypeRecord* return_type = ParseCXXConversionType(parser);
  if (!LexLookingAt(parser->lex, TOK(lparen))) {
    SyntaxError(parser->syntax,
                "Unsupported conversion operator target type");
    SyntaxRecover(parser->syntax, TC(semicolon) | TC(openbra) | TC(closebra));
    return true;
  }
  SyntaxNeedBracket(parser->syntax, TOK(lparen), TC(decl));
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(decl));

  String member_name;
  ConversionOperatorName(return_type, &member_name);
  Symbol* member_symbol =
      NewCXXConversionOperatorSymbol(parser, str, return_type, location,
                                     is_virtual);
  TypeRecord* func = member_symbol->type;
  func->info.function.is_explicit = is_explicit;
  func->info.function.is_explicit_conversion = is_explicit;
  if (parser->syntax->pending_explicit_condition != NULL) {
    func->info.function.explicit_condition =
        parser->syntax->pending_explicit_condition;
    parser->syntax->pending_explicit_condition = NULL;
  }
  StructMember* member = NewStructMember(member_symbol);
  member->is_member_function = true;
  member->access = access;

  StructMember* existing = MapFindPointerKey(&str->symbol_table, &member_name);
  if (existing != NULL) {
    if (!CanOverloadStructMember(existing, member)) {
      SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                  member_name.value);
      StructMemberDelete(member);
      SkipInlineMemberFunctionBody(parser);
      StringDestruct(&member_name);
      return true;
    }
    if (FindStructMemberOverload(existing, member_symbol->type) != NULL) {
      SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                  member_name.value);
      StructMemberDelete(member);
      SkipInlineMemberFunctionBody(parser);
      StringDestruct(&member_name);
      return true;
    }
    AppendStructMemberOverload(parser, str, existing, member);
  } else {
    AddStructMember(parser, str, member);
  }
  ParseCXXVirtSpecifiers(parser, func);
  ParseCXXPureSpecifier(parser, func);
  ParseInlineMemberFunctionBody(parser, member_symbol);
  StringDestruct(&member_name);
  return true;
}


// Copy an anoymous union into the destination struct.  Members of the union
// are inserted into the symbol table of the dest struct and each of the
// members of the anonymous type are assigned offsets in the dest struct.
// The members of the src struct/union are not inserted as members of the
// dest struct (just in the symbol table, not the members vector).
static void CopyAnonymousMembers(TypeParser* parser, Struct* dest, Struct* src ) {
  for (size_t i = 0; i < src->members.length; i++) {
    StructMember* member = src->members.value.p[i];
    Symbol* symbol = member->symbol;
    if (member->is_anon) {
      // Anonymous member, deal with recursively.
      CopyAnonymousMembers(parser, dest, symbol->type->info.struct_info);
      continue;
    }
    if (!CheckStructMember(dest, &symbol->name)) {
      SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                symbol->name.value);
    } else {
      // Insert a *copy* of the member into the destination's symbol table with
      // the byte offset adjusted to be relative to the destination struct.  We
      // must not mutate the original member's byte_offset: it is shared with the
      // anonymous aggregate's own members vector, where the offset must stay
      // relative to that aggregate (otherwise designated/positional static
      // initializers and member access through the aggregate compute the wrong
      // offset).
      if (!src->is_union) {
        AlignNextOffset(dest, symbol->type);
      }
      StructMember* dest_member = NewStructMember(symbol);
      dest_member->bit_offset = member->bit_offset;
      dest_member->bit_size = member->bit_size;
      dest_member->index = member->index;
      dest_member->is_anon = member->is_anon;
      dest_member->byte_offset = dest->next_offset;

      StructInsertMemberIntoTables(dest, dest_member);

      if (!src->is_union) {
        // Members of an anonymous struct are laid out sequentially regardless
        // of whether the enclosing aggregate is a union, so always advance.
        UpdateStructSize(dest, symbol->type, false);
      }
    }
  }
}

static ASTNode* ParseCXXStaticDataMemberInitializer(TypeParser* parser,
                                                    Symbol* symbol) {
  if (symbol == NULL) {
    return NULL;
  }
  if (LexMatch(parser->lex, TOK(equal))) {
    parser->syntax->init_storage = symbol->storage;
    return SyntaxParseInitializer(parser->syntax, symbol, symbol->storage);
  }
  if (LexMatch(parser->lex, TOK(lbrace))) {
    return SyntaxParseBracedInitializer(parser->syntax);
  }
  return NULL;
}

static bool CXXStaticDataMemberAllowsInClassInitializer(Symbol* symbol) {
  return symbol != NULL && TypeIsConst(symbol->type) &&
         (TypeIsIntegral(symbol->type) || TypeIsEnum(symbol->type));
}

static void AnalyzeCXXStaticDataMemberConstantInitializer(TypeParser* parser,
                                                          Symbol* symbol,
                                                          ASTNode* initializer) {
  ASTNode* decl = NewVariableDeclarationASTNode(
      symbol, initializer, symbol->location);
  SemanticAnalyzeVariableDefinition(parser->syntax,
                                    (VariableDeclarationASTNode*)decl);
  if (!symbol->flags.value_set) {
    SyntaxError(parser->syntax,
                "Static data member initializer must be a constant expression");
  }
  ASTNodeDelete(decl);
}

static void QueueCXXInlineStaticDataMemberDefinition(TypeParser* parser,
                                                     Struct* owner,
                                                     StructMember* member,
                                                     ASTNode* initializer,
                                                     bool is_inline_member) {
  if (!CompilerIsCXX() || parser == NULL || parser->syntax == NULL ||
      owner == NULL || member == NULL || member->symbol == NULL ||
      !member->is_static || member->is_member_function ||
      parser->syntax->parsing_template_declaration) {
    return;
  }

  Symbol* symbol = member->symbol;
  bool implicit_inline = symbol->flags.is_constexpr;
  if (!is_inline_member && !implicit_inline) {
    if (initializer != NULL) {
      if (CXXStaticDataMemberAllowsInClassInitializer(symbol)) {
        AnalyzeCXXStaticDataMemberConstantInitializer(parser, symbol,
                                                     initializer);
        return;
      }
      SyntaxError(parser->syntax,
                  "Static data member initializer requires inline");
    }
    return;
  }

  TypeRecordCalculateSize(symbol->type);
  symbol->flags.is_defined = true;
  if (!StorageIs(symbol->storage, STO(static))) {
    symbol->flags.is_weak = true;
    SymbolSetCXXDataAsmName(symbol, owner);
  }
  if (symbol->flags.is_constexpr && initializer == NULL) {
    SyntaxError(parser->syntax,
                "constexpr variable requires an initializer");
  }

  ASTNode* decl = NewVariableDeclarationASTNode(
      symbol, initializer, symbol->location);
  bool const_eval_candidate =
      symbol->flags.is_constexpr || symbol->flags.is_constinit ||
      (TypeIsConst(symbol->type) && !TypeIsStructOrUnion(symbol->type));
  // A static data member whose own type is the enclosing class cannot be
  // evaluated here: the class is still incomplete (we are mid-definition) and
  // its constructors' inline bodies have not yet been semantically analyzed, so
  // constant-evaluating the initializer would fail.  Defer such a member to a
  // separate queue that is evaluated and code generated only once the whole
  // class -- including its inline member-function bodies -- is complete (see
  // CompileDeferredCXXStaticMembers).  Its value is a class object, so it is
  // never needed as a constant within the class body itself.  Keeping it out of
  // inline_static_member_definitions also prevents it from being code generated
  // (as a still-unevaluated dynamic initializer) before that point.
  if (const_eval_candidate && TypeIsStructOrUnion(symbol->type) &&
      symbol->type->info.struct_info == owner) {
    VectorAppend(&compiler->cxx_deferred_static_member_definitions, decl);
    return;
  }
  VectorAppend(&parser->syntax->inline_static_member_definitions, decl);
  if (!const_eval_candidate) {
    return;
  }
  // The initializer is in the scope of `owner` and may use its private
  // members (e.g. a private constructor of a comparison category).
  Struct* saved_access = compiler->current_class_access_context;
  compiler->current_class_access_context = owner;
  SemanticAnalyzeVariableDefinition(parser->syntax,
                                    (VariableDeclarationASTNode*)decl);
  compiler->current_class_access_context = saved_access;
}


static void ParseStructMembers(TypeParser* parser, Struct* str, bool is_union,
                               String* tag_name) {
  CXXAccess current_access = str->is_class ? kAccessPrivate : kAccessPublic;
  while (!LexLookingAt(parser->lex, TOK(rbrace))) {
    if (CompilerIsCXX() && LexLookingAt(parser->lex, TOK(static_assert))) {
      ASTNode* node = SyntaxParseStaticAssert(parser->syntax);
      ASTNodeDelete(node);
      continue;
    } else if (LexLookingAt(parser->lex, TOK(public))) {
      current_access = kAccessPublic;
      LexNextToken(parser->lex);
      SyntaxNeedBracket(parser->syntax, TOK(colon), TC(decl));
      continue;
    } else if (LexLookingAt(parser->lex, TOK(private))) {
      current_access = kAccessPrivate;
      LexNextToken(parser->lex);
      SyntaxNeedBracket(parser->syntax, TOK(colon), TC(decl));
      continue;
    } else if (LexLookingAt(parser->lex, TOK(protected))) {
      current_access = kAccessProtected;
      LexNextToken(parser->lex);
      SyntaxNeedBracket(parser->syntax, TOK(colon), TC(decl));
      continue;
    } else if (CompilerIsCXX() && LexLookingAt(parser->lex, TOK(friend))) {
      // Friend declarations grant access but do not introduce a member; they
      // are parsed in the enclosing namespace scope by the syntax layer.
      SyntaxParseFriendDeclaration(parser->syntax, str);
      continue;
    }

    Vector member_attributes = {0};
    VectorInit(&member_attributes);
    while (SyntaxParseCXXAlignas(parser->syntax, &member_attributes) ||
           SyntaxParseCXXAttributes(parser->syntax, &member_attributes)) {
    }

    if (CompilerIsCXX() && LexMatch(parser->lex, TOK(using))) {
      ParseCXXMemberUsingDeclaration(parser, str, current_access,
                                     parser->lex->current_token_location);
      AttributeListDestruct(&member_attributes);
      if (!LexLookingAt(parser->lex, TOK(rbrace))) {
        SyntaxNeedSemicolon(parser->syntax, TC(type));
      }
      continue;
    }

    if (CompilerIsCXX() && LexMatch(parser->lex, TOK(typedef))) {
      ParseCXXMemberTypedef(parser, str, current_access,
                            parser->lex->current_token_location);
      AttributeListDestruct(&member_attributes);
      if (!LexLookingAt(parser->lex, TOK(rbrace))) {
        SyntaxNeedSemicolon(parser->syntax, TC(type));
      }
      continue;
    }

    bool is_member_template = false;
    bool old_parsing_template = parser->syntax->parsing_template_declaration;
    int old_template_parameter_count =
        parser->syntax->current_template_parameter_count;
    Vector* old_template_parameters = parser->syntax->current_template_parameters;
    Vector* member_template_parameters = NULL;
    int member_template_parameter_base = old_template_parameter_count;
    if (CompilerIsCXX() && LexMatch(parser->lex, TOK(template))) {
      SyntaxOpenScope(parser->syntax);
      member_template_parameters =
          SyntaxParseTemplateParameterListWithBase(
              parser->syntax, member_template_parameter_base);
      parser->syntax->parsing_template_declaration = true;
      parser->syntax->current_template_parameters = member_template_parameters;
      parser->syntax->current_template_parameter_count =
          member_template_parameter_base +
          (int)member_template_parameters->length;
      is_member_template = true;
    }

    bool is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
    bool is_constexpr_member = false;
    bool is_consteval_member = false;
    if (CompilerIsCXX() && LexMatch(parser->lex, TOK(consteval))) {
      is_consteval_member = true;
      is_constexpr_member = true;
    } else {
      is_constexpr_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(constexpr));
    }
    parser->is_inline = is_inline_member;
    parser->is_constexpr = is_constexpr_member;
    parser->is_consteval = is_consteval_member;
    bool saw_explicit_member = false;
    bool is_explicit_member =
        ParseCXXExplicitSpecifier(parser, &saw_explicit_member);
    if (!is_inline_member) {
      is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
      parser->is_inline = is_inline_member;
    }
    if (!is_constexpr_member && !is_consteval_member) {
      if (CompilerIsCXX() && LexMatch(parser->lex, TOK(consteval))) {
        is_consteval_member = true;
        is_constexpr_member = true;
      } else {
        is_constexpr_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(constexpr));
      }
      parser->is_constexpr = is_constexpr_member;
      parser->is_consteval = is_consteval_member;
    }
    bool is_virtual_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(virtual));
    if (!is_inline_member) {
      is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
      parser->is_inline = is_inline_member;
    }
    if (ParseClassSpecialMember(parser, str, tag_name, current_access,
                                is_virtual_member, is_constexpr_member,
                                is_consteval_member, is_explicit_member,
                                is_member_template, member_template_parameters,
                                member_template_parameter_base)) {
      AttributeListDestruct(&member_attributes);
      if (is_member_template) {
        // A constructor template consumes the parameter entries (moving them
        // into the function type) and leaves the vector empty; a destructor
        // template is diagnosed inside ParseClassSpecialMember with its
        // entries still present.  Either way the scope, parsing flags, and the
        // (now possibly empty) parameter vector are torn down here.
        SyntaxCloseScope(parser->syntax);
        VectorDestructWithContents(member_template_parameters,
                                   (VectorElementDestructor)TemplateParameterDelete,
                                   /*free_element=*/false);
        parser->syntax->parsing_template_declaration = old_parsing_template;
        parser->syntax->current_template_parameter_count =
            old_template_parameter_count;
        parser->syntax->current_template_parameters = old_template_parameters;
      }
      if (!LexLookingAt(parser->lex, TOK(rbrace))) {
        LexMatch(parser->lex, TOK(semicolon));
      }
      continue;
    }

    bool is_static_member = LexMatch(parser->lex, TOK(static));
    // 'mutable' is a storage-class specifier on a data member.  Accept it in
    // either order with respect to 'static' so the (ill-formed) combination is
    // still reported by the conflict check rather than as a parse error.
    bool is_mutable_member =
        CompilerIsCXX() && LexMatch(parser->lex, TOK(mutable));
    if (is_mutable_member && !is_static_member) {
      is_static_member = LexMatch(parser->lex, TOK(static));
    }
    if (!is_inline_member) {
      is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
      parser->is_inline = is_inline_member;
    }
    if (!is_constexpr_member && !is_consteval_member) {
      if (CompilerIsCXX() && LexMatch(parser->lex, TOK(consteval))) {
        is_consteval_member = true;
        is_constexpr_member = true;
      } else {
        is_constexpr_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(constexpr));
      }
      parser->is_constexpr = is_constexpr_member;
      parser->is_consteval = is_consteval_member;
    }
    if (is_virtual_member && is_static_member) {
      SyntaxError(parser->syntax, "static member functions cannot be virtual");
    }
    if (!is_static_member &&
        ParseCXXConversionOperatorMember(parser, str, current_access,
                                         is_virtual_member,
                                         is_explicit_member)) {
      AttributeListDestruct(&member_attributes);
      if (is_member_template) {
        SyntaxError(parser->syntax,
                    "Conversion operator templates are not supported yet");
        SyntaxCloseScope(parser->syntax);
        VectorDestructWithContents(
            member_template_parameters,
            (VectorElementDestructor)TemplateParameterDelete,
            /*free_element=*/false);
        parser->syntax->parsing_template_declaration = old_parsing_template;
        parser->syntax->current_template_parameter_count =
            old_template_parameter_count;
        parser->syntax->current_template_parameters = old_template_parameters;
      }
      if (!LexLookingAt(parser->lex, TOK(rbrace))) {
        LexMatch(parser->lex, TOK(semicolon));
      }
      continue;
    }
    if (saw_explicit_member) {
      SyntaxError(parser->syntax,
                  "explicit is only supported on conversion operators");
    }
    bool possible_anon = LexLookingAt(parser->lex, TOK(union)) ||
            LexLookingAt(parser->lex, TOK(struct)) ||
            LexLookingAt(parser->lex, TOK(enum));
    TypeRecord* member_type = TypeParserParseType(parser, true);
    bool member_decl_had_inline_body = false;
    while (!LexEof(parser->lex)) {
      bool has_inline_body = false;
      if (possible_anon && LexLookingAt(parser->lex, TOK(semicolon))) {
        if (TypeIsNamedCXXNestedType(member_type)) {
          AddCXXNestedTypeMember(parser, str, member_type, current_access);
          if (!TypeIsEnum(member_type)) {
            break;
          }
        }
        if (TypeIsEnum(member_type)) {
          AddCXXUnscopedEnumConstantMembers(parser, str, member_type,
                                            current_access);
          break;
        }
        TypeRecordCalculateSize(member_type);
        AlignNextOffset(str, member_type);
        int anon_base = str->next_offset;
        CopyAnonymousMembers(parser, str, member_type->info.struct_info);
        
        // Make a fake member symbol to represent the anonymous member.  This
        // is inserted into the members vector but not the symbol table.
        Symbol* member_symbol = NewSymbol(SyntaxFakeName(parser->syntax),
                                          member_type, STO(implicit));
        StructMember* member = NewStructMember(member_symbol);
        member->byte_offset = anon_base;
        member->access = current_access;
        VectorAppend(&str->members, member);

        // Account for the space occupied by the anonymous aggregate.  In a
        // struct we advance past it; in a union it overlays the other members
        // at the same base offset, so we only grow the union's size and leave
        // next_offset where it was.
        if (is_union) {
          if (anon_base + member_type->size > str->size) {
            str->size = anon_base + member_type->size;
          }
          str->next_offset = anon_base;
        } else {
          str->next_offset = anon_base + member_type->size;
          str->size = str->next_offset;
        }
        member->is_anon = true;
        // Syntax doesn't allow anonymous members to be in a comma-separated
        // list.
        break;
      }
      Symbol* member_symbol = TypeParserParseDeclarator(parser, member_type);
      if (member_symbol == NULL) {
        SyntaxError(parser->syntax, "Invalid type for struct member");
      } else {
        while (SyntaxParseCXXAlignas(parser->syntax, &member_attributes) ||
               SyntaxParseCXXAttributes(parser->syntax, &member_attributes)) {
        }
        VectorCopy(&member_symbol->attributes, &member_attributes);
        VectorClear(&member_attributes);
        SyntaxApplyDeclarationAttributes(member_symbol);
        StructMember* member = NewStructMember(member_symbol);
        member_symbol->flags.is_constexpr = is_constexpr_member &&
                                            !TypeIsFunction(member_symbol->type);
        if (member_symbol->flags.is_constexpr) {
          member_symbol->type->qualifiers |= kQualConst;
        }
        member->is_static = is_static_member;
        member->is_member_function = TypeIsFunction(member_symbol->type);
        if (member->is_member_function &&
            SymbolIsCXXAllocationFunction(member_symbol)) {
          member->is_static = true;
        }
        member->is_mutable = is_mutable_member;
        if (is_mutable_member) {
          if (member->is_static) {
            SyntaxError(parser->syntax,
                        "member cannot be declared both 'mutable' and 'static'");
            member->is_mutable = false;
          } else if (member->is_member_function) {
            SyntaxError(parser->syntax,
                        "'mutable' can only be applied to data members");
            member->is_mutable = false;
          } else if (TypeIsConst(member_symbol->type)) {
            SyntaxError(parser->syntax,
                        "'mutable' cannot be applied to a const member");
            member->is_mutable = false;
          } else if (TypeIsReference(member_symbol->type)) {
            SyntaxError(parser->syntax,
                        "'mutable' cannot be applied to a reference member");
            member->is_mutable = false;
          }
        }
        member->access = current_access;
        if (member->is_member_function) {
          member_symbol->type->info.function.is_constexpr =
              is_constexpr_member;
          member_symbol->type->info.function.is_consteval =
              is_consteval_member;
          if (is_constexpr_member || is_consteval_member) {
            member_symbol->type->info.function.is_inline = true;
          }
        }
        if (member->is_member_function && !member->is_static) {
          member_symbol->type->info.function.is_virtual = is_virtual_member;
          TypeRecordAddCXXThisParameter(member_symbol->type, str,
                                        member_symbol->location);
        } else if (member->is_member_function) {
          member_symbol->type->info.function.cxx_member_owner = str;
        }
        if (is_member_template) {
          if (member->is_member_function) {
            member_symbol->flags.is_template = true;
            member_symbol->type->info.function.template_parameter_count =
                (int)member_template_parameters->length;
            member_symbol->type->info.function.template_parameter_base =
                member_template_parameter_base;
            VectorDestructWithContents(
                &member_symbol->type->info.function.template_parameters,
                (VectorElementDestructor)TemplateParameterDelete,
                /*free_element=*/false);
            VectorInit(&member_symbol->type->info.function.template_parameters);
            for (size_t i = 0; i < member_template_parameters->length; i++) {
              VectorAppend(
                  &member_symbol->type->info.function.template_parameters,
                  member_template_parameters->value.p[i]);
            }
            member_template_parameters->length = 0;
          } else {
            SyntaxError(parser->syntax,
                        "Member templates must be function templates");
          }
        }
        if (member->is_member_function) {
          ParseCXXVirtSpecifiers(parser, member_symbol->type);
          ParseCXXPureSpecifier(parser, member_symbol->type);
          CXXFinalizeSpecialMemberMetadata(member_symbol, str, true);
          if (member->is_static &&
              (member_symbol->type->info.function.is_override ||
               member_symbol->type->info.function.is_final ||
               member_symbol->type->info.function.is_pure_virtual)) {
            SyntaxError(parser->syntax,
                        "static member functions cannot use virtual specifiers");
          }
        }
        StructMember* existing =
            MapFindPointerKey(&str->symbol_table, &member_symbol->name);
        if (existing != NULL) {
          if (!CanOverloadStructMember(existing, member)) {
            SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                        member_symbol->name.value);
            StructMemberDelete(member);
            member = NULL;
          } else if (FindStructMemberOverload(existing,
                                             member_symbol->type) != NULL) {
            SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                        member_symbol->name.value);
            StructMemberDelete(member);
            member = NULL;
          } else {
            AppendStructMemberOverload(parser, str, existing, member);
          }
        } else {
          // Add to symbol table.
          AddStructMember(parser, str, member);
        }

        // Make a static member function visible by unqualified name within the
        // class body so a sibling member (e.g. a static method calling another
        // static method) can reference it without the Class:: qualifier, as
        // C++ class-scope lookup requires.  Only the first overload needs to be
        // injected: overload resolution recovers the full set from the owning
        // struct via the function's cxx_member_owner.  Non-static members are
        // intentionally excluded so unqualified uses still route through the
        // implicit `this->` member access.
        if (member != NULL && member->is_static && member->is_member_function &&
            member->symbol != NULL) {
          Symbol* scope_fn = SymbolClone(member->symbol);
          scope_fn->overload_next = NULL;
          if (!SyntaxAddSymbol(parser->syntax, scope_fn)) {
            SymbolDelete(scope_fn);
          }
        }

        // Check for bitfield.
        if (member == NULL) {
          // Already diagnosed as a duplicate.
        } else if (LexMatch(parser->lex, TOK(colon))) {
          if (member->is_static || member->is_member_function) {
            SyntaxError(parser->syntax,
                        "Only non-static data members can be bitfields");
          }
          ParseBitField(parser, is_union, str, member_symbol, member);
          if (!member->is_static && !member->is_member_function) {
            member->default_initializer =
                SyntaxParseCXXDefaultMemberInitializer(parser->syntax);
          }
        } else if (member->is_static || member->is_member_function) {
          if (member->is_member_function &&
              member_symbol->type->info.function.is_defaulted) {
            SynthesizeDefaultedMemberFunctionBody(parser, member_symbol);
          } else if (member->is_member_function &&
                     member_symbol->type->info.function.is_deleted) {
            has_inline_body = false;
          } else if (member->is_member_function) {
            has_inline_body = member->is_member_function &&
                              ParseInlineMemberFunctionBody(parser,
                                                            member_symbol);
          } else {
            ASTNode* initializer =
                ParseCXXStaticDataMemberInitializer(parser, member_symbol);
            // A const integral/enum static data member with an in-class
            // initializer yields a constant usable by unqualified name in the
            // rest of the class body (array bounds, default arguments, etc.).
            // Inside a template the regular queue defers this, but such
            // initializers are typically non-dependent, so evaluate them now to
            // make the constant available; otherwise use the normal path.
            if (parser->syntax->parsing_template_declaration &&
                !is_inline_member && initializer != NULL &&
                CXXStaticDataMemberAllowsInClassInitializer(member_symbol)) {
              AnalyzeCXXStaticDataMemberConstantInitializer(parser, member_symbol,
                                                            initializer);
            } else {
              QueueCXXInlineStaticDataMemberDefinition(
                  parser, str, member, initializer, is_inline_member);
            }
            // Inject a value-carrying clone into the class scope so the
            // constant resolves by unqualified name.  The clone lives in
            // all_local_symbols and is freed at end of compilation, separate
            // from the struct's own member symbol.
            if (member_symbol->flags.value_set) {
              Symbol* scope_constant = SymbolClone(member_symbol);
              if (!SyntaxAddSymbol(parser->syntax, scope_constant)) {
                SymbolDelete(scope_constant);
              }
            }
          }
          member_decl_had_inline_body |= has_inline_body;
        } else {
          // Regular member, align the member to the appropriate boundary.
          AlignNextOffsetForSymbol(str, member_symbol);
          member->byte_offset = str->next_offset;
          member->index = str->members.length - 1;

          UpdateStructSize(str, member_symbol->type, is_union);
          member->default_initializer =
              SyntaxParseCXXDefaultMemberInitializer(parser->syntax);
        }
      }
      if (!LexMatch(parser->lex, TOK(comma))) {
        if (has_inline_body) {
          break;
        }
        break;
      }
    }
  
    if (!member_decl_had_inline_body && !LexLookingAt(parser->lex, TOK(rbrace))) {
      SyntaxNeedSemicolon(parser->syntax, TC(type));
    }
    AttributeListDestruct(&member_attributes);
    if (is_member_template) {
      SyntaxCloseScope(parser->syntax);
      VectorDestructWithContents(member_template_parameters,
                                 (VectorElementDestructor)TemplateParameterDelete,
                                 /*free_element=*/false);
      parser->syntax->parsing_template_declaration = old_parsing_template;
      parser->syntax->current_template_parameter_count =
          old_template_parameter_count;
      parser->syntax->current_template_parameters = old_template_parameters;
    }
  }
}

static bool CXXStructHasUserDeclaredConstructor(Struct* str) {
  if (str == NULL || str->tag_name == NULL) {
    return false;
  }
  StructMember* first = FindStructMember(str, str->tag_name);
  for (StructMember* member = first; member != NULL;
       member = member->overload_next) {
    TypeRecord* func = member->symbol != NULL ? member->symbol->type : NULL;
    if (member->is_member_function && func != NULL &&
        func->info.function.is_constructor &&
        func->info.function.is_user_declared) {
      return true;
    }
  }
  return false;
}

static bool CXXStructHasVirtualMemberFunction(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    TypeRecord* func = member != NULL && member->symbol != NULL
                           ? member->symbol->type
                           : NULL;
    if (member != NULL && member->is_member_function &&
        !member->is_using_declaration && func != NULL &&
        func->info.function.is_virtual) {
      return true;
    }
  }
  return false;
}

static bool CXXStructHasNonPublicDataMember(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    if (member->access != kAccessPublic) {
      return true;
    }
  }
  return false;
}

static void ComputeCXXAggregateStatus(Struct* str) {
  if (!CompilerIsCXX() || str == NULL) {
    return;
  }
  bool aggregate = !str->is_union &&
                   !CXXStructHasUserDeclaredConstructor(str) &&
                   !CXXStructHasVirtualMemberFunction(str) &&
                   // A class that inherits virtual functions still "has virtual
                   // functions" and so is not an aggregate; virtual_members
                   // already includes slots copied from polymorphic bases.
                   str->virtual_members.length == 0 &&
                   !CXXStructHasNonPublicDataMember(str);
  for (size_t i = 0; aggregate && i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->is_virtual || base->access != kAccessPublic) {
      aggregate = false;
    }
  }
  if (aggregate && str->virtual_bases.length > 0) {
    aggregate = false;
  }
  str->is_aggregate = aggregate;
}

static bool StructHasDeclaredCXXDestructor(Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->tag_name == NULL) {
    return false;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, str->tag_name);
  StructMember* destructor = FindStructMember(str, &destructor_name);
  StringDestruct(&destructor_name);
  return destructor != NULL && destructor->is_member_function &&
         destructor->symbol != NULL && destructor->symbol->type != NULL &&
         destructor->symbol->type->info.function.is_destructor;
}

static bool StructNeedsImplicitCXXDestructor(Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->is_union) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    if (CXXDestructibleElementType(member->symbol->type) != NULL) {
      return true;
    }
  }
  // A base (or virtual base) class with a non-trivial destructor also requires
  // this class to have a destructor, so the base subobject gets destroyed.
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base != NULL && base->type != NULL &&
        FindCXXDestructorForObjectType(base->type) != NULL) {
      return true;
    }
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
    if (base != NULL && base->type != NULL &&
        FindCXXDestructorForObjectType(base->type) != NULL) {
      return true;
    }
  }
  return false;
}

static void AddImplicitCXXDestructorIfNeeded(TypeParser* parser, Struct* str,
                                             Symbol* tag) {
  if (!CompilerIsCXX() || parser == NULL || str == NULL || tag == NULL ||
      str->tag_name == NULL || StructHasDeclaredCXXDestructor(str) ||
      !StructNeedsImplicitCXXDestructor(str)) {
    return;
  }

  SourceLocation location = tag->location;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_destructor = true;
  func->info.function.is_constexpr = true;
  func->info.function.is_inline = true;
  func->info.function.definition = true;
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  TypeRecordAddCXXThisParameter(func, str, location);

  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, str->tag_name);
  Symbol* symbol = NewSymbol(destructor_name.value, func, STO(implicit));
  StringDestruct(&destructor_name);
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->flags.is_inline_defn = true;
  if (!StorageIs(symbol->storage, STO(static))) {
    symbol->flags.is_weak = true;
  }
  symbol->location = location;
  symbol->value.func_defn = symbol;
  func->info.function.symbol = symbol;

  StructMember* member = NewStructMember(symbol);
  member->is_member_function = true;
  member->access = str->is_class ? kAccessPublic : kAccessPublic;
  AddStructMember(parser, str, member);

  Vector* body = NewVector();
  AppendCXXMemberDestructorCalls(func, body, location);
  AppendCXXBaseDestructorCalls(parser->syntax, func, body, location);
  func->info.function.body = NewCompoundStatementASTNode(body, location);
  VectorAppend(&compiler->declaration_asts, func->info.function.body);
  if (!parser->syntax->parsing_template_declaration) {
    QueueInlineMemberFunctionDefinition(symbol);
  }
}

static TypeRecord* NewCXXClassTypeForStruct(Struct* str, Qualifiers quals) {
  TypeRecord* type = NewTypeRecordWithSize(str->is_union ? kTypeUnion
                                                         : kTypeStruct,
                                           quals);
  type->info.struct_info = str;
  TypeRecordCalculateSize(type);
  return type;
}

static TypeRecord* NewCXXClassReferenceType(Struct* str, bool is_const,
                                            bool rvalue) {
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
  TypeRecordChain(ref, NewCXXClassTypeForStruct(str,
                                                is_const ? kQualConst
                                                         : kQualPlain));
  TypeRecordCalculateSize(ref);
  return ref;
}

static Symbol* NewCXXSyntheticFormal(const char* name, TypeRecord* type,
                                     SourceLocation location) {
  Symbol* formal = NewSymbol(name, type, STO(implicit));
  formal->flags.is_argument = true;
  formal->flags.invented = true;
  formal->flags.is_defined = true;
  formal->location = location;
  return formal;
}

static void AppendCXXSyntheticFormal(TypeRecord* func, Symbol* formal) {
  formal->value.arg_number = (int32_t)func->info.function.prototype.length;
  VectorAppend(&func->info.function.prototype, formal);
}

static bool CXXStructHasAnyConstructor(Struct* str) {
  if (str == NULL || str->tag_name == NULL) {
    return false;
  }
  StructMember* first = FindStructMember(str, str->tag_name);
  for (StructMember* member = first; member != NULL;
       member = member->overload_next) {
    if (member->is_member_function && member->symbol != NULL &&
        member->symbol->type != NULL &&
        member->symbol->type->info.function.is_constructor) {
      return true;
    }
  }
  return false;
}

static bool CXXStructHasSpecialMemberKind(Struct* str,
                                          CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    for (StructMember* overload = member; overload != NULL;
         overload = overload->overload_next) {
      TypeRecord* func = overload != NULL && overload->symbol != NULL
                             ? overload->symbol->type
                             : NULL;
      if (overload != NULL && overload->is_member_function && func != NULL &&
          func->info.function.cxx_special_member_kind == kind) {
        return true;
      }
    }
  }
  return false;
}

static bool CXXStructHasUserDeclaredSpecialMemberKind(
    Struct* str, CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    for (StructMember* overload = member; overload != NULL;
         overload = overload->overload_next) {
      TypeRecord* func = overload != NULL && overload->symbol != NULL
                             ? overload->symbol->type
                             : NULL;
      if (overload != NULL && overload->is_member_function && func != NULL &&
          func->info.function.cxx_special_member_kind == kind &&
          func->info.function.is_user_declared) {
        return true;
      }
    }
  }
  return false;
}

static bool CXXStructHasUserDeclaredDestructor(Struct* str) {
  return CXXStructHasUserDeclaredSpecialMemberKind(
      str, kCXXSpecialMemberDestructor);
}

static bool CXXStructHasUserDeclaredCopyOrMove(Struct* str) {
  return CXXStructHasUserDeclaredSpecialMemberKind(
             str, kCXXSpecialMemberCopyConstructor) ||
         CXXStructHasUserDeclaredSpecialMemberKind(
             str, kCXXSpecialMemberMoveConstructor) ||
         CXXStructHasUserDeclaredSpecialMemberKind(
             str, kCXXSpecialMemberCopyAssignment) ||
         CXXStructHasUserDeclaredSpecialMemberKind(
             str, kCXXSpecialMemberMoveAssignment);
}

static bool CXXStructHasUnassignableMember(Struct* str) {
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function) {
      continue;
    }
    TypeRecord* type = member->symbol->type;
    if (TypeIsReference(type) || TypeIsConst(type)) {
      return true;
    }
  }
  return false;
}

static bool CXXStructHasDeletedSpecialMemberKind(Struct* str,
                                                 CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    for (StructMember* overload = member; overload != NULL;
         overload = overload->overload_next) {
      TypeRecord* func = overload != NULL && overload->symbol != NULL
                             ? overload->symbol->type
                             : NULL;
      if (overload != NULL && overload->is_member_function && func != NULL &&
          func->info.function.cxx_special_member_kind == kind &&
          func->info.function.is_deleted) {
        return true;
      }
    }
  }
  return false;
}

static bool CXXStructHasDeletedBaseSpecialMemberKind(
    Struct* str, CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (CXXStructHasDeletedSpecialMemberKind(base_struct, kind) ||
        CXXStructHasDeletedBaseSpecialMemberKind(base_struct, kind)) {
      return true;
    }
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (CXXStructHasDeletedSpecialMemberKind(base_struct, kind) ||
        CXXStructHasDeletedBaseSpecialMemberKind(base_struct, kind)) {
      return true;
    }
  }
  return false;
}

static bool CXXTypeNeedsDefaultInitializer(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  if (TypeIsFixedArray(type)) {
    return CXXTypeNeedsDefaultInitializer(type->next);
  }
  return TypeIsReference(type) || TypeIsConst(type);
}

static bool CXXStructHasMemberWithoutDefaultInitialization(Struct* str) {
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        member->default_initializer != NULL) {
      continue;
    }
    if (CXXTypeNeedsDefaultInitializer(member->symbol->type)) {
      return true;
    }
  }
  return false;
}

static void AddCXXSyntheticMemberFunction(TypeParser* parser, Struct* str,
                                          Symbol* symbol) {
  StructMember* member = NewStructMember(symbol);
  member->is_member_function = true;
  member->access = kAccessPublic;
  StructMember* existing = MapFindPointerKey(&str->symbol_table,
                                             &symbol->name);
  if (existing != NULL) {
    AppendStructMemberOverload(parser, str, existing, member);
  } else {
    AddStructMember(parser, str, member);
  }
  CXXSpecialMemberKind kind = symbol->type->info.function.cxx_special_member_kind;
  bool template_instantiation =
      str->tag_symbol != NULL && str->tag_symbol->type != NULL &&
      str->tag_symbol->type->template_origin != NULL &&
      symbol->type->info.function.is_implicitly_declared;
  bool inherited_copy_or_assign =
      (str->bases.length > 0 || str->virtual_bases.length > 0) &&
      (kind == kCXXSpecialMemberCopyConstructor ||
       kind == kCXXSpecialMemberMoveConstructor ||
       kind == kCXXSpecialMemberCopyAssignment ||
       kind == kCXXSpecialMemberMoveAssignment);
  if (!inherited_copy_or_assign && !template_instantiation) {
    SynthesizeDefaultedMemberFunctionBody(parser, symbol);
  }
}

// True if the class has a member function named `name` (following overload
// chains).  When `defaulted_only`, only an explicitly defaulted one counts.
static bool CXXStructHasMemberFunctionNamed(Struct* str, const char* name,
                                            bool defaulted_only) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    for (StructMember* m = str->members.value.p[i]; m != NULL;
         m = m->overload_next) {
      if (!m->is_member_function || m->symbol == NULL ||
          m->symbol->type == NULL ||
          strcmp(m->symbol->name.value, name) != 0) {
        continue;
      }
      if (!defaulted_only || m->symbol->type->info.function.is_defaulted) {
        return true;
      }
    }
  }
  return false;
}

// [class.compare.default]: a class that explicitly defaults `operator<=>` and
// does not declare `operator==` gets an implicitly defaulted `operator==`.
static void AddImplicitCXXEqualityOperator(TypeParser* parser, Struct* str,
                                           Symbol* tag) {
  if (!CompilerIsCXX() || str == NULL || tag == NULL ||
      !CXXStructHasMemberFunctionNamed(str, "operator<=>", true) ||
      CXXStructHasMemberFunctionNamed(str, "operator==", false)) {
    return;
  }
  SourceLocation location = tag->location;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constexpr = true;
  func->info.function.is_inline = true;
  func->info.function.is_defaulted = true;
  func->info.function.is_implicitly_declared = true;
  func->info.function.is_const_member = true;
  func->info.function.is_constexpr_eligible = true;
  func->info.function.is_noexcept_eligible = true;
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeBool, kQualPlain));
  TypeRecordAddCXXThisParameter(func, str, location);
  AppendCXXSyntheticFormal(
      func, NewCXXSyntheticFormal(
                "__other", NewCXXClassReferenceType(str, true, false),
                location));

  Symbol* symbol = NewSymbol("operator==", func, STO(implicit));
  symbol->flags.invented = true;
  symbol->location = location;
  symbol->namespace_ = tag->namespace_;
  func->info.function.symbol = symbol;
  symbol->flags.is_defined = true;
  symbol->flags.is_inline_defn = true;
  if (!StorageIs(symbol->storage, STO(static))) {
    symbol->flags.is_weak = true;
  }
  symbol->value.func_defn = symbol;
  AddCXXSyntheticMemberFunction(parser, str, symbol);
}

static Symbol* NewCXXSyntheticSpecialMember(TypeParser* parser, Struct* str,
                                            Symbol* tag,
                                            const char* name,
                                            TypeRecord* return_type,
                                            CXXSpecialMemberKind kind,
                                            bool is_constructor,
                                            bool is_destructor,
                                            bool source_is_const,
                                            bool source_is_rvalue,
                                            bool deleted) {
  SourceLocation location = tag->location;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constructor = is_constructor;
  func->info.function.is_destructor = is_destructor;
  func->info.function.is_constexpr = true;
  func->info.function.is_inline = true;
  func->info.function.is_defaulted = true;
  func->info.function.is_explicitly_defaulted = false;
  func->info.function.is_implicitly_declared = true;
  func->info.function.is_deleted = deleted;
  func->info.function.is_implicitly_deleted = deleted;
  func->info.function.cxx_special_member_kind = kind;
  func->info.function.is_constexpr_eligible = true;
  func->info.function.is_noexcept_eligible = true;
  func->info.function.is_trivial_special_member =
      !deleted &&
      (kind != kCXXSpecialMemberDestructor ||
       (!StructNeedsImplicitCXXDestructor(str) && str->bases.length == 0 &&
        str->virtual_bases.length == 0));
  TypeRecordChain(func, return_type);
  TypeRecordAddCXXThisParameter(func, str, location);
  if (!is_destructor && (source_is_const || source_is_rvalue)) {
    AppendCXXSyntheticFormal(
        func,
        NewCXXSyntheticFormal("__other",
                              NewCXXClassReferenceType(str, source_is_const,
                                                       source_is_rvalue),
                              location));
  }

  Symbol* symbol = NewSymbol(name, func, STO(implicit));
  symbol->flags.invented = true;
  symbol->location = location;
  symbol->namespace_ = tag->namespace_;
  func->info.function.symbol = symbol;
  if (!deleted) {
    symbol->flags.is_defined = true;
    symbol->flags.is_inline_defn = true;
    if (!StorageIs(symbol->storage, STO(static))) {
      symbol->flags.is_weak = true;
    }
    symbol->value.func_defn = symbol;
  }
  (void)parser;
  return symbol;
}

static void AddImplicitCXXSpecialMembers(TypeParser* parser, Struct* str,
                                         Symbol* tag) {
  if (!CompilerIsCXX() || parser == NULL || str == NULL || tag == NULL ||
      str->tag_name == NULL || str->cxx_special_members_complete ||
      tag->flags.invented ||
      strcmp(tag->name.value, "__va_list_tag") == 0 ||
      parser->syntax->parsing_template_declaration ||
      (tag->type != NULL && tag->type->template_origin != NULL)) {
    return;
  }
  bool user_declared_move =
      CXXStructHasUserDeclaredSpecialMemberKind(
          str, kCXXSpecialMemberMoveConstructor) ||
      CXXStructHasUserDeclaredSpecialMemberKind(
          str, kCXXSpecialMemberMoveAssignment);
  bool user_declared_copy_or_move = CXXStructHasUserDeclaredCopyOrMove(str);
  bool user_declared_destructor = CXXStructHasUserDeclaredDestructor(str);
  AddImplicitCXXEqualityOperator(parser, str, tag);
  if (str->is_aggregate) {
    bool deleted_assignment =
        CXXStructHasUnassignableMember(str) ||
        CXXStructHasDeletedBaseSpecialMemberKind(
            str, kCXXSpecialMemberCopyAssignment) ||
        CXXStructHasDeletedBaseSpecialMemberKind(
            str, kCXXSpecialMemberMoveAssignment);
    if (deleted_assignment &&
        !CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberCopyAssignment)) {
      TypeRecord* assign_return = NewCXXClassReferenceType(str, false, false);
      Symbol* copy_assign = NewCXXSyntheticSpecialMember(
          parser, str, tag, "operator=", assign_return,
          kCXXSpecialMemberCopyAssignment, false, false, true, false, true);
      AddCXXSyntheticMemberFunction(parser, str, copy_assign);
      TypeRecord* move_return = NewCXXClassReferenceType(str, false, false);
      Symbol* move_assign = NewCXXSyntheticSpecialMember(
          parser, str, tag, "operator=", move_return,
          kCXXSpecialMemberMoveAssignment, false, false, false, true, true);
      AddCXXSyntheticMemberFunction(parser, str, move_assign);
    }
    str->cxx_special_members_complete = true;
    return;
  }

  if (!CXXStructHasAnyConstructor(str)) {
    Symbol* ctor = NewCXXSyntheticSpecialMember(
        parser, str, tag, str->tag_name->value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberDefaultConstructor, true, false, false, false,
        CXXStructHasMemberWithoutDefaultInitialization(str) ||
            CXXStructHasDeletedBaseSpecialMemberKind(
                str, kCXXSpecialMemberDefaultConstructor));
    AddCXXSyntheticMemberFunction(parser, str, ctor);
  }

  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberDestructor)) {
    String destructor_name;
    StringInit(&destructor_name, "~");
    StringAppendString(&destructor_name, str->tag_name);
    Symbol* dtor = NewCXXSyntheticSpecialMember(
        parser, str, tag, destructor_name.value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberDestructor, false, true, false, false,
        CXXStructHasDeletedBaseSpecialMemberKind(
            str, kCXXSpecialMemberDestructor));
    StringDestruct(&destructor_name);
    AddCXXSyntheticMemberFunction(parser, str, dtor);
  }

  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberCopyConstructor)) {
    Symbol* copy = NewCXXSyntheticSpecialMember(
        parser, str, tag, str->tag_name->value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberCopyConstructor, true, false, true, false,
        user_declared_move ||
            CXXStructHasDeletedBaseSpecialMemberKind(
                str, kCXXSpecialMemberCopyConstructor));
    AddCXXSyntheticMemberFunction(parser, str, copy);
  }

  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberCopyAssignment)) {
    TypeRecord* assign_return = NewCXXClassReferenceType(str, false, false);
    Symbol* copy_assign = NewCXXSyntheticSpecialMember(
        parser, str, tag, "operator=", assign_return,
        kCXXSpecialMemberCopyAssignment, false, false, true, false,
        user_declared_move || CXXStructHasUnassignableMember(str) ||
            CXXStructHasDeletedBaseSpecialMemberKind(
                str, kCXXSpecialMemberCopyAssignment));
    AddCXXSyntheticMemberFunction(parser, str, copy_assign);
  }

  if (!user_declared_copy_or_move && !user_declared_destructor &&
      !CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberMoveConstructor)) {
    Symbol* move = NewCXXSyntheticSpecialMember(
        parser, str, tag, str->tag_name->value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberMoveConstructor, true, false, false, true,
        CXXStructHasDeletedBaseSpecialMemberKind(
            str, kCXXSpecialMemberMoveConstructor));
    AddCXXSyntheticMemberFunction(parser, str, move);
  }

  if (!user_declared_copy_or_move && !user_declared_destructor &&
      !CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberMoveAssignment)) {
    TypeRecord* move_return = NewCXXClassReferenceType(str, false, false);
    Symbol* move_assign = NewCXXSyntheticSpecialMember(
        parser, str, tag, "operator=", move_return,
        kCXXSpecialMemberMoveAssignment, false, false, false, true,
        CXXStructHasUnassignableMember(str) ||
            CXXStructHasDeletedBaseSpecialMemberKind(
                str, kCXXSpecialMemberMoveAssignment));
    AddCXXSyntheticMemberFunction(parser, str, move_assign);
  }
  str->cxx_special_members_complete = true;
}

static TemplateArgument* NewTemplateParameterPatternArgument(
    TemplateParameter* param) {
  if (param == NULL) {
    return NULL;
  }
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = param->kind;
  arg->is_pack_expansion = param->is_parameter_pack;
  arg->type = NULL;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  if (param->kind == kTemplateParameterType) {
    arg->type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    arg->type->template_parameter_index = param->index;
  } else {
    arg->template_parameter_index = param->index;
  }
  return arg;
}

static TypeRecord* NewCXXDeductionGuideReturnType(Symbol* tag) {
  if (tag == NULL || tag->type == NULL ||
      !TypeIsStructOrUnion(tag->type) ||
      tag->type->info.struct_info == NULL) {
    return NULL;
  }
  TypeRecord* type = TypeRecordCopy(tag->type);
  type->template_origin = tag;
  type->template_arguments = NewVector();
  Struct* str = tag->type->info.struct_info;
  for (size_t i = 0; i < str->template_parameters.length; i++) {
    TemplateArgument* arg = NewTemplateParameterPatternArgument(
        str->template_parameters.value.p[i]);
    if (arg != NULL) {
      VectorAppend(type->template_arguments, arg);
    }
  }
  return type;
}

static void CopyClassTemplateParametersToGuide(Struct* str, TypeRecord* func) {
  if (str == NULL || func == NULL || !TypeIsFunction(func)) {
    return;
  }
  VectorDestructWithContents(&func->info.function.template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&func->info.function.template_parameters);
  for (size_t i = 0; i < str->template_parameters.length; i++) {
    VectorAppend(&func->info.function.template_parameters,
                 TemplateParameterCopy(str->template_parameters.value.p[i]));
  }
  func->info.function.template_parameter_count =
      (int)str->template_parameters.length;
  func->info.function.template_parameter_base = 0;
}

static size_t CXXConstructorImplicitParameterCount(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL) {
    return 0;
  }
  size_t count = 1;
  if (func->info.function.is_constructor &&
      StructHasVirtualBases(func->info.function.cxx_member_owner)) {
    count++;
  }
  return count;
}

static Symbol* NewCXXDeductionGuideSymbol(Struct* str, Symbol* tag,
                                          TypeRecord* pattern,
                                          size_t first_formal) {
  TypeRecord* guide_type = NewFunctionTypeRecord();
  guide_type->info.function.is_deduction_guide = true;
  guide_type->info.function.is_implicitly_declared = true;
  CopyClassTemplateParametersToGuide(str, guide_type);
  TypeRecord* return_type = NewCXXDeductionGuideReturnType(tag);
  if (return_type == NULL) {
    TypeRecordDelete(guide_type);
    return NULL;
  }
  TypeRecordChain(guide_type, return_type);
  if (pattern != NULL && TypeIsFunction(pattern)) {
    for (size_t i = first_formal; i < pattern->info.function.prototype.length; i++) {
      Symbol* formal = pattern->info.function.prototype.value.p[i];
      if (formal == NULL || formal->type == NULL) {
        continue;
      }
      Symbol* clone = NewSymbol(formal->name.value, formal->type, formal->storage);
      clone->flags = formal->flags;
      clone->flags.is_argument = true;
      clone->location = formal->location;
      clone->default_argument =
          ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
      clone->value.arg_number = (int32_t)guide_type->info.function.prototype.length;
      VectorAppend(&guide_type->info.function.prototype, clone);
    }
  }
  Symbol* guide = NewSymbol(tag->name.value, guide_type, STO(implicit));
  guide->flags.is_template = guide_type->info.function.template_parameter_count > 0;
  guide->location = tag->location;
  guide_type->info.function.symbol = guide;
  return guide;
}

static void AddImplicitCXXConstructorDeductionGuide(Struct* str, Symbol* tag,
                                                   StructMember* member) {
  if (member == NULL || !member->is_member_function ||
      member->symbol == NULL || member->symbol->type == NULL ||
      !member->symbol->type->info.function.is_constructor ||
      member->symbol->flags.is_template) {
    return;
  }
  TypeRecord* func = member->symbol->type;
  Symbol* guide = NewCXXDeductionGuideSymbol(
      str, tag, func, CXXConstructorImplicitParameterCount(func));
  TypeAddCXXDeductionGuide(tag, guide);
}

static bool CXXAggregateDeductionMember(StructMember* member) {
  return member != NULL && member->symbol != NULL && !member->is_static &&
         !member->is_member_function && !member->is_using_declaration &&
         !StructMemberIsNestedType(member);
}

typedef struct CXXAggregateDeductionElement {
  const char* name;
  TypeRecord* type;
  SourceLocation location;
  bool has_default;
} CXXAggregateDeductionElement;

static CXXAggregateDeductionElement* NewCXXAggregateDeductionElement(
    const char* name, TypeRecord* type, SourceLocation location,
    bool has_default) {
  CXXAggregateDeductionElement* element = malloc(sizeof(*element));
  element->name = name;
  element->type = type;
  element->location = location;
  element->has_default = has_default;
  return element;
}

static bool CollectCXXAggregateDeductionElements(Struct* str,
                                                 SourceLocation location,
                                                 Vector* elements) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->is_virtual || base->access != kAccessPublic ||
        base->type == NULL) {
      continue;
    }
    if (TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL &&
        base->type->info.struct_info->is_aggregate &&
        CollectCXXAggregateDeductionElements(base->type->info.struct_info,
                                             location, elements)) {
      continue;
    } else {
      VectorAppend(elements,
                   NewCXXAggregateDeductionElement(
                       str->tag_name != NULL ? str->tag_name->value
                                             : "__ctad_base",
                       base->type, location, false));
    }
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (!CXXAggregateDeductionMember(member)) {
      continue;
    }
    VectorAppend(elements,
                 NewCXXAggregateDeductionElement(
                     member->symbol->name.value, member->symbol->type,
                     member->symbol->location,
                     member->default_initializer != NULL));
  }
  return true;
}

static void AddImplicitCXXAggregateDeductionGuideForPrefix(Struct* str,
                                                           Symbol* tag,
                                                           Vector* elements,
                                                           size_t prefix_count) {
  TypeRecord* guide_type = NewFunctionTypeRecord();
  guide_type->info.function.is_deduction_guide = true;
  guide_type->info.function.is_implicitly_declared = true;
  CopyClassTemplateParametersToGuide(str, guide_type);
  TypeRecord* return_type = NewCXXDeductionGuideReturnType(tag);
  if (return_type == NULL) {
    TypeRecordDelete(guide_type);
    return;
  }
  TypeRecordChain(guide_type, return_type);
  for (size_t i = 0; i < prefix_count; i++) {
    CXXAggregateDeductionElement* element = elements->value.p[i];
    Symbol* formal =
        NewSymbol(element->name, element->type, STO(auto));
    formal->flags.is_argument = true;
    formal->location = element->location;
    formal->value.arg_number = (int32_t)guide_type->info.function.prototype.length;
    VectorAppend(&guide_type->info.function.prototype, formal);
  }
  Symbol* guide = NewSymbol(tag->name.value, guide_type, STO(implicit));
  guide->flags.is_template = guide_type->info.function.template_parameter_count > 0;
  guide->location = tag->location;
  guide_type->info.function.symbol = guide;
  TypeAddCXXDeductionGuide(tag, guide);
}

static void AddImplicitCXXAggregateDeductionGuide(Struct* str, Symbol* tag) {
  if (!str->is_aggregate) {
    return;
  }
  Vector elements;
  VectorInit(&elements);
  CollectCXXAggregateDeductionElements(str, tag->location, &elements);
  size_t total = 0;
  size_t required = 0;
  for (size_t i = 0; i < elements.length; i++) {
    CXXAggregateDeductionElement* element = elements.value.p[i];
    total++;
    if (!element->has_default) {
      required = total;
    }
  }
  for (size_t prefix = required; prefix <= total; prefix++) {
    AddImplicitCXXAggregateDeductionGuideForPrefix(str, tag, &elements, prefix);
  }
  VectorDestructWithContents(&elements, NULL, /*free_element=*/true);
}

static void AddImplicitCXXCopyDeductionGuide(Struct* str, Symbol* tag) {
  TypeRecord* guide_type = NewFunctionTypeRecord();
  guide_type->info.function.is_deduction_guide = true;
  guide_type->info.function.is_implicitly_declared = true;
  CopyClassTemplateParametersToGuide(str, guide_type);
  TypeRecord* return_type = NewCXXDeductionGuideReturnType(tag);
  TypeRecord* source_type = NewCXXDeductionGuideReturnType(tag);
  if (return_type == NULL || source_type == NULL) {
    if (return_type != NULL) {
      TypeRecordDelete(return_type);
    }
    if (source_type != NULL) {
      TypeRecordDelete(source_type);
    }
    TypeRecordDelete(guide_type);
    return;
  }
  TypeRecordChain(guide_type, return_type);
  Symbol* formal = NewSymbol("__ctad_source", source_type, STO(auto));
  formal->flags.is_argument = true;
  formal->location = tag->location;
  formal->value.arg_number = 0;
  VectorAppend(&guide_type->info.function.prototype, formal);
  Symbol* guide = NewSymbol(tag->name.value, guide_type, STO(implicit));
  guide->flags.is_template = guide_type->info.function.template_parameter_count > 0;
  guide->location = tag->location;
  guide_type->info.function.symbol = guide;
  TypeAddCXXDeductionGuide(tag, guide);
}

static void AddImplicitCXXDeductionGuides(Struct* str, Symbol* tag) {
  if (!CompilerIsCXX() || str == NULL || tag == NULL || !str->is_template ||
      str->template_parameters.length == 0 || str->deduction_guides.length > 0) {
    return;
  }
  if (str->tag_name != NULL) {
    StructMember* first = FindStructMember(str, str->tag_name);
    for (StructMember* member = first; member != NULL;
         member = member->overload_next) {
      AddImplicitCXXConstructorDeductionGuide(str, tag, member);
    }
  }
  AddImplicitCXXCopyDeductionGuide(str, tag);
  AddImplicitCXXAggregateDeductionGuide(str, tag);
}

void TypeEnsureCXXDeductionGuides(Symbol* class_template) {
  if (class_template == NULL || class_template->type == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL) {
    return;
  }
  AddImplicitCXXDeductionGuides(class_template->type->info.struct_info,
                                class_template);
}

static void CheckFlexibleArrays(TypeParser* parser, Struct* str, bool is_union) {
  // Check the constraints for flexible arrays.
  // 1. Flexible array cannot be the only member
  // 2. Flexible array must be at the end of the struct.
  // 3. No flexible arrays in unions.
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (TypeIsArray(member->symbol->type)) {
      if (member->symbol->type->info.array.is_flexible) {
        if (is_union) {
          SyntaxError(parser->syntax, "No flexible arrays allowed in unions");
          break;
        }
        if (str->members.length == 1) {
          SyntaxError(parser->syntax,
                      "Flexible array '%s' cannot be the only "
                      "member in a struct",
                      member->symbol->name.value);
          break;
        }
        if (i != str->members.length - 1) {
          SyntaxError(parser->syntax,
                      "Flexible array '%s' needs to be "
                      "the last member in a struct",
                      member->symbol->name.value);
          break;
        }
      }
    }
  }
}

static void CheckTagType(TypeParser* parser, Symbol* old,
                         bool is_union, bool is_enum) {
  bool error = false;
  if (TypeIsEnum(old->type)) {
    error = !is_enum;
  } else {
    // Tag is a struct or union.
    Struct* str = old->type->info.struct_info;
    error = str->is_union != is_union;
  }
  if (error) {
    SyntaxError(parser->syntax,
              "Tag %s declared with different tag type",
              old->name.value);
  }
}

static void AddInjectedClassName(TypeParser* parser, Symbol* tag) {
  if (tag == NULL || tag->flags.invented || tag->type == NULL ||
      tag->type->info.struct_info == NULL) {
    return;
  }
  Struct* str = tag->type->info.struct_info;
  if (!str->is_class && (!CompilerIsCXX() || str->is_union)) {
    return;
  }
  if (SyntaxFindSymbol(parser->syntax, &tag->name) != NULL) {
    return;
  }

  Symbol* alias = NewSymbol(tag->name.value, tag->type, STO(typedef));
  alias->namespace_ = tag->namespace_;
  if (!SyntaxAddSymbol(parser->syntax, alias)) {
    SymbolDelete(alias);
  }
}

static void AddInjectedEnumName(TypeParser* parser, Symbol* tag) {
  if (!CompilerIsCXX() || tag == NULL || tag->flags.invented ||
      tag->type == NULL || tag->type->info.enum_info == NULL) {
    return;
  }
  if (SyntaxFindSymbol(parser->syntax, &tag->name) != NULL) {
    return;
  }

  Symbol* alias = NewSymbol(tag->name.value, tag->type, STO(typedef));
  alias->namespace_ = tag->namespace_;
  if (!SyntaxAddSymbol(parser->syntax, alias)) {
    SymbolDelete(alias);
  }
}

static Symbol* ParseStructBody(TypeParser* parser, String* tag_name,
                               bool is_union, bool is_class,
                               Vector* attributes, Vector* bases) {
  // We have a struct body.
  // First check that this is not a duplicate definition.
  Struct* str = NULL;
  bool empty_tag_name = tag_name->length == 0;
  if (empty_tag_name) {
    SyntaxFakeTagName(parser->syntax, tag_name);
  }
  Symbol* tag = SyntaxFindTopScopeTag(parser->syntax, tag_name);
  if (tag != NULL) {
    if (!tag->flags.is_forward_declared) {
      SyntaxError(parser->syntax, "Duplicate definition of struct/union %s",
                   tag_name->value);
    } else {
      CheckTagType(parser, tag, is_union, false);
    }
    str = tag->type->info.struct_info;
    if (str != NULL && str->tag_symbol == NULL) {
      str->tag_symbol = tag;
    }
    // The definition's class-key determines default member access, even when a
    // prior forward declaration (e.g. 'friend class X;' or 'class X;') used a
    // different key than the defining 'struct'/'class'.
    if (str != NULL) {
      str->is_class = is_class;
    }
  } else {
    // Tag doesn't exist in the, create one.
    str = NewStruct(is_union);
    str->is_class = is_class;
    TypeRecord* type = NewTypeRecord(is_union ? kTypeUnion : kTypeStruct,
                                      kQualPlain);
    type->info.struct_info = str;
    tag = NewSymbol(tag_name->value, type, STO(implicit));
    str->tag_name = &tag->name;
    str->tag_symbol = tag;
    if (empty_tag_name) {
      tag->flags.invented = true;
    }
    SyntaxAddTag(parser->syntax, tag);
    AddInjectedEnumName(parser, tag);
  }

  // Note in the symbol that this tag is now defined and not
  // forward declared.
  tag->flags.is_forward_declared = false;
  tag->flags.is_defined = true;

  // Now 'tag' will be the struct tag pointer
  // and 'str' will be a pointer to the Struct information.
  // Capture the #pragma pack(n) value in effect at the point of definition so
  // member offsets reflect it (and any later push/pop does not retroactively
  // change this struct).
  str->pack = compiler->pack_alignment;
  // Apply layout attributes (packed, aligned) before laying out members so the
  // member offsets reflect them in a single pass.
  StructApplyLayoutAttributes(str, attributes);
  for (size_t i = 0; i < bases->length; i++) {
    VectorAppend(&str->bases, bases->value.p[i]);
  }
  bases->length = 0;
  CollectCXXVirtualBases(str);
  CopyCXXBaseVirtualMembers(str);
  LayoutCXXBaseSpecifiers(str);

  LocalSymbolTable* class_tag_scope = NULL;
  LocalSymbolTable* class_symbol_scope = NULL;
  if (CompilerIsCXX()) {
    class_tag_scope = NewLocalSymbolTable();
    class_tag_scope->prev = parser->syntax->local_tag_stack;
    parser->syntax->local_tag_stack = class_tag_scope;
    // Open a symbol scope for the class body so that member type aliases
    // (`using`/`typedef`) become visible as type-names to subsequent member
    // declarations and inline member function bodies, as required by C++ class
    // scope rules.
    class_symbol_scope = NewLocalSymbolTable();
    class_symbol_scope->prev = parser->syntax->local_symbol_stack;
    parser->syntax->local_symbol_stack = class_symbol_scope;
  }
  Struct* saved_member_owner = parser->cxx_member_owner;
  parser->cxx_member_owner = str;
  ParseStructMembers(parser, str, is_union, tag_name);
  parser->cxx_member_owner = saved_member_owner;
  if (class_symbol_scope != NULL) {
    assert(parser->syntax->local_symbol_stack == class_symbol_scope);
    parser->syntax->local_symbol_stack = class_symbol_scope->prev;
    LocalSymbolTableDelete(class_symbol_scope);
  }
  ComputeCXXAggregateStatus(str);
  AddImplicitCXXSpecialMembers(parser, str, tag);
  AddImplicitCXXDestructorIfNeeded(parser, str, tag);
  AddImplicitCXXDeductionGuides(str, tag);
  if (class_tag_scope != NULL) {
    assert(parser->syntax->local_tag_stack == class_tag_scope);
    parser->syntax->local_tag_stack = class_tag_scope->prev;
    LocalSymbolTableDelete(class_tag_scope);
  }
  UpdateCXXAbstractStatus(str);
  AddCXXVPtrMember(parser, str);
  AddCXXVBPtrMember(parser, str);
  str->non_virtual_size = str->size;
  LayoutCXXVirtualBaseSpecifiers(str);
  RegisterCXXVTable(parser, str);
  RegisterCXXVBTables(parser, str);
  // The class's vtables now exist, so any constructor preambles built earlier
  // during member parsing can have their deferred __vptr initializers emitted.
  str->vtables_registered = true;
  SyntaxFlushPendingVPtrInitializers(str);
  
  // Round the size of the struct up to its own alignment (the maximum
  // alignment of its members, or an explicit aligned(N)), as required by the
  // ABI.
  FinalizeStructAlignment(str);
  SyntaxNeedBracket(parser->syntax, TOK(rbrace), TC(exprsep));
   
  CheckFlexibleArrays(parser, str, is_union);
  AddInjectedClassName(parser, tag);
  return tag;
}

// Recomputes member offsets and the struct size after packed/aligned has been
// applied post-hoc (the trailing / typedef attribute form, e.g.
// `typedef struct {...} __attribute__((packed)) T;`).  Returns false, leaving
// the layout untouched, for shapes we don't safely re-lay-out (anonymous
// members, whose copied symbol-table offsets would also need adjusting).
static bool RelayoutStruct(Struct* str) {
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* m = str->members.value.p[i];
    if (m->is_anon) {
      return false;
    }
  }
  bool is_union = str->is_union;
  str->next_offset = 0;
  str->current_offset = 0;
  str->size = 0;
  str->non_virtual_size = 0;
  str->alignment = 1;
  str->next_bit_pos = 65;
  LayoutCXXBaseSpecifiers(str);
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* m = str->members.value.p[i];
    if (m->is_using_declaration) {
      continue;
    }
    TypeRecord* type = m->symbol->type;
    if (m->bit_size > 0) {
      // Bitfield: replicate ParseBitField's placement using the stored width.
      int word_width = type->size * 8;
      if (str->next_bit_pos + m->bit_size > word_width) {
        AlignNextOffsetForSymbol(str, m->symbol);
        m->byte_offset = str->next_offset;
        m->index = i;
        str->next_bit_pos = 0;
        if (!is_union) {
          str->next_offset += type->size;
          str->size = str->next_offset;
        } else if (type->size > str->size) {
          str->size = type->size;
        }
      } else {
        m->byte_offset = str->current_offset;
      }
      m->bit_offset = str->next_bit_pos;
      if (!is_union) {
        str->next_bit_pos += m->bit_size;
      }
    } else {
      AlignNextOffsetForSymbol(str, m->symbol);
      m->byte_offset = str->next_offset;
      m->index = i;
      UpdateStructSize(str, type, is_union);
    }
  }
  str->non_virtual_size = str->size;
  LayoutCXXVirtualBaseSpecifiers(str);
  FinalizeStructAlignment(str);
  return true;
}

void TypeApplyStructAttributesFromSymbol(Symbol* sym) {
  if (sym == NULL || sym->attributes.length == 0) {
    return;
  }
  if (!AttributeListHas(&sym->attributes, "packed") &&
      AttributeListFind(&sym->attributes, "aligned") == NULL) {
    return;
  }
  TypeRecord* type = sym->type;
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return;
  }
  Struct* str = type->info.struct_info;
  bool was_packed = str->packed;
  int was_align = str->explicit_alignment;
  StructApplyLayoutAttributes(str, &sym->attributes);
  if (str->packed != was_packed || str->explicit_alignment != was_align) {
    if (RelayoutStruct(str)) {
      // The type's size may already have been cached at the old (unpacked)
      // value; force it to be recomputed from the struct's new size.
      type->size = 0;
      TypeRecordCalculateSize(type);
    }
  }
}

// Parse a struct.  The 'struct' or 'union' keyword has been
// consumed and the current token will be the follower.  This may
// be a tag name or an open brace, or semicolon.  Don't consume
// a semicolon at the end of the struct.
Symbol* TypeParserParseStruct(TypeParser* parser, bool is_union, bool is_class) {
  // Parse common __attribute__ syntax (e.g. struct __attribute__((packed)) ...).
  Vector attributes = {0};
  Vector bases = {0};
  VectorInit(&bases);
  while (true) {
    if (SyntaxParseCXXAlignas(parser->syntax, &attributes)) {
      continue;
    }
    if (LexMatch(parser->lex, TOK(attribute))) {
      SyntaxParseAttribute(parser->syntax, &attributes);
    } else if (SyntaxLookingAtCXXAttribute(parser->syntax)) {
      SyntaxParseCXXAttributes(parser->syntax, &attributes);
    } else {
      break;
    }
  }

  String tag_name = {0};
  FullyQualifiedIdentifier qualified_tag = {0};
  bool has_qualified_tag = false;
  Vector* specialization_args = NULL;
  Vector* completed_specialization_args = NULL;
  Symbol* tag = NULL;
  Symbol* specialization_template = NULL;
  String specialization_name;
  LocalSymbolTable* saved_specialization_tag_stack = NULL;
  Namespace* saved_specialization_namespace = NULL;
  bool using_specialization_namespace = false;
  bool is_full_specialization = false;
  bool is_partial_specialization = false;
  bool is_final = false;
  StringInit(&specialization_name, NULL);
  FullyQualifiedIdentifierInit(&qualified_tag);

  if (LexLookingAt(parser->lex, TOK(semicolon))) {
    // Don't consume the semicolon.
    goto done;
  }

  // Read the tag name if there is one.
  if (CompilerIsCXX() && (SyntaxCurrentTokenStartsQualifiedName(parser->syntax) ||
                          LexLookingAt(parser->lex, TOK(identifier)))) {
    SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
        parser->syntax, &qualified_tag, TC(openbra) | TC(decl));
    has_qualified_tag = qualified_tag.is_qualified;
    if (!has_qualified_tag) {
      StringSet(&tag_name, FullyQualifiedIdentifierLast(&qualified_tag));
    }
    if (qualified_tag.template_arguments.length > 0) {
      Vector* args =
          qualified_tag.template_arguments.value.p[
              qualified_tag.template_arguments.length - 1];
      specialization_args = TemplateArgumentVectorCopy(args);
    }
  } else if (SyntaxCurrentTokenStartsQualifiedName(parser->syntax)) {
    SyntaxParseFullyQualifiedIdentifier(parser->syntax, &qualified_tag);
    has_qualified_tag = qualified_tag.is_qualified;
    if (!has_qualified_tag) {
      StringSet(&tag_name, FullyQualifiedIdentifierLast(&qualified_tag));
    }
  } else if (LexLookingAt(parser->lex, TOK(identifier))) {
    // Struct tag is present.
    StringSetString(&tag_name, &parser->lex->spelling);
    LexNextToken(parser->lex);
  }
  while (SyntaxParseCXXAlignas(parser->syntax, &attributes) ||
         SyntaxParseCXXAttributes(parser->syntax, &attributes)) {
  }
  is_final = ParseCXXClassFinalSpecifier(parser);
  while (SyntaxParseCXXAlignas(parser->syntax, &attributes) ||
         SyntaxParseCXXAttributes(parser->syntax, &attributes)) {
  }
  is_full_specialization =
      parser->syntax->parsing_template_specialization &&
      specialization_args != NULL;
  is_partial_specialization =
      parser->syntax->parsing_template_declaration &&
      specialization_args != NULL;
  if (is_full_specialization || is_partial_specialization) {
    specialization_template =
        has_qualified_tag ? SyntaxFindQualifiedSymbol(parser->syntax,
                                                      &qualified_tag)
                          : SyntaxFindSymbol(parser->syntax, &tag_name);
    if (specialization_template == NULL ||
        !specialization_template->flags.is_template ||
        specialization_template->type == NULL ||
        !TypeIsStructOrUnion(specialization_template->type) ||
        specialization_template->type->info.struct_info == NULL) {
      SyntaxError(parser->syntax, "%s is not a class template",
                  qualified_tag.spelling.value);
    } else {
      completed_specialization_args = CompleteClassTemplateArguments(
          parser, specialization_template->type->info.struct_info,
          specialization_args);
      if (completed_specialization_args != NULL) {
        AppendTemplateInstantiationName(&specialization_name,
                                        specialization_template,
                                        completed_specialization_args);
        StringSetString(&tag_name, &specialization_name);
        has_qualified_tag = false;
        saved_specialization_tag_stack = parser->syntax->local_tag_stack;
        saved_specialization_namespace = parser->syntax->current_namespace;
        parser->syntax->local_tag_stack = NULL;
        parser->syntax->current_namespace =
            specialization_template->namespace_ != NULL
                ? specialization_template->namespace_
                : compiler->global_namespace;
        using_specialization_namespace = true;
      }
    }
  }

  ParseCXXBaseSpecifiers(parser, &bases, is_union, is_class);
  if (LexMatch(parser->lex, TOK(lbrace))) {
    if (has_qualified_tag) {
      SyntaxError(parser->syntax, "Cannot define qualified struct tag %s",
                  qualified_tag.spelling.value);
    }
    tag = ParseStructBody(parser, &tag_name, is_union, is_class, &attributes,
                          &bases);
    if (is_final && tag != NULL && tag->type != NULL &&
        tag->type->info.struct_info != NULL) {
      tag->type->info.struct_info->is_final = true;
    }
    if (is_partial_specialization && tag != NULL &&
        completed_specialization_args != NULL) {
      AddClassTemplatePartialSpecialization(parser, specialization_template,
                                            tag,
                                            completed_specialization_args);
    }
  } else {
    // No open brace, this is a reference to an existing struct or the
    // creation of a new one.
    if (tag_name.length == 0 && !has_qualified_tag) {
      // No tag name, nothing to do.
      goto done;
    }
    tag = has_qualified_tag ? SyntaxFindQualifiedTag(parser->syntax, &qualified_tag)
                            : SyntaxFindTag(parser->syntax, &tag_name);
    if (tag == NULL) {
      if (has_qualified_tag) {
        SyntaxError(parser->syntax, "Unknown struct tag %s",
                    qualified_tag.spelling.value);
        goto done;
      }
      // New tag.
      Struct* str = NewStruct(is_union);
      str->is_class = is_class;
      TypeRecord* type = NewTypeRecord(kTypeStruct, kQualPlain);
      type->info.struct_info = str;
      tag = NewSymbol(tag_name.value, type, STO(implicit));
      tag->flags.is_forward_declared = true;
      str->tag_name = &tag->name;
      str->tag_symbol = tag;
      SyntaxAddTag(parser->syntax, tag);
      AddInjectedClassName(parser, tag);
    } else {
      // Tag already exists, make sure it's the same tag type.
      CheckTagType(parser, tag, is_union, false);
    }
  }

done:
  if (using_specialization_namespace) {
    parser->syntax->local_tag_stack = saved_specialization_tag_stack;
    parser->syntax->current_namespace = saved_specialization_namespace;
  }
  if (tag != NULL && !is_partial_specialization) {
    parser->syntax->last_parsed_tag = tag;
  }
  FullyQualifiedIdentifierDestruct(&qualified_tag);
  StringDestruct(&tag_name);
  StringDestruct(&specialization_name);
  if (specialization_args != NULL) {
    VectorDeleteWithContents(specialization_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  if (completed_specialization_args != NULL) {
    VectorDeleteWithContents(completed_specialization_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  VectorDestructWithContents(&bases,
                             (VectorElementDestructor)CXXBaseSpecifierDelete,
                             /*free_element=*/false);
  AttributeListDestruct(&attributes);
  return tag;
}

//  C99 6.7.2.2
//  Each enumerated type shall be compatible with char, a signed integer type,
//  or an unsigned integer type. The choice of type is implementation-defined,110)
//  but shall be capable of representing the values of all the members of the
//  enumeration. The enumerated type is incomplete until after the }
//  that terminates the list of enumerator declarations.
//
//  110) An implementation may delay the choice of which integer type until
//  all enumeration constants have been seen.
//
// However, it appears that using a char as a type isn't a good idea since
// exising code might treat enums as ints and pass pointers to them.
// TODO: add a pragma or command line option to enable chars?
static bool EnumUnderlyingTypesMatch(Enum* e, TypeRecord* type) {
  return e != NULL && type != NULL && e->has_fixed_underlying &&
         e->fixed_underlying_type == type->type &&
         e->fixed_underlying_size == type->size;
}

static void SetEnumFixedUnderlying(Enum* e, TypeRecord* type) {
  if (e == NULL || type == NULL) {
    return;
  }
  e->has_fixed_underlying = true;
  e->fixed_underlying_type = type->type;
  e->fixed_underlying_size = type->size;
}

static TypeRecord* ParseEnumUnderlyingType(TypeParser* parser) {
  if (!CompilerIsCXX() || !LexMatch(parser->lex, TOK(colon))) {
    return NULL;
  }

  TypeParser underlying_parser;
  TypeParserInit(&underlying_parser, parser->lex, parser->syntax, STO(implicit),
                 parser->context);
  TypeRecord* type = TypeParserParseType(&underlying_parser, true);
  TypeParserDestruct(&underlying_parser);
  if (type == NULL) {
    type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }
  if (!TypeIsIntegral(type) || TypeIsEnum(type)) {
    SyntaxError(parser->syntax, "Enum underlying type must be integral");
    TypeRecordDelete(type);
    type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }
  return type;
}

static void ApplyEnumUnderlyingType(Syntax* syntax, Enum* e,
                                    TypeRecord* enum_type,
                                    TypeRecord* explicit_underlying,
                                    bool is_scoped) {
  TypeRecord* fixed_underlying = explicit_underlying;
  if (fixed_underlying == NULL && is_scoped && !e->has_fixed_underlying) {
    fixed_underlying = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }
  if (fixed_underlying == NULL) {
    return;
  }
  if (e->has_fixed_underlying &&
      !EnumUnderlyingTypesMatch(e, fixed_underlying)) {
    SyntaxError(syntax, "Enum %s redeclared with different underlying type",
                e->tag_name != NULL ? e->tag_name->value : "<anonymous>");
  }
  SetEnumFixedUnderlying(e, fixed_underlying);
  enum_type->type = kTypeEnum | e->fixed_underlying_type;
  enum_type->size = e->fixed_underlying_size;
  if (explicit_underlying == NULL) {
    TypeRecordDelete(fixed_underlying);
  }
}

static Type ParseEnumConstants(TypeParser* parser, Enum* e,
                               TypeRecord* enum_type) {
  enum TypeSelection {
    kUnsignedChar,      // Not used.
    kSignedChar,        // Not used.
    kUnsignedInt,
    kSignedInt,
  } type_selection = kUnsignedInt;
  
  while (!LexLookingAt(parser->lex, TOK(rbrace))) {
    if (LexLookingAt(parser->lex, TOK(identifier))) {
      String const_name;
      StringInit(&const_name, parser->lex->spelling.value);
      LexNextToken(parser->lex);
      int dependent_value_template_parameter_index = -1;
      if (LexMatch(parser->lex, TOK(equal))) {
        ASTNode* value =
            SyntaxParseSingleExpression(parser->syntax, TC(semicolon));
        value = AnalyzeExpression(value);
        int64_t next_value = e->next_value;
        if (!EvaluateIntegerExpression(value, &next_value)) {
          if (!CXXExpressionNamesNonTypeTemplateParameter(
                  value, &dependent_value_template_parameter_index)) {
            SyntaxError(parser->syntax,
                        "Constant integer expression required for value of "
                        "enum constant %s",
                        const_name.value);
          }
          next_value = e->next_value;
        }
        e->next_value = (int32_t)next_value;
        ASTNodeDelete(value);
      }
      // Determine the type of the enum based on the constant value.
      switch (type_selection) {
        case kUnsignedInt:
          if (e->next_value < 0) {
            type_selection = kSignedInt;
          }
          break;
        case kUnsignedChar:
          if (e->next_value > 255) {
            type_selection = kUnsignedInt;
          }
          if (e->next_value < 0) {
            if (e->next_value < 256) {
              type_selection = kSignedInt;
            } else {
              type_selection = kUnsignedInt;
            }
          }
          break;
        case kSignedInt:
          break;
        case kSignedChar:
          if (e->next_value > 255) {
            type_selection = kSignedInt;
          }
          break;
      }

      Symbol* ec = e->is_scoped
          ? NewScopedEnumConstant(const_name.value, e->next_value, enum_type)
          : NewEnumConstant(const_name.value, e->next_value);
      ec->dependent_value_template_parameter_index =
          dependent_value_template_parameter_index;
      if (dependent_value_template_parameter_index >= 0) {
        ec->flags.value_set = false;
      }
      e->next_value++;
      StringDestruct(&const_name);

      if (e->is_scoped) {
        VectorAppend(&e->constants, ec);
      } else {
        // Insert the constant as a symbol in the current scope.
        bool ok = SyntaxAddSymbol(parser->syntax, ec);
        if (!ok) {
          SyntaxError(parser->syntax,
                      "Enum constant %s is already defined in this scope",
                      ec->name.value);
          SymbolDelete(ec);
        } else {
          VectorAppend(&e->constants, ec);
        }
      }
    }
    if (!LexMatch(parser->lex, TOK(comma))) {
      break;
    }
  }
  if (e->has_fixed_underlying) {
    return e->fixed_underlying_type;
  }
  switch (type_selection) {
    case kUnsignedInt:
      return kTypeInt | kTypeUnsigned;
    case kUnsignedChar:
      return kTypeChar | kTypeUnsigned;
    case kSignedInt:
      return kTypeInt;
    case kSignedChar:
      return kTypeChar;
  }
}

static Symbol* ParseEnumBody(TypeParser* parser, String* tag_name,
                             bool is_scoped,
                             TypeRecord* explicit_underlying) {
  // We have an enum body.
  // First check that this is not a duplicate definition.
  Enum* e = NULL;
  bool empty_tag_name = tag_name->length == 0;
  if (empty_tag_name) {
    SyntaxFakeTagName(parser->syntax, tag_name);
  }
  Symbol* tag = SyntaxFindTopScopeTag(parser->syntax, tag_name);
  if (tag != NULL) {
    if (!tag->flags.is_forward_declared) {
      SyntaxError(parser->syntax, "Duplicate definition of enum %s",
                  tag_name->value);
    } else {
      CheckTagType(parser, tag, false, true);
      if (tag->type->info.enum_info != NULL &&
          tag->type->info.enum_info->is_scoped != is_scoped) {
        SyntaxError(parser->syntax, "Enum %s redeclared with different scopedness",
                    tag->name.value);
      }
    }
    e = tag->type->info.enum_info;
  } else {
    // Tag doesn't exist, create one.
    e = NewEnum();
    TypeRecord* type = NewTypeRecordWithSize(kTypeEnum, kQualPlain);
    type->info.enum_info = e;
    tag = NewSymbol(tag_name->value, type, STO(implicit));
    e->tag_name = &tag->name;
    e->tag_symbol = tag;
    e->is_scoped = is_scoped;
    if (empty_tag_name) {
      tag->flags.invented = true;
    }
    SyntaxAddTag(parser->syntax, tag);
    AddInjectedEnumName(parser, tag);
  }
  ApplyEnumUnderlyingType(parser->syntax, e, tag->type, explicit_underlying,
                          is_scoped);

  // Note in the symbol that this tag is now defined and not
  // forward declared.
  tag->flags.is_forward_declared = false;
  tag->flags.is_defined = true;

  // Now 'tag' will be the struct tag pointer
  // and 'e' will be a pointer to the Enum information.
  e->tag_symbol = tag;
  e->is_scoped = is_scoped;
  Type t = ParseEnumConstants(parser, e, tag->type);
  tag->type->type |= t;
  tag->type->size = SizeofType(t);
  if (e->is_scoped) {
    for (size_t i = 0; i < e->constants.length; i++) {
      Symbol* constant = e->constants.value.p[i];
      constant->type->type |= t;
      constant->type->size = tag->type->size;
    }
  }
  
  SyntaxNeedBracket(parser->syntax, TOK(rbrace), TC(expr));
  return tag;
}

// Parse an enum definition or reference.
Symbol* TypeParserParseEnum(TypeParser* parser) {
  // Parse common __attribute__ syntax.
  Vector attributes = {0};
  while (LexLookingAt(parser->lex, TOK(attribute)) ||
         SyntaxLookingAtCXXAttribute(parser->syntax)) {
    if (LexMatch(parser->lex, TOK(attribute))) {
      SyntaxParseAttribute(parser->syntax, &attributes);
    } else {
      SyntaxParseCXXAttributes(parser->syntax, &attributes);
    }
  }

  String tag_name = {0};
  FullyQualifiedIdentifier qualified_tag = {0};
  bool has_qualified_tag = false;
  Symbol* tag = NULL;
  FullyQualifiedIdentifierInit(&qualified_tag);
  bool is_scoped = false;
  TypeRecord* explicit_underlying = NULL;

  if (CompilerIsCXX() &&
      (LexLookingAt(parser->lex, TOK(class)) ||
       LexLookingAt(parser->lex, TOK(struct)))) {
    is_scoped = true;
    LexNextToken(parser->lex);
    SyntaxParseCXXAttributes(parser->syntax, &attributes);
  }

  if (LexLookingAt(parser->lex, TOK(semicolon))) {
    // Don't consume the semicolon.
    goto done;
  }

  // Read the tag name if there is one.
  if (SyntaxCurrentTokenStartsQualifiedName(parser->syntax)) {
    SyntaxParseFullyQualifiedIdentifier(parser->syntax, &qualified_tag);
    has_qualified_tag = qualified_tag.is_qualified;
    if (!has_qualified_tag) {
      StringSet(&tag_name, FullyQualifiedIdentifierLast(&qualified_tag));
    }
  } else if (LexLookingAt(parser->lex, TOK(identifier))) {
    // Struct tag is present.
    StringSetString(&tag_name, &parser->lex->spelling);
    LexNextToken(parser->lex);
  }
  SyntaxParseCXXAttributes(parser->syntax, &attributes);
  explicit_underlying = ParseEnumUnderlyingType(parser);
  if (LexMatch(parser->lex, TOK(lbrace))) {
    if (has_qualified_tag) {
      SyntaxError(parser->syntax, "Cannot define qualified enum tag %s",
                  qualified_tag.spelling.value);
    }
    tag = ParseEnumBody(parser, &tag_name, is_scoped, explicit_underlying);
  } else {
    // No open brace, this is a reference to an existing enum or the
    // creation of a new one.
    if (tag_name.length == 0 && !has_qualified_tag) {
      // No tag name, nothing to do.
      goto done;
    }
    tag = has_qualified_tag ? SyntaxFindQualifiedTag(parser->syntax, &qualified_tag)
                            : SyntaxFindTag(parser->syntax, &tag_name);
    if (tag == NULL) {
      if (has_qualified_tag) {
        SyntaxError(parser->syntax, "Unknown enum tag %s",
                    qualified_tag.spelling.value);
        goto done;
      }
      // New tag.
      Enum* e = NewEnum();
      TypeRecord* type = NewTypeRecordWithSize(kTypeEnum, kQualPlain);
      type->info.enum_info = e;
      tag = NewSymbol(tag_name.value, type, STO(implicit));
      tag->flags.is_forward_declared = true;
      e->tag_name = &tag->name;
      e->tag_symbol = tag;
      e->is_scoped = is_scoped;
      ApplyEnumUnderlyingType(parser->syntax, e, tag->type, explicit_underlying,
                              is_scoped);
      SyntaxAddTag(parser->syntax, tag);
      AddInjectedEnumName(parser, tag);
    } else {
      // Tag already exists, make sure it's the same tag type.
      CheckTagType(parser, tag, false, true);
      if (tag->type->info.enum_info != NULL) {
        tag->type->info.enum_info->tag_symbol = tag;
      }
      if (tag->type->info.enum_info != NULL &&
          tag->type->info.enum_info->is_scoped != is_scoped) {
        SyntaxError(parser->syntax, "Enum %s redeclared with different scopedness",
                    tag->name.value);
      }
      ApplyEnumUnderlyingType(parser->syntax, tag->type->info.enum_info,
                              tag->type, explicit_underlying, is_scoped);
    }
  }

done:
  if (explicit_underlying != NULL) {
    TypeRecordDelete(explicit_underlying);
  }
  FullyQualifiedIdentifierDestruct(&qualified_tag);
  StringDestruct(&tag_name);
  AttributeListDestruct(&attributes);
  return tag;
}

//
// Type inference functions.
//

bool TypeIsInt(TypeRecord* type);
bool TypeIsChar(TypeRecord* type);
bool TypeIsShort(TypeRecord* type);
bool TypeIsLong(TypeRecord* type);
bool TypeIsLongLong(TypeRecord* type);
bool TypeIsUnsignedInt(TypeRecord* type);
bool TypeIsUnsignedChar(TypeRecord* type);
bool TypeIsUnsignedShort(TypeRecord* type);
bool TypeIsUnsignedLong(TypeRecord* type);
bool TypeIsUnsignedLongLong(TypeRecord* type);
bool TypeIsFloat(TypeRecord* type);
bool TypeIsDouble(TypeRecord* type);
bool TypeIsLongDouble(TypeRecord* type);
bool TypeIsBool(TypeRecord* type);
bool TypeIsVoid(TypeRecord* type);
bool TypeIsNullPointer(TypeRecord* type);

bool TypeIsPointer(TypeRecord* type);
bool TypeIsPrimitive(TypeRecord* type);
bool TypeIsPointerOrArray(TypeRecord* type);
bool TypeIsIntegral(TypeRecord* type);
bool TypeIsFloatingPoint(TypeRecord* type);
bool TypeIsFunction(TypeRecord* type);
bool TypeIsFunctionDefinition(TypeRecord* type);
bool TypeIsFunctionPointer(TypeRecord* type);
bool TypeIsStructOrUnionPointer(TypeRecord* type);
bool TypeIsFunctionReturningStructOrUnion(TypeRecord* type);
bool TypeIsVoidFunction(TypeRecord* type);

bool TypeIsPointerToSameType(TypeRecord* ptr1, TypeRecord* ptr2);
bool TypeIsStructOrUnion(TypeRecord* type);
bool TypeIsScalar(TypeRecord* type);
bool TypeIsVoidPointer(TypeRecord* type);
bool TypeIsArray(TypeRecord* type);
bool TypeIsConst(TypeRecord* type);
bool TypeIsVolatile(TypeRecord* type);
bool TypeIsEnum(TypeRecord* type);

bool TypeIsUnsigned(TypeRecord* type) {
  if (!compiler->plain_char_is_signed && type->type == kTypeChar) {
    return true;
  }
  return TypeIsPrimitive(type) && (type->type & (kTypeUnsigned | kTypeBool)) != 0;
}

bool TypeIsSigned(TypeRecord* type) {
  if (compiler->plain_char_is_signed && type->type == kTypeChar) {
    return true;
  }
  return TypeIsPrimitive(type) && (type->type & kTypeSigned) != 0;
}

bool TypeIsReference(TypeRecord* type) {
  return type->declarator == kDeclReference ||
         type->declarator == kDeclRValueReference;
}

bool TypeIsScopedEnum(TypeRecord* type) {
  return TypeIsEnum(type) && type->info.enum_info != NULL &&
         type->info.enum_info->is_scoped;
}


bool TypeIsIntConstant(TypeRecord* type);
bool TypeIsFloatingPointConstant(TypeRecord* type);
bool TypeIsUnknown(TypeRecord* type);
bool TypeIsFixedArray(TypeRecord* type);
bool TypeIsVLA(TypeRecord* type);

static bool FunctionPrototypesEqual(FunctionInfo* a, FunctionInfo* b) {
  if (a->prototype.length != b->prototype.length) {
    return false;
  }
  // The trailing const on a C++ member function is part of its signature: a
  // non-const and a const member function with otherwise identical parameters
  // are distinct overloads.  This matters for dependent return types (e.g.
  // `T&` vs `const T&`) where the return type comparison cannot tell them
  // apart, leaving the const qualifier as the only distinguishing feature.
  if (a->is_const_member != b->is_const_member) {
    return false;
  }
  if (a->ref_qualifier != b->ref_qualifier) {
    return false;
  }
  for (size_t i = 0; i < a->prototype.length; i++) {
    Symbol* s1 = a->prototype.value.p[i];
    Symbol* s2 = b->prototype.value.p[i];
    if (!TypeEqual(s1->type, s2->type)) {
      return false;
    }
  }
  return true;
}

bool TypeEqual(TypeRecord* t1, TypeRecord* t2) {
  if (t1 == NULL || t2 == NULL) {
    return t1 == t2;
  }
  if (t1->declarator != t2->declarator) {
    return false;
  }
  // `restrict` is an optimizer hint, not part of a type's identity for
  // redeclaration or overload purposes (e.g. `const char *restrict` and
  // `const char *` name the same parameter type), so ignore it when comparing
  // qualifiers.  The remaining cv-qualifiers stay significant so that template
  // argument identities such as `const T` versus `T` remain distinct.
  if ((t1->qualifiers & ~kQualRestrict) != (t2->qualifiers & ~kQualRestrict)) {
    return false;
  }
  // Dependent (unknown) leaves need care.  Only a leaf primitive carries a
  // template parameter's positional identity, so pointer/reference/array
  // wrappers fall through to the structural comparison below and recurse into
  // `next` (their declarator shapes already matched); function types likewise
  // compare their full signature via FunctionPrototypesEqual.  At a dependent
  // leaf, distinct template parameters (`T` vs `U`), a parameter versus a
  // concrete type, and a parameter versus a dependent member type (`T` vs
  // `T::type`) are all different signatures and must stay distinct so that
  // overloads like `f(const T&)` and `f(const U&)` do not collide.  Only when
  // neither leaf is a positionally-identified parameter do we keep the lenient
  // "unknown matches anything" behavior, so that an earlier error involving a
  // genuinely unresolved symbol does not cascade into a spurious overload
  // clash.
  if (t1->declarator == kDeclPrimitive &&
      ((t1->type & kTypeUnknown) != 0 || (t2->type & kTypeUnknown) != 0)) {
    bool t1_param = t1->template_parameter_index >= 0;
    bool t2_param = t2->template_parameter_index >= 0;
    if (t1_param || t2_param) {
      if (t1_param != t2_param ||
          t1->template_parameter_index != t2->template_parameter_index) {
        return false;
      }
      String* m1 = t1->dependent_member_name;
      String* m2 = t2->dependent_member_name;
      if ((m1 == NULL) != (m2 == NULL)) {
        return false;
      }
      return m1 == NULL || StringEqualString(m1, m2);
    }
    // A dependent member of a template-id scope (`enable_if<Cond, T>::type`):
    // the scope template, member name, and the scope's arguments -- which may
    // include value-dependent SFINAE conditions -- distinguish otherwise
    // identically-spelled unknown leaves (so two enable_if-guarded overloads are
    // not treated as one).
    if (t1->template_origin != NULL && t2->template_origin != NULL) {
      if (t1->template_origin != t2->template_origin) {
        return false;
      }
      String* m1 = t1->dependent_member_name;
      String* m2 = t2->dependent_member_name;
      if ((m1 == NULL) != (m2 == NULL)) {
        return false;
      }
      if (m1 != NULL && !StringEqualString(m1, m2)) {
        return false;
      }
      return TemplateArgumentVectorEqual(t1->template_arguments,
                                         t2->template_arguments);
    }
    return true;
  }
  switch (t1->declarator) {
    case kDeclArray:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return t1->info.array.size.fixed == t2->info.array.size.fixed;
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return TypeEqual(t1->next, t2->next);

    case kDeclFunction:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return FunctionPrototypesEqual(&t1->info.function, &t2->info.function);
    case kDeclPrimitive:
      if (TypeIsStructOrUnion(t1) || TypeIsStructOrUnion(t2)) {
        if (!TypeIsStructOrUnion(t1) || !TypeIsStructOrUnion(t2) ||
            t1->type != t2->type || t1->qualifiers != t2->qualifiers) {
          return false;
        }
        if (t1->template_origin != NULL || t2->template_origin != NULL) {
          return t1->template_origin == t2->template_origin &&
                 TemplateArgumentVectorEqual(t1->template_arguments,
                                             t2->template_arguments);
        }
        return t1->info.struct_info == t2->info.struct_info;
      }
      if (TypeIsEnum(t1) && TypeIsEnum(t2)) {
        if (TypeIsScopedEnum(t1) || TypeIsScopedEnum(t2)) {
          return t1->info.enum_info == t2->info.enum_info &&
                 t1->qualifiers == t2->qualifiers;
        }
        // Enums can be char, signed int or unsigned int.
        int e1 = t1->type & ~(kTypeInt | kTypeChar | kTypeSigned | kTypeUnsigned);
        int e2 = t2->type & ~(kTypeInt | kTypeChar | kTypeSigned | kTypeUnsigned);
        return e1 == e2 && t1->qualifiers == t2->qualifiers;

      }
      return t1->type == t2->type && t1->qualifiers == t2->qualifiers;
  }
}

bool StructIsDerivedFrom(Struct* from, Struct* to, bool public_only) {
  if (from == NULL || to == NULL) {
    return false;
  }
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if ((public_only && base->access != kAccessPublic) ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (base->type->info.struct_info == to ||
        StructIsDerivedFrom(base->type->info.struct_info, to, public_only)) {
      return true;
    }
  }
  return false;
}

bool TypeIsDerivedFrom(TypeRecord* from, TypeRecord* to) {
  if (from == NULL || to == NULL || !TypeIsStructOrUnion(from) ||
      !TypeIsStructOrUnion(to) || from->info.struct_info == NULL ||
      to->info.struct_info == NULL) {
    return false;
  }
  return StructIsDerivedFrom(from->info.struct_info, to->info.struct_info,
                             /*public_only=*/true);
}

static bool StructBaseOffset(Struct* from, Struct* to, bool public_only,
                             int inherited_offset, int* offset) {
  if (from == NULL || to == NULL) {
    return false;
  }
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if ((public_only && base->access != kAccessPublic) ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    int base_offset = inherited_offset + base->byte_offset;
    if (base->type->info.struct_info == to) {
      if (offset != NULL) {
        *offset = base_offset;
      }
      return true;
    }
    if (StructBaseOffset(base->type->info.struct_info, to, public_only,
                         base_offset, offset)) {
      return true;
    }
  }
  return false;
}

bool TypeBaseOffset(TypeRecord* from, TypeRecord* to, bool public_only,
                    int* offset) {
  if (from == NULL || to == NULL || !TypeIsStructOrUnion(from) ||
      !TypeIsStructOrUnion(to) || from->info.struct_info == NULL ||
      to->info.struct_info == NULL) {
    return false;
  }
  if (from->info.struct_info == to->info.struct_info) {
    if (offset != NULL) {
      *offset = 0;
    }
    return true;
  }
  return StructBaseOffset(from->info.struct_info, to->info.struct_info,
                          public_only, 0, offset);
}

static void CXXBaseAdjustmentSet(CXXBaseAdjustment* adjustment,
                                 CXXBaseAdjustmentKind kind,
                                 int byte_offset,
                                 int vbtable_index) {
  if (adjustment == NULL) {
    return;
  }
  adjustment->kind = kind;
  adjustment->byte_offset = byte_offset;
  adjustment->vbtable_index = vbtable_index;
}

static bool StructNonVirtualBaseOffset(Struct* from, Struct* to,
                                       bool public_only,
                                       int inherited_offset,
                                       int* offset) {
  if (from == NULL || to == NULL) {
    return false;
  }
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if (base->is_virtual ||
        (public_only && base->access != kAccessPublic) ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    int base_offset = inherited_offset + base->byte_offset;
    if (base->type->info.struct_info == to) {
      if (offset != NULL) {
        *offset = base_offset;
      }
      return true;
    }
    if (StructNonVirtualBaseOffset(base->type->info.struct_info, to,
                                   public_only, base_offset, offset)) {
      return true;
    }
  }
  return false;
}

static bool StructBaseAdjustment(Struct* from, Struct* to, bool public_only,
                                 int inherited_offset,
                                 CXXBaseAdjustment* adjustment) {
  if (from == NULL || to == NULL) {
    return false;
  }
  for (size_t i = 0; i < from->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = from->virtual_bases.value.p[i];
    if ((public_only && base->access != kAccessPublic) ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    int virtual_base_offset = 0;
    if (base_struct == to ||
        StructNonVirtualBaseOffset(base_struct, to, public_only, 0,
                                   &virtual_base_offset)) {
      CXXBaseAdjustmentSet(adjustment, kCXXBaseAdjustmentVirtual,
                           virtual_base_offset, base->vbtable_index);
      return true;
    }
  }
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if ((public_only && base->access != kAccessPublic) ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (base->is_virtual) {
      continue;
    }
    int base_offset = inherited_offset + base->byte_offset;
    if (base->type->info.struct_info == to) {
      CXXBaseAdjustmentSet(adjustment, kCXXBaseAdjustmentStatic,
                           base_offset, -1);
      return true;
    }
    if (StructBaseAdjustment(base->type->info.struct_info, to, public_only,
                             base_offset, adjustment)) {
      return true;
    }
  }
  return false;
}

bool TypeBaseAdjustment(TypeRecord* from, TypeRecord* to, bool public_only,
                        CXXBaseAdjustment* adjustment) {
  if (from == NULL || to == NULL || !TypeIsStructOrUnion(from) ||
      !TypeIsStructOrUnion(to) || from->info.struct_info == NULL ||
      to->info.struct_info == NULL) {
    return false;
  }
  if (from->info.struct_info == to->info.struct_info) {
    CXXBaseAdjustmentSet(adjustment, kCXXBaseAdjustmentNone, 0, -1);
    return true;
  }
  return StructBaseAdjustment(from->info.struct_info, to->info.struct_info,
                              public_only, 0, adjustment);
}

bool TypeIsAbstractClass(TypeRecord* type) {
  return CompilerIsCXX() && type != NULL && TypeIsStructOrUnion(type) &&
         type->info.struct_info != NULL && type->info.struct_info->is_abstract;
}

bool TypeContainsAuto(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if ((t->type & kTypeAuto) != 0) {
      return true;
    }
  }
  return false;
}

bool TypeFunctionReturnContainsAuto(TypeRecord* type) {
  return TypeIsFunction(type) && TypeContainsAuto(type->next);
}

static TypeRecord* NewDeclaratorLike(TypeRecord* pattern) {
  TypeRecord* result = NULL;
  switch (pattern->declarator) {
    case kDeclPointer:
      result = NewPointerTypeRecord(pattern->qualifiers);
      break;
    case kDeclReference:
      result = NewReferenceTypeRecord(pattern->qualifiers, false);
      break;
    case kDeclRValueReference:
      result = NewReferenceTypeRecord(pattern->qualifiers, true);
      break;
    case kDeclArray:
      result = NewBasicArrayTypeRecord(pattern->qualifiers,
                                       pattern->info.array.size.fixed,
                                       pattern->info.array.is_vla);
      break;
    default:
      result = TypeRecordCopy(pattern);
      break;
  }
  return result;
}

TypeRecord* TypeDeduceAuto(TypeRecord* pattern, TypeRecord* initializer_type) {
  if (pattern == NULL || initializer_type == NULL) {
    return NULL;
  }
  if ((pattern->type & kTypeAuto) != 0 &&
      pattern->declarator == kDeclPrimitive) {
    TypeRecord* deduced = TypeRecordCopy(initializer_type);
    deduced->qualifiers |= pattern->qualifiers;
    TypeRecordCalculateSize(deduced);
    return deduced;
  }

  switch (pattern->declarator) {
    case kDeclPointer:
      if (!TypeIsPointerOrArray(initializer_type)) {
        return NULL;
      }
      break;
    case kDeclArray:
      if (!TypeIsArray(initializer_type)) {
        return NULL;
      }
      break;
    case kDeclReference:
    case kDeclRValueReference:
      break;
    default:
      if ((pattern->type & kTypeAuto) != 0) {
        return NULL;
      }
      return TypeRecordCopy(pattern);
  }

  TypeRecord* next_initializer =
      TypeIsPointerOrArray(initializer_type) &&
              pattern->declarator != kDeclReference &&
              pattern->declarator != kDeclRValueReference
          ? initializer_type->next
          : initializer_type;
  TypeRecord* next = TypeDeduceAuto(pattern->next, next_initializer);
  if (next == NULL) {
    return NULL;
  }
  TypeRecord* result = NewDeclaratorLike(pattern);
  TypeRecordChain(result, next);
  result->type = next->type;
  TypeRecordCalculateSize(result);
  return result;
}

bool TypeAssignmentCompatible(TypeRecord* from, TypeRecord* to) {
  if (TypeEqual(to, from)) {
    return true;
  }
  // A pointer can be assigned to a const pointer of the same type.
  if (TypeIsPointerOrArray(to) && TypeIsPointerOrArray(from) &&
      to->next != NULL && from->next != NULL) {
    if (TypeBaseOffset(from->next, to->next, /*public_only=*/true, NULL)) {
      return true;
    }
    int to_quals = to->next->qualifiers & ~kQualConst;
    int from_quals = from->next->qualifiers & ~kQualConst;
    if (to_quals == from_quals) {
      return true;
    }
  }
  return false;
}

bool TypeEqualIgnoringSign(TypeRecord* t1, TypeRecord* t2) {
  if (t1 == NULL || t2 == NULL) {
    return t1 == t2;
  }
  if (t1->declarator != t2->declarator) {
    return false;
  }
  switch (t1->declarator) {
    case kDeclArray:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return t1->info.array.size.fixed == t2->info.array.size.fixed;
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return TypeEqual(t1->next, t2->next);

    case kDeclFunction:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return FunctionPrototypesEqual(&t1->info.function, &t2->info.function);
    case kDeclPrimitive: {
      Type a = t1->type & ~(kTypeUnsigned | kTypeSigned);
      Type b = t2->type & ~(kTypeUnsigned | kTypeSigned);
      return a == b;
    }
  }
}

static void FunctionPrototypesDetails(SourceLocation location, FunctionInfo* a, FunctionInfo* b) {
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  if (a->prototype.length != b->prototype.length) {
    ReportNote(filename, lineno, "Different number of arguments: %zd vs %zd",
               a->prototype.length, b->prototype.length);
    return;
  }
  for (size_t i = 0; i < a->prototype.length; i++) {
    Symbol* s1 = a->prototype.value.p[i];
    Symbol* s2 = b->prototype.value.p[i];
    if (!TypeEqual(s1->type, s2->type)) {
      TypeErrorDetails(location, s1->type, s2->type);
      ReportNote(filename, lineno, "  for argument #%zd", i+1);
    }
  }
}

void TypeErrorDetails(SourceLocation location, TypeRecord* t1, TypeRecord* t2) {
  String error1 = {0};
  String error2 = {0};
  TypeRecordToString(t1, &error1);
  TypeRecordToString(t2, &error2);

  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
    
  if (t1->declarator != t2->declarator) {
    ReportNote(filename, lineno, "Declarators '%s' and '%s' are different",
               error1.value, error2.value);
    return;
  }
  switch (t1->declarator) {
    case kDeclArray:
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      ReportNote(filename, lineno, "Declaration of '%s' and '%s' are different",
                 error1.value, error2.value);
      TypeErrorDetails(location, t1->next, t2->next);
      break;

    case kDeclFunction:
      ReportNote(filename, lineno, "Declaration of '%s' and '%s' are different",
                 error1.value, error2.value);
      TypeErrorDetails(location, t1->next, t2->next);
      return FunctionPrototypesDetails(location, &t1->info.function, &t2->info.function);
      
    case kDeclPrimitive:
      if (t1->type != t2->type || t1->qualifiers != t2->qualifiers) {
        ReportNote(filename, lineno, "Types '%s' and '%s' are different",
                   error1.value, error2.value);

      }
  }
  StringDestruct(&error1);
  StringDestruct(&error1);
}

