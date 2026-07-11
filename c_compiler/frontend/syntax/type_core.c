//
//  type_core.c
//  c_compiler
//

#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <assert.h>
#include "ast.h"
#include "compiler.h"
#include "concepts.h"
#include "constexpr.h"
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"

static int next_type_id = 0;

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

void TemplateArgumentVectorDelete(Vector* args) {
  if (args == NULL) {
    return;
  }
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
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
  record->dependent_member_template_arguments = NULL;
  record->dependent_decltype_expr = NULL;
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
    if (record->dependent_member_template_arguments != NULL) {
      VectorDeleteWithContents(
          record->dependent_member_template_arguments,
          (VectorElementDestructor)TemplateArgumentVectorDelete,
          /*free_element=*/false);
      record->dependent_member_template_arguments = NULL;
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
  if (record->declarator == kDeclArray && !record->info.array.is_vla) {
    record->size = record->info.array.size.fixed * record->next->size;
    return record;
  }
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
TemplateParameter* TemplateParameterCopy(TemplateParameter* param) {
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
TemplateArgument* TemplateArgumentCopy(TemplateArgument* arg) {
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

Vector* TemplateArgumentVectorListCopy(Vector* list) {
  if (list == NULL) {
    return NULL;
  }
  Vector* copy = NewVector();
  for (size_t i = 0; i < list->length; i++) {
    VectorAppend(copy, TemplateArgumentVectorCopy(list->value.p[i]));
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
  r->dependent_member_template_arguments =
      TemplateArgumentVectorListCopy(record->dependent_member_template_arguments);
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
      NewTypeRecord(owner != NULL && owner->is_union ? kTypeUnion : kTypeStruct,
                    is_const_member ? kQualConst : kQualPlain);
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
ClassTemplatePartialSpecialization* NewClassTemplatePartialSpecialization(
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
void ClassTemplatePartialSpecializationDelete(
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

CXXBaseSpecifier* NewCXXBaseSpecifier(TypeRecord* type,
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

void CXXBaseSpecifierDelete(CXXBaseSpecifier* base) {
  TypeRecordDelete(base->type);
  free(base);
}

void CXXMemberUsingDeclarationDelete(CXXMemberUsingDeclaration* decl) {
  if (decl == NULL) {
    return;
  }
  TypeRecordDelete(decl->base_type);
  StringDestruct(&decl->member_name);
  free(decl);
}

CXXVirtualBaseInfo* NewCXXVirtualBaseInfo(TypeRecord* type,
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

void CXXVirtualBaseInfoDelete(CXXVirtualBaseInfo* base) {
  TypeRecordDelete(base->type);
  free(base);
}

void CXXVBTableInfoDelete(CXXVBTableInfo* info) {
  free(info);
}

void CXXVTableInfoDelete(CXXVTableInfo* info) {
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
  s->lexical_parent = NULL;
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
