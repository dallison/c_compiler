//
//  type_traits_semantics.c
//  c_compiler
//
//  Compile-time type trait evaluation for DaveCC builtins.
//

#include "type_traits_semantics.h"

#include <string.h>

#include "compiler.h"
#include "concepts.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "init_semantics.h"
#include "type_compare.h"
#include "type_inheritance.h"
#include "type_template.h"
#include "type_member.h"
#include "type_template_internal.h"
#include "syntax.h"

extern bool (*type_ranks[])(TypeRecord*);

static Symbol* g_invoke_result_placeholder_origin;
static Symbol* g_common_type_placeholder_origin;

static Symbol* InvokeResultPlaceholderOrigin(void) {
  if (g_invoke_result_placeholder_origin == NULL) {
    g_invoke_result_placeholder_origin =
        NewSymbol("__davecc_invoke_result_t", NULL, STO(typedef));
    g_invoke_result_placeholder_origin->flags.invented = true;
  }
  return g_invoke_result_placeholder_origin;
}

static Symbol* CommonTypePlaceholderOrigin(void) {
  if (g_common_type_placeholder_origin == NULL) {
    g_common_type_placeholder_origin =
        NewSymbol("__davecc_common_type_t", NULL, STO(typedef));
    g_common_type_placeholder_origin->flags.invented = true;
  }
  return g_common_type_placeholder_origin;
}

bool TypeRecordIsInvokeResultPlaceholder(TypeRecord* type) {
  return type != NULL && type->template_origin == InvokeResultPlaceholderOrigin() &&
         type->template_arguments != NULL;
}

static Vector* TypeVectorFromTemplateArguments(Vector* template_args) {
  Vector* types = NewVector();
  if (template_args == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < template_args->length; i++) {
    TemplateArgument* arg = template_args->value.p[i];
    if (arg == NULL || arg->kind != kTemplateParameterType || arg->type == NULL) {
      VectorDelete(types);
      return NULL;
    }
    VectorAppend(types, arg->type);
  }
  return types;
}

static Vector* TemplateArgumentsFromTypeVector(Vector* types,
                                                    Vector* pack_flags) {
  Vector* args = NewVector();
  if (types == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < types->length; i++) {
    TemplateArgument* arg = malloc(sizeof(TemplateArgument));
    arg->kind = kTemplateParameterType;
    arg->is_pack_expansion =
        pack_flags != NULL && i < pack_flags->length &&
        pack_flags->value.p[i] != NULL;
    arg->type = TypeRecordCopy((TypeRecord*)types->value.p[i]);
    arg->int_value = 0;
    arg->template_parameter_index = arg->type->template_parameter_index;
    arg->pack_arguments = NULL;
    arg->dependent_expr = NULL;
    arg->location = SOURCE_LOCATION_MISSING;
    VectorAppend(args, arg);
  }
  return args;
}

TypeRecord* TypeRecordSubstituteInvokeResultPlaceholder(TypeParser* parser,
                                                        TypeRecord* type,
                                                        Vector* args) {
  if (!TypeRecordIsInvokeResultPlaceholder(type) ||
      parser->template_substitution_failed) {
    return NULL;
  }
  Vector* substituted =
      SubstituteTemplateArgumentVector(parser, type->template_arguments, args,
                                       0);
  if (substituted == NULL || parser->template_substitution_failed) {
    if (substituted != NULL) {
      VectorDeleteWithContents(
          substituted, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    return NULL;
  }
  Vector* types = TypeVectorFromTemplateArguments(substituted);
  VectorDeleteWithContents(substituted,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (types == NULL) {
    return NULL;
  }
  TypeRecord* result = CXXTypeTraitInvokeResultType(parser->syntax, types);
  VectorDelete(types);
  if (result == NULL) {
    return NULL;
  }
  result->qualifiers |= type->qualifiers;
  return TypeRecordCalculateSize(result);
}

TypeRecord* TypeRecordNewInvokeResultPlaceholderWithFlags(Vector* type_args,
                                                          Vector* pack_flags) {
  if (type_args == NULL) {
    return NULL;
  }
  Vector* template_args = TemplateArgumentsFromTypeVector(type_args, pack_flags);
  if (template_args == NULL) {
    return NULL;
  }
  TypeRecord* placeholder =
      NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  placeholder->template_origin = InvokeResultPlaceholderOrigin();
  placeholder->template_arguments = template_args;
  return placeholder;
}

TypeRecord* TypeRecordNewInvokeResultPlaceholder(Vector* type_args) {
  return TypeRecordNewInvokeResultPlaceholderWithFlags(type_args, NULL);
}

bool TypeRecordIsCommonTypePlaceholder(TypeRecord* type) {
  return type != NULL && type->template_origin == CommonTypePlaceholderOrigin() &&
         type->template_arguments != NULL;
}

TypeRecord* TypeRecordSubstituteCommonTypePlaceholder(TypeParser* parser,
                                                      TypeRecord* type,
                                                      Vector* args) {
  if (!TypeRecordIsCommonTypePlaceholder(type) ||
      parser->template_substitution_failed) {
    return NULL;
  }
  Vector* substituted =
      SubstituteTemplateArgumentVector(parser, type->template_arguments, args,
                                       0);
  if (substituted == NULL || parser->template_substitution_failed) {
    if (substituted != NULL) {
      VectorDeleteWithContents(
          substituted, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    return NULL;
  }
  Vector* types = TypeVectorFromTemplateArguments(substituted);
  VectorDeleteWithContents(substituted,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (types == NULL) {
    return NULL;
  }
  TypeRecord* result = CXXTypeTraitCommonType(parser->syntax, types);
  VectorDelete(types);
  if (result == NULL) {
    return NULL;
  }
  result->qualifiers |= type->qualifiers;
  return TypeRecordCalculateSize(result);
}

TypeRecord* TypeRecordNewCommonTypePlaceholder(Vector* type_args) {
  if (type_args == NULL) {
    return NULL;
  }
  Vector* template_args = TemplateArgumentsFromTypeVector(type_args, NULL);
  if (template_args == NULL) {
    return NULL;
  }
  TypeRecord* placeholder =
      NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  placeholder->template_origin = CommonTypePlaceholderOrigin();
  placeholder->template_arguments = template_args;
  return placeholder;
}

static bool TypeVectorHasDependentTemplateParameter(Vector* types) {
  if (types == NULL) {
    return true;
  }
  for (size_t i = 0; i < types->length; i++) {
    TypeRecord* type = (TypeRecord*)types->value.p[i];
    if (type == NULL || type->template_parameter_index >= 0 ||
        (type->type & kTypeUnknown) != 0) {
      return true;
    }
  }
  return false;
}

static TypeRecord* TypeTraitCopyOwnedSpine(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  TypeRecord* copy = TypeRecordCopy(type);
  if (type->next != NULL) {
    TypeRecordDelete(copy->next);
    copy->next = TypeTraitCopyOwnedSpine(type->next);
    TypeRecordIncRef(copy->next);
  }
  return TypeRecordCalculateSize(copy);
}

static void TypeTraitStripAllQualifiers(TypeRecord* type) {
  for (TypeRecord* cur = type; cur != NULL; cur = cur->next) {
    cur->qualifiers = kQualPlain;
  }
}

static TypeRecord* TypeRecordStripCvRefForTraitPlaceholder(TypeRecord* type) {
  TypeRecord* owned = TypeTraitCopyOwnedSpine(type);
  if (owned == NULL) {
    return NULL;
  }
  TypeRecord* cur = owned;
  while (cur != NULL) {
    TypeTraitStripAllQualifiers(cur);
    if (TypeIsReference(cur) || TypeIsArray(cur)) {
      cur = cur->next;
      continue;
    }
    break;
  }
  return owned;
}

TypeRecord* TypeRecordTryResolveTraitPlaceholder(Syntax* syntax,
                                                 TypeRecord* type) {
  if (syntax == NULL || type == NULL) {
    return NULL;
  }
  TypeRecord* copy = TypeRecordCopy(type);
  for (TypeRecord* cur = copy; cur != NULL; cur = cur->next) {
    if (TypeRecordIsCommonTypePlaceholder(cur)) {
      Vector* types = TypeVectorFromTemplateArguments(cur->template_arguments);
      TypeRecord* resolved = NULL;
      if (types != NULL && !TypeVectorHasDependentTemplateParameter(types)) {
        TypeRecord* common = CXXTypeTraitCommonType(syntax, types);
        if (common != NULL) {
          resolved = TypeTraitCopyOwnedSpine(common);
          if (resolved != NULL) {
            TypeTraitStripAllQualifiers(resolved);
          }
        }
      }
      VectorDelete(types);
      TypeRecordDelete(copy);
      return resolved;
    }
    if (TypeRecordIsInvokeResultPlaceholder(cur)) {
      Vector* types = TypeVectorFromTemplateArguments(cur->template_arguments);
      TypeRecord* resolved = NULL;
      if (types != NULL && !TypeVectorHasDependentTemplateParameter(types)) {
        resolved = CXXTypeTraitInvokeResultType(syntax, types);
        if (resolved != NULL) {
          resolved = TypeRecordCalculateSize(resolved);
        }
      }
      VectorDelete(types);
      TypeRecordDelete(copy);
      return resolved;
    }
  }
  TypeRecordDelete(copy);
  return NULL;
}

static SourceLocation kTypeTraitLocation;

static ASTNode* TypeTraitIdentityCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static bool TypeEqualIgnoringQualifiers(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  TypeRecord* plain_left = TypeTraitCopyOwnedSpine(left);
  TypeRecord* plain_right = TypeTraitCopyOwnedSpine(right);
  if (plain_left == NULL || plain_right == NULL) {
    TypeRecordDelete(plain_left);
    TypeRecordDelete(plain_right);
    return false;
  }
  TypeTraitStripAllQualifiers(plain_left);
  TypeTraitStripAllQualifiers(plain_right);
  bool equal = TypeEqual(plain_left, plain_right);
  TypeRecordDelete(plain_left);
  TypeRecordDelete(plain_right);
  return equal;
}

static TypeRecord* MaterializeTraitType(Syntax* syntax, TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  return TypeMaterializeClassTemplateSpecialization(syntax, type);
}

static ASTNode* NewSyntheticValue(Syntax* syntax, TypeRecord* type) {
  type = MaterializeTraitType(syntax, type);
  if (type == NULL) {
    return NULL;
  }
  type = TypeRecordCopy(type);
  Symbol* temp = SyntaxNewTemporary(syntax, type);
  temp->location = kTypeTraitLocation;
  ASTNode* id = NewIdentifierASTNode(temp, kTypeTraitLocation);
  id->value_category = kValueCategoryPrvalue;
  return id;
}

static ASTNode* NewSyntheticLvalue(Syntax* syntax, TypeRecord* type) {
  type = MaterializeTraitType(syntax, type);
  if (type == NULL) {
    return NULL;
  }
  type = TypeRecordCopy(type);
  Symbol* temp = SyntaxNewTemporary(syntax, type);
  temp->location = kTypeTraitLocation;
  ASTNode* id = NewIdentifierASTNode(temp, kTypeTraitLocation);
  id->flags |= kASTNeedAddress;
  id->value_category = kValueCategoryLvalue;
  return id;
}

static ASTNode* TypeTraitSyntheticExpressionFromType(Syntax* syntax,
                                                     TypeRecord* type) {
  type = MaterializeTraitType(syntax, type);
  if (type == NULL) {
    return NULL;
  }
  if (TypeIsReference(type)) {
    TypeRecord* object_type = TypeTraitCopyOwnedSpine(type->next);
    if (object_type == NULL) {
      return NULL;
    }
    object_type->qualifiers |=
        (type->qualifiers & (kQualConst | kQualVolatile));
    ASTNode* expr = NULL;
    if (type->declarator == kDeclRValueReference) {
      expr = NewSyntheticValue(syntax, object_type);
      if (expr != NULL) {
        expr->value_category = kValueCategoryXvalue;
      }
    } else {
      expr = NewSyntheticLvalue(syntax, object_type);
    }
    TypeRecordDelete(object_type);
    return expr;
  }
  return NewSyntheticValue(syntax, type);
}

static ASTNode* NewSyntheticTypeCallee(Syntax* syntax, TypeRecord* type) {
  type = MaterializeTraitType(syntax, type);
  if (type == NULL) {
    return NULL;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
      type->info.struct_info->tag_name != NULL) {
    Symbol* tag = SyntaxFindSymbol(syntax, type->info.struct_info->tag_name);
    if (tag != NULL) {
      ASTNode* id = NewIdentifierASTNode(tag, kTypeTraitLocation);
      id->type = TypeRecordCopy(type);
      return id;
    }
  }
  Symbol* invented = NewSymbol("", TypeRecordCopy(type), STO(typedef));
  invented->flags.invented = true;
  invented->location = kTypeTraitLocation;
  return NewIdentifierASTNode(invented, kTypeTraitLocation);
}

static bool TryAnalyzeExpression(ASTNode* expr) {
  if (expr == NULL) {
    return false;
  }
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  ASTNode* analyzed = AnalyzeExpression(expr);
  bool ok = analyzed != NULL && !DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  if (analyzed != NULL) {
    ASTNodeDelete(analyzed);
  }
  return ok;
}

static bool TryConvertImplicit(ASTNode* from, TypeRecord* to);

static bool TypeTraitCanBindToReference(ASTNode* from, TypeRecord* to) {
  if (from == NULL || from->type == NULL || !TypeIsReference(to) ||
      to->next == NULL) {
    return false;
  }
  if (TypeIsConst(from->type) && !TypeIsConst(to->next)) {
    return false;
  }
  TypeRecord* source = from->type;
  if (TypeIsReference(source)) {
    source = source->next;
  }
  TypeRecord* target = to->next;
  if (to->declarator == kDeclRValueReference) {
    if (from->value_category != kValueCategoryXvalue &&
        from->value_category != kValueCategoryPrvalue) {
      return false;
    }
  } else if (from->value_category != kValueCategoryLvalue &&
             from->value_category != kValueCategoryXvalue) {
    if (!TypeIsConst(target)) {
      return false;
    }
  }
  if (TypeEqualIgnoringQualifiers(source, target)) {
    return true;
  }
  if (TypeIsStructOrUnion(source) && TypeIsStructOrUnion(target)) {
    if (TypeIsDerivedFrom(source, target)) {
      return true;
    }
  }
  // A const lvalue reference or an rvalue reference can bind to a materialized
  // temporary of the referenced type, so binding also succeeds when the source
  // is implicitly convertible to that (cv-unqualified) referenced type
  // (e.g. `const long& r = intValue;` converts the int to a long temporary).
  bool binds_to_temporary =
      to->declarator == kDeclRValueReference ||
      (to->declarator == kDeclReference && TypeIsConst(target));
  if (binds_to_temporary) {
    TypeRecord* value_target = TypeRecordCopy(target);
    value_target->qualifiers = kQualPlain;
    ASTNode* prvalue = NewSyntheticValue(&compiler->syntax, source);
    bool ok = false;
    if (prvalue != NULL) {
      ok = TryConvertImplicit(prvalue, value_target);
      ASTNodeDelete(prvalue);
    }
    TypeRecordDelete(value_target);
    return ok;
  }
  return false;
}

static bool TryConvertImplicit(ASTNode* from, TypeRecord* to) {
  if (from == NULL || to == NULL) {
    return false;
  }
  if (TypeEqual(from->type, to) || TypeTraitCanBindToReference(from, to)) {
    return true;
  }
  from = ASTNodeClone(from, TypeTraitIdentityCloneNode, NULL, NULL);
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  SemanticConvertType(from, TypeRecordCopy(to), kConvertNormal);
  bool ok = !DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  ASTNodeDelete(from);
  return ok;
}

static bool TypeIsVoidLike(TypeRecord* type) {
  return type != NULL && TypeIsPrimitive(type) && (type->type & kTypeVoid) != 0;
}

static bool TypeIsAbstractArray(TypeRecord* type) {
  return type != NULL && TypeIsArray(type) && type->info.array.is_vla;
}

static bool TypeIsReferenceConstructibleFrom(Syntax* syntax, TypeRecord* target,
                                             Vector* arg_types) {
  if (!TypeIsReference(target) || arg_types->length != 1) {
    return false;
  }
  TypeRecord* arg_type =
      MaterializeTraitType(syntax, (TypeRecord*)arg_types->value.p[0]);
  if (arg_type == NULL) {
    return false;
  }
  ASTNode* arg = TypeTraitSyntheticExpressionFromType(syntax, arg_type);
  if (arg == NULL) {
    return false;
  }
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  bool ok = TypeTraitCanBindToReference(arg, target);
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  ASTNodeDelete(arg);
  return ok && !DiagnosticErrorTrapped();
}

static StructMember* TypeTraitFindSpecialMember(
    Struct* str, CXXSpecialMemberKind kind) {
  if (str == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    for (StructMember* member = str->members.value.p[i]; member != NULL;
         member = member->overload_next) {
      TypeRecord* func =
          member->is_member_function && member->symbol != NULL
              ? member->symbol->type
              : NULL;
      if (func != NULL && TypeIsFunction(func) &&
          func->info.function.cxx_special_member_kind == kind) {
        return member;
      }
    }
  }
  return NULL;
}

static bool TypeTraitSameClassSpecialMemberConstruction(
    TypeRecord* target, Vector* arg_types, bool check_nothrow, bool* handled) {
  *handled = false;
  if (!TypeIsStructOrUnion(target) || target->info.struct_info == NULL ||
      arg_types == NULL || arg_types->length != 1) {
    return false;
  }
  TypeRecord* arg = arg_types->value.p[0];
  bool rvalue = !TypeIsReference(arg) ||
                arg->declarator == kDeclRValueReference;
  TypeRecord* object = TypeIsReference(arg) ? arg->next : arg;
  if (object == NULL || !TypeIsStructOrUnion(object) ||
      object->info.struct_info != target->info.struct_info) {
    return false;
  }
  *handled = true;
  // A const source cannot bind to a move constructor's `T&&` parameter, so a
  // const rvalue (e.g. `const T`) selects the copy constructor (`const T&`)
  // just like a const lvalue does.  Only a non-const rvalue uses the move ctor.
  bool use_move = rvalue && !TypeIsConst(object);
  CXXSpecialMemberKind kind =
      use_move ? kCXXSpecialMemberMoveConstructor
               : kCXXSpecialMemberCopyConstructor;
  StructMember* member =
      TypeTraitFindSpecialMember(target->info.struct_info, kind);
  if (member == NULL && use_move) {
    member = TypeTraitFindSpecialMember(target->info.struct_info,
                                        kCXXSpecialMemberCopyConstructor);
  }
  if (member == NULL || member->symbol == NULL ||
      member->symbol->type == NULL ||
      member->access != kAccessPublic ||
      member->symbol->type->info.function.is_deleted) {
    return false;
  }
  if (TypeIsConst(object)) {
    TypeRecord* source_type = NULL;
    FunctionInfo* info = &member->symbol->type->info.function;
    for (size_t i = 0; i < info->prototype.length; i++) {
      Symbol* formal = info->prototype.value.p[i];
      TypeRecord* formal_object =
          formal != NULL && TypeIsReference(formal->type)
              ? formal->type->next
              : NULL;
      if (formal_object != NULL && TypeIsStructOrUnion(formal_object) &&
          formal_object->info.struct_info == target->info.struct_info) {
        source_type = formal->type;
        break;
      }
    }
    TypeRecord* source_object =
        source_type != NULL && TypeIsReference(source_type)
            ? source_type->next
            : source_type;
    if (source_object == NULL || !TypeIsConst(source_object)) {
      return false;
    }
  }
  return !check_nothrow ||
         member->symbol->type->info.function.is_noexcept;
}

static bool TypeTraitIsConstructible(Syntax* syntax, Vector* type_args,
                                     bool check_nothrow) {
  if (type_args == NULL || type_args->length == 0) {
    return false;
  }
  TypeRecord* target =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[0]);
  if (target == NULL || TypeIsVoidLike(target) || TypeIsAbstractArray(target) ||
      TypeIsFunction(target)) {
    return false;
  }
  Vector arg_types;
  VectorInit(&arg_types);
  for (size_t i = 1; i < type_args->length; i++) {
    VectorAppend(&arg_types, type_args->value.p[i]);
  }

  bool result = false;
  bool handled_special_member = false;
  result = TypeTraitSameClassSpecialMemberConstruction(
      target, &arg_types, check_nothrow, &handled_special_member);
  if (handled_special_member) {
    VectorDestruct(&arg_types);
    return result;
  }
  if (TypeIsReference(target)) {
    result = TypeIsReferenceConstructibleFrom(syntax, target, &arg_types);
    VectorDestruct(&arg_types);
    return result;
  }

  ASTNode* callee = NewSyntheticTypeCallee(syntax, target);
  Vector* actuals = NewVector();
  for (size_t i = 0; i < arg_types.length; i++) {
    ASTNode* arg =
        NewSyntheticValue(syntax, (TypeRecord*)arg_types.value.p[i]);
    if (arg == NULL) {
      VectorDestruct(&arg_types);
      VectorDelete(actuals);
      ASTNodeDelete(callee);
      return false;
    }
    VectorAppend(actuals, arg);
  }
  VectorDestruct(&arg_types);

  ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, kTypeTraitLocation,
                                   callee, actuals);
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  ASTNode* analyzed = AnalyzeExpression(call);
  result = analyzed != NULL && !DiagnosticErrorTrapped();
  if (result && check_nothrow) {
    // is_nothrow_constructible is `noexcept(T(args...))`: nothrow iff no
    // potentially-evaluated call in the whole construction expression can
    // throw.  Class construction analyzes to a comma expression wrapping the
    // constructor call (and any argument/subobject initialization), not a bare
    // call node, so scan the analyzed expression rather than special-casing a
    // top-level call.
    result = !ExpressionPotentiallyThrows(analyzed);
  }
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  ASTNodeDelete(analyzed);
  return result;
}

static bool TypeTraitIsConvertible(Syntax* syntax, Vector* type_args) {
  if (type_args == NULL || type_args->length != 2) {
    return false;
  }
  TypeRecord* from =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[0]);
  TypeRecord* to =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[1]);
  if (from == NULL || to == NULL) {
    TypeRecordDelete(from);
    TypeRecordDelete(to);
    return false;
  }
  from = TypeRecordCopy(from);
  to = TypeRecordCopy(to);
  if (from == NULL || to == NULL) {
    TypeRecordDelete(from);
    TypeRecordDelete(to);
    return false;
  }
  if (TypeIsVoidLike(from) || TypeIsVoidLike(to)) {
    return TypeEqualIgnoringQualifiers(from, to);
  }
  if (TypeIsAbstractArray(from) || TypeIsAbstractArray(to) ||
      TypeIsFunction(from) || TypeIsFunction(to)) {
    return false;
  }
  TypeRecord* from_copy = TypeRecordCopy(from);
  TypeRecord* to_copy = TypeRecordCopy(to);
  while (from_copy != NULL && TypeIsPointer(from_copy)) {
    from_copy = from_copy->next;
  }
  while (to_copy != NULL && TypeIsPointer(to_copy)) {
    to_copy = to_copy->next;
  }
  if (from_copy != from && to_copy != to && from_copy != NULL &&
      to_copy != NULL && TypeIsStructOrUnion(from_copy) &&
      TypeIsStructOrUnion(to_copy) && from_copy->info.struct_info != NULL &&
      to_copy->info.struct_info != NULL &&
      !TypeEqualIgnoringQualifiers(from_copy, to_copy)) {
    int paths = StructCountPublicDerivationPaths(from_copy->info.struct_info,
                                                 to_copy->info.struct_info);
    TypeRecordDelete(from_copy);
    TypeRecordDelete(to_copy);
    if (paths != 1) {
      return false;
    }
  } else {
    TypeRecordDelete(from_copy);
    TypeRecordDelete(to_copy);
  }
  ASTNode* value = TypeTraitSyntheticExpressionFromType(syntax, from);
  if (value == NULL) {
    TypeRecordDelete(from);
    TypeRecordDelete(to);
    return false;
  }
  bool ok = TryConvertImplicit(value, to);
  TypeRecordDelete(from);
  TypeRecordDelete(to);
  return ok;
}

static bool TypeTraitIsAssignable(Syntax* syntax, Vector* type_args,
                                  bool check_nothrow) {
  if (type_args == NULL || type_args->length != 2) {
    return false;
  }
  TypeRecord* lhs =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[0]);
  TypeRecord* rhs =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[1]);
  if (lhs == NULL || rhs == NULL || TypeIsVoidLike(lhs) || TypeIsConst(lhs) ||
      TypeIsFunction(lhs) || TypeIsArray(lhs)) {
    return false;
  }
  ASTNode* left = NewSyntheticLvalue(syntax, lhs);
  ASTNode* right = NewSyntheticValue(syntax, rhs);
  if (left == NULL || right == NULL) {
    ASTNodeDelete(left);
    ASTNodeDelete(right);
    return false;
  }
  ASTNode* assign =
      NewBinaryASTNode(AST_OP(assign), NULL, kTypeTraitLocation, left, right);
  bool ok = TryAnalyzeExpression(assign);
  if (ok && check_nothrow && TypeIsStructOrUnion(lhs) &&
      lhs->info.struct_info != NULL) {
    StructMember* assign_op =
        FindStructMemberByName(lhs->info.struct_info, "operator=");
    if (assign_op != NULL && assign_op->symbol != NULL &&
        assign_op->symbol->type != NULL &&
        TypeIsFunction(assign_op->symbol->type) &&
        !assign_op->symbol->type->info.function.is_noexcept) {
      return false;
    }
  }
  return ok;
}

static StructMember* TypeTraitFindDestructor(Struct* str) {
  if (str == NULL || str->tag_name == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, str->tag_name);
  StructMember* dtor = FindStructMember(str, &destructor_name);
  StringDestruct(&destructor_name);
  return dtor;
}

static bool TypeTraitIsDestructible(Syntax* syntax, Vector* type_args,
                                    bool check_nothrow) {
  if (type_args == NULL || type_args->length != 1) {
    return false;
  }
  TypeRecord* target =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[0]);
  if (target == NULL || TypeIsVoidLike(target) || TypeIsFunction(target) ||
      TypeIsReference(target) || TypeIsAbstractArray(target)) {
    return false;
  }
  if (TypeIsArray(target)) {
    Vector element_args;
    VectorInit(&element_args);
    VectorAppend(&element_args, target->next);
    bool element_ok =
        TypeTraitIsDestructible(syntax, &element_args, check_nothrow);
    VectorDestruct(&element_args);
    return element_ok;
  }
  if (!TypeIsStructOrUnion(target)) {
    return true;
  }
  Struct* str = target->info.struct_info;
  if (str == NULL || str->tag_name == NULL) {
    return false;
  }
  StructMember* dtor = TypeTraitFindDestructor(str);
  if (dtor != NULL && dtor->symbol != NULL &&
      dtor->symbol->type != NULL &&
      dtor->symbol->type->info.function.is_deleted) {
    return false;
  }
  if (check_nothrow && dtor != NULL && dtor->symbol != NULL &&
      dtor->symbol->type != NULL &&
      TypeIsFunction(dtor->symbol->type)) {
    if (dtor->symbol->type->info.function.is_deleted) {
      return false;
    }
    if (!dtor->symbol->type->info.function.is_noexcept &&
        !dtor->symbol->type->info.function.is_implicitly_declared) {
      return false;
    }
  }
  return true;
}

static bool TypeTraitIsBaseOf(Syntax* syntax, Vector* type_args) {
  if (type_args == NULL || type_args->length != 2) {
    return false;
  }
  TypeRecord* base =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[0]);
  TypeRecord* derived =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[1]);
  if (base == NULL || derived == NULL) {
    return false;
  }
  if (!TypeIsStructOrUnion(base) || !TypeIsStructOrUnion(derived) ||
      base->info.struct_info == NULL || derived->info.struct_info == NULL) {
    return false;
  }
  if (TypeEqualIgnoringQualifiers(base, derived)) {
    return base->size > 0 && derived->size > 0;
  }
  if (base->size == 0) {
    return false;
  }
  return StructIsDerivedFrom(derived->info.struct_info, base->info.struct_info,
                             /*public_only=*/false);
}

static TypeRecord* TypeTraitReferenceType(TypeRecord* type, bool rvalue) {
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
  TypeRecordChain(ref, TypeRecordCopy(type));
  ref->type = type->type;
  TypeRecordCalculateSize(ref);
  return ref;
}

static bool TypeTraitStdSwapWorks(Syntax* syntax, TypeRecord* left_type,
                                  TypeRecord* right_type) {
  TypeRecord* left_lref = TypeTraitReferenceType(left_type, false);
  TypeRecord* right_lref = TypeTraitReferenceType(right_type, false);
  TypeRecord* left_rref = TypeTraitReferenceType(left_type, true);
  TypeRecord* right_rref = TypeTraitReferenceType(right_type, true);

  Vector left_ctor_args;
  Vector right_ctor_args;
  Vector left_assign_args;
  Vector right_assign_args;
  VectorInit(&left_ctor_args);
  VectorInit(&right_ctor_args);
  VectorInit(&left_assign_args);
  VectorInit(&right_assign_args);
  VectorAppend(&left_ctor_args, left_type);
  VectorAppend(&left_ctor_args, left_rref);
  VectorAppend(&right_ctor_args, right_type);
  VectorAppend(&right_ctor_args, right_rref);
  VectorAppend(&left_assign_args, left_lref);
  VectorAppend(&left_assign_args, left_rref);
  VectorAppend(&right_assign_args, right_lref);
  VectorAppend(&right_assign_args, right_rref);

  bool move_constructible =
      TypeTraitIsConstructible(syntax, &left_ctor_args, false) &&
      TypeTraitIsConstructible(syntax, &right_ctor_args, false);
  bool move_assignable =
      TypeTraitIsAssignable(syntax, &left_assign_args, false) &&
      TypeTraitIsAssignable(syntax, &right_assign_args, false);

  TypeRecordDelete(left_lref);
  TypeRecordDelete(right_lref);
  TypeRecordDelete(left_rref);
  TypeRecordDelete(right_rref);
  VectorDestruct(&left_ctor_args);
  VectorDestruct(&right_ctor_args);
  VectorDestruct(&left_assign_args);
  VectorDestruct(&right_assign_args);
  return move_constructible && move_assignable;
}

static bool TypeTraitFindSwap(Syntax* syntax, TypeRecord* left_type,
                              TypeRecord* right_type) {
  left_type = MaterializeTraitType(syntax, left_type);
  right_type = MaterializeTraitType(syntax, right_type);
  if (left_type == NULL || right_type == NULL) {
    return false;
  }
  ASTNode* left = NewSyntheticLvalue(syntax, left_type);
  ASTNode* right = NewSyntheticLvalue(syntax, right_type);
  if (left == NULL || right == NULL) {
    ASTNodeDelete(left);
    ASTNodeDelete(right);
    return false;
  }
  String swap_name;
  StringInit(&swap_name, "swap");
  Symbol* swap_symbol = SyntaxFindSymbol(syntax, &swap_name);
  StringDestruct(&swap_name);
  if (swap_symbol != NULL) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, left);
    VectorAppend(actuals, right);
    ASTNode* callee = NewIdentifierASTNode(swap_symbol, kTypeTraitLocation);
    ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, kTypeTraitLocation,
                                     callee, actuals);
    if (TryAnalyzeExpression(call)) {
      return true;
    }
  }
  ASTNodeDelete(left);
  ASTNodeDelete(right);
  return TypeTraitStdSwapWorks(syntax, left_type, right_type);
}

static bool TypeTraitIsSwappable(Syntax* syntax, Vector* type_args) {
  if (type_args == NULL || type_args->length != 1) {
    return false;
  }
  TypeRecord* type = (TypeRecord*)type_args->value.p[0];
  return TypeTraitFindSwap(syntax, type, type);
}

static bool TypeTraitIsSwappableWith(Syntax* syntax, Vector* type_args) {
  if (type_args == NULL || type_args->length != 2) {
    return false;
  }
  return TypeTraitFindSwap(syntax, (TypeRecord*)type_args->value.p[0],
                           (TypeRecord*)type_args->value.p[1]);
}

static bool TypeTraitIsMemberFunctionPointer(TypeRecord* type);
static bool TypeTraitIsMemberPointer(TypeRecord* type);
static bool TypeTraitMemberPointerInvoke(Syntax* syntax, Vector* type_args,
                                         bool check_nothrow,
                                         TypeRecord** result_out);

static TypeRecord* TypeTraitCalleeFunctionType(ASTNode* left) {
  if (left == NULL) {
    return NULL;
  }
  if (left->op == AST_OP(dot) || left->op == AST_OP(arrow)) {
    BinaryASTNode* access = (BinaryASTNode*)left;
    if (access->right != NULL && access->right->op == AST_OP(structmember)) {
      StructMemberASTNode* member_node = (StructMemberASTNode*)access->right;
      if (member_node->member != NULL && member_node->member->symbol != NULL) {
        TypeRecord* fn_type = member_node->member->symbol->type;
        while (fn_type != NULL && !TypeIsFunction(fn_type)) {
          fn_type = fn_type->next;
        }
        return fn_type;
      }
    }
  }
  TypeRecord* type = left->type;
  if (type != NULL && TypeIsPointer(type) && TypeIsFunction(type->next)) {
    return type->next;
  }
  while (type != NULL && !TypeIsFunction(type)) {
    type = type->next;
  }
  return type;
}

static bool TypeTraitCallArityMatches(VectorASTNode* call_node) {
  if (call_node == NULL || call_node->children == NULL) {
    return false;
  }
  size_t actual_count = call_node->children->length;
  TypeRecord* callee_type = TypeTraitCalleeFunctionType(call_node->left);
  if (callee_type == NULL) {
    return false;
  }
  size_t formal_count = callee_type->info.function.prototype.length;
  bool implicit_object_param = false;
  if (call_node->left != NULL &&
      (call_node->left->op == AST_OP(dot) ||
       call_node->left->op == AST_OP(arrow))) {
    BinaryASTNode* access = (BinaryASTNode*)call_node->left;
    if (access->right != NULL && access->right->op == AST_OP(structmember)) {
      StructMemberASTNode* member_node = (StructMemberASTNode*)access->right;
      if (member_node->member != NULL && member_node->member->is_member_function &&
          !member_node->member->is_static) {
        implicit_object_param = true;
      }
    }
  } else if (callee_type->info.function.cxx_member_owner != NULL &&
             formal_count > 0) {
    implicit_object_param = true;
  }
  if (implicit_object_param && formal_count > 0) {
    formal_count -= 1;
  }
  return actual_count == formal_count;
}

static size_t TypeTraitCallableFormalArgCount(Syntax* syntax,
                                              TypeRecord* callable) {
  callable = MaterializeTraitType(syntax, callable);
  if (callable == NULL) {
    return (size_t)-1;
  }
  TypeRecord* fn = NULL;
  bool implicit_object_param = false;
  TypeRecord* canonical = callable;
  while (canonical != NULL &&
         (TypeIsReference(canonical) || TypeIsPointer(canonical))) {
    canonical = canonical->next;
  }
  if (canonical != NULL && TypeIsStructOrUnion(canonical) &&
      canonical->info.struct_info != NULL) {
    StructMember* call_op =
        FindStructMemberByName(canonical->info.struct_info, "operator()");
    if (call_op != NULL && call_op->symbol != NULL &&
        call_op->symbol->type != NULL) {
      fn = call_op->symbol->type;
      implicit_object_param =
          call_op->is_member_function && !call_op->is_static;
    }
  } else {
    fn = callable;
    while (fn != NULL && !TypeIsFunction(fn)) {
      fn = fn->next;
    }
  }
  if (fn == NULL) {
    return (size_t)-1;
  }
  while (fn != NULL && !TypeIsFunction(fn)) {
    fn = fn->next;
  }
  if (fn == NULL) {
    return (size_t)-1;
  }
  size_t formal_count = fn->info.function.prototype.length;
  if (implicit_object_param && formal_count > 0) {
    formal_count -= 1;
  }
  return formal_count;
}

static ASTNode* TypeTraitSynthesizeFunctorInvoke(Syntax* syntax,
                                                    TypeRecord* callable,
                                                    Vector* type_args) {
  if (callable == NULL || type_args == NULL || type_args->length < 2) {
    return NULL;
  }
  TypeRecord* canonical = callable;
  while (canonical != NULL &&
         (TypeIsReference(canonical) || TypeIsPointer(canonical))) {
    canonical = canonical->next;
  }
  if (canonical == NULL || !TypeIsStructOrUnion(canonical) ||
      canonical->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(canonical->info.struct_info, "operator()");
  if (member == NULL || !member->is_member_function) {
    return NULL;
  }
  ASTNode* receiver = NewSyntheticLvalue(syntax, callable);
  if (receiver == NULL) {
    return NULL;
  }
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, kTypeTraitLocation, receiver,
                       NewStructMemberASTNode(member, kTypeTraitLocation));
  Vector* actuals = NewVector();
  for (size_t i = 1; i < type_args->length; i++) {
    ASTNode* arg =
        NewSyntheticValue(syntax, (TypeRecord*)type_args->value.p[i]);
    if (arg == NULL) {
      ASTNodeDelete(member_access);
      VectorDelete(actuals);
      return NULL;
    }
    VectorAppend(actuals, arg);
  }
  return NewVectorASTNode(AST_OP(call), NULL, kTypeTraitLocation, member_access,
                          actuals);
}

static bool TypeTraitIsInvocable(Syntax* syntax, Vector* type_args,
                                 bool check_nothrow) {
  if (type_args == NULL || type_args->length == 0) {
    return false;
  }
  TypeRecord* callable =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[0]);
  if (callable == NULL) {
    return false;
  }
  callable = TypeRecordCopy(callable);
  if (callable == NULL) {
    return false;
  }
  if (TypeTraitIsMemberPointer(callable) && type_args->length >= 2) {
    bool ok =
        TypeTraitMemberPointerInvoke(syntax, type_args, check_nothrow, NULL);
    TypeRecordDelete(callable);
    return ok;
  }
  ASTNode* call = NULL;
  {
    TypeRecord* callable_object = callable;
    while (callable_object != NULL &&
           (TypeIsReference(callable_object) ||
            TypeIsPointer(callable_object))) {
      callable_object = callable_object->next;
    }
    ASTNode* callee =
        (callable_object != NULL && TypeIsStructOrUnion(callable_object))
            ? NewSyntheticLvalue(syntax, callable)
            : NewSyntheticValue(syntax, callable);
    Vector* actuals = NewVector();
    for (size_t i = 1; i < type_args->length; i++) {
      ASTNode* arg =
          NewSyntheticValue(syntax, (TypeRecord*)type_args->value.p[i]);
      if (arg == NULL) {
        ASTNodeDelete(callee);
        VectorDelete(actuals);
        TypeRecordDelete(callable);
        return false;
      }
      VectorAppend(actuals, arg);
    }
    call = NewVectorASTNode(AST_OP(call), NULL, kTypeTraitLocation, callee,
                            actuals);
  }
  if (call == NULL) {
    TypeRecordDelete(callable);
    return false;
  }
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  compiler->speculative_template_instantiation_depth++;
  ASTNode* analyzed = AnalyzeExpression(call);
  compiler->speculative_template_instantiation_depth--;
  bool ok = analyzed != NULL && !DiagnosticErrorTrapped();
  if (ok && check_nothrow && analyzed != NULL && analyzed->op == AST_OP(call)) {
    VectorASTNode* call_node = (VectorASTNode*)analyzed;
    TypeRecord* callee_type =
        call_node->left != NULL ? call_node->left->type : NULL;
    while (callee_type != NULL && !TypeIsFunction(callee_type)) {
      callee_type = callee_type->next;
    }
    if (callee_type != NULL && TypeIsFunction(callee_type) &&
        !callee_type->info.function.is_noexcept) {
      ok = false;
    }
  }
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  if (analyzed != NULL) {
    ASTNodeDelete(analyzed);
  }
  TypeRecordDelete(callable);
  return ok;
}

static bool TypeTraitIsEnum(TypeRecord* type) {
  type = TypeRecordCopy(type);
  while (type != NULL && (type->qualifiers & kQualConst) != 0) {
    type->qualifiers &= ~kQualConst;
  }
  while (type != NULL && (type->qualifiers & kQualVolatile) != 0) {
    type->qualifiers &= ~kQualVolatile;
  }
  bool result = type != NULL && TypeIsPrimitive(type) && (type->type & kTypeEnum) != 0;
  TypeRecordDelete(type);
  return result;
}

static bool TypeTraitIsUnion(TypeRecord* type) {
  type = TypeRecordCopy(type);
  while (type != NULL && (type->qualifiers & kQualConst) != 0) {
    type->qualifiers &= ~kQualConst;
  }
  while (type != NULL && (type->qualifiers & kQualVolatile) != 0) {
    type->qualifiers &= ~kQualVolatile;
  }
  bool result = type != NULL && TypeIsStructOrUnion(type) &&
                (type->type & kTypeUnion) != 0;
  TypeRecordDelete(type);
  return result;
}

static bool TypeTraitIsClass(TypeRecord* type) {
  type = TypeRecordCopy(type);
  while (type != NULL && (type->qualifiers & kQualConst) != 0) {
    type->qualifiers &= ~kQualConst;
  }
  while (type != NULL && (type->qualifiers & kQualVolatile) != 0) {
    type->qualifiers &= ~kQualVolatile;
  }
  bool result = type != NULL && TypeIsStructOrUnion(type) &&
                (type->type & kTypeUnion) == 0 &&
                type->info.struct_info != NULL;
  TypeRecordDelete(type);
  return result;
}

static TypeRecord* TypeTraitStripQualifiers(TypeRecord* type) {
  type = TypeRecordCopy(type);
  while (type != NULL && (type->qualifiers & kQualConst) != 0) {
    type->qualifiers &= ~kQualConst;
  }
  while (type != NULL && (type->qualifiers & kQualVolatile) != 0) {
    type->qualifiers &= ~kQualVolatile;
  }
  return type;
}

static bool TypeTraitHasMemberPointerDeclarator(TypeRecord* type) {
  for (TypeRecord* cur = type; cur != NULL; cur = cur->next) {
    if (cur->declarator == kDeclMemberPointer) {
      return true;
    }
  }
  return false;
}

static TypeRecord* TypeTraitMemberPointerPointee(TypeRecord* type,
                                                 Struct** class_info) {
  type = TypeTraitStripQualifiers(type);
  if (type == NULL) {
    return NULL;
  }
  for (TypeRecord* cur = type; cur != NULL; cur = cur->next) {
    if (cur->declarator == kDeclMemberPointer) {
      if (class_info != NULL) {
        *class_info = cur->info.struct_info;
      }
      TypeRecord* pointee =
          cur->next != NULL ? TypeRecordCopy(cur->next) : NULL;
      TypeRecordDelete(type);
      return pointee;
    }
  }
  TypeRecordDelete(type);
  return NULL;
}

static bool TypeTraitIsMemberPointer(TypeRecord* type) {
  type = TypeTraitStripQualifiers(type);
  bool result = TypeTraitHasMemberPointerDeclarator(type);
  TypeRecordDelete(type);
  return result;
}

static bool TypeTraitIsMemberFunctionPointer(TypeRecord* type) {
  TypeRecord* pointee = TypeTraitMemberPointerPointee(type, NULL);
  bool result = pointee != NULL && TypeIsFunction(pointee);
  TypeRecordDelete(pointee);
  return result;
}

static bool TypeTraitIsMemberObjectPointer(TypeRecord* type) {
  return TypeTraitIsMemberPointer(type) &&
         !TypeTraitIsMemberFunctionPointer(type);
}

static bool TypeTraitObjectCompatibleWithClass(Syntax* syntax, TypeRecord* object,
                                               Struct* class_info) {
  object = MaterializeTraitType(syntax, object);
  if (object == NULL || class_info == NULL) {
    return false;
  }
  while (object != NULL && TypeIsReference(object)) {
    object = object->next;
  }
  if (TypeIsPointer(object)) {
    object = object->next;
  }
  return object != NULL && TypeIsStructOrUnion(object) &&
         object->info.struct_info == class_info;
}

static bool TypeTraitMemberPointerUsesDirectObject(Syntax* syntax,
                                                   Vector* type_args) {
  if (type_args == NULL || type_args->length != 2) {
    return false;
  }
  TypeRecord* member_pointer =
      MaterializeTraitType(syntax, type_args->value.p[0]);
  Struct* owner = NULL;
  TypeRecord* pointee =
      TypeTraitMemberPointerPointee(member_pointer, &owner);
  TypeRecordDelete(pointee);
  if (owner == NULL) {
    return false;
  }
  TypeRecord* object =
      MaterializeTraitType(syntax, type_args->value.p[1]);
  while (object != NULL && TypeIsReference(object)) {
    object = object->next;
  }
  if (object == NULL || !TypeIsStructOrUnion(object) ||
      object->info.struct_info == NULL) {
    return false;
  }
  if (object->info.struct_info == owner) {
    return true;
  }
  if (owner->tag_symbol == NULL || owner->tag_symbol->type == NULL) {
    return false;
  }
  CXXBaseAdjustment adjustment;
  return TypeBaseAdjustment(object, owner->tag_symbol->type,
                            /*public_only=*/true, &adjustment);
}

static bool TypeTraitObjectCanAccessMemberClass(TypeRecord* object,
                                                Struct* owner) {
  if (object == NULL || owner == NULL || !TypeIsStructOrUnion(object) ||
      object->info.struct_info == NULL) {
    return false;
  }
  if (object->info.struct_info == owner) {
    return true;
  }
  if (owner->tag_symbol == NULL || owner->tag_symbol->type == NULL) {
    return false;
  }
  CXXBaseAdjustment adjustment;
  return TypeBaseAdjustment(object, owner->tag_symbol->type,
                            /*public_only=*/true, &adjustment);
}

// Models INVOKE(pm, t1, ...tN) for a pointer to member, per [func.require].
// The real `.*` analysis path requires a *constant* member pointer whose target
// member (and thus its offset/prototype) is statically known, but a type trait
// only has the member-pointer *type*.  We therefore reason structurally about
// the types instead of synthesizing a real member-access expression.  On
// success returns true; when `result_out` is non-NULL an owned result type is
// stored there.  For member functions, `check_nothrow` additionally requires
// the pointed-to function to be non-throwing.
static bool TypeTraitMemberPointerInvoke(Syntax* syntax, Vector* type_args,
                                         bool check_nothrow,
                                         TypeRecord** result_out) {
  if (result_out != NULL) {
    *result_out = NULL;
  }
  if (type_args == NULL || type_args->length < 2) {
    return false;
  }
  TypeRecord* callable =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[0]);
  Struct* class_info = NULL;
  TypeRecord* pointee = TypeTraitMemberPointerPointee(callable, &class_info);
  if (pointee == NULL || class_info == NULL) {
    TypeRecordDelete(pointee);
    return false;
  }

  // Classify the object argument according to INVOKE: reference_wrapper uses
  // get(), an object of the member's class (or a derived class) uses .*, and
  // every other type uses *t1 before applying .*.
  TypeRecord* object_type =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[1]);
  TypeRecord* obj = object_type;
  bool object_is_lvalue = false;
  while (obj != NULL && TypeIsReference(obj)) {
    object_is_lvalue =
        object_is_lvalue || obj->declarator == kDeclReference;
    obj = obj->next;
  }
  if (obj != NULL && TypeIsStructOrUnion(obj) &&
      obj->template_arguments != NULL &&
      obj->template_arguments->length == 1) {
    Symbol* origin = obj->template_origin;
    if (origin == NULL && obj->info.struct_info != NULL &&
        obj->info.struct_info->tag_symbol != NULL &&
        obj->info.struct_info->tag_symbol->type != NULL) {
      origin = obj->info.struct_info->tag_symbol->type->template_origin;
    }
    if (origin != NULL &&
        strcmp(origin->name.value, "reference_wrapper") == 0) {
      TemplateArgument* wrapped = obj->template_arguments->value.p[0];
      if (wrapped != NULL && wrapped->kind == kTemplateParameterType) {
        obj = wrapped->type;
        while (obj != NULL && TypeIsReference(obj)) {
          obj = obj->next;
        }
        object_is_lvalue = true;
      }
    }
  }
  bool object_ok = TypeTraitObjectCanAccessMemberClass(obj, class_info);
  TypeRecord* dereferenced_type = NULL;
  if (!object_ok) {
    ASTNode* operand = NewSyntheticValue(syntax, object_type);
    ASTNode* dereference =
        operand != NULL
            ? NewUnaryASTNode(AST_OP(contents), NULL, kTypeTraitLocation,
                              operand)
            : NULL;
    SyntaxOpenScope(syntax);
    bool saved_trap = DiagnosticErrorTrapBegin();
    DiagnosticSuppressBegin();
    ASTNode* analyzed =
        dereference != NULL ? AnalyzeExpression(dereference) : NULL;
    bool dereference_ok = analyzed != NULL && !DiagnosticErrorTrapped();
    DiagnosticSuppressEnd();
    DiagnosticErrorTrapEnd(saved_trap);
    SyntaxCloseScope(syntax);
    if (dereference_ok && analyzed->type != NULL) {
      dereferenced_type = TypeRecordCopy(analyzed->type);
      object_is_lvalue = analyzed->value_category == kValueCategoryLvalue;
      obj = dereferenced_type;
      while (obj != NULL && TypeIsReference(obj)) {
        object_is_lvalue =
            object_is_lvalue || obj->declarator == kDeclReference;
        obj = obj->next;
      }
      object_ok = TypeTraitObjectCanAccessMemberClass(obj, class_info);
    }
    ASTNodeDelete(analyzed);
  }
  if (!object_ok) {
    TypeRecordDelete(dereferenced_type);
    TypeRecordDelete(pointee);
    return false;
  }

  if (!TypeIsFunction(pointee)) {
    // Pointer to data member: INVOKE(pm, t1) == t1.*pm, well-formed only with
    // the single object argument.  The result is a reference to the member
    // whose value category follows the object's.
    if (type_args->length != 2) {
      TypeRecordDelete(pointee);
      return false;
    }
    if (result_out != NULL) {
      TypeRecord* ref = NewTypeRecord(kTypeImplicit, kQualPlain);
      ref->declarator = object_is_lvalue ? kDeclReference
                                         : kDeclRValueReference;
      ref->next = TypeRecordCopy(pointee);
      if (obj != NULL) {
        ref->next->qualifiers |=
            obj->qualifiers & (kQualConst | kQualVolatile);
      }
      *result_out = ref;
    }
    TypeRecordDelete(dereferenced_type);
    TypeRecordDelete(pointee);
    return true;
  }

  // Pointer to member function: the remaining call arguments (t2..tN) must be
  // implicitly convertible to the (this-stripped) parameter types.
  FunctionInfo* info = &pointee->info.function;
  if ((!info->is_const_member && obj != NULL && TypeIsConst(obj)) ||
      (!info->is_volatile_member && obj != NULL && TypeIsVolatile(obj)) ||
      (info->ref_qualifier == kCXXRefQualifierLValue &&
       !object_is_lvalue) ||
      (info->ref_qualifier == kCXXRefQualifierRValue &&
       object_is_lvalue)) {
    TypeRecordDelete(dereferenced_type);
    TypeRecordDelete(pointee);
    return false;
  }
  size_t formal_count = info->prototype.length;
  size_t call_args = type_args->length - 2;
  if (call_args != formal_count &&
      !(info->varargs && call_args >= formal_count)) {
    TypeRecordDelete(dereferenced_type);
    TypeRecordDelete(pointee);
    return false;
  }
  bool args_ok = true;
  for (size_t i = 0; i < formal_count && args_ok; i++) {
    Symbol* param = (Symbol*)info->prototype.value.p[i];
    TypeRecord* param_type = param != NULL ? param->type : NULL;
    ASTNode* arg =
        NewSyntheticValue(syntax, (TypeRecord*)type_args->value.p[i + 2]);
    if (param_type == NULL || arg == NULL ||
        !TryConvertImplicit(arg, param_type)) {
      args_ok = false;
    }
    ASTNodeDelete(arg);
  }
  if (!args_ok || (check_nothrow && !info->is_noexcept)) {
    TypeRecordDelete(dereferenced_type);
    TypeRecordDelete(pointee);
    return false;
  }
  if (result_out != NULL) {
    *result_out = pointee->next != NULL
                      ? TypeRecordCopy(pointee->next)
                      : NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  }
  TypeRecordDelete(dereferenced_type);
  TypeRecordDelete(pointee);
  return true;
}

bool CXXTypeTraitEvaluateBool(Syntax* syntax, CXXTypeTraitKind kind,
                              Vector* type_args) {
  SyntaxOpenScope(syntax);
  bool result = false;
  switch (kind) {
    case kCXXTypeTraitIsConstructible:
      result = TypeTraitIsConstructible(syntax, type_args, false);
      break;
    case kCXXTypeTraitIsNothrowConstructible:
      result = TypeTraitIsConstructible(syntax, type_args, true);
      break;
    case kCXXTypeTraitIsConvertible:
      result = TypeTraitIsConvertible(syntax, type_args);
      break;
    case kCXXTypeTraitIsAssignable:
      result = TypeTraitIsAssignable(syntax, type_args, false);
      break;
    case kCXXTypeTraitIsNothrowAssignable:
      result = TypeTraitIsAssignable(syntax, type_args, true);
      break;
    case kCXXTypeTraitIsDestructible:
      result = TypeTraitIsDestructible(syntax, type_args, false);
      break;
    case kCXXTypeTraitIsNothrowDestructible:
      result = TypeTraitIsDestructible(syntax, type_args, true);
      break;
    case kCXXTypeTraitIsBaseOf:
      result = TypeTraitIsBaseOf(syntax, type_args);
      break;
    case kCXXTypeTraitIsSwappable:
      result = TypeTraitIsSwappable(syntax, type_args);
      break;
    case kCXXTypeTraitIsSwappableWith:
      result = TypeTraitIsSwappableWith(syntax, type_args);
      break;
    case kCXXTypeTraitIsInvocable:
      result = TypeTraitIsInvocable(syntax, type_args, false);
      break;
    case kCXXTypeTraitIsNothrowInvocable:
      result = TypeTraitIsInvocable(syntax, type_args, true);
      break;
    case kCXXTypeTraitIsClass:
      result = type_args != NULL && type_args->length == 1 &&
               TypeTraitIsClass((TypeRecord*)type_args->value.p[0]);
      break;
    case kCXXTypeTraitIsUnion:
      result = type_args != NULL && type_args->length == 1 &&
               TypeTraitIsUnion((TypeRecord*)type_args->value.p[0]);
      break;
    case kCXXTypeTraitIsEnum:
      result = type_args != NULL && type_args->length == 1 &&
               TypeTraitIsEnum((TypeRecord*)type_args->value.p[0]);
      break;
    case kCXXTypeTraitIsMemberPointer:
      result = type_args != NULL && type_args->length == 1 &&
               TypeTraitIsMemberPointer((TypeRecord*)type_args->value.p[0]);
      break;
    case kCXXTypeTraitIsMemberObjectPointer:
      result = type_args != NULL && type_args->length == 1 &&
               TypeTraitIsMemberObjectPointer(
                   (TypeRecord*)type_args->value.p[0]);
      break;
    case kCXXTypeTraitIsMemberFunctionPointer:
      result = type_args != NULL && type_args->length == 1 &&
               TypeTraitIsMemberFunctionPointer(
                   (TypeRecord*)type_args->value.p[0]);
      break;
    case kCXXTypeTraitMemberPointerDirectObject:
      result = TypeTraitMemberPointerUsesDirectObject(syntax, type_args);
      break;
  }
  SyntaxCloseScope(syntax);
  return result;
}

static TypeRecord* CXXTypeTraitInvokeResultTypeImpl(Syntax* syntax,
                                                    Vector* type_args) {
  if (type_args == NULL || type_args->length == 0) {
    return NULL;
  }
  TypeRecord* callable =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[0]);
  if (callable == NULL) {
    return NULL;
  }
  if (TypeTraitIsMemberPointer(callable) && type_args->length >= 2) {
    TypeRecord* member_result = NULL;
    TypeTraitMemberPointerInvoke(syntax, type_args, false, &member_result);
    return member_result;
  }
  ASTNode* call = NULL;
  {
    TypeRecord* callable_object = callable;
    while (callable_object != NULL &&
           (TypeIsReference(callable_object) ||
            TypeIsPointer(callable_object))) {
      callable_object = callable_object->next;
    }
    ASTNode* callee =
        (callable_object != NULL && TypeIsStructOrUnion(callable_object))
            ? NewSyntheticLvalue(syntax, callable)
            : NewSyntheticValue(syntax, callable);
    Vector* actuals = NewVector();
    for (size_t i = 1; i < type_args->length; i++) {
      ASTNode* arg =
          NewSyntheticValue(syntax, (TypeRecord*)type_args->value.p[i]);
      if (arg == NULL) {
        ASTNodeDelete(callee);
        VectorDelete(actuals);
        return NULL;
      }
      VectorAppend(actuals, arg);
    }
    call = NewVectorASTNode(AST_OP(call), NULL, kTypeTraitLocation, callee,
                            actuals);
  }
  if (call == NULL) {
    return NULL;
  }
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  compiler->speculative_template_instantiation_depth++;
  ASTNode* analyzed = AnalyzeExpression(call);
  compiler->speculative_template_instantiation_depth--;
  TypeRecord* result = NULL;
  if (analyzed != NULL && analyzed->type != NULL && !DiagnosticErrorTrapped()) {
    result = TypeRecordCopy(analyzed->type);
  }
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  if (analyzed != NULL) {
    ASTNodeDelete(analyzed);
  }
  return result;
}

TypeRecord* CXXTypeTraitInvokeResultType(Syntax* syntax, Vector* type_args) {
  SyntaxOpenScope(syntax);
  TypeRecord* result = CXXTypeTraitInvokeResultTypeImpl(syntax, type_args);
  SyntaxCloseScope(syntax);
  return result;
}

static TypeRecord* TypeTraitPlainArithmeticType(TypeRecord* type) {
  TypeRecord* head = TypeRecordCopy(type);
  TypeRecord* plain = head;
  while (plain != NULL && (TypeIsReference(plain) || TypeIsArray(plain))) {
    plain = plain->next;
  }
  if (plain == NULL || !TypeIsPrimitive(plain) || TypeIsVoid(plain) ||
      TypeIsBool(plain)) {
    TypeRecordDelete(head);
    return NULL;
  }
  TypeRecord* result =
      NewTypeRecordWithSize(plain->type & ~kTypeUnknown, kQualPlain);
  TypeRecordDelete(head);
  return result;
}

static int TypeTraitArithmeticRank(TypeRecord* type) {
  for (int i = 0; type_ranks[i] != NULL; i++) {
    if (type_ranks[i](type)) {
      return i + 1;
    }
  }
  return -1;
}

static TypeRecord* TypeTraitArithmeticCommonType(TypeRecord* left,
                                                 TypeRecord* right) {
  TypeRecord* plain_left = TypeTraitPlainArithmeticType(left);
  TypeRecord* plain_right = TypeTraitPlainArithmeticType(right);
  if (plain_left == NULL || plain_right == NULL) {
    TypeRecordDelete(plain_left);
    TypeRecordDelete(plain_right);
    return NULL;
  }
  int left_rank = TypeTraitArithmeticRank(plain_left);
  int right_rank = TypeTraitArithmeticRank(plain_right);
  TypeRecord* result = NULL;
  if (left_rank >= 0 && right_rank >= 0) {
    TypeRecord* preferred = NULL;
    if (left_rank > right_rank) {
      preferred = left;
    } else if (right_rank > left_rank) {
      preferred = right;
    } else if (TypeIsUnsigned(plain_left) != TypeIsUnsigned(plain_right)) {
      preferred = TypeIsUnsigned(plain_left) ? left : right;
    } else {
      preferred = left;
    }
    while (preferred != NULL &&
           (TypeIsReference(preferred) || TypeIsArray(preferred))) {
      preferred = preferred->next;
    }
    if (preferred != NULL) {
      TypeRecord* end = preferred;
      while (end != NULL && (TypeIsReference(end) || TypeIsArray(end))) {
        end = end->next;
      }
      if (end != NULL) {
        result = NewTypeRecordWithSize(end->type & ~kTypeUnknown, kQualPlain);
        result = TypeRecordCalculateSize(result);
      }
    }
  }
  TypeRecordDelete(plain_left);
  TypeRecordDelete(plain_right);
  return result;
}

static TypeRecord* TypeTraitConditionalCommonType(Syntax* syntax,
                                                  TypeRecord* left,
                                                  TypeRecord* right) {
  ASTNode* left_expr = NewSyntheticValue(syntax, left);
  ASTNode* right_expr = NewSyntheticValue(syntax, right);
  if (left_expr == NULL || right_expr == NULL) {
    ASTNodeDelete(left_expr);
    ASTNodeDelete(right_expr);
    return NULL;
  }
  ASTNode* condition = (ASTNode*)NewIntConstantASTNode(
      0, NewTypeRecordWithSize(kTypeBool, kQualPlain), kTypeTraitLocation);
  BinaryASTNode* colon =
      NewBinaryASTNode(AST_OP(colon), NULL, kTypeTraitLocation, left_expr,
                       right_expr);
  BinaryASTNode* conditional = NewBinaryASTNode(
      AST_OP(question), NULL, kTypeTraitLocation, condition, (ASTNode*)colon);
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  ASTNode* analyzed = AnalyzeExpression((ASTNode*)conditional);
  TypeRecord* result = NULL;
  if (analyzed != NULL && analyzed->type != NULL && !DiagnosticErrorTrapped()) {
    result = TypeRecordCopy(analyzed->type);
  }
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  if (analyzed != NULL) {
    ASTNodeDelete(analyzed);
  }
  return result;
}

static TypeRecord* CXXTypeTraitCommonTypeImpl(Syntax* syntax,
                                              Vector* type_args) {
  if (type_args == NULL || type_args->length != 2) {
    return NULL;
  }
  TypeRecord* left =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[0]);
  TypeRecord* right =
      MaterializeTraitType(syntax, (TypeRecord*)type_args->value.p[1]);
  if (left == NULL || right == NULL) {
    TypeRecordDelete(left);
    TypeRecordDelete(right);
    return NULL;
  }
  TypeRecord* result = TypeTraitArithmeticCommonType(left, right);
  if (result == NULL) {
    result = TypeTraitConditionalCommonType(syntax, left, right);
  }
  TypeRecordDelete(left);
  TypeRecordDelete(right);
  return result;
}

TypeRecord* CXXTypeTraitCommonType(Syntax* syntax, Vector* type_args) {
  SyntaxOpenScope(syntax);
  TypeRecord* result = CXXTypeTraitCommonTypeImpl(syntax, type_args);
  SyntaxCloseScope(syntax);
  return result;
}
