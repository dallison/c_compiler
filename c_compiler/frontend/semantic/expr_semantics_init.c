//
//  expr_semantics_init.c
//  c_compiler
//
//  Temporary materialization, braced initialization, and initialization.
//

#include "expr_semantics.h"
#include "expr_semantics_internal.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "concepts.h"
#include "contracts.h"
#include "constexpr.h"
#include "expr_evaluator.h"
#include "init_semantics.h"
#include "statement_semantics.h"
#include "compiler.h"
#include "errors.h"
#include "symbol_table.h"
#include "typo_correction.h"
#include "rtti.h"
#include "reflection.h"
#include "reflection_meta_synthesis.h"
#include "reflection_meta_traits.h"
#include "reflection_semantics.h"
#include "type_traits_semantics.h"
#include "type_compare.h"
#include "member_pointer.h"
#include "statement_parser.h"
#include "type_parse.h"
#include "type_class_internal.h"
#include "type_inheritance.h"
#include "type_special_member.h"
#include "type_template.h"

static ASTNode* NewCXXInitializerListBackingArray(TypeRecord* element_type, BracedInitializerASTNode* braced, SourceLocation location);
static ASTNode* LowerCXXInitializerListBracedInit(TypeRecord* target_type, BracedInitializerASTNode* braced, SourceLocation location);
static ASTNode* LowerCXXBracedInitToConstructorCall( BracedInitializerASTNode* braced, TypeRecord* target, SourceLocation location);
static bool NarrowingIntegerConstantFits(int64_t value, TypeRecord* target);
static void DiagnoseScalarNarrowing(ASTNode* source, TypeRecord* target);
static ASTNode* LowerScalarBracedInitToTarget(BracedInitializerASTNode* braced,
                                              TypeRecord* target,
                                              SourceLocation location);
static int CXXRankOneInitializerListBracedElement(ASTNode* initializer,
                                                  TypeRecord* element_type);
static void SetReferenceInitializerNeedAddress(ASTNode* target, ASTNode* init);
static ASTNode* AnalyzeAutoDeductionAndRelatedRewrites(
    ASTNode* node, ASTNode* target, Symbol* symbol, ASTNode* init,
    ASTNode** init_io);
static void AnalyzeReferenceExpressionInit(ExpressionInitializerASTNode* e,
                                             TypeRecord* reference_type);
static void AnalyzeNonReferenceExpressionInit(ASTNode* target,
                                                ExpressionInitializerASTNode* e);
static void AnalyzeBracedInitForTarget(ASTNode* target, bool is_reference_init,
                                       ASTNode** init_io);
static void AnalyzeInitializerByKind(ASTNode* target, ASTNode** init_io);
static void ApplyMemberPointerConstantSymbolValue(Symbol* symbol, ASTNode* init);
static void ComputeConstantsOnlyFlags(Symbol* symbol, ASTNode* init,
                                      bool is_static_storage,
                                      bool is_thread_local,
                                      bool* constants_only_io,
                                      bool* is_cxx_local_static_out);
static ASTNode* AnalyzeSymbolBackedInitialization(ASTNode* node,
                                                    Symbol* symbol,
                                                    ASTNode* init,
                                                    bool is_static_storage,
                                                    bool is_thread_local);


// Contract: Returns whether `node` designates storage for which & is valid
// (lvalues, certain subscripts/members, prvalue temporaries via comma, etc.).
bool HasAddress(ASTNode* node) {
  if (ASTNodeIsGLValue(node)) {
    return !IsBitfieldReference(node);
  }
  switch (node->op) {
    case AST_OP(identifier):
      // Variables are fine.
      return true;
    case AST_OP(subscript):
      // Array indexes are ok.
      return true;
    case AST_OP(contents):
      // *pointer is good.
      return true;
    case AST_OP(dot):
    case AST_OP(arrow):
      // struct/union member references are ok as long as they are
      // not bitfields
      return !IsBitfieldReference(node);
    case AST_OP(cast): {
      // A scalar conversion such as `double(1)` yields a new value; only a
      // qualification-only cast still designates its operand.
      CastASTNode* cast = (CastASTNode*)node;
      if (cast->cast_type != NULL && cast->expr != NULL &&
          cast->expr->type != NULL && TypeIsScalar(cast->cast_type) &&
          !TypeIsReference(cast->cast_type)) {
        return TypeEqualIgnoringQualifiers(cast->cast_type,
                                           cast->expr->type) &&
               HasAddress(cast->expr);
      }
      return true;
    }
    case AST_OP(compound_literal):
      return true;
    case AST_OP(comma):
      // In C++, the built-in comma expression has the value category of its
      // right operand.  Converting-constructor lowering represents an already
      // materialized class temporary as `(constructor_call, temporary)`;
      // recognizing the temporary's address here avoids materializing and
      // moving that same object a second time when it binds to a reference.
      return CompilerIsCXX() &&
             HasAddress(((BinaryASTNode*)node)->right);
    default:
      return false;
  }
}

// Contract: Wraps prvalue `expr` in a compound literal temporary with address
// taken; result is an xvalue suitable for reference binding.
ASTNode* MaterializeTemporary(ASTNode* expr, TypeRecord* type) {
  SourceLocation location = expr->location;
  // Temporary materialization preserves the prvalue's object type.  The
  // reference target may add top-level cv-qualification (e.g. `const T&`), but
  // that qualification belongs to the reference binding, not to the temporary
  // itself.  Making the temporary `const` also incorrectly sends class
  // prvalues through constexpr-object reconstruction during initialization.
  TypeRecord* temporary_type =
      TypeRecordCopy(expr->type != NULL ? expr->type : type);
  SetQualifiers(temporary_type, kQualPlain);
  // TypeRecordCopy hands back an unreferenced node, so the symbol below holds the
  // only reference: releasing one here would drop it to zero and tear down the
  // declarator spine, leaving a pointer temporary pointing at nothing.
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, temporary_type);
  // A materialized temporary exists to be reached through its address -- that is
  // what the reference binding to it holds.  A register-allocated variable has no
  // address to name, so this object has to stay in memory.
  temp->flags.address_taken = true;
  ASTNode* temp_id = NewIdentifierASTNode(temp, location);
  temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* initializer = NewExpressionInitializerASTNode(expr, location);
  ASTNode* materialized =
      NewCompoundLiteralASTNode(temp_id, location, initializer);
  ASTNode* analyzed = AnalyzeExpression(materialized);
  analyzed->value_category = kValueCategoryXvalue;
  return analyzed;
}

// Contract: Materializes non-trivial class prvalues from calls/conditionals when
// they lack an address; preserves parent/child_id linkage. No-op otherwise.
ASTNode* SemanticMaterializeClassPrvalue(ASTNode* expr) {
  if (!CompilerIsCXX() || expr == NULL || expr->type == NULL) {
    return expr;
  }
  // Only a call's result is a fresh object here; other class prvalues (an
  // `init`, an inlined constructor) already designate an object with an owner.
  bool creates_object =
      expr->op == AST_OP(call) || expr->op == AST_OP(question) ||
      (expr->op == AST_OP(inline_call) && !ASTIsInlinedConstructor(expr));
  if (!creates_object || expr->value_category != kValueCategoryPrvalue ||
      !TypeIsStructOrUnion(expr->type) || HasAddress(expr) ||
      ExpressionIsTemplateDependent(expr) ||
      !TypeHasNonTrivialDestructor(expr->type)) {
    return expr;
  }
  ASTNode* parent = expr->parent;
  int child_id = expr->child_id;
  ASTNode* materialized = MaterializeTemporary(expr, expr->type);
  materialized->value_category = kValueCategoryPrvalue;
  materialized->parent = parent;
  materialized->child_id = child_id;
  return materialized;
}

// Contract: Returns whether `node` may appear as an assignment/init target;
// `is_init` relaxes const/array rules for initialization contexts.
bool IsAssignable(ASTNode* node, bool is_init) {
  if (is_init) {
    // During initialization, constants and arrays can be assigned to.
    return !TypeIsFunction(node->type);
  }

  // Constants are not assignable.
  if (TypeIsConst(node->type)) {
    return false;
  }

  // Functions and arrays can't be assigned to.
  if (TypeIsFunction(node->type) || TypeIsArray(node->type)) {
    return false;
  }

  // Multiple assignment are ok.
  if (node->op == AST_OP(assign)) {
    return true;
  }

  return IsBitfieldReference(node) || HasAddress(node);
}

// Contract: Returns whether class prvalue/class lvalue `actual` has a
// conversion to cv-unqualified `target` (template deduction or result-type match).
bool ClassHasConversionOperatorTo(ASTNode* actual, TypeRecord* target) {
  if (!CompilerIsCXX() || actual == NULL || actual->type == NULL ||
      !TypeIsStructOrUnion(actual->type)) {
    return false;
  }
  // Match against the cv-unqualified target: a parameter of type `const T&`
  // yields target `const T` here, but a conversion function is named
  // "operator T" (top-level cv-qualifiers on the conversion type are dropped)
  // and yields a prvalue `T` that binds to the `const T&`.
  //
  // Compare the conversion function's *result type* rather than looking members
  // up by a reconstructed "operator T" name.  The recorded name of a conversion
  // operator instantiated from a class template can be stale when its result
  // type was a class template that was still incomplete at the point of
  // instantiation (e.g. std::basic_string::operator basic_string_view, whose
  // result names a specialization completed later in the TU): the stored name
  // renders the still-deferred result as the bare primary "operator
  // basic_string_view" while the target here is the fully materialized
  // "basic_string_view<char,...>", so a name-based lookup would miss it.
  // Materializing each candidate's result type at this point -- where the target
  // is necessarily complete -- and comparing types directly sidesteps the name
  // mismatch entirely.
  TypeRecord* unqualified_target = NULL;
  Vector operators;
  VectorInit(&operators);
  CollectConversionOperators(actual->type->info.struct_info, &operators);
  bool found = false;
  for (size_t i = 0; i < operators.length && !found; i++) {
    StructMember* member = operators.value.p[i];
    if (member == NULL || member->symbol == NULL ||
        !TypeIsFunction(member->symbol->type) ||
        member->symbol->type->next == NULL) {
      continue;
    }
    // `operator Container()` is a template: its result type is the parameter,
    // so it only matches once Container is deduced from the target
    // ([temp.deduct.conv]).  That is how `vector = StrSplit(...)` binds, via
    // Splitter's conversion operator.
    if (member->symbol->flags.is_template) {
      if (unqualified_target == NULL) {
        unqualified_target = TypeRecordCopy(target);
        SetQualifiers(unqualified_target, kQualPlain);
      }
      Vector* args = TypeDeduceConversionOperatorTemplateArguments(
          &compiler->syntax, member->symbol, unqualified_target);
      if (args != NULL) {
        VectorDeleteWithContents(
            args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        found = true;
      }
      continue;
    }
    TypeRecord* result = member->symbol->type->next;
    TypeRecord* materialized =
        TypeMaterializeClassTemplateSpecialization(&compiler->syntax, result);
    if (TypeEqualIgnoringQualifiers(materialized, target)) {
      found = true;
    }
    if (materialized != result) {
      TypeRecordDelete(materialized);
    }
  }
  VectorDestruct(&operators);
  TypeRecordDelete(unqualified_target);
  return found;
}

bool CXXClassHasConversionOperatorTo(ASTNode* from, TypeRecord* to) {
  return ClassHasConversionOperatorTo(from, to);
}

// Contract: Returns the type NormalConversion should target when binding
// `actual` to `reference_type` (referent type, possibly via conversion op).
TypeRecord* ReferenceConversionTarget(ASTNode* actual,
                                             TypeRecord* reference_type) {
  if (!CompilerIsCXX() || actual == NULL || actual->type == NULL ||
      !TypeIsStructOrUnion(actual->type) ||
      TypeEqualIgnoringQualifiers(actual->type, reference_type->next) ||
      TypeIsDerivedFrom(actual->type, reference_type->next) ||
      !ClassHasConversionOperatorTo(actual, reference_type)) {
    return reference_type->next;
  }
  return reference_type;
}

// Contract: Builds a compound-literal temporary array holding moved braced
// elements; consumes braced initializer nodes via ASTNodeMove.
static ASTNode* NewCXXInitializerListBackingArray(TypeRecord* element_type,
                                                  BracedInitializerASTNode* braced,
                                                  SourceLocation location) {
  TypeRecord* array_type =
      NewBasicArrayTypeRecord(kQualPlain, (int)braced->initializers->length,
                              /*is_flexible=*/false);
  TypeRecordChain(array_type, TypeRecordCopy(element_type));
  TypeRecordCalculateSize(array_type);

  Vector* array_initializers = NewVector();
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    VectorAppend(array_initializers, ASTNodeMove(initializer));
  }
  ASTNode* array_init =
      NewBracedInitializerASTNode(array_initializers, array_type, location);
  Symbol* array_symbol = SyntaxNewTemporary(&compiler->syntax, array_type);
  array_symbol->location = location;
  ASTNode* array_id = NewIdentifierASTNode(array_symbol, location);
  array_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  return NewCompoundLiteralASTNode(array_id, location, array_init);
}

// Contract: Lowers braced-init to std::initializer_list's {pointer, length}
// braced form; empty list uses null pointer constant. May consume `braced`.
static ASTNode* LowerCXXInitializerListBracedInit(TypeRecord* target_type,
                                                  BracedInitializerASTNode* braced,
                                                  SourceLocation location) {
  TypeRecord* element_type = TypeCXXInitializerListElement(target_type);
  if (element_type == NULL) {
    return (ASTNode*)braced;
  }

  // An empty braced-init-list is an empty initializer_list.  A zero-length
  // backing array has size 0, which the backend cannot allocate.  A null
  // pointer and a length of 0 is the empty range.
  ASTNode* backing_array;
  if (braced->initializers == NULL || braced->initializers->length == 0) {
    TypeRecord* pointee = TypeRecordCopy(element_type);
    SetQualifiers(pointee, GetQualifiers(pointee) | kQualConst);
    TypeRecord* pointer = NewPointerTypeRecord(kQualPlain);
    TypeRecordChain(pointer, pointee);
    TypeRecordCalculateSize(pointer);
    backing_array = NewIntConstantASTNode(0, pointer, location);
  } else {
    backing_array =
        NewCXXInitializerListBackingArray(element_type, braced, location);
  }
  Vector* list_initializers = NewVector();
  VectorAppend(list_initializers,
               NewExpressionInitializerASTNode(backing_array, location));
  VectorAppend(list_initializers,
               NewExpressionInitializerASTNode(
                   NewIntConstantASTNode((int64_t)braced->initializers->length,
                                         NewSizeTypeRecord(), location),
                   location));
  return NewBracedInitializerASTNode(list_initializers, target_type, location);
}

// Contract: Returns the overload conversion rank for one braced-init element
// toward `element_type`, or -1 if the element cannot list-initialize that type.
// Analyzes expression elements in place; nested braced lists delegate to
// CXXBracedInitTargetRank / CXXInitializerListBracedInitRank.
static int CXXRankOneInitializerListBracedElement(ASTNode* initializer,
                                                    TypeRecord* element_type) {
  if (initializer == NULL) {
    return -1;
  }
  ASTNode* element = initializer;
  if (initializer->op == AST_OP(expr_init)) {
    element = ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (element != NULL && element->op == AST_OP(braced_init)) {
    int nested = CXXBracedInitTargetRank(element, element_type);
    if (nested < 0) {
      nested = CXXInitializerListBracedInitRank(element, element_type);
    }
    return nested;
  }
  if (initializer->op != AST_OP(expr_init)) {
    return -1;
  }
  ExpressionInitializerASTNode* expr_init =
      (ExpressionInitializerASTNode*)initializer;
  expr_init->expr = AnalyzeExpression(expr_init->expr);
  return OverloadConversionRank(expr_init->expr, element_type);
}

// Contract: Returns the worst element conversion rank ([over.ics.list]/4) for
// list-initializing `formal_type` (a std::initializer_list) from `actual`, or -1
// if any element is ill-formed or non-viable. Requires `actual` to be braced_init.
int CXXInitializerListBracedInitRank(ASTNode* actual,
                                            TypeRecord* formal_type) {
  if (actual == NULL || actual->op != AST_OP(braced_init) ||
      !TypeIsCXXInitializerList(formal_type)) {
    return -1;
  }
  TypeRecord* element_type = TypeCXXInitializerListElement(formal_type);
  if (element_type == NULL) {
    return -1;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  int worst = 0;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    int rank = CXXRankOneInitializerListBracedElement(
        braced->initializers->value.p[i], element_type);
    if (rank < 0) {
      return -1;
    }
    if (rank > worst) {
      worst = rank;
    }
  }
  return worst;
}

// Contract: Lowers braced actual to a temporary std::initializer_list for call
// arguments. Returns `actual` unchanged when not applicable.
ASTNode* ConvertCXXInitializerListArgument(ASTNode* actual,
                                                  TypeRecord* formal_type) {
  TypeRecord* target = TypeIsReference(formal_type) ? formal_type->next
                                                    : formal_type;
  if (actual == NULL || actual->op != AST_OP(braced_init) ||
      !TypeIsCXXInitializerList(target)) {
    return actual;
  }
  ASTNode* initializer = LowerCXXInitializerListBracedInit(
      target, (BracedInitializerASTNode*)actual, actual->location);
  Symbol* list_symbol = SyntaxNewTemporary(&compiler->syntax,
                                           TypeRecordCopy(target));
  list_symbol->location = actual->location;
  ASTNode* list_id = NewIdentifierASTNode(list_symbol, actual->location);
  list_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* lowered =
      NewCompoundLiteralASTNode(list_id, actual->location, initializer);
  return AnalyzeExpression(lowered);
}

// Contract: List-init non-aggregate class via `(temp.ctor(args...), temp)` when
// [over.match.list] selects a constructor (CXXBracedInitTargetRank >= 0); returns
// NULL otherwise so callers use member-wise / constexpr slot init. Moves braced
// elements into the call unless an initializer-list ctor applies.
static ASTNode* LowerCXXBracedInitToConstructorCall(
    BracedInitializerASTNode* braced, TypeRecord* target,
    SourceLocation location) {
  static int brace_ctor_depth = 0;
  if (brace_ctor_depth > 8) {
    return NULL;
  }
  if (!CompilerIsCXX() || !TypeIsStructOrUnion(target) ||
      target->info.struct_info == NULL ||
      target->info.struct_info->tag_name == NULL ||
      target->info.struct_info->is_aggregate ||
      TypeIsCXXInitializerList(target)) {
    return NULL;
  }
  // The constexpr evaluator reconstructs a union value with a designated
  // member initializer.  That list describes the already-selected active
  // member; it is not a source-level constructor argument list.
  for (size_t i = 0; braced->initializers != NULL &&
                     i < braced->initializers->length; i++) {
    ASTNode* element = braced->initializers->value.p[i];
    if (element != NULL && element->op == AST_OP(designated_init)) {
      return NULL;
    }
  }
  StructMember* constructor = FindCXXMemberOverloadHead(
      target->info.struct_info, target->info.struct_info->tag_name);
  if (constructor == NULL || !constructor->is_member_function ||
      constructor->symbol == NULL || constructor->symbol->type == NULL ||
      !TypeIsFunction(constructor->symbol->type) ||
      !constructor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  // `T{}` value-initializes ([dcl.init]/8).  A user-provided default
  // constructor must be run and must *not* be preceded by zero-initialization,
  // so call it here.  A usable defaulted or implicit default constructor is
  // left to the caller's zero-initializing aggregate path, which recurses into
  // the members that still need construction.  Without one (deleted, missing,
  // or only reachable through a constructor template) the call is built so
  // that overload resolution runs or diagnoses it.
  if (braced->initializers == NULL || braced->initializers->length == 0) {
    size_t first_user_formal =
        StructHasVirtualBases(target->info.struct_info) ? 2 : 1;
    bool user_provided_default = false;
    bool usable_implicit_default = false;
    for (StructMember* candidate = constructor;
         candidate != NULL && !user_provided_default;
         candidate = candidate->overload_next) {
      if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
          !TypeIsFunction(candidate->symbol->type)) {
        continue;
      }
      FunctionInfo* info = &candidate->symbol->type->info.function;
      if (!info->is_constructor || info->is_deleted ||
          FunctionExtraOf(info)->template_parameter_count > 0 ||
          info->prototype.length < first_user_formal) {
        continue;
      }
      bool callable_without_arguments = true;
      for (size_t i = first_user_formal; i < info->prototype.length; i++) {
        Symbol* formal = info->prototype.value.p[i];
        if (formal == NULL || SymbolExtraOf(formal)->default_argument == NULL) {
          callable_without_arguments = false;
          break;
        }
      }
      if (!callable_without_arguments) {
        continue;
      }
      if (info->is_user_provided) {
        user_provided_default = true;
      } else {
        usable_implicit_default = true;
      }
    }
    if (!user_provided_default && usable_implicit_default) {
      return NULL;
    }
  }

  Vector* actuals = NewVector();
  if (CXXBracedInitSelectsInitializerListConstructor((ASTNode*)braced,
                                                      target)) {
    VectorAppend(actuals, ASTNodeMove((ASTNode*)braced));
  } else {
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ASTNode* element = braced->initializers->value.p[i];
      if (element == NULL) {
        continue;
      }
      if (element->op == AST_OP(expr_init)) {
        element = ((ExpressionInitializerASTNode*)element)->expr;
      }
      // Nested aggregate initialization reaches this lowering before the
      // scalar clauses have necessarily been analyzed.  Constructor overload
      // resolution needs their types (notably for `Union{{&object}}`), so type
      // each argument before moving it into the synthesized call.
      if (element != NULL && element->type == NULL) {
        element = AnalyzeExpression(element);
      }
      VectorAppend(actuals, ASTNodeMove(element));
    }
    braced->initializers->length = 0;
  }
  if (TypeIsStructOrUnion(target) && target->info.struct_info != NULL &&
      StructHasVirtualBases(target->info.struct_info)) {
    VectorInsertBefore(
        actuals, 0,
        NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location));
  }

  TypeRecord* type = TypeRecordCopy(target);
  SetQualifiers(type, kQualPlain);
  TypeRecordCalculateSize(type);
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
  temp->location = location;
  ASTNode* member = NewStringConstantASTNode(
      NewString(constructor->symbol->name.value), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location,
                       NewIdentifierASTNode(temp, location), member);
  ASTNode* constructor_call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  ASTNode* comma =
      NewBinaryASTNode(AST_OP(comma), TypeRecordCopy(type), location,
                       constructor_call, NewIdentifierASTNode(temp, location));
  brace_ctor_depth++;
  ASTNode* analyzed = AnalyzeExpression(comma);
  brace_ctor_depth--;
  if (analyzed != NULL) {
    analyzed->value_category = kValueCategoryPrvalue;
  }
  return analyzed;
}

// Contract: Thin wrapper: braced_init AST to LowerCXXBracedInitToConstructorCall.
ASTNode* LowerCXXBracedClassInitToConstructor(ASTNode* braced,
                                              TypeRecord* target) {
  if (braced == NULL || braced->op != AST_OP(braced_init)) {
    return NULL;
  }
  return LowerCXXBracedInitToConstructorCall(
      (BracedInitializerASTNode*)braced, target, braced->location);
}

// Contract: Lowers scalar braced-init (`{}` or `{v}`) to analyzed scalar
// expressions for `target`. Does not handle class/aggregate temporaries.
static ASTNode* LowerScalarBracedInitToTarget(BracedInitializerASTNode* braced,
                                              TypeRecord* target,
                                              SourceLocation location) {
  if (braced->initializers->length == 0) {
    ASTNode* zero =
        TypeIsFloatingPoint(target)
            ? NewRealConstantASTNode(0.0, TypeRecordCopy(target), location)
        : TypeIsMemberPointer(target)
            ? NewNullMemberPointerConstant(target, location)
            : NewIntConstantASTNode(0, TypeRecordCopy(target), location);
    return AnalyzeExpression(zero);
  }
  ASTNode* element = braced->initializers->value.p[0];
  if (element != NULL && element->op == AST_OP(expr_init)) {
    element = ((ExpressionInitializerASTNode*)element)->expr;
  }
  element = AnalyzeExpression(element);
  DiagnoseScalarNarrowing(element, target);
  ASTNode* cast = NewCastASTNode(TypeRecordCopy(target), location, element);
  return AnalyzeExpression(cast);
}

// Contract: Lowers a braced-init-list at an expression site to a temporary or
// scalar expression of `target`. Re-parents `braced` into compound literals;
// callers splice with delete_old_child=false. Returns `braced` unchanged when
// lowering does not apply.
ASTNode* LowerCXXBracedInitToTarget(ASTNode* braced, TypeRecord* target) {
  if (braced == NULL || braced->op != AST_OP(braced_init) || target == NULL) {
    return braced;
  }
  SourceLocation location = braced->location;
  BracedInitializerASTNode* b = (BracedInitializerASTNode*)braced;
  if (TypeIsScalar(target) && b->initializers->length <= 1) {
    return LowerScalarBracedInitToTarget(b, target, location);
  }
  ASTNode* constructed = LowerCXXBracedInitToConstructorCall(b, target, location);
  if (constructed != NULL) {
    return constructed;
  }
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(target));
  temp->location = location;
  ASTNode* temp_id = NewIdentifierASTNode(temp, location);
  temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* literal = NewCompoundLiteralASTNode(temp_id, location, braced);
  literal->flags |= kASTCXXBracedTemporary;
  return AnalyzeExpression(literal);
}

// Contract: Constant-only narrowing helper; unknown width => fits (no false +).
static bool NarrowingIntegerConstantFits(int64_t value, TypeRecord* target) {
  int bytes = SizeofType(target->type);
  if (bytes <= 0 || bytes > 8) {
    return true;
  }
  if (TypeIsUnsigned(target)) {
    if (value < 0) {
      return false;
    }
    if (bytes >= 8) {
      return true;
    }
    uint64_t max = (1ULL << (bytes * 8)) - 1;
    return (uint64_t)value <= max;
  }
  if (bytes >= 8) {
    return true;
  }
  int64_t max = (1LL << (bytes * 8 - 1)) - 1;
  int64_t min = -(1LL << (bytes * 8 - 1));
  return value >= min && value <= max;
}

// Contract: Emits diagnostics only for unambiguous scalar list-init narrowing
// ([dcl.init.list]/7); runtime-dependent cases are skipped.
static void DiagnoseScalarNarrowing(ASTNode* source, TypeRecord* target) {
  if (!CompilerIsCXX() || source == NULL || target == NULL) {
    return;
  }
  TypeRecord* src = source->type;
  if (src == NULL || !TypeIsIntegral(target)) {
    return;
  }
  bool narrowing = false;
  if (TypeIsFloatingPoint(src)) {
    // Floating-point to integer is always a narrowing conversion.
    narrowing = true;
  } else if (TypeIsIntegral(src) && !TypeIsBool(target)) {
    // A constant integer that does not fit in the target is narrowing.  A
    // successful integer constant evaluation is exactly the standard's
    // "constant expression" gate, so non-constant operands are skipped.
    int64_t value = 0;
    if (EvaluateIntegerExpression(source, &value) &&
        !NarrowingIntegerConstantFits(value, target)) {
      narrowing = true;
    }
  }
  if (!narrowing) {
    return;
  }
  String from_name;
  String to_name;
  StringInit(&from_name, "");
  StringInit(&to_name, "");
  TypeRecordToString(src, &from_name);
  TypeRecordToString(target, &to_name);
  SemanticError(source,
                "narrowing conversion from '%s' to '%s' in list-initialization",
                from_name.value, to_name.value);
  StringDestruct(&from_name);
  StringDestruct(&to_name);
}

// Contract: Builds a braced-init tree copying `source` element-wise into fixed
// array shape `type`; `indices` holds partial subscripts at `rank`.
ASTNode* NewArrayElementwiseCopy(ASTNode* source, TypeRecord* type,
                                        int64_t* indices, int rank,
                                        SourceLocation location) {
  if (!TypeIsFixedArray(type)) {
    ASTNode* element = ASTNodeClone(source, IdentityCloneNode, NULL, NULL);
    for (int i = 0; i < rank; i++) {
      ASTNode* index = NewIntConstantASTNode(
          indices[i], NewTypeRecordWithSize(kTypeLong, kQualPlain), location);
      element = NewBinaryASTNode(AST_OP(subscript), NULL, location, element,
                                 index);
    }
    if (TypeIsStructOrUnion(type)) {
      element = NewCastASTNode(type, location, element);
    }
    return element;
  }
  Vector* elements = NewVector();
  for (int64_t i = 0; i < (int64_t)type->info.array.size.fixed; i++) {
    indices[rank] = i;
    VectorAppend(elements, NewArrayElementwiseCopy(source, type->next,
                                                   indices, rank + 1,
                                                   location));
  }
  return NewBracedInitializerASTNode(elements, NULL, location);
}

// Contract: Returns nested fixed-array rank, or -1 if a flexible array appears.
int FixedArrayRank(TypeRecord* type) {
  int rank = 0;
  for (; TypeIsFixedArray(type); type = type->next) {
    rank++;
  }
  return TypeIsArray(type) ? -1 : rank;
}

// Contract: True for identifier/member/deref paths safe to duplicate in array copy.
bool LvaluePathIsRepeatable(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  switch (node->op) {
    case AST_OP(identifier):
      return true;
    case AST_OP(arrow):
    case AST_OP(dot): {
      BinaryASTNode* access = (BinaryASTNode*)node;
      return LvaluePathIsRepeatable(access->left);
    }
    case AST_OP(contents):
      return LvaluePathIsRepeatable(((UnaryASTNode*)node)->sub);
    default:
      return false;
  }
}

// Contract: When the target is a reference, marks the peeled initializer
// expression with kASTNeedAddress. Does not analyze or replace `init`.
static void SetReferenceInitializerNeedAddress(ASTNode* target, ASTNode* init) {
  if (target == NULL || target->type == NULL ||
      !TypeIsReference(target->type)) {
    return;
  }
  ASTNode* reference_initializer = init;
  while (reference_initializer != NULL &&
         (reference_initializer->op == AST_OP(expr_init) ||
          reference_initializer->op == AST_OP(braced_init) ||
          reference_initializer->op == AST_OP(designated_init))) {
    if (reference_initializer->op == AST_OP(expr_init)) {
      reference_initializer =
          ((ExpressionInitializerASTNode*)reference_initializer)->expr;
    } else if (reference_initializer->op == AST_OP(braced_init)) {
      BracedInitializerASTNode* braced =
          (BracedInitializerASTNode*)reference_initializer;
      reference_initializer =
          braced->initializers != NULL &&
                  braced->initializers->length == 1
              ? braced->initializers->value.p[0]
              : NULL;
    } else {
      reference_initializer =
          ((DesignatedInitializerASTNode*)reference_initializer)->init;
    }
  }
  if (reference_initializer != NULL) {
    reference_initializer->flags |= kASTNeedAddress;
  }
}

// Contract: Performs `auto` deduction and post-deduction rewrites (copy-init
// constructor, structured-binding array copy). Returns non-NULL to exit
// AnalyzeInitialization early; otherwise may update *init_io.
static ASTNode* AnalyzeAutoDeductionAndRelatedRewrites(
    ASTNode* node, ASTNode* target, Symbol* symbol, ASTNode* init,
    ASTNode** init_io) {
  bool deduced_auto = false;
  if (symbol != NULL && TypeContainsAuto(symbol->type)) {
    if (!SemanticDeduceAutoType(symbol, init, node)) {
      return init;
    }
    if (TypeContainsAuto(symbol->type)) {
      return init;
    }
    ASTNodeSetType(target, symbol->type);
    ASTNodeSetType(node, symbol->type);
    deduced_auto = true;
  }
  if (deduced_auto && TypeIsStructOrUnion(symbol->type) &&
      init->op == AST_OP(expr_init)) {
    BinaryASTNode* initialization = (BinaryASTNode*)node;
    initialization->right = NULL;
    ASTNode* rewritten = SyntaxRewriteCXXCopyInitConstructorIfNeeded(
        &compiler->syntax, symbol, init);
    if (rewritten != init) {
      return AnalyzeExpression(rewritten);
    }
    initialization->right = init;
  }
  if (deduced_auto && symbol->is_structured_binding_object &&
      init->op == AST_OP(expr_init)) {
    ASTNode* source = ((ExpressionInitializerASTNode*)init)->expr;
    int rank = FixedArrayRank(symbol->type);
    if (source != NULL && TypeIsArray(source->type) && rank > 0 &&
        rank <= kArrayElementwiseCopyMaxRank &&
        LvaluePathIsRepeatable(source)) {
      int64_t indices[kArrayElementwiseCopyMaxRank];
      ASTNode* copy = NewArrayElementwiseCopy(source, symbol->type, indices, 0,
                                              source->location);
      ASTNodeReplaceChild(node, 1, copy, true);
      *init_io = AnalyzeExpression(copy);
    }
  }
  return NULL;
}

// Contract: Analyzes reference binding for one expression-initializer: base
// subobject binding, conversions, diagnostics, and temporary materialization.
static void AnalyzeReferenceExpressionInit(ExpressionInitializerASTNode* e,
                                             TypeRecord* reference_type) {
  bool rvalue_ref = reference_type->declarator == kDeclRValueReference;
  bool discards_qualifiers =
      TypeIsEffectivelyConst(e->expr->type) &&
      !TypeIsEffectivelyConst(reference_type->next);
  if (!TypeEqualIgnoringQualifiers(e->expr->type, reference_type->next)) {
    ASTNode* base_bound =
        TryBindReferenceToBaseSubobject(e->expr, reference_type->next);
    if (base_bound != NULL) {
      ASTNodeReplaceChild((ASTNode*)e, 0, base_bound, false);
      e->expr = base_bound;
    } else {
      if (TypeIsScalar(e->expr->type)) {
        e->expr->flags &= ~kASTNeedAddress;
      }
      NormalConversion(e->expr,
                       ReferenceConversionTarget(e->expr, reference_type));
    }
  }
  if (discards_qualifiers) {
    SemanticError(e->expr, "Reference initializer discards qualifiers");
  } else if (!ReferenceCanBind(e->expr, reference_type)) {
    if (rvalue_ref) {
      SemanticError(e->expr,
                    "Rvalue reference initializer must not be an lvalue");
    } else if (TypeIsConst(reference_type->next)) {
      SemanticError(e->expr,
                    "Const reference initializer has incompatible type");
    } else {
      SemanticError(e->expr, "Reference initializer must be an lvalue");
    }
  }
  if (ReferenceCanBind(e->expr, reference_type) && !HasAddress(e->expr)) {
    ASTNode* materialized =
        MaterializeTemporary(e->expr, reference_type->next);
    ASTNodeReplaceChild((ASTNode*)e, 0, materialized, false);
    e->expr = materialized;
  }
  e->expr->flags |= kASTNeedAddress;
}

// Contract: Applies normal initialization conversions to a non-reference
// expression-initializer (constructor-call elision, variable-template
// materialization, NormalConversion).
static void AnalyzeNonReferenceExpressionInit(ASTNode* target,
                                                ExpressionInitializerASTNode* e) {
  bool constructor_call = false;
  if (e->expr->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)e->expr;
    if (call->left != NULL && call->left->op == AST_OP(identifier)) {
      Symbol* callee = ((IdentifierASTNode*)call->left)->symbol;
      constructor_call = callee != NULL && TypeIsFunction(callee->type) &&
                         callee->type->info.function.is_constructor;
    }
  } else if (ASTIsInlinedConstructor(e->expr) &&
             TypeIsStructOrUnion(target->type)) {
    constructor_call = true;
  }
  bool cxx_return_elision_initializer =
      CompilerIsCXX() && e->expr->op == AST_OP(call) &&
      e->expr->value_category != kValueCategoryXvalue && !constructor_call &&
      TypeIsStructOrUnion(target->type) &&
      TypeEqual(e->expr->type, target->type);
  ASTNode* materialized = CXXMaterializeVariableTemplateExpression(e->expr);
  if (materialized != NULL) {
    ASTNodeReplaceChild((ASTNode*)e, 0, materialized, true);
    e->expr = materialized;
  }
  if (!constructor_call && !cxx_return_elision_initializer) {
    SemanticConvertType(
        e->expr, target->type,
        e->is_direct_initializer ? kConvertDirectInit : kConvertNormal);
  }
}

// Contract: Handles braced-init for a known target type. May replace *init_io
// with lowered std::initializer_list form; otherwise diagnoses scalar narrowing.
static void AnalyzeBracedInitForTarget(ASTNode* target, bool is_reference_init,
                                       ASTNode** init_io) {
  ASTNode* init = *init_io;
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)init;
  if (!is_reference_init && TypeIsCXXInitializerList(target->type) &&
      !TypeIsCXXInitializerList(init->type)) {
    *init_io = LowerCXXInitializerListBracedInit(target->type, braced,
                                                 init->location);
    return;
  }
  if (!is_reference_init && TypeIsScalar(target->type) &&
      braced->initializers->length == 1) {
    ASTNode* initializer = braced->initializers->value.p[0];
    if (initializer->op == AST_OP(designated_init)) {
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)initializer;
      NormalConversion(designated->init, target->type);
      DiagnoseScalarNarrowing(designated->init, target->type);
    } else {
      ASTNode* value_expr = initializer;
      if (value_expr->op == AST_OP(expr_init)) {
        value_expr = ((ExpressionInitializerASTNode*)value_expr)->expr;
      }
      DiagnoseScalarNarrowing(value_expr, target->type);
    }
  }
}

// Contract: Dispatches initializer analysis by AST opcode and target kind
// (reference vs value, std::initializer_list, scalar braced-init).
static void AnalyzeInitializerByKind(ASTNode* target, ASTNode** init_io) {
  ASTNode* init = *init_io;
  bool is_reference_init = TypeIsReference(target->type);
  switch (init->op) {
    case AST_OP(expr_init): {
      ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)init;
      if (is_reference_init) {
        AnalyzeReferenceExpressionInit(e, target->type);
      } else if (e->expr != NULL && e->expr->op == AST_OP(braced_init) &&
                 TypeIsCXXInitializerList(target->type) &&
                 !TypeIsCXXInitializerList(e->expr->type)) {
        *init_io = LowerCXXInitializerListBracedInit(
            target->type, (BracedInitializerASTNode*)e->expr,
            e->expr->location);
      } else {
        AnalyzeNonReferenceExpressionInit(target, e);
      }
      break;
    }
    case AST_OP(braced_init):
      AnalyzeBracedInitForTarget(target, is_reference_init, init_io);
      break;
    default:
      break;
  }
}

// Contract: Fills compile-time member-pointer symbol values when the initializer
// is a constant or structmember form. Does not replace AST nodes.
static void ApplyMemberPointerConstantSymbolValue(Symbol* symbol, ASTNode* init) {
  if (!TypeIsMemberPointer(symbol->type)) {
    return;
  }
  ASTNode* init_expr = init;
  if (init_expr != NULL && init_expr->op == AST_OP(expr_init)) {
    init_expr = ((ExpressionInitializerASTNode*)init_expr)->expr;
  }
  if (init_expr != NULL && init_expr->op == AST_OP(member_ptr)) {
    UnaryASTNode* unary = (UnaryASTNode*)init_expr;
    if (unary->sub != NULL && unary->sub->op == AST_OP(structmember)) {
      StructMember* member = ((StructMemberASTNode*)unary->sub)->member;
      Struct* class_info = TypeMemberPointerClass(symbol->type);
      MemberPointerValue pm_value;
      if (member != NULL && class_info != NULL &&
          MemberPointerEncodeFromMember(member, class_info, &pm_value)) {
        symbol->value.other = member;
        symbol->flags.value_is_pointed_member = true;
        symbol->flags.value_set = true;
      }
    }
  } else if (init_expr != NULL) {
    MemberPointerValue pm_value;
    if (MemberPointerTryEvaluateConstant(init_expr, symbol->type, &pm_value)) {
      StructMember* member = MemberPointerMemberFromExpression(init_expr);
      if (member != NULL) {
        symbol->value.other = member;
      } else {
        symbol->value.ivalue = pm_value.ptr;
      }
      symbol->flags.value_is_pointed_member = member != NULL;
      symbol->flags.value_set = true;
    }
  }
}

// Contract: Computes whether AnalyzeInitializer must reject non-constant forms
// and whether the symbol is a C++ function-local static. Updates *constants_only_io.
static void ComputeConstantsOnlyFlags(Symbol* symbol, ASTNode* init,
                                      bool is_static_storage,
                                      bool is_thread_local,
                                      bool* constants_only_io,
                                      bool* is_cxx_local_static_out) {
  *constants_only_io = is_static_storage;
  *is_cxx_local_static_out =
      CompilerIsCXX() && compiler->current_function != NULL &&
      StorageIs(symbol->storage, STO(static)) && !is_thread_local;
  if (is_thread_local) {
    ASTNode* init_expr = init;
    if (init_expr->op == AST_OP(expr_init)) {
      init_expr = ((ExpressionInitializerASTNode*)init_expr)->expr;
    }
    if (init_expr != NULL && init_expr->op != AST_OP(braced_init)) {
      *constants_only_io = IsConstantExpression(init_expr);
    }
  }
  if (CompilerIsCXX() && !is_thread_local && !symbol->flags.is_constexpr &&
      !symbol->flags.is_constinit) {
    ASTNode* init_expr = init;
    if (init_expr != NULL && init_expr->op == AST_OP(expr_init)) {
      init_expr = ((ExpressionInitializerASTNode*)init_expr)->expr;
    }
    bool static_const_constant =
        StorageIs(symbol->storage, STO(static)) && TypeIsConst(symbol->type) &&
        init_expr != NULL && IsConstantExpression(init_expr);
    if (!static_const_constant) {
      *constants_only_io = false;
    }
  }
}

// Contract: Completes semantic analysis and AST lowering for an identifier
// initialization: constant folding, AnalyzeInitializer, static-init flags.
// Replaces child 1 of `node` and returns the simplified initializer.
static ASTNode* AnalyzeSymbolBackedInitialization(ASTNode* node,
                                                    Symbol* symbol,
                                                    ASTNode* init,
                                                    bool is_static_storage,
                                                    bool is_thread_local) {
  bool constants_only = false;
  bool is_cxx_local_static = false;
  ComputeConstantsOnlyFlags(symbol, init, is_static_storage, is_thread_local,
                            &constants_only, &is_cxx_local_static);
  MarkCXX26SymbolicConstexprReference(symbol, init);

  if (init != NULL && init->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)init;
    ASTNode* folded = CXXFoldIsSameVExpression(expr_init->expr);
    if (folded != NULL && folded != expr_init->expr) {
      ASTNodeReplaceChild((ASTNode*)expr_init, 0, folded, true);
      expr_init->expr = folded;
    }
  }

  if (!symbol->flags.is_template &&
      (TypeIsConst(symbol->type) || symbol->flags.is_constexpr ||
       symbol->flags.is_constinit)) {
    EvaluateConstantForSymbol(symbol, init);
  }
  ASTNode* object_init =
      ConstexprObjectInitializerForSymbol(symbol, init->location);
  if (object_init != NULL) {
    init = object_init;
  }
  bool requires_constant_initializer =
      constants_only || object_init != NULL ||
      ((symbol->flags.is_constexpr || symbol->flags.is_constinit) &&
       !symbol->is_constexpr_representable);
  ApplyMemberPointerConstantSymbolValue(symbol, init);
  ASTNode* simplified_init =
      AnalyzeInitializer(node->type, init, requires_constant_initializer);
  ASTNodeReplaceChild(node, 1, simplified_init, true);
  if ((symbol->flags.is_constexpr || symbol->flags.is_constinit) &&
      !symbol->flags.is_template && !symbol->flags.value_set) {
    bool constant = EvaluateConstantForSymbol(symbol, simplified_init);
    if (!symbol->flags.value_set && !symbol->is_constexpr_representable &&
        !(constant && TypeIsReference(symbol->type)) &&
        !ExpressionIsTemplateDependent(simplified_init)) {
      SemanticError(
          simplified_init,
          symbol->flags.is_constinit
              ? "constinit variable initializer is not a constant expression"
              : "constexpr variable initializer is not a constant expression");
    }
  }

  bool link_time_constant = InitializerIsLinkTimeConstant(simplified_init);
  if (is_static_storage && link_time_constant) {
    node->flags |= kASTStaticInit;
  }
  if (is_cxx_local_static && node->parent != NULL &&
      node->parent->op == AST_OP(vardecl)) {
    VariableDeclarationASTNode* declaration =
        (VariableDeclarationASTNode*)node->parent;
    declaration->local_static_init_kind =
        link_time_constant ? kLocalStaticInitConstant : kLocalStaticInitDynamic;
  }
  return simplified_init;
}

// Contract: Analyzes one initialization AST (`node` = target init expr).
// Sets types on `node` and the initializer, runs target-kind dispatch, then
// either AnalyzeInitializer for anonymous targets or symbol-backed completion.
ASTNode* AnalyzeInitialization(ASTNode* node,
                                      ASTNode* target, ASTNode* init) {
  target = AnalyzeExpression(target);
  SetReferenceInitializerNeedAddress(target, init);
  init = SemanticAnalyzeDeducedInitializer(
      target != NULL && target->op == AST_OP(identifier)
          ? ((IdentifierASTNode*)target)->symbol
          : NULL,
      init);
  if (init == NULL) {
    return NULL;
  }
  Symbol* symbol = target != NULL && target->op == AST_OP(identifier)
                       ? ((IdentifierASTNode*)target)->symbol
                       : NULL;
  if (symbol != NULL &&
      ASTNodeAny(init, ExpressionRequiresASTConstexpr, NULL)) {
    symbol->requires_ast_constexpr = true;
  }
  if (target == NULL || target->type == NULL) {
    SemanticError(node, "Initialization target has no type");
    return init;
  }
  ASTNode* early = AnalyzeAutoDeductionAndRelatedRewrites(
      node, target, symbol, init, &init);
  if (early != NULL) {
    return early;
  }
  AnalyzeInitializerByKind(target, &init);
  ASTNodeSetType((ASTNode*)init, target->type);
  ASTNodeSetType(node, target->type);

  if (!IsAssignable(target, true)) {
    SemanticError(target, "Cannot initialize a variable of this type");
    return init;
  }

  if (symbol == NULL) {
    ASTNode* simplified_init = AnalyzeInitializer(node->type, init, false);
    ASTNodeReplaceChild(node, 1, simplified_init, true);
    return simplified_init;
  }

  bool is_static_storage = StorageIs(symbol->storage, STO(static)) ||
                           StorageIs(symbol->storage, STO(extern));
  bool is_thread_local = StorageIs(symbol->storage, STO(thread));
  return AnalyzeSymbolBackedInitialization(node, symbol, init,
                                           is_static_storage, is_thread_local);
}

// Contract: Converts compound-assignment RHS; float+=double keeps RHS in double
// for precision; otherwise NormalConversion to the left operand type.
void ConvertCompoundAssignmentOperand(BinaryASTNode* node) {
  bool left_float =
      TypeUsesFloat32Representation(node->left->type) ||
      (TypeIsComplex(node->left->type) &&
       TypeComplexElementType(node->left->type) == kTypeFloat);
  bool right_double =
      TypeUsesFloat64Representation(node->right->type) ||
      (TypeIsComplex(node->right->type) &&
       TypeComplexElementType(node->right->type) != kTypeFloat);
  if (left_float && right_double) {
    TypeRecord* operation_type =
        TypeIsComplex(node->left->type) ||
                TypeIsComplex(node->right->type)
            ? NewComplexTypeRecord(kTypeDouble, kQualPlain)
            : NewTypeRecordWithSize(kTypeDouble, kQualPlain);
    NormalConversion(node->right, operation_type);
    return;
  }
  NormalConversion(node->right, node->left->type);
}
