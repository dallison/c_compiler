//
//  reflection_semantics.c
//  c_compiler
//

#include "reflection_semantics.h"

#include "compiler.h"
#include "constexpr.h"
#include "errors.h"
#include "expr_semantics.h"
#include "type_compare.h"

#include <string.h>

static ReflectionEntityKind ReflectionKindForSymbol(Symbol* symbol) {
  if (symbol == NULL) {
    return kReflectionInvalid;
  }
  if (StorageIs(symbol->storage, STO(typedef))) {
    if (symbol->type != NULL && TypeIsStructOrUnion(symbol->type) &&
        symbol->type->info.struct_info != NULL &&
        (symbol->type->info.struct_info->tag_symbol == symbol ||
         (symbol->type->info.struct_info->tag_name != NULL &&
          symbol->name.value != NULL &&
          strcmp(symbol->type->info.struct_info->tag_name->value,
                 symbol->name.value) == 0))) {
      return kReflectionType;
    }
    if (symbol->type != NULL && TypeIsEnum(symbol->type) &&
        symbol->type->info.enum_info != NULL &&
        (symbol->type->info.enum_info->tag_symbol == symbol ||
         (symbol->type->info.enum_info->tag_name != NULL &&
          symbol->name.value != NULL &&
          strcmp(symbol->type->info.enum_info->tag_name->value,
                 symbol->name.value) == 0))) {
      return kReflectionType;
    }
    return kReflectionTypeAlias;
  }
  if (symbol->flags.is_template || symbol->alias_template != NULL ||
      symbol->variable_template != NULL) {
    return kReflectionTemplate;
  }
  if (symbol->type != NULL && TypeIsFunction(symbol->type)) {
    return kReflectionFunction;
  }
  if (symbol->flags.value_set && symbol->storage == STO(implicit) &&
      !symbol->flags.is_constexpr) {
    return kReflectionEnumerator;
  }
  return kReflectionVariable;
}

static bool ReflectionOperandIsDependent(ASTNode* expression) {
  if (expression == NULL) {
    return false;
  }
  if (expression->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)expression)->symbol;
    if (symbol != NULL && symbol->flags.is_template_parameter) {
      return true;
    }
  }
  return (expression->flags &
          (kASTReferencesParameterPack | kASTDependentFunctorCall)) != 0 ||
         expression->type == NULL || TypeIsUnknown(expression->type) ||
         TypeContainsTemplateParameter(expression->type);
}

ReflectionValue* SemanticReflectionValueFromExpression(ASTNode* expression) {
  if (expression == NULL) {
    return NULL;
  }
  if (expression->op == AST_OP(reflection_constant)) {
    return ((ReflectionASTNode*)expression)->value;
  }
  if (expression->op == AST_OP(reflect)) {
    ReflectionASTNode* reflection = (ReflectionASTNode*)expression;
    if (reflection->value != NULL) {
      return reflection->value;
    }
  }
  if (expression->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)expression)->symbol;
    if (symbol != NULL && TypeIsReflection(symbol->type) &&
        symbol->flags.value_set && symbol->value.other != NULL) {
      return (ReflectionValue*)symbol->value.other;
    }
    if (symbol != NULL && TypeIsReflection(symbol->type) &&
        symbol->constexpr_initializer != NULL) {
      ASTNode* initializer =
          ConstexprInitializerExpression(symbol->constexpr_initializer);
      initializer = AnalyzeExpression(initializer);
      ReflectionValue* value =
          initializer != expression
              ? SemanticReflectionValueFromExpression(initializer)
              : NULL;
      if (value != NULL) {
        symbol->value.other = value;
        symbol->flags.value_set = true;
        return value;
      }
    }
  }
  return NULL;
}

ASTNode* SemanticAnalyzeReflection(ReflectionASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->base.op == AST_OP(reflection_constant) && node->value != NULL) {
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }

  ReflectionValue* value = NULL;
  switch (node->operand_kind) {
    case kReflectionOperandGlobalNamespace:
    case kReflectionOperandNamespace:
      value = ReflectionCreateNamespace(node->namespace_, node->base.location);
      break;
    case kReflectionOperandType:
      if (node->operand_type == NULL ||
          TypeContainsTemplateParameter(node->operand_type)) {
        node->base.flags |= kASTAnalyzed;
        return (ASTNode*)node;
      }
      value = ReflectionCreateType(node->operand_type, NULL,
                                   node->base.location);
      break;
    case kReflectionOperandExpression:
      if (node->operand == NULL) {
        SemanticError((ASTNode*)node, "Invalid reflection operand");
        value = ReflectionCreateInvalid(node->base.location);
        break;
      }
      if (node->operand->op == AST_OP(identifier)) {
        Symbol* symbol = ((IdentifierASTNode*)node->operand)->symbol;
        if (symbol == NULL) {
          node->base.flags |= kASTAnalyzed;
          return (ASTNode*)node;
        }
        if (symbol->flags.is_template_type_parameter) {
          if (node->operand->type == NULL ||
              TypeContainsTemplateParameter(node->operand->type)) {
            node->base.flags |= kASTAnalyzed;
            return (ASTNode*)node;
          }
          value = ReflectionCreateType(node->operand->type, NULL,
                                       node->base.location);
          break;
        }
        if (symbol->type != NULL &&
            TypeContainsTemplateParameter(symbol->type)) {
          node->base.flags |= kASTAnalyzed;
          return (ASTNode*)node;
        }
        if (symbol->flags.is_template_parameter) {
          node->base.flags |= kASTAnalyzed;
          return (ASTNode*)node;
        }
        ReflectionEntityKind kind = ReflectionKindForSymbol(symbol);
        value = kind == kReflectionType || kind == kReflectionTypeAlias
                    ? ReflectionCreateType(
                          symbol->type,
                          kind == kReflectionTypeAlias ? symbol : NULL,
                          node->base.location)
                    : ReflectionCreateSymbol(kind, symbol,
                                             node->base.location);
      } else if (node->operand->op == AST_OP(structmember)) {
        StructMemberASTNode* member =
            (StructMemberASTNode*)node->operand;
        Struct* parent =
            member->owner_type != NULL &&
                    TypeIsStructOrUnion(member->owner_type)
                ? member->owner_type->info.struct_info
                : member->member != NULL &&
                          member->member->symbol != NULL &&
                          member->member->symbol->type != NULL &&
                          TypeIsFunction(member->member->symbol->type)
                      ? member->member->symbol->type->info.function
                            .cxx_member_owner
                      : NULL;
        value = ReflectionCreateMember(member->member, parent,
                                       node->base.location);
      } else {
        SemanticError((ASTNode*)node,
                      "Reflection operand does not name an entity");
        value = ReflectionCreateInvalid(node->base.location);
      }
      break;
    case kReflectionOperandValue:
      value = node->value;
      break;
  }

  node->value = value;
  node->operand_kind = kReflectionOperandValue;
  node->base.op = AST_OP(reflection_constant);
  ASTNodeSetType((ASTNode*)node,
                 NewTypeRecordWithSize(kTypeReflection, kQualPlain));
  node->base.value_category = kValueCategoryPrvalue;
  node->base.flags |= kASTAnalyzed;
  return (ASTNode*)node;
}

static ASTNode* MaterializeReflectedEntity(ReflectionValue* value,
                                           SpliceContext context,
                                           SourceLocation location) {
  if (value == NULL || value->kind == kReflectionInvalid) {
    return NULL;
  }
  switch (value->kind) {
    case kReflectionVariable:
    case kReflectionFunction:
    case kReflectionEnumerator:
      return value->symbol != NULL
                 ? NewIdentifierASTNode(value->symbol, location)
                 : NULL;
    case kReflectionDataMember:
      if (value->member == NULL) {
        return NULL;
      }
      if (context == kSpliceAddressed) {
        ASTNode* member = NewStructMemberASTNode(value->member, location);
        return NewUnaryASTNode(AST_OP(member_ptr), NULL, location, member);
      }
      return NewStructMemberASTNode(value->member, location);
    default:
      return NULL;
  }
}

ASTNode* SemanticAnalyzeSplice(SpliceASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  node->reflection = AnalyzeExpression(node->reflection);
  ReflectionValue* value =
      SemanticReflectionValueFromExpression(node->reflection);
  if (value == NULL) {
    if (!ReflectionOperandIsDependent(node->reflection)) {
      SemanticError((ASTNode*)node,
                    "Splice operand is not a constant reflection");
    }
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                         kQualPlain));
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }

  ASTNode* result =
      MaterializeReflectedEntity(value, node->context, node->base.location);
  if (result == NULL) {
    SemanticError((ASTNode*)node,
                  "Reflection value cannot be used in this splice context");
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }
  return AnalyzeExpression(result);
}

ASTNode* SemanticAnalyzeAddressedSplice(UnaryASTNode* address) {
  if (address == NULL || address->sub == NULL ||
      address->sub->op != AST_OP(splice)) {
    return (ASTNode*)address;
  }
  SpliceASTNode* splice = (SpliceASTNode*)address->sub;
  splice->context = kSpliceAddressed;
  splice->reflection = AnalyzeExpression(splice->reflection);
  ReflectionValue* value =
      SemanticReflectionValueFromExpression(splice->reflection);
  if (value != NULL && value->kind == kReflectionDataMember) {
    ASTNode* result =
        MaterializeReflectedEntity(value, kSpliceAddressed,
                                   address->base.location);
    return result != NULL ? AnalyzeExpression(result) : (ASTNode*)address;
  }
  if (value == NULL &&
      !ReflectionOperandIsDependent(splice->reflection)) {
    SemanticError((ASTNode*)splice,
                  "Splice operand is not a constant reflection");
  }
  return (ASTNode*)address;
}

bool SemanticLowerMemberSplice(BinaryASTNode* access) {
  if (access == NULL || access->right == NULL ||
      access->right->op != AST_OP(splice)) {
    return false;
  }
  SpliceASTNode* splice = (SpliceASTNode*)access->right;
  splice->context = kSpliceMember;
  splice->reflection = AnalyzeExpression(splice->reflection);
  ReflectionValue* value =
      SemanticReflectionValueFromExpression(splice->reflection);
  if (value == NULL) {
    if (!ReflectionOperandIsDependent(splice->reflection)) {
      SemanticError((ASTNode*)splice,
                    "Splice operand is not a constant reflection");
    }
    ASTNodeSetType((ASTNode*)access,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                         kQualPlain));
    access->base.flags |= kASTAnalyzed;
    return true;
  }
  if (value->kind != kReflectionDataMember &&
      !(value->kind == kReflectionFunction && value->member != NULL) &&
      !(value->kind == kReflectionVariable && value->member != NULL)) {
    SemanticError((ASTNode*)splice,
                  "Member splice operand does not reflect a member");
    ASTNodeSetType((ASTNode*)access,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    access->base.flags |= kASTAnalyzed;
    return true;
  }
  access->right = NewStructMemberASTNode(value->member,
                                         splice->base.location);
  access->right->parent = (ASTNode*)access;
  access->right->child_id = 1;
  return false;
}

ASTNode* SemanticAnalyzeReflectionComparison(BinaryASTNode* comparison) {
  comparison->left = AnalyzeExpression(comparison->left);
  comparison->right = AnalyzeExpression(comparison->right);
  ReflectionValue* left =
      SemanticReflectionValueFromExpression(comparison->left);
  ReflectionValue* right =
      SemanticReflectionValueFromExpression(comparison->right);
  if (left == NULL || right == NULL) {
    ASTNodeSetType((ASTNode*)comparison,
                   NewTypeRecordWithSize(kTypeBool, kQualPlain));
    comparison->base.flags |= kASTAnalyzed;
    return (ASTNode*)comparison;
  }
  bool equal = ReflectionValueEqual(left, right);
  if (comparison->base.op == AST_OP(noteq)) {
    equal = !equal;
  }
  return NewIntConstantASTNode(
      equal ? 1 : 0,
      NewTypeRecordWithSize(kTypeBool, kQualConst),
      comparison->base.location);
}

static Symbol* MetaCallSymbol(VectorASTNode* call) {
  if (call == NULL || call->left == NULL ||
      call->left->op != AST_OP(identifier)) {
    return NULL;
  }
  Symbol* symbol = ((IdentifierASTNode*)call->left)->symbol;
  for (size_t depth = 0;
       symbol != NULL && symbol->alias_target != NULL && depth < 64;
       depth++) {
    symbol = symbol->alias_target;
  }
  if (symbol == NULL ||
      !SymbolHasAttribute(symbol, "meta_intrinsic")) {
    return NULL;
  }
  return symbol;
}

typedef enum {
  kMetaUnknown,
  kMetaBasesOf,
  kMetaDealias,
  kMetaDisplayStringOf,
  kMetaEnumeratorsOf,
  kMetaHasIdentifier,
  kMetaIdentifierOf,
  kMetaIsDataMember,
  kMetaIsEnumerator,
  kMetaIsFunction,
  kMetaIsInvalid,
  kMetaIsNamespace,
  kMetaIsPrivate,
  kMetaIsProtected,
  kMetaIsPublic,
  kMetaIsTemplate,
  kMetaIsType,
  kMetaIsTypeAlias,
  kMetaIsVariable,
  kMetaMembersOf,
  kMetaNonstaticDataMembersOf,
  kMetaParentOf,
  kMetaTypeOf,
} MetaOperation;

typedef struct {
  const char* name;
  MetaOperation operation;
} MetaOperationEntry;

// Keep this table sorted by name for MetaOperationForName's binary search.
static const MetaOperationEntry kMetaOperations[] = {
    {"bases_of", kMetaBasesOf},
    {"dealias", kMetaDealias},
    {"display_string_of", kMetaDisplayStringOf},
    {"enumerators_of", kMetaEnumeratorsOf},
    {"has_identifier", kMetaHasIdentifier},
    {"identifier_of", kMetaIdentifierOf},
    {"is_data_member", kMetaIsDataMember},
    {"is_enumerator", kMetaIsEnumerator},
    {"is_function", kMetaIsFunction},
    {"is_invalid", kMetaIsInvalid},
    {"is_namespace", kMetaIsNamespace},
    {"is_private", kMetaIsPrivate},
    {"is_protected", kMetaIsProtected},
    {"is_public", kMetaIsPublic},
    {"is_template", kMetaIsTemplate},
    {"is_type", kMetaIsType},
    {"is_type_alias", kMetaIsTypeAlias},
    {"is_variable", kMetaIsVariable},
    {"members_of", kMetaMembersOf},
    {"nonstatic_data_members_of", kMetaNonstaticDataMembersOf},
    {"parent_of", kMetaParentOf},
    {"type_of", kMetaTypeOf},
};

static MetaOperation MetaOperationForName(const char* name) {
  size_t begin = 0;
  size_t end = sizeof(kMetaOperations) / sizeof(kMetaOperations[0]);
  while (begin < end) {
    size_t middle = begin + (end - begin) / 2;
    int comparison = strcmp(name, kMetaOperations[middle].name);
    if (comparison == 0) {
      return kMetaOperations[middle].operation;
    }
    if (comparison < 0) {
      end = middle;
    } else {
      begin = middle + 1;
    }
  }
  return kMetaUnknown;
}

static ASTNode* NewMetaBool(bool value, SourceLocation location) {
  return NewIntConstantASTNode(
      value ? 1 : 0,
      NewTypeRecordWithSize(kTypeBool, kQualConst), location);
}

static ASTNode* NewMetaString(const char* text, SourceLocation location) {
  String* contents = NewString(text != NULL ? text : "");
  TypeRecord* array = NewBasicArrayTypeRecord(
      kQualPlain, (int)contents->length + 1, false);
  TypeRecordChain(
      array, NewTypeRecordWithSize(kTypeChar, kQualConst));
  TypeRecordCalculateSize(array);
  return NewStringConstantASTNode(contents, array, location);
}

static CXXAccess ReflectionAccess(const ReflectionValue* value) {
  if (value != NULL && value->member != NULL) {
    return value->member->access;
  }
  if (value != NULL && value->kind == kReflectionBase &&
      value->parent_class != NULL &&
      value->base_index < value->parent_class->bases.length) {
    CXXBaseSpecifier* base =
        value->parent_class->bases.value.p[value->base_index];
    return base != NULL ? base->access : kAccessPublic;
  }
  return kAccessPublic;
}

static ReflectionValue* MetaTypeOf(ReflectionValue* value,
                                   SourceLocation location) {
  if (value == NULL) {
    return NULL;
  }
  if (value->kind == kReflectionType) {
    return value;
  }
  return value->reflected_type != NULL
             ? ReflectionCreateType(value->reflected_type, NULL, location)
             : NULL;
}

static ReflectionValue* MetaParentOf(ReflectionValue* value,
                                     SourceLocation location) {
  if (value == NULL) {
    return NULL;
  }
  if (value->parent_class != NULL && value->parent_class->tag_symbol != NULL) {
    return ReflectionCreateType(value->parent_class->tag_symbol->type, NULL,
                                location);
  }
  if (value->symbol != NULL && value->symbol->type != NULL &&
      TypeIsFunction(value->symbol->type) &&
      value->symbol->type->info.function.cxx_member_owner != NULL) {
    Struct* owner =
        value->symbol->type->info.function.cxx_member_owner;
    return owner->tag_symbol != NULL
               ? ReflectionCreateType(owner->tag_symbol->type, NULL, location)
               : NULL;
  }
  Namespace* namespace_ =
      value->namespace_ != NULL
          ? value->namespace_
          : value->symbol != NULL ? value->symbol->namespace_ : NULL;
  if ((value->kind == kReflectionNamespace ||
       value->kind == kReflectionGlobalNamespace) &&
      namespace_ != NULL) {
    namespace_ = namespace_->parent;
  }
  return namespace_ != NULL
             ? ReflectionCreateNamespace(namespace_, location)
             : NULL;
}

static bool MetaMemberVisible(Struct* owner, StructMember* member,
                              bool include_private) {
  return member != NULL &&
         (include_private || member->access == kAccessPublic ||
          compiler->current_class_access_context == owner);
}

static Vector* MetaCollectRange(MetaOperation operation, ReflectionValue* value,
                                SourceLocation location,
                                bool include_private) {
  Vector* values = NewVector();
  if (value == NULL || value->reflected_type == NULL) {
    return values;
  }
  if (operation == kMetaEnumeratorsOf) {
    if (!TypeIsEnum(value->reflected_type) ||
        value->reflected_type->info.enum_info == NULL) {
      return values;
    }
    Enum* enumeration = value->reflected_type->info.enum_info;
    for (size_t i = 0; i < enumeration->constants.length; i++) {
      Symbol* enumerator = enumeration->constants.value.p[i];
      VectorAppend(values,
                   ReflectionCreateSymbol(kReflectionEnumerator, enumerator,
                                          location));
    }
    return values;
  }
  if (!TypeIsStructOrUnion(value->reflected_type) ||
      value->reflected_type->info.struct_info == NULL) {
    return values;
  }
  Struct* owner = value->reflected_type->info.struct_info;
  if (operation == kMetaBasesOf) {
    for (size_t i = 0; i < owner->bases.length; i++) {
      CXXBaseSpecifier* base = owner->bases.value.p[i];
      if (base != NULL &&
          (include_private || base->access == kAccessPublic ||
           compiler->current_class_access_context == owner)) {
        VectorAppend(values, ReflectionCreateBase(owner, i, location));
      }
    }
    return values;
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (!MetaMemberVisible(owner, member, include_private) ||
        member->symbol == NULL) {
      continue;
    }
    if (operation == kMetaNonstaticDataMembersOf &&
        (member->is_static || TypeIsFunction(member->symbol->type))) {
      continue;
    }
    VectorAppend(values, ReflectionCreateMember(member, owner, location));
  }
  return values;
}

static ASTNode* MetaRangeExpression(VectorASTNode* call, Vector* values) {
  Vector* array_values = NewVector();
  for (size_t i = 0; i < values->length; i++) {
    ReflectionValue* value = values->value.p[i];
    VectorAppend(array_values,
                 NewReflectionConstantASTNode(value,
                                              call->base.location));
  }
  ASTNode* array = NewBracedInitializerASTNode(
      array_values, NULL, call->base.location);
  Vector* aggregate_values = NewVector();
  VectorAppend(aggregate_values,
               NewIntConstantASTNode(
                   (int64_t)values->length, NewSizeTypeRecord(),
                   call->base.location));
  VectorAppend(aggregate_values, array);
  VectorDelete(values);
  ASTNode* aggregate = NewBracedInitializerASTNode(
      aggregate_values, NULL, call->base.location);
  TypeRecord* return_type =
      call->left != NULL && call->left->type != NULL &&
              TypeIsFunction(call->left->type)
          ? call->left->type->next
          : NULL;
  if (return_type == NULL) {
    return aggregate;
  }
  return LowerCXXBracedInitToTarget(aggregate, return_type);
}

ASTNode* SemanticTryAnalyzeMetaCall(VectorASTNode* call) {
  Symbol* function = MetaCallSymbol(call);
  if (function == NULL) {
    return NULL;
  }
  const char* name = function->name.value;
  if (name == NULL || call->children == NULL ||
      call->children->length == 0) {
    return NULL;
  }
  MetaOperation operation = MetaOperationForName(name);
  if (operation == kMetaUnknown) {
    return NULL;
  }
  ReflectionValue* value = SemanticReflectionValueFromExpression(
      call->children->value.p[0]);
  if (value == NULL) {
    ConstEvalContext context;
    ConstEvalContextInit(&context);
    value = ConstexprEvaluateReflectionExpression(
        &context, call->children->value.p[0]);
    ConstEvalContextDestruct(&context);
  }
  if (value == NULL) {
    ASTNodeSetType((ASTNode*)call, function->type->next);
    call->base.flags |= kASTAnalyzed | kASTDependentFunctorCall;
    return (ASTNode*)call;
  }

  ReflectionValue* result = NULL;
  switch (operation) {
    case kMetaIsInvalid:
      return NewMetaBool(value->kind == kReflectionInvalid,
                         call->base.location);
    case kMetaIsNamespace:
      return NewMetaBool(value->kind == kReflectionNamespace ||
                             value->kind == kReflectionGlobalNamespace,
                         call->base.location);
    case kMetaIsType:
      return NewMetaBool(value->kind == kReflectionType ||
                             value->kind == kReflectionTypeAlias ||
                             value->kind == kReflectionBase,
                         call->base.location);
    case kMetaIsTypeAlias:
      return NewMetaBool(value->kind == kReflectionTypeAlias,
                         call->base.location);
    case kMetaIsVariable:
      return NewMetaBool(value->kind == kReflectionVariable,
                         call->base.location);
    case kMetaIsFunction:
      return NewMetaBool(value->kind == kReflectionFunction,
                         call->base.location);
    case kMetaIsDataMember:
      return NewMetaBool(value->kind == kReflectionDataMember,
                         call->base.location);
    case kMetaIsEnumerator:
      return NewMetaBool(value->kind == kReflectionEnumerator,
                         call->base.location);
    case kMetaIsTemplate:
      return NewMetaBool(value->kind == kReflectionTemplate,
                         call->base.location);
    case kMetaIsPublic:
      return NewMetaBool(ReflectionAccess(value) == kAccessPublic,
                         call->base.location);
    case kMetaIsProtected:
      return NewMetaBool(ReflectionAccess(value) == kAccessProtected,
                         call->base.location);
    case kMetaIsPrivate:
      return NewMetaBool(ReflectionAccess(value) == kAccessPrivate,
                         call->base.location);
    case kMetaHasIdentifier:
      return NewMetaBool(ReflectionValueIdentifier(value) != NULL,
                         call->base.location);
    case kMetaIdentifierOf:
    case kMetaDisplayStringOf:
      return NewMetaString(ReflectionValueIdentifier(value),
                           call->base.location);
    case kMetaTypeOf:
      result = MetaTypeOf(value, call->base.location);
      break;
    case kMetaParentOf:
      result = MetaParentOf(value, call->base.location);
      break;
    case kMetaDealias:
      result = value->kind == kReflectionTypeAlias
                   ? ReflectionCreateType(value->reflected_type, NULL,
                                          call->base.location)
                   : value;
      break;
    case kMetaBasesOf:
    case kMetaEnumeratorsOf:
    case kMetaMembersOf:
    case kMetaNonstaticDataMembersOf:
      break;
    case kMetaUnknown:
      return NULL;
  }
  if (result != NULL) {
    return NewReflectionConstantASTNode(result, call->base.location);
  }
  if (operation == kMetaMembersOf ||
      operation == kMetaNonstaticDataMembersOf ||
      operation == kMetaBasesOf ||
      operation == kMetaEnumeratorsOf) {
    bool include_private = false;
    if (call->children->length > 1) {
      int64_t access = 0;
      include_private = ConstexprEvaluateObjectSlotInteger(
                            call->children->value.p[1], 0, &access) &&
                        access != 0;
    }
    return MetaRangeExpression(
        call, MetaCollectRange(operation, value, call->base.location,
                               include_private));
  }
  return NULL;
}
