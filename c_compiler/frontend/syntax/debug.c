//
//  debug.c
//  c_compiler_library
//
//  Created by David Allison on 4/5/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "debug.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "ast.h"
#include "compiler.h"
#include "asm_module.h"
#include "member_pointer.h"

static DIE* NewSymbolDIE(DebugBuilder* builder, Symbol* symbol, DW_TAG tag);
static DIE* NewMemberDIE(DebugBuilder* builder, StructMember* member);
static DIE* NewEnumConstDIE(DebugBuilder* builder, Symbol* c);
static COMPILER_UNUSED void DebugAbbreviationPrint(DebugAbbreviation* abbrev, int level);
static DIE* NewTypeRecordDIE(DebugBuilder* builder, TypeRecord* type);
static void DIEInit(DIE* die, DW_TAG tag, DIEVirtuals* virtuals,
                    bool has_children);
static void DIEBaseDestruct(DIE* die);
static void AddDIE(DebugBuilder* builder, DIE* die);
void DIEBuild(DebugBuilder* builder, DIE* die);

static int CompareSignatures(const void* a, const void* b) {
  const MapKeyValue* v1 = a;
  const MapKeyValue* v2 = b;
  return BufferCompare(v1->key.p, v2->key.p);
}

static void CompileUnitDIEBuild(DebugBuilder* builder, DIE* die);
static void CompileUnitDIEPrint(DIE* die, int level);
static void CompileUnitDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals compile_unit_virtuals = {DIEBaseDestruct, CompileUnitDIEBuild,
                                        CompileUnitDIEPrint, CompileUnitDIEEmit};

void DebugBuilderInit(DebugBuilder* builder,
                      const char* filename, const char* producer,
                      const char* dir) {
  MapInitForCharPointerKeys(&builder->tags);
  MapInit(&builder->abbreviation_map, CompareSignatures);
  VectorInit(&builder->abbreviations);
  BufferInit(&builder->string_table);
  builder->next_abbrev_number = 1;
  VectorInit(&builder->all_dies);
  VectorInit(&builder->top_dies);
  BufferInit(&builder->bytes);
  builder->fp = NULL;
  builder->module = NULL;
  
  // Build DW_TAG(compile_unit) DIE
  builder->compile_unit = malloc(sizeof(DIE));
  DIEInit(builder->compile_unit, DW_TAG(compile_unit), &compile_unit_virtuals, true);
  StringInit(&builder->filename, filename);
  builder->producer = producer;
  StringInit(&builder->dir, dir);
  AddDIE(builder, builder->compile_unit);
  VectorAppend(&builder->top_dies, builder->compile_unit);
}

static void DebugAbbreviationMapDestruct(MapKeyValue* kv) {
  BufferDelete(kv->key.p);
  free(kv->value.p);
}

void DebugBuilderDestruct(DebugBuilder* builder) {
  StringDestruct(&builder->filename);
  StringDestruct(&builder->dir);
  MapDestruct(&builder->tags);
  MapDestructWithContents(&builder->abbreviation_map,
                          DebugAbbreviationMapDestruct);
  VectorDestruct(&builder->abbreviations);
  BufferDestruct(&builder->string_table);
  VectorDestructWithContents(&builder->all_dies,
                             (VectorElementDestructor)DIEDestruct, /*free_element=*/true);
  VectorDestruct(&builder->top_dies);
  BufferDestruct(&builder->bytes);
}

static void AddDIE(DebugBuilder* builder, DIE* die) {
  die->id = (int)builder->all_dies.length + 100;
  VectorAppend(&builder->all_dies, die);
}

DIE* BuildDebugInfo(DebugBuilder* builder, Symbol* sym, DW_TAG tag) {
  DIE* die = NewSymbolDIE(builder, sym, tag);
  VectorAppend(&builder->top_dies, die);
  return die;
}

void BuildDebugInfoAfterCodegen(DebugBuilder* builder, Symbol* sym) {
  DIE* die = sym->die;
  assert(die != NULL);
  // printf("DIE\n");
  // DIEPrint(die, 1);
  die->virtuals->builder(builder, die);
}

DIE* BuildTypeDebugInfo(DebugBuilder* builder, TypeRecord* type) {
  DIE* die = NewTypeRecordDIE(builder, type);
  VectorAppend(&builder->top_dies, die);
  die->virtuals->builder(builder, die);
  return die;
}

void DIEBuild(DebugBuilder* builder, DIE* die) {
  assert(die != NULL);
  if (die->abbrev != NULL) {
    return;
  }
  die->virtuals->builder(builder, die);
}

void DIEPrint(DIE* die, int level) { die->virtuals->printer(die, level); }

static void BaseTypeDIEBuild(DebugBuilder* builder, DIE* die);
static void BaseTypeDIEPrint(DIE* die, int level);
static void BaseTypeDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals base_type_virtuals = {NULL, BaseTypeDIEBuild,
                                         BaseTypeDIEPrint, BaseTypeDIEEmit};

static bool DebugTypeIsPlainChar(TypeRecord* type) {
  return TypeIsChar(type) && !TypeIsSigned(type) && !TypeIsUnsigned(type);
}

static bool DebugTypeIsSignedChar(TypeRecord* type) {
  return TypeIsChar(type) && TypeIsSigned(type);
}

static bool DebugTypeIsSignedShort(TypeRecord* type) {
  return TypeIsShort(type) && !TypeIsUnsigned(type);
}

static bool DebugTypeIsSignedInt(TypeRecord* type) {
  return TypeIsInt(type) && !TypeIsUnsigned(type);
}

static bool DebugTypeIsSignedLong(TypeRecord* type) {
  return TypeIsLong(type) && !TypeIsUnsigned(type);
}

static bool DebugTypeIsSignedLongLong(TypeRecord* type) {
  return TypeIsLongLong(type) && !TypeIsUnsigned(type);
}

static bool DebugTypeIsSignedInt128(TypeRecord* type) {
  return TypeIsInt128(type) && !TypeIsUnsigned(type);
}

static bool DebugTypeIsAuto(TypeRecord* type) {
  return type != NULL && type->declarator == kDeclPrimitive &&
         (type->type & kTypeAuto) != 0;
}

static BaseTypeDIE base_types[] = {
    {{0, DW_TAG(base_type), &base_type_virtuals},
     DebugTypeIsPlainChar,
     "char",
     DW_ATE(signed_char),
     1},
    {{1, DW_TAG(base_type), &base_type_virtuals},
     DebugTypeIsSignedShort,
     "short",
     DW_ATE(signed),
     2},
    {{2, DW_TAG(base_type), &base_type_virtuals},
     DebugTypeIsSignedInt,
     "int",
     DW_ATE(signed),
     4},
    {{3, DW_TAG(base_type), &base_type_virtuals},
     DebugTypeIsSignedLong,
     "long",
     DW_ATE(signed),
     8},
    {{4, DW_TAG(base_type), &base_type_virtuals},
     TypeIsUnsignedChar,
     "unsigned char",
     DW_ATE(unsigned_char),
     1},
    {{5, DW_TAG(base_type), &base_type_virtuals},
     TypeIsUnsignedShort,
     "unsigned short",
     DW_ATE(unsigned),
     2},
    {{6, DW_TAG(base_type), &base_type_virtuals},
     TypeIsUnsignedInt,
     "unsigned int",
     DW_ATE(unsigned),
     4},
    {{7, DW_TAG(base_type), &base_type_virtuals},
     TypeIsUnsignedLong,
     "unsigned long",
     DW_ATE(unsigned),
     8},
    {{8, DW_TAG(base_type), &base_type_virtuals},
     TypeIsFloat,
     "float",
     DW_ATE(float),
     4},
    {{9, DW_TAG(base_type), &base_type_virtuals},
     TypeIsDouble,
     "double",
     DW_ATE(float),
     8},
    {{10, DW_TAG(base_type), &base_type_virtuals},
     TypeIsBool,
     "bool",
     DW_ATE(boolean),
     1},
    {{11, DW_TAG(unspecified_type), &base_type_virtuals}, TypeIsVoid, "void"},
    {{12, DW_TAG(base_type), &base_type_virtuals},
     TypeIsChar8,
     "char8_t",
     DW_ATE(UTF),
     1},
    {{13, DW_TAG(base_type), &base_type_virtuals},
     TypeIsChar16,
     "char16_t",
     DW_ATE(UTF),
     2},
    {{14, DW_TAG(base_type), &base_type_virtuals},
     TypeIsChar32,
     "char32_t",
     DW_ATE(UTF),
     4},
    {{15, DW_TAG(base_type), &base_type_virtuals},
     TypeIsFloat32,
     "std::float32_t",
     DW_ATE(float),
     4},
    {{16, DW_TAG(base_type), &base_type_virtuals},
     TypeIsFloat64,
     "std::float64_t",
     DW_ATE(float),
     8},
    {{17, DW_TAG(base_type), &base_type_virtuals},
     DebugTypeIsSignedChar,
     "signed char",
     DW_ATE(signed_char),
     1},
    {{18, DW_TAG(base_type), &base_type_virtuals},
     DebugTypeIsSignedLongLong,
     "long long",
     DW_ATE(signed),
     8},
    {{19, DW_TAG(base_type), &base_type_virtuals},
     TypeIsUnsignedLongLong,
     "unsigned long long",
     DW_ATE(unsigned),
     8},
    {{23, DW_TAG(base_type), &base_type_virtuals},
     DebugTypeIsSignedInt128,
     "__int128",
     DW_ATE(signed),
     16},
    {{24, DW_TAG(base_type), &base_type_virtuals},
     TypeIsUnsignedInt128,
     "unsigned __int128",
     DW_ATE(unsigned),
     16},
    {{25, DW_TAG(base_type), &base_type_virtuals},
     TypeIsWchar,
     "wchar_t",
     DW_ATE(signed),
     4},
    {{20, DW_TAG(base_type), &base_type_virtuals},
     TypeIsLongDouble,
     "long double",
     DW_ATE(float),
     8},
    {{21, DW_TAG(base_type), &base_type_virtuals},
     TypeIsNullPointer,
     "std::nullptr_t",
     DW_ATE(address),
     8},
    {{22, DW_TAG(unspecified_type), &base_type_virtuals},
     DebugTypeIsAuto,
     "auto",
     0,
     0},
};

#define NUM_BASE_TYPES (sizeof(base_types) / sizeof(BaseTypeDIE))

static DIE* PrimitiveToDIE(TypeRecord* type) {
  for (int i = 0; i < NUM_BASE_TYPES; i++) {
    if (base_types[i].func(type)) {
      if (!TypeIsVoid(type) && type->size > 0) {
        base_types[i].byte_size = type->size;
      }
      return &base_types[i].die;
    }
  }
  for (int i = 0; i < NUM_BASE_TYPES; i++) {
    if (base_types[i].func == DebugTypeIsAuto) {
      return &base_types[i].die;
    }
  }
  return &base_types[0].die;
}

static DIE* FindBaseType(bool (*func)(TypeRecord*)) {
  for (int i = 0; i < NUM_BASE_TYPES; i++) {
    if (base_types[i].func == func) {
      return &base_types[i].die;
    }
  }
  assert(false);
  return NULL;
}

static void DIEBaseDestruct(DIE* die) {
  VectorDestructWithContents(
      &die->attr_values, (VectorElementDestructor)DebugAttributeValueDestruct, /*free_element=*/true);
}

void DIEDestruct(DIE* die) {
  if (die == NULL) {
    return;
  }
  if (die->virtuals->destructor != NULL) {
    die->virtuals->destructor(die);
  }
}

void NamedDIEDestruct(DIE* die) {
  if (die == NULL) {
    return;
  }
  NamedDIE* named_die = (NamedDIE*)die;
  StringDestruct(&named_die->name);
  DIEBaseDestruct(die);
}

static void DIEInit(DIE* die, DW_TAG tag, DIEVirtuals* virtuals,
                    bool has_children) {
  die->tag = tag;
  die->virtuals = virtuals;
  VectorInit(&die->attr_values);
  die->has_children = has_children;
  die->abbrev = NULL;
  die->id = -1;
  die->emitted = false;
}

static void NamedDIEInit(NamedDIE* die, DW_TAG tag, DIEVirtuals* virtuals,
                         const char* name, bool has_children) {
  DIEInit(&die->die, tag, virtuals, has_children);
  StringInit(&die->name, name);
}

static void SubrangeDIEBuild(DebugBuilder* builder, DIE* die);
static void SubrangeDIEPrint(DIE* die, int level);
static void SubrangeDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals subrange_virtuals = {DIEBaseDestruct, SubrangeDIEBuild,
                                        SubrangeDIEPrint, SubrangeDIEEmit};

static DIE* NewSubrangeDIE(DebugBuilder* builder, int upper_bound) {
  SubrangeDIE* s = malloc(sizeof(SubrangeDIE));
  DIEInit(&s->die, DW_TAG(subrange_type), &subrange_virtuals, false);
  s->upper_bound = upper_bound;
  s->type = FindBaseType(TypeIsUnsignedLong);  // Is this right?
  AddDIE(builder, &s->die);
  return &s->die;
}

static void ArrayDIEDestruct(DIE* die) { DIEBaseDestruct(die); }

static void ArrayDIEBuild(DebugBuilder* builder, DIE* die);
static void ArrayDIEPrint(DIE* die, int level);
static void ArrayDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals array_virtuals = {ArrayDIEDestruct, ArrayDIEBuild,
                                     ArrayDIEPrint, ArrayDIEEmit};

static DIE* NewArrayDIE(DebugBuilder* builder, int num_elements, DIE* subtype) {
  ArrayTypeDIE* array = malloc(sizeof(ArrayTypeDIE));
  DIEInit(&array->die, DW_TAG(array_type), &array_virtuals, true);
  array->subrange = NewSubrangeDIE(builder, num_elements);
  array->subtype = subtype;
  AddDIE(builder, &array->die);
  return &array->die;
}

static void PointerDIEBuild(DebugBuilder* builder, DIE* die);
static void PointerDIEPrint(DIE* die, int level);
static void PointerDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals pointer_virtuals = {DIEBaseDestruct, PointerDIEBuild,
                                       PointerDIEPrint, PointerDIEEmit};

static DIE* NewPointerDIE(DebugBuilder* builder, DIE* subtype) {
  PointerTypeDIE* ptr = malloc(sizeof(PointerTypeDIE));
  DIEInit(&ptr->die, DW_TAG(pointer_type), &pointer_virtuals, false);
  ptr->subtype = subtype;
  AddDIE(builder, &ptr->die);
  return &ptr->die;
}

static void PtrToMemberDIEBuild(DebugBuilder* builder, DIE* die);
static void PtrToMemberDIEPrint(DIE* die, int level);
static void PtrToMemberDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals ptr_to_member_virtuals = {
    DIEBaseDestruct, PtrToMemberDIEBuild, PtrToMemberDIEPrint, PtrToMemberDIEEmit};

static DIE* NewPtrToMemberDIE(DebugBuilder* builder, DIE* member_type,
                              DIE* containing_type, int byte_size) {
  PtrToMemberTypeDIE* ptr = malloc(sizeof(PtrToMemberTypeDIE));
  DIEInit(&ptr->die, DW_TAG(ptr_to_member_type), &ptr_to_member_virtuals, false);
  ptr->member_type = member_type;
  ptr->containing_type = containing_type;
  ptr->byte_size = byte_size;
  AddDIE(builder, &ptr->die);
  return &ptr->die;
}

static void LexicalScopeDestruct(DIE* die) {
  LexicalScopeDIE* scope = (LexicalScopeDIE*)die;
  VectorDestruct(&scope->variables);
  VectorDestruct(&scope->lexical_scopes);
  StringDestruct(&scope->low_pc_label);
  StringDestruct(&scope->high_pc_expr);
  DIEBaseDestruct(die);
}

static void LexicalScopeDIEBuild(DebugBuilder* builder, DIE* die);
static void LexicalScopeDIEPrint(DIE* die, int level);
static void LexicalScopeDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals lexical_scope_virtuals = {
    LexicalScopeDestruct, LexicalScopeDIEBuild, LexicalScopeDIEPrint,
    LexicalScopeDIEEmit};

static LexicalScopeDIE* NewLexicalScope(DebugBuilder* builder, DW_TAG tag,
                                        LexicalScopeDIE* parent,
                                        LabelASTNode* low_pc,
                                        LabelASTNode* high_pc) {
  LexicalScopeDIE* scope = malloc(sizeof(LexicalScopeDIE));
  DIEInit(&scope->die, tag, &lexical_scope_virtuals, true);
  VectorInit(&scope->variables);
  VectorInit(&scope->lexical_scopes);
  StringInit(&scope->low_pc_label,
             low_pc == NULL ? NULL : low_pc->name.value);
  StringInit(&scope->high_pc_expr, NULL);
  if (low_pc != NULL && high_pc != NULL) {
    StringPrintf(&scope->high_pc_expr, "%s-%s", high_pc->name.value,
                 low_pc->name.value);
  }
  scope->parent = parent;
  AddDIE(builder, &scope->die);
  return scope;
}

static void FunctionDIEDestruct(DIE* die) { DIEBaseDestruct(die); }

static void FunctionDIEBuild(DebugBuilder* builder, DIE* die);
static void FunctionDIEPrint(DIE* die, int level);
static void FunctionDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals function_virtuals = {FunctionDIEDestruct, FunctionDIEBuild,
                                        FunctionDIEPrint, FunctionDIEEmit};

static DIE* NewFunctionDIE(DebugBuilder* builder, DIE* subtype,
                           const char* name, const char* linkage_name,
                           bool is_external, const char* code_name,
                           LabelASTNode* low_pc, LabelASTNode* high_pc) {
  FunctionTypeDIE* func = malloc(sizeof(FunctionTypeDIE));
  NamedDIEInit(&func->die, DW_TAG(subprogram), &function_virtuals, name, true);
  func->subtype = subtype;
  func->linkage_name = linkage_name;
  func->is_external = is_external;
  func->top_scope = NewLexicalScope(builder, 0, NULL, low_pc, high_pc);
  if (code_name != NULL && code_name[0] != '\0') {
    StringSet(&func->top_scope->low_pc_label, code_name);
    StringClear(&func->top_scope->high_pc_expr);
    StringPrintf(&func->top_scope->high_pc_expr, ".func_end_%s-%s",
                 code_name, code_name);
  }
  AddDIE(builder, &func->die.die);
  return &func->die.die;
}

static DIE* NewFunctionTypeDIE(DebugBuilder* builder, DIE* subtype) {
  FunctionTypeDIE* func = malloc(sizeof(FunctionTypeDIE));
  NamedDIEInit(&func->die, DW_TAG(subroutine_type), &function_virtuals, NULL,
               true);
  func->subtype = subtype;
  func->linkage_name = NULL;
  func->is_external = false;
  func->top_scope = NewLexicalScope(builder, 0, NULL, NULL, NULL);
  AddDIE(builder, &func->die.die);
  return &func->die.die;
}

static bool DebugLabelIsReady(const LabelASTNode* label) {
  return label != NULL && label->name.value != NULL &&
         label->name.length != 0 &&
         (label->name.capacity == STRING_IMMUTABLE ||
          label->name.length < label->name.capacity);
}

static void StructDIEDestruct(DIE* die) {
  StructDIE* s = (StructDIE*)die;
  VectorDestruct(&s->members);
  NamedDIEDestruct(die);
}

static void StructDIEBuild(DebugBuilder* builder, DIE* die);
static void StructDIEPrint(DIE* die, int level);
static void StructDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals struct_virtuals = {StructDIEDestruct, StructDIEBuild,
                                      StructDIEPrint, StructDIEEmit};

static DIE* NewStructDIE(DebugBuilder* builder, TypeRecord* type) {
  StructDIE* die = malloc(sizeof(StructDIE));
  NamedDIEInit(&die->die,
               type->info.struct_info->is_union ? DW_TAG(union_type)
                                                : DW_TAG(structure_type),
               &struct_virtuals, type->info.struct_info->tag_name->value, true);
  StringInit(&die->die.name, type->info.struct_info->tag_name->value);
  VectorInit(&die->members);
  die->byte_size = type->size;
  AddDIE(builder, (DIE*)die);
  return (DIE*)die;
}

static void EnumDIEDestruct(DIE* die) {
  EnumDIE* s = (EnumDIE*)die;
  VectorDestruct(&s->enumerators);
  NamedDIEDestruct(die);
}

static void EnumDIEBuild(DebugBuilder* builder, DIE* die);
static void EnumDIEPrint(DIE* die, int level);
static void EnumDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals enum_virtuals = {EnumDIEDestruct, EnumDIEBuild, EnumDIEPrint,
                                    EnumDIEEmit};

static DIE* NewEnumDIE(DebugBuilder* builder, TypeRecord* type) {
  EnumDIE* die = malloc(sizeof(EnumDIE));
  NamedDIEInit(&die->die, DW_TAG(enumeration_type), &enum_virtuals,
               type->info.enum_info->tag_name->value, true);
  VectorInit(&die->enumerators);
  die->byte_size = type->size > 0 ? type->size : 4;
  AddDIE(builder, (DIE*)die);
  return (DIE*)die;
}

static DIE* FindTag(DebugBuilder* builder, const char* name) {
  MapKeyType key = {.p = (void*)name};
  return MapFind(&builder->tags, key);
}

typedef struct {
  DebugBuilder* builder;
  LexicalScopeDIE* current;
  int level;
} LexicalScopeBuilder;

static void LexicalScopeVisitor(ASTNode* node, void* data, int child_id,
                                VisitorMode mode) {
  LexicalScopeBuilder* builder = data;
  if (node->op == AST_OP(compound)) {
    CompoundStatementASTNode* compound =
        (CompoundStatementASTNode*)node;
    bool has_debug_range =
        DebugLabelIsReady(compound->low_pc) &&
        DebugLabelIsReady(compound->high_pc);
    if (mode == kVisitPreChildren) {
      if (builder->level > 0 && has_debug_range) {
        LexicalScopeDIE* new_scope =
            NewLexicalScope(builder->builder, DW_TAG(lexical_block),
                            builder->current, compound->low_pc,
                            compound->high_pc);

        VectorAppend(&builder->current->lexical_scopes, new_scope);
        builder->current = new_scope;
      }
      builder->level++;
    } else {
      builder->level--;
      if (builder->level > 0 && has_debug_range) {
        builder->current = builder->current->parent;
      }
    }
  } else if (node->op == AST_OP(vardecl)) {
    if (mode == kVisitPreChildren) {
      VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
      DIE* die = NewSymbolDIE(builder->builder, decl->symbol, DW_TAG(variable));
      VectorAppend(&builder->current->variables, die);
      decl->symbol->die = die;
    }
  }
}

static void BuildLexicalScopes(DebugBuilder* builder, TypeRecord* func_type,
                               FunctionTypeDIE* die) {
  LexicalScopeBuilder b = {builder, die->top_scope, 0};
  ASTNodeVisit(func_type->info.function.body, LexicalScopeVisitor, 0, &b);
}

static void CVDIEBuild(DebugBuilder* builder, DIE* die);
static void CVDIEPrint(DIE* die, int level);
static void CVDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals cv_virtuals = {DIEBaseDestruct, CVDIEBuild, CVDIEPrint,
                                  CVDIEEmit};

static DIE* NewTypeRecordDIE(DebugBuilder* builder, TypeRecord* type) {
  DIE* die = NULL;
  switch (type->declarator) {
    case kDeclArray:
      die = NewArrayDIE(builder, type->info.array.size.fixed,
                        NewTypeRecordDIE(builder, type->next));
      break;

    case kDeclVector:
      // Until DW_TAG_GNU_vector_type is emitted, describe the storage as a
      // fixed array so debuggers can still inspect individual lanes.
      die = NewArrayDIE(builder, type->info.array.size.fixed,
                        NewTypeRecordDIE(builder, type->next));
      break;

    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      die = NewPointerDIE(builder, NewTypeRecordDIE(builder, type->next));
      break;

    case kDeclMemberPointer: {
      Struct* class_info = type->info.struct_info;
      TypeRecord* class_type =
          NewTypeRecord(class_info->is_union ? kTypeUnion : kTypeStruct,
                        kQualPlain);
      TypeRecordSetStructInfo(class_type, class_info);
      die = NewPtrToMemberDIE(builder, NewTypeRecordDIE(builder, type->next),
                              NewTypeRecordDIE(builder, class_type),
                              MemberPointerSize(type));
      break;
    }

    case kDeclFunction: {
      CompoundStatementASTNode* body =
          (CompoundStatementASTNode*)type->info.function.body;
      DIE* return_type = NULL;
      if (!TypeIsVoid(type->next)) {
        return_type = NewTypeRecordDIE(builder, type->next);
      }
      if (body == NULL || body->base.op != AST_OP(compound) ||
          !DebugLabelIsReady(body->low_pc) ||
          !DebugLabelIsReady(body->high_pc)) {
        die = NewFunctionTypeDIE(builder, return_type);
      } else {
        Symbol* func_sym = type->info.function.symbol;
        const char* name = func_sym == NULL ? NULL : func_sym->name.value;
        const char* linkage = NULL;
        const char* code_name = name;
        bool is_external = false;
        if (func_sym != NULL) {
          if (func_sym->asm_name.value != NULL &&
              func_sym->asm_name.length > 0) {
            code_name = func_sym->asm_name.value;
            if (name == NULL || strcmp(func_sym->asm_name.value, name) != 0) {
              linkage = func_sym->asm_name.value;
            }
          }
          is_external = !StorageIs(func_sym->storage, STO(static));
        }
        die = NewFunctionDIE(builder, return_type, name, linkage, is_external,
                              code_name, body->low_pc, body->high_pc);
      }
      FunctionTypeDIE* func = (FunctionTypeDIE*)die;
      for (size_t i = 0; i < type->info.function.prototype.length; i++) {
        Symbol* formal = (Symbol*)type->info.function.prototype.value.p[i];
        DIE* arg = NewSymbolDIE(builder, formal, DW_TAG(formal_parameter));
        VectorAppend(&func->top_scope->variables, arg);
        formal->die = arg;
      }
      if (die->tag == DW_TAG(subprogram)) {
        BuildLexicalScopes(builder, type, func);
      }
      break;
    }
    case kDeclPrimitive:
      if (TypeIsStructOrUnion(type)) {
        die = FindTag(builder, type->info.struct_info->tag_name->value);
        if (die == NULL) {
          die = NewStructDIE(builder, type);
          MapKeyValue kv = {.key.p =
                                type->info.struct_info->tag_name->value,
                            .value.p = die};
          MapInsert(&builder->tags, kv);
          for (size_t i = 0; i < type->info.struct_info->members.length; i++) {
            DIE* member = NewMemberDIE(
                builder,
                (StructMember*)type->info.struct_info->members.value.p[i]);
            StructDIE* s = (StructDIE*)die;
            VectorAppend(&s->members, member);
          }
        }
      } else if (TypeIsEnum(type)) {
        die = FindTag(builder, type->info.enum_info->tag_name->value);
        if (die == NULL) {
          die = NewEnumDIE(builder, type);
          for (size_t i = 0; i < type->info.enum_info->constants.length; i++) {
            DIE* c = NewEnumConstDIE(builder,
                (Symbol*)type->info.enum_info->constants.value.p[i]);
            EnumDIE* e = (EnumDIE*)die;
            VectorAppend(&e->enumerators, c);
          }
          MapKeyValue kv = {.key.p = type->info.enum_info->tag_name->value,
                            .value.p = die};
          MapInsert(&builder->tags, kv);
        }
      } else {
        die = PrimitiveToDIE(type);
      }
      break;
  }
  assert(die != NULL);

  // Handle const and volatile.
  if (TypeIsConst(type)) {
    ConstVolatileDIE* cv = malloc(sizeof(ConstVolatileDIE));
    DIEInit(&cv->die, DW_TAG(const_type), &cv_virtuals, false);
    cv->subtype = die;
    AddDIE(builder, &cv->die);
    die = &cv->die;
  }
  if (TypeIsVolatile(type)) {
    ConstVolatileDIE* cv = malloc(sizeof(ConstVolatileDIE));
    DIEInit(&cv->die, DW_TAG(volatile_type), &cv_virtuals, false);
    cv->subtype = die;
    AddDIE(builder, &cv->die);
    die = &cv->die;
  }

  return die;
}

static void SymbolDIEBuild(DebugBuilder* builder, DIE* die);
static void SymbolDIEPrint(DIE* die, int level);
static void SymbolDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals symbol_virtuals = {NamedDIEDestruct, SymbolDIEBuild,
                                      SymbolDIEPrint, SymbolDIEEmit};

static DIE* NewSymbolDIE(DebugBuilder* builder, Symbol* symbol, DW_TAG tag) {
  if (TypeIsFunction(symbol->type)) {
    return NewTypeRecordDIE(builder, symbol->type);
  }
  VariableDIE* var = malloc(sizeof(VariableDIE));
  NamedDIEInit(&var->die, tag, &symbol_virtuals, symbol->name.value, false);
  var->is_declaration = !symbol->flags.is_defined;
  var->is_external = symbol->flags.is_forward_declared;
  var->type = NewTypeRecordDIE(builder, symbol->type);
  var->location.type = kLocationUnknown;
  AddDIE(builder, (DIE*)var);
  return (DIE*)var;
}

static void MemberDIEBuild(DebugBuilder* builder, DIE* die);
static void MemberDIEPrint(DIE* die, int level);
static void MemberDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals member_virtuals = {NamedDIEDestruct, MemberDIEBuild,
                                      MemberDIEPrint, MemberDIEEmit};

static DIE* NewMemberDIE(DebugBuilder* builder, StructMember* member) {
  MemberDIE* mem = malloc(sizeof(MemberDIE));
  NamedDIEInit(&mem->var.die, DW_TAG(member), &member_virtuals,
               member->symbol->name.value, false);
  mem->var.is_declaration = false;
  mem->var.is_external = false;
  mem->byte_offset = member->byte_offset;
  mem->is_bit_field = StructMemberIsBitField(member);
  mem->bit_offset = member->bit_offset;
  mem->bit_size = member->bit_size;
  mem->var.type = NewTypeRecordDIE(builder, member->symbol->type);
  AddDIE(builder, (DIE*)mem);
  return (DIE*)mem;
}

static void EnumConstDIEBuild(DebugBuilder* builder, DIE* die);
static void EnumConstDIEPrint(DIE* die, int level);
static void EnumConstDIEEmit(DebugBuilder* builder, DIE* die);

static DIEVirtuals enum_const_virtuals = {NamedDIEDestruct, EnumConstDIEBuild,
                                          EnumConstDIEPrint, EnumConstDIEEmit};

static DIE* NewEnumConstDIE(DebugBuilder* builder, Symbol* c) {
  EnumeratorDIE* e = malloc(sizeof(EnumeratorDIE));
  NamedDIEInit(&e->die, DW_TAG(enumerator), &enum_const_virtuals, c->name.value,
               false);
  e->value = c->value.ivalue;
  AddDIE(builder, (DIE*)e);
  return (DIE*)e;
}

static DebugAbbreviation* NewAbbreviation(DebugBuilder* builder,
                                          Buffer* signature) {
  DebugAbbreviation* d = malloc(sizeof(DebugAbbreviation));
  d->num = builder->next_abbrev_number++;
  d->signature = signature;
  return d;
}

static DebugAbbreviation* GetOrCreateAbbbreviation(DebugBuilder* builder,
                                                   Buffer* signature) {
  MapKeyType key = {.p = signature};
  DebugAbbreviation* abbrev = MapFind(&builder->abbreviation_map, key);
  if (abbrev == NULL) {
    abbrev = NewAbbreviation(builder, signature);
    VectorAppend(&builder->abbreviations, abbrev);
    MapKeyValue kv = {.key.p = signature, .value.p = abbrev};
    MapInsert(&builder->abbreviation_map, kv);
  } else {
    // Already exists in map, delete the signature.
    BufferDelete(signature);
  }
  assert(abbrev != NULL);
  return abbrev;
}

DebugAttributeValue* NewDebugAttributeValue(DW_AT id, DW_FORM form) {
  DebugAttributeValue* v = malloc(sizeof(DebugAttributeValue));
  v->form = form;
  v->id = id;
  v->size = 0;
  v->addr_symbol = NULL;
  switch (form) {
    case DW_FORM(block1):
    case DW_FORM(block2):
    case DW_FORM(block4):
    case DW_FORM(block):
    case DW_FORM(exprloc):
      BufferInit(&v->v.block);
      break;
    case DW_FORM(string):
    case DW_FORM(strp):
    case DW_FORM(addr):
    case DW_FORM(high_pc):
      StringInit(&v->v.string, NULL);
      break;
    default:
      v->v.address = 0;
      break;
  }
  return v;
}

void DebugAttributeValueDestruct(DebugAttributeValue* v) {
  switch (v->form) {
    case DW_FORM(block1):
    case DW_FORM(block2):
    case DW_FORM(block4):
    case DW_FORM(block):
    case DW_FORM(exprloc):
      BufferDestruct(&v->v.block);
      break;
    case DW_FORM(string):
    case DW_FORM(strp):
    case DW_FORM(addr):
    case DW_FORM(high_pc):
      StringDestruct(&v->v.string);
      break;
    default:
      break;
  }
}

static void AddAttributeAndValue(DebugBuilder* builder, DIE* die,
                                 DebugAttributeValue* value) {
  VectorAppend(&die->attr_values, value);
}

static DebugAttributeValue* SignedConstant(DW_AT attr_id, int64_t data) {
  DebugAttributeValue* v;
  uint64_t value = data;

  if (data < 0) {
    value = -value;
  }
  if (value < (1LL << 8)) {
    v = NewDebugAttributeValue(attr_id, DW_FORM(data1));
  } else if (value < (1LL << 16)) {
    v = NewDebugAttributeValue(attr_id, DW_FORM(data2));
  } else if (value < (1LL << 32)) {
    v = NewDebugAttributeValue(attr_id, DW_FORM(data4));
  } else {
    v = NewDebugAttributeValue(attr_id, DW_FORM(data8));
  }
  switch (v->form) {
    case DW_FORM(data1):
      v->v.data1 = (uint8_t)data;
      v->size = 1;
      break;
    case DW_FORM(data2):
      v->v.data2 = (uint16_t)data;
      v->size = 2;
      break;
    case DW_FORM(data4):
      v->v.data4 = (uint32_t)data;
      v->size = 4;
      break;
    case DW_FORM(data8):
      v->v.data8 = data;
      v->size = 8;
      break;
    default:
      break;
  }
  return v;
}

static DebugAttributeValue* UnsignedConstant(DW_AT attr_id, uint64_t data) {
  DebugAttributeValue* v;

  if (data < (1LL << 8)) {
    v = NewDebugAttributeValue(attr_id, DW_FORM(data1));
  } else if (data < (1LL << 16)) {
    v = NewDebugAttributeValue(attr_id, DW_FORM(data2));
  } else if (data < (1LL << 32)) {
    v = NewDebugAttributeValue(attr_id, DW_FORM(data4));
  } else {
    v = NewDebugAttributeValue(attr_id, DW_FORM(data8));
  }
  switch (v->form) {
    case DW_FORM(data1):
      v->v.data1 = (uint8_t)data;
      v->size = 1;
      break;
    case DW_FORM(data2):
      v->v.data2 = (uint16_t)data;
      v->size = 2;
      break;
    case DW_FORM(data4):
      v->v.data4 = (uint32_t)data;
      v->size = 4;
      break;
    case DW_FORM(data8):
      v->v.data8 = data;
      v->size = 8;
      break;
    default:
      break;
  }
  return v;
}

static DebugAttributeValue* AddressConstant(DW_AT attr_id,
                                            DW_FORM form, const char* value) {
  DebugAttributeValue* v = NewDebugAttributeValue(attr_id, form);
  StringInit(&v->v.string, value);
  v->size = 8;
  return v;
}

static DebugAttributeValue* StringConstant(DW_AT attr_id,
                                           const char* s) {
  if (s == NULL) {
    s = "";
  }
  DW_FORM form = DW_FORM(string);
  size_t len = strlen(s) + 1;
  // Strings longer than 4 bytes are stored as indirect refs into
  // a string table.
  if (len > 4) {
    form = DW_FORM(strp);
  }
  DebugAttributeValue* v = NewDebugAttributeValue(attr_id, form);
  if (form == DW_FORM(string)) {
    v->size = (int)strlen(s) + 1;
  } else {
    v->size = 4;
  }
  StringSet(&v->v.string, s);
  return v;
}

static DebugAttributeValue* DIEReference(DW_AT attr_id, DIE* die) {
  DebugAttributeValue* v = NewDebugAttributeValue(attr_id, DW_FORM(ref4));
  v->v.die = die;
  v->size = 4;
  return v;
}

static DebugAttributeValue* Flag(DW_AT attr_id, bool value) {
  DebugAttributeValue* v = NewDebugAttributeValue(attr_id, DW_FORM(flag));
  v->v.flag = value;
  v->size = 1;
  return v;
}

static DebugAttributeValue* SectionOffset(DW_AT attr_id, uint32_t offset) {
  DebugAttributeValue* v = NewDebugAttributeValue(attr_id, DW_FORM(sec_offset));
  v->v.data4 = offset;
  v->size = 4;
  return v;
}

static DW_LANG DebugLanguage(void) {
  if (CompilerIsCXX()) {
    if (compiler->language_standard >= kLanguageStandardCXX23) {
      return DW_LANG(C_plus_plus_23);
    }
    if (compiler->language_standard >= kLanguageStandardCXX20) {
      return DW_LANG(C_plus_plus_20);
    }
    if (compiler->language_standard >= kLanguageStandardCXX17) {
      return DW_LANG(C_plus_plus_17);
    }
    if (compiler->language_standard >= kLanguageStandardCXX14) {
      return DW_LANG(C_plus_plus_14);
    }
    if (compiler->language_standard >= kLanguageStandardCXX11) {
      return DW_LANG(C_plus_plus_11);
    }
    return DW_LANG(C_plus_plus);
  }
  if (compiler->language_standard >= kLanguageStandardC11) {
    return DW_LANG(C11);
  }
  if (compiler->language_standard >= kLanguageStandardC99) {
    return DW_LANG(C99);
  }
  if (compiler->language_standard >= kLanguageStandardC89) {
    return DW_LANG(C89);
  }
  return DW_LANG(C);
}

static const char* AddressDirective(void) {
  switch (compiler->pointer_size) {
    case 2:
      return ".short";
    case 4:
      return ".word";
    default:
      return ".long";
  }
}


void DebugAttributeValueDelete(DebugAttributeValue* v) {
  DebugAttributeValueDestruct(v);
  free(v);
}

// DIE Builder and printer functions.
// The builders convert a DIE tree into DWARF output format.

static void FlushAccumulator(DebugBuilder* builder) {
  if (builder->module != NULL) {
    AsmModuleBytes(builder->module, builder->bytes.value,
                   builder->bytes.length);
    BufferClear(&builder->bytes);
    return;
  }
  FILE* fp = builder->fp;
  for (size_t i = 0; i < builder->bytes.length; i++) {
    fprintf(fp, "\t.byte 0x%02x\n", builder->bytes.value[i] & 0xff);
  }
  BufferClear(&builder->bytes);
}

static void DebugModuleInteger(DebugBuilder* builder, int width,
                               const char* expression) {
  AsmExpr expr;
  const char* value = expression;
  size_t length = strlen(value);
  if (length >= 2 && value[0] == '(' && value[length - 1] == ')') {
    value++;
    length -= 2;
  }
  const char* minus = memchr(value, '-', length);
  if (minus != NULL) {
    char left[512];
    char right[512];
    size_t left_length = (size_t)(minus - value);
    size_t right_length = length - left_length - 1;
    assert(left_length < sizeof(left) && right_length < sizeof(right));
    memcpy(left, value, left_length);
    left[left_length] = '\0';
    memcpy(right, minus + 1, right_length);
    right[right_length] = '\0';
    AsmExprInitDifference(&expr, left, right, 0);
  } else {
    char symbol[512];
    assert(length < sizeof(symbol));
    memcpy(symbol, value, length);
    symbol[length] = '\0';
    AsmExprInitSymbol(&expr, symbol, 0);
  }
  AsmModuleInteger(builder->module, width, &expr);
  AsmExprDestruct(&expr);
}

static void WriteUnsignedLEB128(Buffer* buffer, uint64_t value) {
  do {
    int8_t byte = value & 0x7f;
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }
    BufferAppendByte(buffer, byte);
  } while (value != 0);
}

static void WriteSignedLEB128(Buffer* buffer, int64_t value) {
  bool more = true;
  while (more) {
    int8_t byte = value & 0x7f;
    value >>= 7;
    /* sign bit of byte is second high order bit (0x40) */
    if ((value == 0 && (byte & 0x40) == 0) ||
        (value == -1 && (byte & 0x40) == 0x40)) {
      more = false;
    } else {
      byte |= 0x80;
    }
    BufferAppendByte(buffer, byte);
  }
}

static void WriteByte(Buffer* buffer, int8_t v) {
  BufferAppendByte(buffer, v);
}

static void DebugEmitLabel(DebugBuilder* builder, const char* name) {
  FlushAccumulator(builder);
  if (builder->module != NULL) {
    AsmModuleLabel(builder->module, name);
  } else {
    fprintf(builder->fp, "%s:\n", name);
  }
}

static void DebugEmitConstant(DebugBuilder* builder, int width, int64_t value) {
  FlushAccumulator(builder);
  if (builder->module != NULL) {
    AsmExpr expr;
    AsmExprInitConstant(&expr, value);
    AsmModuleInteger(builder->module, width, &expr);
    AsmExprDestruct(&expr);
    return;
  }
  switch (width) {
    case 1:
      fprintf(builder->fp, "\t.byte %d\n", (int)value);
      break;
    case 2:
      fprintf(builder->fp, "\t.short %d\n", (int)value);
      break;
    case 4:
      fprintf(builder->fp, "\t.word %d\n", (int)value);
      break;
    default:
      fprintf(builder->fp, "\t.long %lld\n", (long long)value);
      break;
  }
}

static void DebugEmitAddress(DebugBuilder* builder, const char* symbol) {
  FlushAccumulator(builder);
  if (builder->module != NULL) {
    DebugModuleInteger(builder, compiler->pointer_size, symbol);
  } else {
    fprintf(builder->fp, "\t%s %s\n", AddressDirective(), symbol);
  }
}

static DebugAttributeValue* LocationAttribute(VariableDIE* var) {
  DebugAttributeValue* v =
      NewDebugAttributeValue(DW_AT(location), DW_FORM(exprloc));
  switch (var->location.type) {
    case kLocationInRegister:
      if (var->location.v.reg >= 0 && var->location.v.reg < 32) {
        BufferAppendByte(&v->v.block, DW_OP(reg0) + var->location.v.reg);
      } else {
        BufferAppendByte(&v->v.block, (char)DW_OP(regx));
        WriteUnsignedLEB128(&v->v.block, (uint64_t)var->location.v.reg);
      }
      break;
    case kLocationOnStack:
      BufferAppendByte(&v->v.block, (char)DW_OP(fbreg));
      WriteSignedLEB128(&v->v.block, var->location.v.stack_offset);
      break;
    case kLocationStatic:
      if (var->location.v.symbol_name == NULL) {
        DebugAttributeValueDelete(v);
        return NULL;
      }
      v->addr_symbol = var->location.v.symbol_name;
      break;
    case kLocationUnknown:
      DebugAttributeValueDelete(v);
      return NULL;
  }
  return v;
}

static DebugAttributeValue* FrameBaseAttribute(int dwarf_reg) {
  DebugAttributeValue* v =
      NewDebugAttributeValue(DW_AT(frame_base), DW_FORM(exprloc));
  if (dwarf_reg >= 0 && dwarf_reg < 32) {
    BufferAppendByte(&v->v.block, DW_OP(reg0) + dwarf_reg);
  } else {
    BufferAppendByte(&v->v.block, (char)DW_OP(regx));
    WriteUnsignedLEB128(&v->v.block, (uint64_t)dwarf_reg);
  }
  return v;
}

static void DebugAttributeValueEmit(DebugBuilder* builder, DebugAttributeValue* attr) {
  Buffer* buffer = &builder->bytes;
  switch (attr->form) {
    case DW_FORM(data1):
      WriteByte(buffer, attr->v.data1);
      break;
    case DW_FORM(data2):
      WriteByte(buffer, attr->v.data2 & 0xff);
      WriteByte(buffer, (attr->v.data2 >> 8) & 0xff);
      break;
    case DW_FORM(data4):
    case DW_FORM(sec_offset):
      WriteByte(buffer, attr->v.data4 & 0xff);
      WriteByte(buffer, (attr->v.data4 >> 8) & 0xff);
      WriteByte(buffer, (attr->v.data4 >> 16) & 0xff);
      WriteByte(buffer, (attr->v.data4 >> 24) & 0xff);
      break;
    case DW_FORM(data8):
      WriteByte(buffer, attr->v.data8 & 0xff);
      WriteByte(buffer, (attr->v.data8 >> 8) & 0xff);
      WriteByte(buffer, (attr->v.data8 >> 16) & 0xff);
      WriteByte(buffer, (attr->v.data8 >> 24) & 0xff);
      WriteByte(buffer, (attr->v.data8 >> 32) & 0xff);
      WriteByte(buffer, (attr->v.data8 >> 40) & 0xff);
      WriteByte(buffer, (attr->v.data8 >> 48) & 0xff);
      WriteByte(buffer, (attr->v.data8 >> 56) & 0xff);
      break;
    case DW_FORM(ref4): {
      FlushAccumulator(builder);
      char label[64];
      snprintf(label, sizeof(label), "__DW_DIE%d", attr->v.die->id);
      if (builder->module != NULL) {
        AsmExpr expr;
        AsmExprInitDifference(&expr, label, ".DW_info_begin", 0);
        AsmModuleInteger(builder->module, 4, &expr);
        AsmExprDestruct(&expr);
      } else {
        fprintf(builder->fp, "\t.word %s-.DW_info_begin\n", label);
      }
      break;
    }
    case DW_FORM(flag):
      WriteByte(buffer, attr->v.flag);
      break;
    case DW_FORM(string):
      FlushAccumulator(builder);
      if (builder->module != NULL) {
        AsmModuleString(builder->module, attr->v.string.value, true);
      } else {
        fprintf(builder->fp, "\t.string \"%s\"\n", attr->v.string.value);
      }
      break;
    case DW_FORM(strp): {
      FlushAccumulator(builder);
      size_t offset = builder->string_table.length;
      if (builder->module != NULL) {
        AsmExpr value;
        AsmExprInitConstant(&value, (int64_t)offset);
        AsmModuleInteger(builder->module, 4, &value);
        AsmExprDestruct(&value);
      } else {
        fprintf(builder->fp, "\t.word 0x%zx\n", offset);
      }
      BufferAppend(&builder->string_table, attr->v.string.value,
                   attr->v.string.length + 1);
      break;
    }
    case DW_FORM(addr):
      DebugEmitAddress(builder, attr->v.string.value);
      break;
    case DW_FORM(high_pc):
      FlushAccumulator(builder);
      if (builder->module != NULL) {
        DebugModuleInteger(builder, 4, attr->v.string.value);
      } else {
        fprintf(builder->fp, "\t.word %s\n", attr->v.string.value);
      }
      break;
    case DW_FORM(exprloc): {
      size_t expr_length = attr->v.block.length;
      if (attr->addr_symbol != NULL) {
        expr_length = 1 + (size_t)compiler->pointer_size;
      }
      WriteUnsignedLEB128(buffer, expr_length);
      if (attr->addr_symbol != NULL) {
        WriteByte(buffer, DW_OP(addr));
        FlushAccumulator(builder);
        DebugEmitAddress(builder, attr->addr_symbol);
      } else {
        for (size_t i = 0; i < attr->v.block.length; i++) {
          WriteByte(buffer, attr->v.block.value[i]);
        }
      }
      break;
    }
    default:
      assert(false);
  }
}

static void GenerateAbbreviationSignature(DIE* die, Buffer* buffer) {
  WriteUnsignedLEB128(buffer, die->tag);
  WriteByte(buffer, die->has_children);
  for (size_t i = 0; i < die->attr_values.length; i++) {
    DebugAttributeValue* attr = die->attr_values.value.p[i];
    WriteUnsignedLEB128(buffer, attr->id);
    // DW_FORM(high_pc) is a fake form that is encoded as DW_FORM(data4).
    if (attr->form == DW_FORM(high_pc)) {
      WriteUnsignedLEB128(buffer, DW_FORM(data4));
    } else {
      WriteUnsignedLEB128(buffer, attr->form);
    }
  }
}

void DIEAllocateAbbreviation(DebugBuilder* builder, DIE* die) {
  Buffer* signature = NewBuffer();
  GenerateAbbreviationSignature(die, signature);
  DebugAbbreviation* abbrev = GetOrCreateAbbbreviation(builder, signature);
  die->abbrev = abbrev;
}

static void DIEBaseEmit(DebugBuilder* builder, DIE* die) {
  FlushAccumulator(builder);
  if (builder->module != NULL) {
    char label[64];
    char comment[256];
    snprintf(label, sizeof(label), "__DW_DIE%d", die->id);
    snprintf(comment, sizeof(comment), "%s (abbrev %d)",
             DW_TAGString(die->tag), die->abbrev->num);
    AsmModuleLabel(builder->module, label);
    AsmModuleComment(builder->module, comment);
  } else {
    fprintf(builder->fp, "__DW_DIE%d:\t\t// %s (abbrev %d)\n", die->id,
            DW_TAGString(die->tag), die->abbrev->num);
  }
  WriteUnsignedLEB128(&builder->bytes, die->abbrev->num);
  for (size_t i = 0; i < die->attr_values.length; i++) {
    DebugAttributeValueEmit(builder, die->attr_values.value.p[i]);
  }
}

static void DIEEmit(DebugBuilder* builder, DIE* die) {
  assert(die != NULL);
  if (die->emitted) {
    return;
  }
  // Mark before emitting children so recursive type graphs (C++ methods
  // whose `this` pointer refers back to the enclosing class) terminate.
  die->emitted = true;
  die->virtuals->emitter(builder, die);
}

static void BaseTypeDIEBuild(DebugBuilder* builder, DIE* die) {
  BaseTypeDIE* base = (BaseTypeDIE*)die;
  if (die->tag == DW_TAG(base_type)) {
    AddAttributeAndValue(builder, die,
                         SignedConstant(DW_AT(byte_size), base->byte_size));
    AddAttributeAndValue(builder, die,
                         SignedConstant(DW_AT(encoding), base->encoding));
  }
  AddAttributeAndValue(
      builder, die, StringConstant(DW_AT(name), base->name));
  DIEAllocateAbbreviation(builder, die);
}

static void DoIndent(int level) {
  while (level-- > 0) {
    printf("  ");
  }
}

static void BaseTypeDIEPrint(DIE* die, int level) {
  DoIndent(level);
  BaseTypeDIE* base = (BaseTypeDIE*)die;
  printf("%s, encoding: %s, byte_size: %d, name: %s\n", DW_TAGString(die->tag),
         DW_ATEString(base->encoding), base->byte_size, base->name);
}

static void BaseTypeDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
}

static void EmitEndOfChildren(DebugBuilder* builder) {
  WriteByte(&builder->bytes, 0);
}

static void SubrangeDIEBuild(DebugBuilder* builder, DIE* die) {
  SubrangeDIE* s = (SubrangeDIE*)die;
  AddAttributeAndValue(builder, die, DIEReference(DW_AT(type), s->type));
  if (s->upper_bound > 0) {
    AddAttributeAndValue(builder, die,
                         UnsignedConstant(DW_AT(count), (uint64_t)s->upper_bound));
  }
  DIEAllocateAbbreviation(builder, die);
  DIEBuild(builder, s->type);
}

static void SubrangeDIEPrint(DIE* die, int level) {
  DoIndent(level);
  SubrangeDIE* s = (SubrangeDIE*)die;
  printf("%s: upper_bound: %d\n", DW_TAGString(die->tag), s->upper_bound);
  DIEPrint(s->type, level + 1);
}

static void SubrangeDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
  SubrangeDIE* s = (SubrangeDIE*)die;
  DIEEmit(builder, s->type);
}

static void ArrayDIEBuild(DebugBuilder* builder, DIE* die) {
  ArrayTypeDIE* array = (ArrayTypeDIE*)die;
  DIEBuild(builder, array->subtype);
  AddAttributeAndValue(builder, die, DIEReference(DW_AT(type), array->subtype));
  DIEAllocateAbbreviation(builder, die);
  DIEBuild(builder, array->subrange);
}

static void ArrayDIEPrint(DIE* die, int level) {
  DoIndent(level);
  ArrayTypeDIE* array = (ArrayTypeDIE*)die;
  printf("%s:\n", DW_TAGString(die->tag));
  DIEPrint(array->subtype, level + 1);
  DIEPrint(array->subrange, level + 1);
}

static void ArrayDIEEmit(DebugBuilder* builder, DIE* die) {
  ArrayTypeDIE* array = (ArrayTypeDIE*)die;
  DIEBaseEmit(builder, die);
  DIEEmit(builder, array->subrange);
  EmitEndOfChildren(builder);
  DIEEmit(builder, array->subtype);
}

static void PointerDIEBuild(DebugBuilder* builder, DIE* die) {
  PointerTypeDIE* ptr = (PointerTypeDIE*)die;
  DIEBuild(builder, ptr->subtype);
  AddAttributeAndValue(builder, die, DIEReference(DW_AT(type), ptr->subtype));
  AddAttributeAndValue(
      builder, die, UnsignedConstant(DW_AT(byte_size), compiler->pointer_size));
  DIEAllocateAbbreviation(builder, die);
}

static void PointerDIEPrint(DIE* die, int level) {
  DoIndent(level);
  PointerTypeDIE* ptr = (PointerTypeDIE*)die;
  printf("%s:\n", DW_TAGString(die->tag));
  DIEPrint(ptr->subtype, level + 1);
}

static void PointerDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
  PointerTypeDIE* ptr = (PointerTypeDIE*)die;
  DIEEmit(builder, ptr->subtype);
}

static void PtrToMemberDIEBuild(DebugBuilder* builder, DIE* die) {
  PtrToMemberTypeDIE* ptr = (PtrToMemberTypeDIE*)die;
  DIEBuild(builder, ptr->member_type);
  DIEBuild(builder, ptr->containing_type);
  AddAttributeAndValue(builder, die,
                       DIEReference(DW_AT(type), ptr->member_type));
  AddAttributeAndValue(builder, die, DIEReference(DW_AT(containing_type),
                                                  ptr->containing_type));
  AddAttributeAndValue(
      builder, die, UnsignedConstant(DW_AT(byte_size), ptr->byte_size));
  DIEAllocateAbbreviation(builder, die);
}

static void PtrToMemberDIEPrint(DIE* die, int level) {
  DoIndent(level);
  PtrToMemberTypeDIE* ptr = (PtrToMemberTypeDIE*)die;
  printf("%s:\n", DW_TAGString(die->tag));
  DIEPrint(ptr->member_type, level + 1);
  DIEPrint(ptr->containing_type, level + 1);
}

static void PtrToMemberDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
  PtrToMemberTypeDIE* ptr = (PtrToMemberTypeDIE*)die;
  DIEEmit(builder, ptr->member_type);
  DIEEmit(builder, ptr->containing_type);
}

static void LexicalScopeDIEBuild(DebugBuilder* builder, DIE* die) {
  LexicalScopeDIE* scope = (LexicalScopeDIE*)die;
  AddAttributeAndValue(builder, die,
                       AddressConstant(DW_AT(low_pc), DW_FORM(addr), scope->low_pc_label.value));
  AddAttributeAndValue(builder, die,
                       AddressConstant(DW_AT(high_pc), DW_FORM(high_pc), scope->high_pc_expr.value));

  DIEAllocateAbbreviation(builder, die);

  // Add variable children.
  for (size_t i = 0; i < scope->variables.length; i++) {
    DIEBuild(builder, scope->variables.value.p[i]);
  }

  // Add LexicalScopes.
  for (size_t i = 0; i < scope->lexical_scopes.length; i++) {
    DIEBuild(builder, scope->lexical_scopes.value.p[i]);
  }
}

static void LexicalScopeDIEPrint(DIE* die, int level) {
  DoIndent(level);
  LexicalScopeDIE* scope = (LexicalScopeDIE*)die;
  printf("%s: low_pc: %s, high_pc: %s\n", DW_TAGString(die->tag),
         scope->low_pc_label.value, scope->high_pc_expr.value);
  for (size_t i = 0; i < scope->variables.length; i++) {
    DIEPrint(scope->variables.value.p[i], level + 1);
  }

  for (size_t i = 0; i < scope->lexical_scopes.length; i++) {
    DIEPrint(scope->lexical_scopes.value.p[i], level + 1);
  }
}

static void LexicalScopeDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
  LexicalScopeDIE* scope = (LexicalScopeDIE*)die;
  
  // Emit variable children.
  for (size_t i = 0; i < scope->variables.length; i++) {
    DIEEmit(builder, scope->variables.value.p[i]);
  }

  // Emit LexicalScopes.
  for (size_t i = 0; i < scope->lexical_scopes.length; i++) {
    DIEEmit(builder, scope->lexical_scopes.value.p[i]);
  }
  EmitEndOfChildren(builder);
}

static void FunctionDIEBuild(DebugBuilder* builder, DIE* die) {
  FunctionTypeDIE* func = (FunctionTypeDIE*)die;
  if (func->subtype != NULL) {
    DIEBuild(builder, func->subtype);
    AddAttributeAndValue(builder, die,
                         DIEReference(DW_AT(type), func->subtype));
  }
  if (die->tag == DW_TAG(subprogram)) {
    AddAttributeAndValue(
        builder, die,
        StringConstant(DW_AT(name), func->die.name.value));
    if (func->linkage_name != NULL) {
      AddAttributeAndValue(builder, die,
                           StringConstant(DW_AT(linkage_name),
                                          func->linkage_name));
    }
    AddAttributeAndValue(builder, die, Flag(DW_AT(external), func->is_external));
    AddAttributeAndValue(builder, die, Flag(DW_AT(prototyped), true));
    if (compiler->target->dwarf_frame_register >= 0) {
      AddAttributeAndValue(builder, die,
                           FrameBaseAttribute(compiler->target->dwarf_frame_register));
    }
    AddAttributeAndValue(
        builder, die,
        AddressConstant(DW_AT(low_pc), DW_FORM(addr),
                        func->top_scope->low_pc_label.value));
    AddAttributeAndValue(
        builder, die,
        AddressConstant(DW_AT(high_pc), DW_FORM(high_pc),
                        func->top_scope->high_pc_expr.value));
  } else {
    AddAttributeAndValue(builder, die, Flag(DW_AT(prototyped), true));
  }
  DIEAllocateAbbreviation(builder, die);

  // Add variable and parameter children.
  for (size_t i = 0; i < func->top_scope->variables.length; i++) {
    DIEBuild(builder, func->top_scope->variables.value.p[i]);
  }

  // Add LexicalScopes.
  for (size_t i = 0; i < func->top_scope->lexical_scopes.length; i++) {
    DIEBuild(builder, func->top_scope->lexical_scopes.value.p[i]);
  }
}

static void FunctionDIEPrint(DIE* die, int level) {
  DoIndent(level);
  FunctionTypeDIE* func = (FunctionTypeDIE*)die;
  printf("%s: %s: low_pc: %s, high_pc: %s\n", DW_TAGString(die->tag),
         func->die.name.value, func->top_scope->low_pc_label.value,
         func->top_scope->high_pc_expr.value);
  for (size_t i = 0; i < func->top_scope->variables.length; i++) {
    DIEPrint(func->top_scope->variables.value.p[i], level + 1);
  }

  for (size_t i = 0; i < func->top_scope->lexical_scopes.length; i++) {
    DIEPrint(func->top_scope->lexical_scopes.value.p[i], level + 1);
  }
}

static void FunctionDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
  FunctionTypeDIE* func = (FunctionTypeDIE*)die;
  LexicalScopeDIE* scope = func->top_scope;
  
  // Emit variable children.
  for (size_t i = 0; i < scope->variables.length; i++) {
    DIEEmit(builder, scope->variables.value.p[i]);
  }

  // Emit LexicalScopes.
  for (size_t i = 0; i < scope->lexical_scopes.length; i++) {
    DIEEmit(builder, scope->lexical_scopes.value.p[i]);
  }
  EmitEndOfChildren(builder);
  if (func->subtype != NULL) {
    DIEEmit(builder, func->subtype);
  }
}

static void StructDIEBuild(DebugBuilder* builder, DIE* die) {
  StructDIE* s = (StructDIE*)die;

  AddAttributeAndValue(
      builder, die,
      StringConstant(DW_AT(name), s->die.name.value));
  AddAttributeAndValue(builder, die,
                       UnsignedConstant(DW_AT(byte_size), s->byte_size));
  DIEAllocateAbbreviation(builder, die);
  
  // Members.
  for (size_t i = 0; i < s->members.length; i++) {
    DIEBuild(builder, s->members.value.p[i]);
  }
}

static void StructDIEPrint(DIE* die, int level) {
  DoIndent(level);
  StructDIE* s = (StructDIE*)die;
  printf("%s: %s, byte_size: %d\n", DW_TAGString(die->tag), s->die.name.value,
         s->byte_size);
  for (size_t i = 0; i < s->members.length; i++) {
    DIEPrint(s->members.value.p[i], level + 1);
  }
}

static void StructDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
  StructDIE* s = (StructDIE*)die;
  
  // Members.
  for (size_t i = 0; i < s->members.length; i++) {
    DIEEmit(builder, s->members.value.p[i]);
  }
  EmitEndOfChildren(builder);
}

static void EnumDIEBuild(DebugBuilder* builder, DIE* die) {
  EnumDIE* e = (EnumDIE*)die;
  AddAttributeAndValue(
      builder, die,
      StringConstant(DW_AT(name), e->die.name.value));
  AddAttributeAndValue(builder, die, UnsignedConstant(DW_AT(byte_size),
                                                       (uint64_t)e->byte_size));
  DIEAllocateAbbreviation(builder, die);
  
  // Enumerators.
  for (size_t i = 0; i < e->enumerators.length; i++) {
    DIEBuild(builder, e->enumerators.value.p[i]);
  }
}

static void EnumDIEPrint(DIE* die, int level) {
  DoIndent(level);
  EnumDIE* e = (EnumDIE*)die;
  printf("%s: %s\n", DW_TAGString(die->tag), e->die.name.value);
  for (size_t i = 0; i < e->enumerators.length; i++) {
    DIEPrint(e->enumerators.value.p[i], level + 1);
  }
}

static void EnumDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
  EnumDIE* e = (EnumDIE*)die;
  for (size_t i = 0; i < e->enumerators.length; i++) {
    DIEEmit(builder, e->enumerators.value.p[i]);
  }
  EmitEndOfChildren(builder);
}

static void CVDIEBuild(DebugBuilder* builder, DIE* die) {
  ConstVolatileDIE* cv = (ConstVolatileDIE*)die;
  DIEBuild(builder, cv->subtype);
  AddAttributeAndValue(builder, die, DIEReference(DW_AT(type), cv->subtype));
  DIEAllocateAbbreviation(builder, die);
}

static void CVDIEPrint(DIE* die, int level) {
  DoIndent(level);
  ConstVolatileDIE* cv = (ConstVolatileDIE*)die;
  printf("%s:\n", DW_TAGString(die->tag));
  DIEPrint(cv->subtype, level + 1);
}

static void CVDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
  ConstVolatileDIE* cv = (ConstVolatileDIE*)die;
  DIEEmit(builder, cv->subtype);
}

static void SymbolDIEBuild(DebugBuilder* builder, DIE* die) {
  VariableDIE* var = (VariableDIE*)die;
  AddAttributeAndValue(
      builder, die,
      StringConstant(DW_AT(name), var->die.name.value));
  DIEBuild(builder, var->type);

  AddAttributeAndValue(builder, die, DIEReference(DW_AT(type), var->type));
  AddAttributeAndValue(builder, die, Flag(DW_AT(external), var->is_external));
  DebugAttributeValue* location = LocationAttribute(var);
  if (location != NULL) {
    AddAttributeAndValue(builder, die, location);
  }
  DIEAllocateAbbreviation(builder, die);
}

static void SymbolDIEPrint(DIE* die, int level) {
  DoIndent(level);
  VariableDIE* var = (VariableDIE*)die;
  printf("%s: %s ", DW_TAGString(die->tag), var->die.name.value);
  switch (var->location.type) {
    case kLocationStatic:
      printf("%s\n", var->location.v.symbol_name == NULL
                         ? "null"
                         : var->location.v.symbol_name);
      break;
    case kLocationOnStack:
      printf("offset %d\n", var->location.v.stack_offset);
      break;
    case kLocationUnknown:
      printf("<unknown>\n");
      break;
    case kLocationInRegister:
      printf("reg %d\n", var->location.v.reg);
      break;
  }
  DIEPrint(var->type, level + 1);
}

static void SymbolDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
  VariableDIE* var = (VariableDIE*)die;
  DIEEmit(builder, var->type);
}

void VariableDIESetRegister(DIE* die, int reg) {
  if (die == NULL) {
    return;
  }
  VariableDIE* var = (VariableDIE*)die;
  var->location.type = kLocationInRegister;
  var->location.v.reg = reg;
}

void VariableDIESetStackOffset(DIE* die, int offset) {
  if (die == NULL) {
    return;
  }
  VariableDIE* var = (VariableDIE*)die;
  var->location.type = kLocationOnStack;
  var->location.v.stack_offset = offset;
}

void VariableDIESetStatic(DIE* die, const char* symbol) {
  if (die == NULL) {
    return;
  }
  VariableDIE* var = (VariableDIE*)die;
  var->location.type = kLocationStatic;
  var->location.v.symbol_name = symbol;
}

static void MemberDIEBuild(DebugBuilder* builder, DIE* die) {
  MemberDIE* member = (MemberDIE*)die;
  AddAttributeAndValue(
      builder, die,
      StringConstant(DW_AT(name), member->var.die.name.value));
  DIEBuild(builder, member->var.type);

  AddAttributeAndValue(builder, die,
                       DIEReference(DW_AT(type), member->var.type));
  AddAttributeAndValue(builder, die,
                       Flag(DW_AT(external), member->var.is_external));
  AddAttributeAndValue(
      builder, die,
      UnsignedConstant(DW_AT(data_member_location), member->byte_offset));
  if (member->is_bit_field) {
    AddAttributeAndValue(builder, die, UnsignedConstant(DW_AT(byte_size), 8));
    AddAttributeAndValue(
        builder, die, UnsignedConstant(DW_AT(bit_offset), member->bit_offset));
    AddAttributeAndValue(builder, die,
                         UnsignedConstant(DW_AT(bit_size), member->bit_size));
  }
  DIEAllocateAbbreviation(builder, die);
}

static void MemberDIEPrint(DIE* die, int level) {
  MemberDIE* member = (MemberDIE*)die;
  SymbolDIEPrint(&member->var.die.die, level);
  DoIndent(level);
  printf("  byte_offset: %d, is_bitfield: %d\n", member->byte_offset,
         member->is_bit_field);
  if (member->is_bit_field) {
    DoIndent(level);
    printf("  bit_offset: %d, bit_size: %d\n", member->bit_offset,
           member->bit_size);
  }
}

static void MemberDIEEmit(DebugBuilder* builder, DIE* die) {
  SymbolDIEEmit(builder, die);
}

static void EnumConstDIEBuild(DebugBuilder* builder, DIE* die) {
  EnumeratorDIE* e = (EnumeratorDIE*)die;
  AddAttributeAndValue(
      builder, die,
      StringConstant(DW_AT(name),  e->die.name.value));
  AddAttributeAndValue(builder, die,
                       SignedConstant(DW_AT(const_value), e->value));
  DIEAllocateAbbreviation(builder, die);
}

static void EnumConstDIEPrint(DIE* die, int level) {
  DoIndent(level);
  EnumeratorDIE* e = (EnumeratorDIE*)die;
  printf("%s: value: %" PRId64 "\n", DW_TAGString(die->tag), e->value);
}

static void EnumConstDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
}

static int8_t* DecodeUnsignedLEB128(int8_t* data, int64_t* result) {
  *result = 0;
  int shift = 0;
  while (true) {
    int8_t byte = *data++;
    *result |= (byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) {
      break;
    }
    shift += 7;
  }
  return data;
}

static COMPILER_UNUSED void DebugAbbreviationPrint(DebugAbbreviation* abbrev, int level) {
  DoIndent(level);

  // Decode the signature.  It starts with the tag, then the has_children
  // byte.  Then follow the attributes.
  int8_t* data = (int8_t*)abbrev->signature->value;
  int8_t* end = (int8_t*)abbrev->signature->value + abbrev->signature->length;
  int64_t tag;
  data = DecodeUnsignedLEB128(data, &tag);
  bool has_children = *data++;
  printf("%d: %s [%s_children]\n", abbrev->num, DW_TAGString((DW_TAG)tag),
         has_children ? "has" : "no");
  while (data < end) {
    int64_t id, form;
    data = DecodeUnsignedLEB128(data, &id);
    data = DecodeUnsignedLEB128(data, &form);
    DoIndent(level + 1);
    printf("%s %s\n", DW_ATString((DW_AT)id), DW_FORMString((DW_FORM)form));
  }
  printf("\n");
}

static void DebugAbbreviationEmit(DebugBuilder* builder,
                                  DebugAbbreviation* abbrev) {
  Buffer* buffer = &builder->bytes;
  WriteUnsignedLEB128(buffer, abbrev->num);
  for (size_t i = 0; i < abbrev->signature->length; i++) {
    WriteByte(buffer, abbrev->signature->value[i]);
  }
  WriteUnsignedLEB128(buffer, 0);
  WriteUnsignedLEB128(buffer, 0);
}

void DebugBuilderEmitAbbreviations(DebugBuilder* builder) {
#if 0
  for (size_t i = 0; i < builder->abbreviations.length; i++) {
    DebugAbbreviationPrint(builder->abbreviations.value.p[i], 0);
  }
#endif
  if (builder->module != NULL) {
    AsmModuleSection(builder->module, ".debug_abbrev", SHT(progbits), 0, 1);
    AsmModuleLabel(builder->module, ".DW_abbrev_begin");
  } else {
    fprintf(builder->fp, "\n\t.section .debug_abbrev,\"\",@progbits,1\n");
    fprintf(builder->fp, ".DW_abbrev_begin:\n");
  }
  for (size_t i = 0; i < builder->abbreviations.length; i++) {
    DebugAbbreviationEmit(builder, builder->abbreviations.value.p[i]);
  }
  WriteByte(&builder->bytes, 0);
  FlushAccumulator(builder);
}

// Compile unit
static void CompileUnitDIEBuild(DebugBuilder* builder, DIE* die) {
  AddAttributeAndValue(
       builder, die,
       StringConstant(DW_AT(producer), builder->producer));
  AddAttributeAndValue(
       builder, die,
       UnsignedConstant(DW_AT(language), DebugLanguage()));
  AddAttributeAndValue(
       builder, die,
       StringConstant(DW_AT(name), builder->filename.value));
  AddAttributeAndValue(
       builder, die,
       StringConstant(DW_AT(comp_dir), builder->dir.value));
  AddAttributeAndValue(builder, die, SectionOffset(DW_AT(stmt_list), 0));
  AddAttributeAndValue(builder, die,
                       AddressConstant(DW_AT(low_pc), DW_FORM(addr), ".PCbegin"));
  AddAttributeAndValue(builder, die,
                        AddressConstant(DW_AT(high_pc), DW_FORM(high_pc), ".PCend-.PCbegin"));
  DIEAllocateAbbreviation(builder, die);
}

static void CompileUnitDIEPrint(DIE* die, int level) {
  printf("compile_unit\n");
}

static void CompileUnitDIEEmit(DebugBuilder* builder, DIE* die) {
  DIEBaseEmit(builder, die);
}

void DebugBuilderEmitDebugInfo(DebugBuilder* builder) {
  DIEBuild(builder, builder->compile_unit);
  FlushAccumulator(builder);
  if (builder->module != NULL) {
    AsmModuleSection(builder->module, ".debug_info", SHT(progbits), 0, 1);
    AsmModuleLabel(builder->module, ".DW_info_begin");
    AsmExpr length;
    AsmExprInitDifference(&length, ".DW_info_end", ".DW_info_begin", -4);
    AsmModuleInteger(builder->module, 4, &length);
    AsmExprDestruct(&length);
    DebugEmitConstant(builder, 2, 4);
    DebugEmitConstant(builder, 4, 0);
    DebugEmitConstant(builder, 1, compiler->pointer_size);
  } else {
    fprintf(builder->fp, "\n\t.section .debug_info,\"\",@progbits,1\n");
    fprintf(builder->fp, ".DW_info_begin:\n");
    fprintf(builder->fp, "\t.word .DW_info_end-.DW_info_begin-4\n");
    fprintf(builder->fp, "\t.short 4\n");
    fprintf(builder->fp, "\t.word 0\n");
    fprintf(builder->fp, "\t.byte %d\n", compiler->pointer_size);
  }
  
  // Write out the DIEs.
  for (size_t i = 0; i < builder->top_dies.length; i++) {
    FlushAccumulator(builder);
    DIE* die = builder->top_dies.value.p[i];
    assert(die->id >= 0);
    DIEEmit(builder, die);
  }
  EmitEndOfChildren(builder);
  FlushAccumulator(builder);
  DebugEmitLabel(builder, ".DW_info_end");

  // Emit string table.
  Buffer* strtab = &builder->string_table;
    if (builder->module != NULL) {
      AsmModuleSection(builder->module, ".debug_str", SHT(progbits),
                       SHF(merge) | SHF(strings), 1);
    } else {
      fprintf(builder->fp, "\n\t.section .debug_str,\"MS\",@progbits,1\n");
    }
    size_t index = 0;
    while (index < strtab->length) {
      // Output string and move to the end of it.
      if (builder->module != NULL) {
        AsmModuleString(builder->module, &strtab->value[index], true);
      } else {
        fprintf(builder->fp, "\t.string \"%s\"\n", &strtab->value[index]);
      }
      index += strlen(&strtab->value[index]) + 1;
    }
}
