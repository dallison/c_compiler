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

static int next_type_id = 0;

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
  copy->index = param->index;
  return copy;
}

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
  return copy;
}

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
      VectorAppend(&r->info.function.prototype, clone);
    }
    VectorInit(&r->info.function.template_parameters);
    for (size_t i = 0; i < record->info.function.template_parameters.length; i++) {
      VectorAppend(&r->info.function.template_parameters,
                   TemplateParameterCopy(
                       record->info.function.template_parameters.value.p[i]));
    }
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
  return t;
}

TypeRecord* NewSizeTypeRecord() {
  if (compiler->pointer_size == 8) {
    return NewTypeRecordWithSize(kTypeLongLong | kTypeUnsigned, kQualPlain);
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
  mem->is_member_function = false;
  mem->access = kAccessPublic;
  mem->overload_next = NULL;
  return mem;
}

void TemplateParameterDelete(TemplateParameter* param) {
  if (param == NULL) {
    return;
  }
  StringDestruct(&param->name);
  TypeRecordDelete(param->type);
  TypeRecordDelete(param->default_type);
  free(param);
}

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

static CXXBaseSpecifier* NewCXXBaseSpecifier(TypeRecord* type,
                                             CXXAccess access,
                                             bool is_virtual) {
  CXXBaseSpecifier* base = malloc(sizeof(CXXBaseSpecifier));
  base->type = type;
  TypeRecordIncRef(type);
  base->access = access;
  base->byte_offset = 0;
  base->is_virtual = is_virtual;
  return base;
}

static void CXXBaseSpecifierDelete(CXXBaseSpecifier* base) {
  TypeRecordDelete(base->type);
  free(base);
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
  VectorInit(&s->virtual_bases);
  VectorInit(&s->members);
  VectorInit(&s->virtual_members);
  VectorInit(&s->template_parameters);
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
  s->is_template = false;
  s->is_aggregate = CompilerIsCXX();
  s->cxx_special_members_complete = false;
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

static void TypeRecordToTemplateKeyString(TypeRecord* type, String* result) {
  switch (type->declarator) {
    case kDeclPrimitive:
      if (TypeIsUnknown(type) && type->template_parameter_index >= 0) {
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
  parser->context = context;
  parser->cxx_member_owner = NULL;
  parser->cxx_member_definition = NULL;
  parser->declarator_template_arguments = NULL;
}

void TypeParserReset(TypeParser* parser) {
  parser->symbol = NULL;
  parser->storage = STO(implicit);
  parser->found_void = false;
  parser->dimension_count = 0;
  parser->is_consteval = false;
  parser->is_constinit = false;
  parser->cxx_member_owner = NULL;
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
    {TOK(bad), kTypeImplicit},
};

static TypeRecord* NewDecltypeReference(TypeRecord* expr_type, bool rvalue) {
  TypeRecord* base = TypeIsReference(expr_type) ? expr_type->next : expr_type;
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
  TypeRecordChain(ref, base);
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

static Vector* CompleteFunctionTemplateArguments(TypeParser* parser,
                                                 TypeRecord* func,
                                                 Vector* args,
                                                 bool emit_error);
static TypeRecord* SubstituteTemplateParameters(TypeParser* parser,
                                                TypeRecord* type,
                                                Vector* args);
static TemplateArgument* NewEmptyPackTemplateArgument(
    TemplateParameterKind kind);
static int FindTemplateParameterPackIndex(Vector* template_parameters);
static void AppendSubstitutedFormalParameter(TypeParser* parser, Vector* out,
                                             Symbol* formal, Vector* args,
                                             int rebase_base);
static void RebaseTemplateParameterIndices(TypeRecord* type, int base);
static bool StructContainsTemplateParameter(Struct* str);
static TypeRecord* SubstituteNestedStructTemplateParameters(TypeParser* parser,
                                                            TypeRecord* type,
                                                            Vector* args);
static StructMember* InstantiateTemplateMemberFunction(TypeParser* parser,
                                                       Struct* owner,
                                                       StructMember* member,
                                                       Vector* args);

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
  if (!CompilerIsCXX() || type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_symbol == NULL) {
    return false;
  }
  return !type->info.struct_info->tag_symbol->flags.invented;
}

static void AddCXXNestedTypeMember(TypeParser* parser, Struct* owner,
                                   TypeRecord* type, CXXAccess access) {
  Struct* nested = type->info.struct_info;
  Symbol* tag = nested->tag_symbol;
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
    return;
  }

  if (arg->kind == kTemplateParameterType && arg->type == NULL) {
    return;
  }

  TemplateArgument* concrete = malloc(sizeof(TemplateArgument));
  concrete->kind = arg->kind;
  concrete->is_pack_expansion = false;
  concrete->type = NULL;
  concrete->int_value = arg->int_value;
  concrete->template_parameter_index = arg->template_parameter_index;
  concrete->pack_arguments = NULL;
  if (arg->kind == kTemplateParameterType && arg->type != NULL) {
    concrete->type = SubstituteTemplateParameters(parser, arg->type, args);
    concrete->template_parameter_index = -1;
  } else if (arg->kind == kTemplateParameterNonType &&
             arg->template_parameter_index >= 0 &&
             (size_t)arg->template_parameter_index < args->length) {
    TemplateArgument* actual = args->value.p[arg->template_parameter_index];
    if (actual->kind == kTemplateParameterNonType &&
        actual->pack_arguments == NULL) {
      concrete->int_value = actual->int_value;
      concrete->template_parameter_index = actual->template_parameter_index;
    }
  }
  VectorAppend(out, concrete);
}

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

static TypeRecord* SubstituteTemplateParameters(TypeParser* parser,
                                                TypeRecord* type,
                                                Vector* args) {
  if (type == NULL) {
    return NULL;
  }
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
      return TypeRecordCopy(type);
    }
    TypeRecord* subst = TypeRecordCopy(member->symbol->type);
    subst->qualifiers |= type->qualifiers;
    return TypeRecordCalculateSize(subst);
  }
  if (type->template_origin != NULL && type->template_arguments != NULL) {
    Vector* concrete_args =
        SubstituteTemplateArgumentVectorForTypes(parser,
                                                type->template_arguments, args);
    TypeRecord* subst =
        InstantiateSimpleClassTemplate(parser, type->template_origin,
                                       concrete_args);
    subst->qualifiers |= type->qualifiers;
    VectorDeleteWithContents(concrete_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCalculateSize(subst);
  }
  if (type->declarator == kDeclPrimitive &&
      type->template_parameter_index >= 0) {
    int index = type->template_parameter_index;
    if (index < 0 || (size_t)index >= args->length) {
      return TypeRecordCopy(type);
    }
    TemplateArgument* arg = args->value.p[index];
    if (arg->kind != kTemplateParameterType || arg->type == NULL) {
      return TypeRecordCopy(type);
    }
    TypeRecord* subst = TypeRecordCopy(arg->type);
    subst->qualifiers |= type->qualifiers;
    return subst;
  }
  if (CompilerIsCXX() && TypeIsStructOrUnion(type) &&
      StructContainsTemplateParameter(type->info.struct_info)) {
    return SubstituteNestedStructTemplateParameters(parser, type, args);
  }

  TypeRecord* copy = TypeRecordCopy(type);
  if (copy->declarator == kDeclArray &&
      type->info.array.template_parameter_index >= 0) {
    int index = type->info.array.template_parameter_index;
    if ((size_t)index < args->length) {
      TemplateArgument* arg = args->value.p[index];
      if (arg->kind == kTemplateParameterNonType) {
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
      TypeRecordIncRef(copy->next);
      copy->type = copy->next->type;
    }
  }
  if (TypeIsFunction(copy)) {
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
  VectorAppend(out, clone);
}

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

  TypeRecord* formal_type =
      SubstituteTemplateParameters(parser, formal->type, args);
  AppendFormalClone(out, formal, formal_type, rebase_base);
}

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
  copy->info.struct_info = str;
  copy->size = 0;

  for (size_t i = 0; i < from->members.length; i++) {
    StructMember* member = from->members.value.p[i];
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    if (member->is_member_function) {
      StructMember* instantiated =
          InstantiateTemplateMemberFunction(parser, str, member, args);
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
        SubstituteTemplateParameters(parser, member->symbol->type, args);
    TypeRecordCalculateSize(member_type);
    Symbol* member_symbol =
        NewSymbol(member->symbol->name.value, member_type, member->symbol->storage);
    member_symbol->location = member->symbol->location;
    member_symbol->flags = member->symbol->flags;

    StructMember* instantiated = NewStructMember(member_symbol);
    instantiated->default_initializer =
        CloneCXXDefaultMemberInitializer(member->default_initializer);
    instantiated->access = member->access;
    instantiated->is_anon = member->is_anon;
    instantiated->is_static = member->is_static;
    instantiated->is_member_function = member->is_member_function;
    instantiated->bit_size = member->bit_size;
    instantiated->bit_offset = member->bit_offset;
    instantiated->cxx_vcall_offset = member->cxx_vcall_offset;

    if (!instantiated->is_static && !instantiated->is_member_function &&
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
  FinalizeStructAlignment(str);
  return TypeRecordCalculateSize(copy);
}

static bool TypeContainsTemplateParameter(TypeRecord* type);

static void RebaseTemplateArgumentParameterIndices(TemplateArgument* arg,
                                                   int base);

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
  }
}

static void RebaseTemplateArgumentParameterIndices(TemplateArgument* arg,
                                                   int base) {
  if (arg == NULL || base <= 0) {
    return;
  }
  if (arg->template_parameter_index >= base) {
    arg->template_parameter_index -= base;
  }
  RebaseTemplateParameterIndices(arg->type, base);
}

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
    TemplateParameter* param =
        TemplateParameterCopy(from->info.function.template_parameters.value.p[i]);
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
  return TypeContainsTemplateParameter(arg->type);
}

static bool TemplateArgumentVectorContainsTemplateParameter(Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    if (TemplateArgumentContainsTemplateParameter(args->value.p[i])) {
      return true;
    }
  }
  return false;
}

static bool TypeContainsTemplateParameter(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsUnknown(t) && t->template_parameter_index >= 0) {
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

static Symbol* FindTemplateInstantiationTag(TypeParser* parser, Symbol* templ,
                                            String* name) {
  LocalSymbolTable* saved_tag_stack = parser->syntax->local_tag_stack;
  Namespace* saved_namespace = parser->syntax->current_namespace;
  parser->syntax->local_tag_stack = NULL;
  parser->syntax->current_namespace =
      templ->namespace_ != NULL ? templ->namespace_ : compiler->global_namespace;
  Symbol* symbol = SyntaxFindTag(parser->syntax, name);
  parser->syntax->local_tag_stack = saved_tag_stack;
  parser->syntax->current_namespace = saved_namespace;
  return symbol;
}

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
  func->info.function.is_explicit = from->info.function.is_explicit;
  func->info.function.is_explicit_conversion =
      from->info.function.is_explicit_conversion;
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
  CopyFunctionTemplateParameters(func, from,
                                 from->info.function.template_parameter_base);
  TypeRecord* return_type = SubstituteTemplateParameters(parser, from->next, args);
  RebaseTemplateParameterIndices(return_type,
                                 from->info.function.template_parameter_base);
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
          parser, &func->info.function.prototype, formal, args,
          from->info.function.template_parameter_base);
      continue;
    }
    TypeRecord* formal_type =
        SubstituteTemplateParameters(parser, formal->type, args);
    RebaseTemplateParameterIndices(formal_type,
                                   from->info.function.template_parameter_base);
    Symbol* clone = NewSymbol(formal->name.value, formal_type, formal->storage);
    clone->flags = formal->flags;
    clone->flags.is_argument = true;
    clone->location = formal->location;
    VectorAppend(&func->info.function.prototype, clone);
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    formal->value.arg_number = (int32_t)i;
  }
  (void)parser;
  return func;
}

typedef struct {
  Map symbol_map;
  Map pack_symbol_map;
  TypeParser* parser;
  Vector* args;
  TypeRecord* to_func;
  int rebase_template_parameter_base;
} TemplateFunctionBodyClone;

static void DeleteMappedVector(MapKeyValue* kv) {
  VectorDelete(kv->value.p);
}

static const char* CXXConstructorNameForRecord(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  return type->info.struct_info->tag_name->value;
}

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
  StringSet(member_name->value.string, constructor_name);
}

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

static void RewriteTemplateBodyIdentifiers(ASTNode* node, Map* symbol_map) {
  ASTNodeVisit(node, RewriteTemplateBodyIdentifierVisitor, 0, symbol_map);
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

static bool IsIdentifierPackExpansion(ASTNode* node) {
  if (node == NULL || (node->flags & kASTPackExpansion) == 0 ||
      node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  return id->symbol != NULL && id->symbol->flags.is_parameter_pack;
}

static void ExpandClonedCallPackActuals(TemplateFunctionBodyClone* clone,
                                        ASTNode* node) {
  if (node->op != AST_OP(call)) {
    return;
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
}

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

static ASTNode* NewFoldIdentifier(Symbol* symbol, SourceLocation location) {
  return NewIdentifierASTNode(symbol, location);
}

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
  if (pack_node == NULL || pack_node->op != AST_OP(identifier)) {
    return node;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)pack_node;
  Vector* replacements = MapFindPointerKey(&clone->pack_symbol_map, id->symbol);
  if (replacements == NULL) {
    return node;
  }
  if (replacements->length == 0) {
    if (seed != NULL) {
      return seed;
    }
    return NewFoldIdentity(node->op, node->location, clone->parser);
  }

  if (pack_on_left) {
    ASTNode* result = seed != NULL
                          ? seed
                          : NewFoldIdentifier(
                                replacements->value.p[replacements->length - 1],
                                node->location);
    size_t start = seed != NULL ? replacements->length
                                : replacements->length - 1;
    for (size_t i = start; i > 0; i--) {
      ASTNode* left = NewFoldIdentifier(replacements->value.p[i - 1],
                                        node->location);
      result = NewBinaryASTNode(node->op, NULL, node->location, left, result);
    }
    return result;
  }

  ASTNode* result =
      seed != NULL ? seed : NewFoldIdentifier(replacements->value.p[0],
                                              node->location);
  size_t start = seed != NULL ? 0 : 1;
  for (size_t i = start; i < replacements->length; i++) {
    ASTNode* right = NewFoldIdentifier(replacements->value.p[i], node->location);
    result = NewBinaryASTNode(node->op, NULL, node->location, result, right);
  }
  return result;
}

static ASTNode* CloneTemplateFunctionBodyNode(ASTNode* node, void* data) {
  TemplateFunctionBodyClone* clone = data;
  if (node->op == AST_OP(sizeof)) {
    SizeofASTNode* sizeof_node = (SizeofASTNode*)node;
    if (sizeof_node->is_pack_size && sizeof_node->expr != NULL &&
        sizeof_node->expr->op == AST_OP(identifier)) {
      IdentifierASTNode* id = (IdentifierASTNode*)sizeof_node->expr;
      int pack_index = id->symbol != NULL ? id->symbol->template_parameter_index
                                          : -1;
      if ((pack_index < 0) && id->symbol != NULL) {
        TypeIsTemplateParameterPlaceholder(id->symbol->type, &pack_index);
      }
      if (id->symbol != NULL && id->symbol->flags.is_template_parameter &&
          id->symbol->flags.is_parameter_pack &&
          pack_index >= 0 && (size_t)pack_index < clone->args->length) {
        TemplateArgument* arg = clone->args->value.p[pack_index];
        if (arg != NULL && arg->pack_arguments != NULL) {
          return NewIntConstantASTNode(
              (int64_t)arg->pack_arguments->length, NewSizeTypeRecord(),
              node->location);
        }
      }
    }
  }
  if ((node->flags & kASTFoldExpression) != 0) {
    return ExpandClonedFoldExpression(clone, node);
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->template_arguments != NULL) {
      Vector* concrete_args = SubstituteTemplateArgumentVector(
          clone->parser, id->template_arguments, clone->args,
          clone->rebase_template_parameter_base);
      VectorDeleteWithContents(id->template_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      id->template_arguments = concrete_args;
    }
    if ((node->flags & kASTPackExpansion) != 0 &&
        id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
      return node;
    }
    Symbol* replacement = MapFindPointerKey(&clone->symbol_map, id->symbol);
    if (replacement != NULL) {
      id->symbol = replacement;
      ASTNodeSetType(node, replacement->type);
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
  if (node->type != NULL) {
    TypeRecord* type =
        SubstituteTemplateParameters(clone->parser, node->type, clone->args);
    RebaseTemplateParameterIndices(type, clone->rebase_template_parameter_base);
    ASTNodeSetType(node, type);
    TypeRecordDelete(type);
  }
  ExpandClonedCallPackActuals(clone, node);
  ExpandClonedBracedInitializerPackElements(clone, node);
  CloneTemplateLocalDeclarationSymbol(clone, node);
  RewriteClonedConstructorMemberCall(clone, node);
  return node;
}

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
  size_t to_index = 0;
  for (size_t i = 0; i < from->info.function.prototype.length; i++) {
    Symbol* from_formal = from->info.function.prototype.value.p[i];
    if (from_formal == NULL) {
      continue;
    }
    if (from_formal->flags.is_parameter_pack) {
      Vector* replacements = NewVector();
      int pack_index = -1;
      size_t pack_length = 0;
      if (TypeIsTemplateParameterPlaceholder(from_formal->type, &pack_index) &&
          pack_index >= 0 && (size_t)pack_index < args->length) {
        TemplateArgument* pack = args->value.p[pack_index];
        if (pack != NULL && pack->pack_arguments != NULL) {
          pack_length = pack->pack_arguments->length;
        }
      }
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
  ASTNode* body = ASTNodeClone(from->info.function.body,
                               CloneTemplateFunctionBodyNode, &clone, NULL);
  MapDestructWithContents(&clone.pack_symbol_map, DeleteMappedVector);
  MapDestruct(&clone.symbol_map);
  return body;
}

static void QueueTemplateMemberFunctionDefinition(Symbol* symbol,
                                                  Symbol* template_definition,
                                                  TypeParser* parser,
                                                  Vector* args) {
  if (symbol == NULL || symbol->type == NULL ||
      template_definition == NULL || template_definition->type == NULL ||
      template_definition->type->info.function.body == NULL) {
    return;
  }
  symbol->type->info.function.body =
      CloneTemplateFunctionBody(parser, template_definition->type,
                                symbol->type, args);
  symbol->type->info.function.definition = true;
  symbol->flags.is_defined = true;
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
  func->info.function.is_explicit = from->info.function.is_explicit;
  func->info.function.is_explicit_conversion =
      from->info.function.is_explicit_conversion;
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
    VectorAppend(&func->info.function.prototype, clone);
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    formal->value.arg_number = (int32_t)i;
  }
  return func;
}

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
  return left->int_value == right->int_value &&
         left->template_parameter_index == right->template_parameter_index;
}

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

static Symbol* FindFunctionTemplateInstantiation(Symbol* templ,
                                                 TypeRecord* type,
                                                 Vector* args) {
  for (Symbol* candidate = templ; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->flags.is_template && candidate->type != NULL &&
        TypeIsFunction(candidate->type) && TypeEqual(candidate->type, type) &&
        TemplateArgumentVectorEqual(candidate->type->template_arguments, args)) {
      return candidate;
    }
  }
  return NULL;
}

static void AppendFunctionTemplateInstantiation(Symbol* templ,
                                                Symbol* instantiated) {
  Symbol* tail = templ;
  while (tail->overload_next != NULL) {
    tail = tail->overload_next;
  }
  tail->overload_next = instantiated;
  templ->flags.is_overloaded = true;
  instantiated->flags.is_overloaded = true;
}

static Symbol* InstantiateSimpleFunctionTemplate(TypeParser* parser,
                                                 Symbol* templ,
                                                 Vector* args) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type)) {
    return templ;
  }
  Vector* completed_args =
      CompleteFunctionTemplateArguments(parser, templ->type, args,
                                        /*emit_error=*/true);
  if (completed_args == NULL) {
    return templ;
  }

  Symbol* template_definition = templ->value.func_defn;
  if (template_definition == NULL || template_definition->type == NULL ||
      template_definition->type->info.function.body == NULL) {
    template_definition = templ;
  }
  if (template_definition->type->info.function.body == NULL) {
    SyntaxError(parser->syntax,
                "Function template definition is required for instantiation");
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }
  TypeRecord* func =
      InstantiateFunctionTemplateType(parser, template_definition->type,
                                      completed_args);
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

static TemplateArgument* NewDeducedTypeTemplateArgument(TypeRecord* type) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = TypeRecordCopy(type);
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  return arg;
}

static TemplateArgument* NewDeducedNonTypeTemplateArgument(long long value) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = kTemplateParameterNonType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = value;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  return arg;
}

static TypeRecord* FunctionTemplateDeductionActualType(TypeRecord* actual) {
  if (actual == NULL) {
    return NULL;
  }
  TypeRecord* deduced = TypeRecordCopy(actual);
  deduced->qualifiers = kQualPlain;
  return deduced;
}

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

static bool DeduceFunctionTemplateTemplateArguments(Vector* args,
                                                    size_t explicit_arg_count,
                                                    TypeRecord* formal,
                                                    TypeRecord* actual) {
  if (formal == NULL || actual == NULL ||
      formal->template_origin == NULL ||
      formal->template_origin != actual->template_origin ||
      formal->template_arguments == NULL ||
      actual->template_arguments == NULL ||
      formal->template_arguments->length != actual->template_arguments->length) {
    return false;
  }
  for (size_t i = 0; i < formal->template_arguments->length; i++) {
    TemplateArgument* formal_arg = formal->template_arguments->value.p[i];
    TemplateArgument* actual_arg = actual->template_arguments->value.p[i];
    if (formal_arg == NULL || actual_arg == NULL ||
        formal_arg->kind != actual_arg->kind) {
      return false;
    }
    if (formal_arg->kind == kTemplateParameterType) {
      if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                              formal_arg->type,
                                              actual_arg->type)) {
        return false;
      }
    } else if (formal_arg->template_parameter_index >= 0) {
      int index = formal_arg->template_parameter_index;
      if (!SetDeducedFunctionTemplateNonTypeArgument(
              args, explicit_arg_count, index, actual_arg->int_value)) {
        return false;
      }
    } else if (formal_arg->int_value != actual_arg->int_value) {
      return false;
    }
  }
  return true;
}

static bool StructContainsTemplateParameter(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && member->symbol != NULL &&
        !member->is_static && !member->is_member_function &&
        !StructMemberIsNestedType(member) &&
        TypeContainsTemplateParameter(member->symbol->type)) {
      return true;
    }
  }
  return false;
}

static bool DeduceFunctionTemplateTypeArgument(Vector* args,
                                               size_t explicit_arg_count,
                                               TypeRecord* formal,
                                               TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return false;
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

    TemplateArgument* explicit_arg =
        explicit_args != NULL && explicit_index < explicit_args->length
            ? TemplateArgumentCopy(explicit_args->value.p[explicit_index++])
            : NULL;
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

static Vector* DeduceSimpleFunctionTemplateArguments(Symbol* templ,
                                                     Vector* explicit_args,
                                                     Vector* actuals,
                                                     size_t first_formal_arg) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type)) {
    return NULL;
  }
  TypeRecord* func = templ->value.func_defn != NULL &&
                             templ->value.func_defn->type != NULL
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
  if (func->info.function.template_parameter_count <= 0 ||
      func->info.function.unknown_args || func->info.function.varargs ||
      (formal_pack_index < 0 &&
       actuals->length != fixed_formal_count) ||
      (formal_pack_index >= 0 && actuals->length < fixed_formal_count)) {
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
      if (formal == NULL ||
          !TypeIsTemplateParameterPlaceholder(formal->type,
                                              &pack_type_index) ||
          pack_type_index < 0 ||
          (size_t)pack_type_index >= args->length) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        return NULL;
      }
      TemplateArgument* pack = args->value.p[pack_type_index];
      if (pack == NULL) {
        pack = NewEmptyPackTemplateArgument(kTemplateParameterType);
        args->value.p[pack_type_index] = pack;
      }
      ASTNode* actual = actuals->value.p[i];
      if (actual == NULL || actual->type == NULL ||
          pack->kind != kTemplateParameterType) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        return NULL;
      }
      VectorAppend(pack->pack_arguments,
                   NewDeducedTypeTemplateArgument(actual->type));
      continue;
    }
    formal = func->info.function.prototype.value.p[i + first_formal_arg];
    ASTNode* actual = actuals->value.p[i];
    if (formal == NULL || actual == NULL ||
        !(DeduceFunctionTemplateArrayInitializerArgument(
              args, explicit_arg_count, formal->type, actual) ||
          DeduceFunctionTemplateInitializerListArgument(
              args, explicit_arg_count, formal->type, actual) ||
          DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                             formal->type, actual->type))) {
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

Symbol* TypeDeduceFunctionTemplateFromCall(Syntax* syntax, Symbol* templ,
                                           Vector* actuals) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, NULL, actuals, 0);
}

Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
    Syntax* syntax, Symbol* templ, Vector* explicit_args, Vector* actuals) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, explicit_args, actuals, 0);
}

Symbol* TypeDeduceFunctionTemplateFromCallWithOffset(Syntax* syntax,
                                                     Symbol* templ,
                                                     Vector* actuals,
                                                     size_t first_formal_arg) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, NULL, actuals, first_formal_arg);
}

Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    Syntax* syntax, Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return templ;
  }
  Symbol* symbol = TypeInstantiateFunctionTemplate(syntax, templ, args);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return symbol;
}

bool TypeCanDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return false;
  }
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return true;
}

Vector* TypeDeduceFunctionTemplateArgumentsFromCall(Symbol* templ,
                                                    Vector* actuals,
                                                    size_t first_formal_arg) {
  return DeduceSimpleFunctionTemplateArguments(templ, NULL, actuals,
                                               first_formal_arg);
}

void TypeAddCXXDeductionGuide(Symbol* class_template, Symbol* guide) {
  if (class_template == NULL || class_template->type == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL || guide == NULL) {
    return;
  }
  VectorAppend(&class_template->type->info.struct_info->deduction_guides, guide);
}

static bool CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(Symbol* origin) {
  return origin != NULL && StorageIs(origin->storage, STO(typedef)) &&
         origin->type != NULL && origin->type->template_origin != NULL;
}

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

static StructMember* InstantiateTemplateMemberFunction(TypeParser* parser,
                                                       Struct* owner,
                                                       StructMember* member,
                                                       Vector* args) {
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
  QueueTemplateMemberFunctionDefinition(symbol, template_definition,
                                        parser, args);
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

static Vector* CompleteClassTemplateArguments(TypeParser* parser,
                                              Struct* template_struct,
                                              Vector* args) {
  return CompleteTemplateArguments(parser, &template_struct->template_parameters,
                                   args,
                                   "Class template instantiation is not supported yet",
                                   /*emit_error=*/true);
}

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

static TypeRecord* InstantiateSimpleClassTemplate(TypeParser* parser,
                                                  Symbol* templ,
                                                  Vector* args) {
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
  if (!ClassTemplateInstantiationMembersSupported(parser, template_struct)) {
    StringDestruct(&instantiated_name);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(templ->type);
  }

  Struct* str = NewStruct(template_struct->is_union);
  str->is_class = template_struct->is_class;
  str->packed = template_struct->packed;
  str->explicit_alignment = template_struct->explicit_alignment;
  str->pack = template_struct->pack;
  TypeRecord* type = NewTypeRecord(template_struct->is_union ? kTypeUnion
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
    return TypeRecordCopy(templ->type);
  }

  for (size_t i = 0; i < template_struct->bases.length; i++) {
    CXXBaseSpecifier* template_base = template_struct->bases.value.p[i];
    TypeRecord* base_type =
        SubstituteTemplateParameters(parser, template_base->type,
                                     completed_args);
    TypeRecordCalculateSize(base_type);
    VectorAppend(&str->bases,
                 NewCXXBaseSpecifier(base_type, template_base->access,
                                     template_base->is_virtual));
    TypeRecordDelete(base_type);
  }
  CollectCXXVirtualBases(str);
  CopyCXXBaseVirtualMembers(str);
  LayoutCXXBaseSpecifiers(str);

  for (size_t i = 0; i < template_struct->members.length; i++) {
    StructMember* member = template_struct->members.value.p[i];
    if (StructMemberIsNestedType(member)) {
      TypeRecord* nested_type =
          SubstituteTemplateParameters(parser, member->symbol->type,
                                       completed_args);
      Symbol* nested_symbol =
          NewSymbol(member->symbol->name.value, nested_type, STO(typedef));
      nested_symbol->location = member->symbol->location;
      StructMember* nested_member = NewStructMember(nested_symbol);
      nested_member->access = member->access;
      AddStructMember(parser, str, nested_member);
      continue;
    }
    if (member->is_member_function) {
      StructMember* instantiated =
          InstantiateTemplateMemberFunction(parser, str, member, completed_args);
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
                                     completed_args);
    TypeRecordCalculateSize(member_type);
    Symbol* member_symbol =
        NewSymbol(member->symbol->name.value, member_type, member->symbol->storage);
    member_symbol->location = member->symbol->location;
    member_symbol->flags = member->symbol->flags;
    StructMember* instantiated = NewStructMember(member_symbol);
    instantiated->default_initializer =
        CloneCXXDefaultMemberInitializer(member->default_initializer);
    instantiated->access = member->access;
    instantiated->is_anon = member->is_anon;
    instantiated->is_static = member->is_static;
    instantiated->is_member_function = member->is_member_function;
    instantiated->bit_size = member->bit_size;
    instantiated->bit_offset = member->bit_offset;
    instantiated->cxx_vcall_offset = member->cxx_vcall_offset;
    if (!instantiated->is_static && !StructMemberIsNestedType(instantiated)) {
      AlignNextOffset(str, member_type);
      instantiated->byte_offset = str->next_offset;
    } else {
      instantiated->byte_offset = member->byte_offset;
    }
    instantiated->index = str->members.length;
    AddStructMember(parser, str, instantiated);
    if (!instantiated->is_static && !StructMemberIsNestedType(instantiated)) {
      UpdateStructSize(str, member_type, str->is_union);
    }
  }
  FinalizeStructAlignment(str);
  TypeRecordCalculateSize(type);
  ComputeCXXAggregateStatus(str);
  AddImplicitCXXSpecialMembers(parser, str, tag);
  AddImplicitCXXDestructorIfNeeded(parser, str, tag);
  AddImplicitCXXDeductionGuides(str, tag);
  StringDestruct(&instantiated_name);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return TypeRecordCopy(type);
}

TypeRecord* TypeInstantiateClassTemplate(Syntax* syntax, Symbol* templ,
                                         Vector* args) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* type = InstantiateSimpleClassTemplate(&parser, templ, args);
  TypeParserDestruct(&parser);
  return type;
}

static bool CXXSymbolIsStdInitializerListTemplate(Symbol* symbol) {
  return symbol != NULL && strcmp(symbol->name.value, "initializer_list") == 0 &&
         symbol->namespace_ != NULL &&
         strcmp(symbol->namespace_->qualified_name.value, "std") == 0;
}

bool TypeIsCXXInitializerList(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_symbol == NULL) {
    return false;
  }
  Symbol* tag = type->info.struct_info->tag_symbol;
  if (CXXSymbolIsStdInitializerListTemplate(tag->type != NULL
                                                ? tag->type->template_origin
                                                : NULL)) {
    return true;
  }
  return CXXSymbolIsStdInitializerListTemplate(type->template_origin);
}

TypeRecord* TypeCXXInitializerListElement(TypeRecord* type) {
  if (!TypeIsCXXInitializerList(type) || type->template_arguments == NULL ||
      type->template_arguments->length != 1) {
    return NULL;
  }
  TemplateArgument* arg = type->template_arguments->value.p[0];
  return arg != NULL && arg->kind == kTemplateParameterType ? arg->type : NULL;
}

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
  VectorAppend(args, arg);
  TypeRecord* type = TypeInstantiateClassTemplate(syntax, templ, args);
  VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return type;
}

Symbol* TypeInstantiateFunctionTemplate(Syntax* syntax, Symbol* templ,
                                        Vector* args) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  Symbol* symbol = InstantiateSimpleFunctionTemplate(&parser, templ, args);
  TypeParserDestruct(&parser);
  return symbol;
}

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
              parser->syntax->parsing_template_declaration &&
              TypeIsStructOrUnion(symbol->type)) {
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
      if (symbol != NULL) {
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
            if (symbol->flags.is_template && args != NULL &&
                parser->syntax->parsing_template_declaration &&
                TypeIsStructOrUnion(symbol->type)) {
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
  bool is_parameter_pack = CompilerIsCXX() && LexMatch(parser->lex,
                                                       TOK(ellipsis));
  TypeParserParsePointer(parser);

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
  parser->symbol->flags.is_parameter_pack = is_parameter_pack;
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
  if (LexMatch(parser->lex, TOK(star))) {
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

// Parse a formal argument declaration.  Takes ownership of
// formal.
static void ParseFormalArgument(TypeParser* proto_parser,
                                TypeRecord* func,
                                Symbol* formal,
                                int arg_number) {
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


// Prototype style.  C still allows old-style K&R code.
typedef enum  {
  kStyleUnknown,
  kStyleOld,
  kStyleNew
} PrototypeStyle;

static PrototypeStyle ParseFunctionParameter(TypeParser* proto_parser, TypeRecord* func,
                                             PrototypeStyle style,
                                             int arg_number) {
  if (proto_parser->found_void ||
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
    style = ParseFunctionParameter(proto_parser, func, style, arg_number);
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

static void SkipBalancedParenthesizedTokens(TypeParser* parser) {
  SyntaxNeedBracket(parser->syntax, TOK(lparen), TC(exprsep));
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
}

static void ParseCXXExceptionSpecifier(TypeParser* parser) {
  if (!CompilerIsCXX()) {
    return;
  }
  if (LexMatch(parser->lex, TOK(noexcept))) {
    if (LexLookingAt(parser->lex, TOK(lparen))) {
      SkipBalancedParenthesizedTokens(parser);
    }
    return;
  }
  if (LexMatch(parser->lex, TOK(throw))) {
    if (LexLookingAt(parser->lex, TOK(lparen))) {
      SkipBalancedParenthesizedTokens(parser);
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

static void ParseFunctionDecl(TypeParser* parser) {
  TypeParser proto_parser;
  TypeParserInit(&proto_parser, parser->lex, parser->syntax, STO(auto), kParsingPrototype);
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
  ParseCXXExceptionSpecifier(parser);
  if (CompilerIsCXX() && TypeContainsAuto(parser->base_type) &&
      LexMatch(parser->lex, TOK(arrow))) {
    TypeRecord* trailing_return = ParseCXXTrailingReturnType(parser);
    TypeRecordChain(func, trailing_return);
  }
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
  if (parser->context != kParsingFileScope ||
      (!parser->is_constexpr && !parser->is_constinit)) {
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

static void AlignNextOffset(Struct* str, TypeRecord* type) {
  // A packed struct places members on byte boundaries with no padding, so the
  // effective member alignment is 1.
  int alignment = str->packed ? 1 : TypeRecordAlignment(type);
  // #pragma pack(n) caps the effective alignment of each member at n bytes.
  if (str->pack > 0 && alignment > str->pack) {
    alignment = str->pack;
  }
  if (alignment > str->alignment) {
    str->alignment = alignment;
  }
  str->next_offset = (str->next_offset + (alignment - 1)) & ~(alignment - 1);
  str->next_bit_pos = 65;
  str->current_offset = str->next_offset;
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
    if (base_type == NULL || !TypeIsStructOrUnion(base_type) ||
        base_type->info.struct_info == NULL) {
      SyntaxError(parser->syntax, "base class must be a class or struct type");
      TypeRecordDelete(base_type);
      continue;
    }
    TypeRecordCalculateSize(base_type);
    VectorAppend(bases, NewCXXBaseSpecifier(base_type, access, is_virtual));
    TypeRecordDelete(base_type);
  } while (LexMatch(parser->lex, TOK(comma)));
}

static void LayoutCXXBaseSpecifiers(Struct* str) {
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->is_virtual) {
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

static StructMember* FindCXXFinalOverrider(Struct* complete,
                                           StructMember* base_member) {
  if (complete == NULL || base_member == NULL || base_member->symbol == NULL ||
      base_member->symbol->type == NULL) {
    return base_member;
  }
  for (size_t i = 0; i < complete->members.length; i++) {
    StructMember* candidate = complete->members.value.p[i];
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
  return base_member;
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
  Symbol* symbol = NewSymbol(name.value,
                             NewCXXVTableType(source->virtual_members.length),
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
  for (size_t i = 0; i < source->virtual_members.length; i++) {
    StructMember* member = source->virtual_members.value.p[i];
    member = FindCXXFinalOverrider(complete, member);
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    Initializer* init = malloc(sizeof(Initializer));
    init->offset = (int32_t)(i * SizeofPointer());
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
    AlignNextOffset(
        str, member_symbol->type);  // Will set current_offset and next_offset.
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
      a->info.function.is_const_member != b->info.function.is_const_member) {
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
    SyntaxError(parser->syntax, "%s marked override but does not override",
                member->symbol->name.value);
  }
  if (override != NULL) {
    if (override->symbol->type->info.function.is_final) {
      SyntaxError(parser->syntax, "%s overrides final function",
                  member->symbol->name.value);
    }
    func->info.function.is_virtual = true;
    func->info.function.virtual_index =
        override->symbol->type->info.function.virtual_index;
    member->cxx_vcall_offset = CXXBaseOffsetForMember(str, override);
  }
  if (func->info.function.is_final && !func->info.function.is_virtual) {
    SyntaxError(parser->syntax, "%s marked final but is not virtual",
                member->symbol->name.value);
  }
  if (func->info.function.is_pure_virtual && !func->info.function.is_virtual) {
    SyntaxError(parser->syntax, "%s is pure but is not virtual",
                member->symbol->name.value);
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
        member->is_member_function) {
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
                                    bool is_virtual, bool is_constexpr) {
  if (!CompilerIsCXX() || class_name->length == 0) {
    return false;
  }

  bool is_destructor = LexMatch(parser->lex, TOK(tilde));
  if (!LexLookingAt(parser->lex, TOK(identifier)) ||
      !CXXClassNameMatchesUnqualifiedTemplateName(class_name,
                                                  &parser->lex->spelling)) {
    if (is_destructor) {
      SyntaxError(parser->syntax, "Expected class name after '~'");
    }
    return is_destructor;
  }

  SourceLocation location = parser->lex->current_token_location;
  LexNextToken(parser->lex);
  if (!LexMatch(parser->lex, TOK(lparen))) {
    if (is_destructor) {
      SyntaxError(parser->syntax, "Expected '(' in destructor declaration");
    }
    return is_destructor;
  }

  TypeParser proto_parser;
  TypeParserInit(&proto_parser, parser->lex, parser->syntax, STO(auto),
                 kParsingPrototype);
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constexpr = is_constexpr;
  func->info.function.is_constructor = !is_destructor;
  func->info.function.is_destructor = is_destructor;
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
  CXXFinalizeSpecialMemberMetadata(member_symbol, str, true);
  StructMember* member = NewStructMember(member_symbol);
  member->is_member_function = true;
  member->access = access;
  StructMember* existing = MapFindPointerKey(&str->symbol_table, &member_name);
  if (existing != NULL) {
    if (!CanOverloadStructMember(existing, member)) {
      SyntaxError(parser->syntax, "Duplicate class member %s",
                  member_name.value);
      StructMemberDelete(member);
      StringDestruct(&member_name);
      SkipInlineMemberFunctionBody(parser);
      return true;
    }
    if (FindStructMemberOverload(existing, member_symbol->type) != NULL) {
      SyntaxError(parser->syntax, "Duplicate class member %s",
                  member_name.value);
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
  VectorAppend(&parser->syntax->inline_static_member_definitions, decl);
  if (symbol->flags.is_constexpr || symbol->flags.is_constinit ||
      (TypeIsConst(symbol->type) && !TypeIsStructOrUnion(symbol->type))) {
    SemanticAnalyzeVariableDefinition(parser->syntax,
                                      (VariableDeclarationASTNode*)decl);
  }
}


static void ParseStructMembers(TypeParser* parser, Struct* str, bool is_union,
                               String* tag_name) {
  CXXAccess current_access = str->is_class ? kAccessPrivate : kAccessPublic;
  while (!LexLookingAt(parser->lex, TOK(rbrace))) {
    if (CompilerIsCXX() && LexLookingAt(parser->lex, TOK(static_assert))) {
      SyntaxParseStaticAssert(parser->syntax);
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
    }

    Vector member_attributes = {0};
    VectorInit(&member_attributes);
    SyntaxParseCXXAttributes(parser->syntax, &member_attributes);

    if (CompilerIsCXX() && LexMatch(parser->lex, TOK(using))) {
      ParseCXXMemberUsingAlias(parser, str, current_access,
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
    bool is_constexpr_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(constexpr));
    parser->is_inline = is_inline_member;
    parser->is_constexpr = is_constexpr_member;
    bool saw_explicit_member = false;
    bool is_explicit_member =
        ParseCXXExplicitSpecifier(parser, &saw_explicit_member);
    if (!is_inline_member) {
      is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
      parser->is_inline = is_inline_member;
    }
    if (!is_constexpr_member) {
      is_constexpr_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(constexpr));
      parser->is_constexpr = is_constexpr_member;
    }
    bool is_virtual_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(virtual));
    if (!is_inline_member) {
      is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
      parser->is_inline = is_inline_member;
    }
    if (ParseClassSpecialMember(parser, str, tag_name, current_access,
                                is_virtual_member, is_constexpr_member)) {
      AttributeListDestruct(&member_attributes);
      if (is_member_template) {
        SyntaxError(parser->syntax, "Special member templates are not supported yet");
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
    if (!is_inline_member) {
      is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
      parser->is_inline = is_inline_member;
    }
    if (!is_constexpr_member) {
      is_constexpr_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(constexpr));
      parser->is_constexpr = is_constexpr_member;
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
            LexLookingAt(parser->lex, TOK(struct));
    TypeRecord* member_type = TypeParserParseType(parser, true);
    bool member_decl_had_inline_body = false;
    while (!LexEof(parser->lex)) {
      bool has_inline_body = false;
      if (possible_anon && LexLookingAt(parser->lex, TOK(semicolon))) {
        if (TypeIsNamedCXXNestedType(member_type)) {
          AddCXXNestedTypeMember(parser, str, member_type, current_access);
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
        SyntaxParseCXXAttributes(parser->syntax, &member_attributes);
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
        member->access = current_access;
        if (member->is_member_function) {
          member_symbol->type->info.function.is_constexpr =
              is_constexpr_member;
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
            QueueCXXInlineStaticDataMemberDefinition(
                parser, str, member, initializer, is_inline_member);
          }
          member_decl_had_inline_body |= has_inline_body;
        } else {
          // Regular member, align the member to the appropriate boundary.
          AlignNextOffset(str, member_symbol->type);
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
    if (member != NULL && member->is_member_function && func != NULL &&
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
        member->is_member_function || StructMemberIsNestedType(member)) {
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
        member->is_member_function) {
      continue;
    }
    if (CXXDestructibleElementType(member->symbol->type) != NULL) {
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
        member->is_member_function || member->default_initializer != NULL) {
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
         !member->is_member_function && !StructMemberIsNestedType(member);
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
  if (CompilerIsCXX()) {
    class_tag_scope = NewLocalSymbolTable();
    class_tag_scope->prev = parser->syntax->local_tag_stack;
    parser->syntax->local_tag_stack = class_tag_scope;
  }
  ParseStructMembers(parser, str, is_union, tag_name);
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
    TypeRecord* type = m->symbol->type;
    if (m->bit_size > 0) {
      // Bitfield: replicate ParseBitField's placement using the stored width.
      int word_width = type->size * 8;
      if (str->next_bit_pos + m->bit_size > word_width) {
        AlignNextOffset(str, type);
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
      AlignNextOffset(str, type);
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
  Vector* specialization_args = NULL;
  Symbol* tag = NULL;
  Symbol* specialization_template = NULL;
  String specialization_name;
  LocalSymbolTable* saved_specialization_tag_stack = NULL;
  Namespace* saved_specialization_namespace = NULL;
  bool using_specialization_namespace = false;
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
  SyntaxParseCXXAttributes(parser->syntax, &attributes);
  if (parser->syntax->parsing_template_specialization &&
      specialization_args != NULL) {
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
      Vector* completed_args = CompleteClassTemplateArguments(
          parser, specialization_template->type->info.struct_info,
          specialization_args);
      if (completed_args != NULL) {
        AppendTemplateInstantiationName(&specialization_name,
                                        specialization_template,
                                        completed_args);
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
        VectorDeleteWithContents(
            completed_args,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
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
  if (tag != NULL) {
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
      if (LexMatch(parser->lex, TOK(equal))) {
        ASTNode* value =
            SyntaxParseSingleExpression(parser->syntax, TC(semicolon));
        value = AnalyzeExpression(value);
        int64_t next_value = e->next_value;
        if (!EvaluateIntegerExpression(value, &next_value)) {
          SyntaxError(parser->syntax,
                      "Constant integer expression required for value of "
                      "enum constant %s",
                      const_name.value);
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
  // Prevent knock-on errors due to unknown symbols.
  if ((t1->type & kTypeUnknown) != 0 || (t2->type & kTypeUnknown) != 0) {
    return true;
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
    case kDeclPrimitive:
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

