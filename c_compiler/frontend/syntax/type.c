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
#include "symbol_table.h"
#include "syntax.h"
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
      VectorDestructWithContents(&record->info.function.prototype,
                                (VectorElementDestructor)SymbolDestruct, /*free_element=*/true);
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

// Copy a type record and chain it to its existing next,
// incrementing the ref count.
TypeRecord* TypeRecordCopy(TypeRecord* record) {
  TypeRecord* r = TypeArenaAlloc();
  memcpy(r, record, sizeof(TypeRecord));
  r->id = next_type_id;
  r->refs = 0;  // No refs to this yet.
  if (r->next != NULL) {
    TypeRecordIncRef(r->next);  // Another ref to next.
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
  t->info.function.is_virtual = false;
  t->info.function.is_override = false;
  t->info.function.is_final = false;
  t->info.function.is_pure_virtual = false;
  t->info.function.virtual_index = -1;
  t->info.function.cxx_member_owner = NULL;
  t->info.function.old_style = false;
  t->info.function.is_inline = false;
  t->info.function.body = NULL;
  VectorInit(&t->info.function.prototype);
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
  mem->byte_offset = 0;
  mem->bit_offset = 0;
  mem->bit_size = 0;
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
  free(param);
}

void TemplateArgumentDelete(TemplateArgument* arg) {
  if (arg == NULL) {
    return;
  }
  TypeRecordDelete(arg->type);
  free(arg);
}

static CXXBaseSpecifier* NewCXXBaseSpecifier(TypeRecord* type,
                                             CXXAccess access) {
  CXXBaseSpecifier* base = malloc(sizeof(CXXBaseSpecifier));
  base->type = type;
  TypeRecordIncRef(type);
  base->access = access;
  base->byte_offset = 0;
  return base;
}

static void CXXBaseSpecifierDelete(CXXBaseSpecifier* base) {
  TypeRecordDelete(base->type);
  free(base);
}

void StructMemberDelete(StructMember* member) {
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
  VectorInit(&s->members);
  VectorInit(&s->virtual_members);
  VectorInit(&s->template_parameters);
  s->vptr_member = NULL;
  s->vtable_symbol = NULL;
  MapInit(&s->symbol_table, CompareStructMember);
  s->is_union = is_union;
  s->is_class = false;
  s->is_template = false;
  s->template_parameter_count = 0;
  s->next_offset = 0;
  s->current_offset = 0;
  s->size = 0;
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
  VectorDestructWithContents(&s->members,
                             (VectorElementDestructor)StructMemberDelete, /*free_element=*/false);
  VectorDestruct(&s->virtual_members);
  VectorDestructWithContents(&s->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  MapDestruct(&s->symbol_table);
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
    {kTypeEnum, "enum"},          {kTypeImplicit, ""},
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
  switch (type->declarator) {
    case kDeclPrimitive:
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
      if (type->next->declarator == kDeclArray ||
          type->next->declarator == kDeclFunction) {
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
  parser->context = context;
  parser->cxx_member_owner = NULL;
  parser->cxx_member_definition = NULL;
}

void TypeParserReset(TypeParser* parser) {
  parser->symbol = NULL;
  parser->storage = STO(implicit);
  parser->found_void = false;
  parser->dimension_count = 0;
  parser->cxx_member_owner = NULL;
  parser->cxx_member_definition = NULL;
  VectorDestruct(&parser->stack);
  VectorInit(&parser->stack);
}

void TypeParserDestruct(TypeParser* parser) {
  // Only frees the stack's backing array.  Any TypeRecords still referenced by
  // the stack are owned elsewhere (the combined result type) and must not be
  // freed here.
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
static void UpdateStructSize(Struct* str, TypeRecord* member_type,
                             bool is_union);
static void FinalizeStructAlignment(Struct* str);

static TypeRecord* SubstituteTemplateParameters(TypeRecord* type, Vector* args) {
  if (type == NULL) {
    return NULL;
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

  TypeRecord* copy = TypeRecordCopy(type);
  if (copy->declarator == kDeclArray &&
      type->info.array.template_parameter_index >= 0) {
    int index = type->info.array.template_parameter_index;
    if ((size_t)index < args->length) {
      TemplateArgument* arg = args->value.p[index];
      if (arg->kind == kTemplateParameterNonType) {
        copy->info.array.size.fixed = (int)arg->int_value;
        copy->info.array.template_parameter_index = -1;
        copy->size = 0;
      }
    }
  }
  if (copy->next != NULL) {
    TypeRecordDelete(copy->next);
    copy->next = SubstituteTemplateParameters(type->next, args);
    if (copy->next != NULL) {
      TypeRecordIncRef(copy->next);
      copy->type = copy->next->type;
    }
  }
  return TypeRecordCalculateSize(copy);
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
    if (arg->kind == kTemplateParameterType) {
      TypeRecordToTemplateKeyString(arg->type, &arg_name);
    } else {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "%lld", arg->int_value);
      StringAppend(&arg_name, buffer);
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
    if (member->is_member_function || member->is_static || member->is_anon ||
        StructMemberIsBitField(member)) {
      SyntaxError(parser->syntax,
                  "Class template instantiation is not supported yet");
      return false;
    }
  }
  return true;
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
  if (args == NULL ||
      args->length != (size_t)template_struct->template_parameter_count) {
    SyntaxError(parser->syntax, "Class template instantiation is not supported yet");
    return TypeRecordCopy(templ->type);
  }
  for (size_t i = 0; i < template_struct->template_parameters.length; i++) {
    TemplateParameter* param = template_struct->template_parameters.value.p[i];
    TemplateArgument* arg = args->value.p[i];
    if (param->kind != arg->kind) {
      SyntaxError(parser->syntax,
                  "Class template instantiation is not supported yet");
      return TypeRecordCopy(templ->type);
    }
  }

  String instantiated_name;
  StringInit(&instantiated_name, NULL);
  AppendTemplateInstantiationName(&instantiated_name, templ, args);
  Symbol* existing =
      FindTemplateInstantiationTag(parser, templ, &instantiated_name);
  if (existing != NULL && existing->type != NULL &&
      TypeIsStructOrUnion(existing->type) &&
      existing->type->info.struct_info != NULL &&
      !existing->type->info.struct_info->is_template) {
    StringDestruct(&instantiated_name);
    return TypeRecordCopy(existing->type);
  }
  if (existing != NULL) {
    SyntaxError(parser->syntax,
                "Class template instantiation conflicts with existing tag %s",
                instantiated_name.value);
    StringDestruct(&instantiated_name);
    return TypeRecordCopy(templ->type);
  }
  if (!ClassTemplateInstantiationMembersSupported(parser, template_struct)) {
    StringDestruct(&instantiated_name);
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

  for (size_t i = 0; i < template_struct->members.length; i++) {
    StructMember* member = template_struct->members.value.p[i];
    TypeRecord* member_type =
        SubstituteTemplateParameters(member->symbol->type, args);
    TypeRecordCalculateSize(member_type);
    Symbol* member_symbol =
        NewSymbol(member->symbol->name.value, member_type, member->symbol->storage);
    member_symbol->location = member->symbol->location;
    StructMember* instantiated = NewStructMember(member_symbol);
    instantiated->access = member->access;
    AlignNextOffset(str, member_type);
    instantiated->byte_offset = str->next_offset;
    instantiated->index = str->members.length;
    AddStructMember(parser, str, instantiated);
    UpdateStructSize(str, member_type, str->is_union);
  }
  FinalizeStructAlignment(str);
  TypeRecordCalculateSize(type);
  StringDestruct(&instantiated_name);
  return TypeRecordCopy(type);
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
    } else if (allow_typedef && SyntaxCurrentTokenStartsQualifiedName(parser->syntax)) {
      FullyQualifiedIdentifier typedef_name;
      FullyQualifiedIdentifierInit(&typedef_name);
      SyntaxParseFullyQualifiedIdentifier(parser->syntax, &typedef_name);
      Symbol* symbol = SyntaxFindQualifiedSymbol(parser->syntax, &typedef_name);
      if (symbol != NULL && StorageIs(symbol->storage, STO(typedef))) {
        Vector* args = NULL;
        if (symbol->flags.is_template) {
          args = SyntaxParseTemplateArgumentList(parser->syntax, TC(decl));
        }
        if (symbol->flags.is_template && args != NULL &&
            !parser->syntax->parsing_template_declaration &&
            TypeIsStructOrUnion(symbol->type)) {
          type_record = InstantiateSimpleClassTemplate(parser, symbol, args);
        } else {
          type_record = TypeRecordCopy(symbol->type);
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
          if (symbol->flags.is_template) {
            args = SyntaxParseTemplateArgumentList(parser->syntax, TC(decl));
          }
          if (symbol->flags.is_template && args != NULL &&
              !parser->syntax->parsing_template_declaration &&
              TypeIsStructOrUnion(symbol->type)) {
            type_record = InstantiateSimpleClassTemplate(parser, symbol, args);
          } else {
            type_record = TypeRecordCopy(symbol->type);
          }
          type |= type_record->type;
          if (args != NULL) {
            VectorDestructWithContents(args,
                                       (VectorElementDestructor)TemplateArgumentDelete,
                                       /*free_element=*/false);
          }
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
  result.type = t1->type | t2->type;
  
  bool type_ok = result.type == kTypeImplicit ||
              (t1->type & t2->type) == 0;
  if (type_ok) {
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
  if (t1->type_record != NULL || t2->type_record != NULL) {
    // Either t1->typedef_record or t2->typedef_record is non-NULL.
    // Put the non-NULL one in t1 to avoid code duplication.
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
    result.type = t1->type;
    if (!IsValidQualiferCombo(t1->quals, t2->quals)) {
      QualifierComboError(syntax, t1->quals, t2->quals);
    }
    result.quals = t1->quals | t2->quals;
    result.type_record = t1->type_record;
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
  TypeParserParsePointer(parser);

  // Join all the type records together in reverse order.
  size_t i = parser->stack.length;
  TypeRecord* t = parser->base_type;
  while (i > 0) {
    TypeRecord* record = (TypeRecord*)parser->stack.value.p[i - 1];
    TypeRecordChain(record, t);
    record->type = t->type;
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
  return parser->symbol;
}

// Skip any __attribute__((...)) specifiers (a GCC extension) that can appear
// in declarator positions (pointers, parenthesized declarators, type names).
// The attributes are parsed and discarded.  Returns true if at least one was
// seen.
bool TypeParserSkipAttributes(TypeParser* parser) {
  bool any = false;
  while (LexMatch(parser->lex, TOK(attribute))) {
    Vector attrs = {0};
    VectorInit(&attrs);
    SyntaxParseAttribute(parser->syntax, &attrs);
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
    } else if (LexLookingAt(parser->lex, TOK(attribute))) {
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

static void ParseFunctionDecl(TypeParser* parser) {
  TypeParser proto_parser;
  TypeParserInit(&proto_parser, parser->lex, parser->syntax, STO(auto), kParsingPrototype);
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_inline = parser->is_inline;
  if (parser->symbol != NULL) {
    func->info.function.symbol = parser->symbol;
  }
  
  ParseFunctionPrototype(&proto_parser, func);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
  func->info.function.is_const_member = LexMatch(parser->lex, TOK(const));
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

void TypeParserParseFuncOrArray(TypeParser* parser) {
  TypeParserParseBase(parser);
  while (parser->syntax->found_open_paren ||
         LexLookingAt(parser->lex, TOK(lparen)) ||
         LexLookingAt(parser->lex, TOK(lsquare))) {
    if (CompilerIsCXX() && parser->context == kParsingBlockScope &&
        parser->symbol != NULL && parser->stack.length == 0 &&
        TypeIsStructOrUnion(parser->base_type) &&
        LexLookingAt(parser->lex, TOK(lparen))) {
      // In a local declaration like `T obj(args);`, the parens are direct
      // initialization of `obj`, not a function declarator.
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

static bool QualifiedNameIsSpecialMember(Symbol* owner,
                                         FullyQualifiedIdentifier* name,
                                         bool* is_destructor) {
  const char* member_name = FullyQualifiedIdentifierLast(name);
  *is_destructor = member_name[0] == '~';
  const char* class_name = owner->name.value;
  if (*is_destructor) {
    return strcmp(member_name + 1, class_name) == 0;
  }
  return strcmp(member_name, class_name) == 0;
}

Symbol* TypeParserParseCXXSpecialMemberDeclarator(TypeParser* parser) {
  SourceLocation location = parser->lex->current_token_location;
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  if (!SyntaxParseFullyQualifiedIdentifier(parser->syntax, &name)) {
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
  if (!QualifiedNameIsSpecialMember(owner, &name, &is_destructor)) {
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(&name));
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
  func->info.function.is_constructor = !is_destructor;
  func->info.function.is_destructor = is_destructor;
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  ParseFunctionPrototype(&proto_parser, func);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
  if (LexMatch(parser->lex, TOK(const))) {
    SyntaxError(parser->syntax, "Constructors and destructors cannot be const");
  }
  TypeParserDestruct(&proto_parser);
  TypeRecordAddCXXThisParameter(func, parser->cxx_member_owner, location);

  Symbol* sym = NewSymbol(member_name.value, func, STO(implicit));
  sym->location = location;
  func->info.function.symbol = sym;
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
      if (!SyntaxParseFullyQualifiedIdentifier(parser->syntax, &name)) {
        FullyQualifiedIdentifierDestruct(&name);
        return;
      }
      ResolveQualifiedMemberDeclarator(parser, &name);
      parser->symbol =
          NewSymbol(FullyQualifiedIdentifierLast(&name), parser->base_type,
                    parser->storage);
      parser->symbol->location = location;
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

static StructMember* FindStructMemberWithAccessFromBase(Struct* str,
                                                        String* name,
                                                        CXXAccess inherited,
                                                        CXXAccess* access,
                                                        Struct** owner) {
  StructMember* member = MapFindPointerKey(&str->symbol_table, name);
  if (member != NULL) {
    if (access != NULL) {
      *access = CombineInheritedAccess(inherited, member->access);
    }
    if (owner != NULL) {
      *owner = str;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberWithAccessFromBase(
          base->type->info.struct_info, name,
          CombineInheritedAccess(inherited, base->access), access, owner);
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
  StructMember* member = MapFindPointerKey(&str->symbol_table, name);
  if (member != NULL) {
    if (access != NULL) {
      *access = member->access;
    }
    if (owner != NULL) {
      *owner = str;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberWithAccessFromBase(
          base->type->info.struct_info, name, base->access, access, owner);
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
    if (TypeEqual(overload->symbol->type, type)) {
      return overload;
    }
  }
  return NULL;
}

static bool CheckStructMember(Struct* str, String* name) {
  return MapFindPointerKey(&str->symbol_table, name) == NULL;
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
    bool keep_base = true;
    if (bases->length > 0) {
      SyntaxError(parser->syntax,
                  "multiple inheritance is not supported yet");
      keep_base = false;
    }
    if (LexMatch(parser->lex, TOK(virtual))) {
      SyntaxError(parser->syntax, "virtual base classes are not supported yet");
    }
    CXXAccess access = ParseBaseAccess(parser, is_class);
    if (LexMatch(parser->lex, TOK(virtual))) {
      SyntaxError(parser->syntax, "virtual base classes are not supported yet");
    }
    TypeRecord* base_type = TypeParserParseType(parser, true);
    if (base_type == NULL || !TypeIsStructOrUnion(base_type) ||
        base_type->info.struct_info == NULL) {
      SyntaxError(parser->syntax, "base class must be a class or struct type");
      TypeRecordDelete(base_type);
      continue;
    }
    TypeRecordCalculateSize(base_type);
    if (keep_base) {
      VectorAppend(bases, NewCXXBaseSpecifier(base_type, access));
    }
    TypeRecordDelete(base_type);
  } while (LexMatch(parser->lex, TOK(comma)));
}

static void LayoutCXXBaseSpecifiers(Struct* str) {
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
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

static TypeRecord* NewCXXVPtrType(void) {
  TypeRecord* entry_type = NewCXXVTableEntryType();
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
  MapKeyValue kv;
  kv.key.p = &member->symbol->name;
  kv.value.p = member;
  MapInsert(&str->symbol_table, kv);
}

static TypeRecord* NewCXXVTableType(size_t slots) {
  TypeRecord* entry_type = NewCXXVTableEntryType();
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

static void RegisterCXXVTable(TypeParser* parser, Struct* str) {
  if (!CompilerIsCXX() || str->vtable_symbol != NULL ||
      str->virtual_members.length == 0 || str->tag_name == NULL) {
    return;
  }

  String name;
  StringInit(&name, "__davecc_vtbl_");
  StringAppendString(&name, str->tag_name);
  Symbol* symbol = NewSymbol(name.value,
                             NewCXXVTableType(str->virtual_members.length),
                             STO(static));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = parser->lex->current_token_location;
  SyntaxAddSymbol(parser->syntax, symbol);
  str->vtable_symbol = symbol;
  StringDestruct(&name);

  InitializedStaticVariable* var = malloc(sizeof(InitializedStaticVariable));
  var->symbol = symbol;
  var->is_global = false;
  var->size = symbol->type->size;
  var->alignment = TypeRecordAlignment(symbol->type->next);
  VectorInit(&var->initializers);
  var->is_tls = false;
  var->is_local = false;
  for (size_t i = 0; i < str->virtual_members.length; i++) {
    StructMember* member = str->virtual_members.value.p[i];
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
      init->value.symbol = member->symbol;
    }
    VectorAppend(&var->initializers, init);
  }
  VectorAppend(&compiler->initialized_static_variables, var);
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
  if (!LexLookingAt(parser->lex, TOK(number)) || parser->lex->number != 0) {
    SyntaxError(parser->syntax, "pure virtual specifier must be '= 0'");
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
  }
  VectorAppend(&str->members, member);
  MapKeyValue kv;
  kv.key.p = &member->symbol->name;
  kv.value.p = member;
  MapInsert(&str->symbol_table, kv);
}

static bool CanOverloadStructMember(StructMember* existing,
                                    StructMember* member) {
  return CompilerIsCXX() && existing != NULL && member != NULL &&
         existing->is_member_function && member->is_member_function;
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

static bool ParseClassSpecialMember(TypeParser* parser, Struct* str,
                                    String* class_name, CXXAccess access,
                                    bool is_virtual) {
  if (!CompilerIsCXX() || class_name->length == 0) {
    return false;
  }

  bool is_destructor = LexMatch(parser->lex, TOK(tilde));
  if (!LexLookingAt(parser->lex, TOK(identifier)) ||
      !StringEqualString(&parser->lex->spelling, class_name)) {
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
  SkipInlineMemberFunctionBody(parser);
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

      MapKeyValue kv;
      kv.key.p = &symbol->name;
      kv.value.p = dest_member;
      MapInsert(&dest->symbol_table, kv);

      if (!src->is_union) {
        // Members of an anonymous struct are laid out sequentially regardless
        // of whether the enclosing aggregate is a union, so always advance.
        UpdateStructSize(dest, symbol->type, false);
      }
    }
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

    bool is_virtual_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(virtual));
    if (ParseClassSpecialMember(parser, str, tag_name, current_access,
                                is_virtual_member)) {
      if (!LexLookingAt(parser->lex, TOK(rbrace))) {
        LexMatch(parser->lex, TOK(semicolon));
      }
      continue;
    }

    bool is_static_member = LexMatch(parser->lex, TOK(static));
    if (is_virtual_member && is_static_member) {
      SyntaxError(parser->syntax, "static member functions cannot be virtual");
    }
    bool possible_anon = LexLookingAt(parser->lex, TOK(union)) ||
            LexLookingAt(parser->lex, TOK(struct));
    TypeRecord* member_type = TypeParserParseType(parser, true);
    bool member_decl_had_inline_body = false;
    while (!LexEof(parser->lex)) {
      bool has_inline_body = false;
      if (possible_anon && LexLookingAt(parser->lex, TOK(semicolon))) {
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
        StructMember* member = NewStructMember(member_symbol);
        member->is_static = is_static_member;
        member->is_member_function = TypeIsFunction(member_symbol->type);
        member->access = current_access;
        if (member->is_member_function && !member->is_static) {
          member_symbol->type->info.function.is_virtual = is_virtual_member;
          TypeRecordAddCXXThisParameter(member_symbol->type, str,
                                        member_symbol->location);
        } else if (member->is_member_function) {
          member_symbol->type->info.function.cxx_member_owner = str;
        }
        if (member->is_member_function) {
          ParseCXXVirtSpecifiers(parser, member_symbol->type);
          ParseCXXPureSpecifier(parser, member_symbol->type);
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
        } else if (member->is_static || member->is_member_function) {
          has_inline_body = member->is_member_function &&
                            SkipInlineMemberFunctionBody(parser);
          member_decl_had_inline_body |= has_inline_body;
        } else {
          // Regular member, align the member to the appropriate boundary.
          AlignNextOffset(str, member_symbol->type);
          member->byte_offset = str->next_offset;
          member->index = str->members.length - 1;

          UpdateStructSize(str, member_symbol->type, is_union);
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
  }
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
  CopyCXXBaseVirtualMembers(str);
  LayoutCXXBaseSpecifiers(str);
  ParseStructMembers(parser, str, is_union, tag_name);
  UpdateCXXAbstractStatus(str);
  AddCXXVPtrMember(parser, str);
  RegisterCXXVTable(parser, str);
  
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
  while (LexMatch(parser->lex, TOK(attribute))) {
    SyntaxParseAttribute(parser->syntax, &attributes);
  }

  String tag_name = {0};
  FullyQualifiedIdentifier qualified_tag = {0};
  bool has_qualified_tag = false;
  Symbol* tag = NULL;
  FullyQualifiedIdentifierInit(&qualified_tag);

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
  if (tag != NULL) {
    parser->syntax->last_parsed_tag = tag;
  }
  FullyQualifiedIdentifierDestruct(&qualified_tag);
  StringDestruct(&tag_name);
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
  while (LexMatch(parser->lex, TOK(attribute))) {
    SyntaxParseAttribute(parser->syntax, &attributes);
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
      e->is_scoped = is_scoped;
      ApplyEnumUnderlyingType(parser->syntax, e, tag->type, explicit_underlying,
                              is_scoped);
      SyntaxAddTag(parser->syntax, tag);
      AddInjectedEnumName(parser, tag);
    } else {
      // Tag already exists, make sure it's the same tag type.
      CheckTagType(parser, tag, false, true);
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
    if (TypeIsDerivedFrom(from->next, to->next)) {
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

