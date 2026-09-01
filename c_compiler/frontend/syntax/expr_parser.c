//
//  expr_parser.c
//  c_compiler
//
//  Created by David Allison on 10/30/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdint.h>

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "concepts.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "preprocessor.h"
#include "reflection.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_internal.h"
#include "compiler.h"
#include "type_traits_semantics.h"

static struct Intrinsic {
  const char* name;
  ASTOpcode opcode;
  int num_args;
} intrinsics[] = {
    {"__atomic_add_fetch", AST_OP(builtin_atomic_add_fetch), 3},
    {"__atomic_compare_exchange_n", AST_OP(builtin_atomic_compare_exchange_n), 6},
    {"__atomic_fetch_add", AST_OP(builtin_atomic_fetch_add), 3},
    {"__atomic_fetch_sub", AST_OP(builtin_atomic_fetch_sub), 3},
    {"__atomic_load_n", AST_OP(builtin_atomic_load), 2},
    {"__atomic_signal_fence", AST_OP(builtin_atomic_fence), 1},
    {"__atomic_store_n", AST_OP(builtin_atomic_store), 3},
    {"__atomic_sub_fetch", AST_OP(builtin_atomic_sub_fetch), 3},
    {"__atomic_thread_fence", AST_OP(builtin_atomic_fence), 1},
    {"__builtin_COLUMN", AST_OP(builtin_source_column), 0},
    {"__builtin_FILE", AST_OP(builtin_source_file), 0},
    {"__builtin_FUNCTION", AST_OP(builtin_source_function), 0},
    {"__builtin_LINE", AST_OP(builtin_source_line), 0},
    {"__builtin_PRETTY_FUNCTION", AST_OP(builtin_source_pretty_function), 0},
    {"__builtin_expect", AST_OP(builtin_expect), 2},
    {"__builtin_observable_checkpoint",
     AST_OP(builtin_observable_checkpoint), 0},
    {"__builtin_prefetch", AST_OP(builtin_prefetch), 3},
    {"__builtin_start_lifetime", AST_OP(builtin_start_lifetime), 1},
    {"__builtin_trap", AST_OP(builtin_trap), 0},
    {"__builtin_unreachable", AST_OP(builtin_unreachable), 0},
    {"__builtin_va_arg", AST_OP(builtin_va_arg), 2},
    {"__builtin_va_copy", AST_OP(builtin_va_copy), 2},
    {"__builtin_va_end", AST_OP(builtin_va_end), 1},
    {"__builtin_va_start", AST_OP(builtin_va_start), 2},
    {"__davecc_clz", AST_OP(builtin_clz), 2},
    {"__davecc_ctz", AST_OP(builtin_ctz), 2},
    {"__davecc_popcount", AST_OP(builtin_popcount), 2},
    {"__davecc_rotl", AST_OP(builtin_rotl), 2},
    {"__davecc_rotr", AST_OP(builtin_rotr), 2},
    {"__sync_add_and_fetch", AST_OP(builtin_atomic_add_fetch), 2},
    {"__sync_bool_compare_and_swap", AST_OP(builtin_atomic_compare_exchange_bool), 3},
    {"__sync_fetch_and_add", AST_OP(builtin_atomic_fetch_add), 2},
    {"__sync_fetch_and_sub", AST_OP(builtin_atomic_fetch_sub), 2},
    {"__sync_sub_and_fetch", AST_OP(builtin_atomic_sub_fetch), 2},
    {"__sync_synchronize", AST_OP(builtin_atomic_fence), 0},
    {"__sync_val_compare_and_swap", AST_OP(builtin_atomic_compare_exchange_val), 3},
};

// The lambda's default capture mode: none ([]), by-value ([=]) or by-reference
// ([&]).  It governs how identifiers used in the body but not named explicitly
// are implicitly captured.
typedef enum {
  kLambdaCaptureDefaultNone,
  kLambdaCaptureDefaultValue,
  kLambdaCaptureDefaultReference,
} LambdaCaptureDefault;

// One captured entity of a lambda.  Each capture becomes a member of the
// synthesized closure class.
typedef struct {
  Symbol* captured;          // Enclosing variable (or init-capture local) being captured.
  Symbol* field;             // Closure-class member created for the capture.
  ASTNode* initializer;      // Initializer expression for an init-capture, else NULL.
  bool by_reference;         // Captured by reference (stored as a pointer field).
  bool is_pack_expansion;    // Capture expands a parameter pack (e.g. [...xs]).
  bool is_init_capture;      // Init-capture ([x = expr]) introducing a new name.
} LambdaCapture;

static ASTNode* ParseAssignmentExpression(Syntax* syntax,
                                          TokenClass followers);

static int CompareIntrinsicName(const void* key, const void* element) {
  const char* name = key;
  const struct Intrinsic* intrinsic = element;
  return strcmp(name, intrinsic->name);
}

static const struct Intrinsic* GetIntrinsic(const char* name) {
  return bsearch(name, intrinsics,
                 sizeof(intrinsics) / sizeof(intrinsics[0]),
                 sizeof(intrinsics[0]), CompareIntrinsicName);
}

static bool IntrinsicAvailableOnCurrentTarget(
    const struct Intrinsic* intrinsic) {
  if (intrinsic == NULL) {
    return false;
  }
  bool atomic = false;
  switch (intrinsic->opcode) {
    case AST_OP(builtin_atomic_load):
    case AST_OP(builtin_atomic_store):
    case AST_OP(builtin_atomic_fetch_add):
    case AST_OP(builtin_atomic_fetch_sub):
    case AST_OP(builtin_atomic_add_fetch):
    case AST_OP(builtin_atomic_sub_fetch):
    case AST_OP(builtin_atomic_compare_exchange_bool):
    case AST_OP(builtin_atomic_compare_exchange_val):
    case AST_OP(builtin_atomic_compare_exchange_n):
    case AST_OP(builtin_atomic_fence):
      atomic = true;
      break;
    default:
      break;
  }
  return !atomic || CompilerTargetSupportsAtomics();
}

static bool IsBuiltinCallName(const char* name) {
  if (GetIntrinsic(name) != NULL) {
    return true;
  }
  return strncmp(name, "__davecc_is_", 12) == 0;
}

typedef struct {
  const char* name;
  CXXTypeTraitKind kind;
} TypeTraitName;

static const TypeTraitName kTypeTraitNames[] = {
    {"__davecc_is_assignable", kCXXTypeTraitIsAssignable},
    {"__davecc_is_base_of", kCXXTypeTraitIsBaseOf},
    {"__davecc_is_class", kCXXTypeTraitIsClass},
    {"__davecc_is_constructible", kCXXTypeTraitIsConstructible},
    {"__davecc_is_convertible", kCXXTypeTraitIsConvertible},
    {"__davecc_is_destructible", kCXXTypeTraitIsDestructible},
    {"__davecc_is_enum", kCXXTypeTraitIsEnum},
    {"__davecc_is_invocable", kCXXTypeTraitIsInvocable},
    {"__davecc_is_member_function_pointer",
     kCXXTypeTraitIsMemberFunctionPointer},
    {"__davecc_is_member_object_pointer", kCXXTypeTraitIsMemberObjectPointer},
    {"__davecc_is_member_pointer", kCXXTypeTraitIsMemberPointer},
    {"__davecc_is_member_pointer_direct_object",
     kCXXTypeTraitMemberPointerDirectObject},
    {"__davecc_is_nothrow_assignable", kCXXTypeTraitIsNothrowAssignable},
    {"__davecc_is_nothrow_constructible",
     kCXXTypeTraitIsNothrowConstructible},
    {"__davecc_is_nothrow_destructible", kCXXTypeTraitIsNothrowDestructible},
    {"__davecc_is_nothrow_invocable", kCXXTypeTraitIsNothrowInvocable},
    {"__davecc_is_swappable", kCXXTypeTraitIsSwappable},
    {"__davecc_is_swappable_with", kCXXTypeTraitIsSwappableWith},
    {"__davecc_is_trivially_assignable", kCXXTypeTraitIsTriviallyAssignable},
    {"__davecc_is_trivially_constructible",
     kCXXTypeTraitIsTriviallyConstructible},
    {"__davecc_is_trivially_copyable", kCXXTypeTraitIsTriviallyCopyable},
    {"__davecc_is_trivially_destructible",
     kCXXTypeTraitIsTriviallyDestructible},
    {"__davecc_is_union", kCXXTypeTraitIsUnion},
};

static CXXTypeTraitKind TypeTraitKindFromName(const char* name) {
  if (name == NULL || name[0] != '_' || name[1] != '_') {
    return (CXXTypeTraitKind)-1;
  }
  size_t low = 0;
  size_t high = sizeof(kTypeTraitNames) / sizeof(kTypeTraitNames[0]);
  while (low < high) {
    size_t middle = low + (high - low) / 2;
    int comparison = strcmp(name, kTypeTraitNames[middle].name);
    if (comparison < 0) {
      high = middle;
    } else if (comparison > 0) {
      low = middle + 1;
    } else {
      return kTypeTraitNames[middle].kind;
    }
  }
  return (CXXTypeTraitKind)-1;
}

static Vector* ParseTypeTraitTypeArguments(Syntax* syntax, TokenClass followers) {
  Vector* type_args = NewVector();
  while (!LexLookingAt(syntax->lex, TOK(rparen))) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), kParsingBlockScope);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    type = sym->type;
    ASTNode* type_node =
        NewIntConstantASTNode(0, TypeRecordCopy(type), syntax->lex->current_token_location);
    if (sym->flags.is_parameter_pack ||
        (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(ellipsis)))) {
      if (LexMatch(syntax->lex, TOK(ellipsis))) {
        type_node->flags |= kASTPackExpansion;
      } else if (sym->flags.is_parameter_pack) {
        type_node->flags |= kASTPackExpansion;
      }
    }
    VectorAppend(type_args, type_node);
    SymbolDelete(sym);
    TypeParserDestruct(&parser);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return type_args;
}

static ASTNode* TypeTraitIntrinsic(Syntax* syntax, ASTNode* left,
                                   TokenClass followers) {
  if (left->op != AST_OP(identifier)) {
    return NULL;
  }
  IdentifierASTNode* id_node = (IdentifierASTNode*)left;
  const char* name = id_node->symbol->name.value;
  CXXTypeTraitKind kind = TypeTraitKindFromName(name);
  if ((int)kind < 0) {
    return NULL;
  }
  Vector* type_args = ParseTypeTraitTypeArguments(syntax, followers);
  ASTNode* kind_node = NewIntConstantASTNode(
      (int)kind, NewTypeRecordWithSize(kTypeInt, kQualPlain),
      syntax->lex->current_token_location);
  Vector* actuals = NewVector();
  VectorAppend(actuals, kind_node);
  for (size_t i = 0; i < type_args->length; i++) {
    VectorAppend(actuals, type_args->value.p[i]);
  }
  VectorDelete(type_args);
  return NewVectorASTNode(AST_OP(builtin_type_trait), NULL,
                          syntax->lex->current_token_location, left, actuals);
}

static Symbol* FindThisSymbol(Syntax* syntax) {
  String this_name;
  StringInit(&this_name, "this");
  Symbol* symbol = SyntaxFindSymbol(syntax, &this_name);
  StringDestruct(&this_name);
  return symbol;
}

static ASTNode* NewMemberAccessFromThis(Syntax* syntax,
                                        FullyQualifiedIdentifier* name,
                                        bool allow_unresolved_member) {
  if (name->is_qualified) {
    return NULL;
  }
  Symbol* this_symbol = FindThisSymbol(syntax);
  if (this_symbol == NULL || this_symbol->type == NULL ||
      !TypeIsStructOrUnionPointer(this_symbol->type) ||
      this_symbol->type->next == NULL ||
      this_symbol->type->next->info.struct_info == NULL) {
    return NULL;
  }

  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(name));
  StructMember* member =
      FindStructMember(this_symbol->type->next->info.struct_info, &member_name);
  if (member != NULL && member->is_static) {
    StringDestruct(&member_name);
    return NULL;
  }
  if (member == NULL && !allow_unresolved_member) {
    StringDestruct(&member_name);
    return NULL;
  }

  ASTNode* left =
      NewIdentifierASTNode(this_symbol, syntax->lex->current_token_location);
  ASTNode* right = NewStringConstantASTNode(NewString(member_name.value), NULL,
                                            syntax->lex->current_token_location);
  if (member != NULL && member->symbol != NULL &&
      member->symbol->flags.name_independent_lookup_ambiguous) {
    right->flags |= kASTNameIndependentLookupAmbiguous;
  }
  StringDestruct(&member_name);
  return NewBinaryASTNode(AST_OP(arrow), NULL,
                          syntax->lex->current_token_location, left, right);
}

static ASTNode* NewQualifiedBaseMemberAccessFromThis(
    Syntax* syntax, FullyQualifiedIdentifier* name) {
  if (!CompilerIsCXX() || name == NULL || !name->is_qualified ||
      name->components.length < 2) {
    return NULL;
  }
  Symbol* this_symbol = FindThisSymbol(syntax);
  if (this_symbol == NULL || this_symbol->type == NULL ||
      !TypeIsStructOrUnionPointer(this_symbol->type) ||
      this_symbol->type->next == NULL ||
      this_symbol->type->next->info.struct_info == NULL) {
    return NULL;
  }
  // Inside a template definition, any qualified base member access must stay
  // dependent until the template is instantiated; binding it while parsing the
  // primary template (especially once complete bases like basic_ostream are
  // visible) leaves owner/member metadata tied to the primary and the derived
  // class's hidden overloads win later.
  if (syntax->current_template_parameter_count > 0) {
    return NULL;
  }
  Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
      syntax, name, name->components.length - 1);
  if (owner == NULL || owner->type == NULL ||
      !TypeIsStructOrUnion(owner->type) ||
      owner->type->info.struct_info == NULL) {
    return NULL;
  }
  if (TypeContainsTemplateParameter(owner->type)) {
    return NULL;
  }
  TypeRecord* receiver_type = this_symbol->type->next;
  if (receiver_type->info.struct_info != owner->type->info.struct_info &&
      !TypeIsDerivedFrom(receiver_type, owner->type)) {
    return NULL;
  }
  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(name));
  StructMember* member = FindStructMember(owner->type->info.struct_info,
                                          &member_name);
  if (member == NULL || member->is_static) {
    StringDestruct(&member_name);
    return NULL;
  }
  ASTNode* left =
      NewIdentifierASTNode(this_symbol, syntax->lex->current_token_location);
  ASTNode* right =
      NewStructMemberASTNode(member, syntax->lex->current_token_location);
  if (member->symbol != NULL &&
      member->symbol->flags.name_independent_lookup_ambiguous) {
    right->flags |= kASTNameIndependentLookupAmbiguous;
  }
  StringDestruct(&member_name);
  right->flags |= kASTQualifiedName;
  StructMemberASTNode* member_node = (StructMemberASTNode*)right;
  member_node->owner_type = owner->type;
  TypeRecordIncRef(member_node->owner_type);
  return NewBinaryASTNode(AST_OP(arrow), NULL,
                          syntax->lex->current_token_location, left, right);
}

// Resolve an unqualified name that refers to a `static` data member of the
// enclosing class.  Non-static members are reached through the implicit `this`
// (see NewMemberAccessFromThis); a static member has no `this`, and inside a
// static member function there is no `this` at all, so look the name up in the
// class that owns the member function currently being parsed and reference the
// member's (global-linkage) symbol directly.
static ASTNode* NewStaticMemberReference(Syntax* syntax,
                                         FullyQualifiedIdentifier* name) {
  if (!CompilerIsCXX() || name->is_qualified) {
    return NULL;
  }
  Struct* owner = NULL;
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    owner = compiler->current_function->info.function.cxx_member_owner;
  }
  if (owner == NULL) {
    Symbol* this_symbol = FindThisSymbol(syntax);
    if (this_symbol != NULL && this_symbol->type != NULL &&
        TypeIsStructOrUnionPointer(this_symbol->type) &&
        this_symbol->type->next != NULL) {
      owner = this_symbol->type->next->info.struct_info;
    }
  }
  if (owner == NULL) {
    return NULL;
  }
  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(name));
  StructMember* member = FindStructMember(owner, &member_name);
  StringDestruct(&member_name);
  if (member == NULL || !member->is_static || member->is_member_function ||
      member->symbol == NULL) {
    return NULL;
  }
  return NewIdentifierASTNode(member->symbol,
                              syntax->lex->current_token_location);
}

// When an unqualified `name` is a reference to the enclosing class's own name
// (the injected-class-name) while that class's member body is being parsed,
// returns the class's tag symbol; otherwise NULL.  A class template is not yet
// marked as a template while its own body is parsed (that happens after the
// closing brace), so an explicit self-type template-id such as `Box<T>` used
// inside `Box`'s members cannot go through the ordinary class-template symbol
// path.  Routing it through the tag symbol -- which *does* become a template --
// lets the retained arguments be substituted during member-body cloning and the
// resulting specialization be instantiated like any other `A<...>` functional
// cast.
static Symbol* CurrentClassSelfTagSymbol(Syntax* syntax,
                                         FullyQualifiedIdentifier* name) {
  (void)syntax;
  if (!CompilerIsCXX() || name->is_qualified) {
    return NULL;
  }
  Struct* owner = NULL;
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    owner = compiler->current_function->info.function.cxx_member_owner;
  }
  if (owner == NULL || owner->tag_name == NULL || owner->tag_symbol == NULL) {
    return NULL;
  }
  // The caller only reaches this helper when the resolved symbol is the
  // not-yet-a-template injected-class-name, which happens only while a class
  // template's own body is being parsed.  `owner` is therefore always the
  // primary template, whose tag name is the bare class name; instantiated tags
  // (e.g. `Box<int>`) are produced by cloning and never re-parsed through here,
  // so an exact match against the bare tag name is what we want.
  const char* last = FullyQualifiedIdentifierLast(name);
  if (strcmp(last, owner->tag_name->value) == 0) {
    return owner->tag_symbol;
  }
  return NULL;
}

static ASTNode* ParseThisExpression(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);
  Symbol* this_symbol = FindThisSymbol(syntax);
  if (this_symbol == NULL) {
    SyntaxError(syntax, "'this' is only valid inside a C++ member function");
    TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                             kQualPlain);
    return (ASTNode*)NewIntConstantASTNode(0, type, location);
  }
  return NewIdentifierASTNode(this_symbol, location);
}

// Forward declarations.
static ASTNode* ParseCastExpression(Syntax* syntax, TokenClass followers);
static ASTNode* ParseUnaryExpression(Syntax* syntax, TokenClass followers);
static ASTNode* ParseCompoundLiteral(Syntax* syntax, TypeRecord* type);
static StructMember* FindCXXConstructorForType(TypeRecord* type);
static bool CXXNewConstructorSetHasInitializerList(StructMember* ctor);
static Vector* ParseCXXNewInitializerArguments(Syntax* syntax, Token open,
                                               TokenClass followers);

typedef struct {
  Syntax* syntax;
  bool found;
} CXXPackExpressionSearch;

static void FindCXXParameterPackExpression(ASTNode* node, void* data,
                                           int child_id, VisitorMode mode);

static bool CXXTemplateParameterIndexIsPack(Syntax* syntax, int index) {
  Vector* parameters = syntax != NULL ? syntax->current_template_parameters
                                      : NULL;
  for (size_t i = 0; parameters != NULL && i < parameters->length; i++) {
    TemplateParameter* parameter = parameters->value.p[i];
    if (parameter != NULL && parameter->index == index) {
      return parameter->is_parameter_pack;
    }
  }
  TypeRecord* function = compiler->current_function;
  if (function != NULL && TypeIsFunction(function)) {
    parameters = &function->info.function.template_parameters;
    for (size_t i = 0; i < parameters->length; i++) {
      TemplateParameter* parameter = parameters->value.p[i];
      if (parameter != NULL && parameter->index == index) {
        return parameter->is_parameter_pack;
      }
    }
  }
  return false;
}

static TemplateParameter* CXXTemplateParameterForIndex(Syntax* syntax,
                                                       int index) {
  Vector* parameters = syntax != NULL ? syntax->current_template_parameters
                                      : NULL;
  for (size_t i = 0; parameters != NULL && i < parameters->length; i++) {
    TemplateParameter* parameter = parameters->value.p[i];
    if (parameter != NULL && parameter->index == index) {
      return parameter;
    }
  }
  TypeRecord* function = compiler->current_function;
  if (function != NULL && TypeIsFunction(function)) {
    parameters = &function->info.function.template_parameters;
    for (size_t i = 0; i < parameters->length; i++) {
      TemplateParameter* parameter = parameters->value.p[i];
      if (parameter != NULL && parameter->index == index) {
        return parameter;
      }
    }
  }
  return NULL;
}

typedef struct {
  Syntax* syntax;
  bool concept_template_pack;
  bool other_pack;
} CXXFoldPackKinds;

static void ClassifyCXXFoldPackIndex(CXXFoldPackKinds* kinds, int index) {
  TemplateParameter* parameter =
      CXXTemplateParameterForIndex(kinds->syntax, index);
  if (parameter == NULL || !parameter->is_parameter_pack) {
    return;
  }
  if (parameter->kind == kTemplateParameterTemplate &&
      parameter->template_template_kind ==
          kTemplateTemplateParameterConcept) {
    kinds->concept_template_pack = true;
  } else {
    kinds->other_pack = true;
  }
}

static void ClassifyCXXFoldTemplateArgument(CXXFoldPackKinds* kinds,
                                            TemplateArgument* argument) {
  if (argument == NULL) {
    return;
  }
  ClassifyCXXFoldPackIndex(kinds, argument->template_parameter_index);
  if (argument->template_symbol != NULL &&
      argument->template_symbol->flags.is_parameter_pack) {
    ClassifyCXXFoldPackIndex(
        kinds, argument->template_symbol->template_parameter_index);
  }
  for (TypeRecord* type = argument->type; type != NULL; type = type->next) {
    ClassifyCXXFoldPackIndex(kinds, type->template_parameter_index);
  }
  for (size_t i = 0;
       argument->pack_arguments != NULL &&
       i < argument->pack_arguments->length;
       i++) {
    ClassifyCXXFoldTemplateArgument(
        kinds, argument->pack_arguments->value.p[i]);
  }
}

static void ClassifyCXXFoldPacks(ASTNode* node, void* data, int child_id,
                                 VisitorMode mode) {
  (void)child_id;
  if (node == NULL || mode != kVisitPreChildren) {
    return;
  }
  CXXFoldPackKinds* kinds = data;
  Vector* arguments = NULL;
  switch (ASTNodeGetShape(node)) {
    case kASTShapeIdentifier: {
      IdentifierASTNode* id = (IdentifierASTNode*)node;
      if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
        ClassifyCXXFoldPackIndex(kinds,
                                 id->symbol->template_parameter_index);
      }
      arguments = id->template_arguments;
      break;
    }
    case kASTShapeStructMember:
      arguments = ((StructMemberASTNode*)node)->template_arguments;
      break;
    case kASTShapeConstant:
      arguments = ((ConstantASTNode*)node)->template_arguments;
      break;
    default:
      break;
  }
  for (size_t i = 0; arguments != NULL && i < arguments->length; i++) {
    ClassifyCXXFoldTemplateArgument(kinds, arguments->value.p[i]);
  }
}

static bool CXXFoldMixesConceptAndOtherPacks(Syntax* syntax,
                                             ASTNode* pattern) {
  CXXFoldPackKinds kinds = {.syntax = syntax};
  ASTNodeVisit(pattern, ClassifyCXXFoldPacks, 0, &kinds);
  return kinds.concept_template_pack && kinds.other_pack;
}

static bool CXXTemplateArgumentReferencesParameterPack(
    CXXPackExpressionSearch* search, TemplateArgument* argument) {
  if (argument == NULL) {
    return false;
  }
  if (argument->references_parameter_pack) {
    return true;
  }
  if (argument->template_parameter_index >= 0 &&
      CXXTemplateParameterIndexIsPack(
          search->syntax, argument->template_parameter_index)) {
    return true;
  }
  for (size_t i = 0;
       argument->pack_arguments != NULL &&
       i < argument->pack_arguments->length; i++) {
    if (CXXTemplateArgumentReferencesParameterPack(
            search, argument->pack_arguments->value.p[i])) {
      return true;
    }
  }
  if (argument->dependent_expr != NULL) {
    ASTNodeVisit(argument->dependent_expr, FindCXXParameterPackExpression, 0,
                 search);
  }
  return search->found;
}

static void FindCXXParameterPackExpression(ASTNode* node, void* data,
                                           int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  if ((node->flags & kASTReferencesParameterPack) != 0) {
    ((CXXPackExpressionSearch*)data)->found = true;
    return;
  }
  Vector* template_arguments = NULL;
  ASTNodeShape shape = ASTNodeGetShape(node);
  if (shape == kASTShapeIdentifier) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
      ((CXXPackExpressionSearch*)data)->found = true;
      return;
    }
    template_arguments = id->template_arguments;
  } else if (shape == kASTShapeStructMember) {
    template_arguments =
        ((StructMemberASTNode*)node)->template_arguments;
  } else if (shape == kASTShapeConstant) {
    template_arguments = ((ConstantASTNode*)node)->template_arguments;
  } else {
    return;
  }
  CXXPackExpressionSearch* search = data;
  for (size_t i = 0;
       template_arguments != NULL && i < template_arguments->length; i++) {
    if (CXXTemplateArgumentReferencesParameterPack(
            search, template_arguments->value.p[i])) {
      search->found = true;
      return;
    }
  }
}

static bool CXXExpressionContainsParameterPack(Syntax* syntax, ASTNode* node) {
  CXXPackExpressionSearch search = {.syntax = syntax, .found = false};
  ASTNodeVisit(node, FindCXXParameterPackExpression, 0, &search);
  return search.found;
}

static void MarkCXXPackExpansionIfPresent(Syntax* syntax, ASTNode* actual) {
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(ellipsis))) {
    return;
  }
  if (!CXXExpressionContainsParameterPack(syntax, actual)) {
    SyntaxError(syntax, "pack expansion requires a function parameter pack");
  }
  actual->flags |= kASTPackExpansion;
}

/* Map exactly the binary operators permitted as fold-operators by
 * [expr.prim.fold]. Keep this as the single source of truth for both parsing
 * and the top-level-ellipsis lookahead. In particular, <=> is not a permitted
 * fold-operator. */
static bool FoldOperatorOpcode(Token token, ASTOpcode* op) {
  ASTOpcode result = AST_OP(bad);
  switch (token) {
    case TOK(star): result = AST_OP(mult); break;
    case TOK(slash): result = AST_OP(div); break;
    case TOK(percent): result = AST_OP(mod); break;
    case TOK(plus): result = AST_OP(plus); break;
    case TOK(minus): result = AST_OP(minus); break;
    case TOK(lessless): result = AST_OP(lshift); break;
    case TOK(greatergreater): result = AST_OP(rshift); break;
    case TOK(amp): result = AST_OP(and); break;
    case TOK(caret): result = AST_OP(exor); break;
    case TOK(bar): result = AST_OP(bitor); break;
    case TOK(ampamp): result = AST_OP(logand); break;
    case TOK(barbar): result = AST_OP(logor); break;
    case TOK(equalequal): result = AST_OP(equal); break;
    case TOK(bangeq): result = AST_OP(noteq); break;
    case TOK(less): result = AST_OP(less); break;
    case TOK(greater): result = AST_OP(greater); break;
    case TOK(lesseq): result = AST_OP(lesseq); break;
    case TOK(greatereq): result = AST_OP(greatereq); break;
    case TOK(equal): result = AST_OP(assign); break;
    case TOK(pluseq): result = AST_OP(pluseq); break;
    case TOK(minuseq): result = AST_OP(minuseq); break;
    case TOK(stareq): result = AST_OP(multeq); break;
    case TOK(slasheq): result = AST_OP(diveq); break;
    case TOK(percenteq): result = AST_OP(percenteq); break;
    case TOK(lesslesseq): result = AST_OP(lshifteq); break;
    case TOK(greatergreatereq): result = AST_OP(rshifteq); break;
    case TOK(ampeq): result = AST_OP(andeq); break;
    case TOK(bareq): result = AST_OP(oreq); break;
    case TOK(careteq): result = AST_OP(exoreq); break;
    case TOK(comma): result = AST_OP(comma); break;
    case TOK(dotstar): result = AST_OP(dotstar); break;
    case TOK(arrowstar): result = AST_OP(arrowstar); break;
    default: return false;
  }
  if (op != NULL) {
    *op = result;
  }
  return true;
}

static bool ParseFoldOperator(Syntax* syntax, ASTOpcode* op) {
  if (!FoldOperatorOpcode(syntax->lex->current_token, op)) {
    return false;
  }
  LexNextToken(syntax->lex);
  return true;
}

/* davecc parses .* and ->* in its postfix-expression loop. Do not let that
 * loop consume the first operator of `(pack .* ... [.* init])` or
 * `(pack ->* ... [->* init])`; it belongs to the surrounding fold grammar. */
static bool MemberPointerOperatorPrecedesFoldEllipsis(Syntax* syntax) {
  if (!LexLookingAt(syntax->lex, TOK(dotstar)) &&
      !LexLookingAt(syntax->lex, TOK(arrowstar))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result = LexLookingAt(syntax->lex, TOK(ellipsis));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

static bool FoldExpressionContainsPack(Syntax* syntax, ASTNode* node) {
  // A fold pattern can name its pack through a template argument rather than
  // through an ordinary identifier child, as in
  // `(is_convertible_v<Ts, U> && ...)`.  Use the complete expression-pack
  // search so those template arguments participate in fold parsing too.
  return CXXExpressionContainsParameterPack(syntax, node);
}

static ASTNode* ParseFoldPackExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* expr = ParseCastExpression(syntax, followers);
  if (!FoldExpressionContainsPack(syntax, expr)) {
    ASTNodeDelete(expr);
    return NULL;
  }
  return expr;
}

// A parenthesized expression may contain a template argument pack expansion,
// as in `Trait<T, Args&&...>::value`.  That ellipsis is not a fold-expression
// ellipsis.  At a `<`, look ahead for a balanced template close before the
// surrounding `)`; a genuine `<` fold operator has no matching `>`.
static bool FoldLessStartsTemplateArgument(Syntax* syntax) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  int angle_depth = 0;
  int paren_depth = 0;
  int square_depth = 0;
  int brace_depth = 0;
  bool matched = false;
  while (!LexEof(syntax->lex)) {
    Token token = syntax->lex->current_token;
    if (token == TOK(rparen) && paren_depth == 0 &&
        square_depth == 0 && brace_depth == 0) {
      break;
    }
    if (token == TOK(lparen)) {
      paren_depth++;
    } else if (token == TOK(rparen)) {
      paren_depth--;
    } else if (token == TOK(lsquare)) {
      square_depth++;
    } else if (token == TOK(rsquare)) {
      square_depth--;
    } else if (token == TOK(lbrace)) {
      brace_depth++;
    } else if (token == TOK(rbrace)) {
      brace_depth--;
    } else if (paren_depth == 0 && square_depth == 0 && brace_depth == 0) {
      if (token == TOK(less)) {
        angle_depth++;
      } else {
        angle_depth -= LexClosingAngleCount(token);
      }
    }
    LexNextToken(syntax->lex);
    if (angle_depth <= 0) {
      matched = true;
      break;
    }
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return matched;
}

static bool TokenIsFoldOperator(Token token) {
  return FoldOperatorOpcode(token, NULL);
}

static bool FoldExpressionHasTopLevelEllipsis(Syntax* syntax) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool found = false;
  int paren_depth = 0;
  int square_depth = 0;
  int brace_depth = 0;
  int angle_depth = 0;
  Token previous_top_level = TOK(bad);
  bool at_start = true;

  while (!LexEof(syntax->lex)) {
    Token token = syntax->lex->current_token;
    if (token == TOK(rparen) && paren_depth == 0 &&
        square_depth == 0 && brace_depth == 0) {
      break;
    }
    if (token == TOK(ellipsis) && paren_depth == 0 &&
        square_depth == 0 && brace_depth == 0 && angle_depth == 0) {
      if (at_start || TokenIsFoldOperator(previous_top_level)) {
        found = true;
        break;
      }
    }
    if (token == TOK(lparen)) {
      paren_depth++;
    } else if (token == TOK(rparen)) {
      paren_depth--;
    } else if (token == TOK(lsquare)) {
      square_depth++;
    } else if (token == TOK(rsquare)) {
      square_depth--;
    } else if (token == TOK(lbrace)) {
      brace_depth++;
    } else if (token == TOK(rbrace)) {
      brace_depth--;
    } else if (token == TOK(less) && angle_depth == 0 &&
               paren_depth == 0 && square_depth == 0 && brace_depth == 0 &&
        FoldLessStartsTemplateArgument(syntax)) {
      angle_depth++;
    } else if (token == TOK(less) && angle_depth > 0 &&
               paren_depth == 0 && square_depth == 0 && brace_depth == 0) {
      angle_depth++;
    } else if (LexClosingAngleCount(token) != 0 && angle_depth > 0 &&
               paren_depth == 0 && square_depth == 0 && brace_depth == 0) {
      angle_depth -= LexClosingAngleCount(token);
      if (angle_depth < 0) {
        angle_depth = 0;
      }
    }
    if (paren_depth == 0 && square_depth == 0 && brace_depth == 0 &&
        angle_depth == 0) {
      previous_top_level = token;
      at_start = false;
    }
    LexNextToken(syntax->lex);
  }

  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return found;
}

static ASTNode* NewInvalidFoldExpression(Syntax* syntax,
                                         LexCheckpoint* checkpoint,
                                         const char* message,
                                         SourceLocation location,
                                         TokenClass followers) {
  LexCheckpointRestore(syntax->lex, checkpoint);
  SyntaxError(syntax, "%s", message);
  SyntaxRecover(syntax, TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  return NewIntConstantASTNode(0, type, location);
}

static ASTNode* TryParseCXXFoldExpression(Syntax* syntax,
                                          TokenClass followers) {
  if (!CompilerIsCXX()) {
    return NULL;
  }

  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  SourceLocation location = syntax->lex->current_token_location;
  ASTOpcode op = AST_OP(bad);
  ASTOpcode second_op = AST_OP(bad);
  ASTNode* pack = NULL;
  ASTNode* seed = NULL;
  bool pack_on_left = false;

  if (LexMatch(syntax->lex, TOK(ellipsis))) {
    if (!ParseFoldOperator(syntax, &op)) {
      ASTNode* invalid =
          NewInvalidFoldExpression(syntax, &checkpoint,
                                   "fold expression requires an operator",
                                   location, followers);
      LexCheckpointDestruct(&checkpoint);
      return invalid;
    }
    if ((pack = ParseFoldPackExpression(syntax, followers | TC(closebra))) == NULL) {
      ASTNode* invalid =
          NewInvalidFoldExpression(syntax, &checkpoint,
                                   "fold expression requires a parameter pack",
                                   location, followers);
      LexCheckpointDestruct(&checkpoint);
      return invalid;
    }
    if (!LexMatch(syntax->lex, TOK(rparen))) {
      ASTNode* invalid =
          NewInvalidFoldExpression(syntax, &checkpoint,
                                   "fold expression syntax error",
                                   location, followers);
      LexCheckpointDestruct(&checkpoint);
      return invalid;
    }
  } else {
    if (!FoldExpressionHasTopLevelEllipsis(syntax)) {
      LexCheckpointRestore(syntax->lex, &checkpoint);
      LexCheckpointDestruct(&checkpoint);
      return NULL;
    }

    seed = ParseCastExpression(syntax, followers | TC(closebra));
    bool seed_contains_pack = FoldExpressionContainsPack(syntax, seed);
    if (!seed_contains_pack && seed != NULL &&
        ParseFoldOperator(syntax, &op) &&
        LexMatch(syntax->lex, TOK(ellipsis))) {
      // A variable-template specialization can carry the pack only in its
      // explicit template arguments, while the parsed expression has already
      // folded to a constant node.  In `(Trait<Ts, U> && ...)`, the immediate
      // ')' unambiguously identifies a unary right fold even if that constant
      // node no longer exposes the pack to the AST visitor.
      if (LexMatch(syntax->lex, TOK(rparen))) {
        pack = seed;
        seed = NULL;
        pack_on_left = true;
      } else if (!ParseFoldOperator(syntax, &second_op)) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression requires a second operator",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      } else if (second_op != op) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression operators must match",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      } else if ((pack = ParseFoldPackExpression(
                      syntax, followers | TC(closebra))) == NULL) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression requires a parameter pack",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      } else if (!LexMatch(syntax->lex, TOK(rparen))) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression syntax error",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      } else {
        pack_on_left = false;
      }
    } else {
      if (!seed_contains_pack) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression requires a parameter pack",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      }
      pack = seed;
      seed = NULL;
      if (!ParseFoldOperator(syntax, &op)) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression requires an operator",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      }
      if (!LexMatch(syntax->lex, TOK(ellipsis))) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression syntax error",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      }
      if (LexMatch(syntax->lex, TOK(rparen))) {
        pack_on_left = true;
        seed = NULL;
      } else {
        if (!ParseFoldOperator(syntax, &second_op)) {
          ASTNode* invalid =
              NewInvalidFoldExpression(syntax, &checkpoint,
                                       "fold expression requires a second operator",
                                       location, followers);
          LexCheckpointDestruct(&checkpoint);
          return invalid;
        }
        if (second_op != op) {
          ASTNode* invalid =
              NewInvalidFoldExpression(syntax, &checkpoint,
                                       "fold expression operators must match",
                                       location, followers);
          LexCheckpointDestruct(&checkpoint);
          return invalid;
        }
        seed = ParseCastExpression(syntax, followers | TC(closebra));
        if (seed == NULL || FoldExpressionContainsPack(syntax, seed)) {
          ASTNode* invalid =
              NewInvalidFoldExpression(syntax, &checkpoint,
                                       "fold expression requires a non-pack initializer",
                                       location, followers);
          LexCheckpointDestruct(&checkpoint);
          return invalid;
        }
        if (!LexMatch(syntax->lex, TOK(rparen))) {
          ASTNode* invalid =
              NewInvalidFoldExpression(syntax, &checkpoint,
                                       "fold expression syntax error",
                                       location, followers);
          LexCheckpointDestruct(&checkpoint);
          return invalid;
        }
        pack_on_left = true;
      }
    }
  }

  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      CXXFoldMixesConceptAndOtherPacks(syntax, pack)) {
    SyntaxError(syntax,
                "fold expression cannot expand a concept template parameter "
                "pack together with another parameter pack");
  }
  ASTNode* fold =
      pack_on_left ? NewBinaryASTNode(op, NULL, location, pack, seed)
                   : NewBinaryASTNode(op, NULL, location, seed, pack);
  fold->flags |= kASTFoldExpression;
  if (pack_on_left) {
    fold->flags |= kASTFoldPackOnLeft;
  }
  LexCheckpointDestruct(&checkpoint);
  (void)followers;
  return fold;
}

static bool SymbolHasFunctionTemplateOverload(Symbol* symbol) {
  for (Symbol* candidate = symbol; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->flags.is_template && candidate->type != NULL &&
        TypeIsFunction(candidate->type)) {
      return true;
    }
  }
  return false;
}

static bool ExpressionIdentifierNeedsTemplateIdParser(Syntax* syntax) {
  if (!CompilerIsCXX()) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool needs_template_ids = false;
  if (LexMatch(syntax->lex, TOK(coloncolon)) &&
      !LexLookingAt(syntax->lex, TOK(identifier))) {
    goto done;
  }
  while (LexLookingAt(syntax->lex, TOK(identifier))) {
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(less))) {
      int depth = 0;
      int paren_depth = 0;
      int square_depth = 0;
      int brace_depth = 0;
      do {
        // A `;` or brace can never appear at the top level of a
        // template-argument list, so this `<` is a less-than operator rather
        // than a template-id.  Stopping here also keeps the lookahead from
        // running to the end of an #include'd file, which on EOF frees the
        // current source out from under the checkpoint we restore below.
        if (LexLookingAt(syntax->lex, TOK(semicolon)) ||
            LexLookingAt(syntax->lex, TOK(lbrace)) ||
            LexLookingAt(syntax->lex, TOK(rbrace))) {
          break;
        }
        Token token = syntax->lex->current_token;
        if (token == TOK(lparen)) {
          paren_depth++;
        } else if (token == TOK(rparen)) {
          paren_depth--;
        } else if (token == TOK(lsquare)) {
          square_depth++;
        } else if (token == TOK(rsquare)) {
          square_depth--;
        } else if (token == TOK(lbrace)) {
          brace_depth++;
        } else if (token == TOK(rbrace)) {
          brace_depth--;
        } else if (paren_depth == 0 && square_depth == 0 &&
                   brace_depth == 0) {
          if (token == TOK(less)) {
            depth++;
          } else {
            depth -= LexClosingAngleCount(token);
          }
        }
        LexNextToken(syntax->lex);
      } while (!LexEof(syntax->lex) && depth > 0);
      if (depth == 0 && LexLookingAt(syntax->lex, TOK(coloncolon))) {
        needs_template_ids = true;
        goto done;
      }
    }
    if (!LexMatch(syntax->lex, TOK(coloncolon))) {
      break;
    }
  }

done:
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return needs_template_ids;
}

static bool TemplateArgumentListIsDependent(Vector* args);

// A qualified name like `T::member` or `Alias<T>::member::value` whose leading
// nested-name-specifier is dependent cannot be resolved until the template is
// instantiated.  Builds a placeholder identifier carrying the dependent scope
// and the trailing member name (in dependent_member_name), flagged so the
// template-body cloner can resolve it against the concrete type argument.
// Returns NULL when `name` is not such a dependent qualified value name.
static ASTNode* BuildDependentQualifiedValueName(Syntax* syntax,
                                                  FullyQualifiedIdentifier* name,
                                                  SourceLocation location) {
  if (!CompilerIsCXX() || !name->is_qualified || name->absolute ||
      name->components.length < 2) {
    return NULL;
  }
  // No component may carry template arguments (e.g. `T::tmpl<...>`); that needs
  // the richer dependent-template-id handling, which this does not cover.
  for (size_t i = 0; i < name->template_arguments.length; i++) {
    if (name->template_arguments.value.p[i] != NULL) {
      return NULL;
    }
  }
  String* scope = name->components.value.p[0];
  Symbol* scope_symbol = SyntaxFindSymbol(syntax, scope);
  // The scope must name a dependent type: either a template type parameter (or
  // alias to one), or an alias to a dependent class-template specialization such
  // as `using traits_type = allocator_traits<Alloc>;`.
  if (scope_symbol == NULL || scope_symbol->type == NULL) {
    return NULL;
  }
  if (!scope_symbol->flags.is_template_type_parameter &&
      !StorageIs(scope_symbol->storage, STO(typedef))) {
    return NULL;
  }
  bool dependent_scope = scope_symbol->type->template_parameter_index >= 0 ||
                         TypeContainsTemplateParameter(scope_symbol->type) ||
                         (scope_symbol->type->dependent_member_name == NULL &&
                          scope_symbol->type->template_origin != NULL &&
                          TemplateArgumentListIsDependent(
                              scope_symbol->type->template_arguments));
  if (!dependent_scope) {
    return NULL;
  }
  String* member =
      name->components.value.p[name->components.length - 1];
  TypeRecord* dependent_type = TypeRecordCopy(scope_symbol->type);
  if (dependent_type->dependent_member_name == NULL) {
    dependent_type->dependent_member_name = NewString("");
  } else {
    // The scope alias is itself a dependent member access (e.g. `q` was
    // `typename W::period`).  Extend the existing member path rather than
    // discarding it, so `q::num` becomes the full `W::period::num` path instead
    // of collapsing to `W::num` (which would drop the intermediate member and
    // leave the name unresolvable).
    StringAppend(dependent_type->dependent_member_name, "::");
  }
  for (size_t i = 1; i < name->components.length; i++) {
    if (i != 1) {
      StringAppend(dependent_type->dependent_member_name, "::");
    }
    StringAppendString(dependent_type->dependent_member_name,
                       name->components.value.p[i]);
  }
  Symbol* placeholder = NewSymbol(member->value, dependent_type, STO(implicit));
  placeholder->flags.invented = true;
  ASTNode* node = NewIdentifierASTNode(placeholder, location);
  node->flags |= kASTQualifiedName | kASTDependentQualifiedName;
  return node;
}

// True if any template argument in `args` mentions a template parameter, making
// the argument list (and thus a class template specialization using it) dependent.
static bool TemplateArgumentListIsDependent(Vector* args) {
  if (args == NULL) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* a = args->value.p[i];
    if (a == NULL) {
      continue;
    }
    if (a->kind == kTemplateParameterType &&
        TypeContainsTemplateParameter(a->type)) {
      return true;
    }
    if (a->kind == kTemplateParameterNonType && a->template_parameter_index >= 0) {
      return true;
    }
    if (a->kind == kTemplateParameterTemplate &&
        a->template_parameter_index >= 0) {
      return true;
    }
  }
  return false;
}

// A qualified value name like `Trait<Deps...>::member` whose nested-name-specifier
// is a *dependent* class template specialization (its template arguments mention a
// template parameter) is a value-dependent name: the specialization chosen (primary
// vs. an explicit/partial specialization) is not known until the enclosing template
// is instantiated, so its `::member` must not be resolved (and folded) now.  Build a
// placeholder identifier carrying the dependent scope (template_origin +
// template_arguments) and the trailing member name (dependent_member_name), flagged
// so the template-body cloner resolves it against the substituted arguments.  Returns
// NULL when `name` is not such a dependent template-scope value name.
static ASTNode* BuildDependentTemplateScopeValueName(
    Syntax* syntax, FullyQualifiedIdentifier* name, SourceLocation location) {
  if (!CompilerIsCXX() || !name->is_qualified ||
      syntax->current_template_parameter_count <= 0 ||
      name->components.length < 2 ||
      name->template_arguments.length != name->components.length) {
    return NULL;
  }
  size_t member_index = name->components.length - 1;
  size_t base_index = name->components.length - 2;
  // The member component itself must not carry template arguments (`x::f<...>`
  // needs the richer dependent-template-id handling, which this does not cover).
  if (name->template_arguments.value.p[member_index] != NULL) {
    return NULL;
  }
  Vector* scope_args = name->template_arguments.value.p[base_index];
  if (!TemplateArgumentListIsDependent(scope_args)) {
    return NULL;
  }
  // Resolve the class template that names the scope (components[0..base_index]).
  FullyQualifiedIdentifier prefix;
  FullyQualifiedIdentifierInit(&prefix);
  prefix.absolute = name->absolute;
  prefix.is_qualified = prefix.absolute || base_index > 0;
  for (size_t i = 0; i <= base_index; i++) {
    String* component = name->components.value.p[i];
    if (prefix.spelling.length != 0 || prefix.absolute) {
      StringAppend(&prefix.spelling, "::");
    }
    StringAppendString(&prefix.spelling, component);
    VectorAppend(&prefix.components, NewString(component->value));
    VectorAppend(&prefix.template_arguments,
                 TemplateArgumentVectorCopy(name->template_arguments.value.p[i]));
  }
  Symbol* base = SyntaxFindQualifiedSymbol(syntax, &prefix);
  FullyQualifiedIdentifierDestruct(&prefix);
  if (base == NULL || !base->flags.is_template || base->type == NULL ||
      !TypeIsStructOrUnion(base->type)) {
    return NULL;
  }
  String* member = name->components.value.p[member_index];
  TypeRecord* dependent_type = NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
  dependent_type->template_origin = base;
  dependent_type->template_arguments = TemplateArgumentVectorCopy(scope_args);
  dependent_type->dependent_member_name = NewString(member->value);
  Symbol* placeholder = NewSymbol(member->value, dependent_type, STO(implicit));
  placeholder->flags.invented = true;
  ASTNode* node = NewIdentifierASTNode(placeholder, location);
  node->flags |= kASTQualifiedName | kASTDependentQualifiedName;
  return node;
}

static ASTNode* ParseIdentifier(Syntax* syntax,
                                            TokenClass followers) {
  Lex* lex = syntax->lex;

  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  bool parsed = ExpressionIdentifierNeedsTemplateIdParser(syntax)
      ? SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                          followers)
      : SyntaxParseFullyQualifiedIdentifier(syntax, &name);
  if (!parsed) {
    SyntaxError(syntax, "Expected identifier");
    FullyQualifiedIdentifierDestruct(&name);
    return (ASTNode*)NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
        syntax->lex->current_token_location);
  }
  
  // In preprocesor mode we have no symbols, everything is a macro name.
  if (lex->preprocessor_mode) {
    String macro_name;
    StringInit(&macro_name, FullyQualifiedIdentifierLast(&name));
    if (PreprocessorFindMacro(lex->preprocessor, &macro_name) == NULL) {
      PreprocessorWarning(lex->preprocessor, "undef",
                          "'%s' is not defined, evaluates to 0",
                          macro_name.value);
    }
    ASTNode* result = NewMacroNameASTNode(&macro_name, lex->current_token_location);
    StringDestruct(&macro_name);
    FullyQualifiedIdentifierDestruct(&name);
    return result;
  }
  
  // A value name qualified by a dependent class-template specialization
  // (`Trait<T>::member`) must stay dependent: resolving it now would silently
  // select the primary template and fold the wrong value.  Detect and defer it
  // before the ordinary lookup, which would otherwise instantiate the primary.
  if (name.is_qualified) {
    ASTNode* dependent_scope = BuildDependentTemplateScopeValueName(
        syntax, &name, lex->current_token_location);
    if (dependent_scope != NULL) {
      FullyQualifiedIdentifierDestruct(&name);
      return dependent_scope;
    }
    dependent_scope = BuildDependentQualifiedValueName(
        syntax, &name, lex->current_token_location);
    if (dependent_scope != NULL) {
      FullyQualifiedIdentifierDestruct(&name);
      return dependent_scope;
    }
  }

  // Find the symbol by searching all symbol tables.  It must exist.
  Symbol* symbol = SyntaxFindQualifiedSymbol(syntax, &name);

  // An injected class name used in a functional construction expression
  // denotes the class type, even when ordinary member lookup found the
  // same-named constructor overload first.  Redirect source-level `T(...)`
  // through the enclosing class tag before constructing the identifier AST.
  if (symbol != NULL && symbol->type != NULL &&
      TypeIsFunction(symbol->type) &&
      symbol->type->info.function.is_constructor &&
      symbol->type->info.function.cxx_member_owner != NULL &&
      LexLookingAt(lex, TOK(lparen))) {
    Symbol* self_tag = CurrentClassSelfTagSymbol(syntax, &name);
    if (self_tag != NULL && self_tag->type != NULL &&
        TypeIsStructOrUnion(self_tag->type) &&
        self_tag->type->info.struct_info ==
            symbol->type->info.function.cxx_member_owner) {
      symbol = self_tag;
    }
  }

  // Name hiding ([basic.lookup.unqual], [class.member.lookup]): inside a member
  // function, an unqualified name that names a non-static member of the class
  // (or one of its bases) hides any entity of the same name declared in an
  // enclosing namespace.  Ordinary lookup above searches namespace scopes too,
  // so for a class defined in `namespace std`, an unqualified `end()` call can
  // wrongly bind to the free function template `std::end` instead of the
  // class's own `end` member (e.g. `map::rbegin`'s `reverse_iterator(end())`).
  // A block-scope declaration (local variable or parameter) is nearer than the
  // class and legitimately hides the member, so only redirect when the found
  // entity is at namespace/global scope.  Names carrying explicit template
  // arguments are left alone: the arrow member-access form built here does not
  // retain them.
  Vector* hiding_last_args =
      name.template_arguments.length > 0
          ? (Vector*)name.template_arguments.value
                .p[name.template_arguments.length - 1]
          : NULL;
  bool hiding_has_explicit_template_args =
      hiding_last_args != NULL && hiding_last_args->length > 0;
  if (CompilerIsCXX() && !name.is_qualified && symbol != NULL &&
      !symbol->flags.is_block_scope && !hiding_has_explicit_template_args) {
    Symbol* this_symbol = FindThisSymbol(syntax);
    if (this_symbol != NULL && this_symbol->type != NULL &&
        TypeIsStructOrUnionPointer(this_symbol->type) &&
        this_symbol->type->next != NULL &&
        this_symbol->type->next->info.struct_info != NULL) {
      String member_name;
      StringInit(&member_name, FullyQualifiedIdentifierLast(&name));
      StructMember* member = FindStructMember(
          this_symbol->type->next->info.struct_info, &member_name);
      StringDestruct(&member_name);
      bool member_is_constructor =
          member != NULL && member->symbol != NULL &&
          member->symbol->type != NULL &&
          TypeIsFunction(member->symbol->type) &&
          member->symbol->type->info.function.is_constructor;
      if (member != NULL && !member->is_static && !member_is_constructor) {
        ASTNode* member_access = NewMemberAccessFromThis(
            syntax, &name, /*allow_unresolved_member=*/false);
        if (member_access != NULL) {
          FullyQualifiedIdentifierDestruct(&name);
          return member_access;
        }
      }
    }
  }
  if (symbol == NULL) {
    if (name.is_qualified) {
      ASTNode* member_access = NewQualifiedBaseMemberAccessFromThis(syntax, &name);
      if (member_access != NULL) {
        FullyQualifiedIdentifierDestruct(&name);
        return member_access;
      }
      ASTNode* dependent = BuildDependentQualifiedValueName(
          syntax, &name, lex->current_token_location);
      if (dependent != NULL) {
        FullyQualifiedIdentifierDestruct(&name);
        return dependent;
      }
      SyntaxError(syntax, "No such symbol \"%s\"", name.spelling.value);
      TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), type, STO(implicit));
      symbol->flags.invented = true;
    } else if (lex->assembler_mode) {
      // In assembler mode we have symbols but we pre-declare them
      // if they don't exist.  They are declared as variables with
      // type unsigned long.
      TypeRecord* type = NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain);
      symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), type, STO(implicit));
      symbol->flags.is_forward_declared = true;
      // Register it so it is found on later references (and owned/freed by a
      // symbol table) rather than leaked.
      SyntaxAddSymbol(syntax, symbol);
    } else {
      bool builtin_call =
          LexLookingAt(lex, TOK(lparen)) &&
          IsBuiltinCallName(FullyQualifiedIdentifierLast(&name));
      if (!builtin_call) {
        ASTNode* member_access =
            NewMemberAccessFromThis(syntax, &name,
                                    /*allow_unresolved_member=*/true);
        if (member_access != NULL) {
          FullyQualifiedIdentifierDestruct(&name);
          return member_access;
        }
        ASTNode* static_member = NewStaticMemberReference(syntax, &name);
        if (static_member != NULL) {
          FullyQualifiedIdentifierDestruct(&name);
          return static_member;
        }
      }
      if (LexLookingAt(lex, TOK(lparen))) {
        if (!builtin_call) {
          ASTNode* deferred_member_access =
              NewMemberAccessFromThis(syntax, &name,
                                      /*allow_unresolved_member=*/true);
          if (deferred_member_access != NULL) {
            FullyQualifiedIdentifierDestruct(&name);
            return deferred_member_access;
          }
        }
        if (!CompilerIsCXX() &&
            GetIntrinsic(FullyQualifiedIdentifierLast(&name)) == NULL) {
          if (CompilerCAtLeast(kLanguageStandardC23)) {
            SyntaxError(syntax, "Calling undeclared function %s",
                        FullyQualifiedIdentifierLast(&name));
          } else {
            SyntaxWarning(syntax, "implicit-function-declaration",
                          "Calling undeclared function %s",
                          FullyQualifiedIdentifierLast(&name));
          }
        }

        // Declare the function so we don't get more warnings for the same
        // function.
        TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                                 kQualPlain);
        TypeRecord* func_type = NewFunctionTypeRecord();
        func_type->info.function.unknown_args =
            !CompilerCAtLeast(kLanguageStandardC23);
        TypeRecordChain(func_type, type);
        symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), func_type,
                           STO(implicit));
        symbol->flags.is_forward_declared = true;
        // Declare it so repeated calls find this symbol (no duplicate warnings)
        // and so it is owned/freed by a symbol table rather than leaked.
        SyntaxAddSymbol(syntax, symbol);
      } else {
        SyntaxError(syntax, "No such symbol \"%s\"", name.spelling.value);
        TypeRecord* type =
            NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
        symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), type,
                           STO(implicit));
        symbol->flags.invented = true;
        SyntaxAddSymbol(syntax, symbol);
      }
    }
  }
  Vector* template_arguments = NULL;
  if (symbol != NULL && name.template_arguments.length > 0 &&
      (TypeIsFunction(symbol->type) ||
       SymbolHasFunctionTemplateOverload(symbol))) {
    Vector* parsed_args =
        name.template_arguments.value.p[name.template_arguments.length - 1];
    template_arguments = TemplateArgumentVectorCopy(parsed_args);
  }
  // An explicit self-type template-id (`Box<T>`) inside the class's own member
  // body: the class is not marked as a template until its body is fully parsed,
  // so the resolved `symbol` is the injected-class-name (not a template).  Route
  // it through the class tag symbol and retain the arguments, mirroring an
  // ordinary `A<...>` functional cast.  The guard requires `symbol` to actually
  // denote the current class's own type (the injected-class-name), so a
  // shadowing value named like the class -- e.g. a local `Foo` followed by a
  // `<` less-than operator -- is left alone.
  if (template_arguments == NULL && symbol != NULL &&
      !symbol->flags.is_template && StorageIs(symbol->storage, STO(typedef)) &&
      symbol->type != NULL && TypeIsStructOrUnion(symbol->type) &&
      LexLookingAt(lex, TOK(less))) {
    Symbol* self_tag = CurrentClassSelfTagSymbol(syntax, &name);
    if (self_tag != NULL && self_tag->type != NULL &&
        TypeIsStructOrUnion(self_tag->type) &&
        self_tag->type->info.struct_info == symbol->type->info.struct_info) {
      Vector* args = SyntaxParseTemplateArgumentList(syntax, followers);
      if (args != NULL) {
        template_arguments = args;
        symbol = self_tag;
      }
    }
  }
  if (template_arguments == NULL && symbol != NULL &&
      (symbol->flags.is_template || SymbolHasFunctionTemplateOverload(symbol)) &&
      LexLookingAt(lex, TOK(less))) {
    Vector* args = SyntaxParseTemplateArgumentList(syntax, followers);
    if (args != NULL) {
      if (TypeIsFunction(symbol->type) ||
          SymbolHasFunctionTemplateOverload(symbol)) {
        template_arguments = args;
        args = NULL;
      } else if (symbol->flags.is_concept) {
        template_arguments = args;
        args = NULL;
      } else if (symbol->flags.is_template && symbol->type != NULL &&
                 TypeIsStructOrUnion(symbol->type) &&
                 symbol->type->template_arguments == NULL &&
                 symbol->type->info.struct_info != NULL &&
                 symbol->type->info.struct_info->is_template) {
        // A class template-id used as an expression, e.g. the functional cast
        // `A<int>(...)`.  Retain the explicit arguments on the identifier node;
        // functional-construction analysis instantiates the specialization from
        // them (and the template-body cloner substitutes them first when the
        // construction appears inside another template).  Alias templates are
        // excluded here: their type carries the aliased `<...>` arguments, so
        // they still route through the placeholder/CTAD path.
        template_arguments = args;
        args = NULL;
      } else if (symbol->flags.is_template &&
                 symbol->variable_template != NULL) {
        // A variable template-id used as a value, e.g. `variant_size_v<T>`.
        // Retain the explicit arguments; the identifier is instantiated and
        // constant-folded during semantic analysis.
        template_arguments = args;
        args = NULL;
      } else if (symbol->flags.is_template &&
                 StorageIs(symbol->storage, STO(typedef))) {
        // Alias template-id used as an expression, e.g.
        // `make_index_sequence<N>()`.  Preserve its written arguments so a
        // dependent N can be substituted before functional construction
        // instantiates the alias.
        template_arguments = args;
        args = NULL;
      }
      if (args != NULL) {
        VectorDestructWithContents(args,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
      }
    }
  }
  bool is_qualified_name = name.is_qualified;
  Struct* functional_constructor_owner =
      symbol != NULL && symbol->type != NULL &&
              TypeIsFunction(symbol->type) &&
              symbol->type->info.function.is_constructor
          ? symbol->type->info.function.cxx_member_owner
          : NULL;
  bool is_functional_class_construction =
      CompilerIsCXX() && symbol != NULL && symbol->type != NULL &&
      ((TypeIsStructOrUnion(symbol->type) &&
        (StorageIs(symbol->storage, STO(typedef)) ||
         (symbol->flags.is_template &&
          symbol->variable_template == NULL))) ||
       functional_constructor_owner != NULL) &&
      LexLookingAt(lex, TOK(lparen));
  FullyQualifiedIdentifierDestruct(&name);
  ASTNode* node = NewIdentifierASTNode(symbol, lex->current_token_location);
  if (symbol->flags.name_independent_lookup_ambiguous) {
    node->flags |= kASTNameIndependentLookupAmbiguous;
  }
  if (is_qualified_name) {
    node->flags |= kASTQualifiedName;
  }
  if (is_functional_class_construction) {
    node->flags |= kASTCXXFunctionalConstruction;
    if (functional_constructor_owner != NULL) {
      TypeRecord* owner_type = NewTypeRecord(
          functional_constructor_owner->is_union ? kTypeUnion : kTypeStruct,
          kQualPlain);
      TypeRecordSetStructInfo(owner_type, functional_constructor_owner);
      TypeRecordCalculateSize(owner_type);
      ASTNodeSetType(node, owner_type);
      TypeRecordDelete(owner_type);
    }
  }
  ((IdentifierASTNode*)node)->template_arguments = template_arguments;
  CXXPackExpressionSearch pack_search = {.syntax = syntax, .found = false};
  for (size_t i = 0;
       template_arguments != NULL && i < template_arguments->length; i++) {
    if (CXXTemplateArgumentReferencesParameterPack(
            &pack_search, template_arguments->value.p[i])) {
      node->flags |= kASTReferencesParameterPack;
      break;
    }
  }
  return node;
}

static ASTNode* ParseIntegerConstant(Syntax* syntax,
                                                 TokenClass followers) {
  
  Lex* lex = syntax->lex;
  int64_t value = lex->number;

  // The type of an integer constant is the first type from a candidate list
  // (C11 6.4.4.1) that can represent its value.  Decimal constants only
  // consider signed types when there is no 'u' suffix; hexadecimal and octal
  // constants may also pick an unsigned type.  Capture the radix and suffix
  // flags before advancing the lexer, which overwrites them.
  const char* spelling = lex->spelling.value;
  bool octal_or_hex = spelling != NULL && spelling[0] == '0' &&
                      (spelling[1] == 'x' || spelling[1] == 'X' ||
                       spelling[1] == 'b' || spelling[1] == 'B' ||
                       (spelling[1] >= '0' && spelling[1] <= '7'));
  bool has_u = StringContainsChar(&lex->suffix, 'U');
  bool has_wb = StringContainsString(&lex->suffix, "WB");
  bool has_z = StringContainsChar(&lex->suffix, 'Z');
  bool has_ll = StringContainsString(&lex->suffix, "LL");
  bool has_l = !has_ll && StringContainsChar(&lex->suffix, 'L');
  SourceLocation location = lex->current_token_location;
  if (has_wb) {
    uint64_t magnitude = (uint64_t)value;
    int value_bits = 0;
    for (uint64_t remaining = magnitude; remaining != 0; remaining >>= 1) {
      value_bits++;
    }
    int bit_width = has_u ? (value_bits > 0 ? value_bits : 1)
                          : (value_bits > 0 ? value_bits + 1 : 2);
    if (bit_width > DAVECC_BITINT_MAXWIDTH) {
      SyntaxError(syntax,
                  "wb integer literal requires more than BITINT_MAXWIDTH bits");
      bit_width = DAVECC_BITINT_MAXWIDTH;
    }
    TypeRecord* type =
        NewBitIntTypeRecord(bit_width, has_u, kQualPlain);
    LexNextToken(lex);
    return NewIntConstantASTNode(value, type, location);
  }
  if (has_z) {
    TypeRecord* type = NewSizeTypeRecord();
    if (!has_u) {
      // The plain z suffix denotes the signed integer type corresponding to
      // size_t; adding u selects size_t itself.
      type->type &= ~kTypeUnsigned;
    }
    LexNextToken(lex);
    return NewIntConstantASTNode(value, type, location);
  }
  LexNextToken(lex);

  uint64_t uval = (uint64_t)value;
  // Whether an unsigned candidate may be selected when there is no 'u' suffix.
  bool allow_unsigned = octal_or_hex;

  // Candidate integer ranks, in increasing width.  The bit widths are taken
  // from the active target (e.g. int is 16 bits on the 6502 but 32 bits
  // elsewhere) so that a literal is given the narrowest standard type that can
  // actually represent it on that target.
  Type rank_spec[3] = { kTypeInt, kTypeLong, kTypeLongLong };
  int rank_bits[3] = { SizeofType(kTypeInt) * 8,
                       SizeofType(kTypeLong) * 8,
                       SizeofType(kTypeLongLong) * 8 };

  // An explicit L/LL suffix sets a minimum rank.  A 'u' suffix forbids signed
  // candidates; hex/octal constants (or a 'u' suffix) permit unsigned ones.
  int min_rank = has_ll ? 2 : (has_l ? 1 : 0);
  bool try_signed = !has_u;
  bool try_unsigned = has_u || allow_unsigned;

  Type type_specifier = kTypeLongLong | kTypeUnsigned;
  for (int r = min_rank; r < 3; r++) {
    int bits = rank_bits[r];
    uint64_t smax = (bits >= 64) ? 0x7fffffffffffffffULL
                                 : ((1ULL << (bits - 1)) - 1);
    uint64_t umax = (bits >= 64) ? 0xffffffffffffffffULL
                                 : ((1ULL << bits) - 1);
    if (try_signed && uval <= smax) {
      type_specifier = rank_spec[r];
      break;
    }
    if (try_unsigned && uval <= umax) {
      type_specifier = rank_spec[r] | kTypeUnsigned;
      break;
    }
  }

  TypeRecord* type = NewTypeRecordWithSize(type_specifier, kQualPlain);
  return NewIntConstantASTNode(value, type, location);
}

static bool IsCXXSizeLiteralSuffix(String* suffix) {
  if (suffix == NULL || (suffix->length != 1 && suffix->length != 2)) {
    return false;
  }
  char first = toupper((unsigned char)suffix->value[0]);
  if (suffix->length == 1) {
    return first == 'Z';
  }
  char second = toupper((unsigned char)suffix->value[1]);
  return (first == 'U' && second == 'Z') ||
         (first == 'Z' && second == 'U');
}

static void CXXUserDefinedLiteralOperatorName(String* name, String* suffix) {
  StringInit(name, "operator\"\"");
  StringAppendString(name, suffix);
}

static ASTNode* NewCXXLiteralOperatorIdentifier(Syntax* syntax, String* name,
                                                SourceLocation location) {
  Symbol* symbol = SyntaxFindSymbol(syntax, name);
  if (symbol == NULL) {
    SyntaxError(syntax, "No literal operator %s", name->value);
    TypeRecord* result_type =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    TypeRecord* func_type = NewFunctionTypeRecord();
    func_type->info.function.unknown_args = true;
    TypeRecordChain(func_type, result_type);
    symbol = NewSymbol(name->value, func_type, STO(implicit));
    symbol->flags.invented = true;
  }
  return NewIdentifierASTNode(symbol, location);
}

static Type UTF8LiteralElementType(void) {
  if (CompilerCAtLeast(kLanguageStandardC23)) {
    return kTypeChar | kTypeUnsigned;
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX20)) {
    return kTypeChar8;
  }
  return kTypeChar;
}

static ASTNode* NewStringLiteralArgument(String* contents,
                                         SourceLocation location,
                                         LiteralEncoding encoding) {
  int element_size = 1;
  Type element_type = kTypeChar;
  if (encoding == kLiteralEncodingWide) {
    element_size = compiler->wchar_size;
    element_type = kTypeInt;
  } else if (encoding == kLiteralEncodingUTF16) {
    element_size = 2;
    element_type = kTypeChar16;
  } else if (encoding == kLiteralEncodingUTF32) {
    element_size = 4;
    element_type = kTypeChar32;
  } else if (encoding == kLiteralEncodingUTF8) {
    element_type = UTF8LiteralElementType();
  }
  TypeRecord* array = NewBasicArrayTypeRecord(
      kQualPlain, (int)(contents->length / element_size) + 1, false);
  TypeRecord* element = NewTypeRecordWithSize(
      element_type, CompilerIsCXX() ? kQualConst : kQualPlain);
  TypeRecordChain(array, element);
  TypeRecordCalculateSize(array);
  return element_size > 1
             ? NewWideStringConstantASTNode(contents, array, location)
             : NewStringConstantASTNode(contents, array, location);
}

static ASTNode* NewCXXUserDefinedLiteralCallForSymbol(
    Syntax* syntax, String* suffix, Symbol* symbol, Vector* template_arguments,
    Vector* actuals, SourceLocation location);

static ASTNode* NewCXXUserDefinedLiteralCall(Syntax* syntax, String* suffix,
                                             Vector* actuals,
                                             SourceLocation location) {
  return NewCXXUserDefinedLiteralCallForSymbol(
      syntax, suffix, NULL, NULL, actuals, location);
}

static ASTNode* NewCXXUserDefinedLiteralCallForSymbol(
    Syntax* syntax, String* suffix, Symbol* symbol, Vector* template_arguments,
    Vector* actuals, SourceLocation location) {
  String name;
  CXXUserDefinedLiteralOperatorName(&name, suffix);
  ASTNode* callee =
      symbol != NULL ? NewIdentifierASTNode(symbol, location)
                     : NewCXXLiteralOperatorIdentifier(syntax, &name, location);
  ((IdentifierASTNode*)callee)->template_arguments = template_arguments;
  StringDestruct(&name);
  return NewVectorASTNode(AST_OP(call), NULL, location, callee, actuals);
}

static TypeRecord* FirstFunctionFormal(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      symbol->type->info.function.prototype.length == 0) {
    return NULL;
  }
  Symbol* formal = symbol->type->info.function.prototype.value.p[0];
  return formal != NULL ? formal->type : NULL;
}

typedef enum {
  kCXXNumericLiteralOperatorInvalid,
  kCXXNumericLiteralOperatorCooked,
  kCXXNumericLiteralOperatorRaw,
  kCXXNumericLiteralOperatorTemplate,
} CXXNumericLiteralOperatorKind;

typedef struct {
  CXXNumericLiteralOperatorKind kind;
  Symbol* symbol;
} CXXNumericLiteralOperator;

static bool IsCookedNumericLiteralOperator(Symbol* symbol, bool floating) {
  if (symbol == NULL || symbol->flags.is_template ||
      symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      symbol->type->info.function.prototype.length != 1) {
    return false;
  }
  TypeRecord* formal = FirstFunctionFormal(symbol);
  return floating ? TypeIsLongDouble(formal)
                  : TypeIsUnsignedLongLong(formal);
}

static bool IsRawNumericLiteralOperator(Symbol* symbol) {
  if (symbol == NULL || symbol->flags.is_template ||
      symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      symbol->type->info.function.prototype.length != 1) {
    return false;
  }
  TypeRecord* formal = FirstFunctionFormal(symbol);
  return TypeIsPointer(formal) && formal->next != NULL &&
         formal->next->declarator == kDeclPrimitive &&
         formal->next->type == kTypeChar && TypeIsConst(formal->next);
}

static CXXNumericLiteralOperator ResolveCXXNumericLiteralOperator(
    Syntax* syntax, String* suffix, bool floating) {
  String name;
  CXXUserDefinedLiteralOperatorName(&name, suffix);
  Symbol* first = SyntaxFindSymbol(syntax, &name);
  Symbol* cooked = NULL;
  Symbol* raw = NULL;
  Symbol* templ = NULL;
  for (Symbol* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (IsCookedNumericLiteralOperator(candidate, floating)) {
      cooked = candidate;
    } else if (IsRawNumericLiteralOperator(candidate)) {
      raw = candidate;
    } else if (SyntaxIsCXXNumericLiteralOperatorTemplate(candidate)) {
      templ = candidate;
    }
  }
  CXXNumericLiteralOperator result = {
      .kind = kCXXNumericLiteralOperatorInvalid,
      .symbol = first,
  };
  if (cooked != NULL) {
    result.kind = kCXXNumericLiteralOperatorCooked;
    result.symbol = cooked;
  } else if (raw != NULL && templ != NULL) {
    SyntaxError(syntax,
                "Raw and numeric literal operator template cannot both be "
                "declared for %s",
                name.value);
    result.kind = kCXXNumericLiteralOperatorRaw;
    result.symbol = raw;
  } else if (raw != NULL) {
    result.kind = kCXXNumericLiteralOperatorRaw;
    result.symbol = raw;
  } else if (templ != NULL) {
    result.kind = kCXXNumericLiteralOperatorTemplate;
    result.symbol = templ;
  } else if (first != NULL) {
    SyntaxError(syntax, "No matching numeric literal operator %s", name.value);
  }
  StringDestruct(&name);
  return result;
}

static Vector* CXXNumericLiteralTemplateArguments(String* spelling) {
  Vector* arguments = NewVector();
  size_t length = spelling->length;
  if (length > 0 && spelling->value[length - 1] == '\0') {
    length--;
  }
  for (size_t i = 0; i < length; i++) {
    VectorAppend(arguments, NewIntegralTemplateArgument(
                                (unsigned char)spelling->value[i]));
  }
  return arguments;
}

static ASTNode* ParseCXXUserDefinedIntegerLiteral(Syntax* syntax) {
  Lex* lex = syntax->lex;
  int64_t value = lex->number;
  SourceLocation location = lex->current_token_location;
  String suffix;
  StringInit(&suffix, NULL);
  StringSetString(&suffix, &lex->ud_suffix);
  String spelling;
  StringInit(&spelling, NULL);
  StringSetString(&spelling, &lex->literal_spelling);
  CXXNumericLiteralOperator literal_operator =
      ResolveCXXNumericLiteralOperator(syntax, &suffix, /*floating=*/false);
  LexNextToken(lex);

  Vector* actuals = NewVector();
  Vector* template_arguments = NULL;
  if (literal_operator.kind == kCXXNumericLiteralOperatorRaw) {
    VectorAppend(actuals, NewStringLiteralArgument(
                              NewString(spelling.value), location,
                              kLiteralEncodingNone));
  } else if (literal_operator.kind == kCXXNumericLiteralOperatorTemplate) {
    template_arguments = CXXNumericLiteralTemplateArguments(&spelling);
  } else {
    // [lex.ext] fixes the cooked integer argument type regardless of the
    // ordinary type the unsuffixed token would otherwise have.
    TypeRecord* type =
        NewTypeRecordWithSize(kTypeLongLong | kTypeUnsigned, kQualPlain);
    VectorAppend(actuals, NewIntConstantASTNode(value, type, location));
  }
  StringDestruct(&spelling);
  ASTNode* call = NewCXXUserDefinedLiteralCallForSymbol(
      syntax, &suffix, literal_operator.symbol, template_arguments, actuals,
      location);
  StringDestruct(&suffix);
  return call;
}

static ASTNode* ParseFloatingPointConstant(Syntax* syntax,
                                                 TokenClass followers) {
  Lex* lex = syntax->lex;
  double value = lex->fnumber;
  LexNextToken(lex);
  
  // Floating point numbers have the standard F/L suffixes and, in C++23,
  // fixed-width extended f32/f64 suffixes.
  Type type_specifier = kTypeDouble;
  if (StringEqual(&lex->suffix, "F32")) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
      SyntaxError(syntax, "The f32 floating-point suffix requires C++23");
    }
    type_specifier = kTypeFloat32;
  } else if (StringEqual(&lex->suffix, "F64")) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
      SyntaxError(syntax, "The f64 floating-point suffix requires C++23");
    }
    if (StringEqual(compiler->target_name, "6502") ||
        StringEqual(compiler->target_name, "65c02")) {
      SyntaxError(syntax,
                  "The binary64 extended floating-point type is not "
                  "supported on this target");
    }
    type_specifier = kTypeFloat64;
  } else if (StringContainsChar(&lex->suffix, 'F')) {
    type_specifier &= ~kTypeDouble;
    type_specifier |= kTypeFloat;
  } else if (StringContainsChar(&lex->suffix, 'L')) {
    type_specifier &= ~kTypeDouble;
    type_specifier |= kTypeLongDouble;
  }
  TypeRecord* type = NewTypeRecordWithSize(type_specifier, kQualPlain);
  return NewRealConstantASTNode(value, type,
                                syntax->lex->current_token_location);
}

static ASTNode* ParseCXXUserDefinedFloatingLiteral(Syntax* syntax) {
  Lex* lex = syntax->lex;
  double value = lex->fnumber;
  SourceLocation location = lex->current_token_location;
  String suffix;
  StringInit(&suffix, NULL);
  StringSetString(&suffix, &lex->ud_suffix);
  String spelling;
  StringInit(&spelling, NULL);
  StringSetString(&spelling, &lex->literal_spelling);
  CXXNumericLiteralOperator literal_operator =
      ResolveCXXNumericLiteralOperator(syntax, &suffix, /*floating=*/true);
  LexNextToken(lex);

  Vector* actuals = NewVector();
  Vector* template_arguments = NULL;
  if (literal_operator.kind == kCXXNumericLiteralOperatorRaw) {
    VectorAppend(actuals, NewStringLiteralArgument(
                              NewString(spelling.value), location,
                              kLiteralEncodingNone));
  } else if (literal_operator.kind == kCXXNumericLiteralOperatorTemplate) {
    template_arguments = CXXNumericLiteralTemplateArguments(&spelling);
  } else {
    // [lex.ext] fixes the cooked floating argument type at long double.
    TypeRecord* type = NewTypeRecordWithSize(kTypeLongDouble, kQualPlain);
    VectorAppend(actuals, NewRealConstantASTNode(value, type, location));
  }
  StringDestruct(&spelling);
  ASTNode* call = NewCXXUserDefinedLiteralCallForSymbol(
      syntax, &suffix, literal_operator.symbol, template_arguments, actuals,
      location);
  StringDestruct(&suffix);
  return call;
}

static ASTNode* ParseStringLiteral(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  LiteralEncoding encoding = lex->literal_encoding;
  bool user_defined = lex->ud_suffix.length != 0;
  String suffix;
  StringInit(&suffix, NULL);
  if (user_defined) {
    StringSetString(&suffix, &lex->ud_suffix);
  }
  String* contents =
      NewStringWithLength(lex->spelling.value, lex->spelling.length);
  LexNextToken(lex);
  
  // Adjacent string literals are joined together.
  while (LexLookingAt(lex, TOK(string))) {
    LiteralEncoding next_encoding = lex->literal_encoding;
    if (next_encoding != encoding) {
      if (CompilerCXXAtLeast(kLanguageStandardCXX23) ||
          (encoding != kLiteralEncodingNone &&
           next_encoding != kLiteralEncodingNone)) {
        SyntaxError(syntax,
                    "Cannot concatenate string literals with different "
                    "encoding prefixes");
      } else if (encoding == kLiteralEncodingNone) {
        // Before C++23 an ordinary literal adjacent to a prefixed literal
        // adopts the prefixed literal's encoding.
        encoding = next_encoding;
      }
    }
    StringAppendSegment(contents, lex->spelling.value, lex->spelling.length);
    if (lex->ud_suffix.length != 0) {
      user_defined = true;
      StringSetString(&suffix, &lex->ud_suffix);
    }
    LexNextToken(lex);
  }
  if (user_defined) {
    Vector* actuals = NewVector();
    int element_size =
        encoding == kLiteralEncodingUTF16
            ? 2
            : encoding == kLiteralEncodingUTF32 ? 4 : 1;
    size_t length = contents->length / element_size;
    VectorAppend(actuals,
                 NewStringLiteralArgument(contents, location, encoding));
    VectorAppend(actuals,
                 NewIntConstantASTNode((int64_t)length, NewSizeTypeRecord(),
                                       location));
    ASTNode* call =
        NewCXXUserDefinedLiteralCall(syntax, &suffix, actuals, location);
    StringDestruct(&suffix);
    return call;
  }
  StringDestruct(&suffix);
  
  int element_size =
      encoding == kLiteralEncodingUTF16
          ? 2
          : encoding == kLiteralEncodingUTF32 ? 4 : 1;
  TypeRecord* array = NewBasicArrayTypeRecord(
      kQualPlain, (int)(contents->length / element_size) + 1, false);
  // In C++ a narrow string literal has type `const char[N]`; in C it is a
  // non-const `char[N]` (modifying it is undefined behavior, but the type is
  // not const-qualified).
  Type element_type =
      encoding == kLiteralEncodingUTF16
          ? kTypeChar16
          : encoding == kLiteralEncodingUTF32
                ? kTypeChar32
                : encoding == kLiteralEncodingUTF8
                      ? UTF8LiteralElementType()
                      : kTypeChar;
  TypeRecord* type = NewTypeRecordWithSize(
      element_type, CompilerIsCXX() ? kQualConst : kQualPlain);
  TypeRecordChain(array, type);
  TypeRecordCalculateSize(array);
  return element_size > 1
             ? NewWideStringConstantASTNode(contents, array, location)
             : NewStringConstantASTNode(contents, array, location);
  
}

static ASTNode* ParseWideStringLiteral(Syntax* syntax,
                                               TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  bool user_defined = lex->ud_suffix.length != 0;
  String suffix;
  StringInit(&suffix, NULL);
  if (user_defined) {
    StringSetString(&suffix, &lex->ud_suffix);
  }
  String* contents =
      NewStringWithLength(lex->spelling.value, lex->spelling.length);
  LexNextToken(lex);
  
  // Adjacent wide string literals are joined together.
  while (LexLookingAt(lex, TOK(string_wide))) {
    StringAppendSegment(contents, lex->spelling.value, lex->spelling.length);
    if (lex->ud_suffix.length != 0) {
      user_defined = true;
      StringSetString(&suffix, &lex->ud_suffix);
    }
    LexNextToken(lex);
  }
  if (user_defined) {
    Vector* actuals = NewVector();
    size_t length = contents->length / compiler->wchar_size;
    VectorAppend(actuals, NewStringLiteralArgument(
                              contents, location, kLiteralEncodingWide));
    VectorAppend(actuals,
                 NewIntConstantASTNode((int64_t)length, NewSizeTypeRecord(),
                                       location));
    ASTNode* call =
        NewCXXUserDefinedLiteralCall(syntax, &suffix, actuals, location);
    StringDestruct(&suffix);
    return call;
  }
  StringDestruct(&suffix);
  
  TypeRecord* array = NewBasicArrayTypeRecord(
      kQualPlain, (int)(contents->length / compiler->wchar_size) + 1, false);
  TypeRecord* type = NewTypeRecordWithSize(
      kTypeInt, CompilerIsCXX() ? kQualConst : kQualPlain);
  TypeRecordChain(array, type);
  TypeRecordCalculateSize(array);
  return NewWideStringConstantASTNode(contents, array,
                                 syntax->lex->current_token_location);
}

static ASTNode* ParseCharacterConstant(Syntax* syntax,
                                                   TokenClass followers) {
  Lex* lex = syntax->lex;
  int value = (int)lex->number;
  SourceLocation location = lex->current_token_location;
  LiteralEncoding encoding = lex->literal_encoding;
  Type literal_type =
      encoding == kLiteralEncodingUTF16
          ? kTypeChar16
          : encoding == kLiteralEncodingUTF32
                ? kTypeChar32
                : encoding == kLiteralEncodingUTF8
                      ? UTF8LiteralElementType()
                      : kTypeChar;
  if (lex->ud_suffix.length != 0) {
    String suffix;
    StringInit(&suffix, NULL);
    StringSetString(&suffix, &lex->ud_suffix);
    LexNextToken(lex);
    Vector* actuals = NewVector();
    TypeRecord* type = NewTypeRecordWithSize(literal_type, kQualPlain);
    VectorAppend(actuals, NewCharConstantASTNode(value, type, location));
    ASTNode* call =
        NewCXXUserDefinedLiteralCall(syntax, &suffix, actuals, location);
    StringDestruct(&suffix);
    return call;
  }
  LexNextToken(lex);
  TypeRecord* type = NewTypeRecordWithSize(literal_type, kQualPlain);
  return NewCharConstantASTNode(value, type, location);
}

static ASTNode* ParseWideCharacterConstant(Syntax* syntax,
                                       TokenClass followers) {
  Lex* lex = syntax->lex;
  int value = (int)lex->number;
  if (lex->ud_suffix.length != 0) {
    SourceLocation location = lex->current_token_location;
    String suffix;
    StringInit(&suffix, NULL);
    StringSetString(&suffix, &lex->ud_suffix);
    LexNextToken(lex);
    Vector* actuals = NewVector();
    TypeRecord* type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
    VectorAppend(actuals, NewCharConstantASTNode(value, type, location));
    ASTNode* call =
        NewCXXUserDefinedLiteralCall(syntax, &suffix, actuals, location);
    StringDestruct(&suffix);
    return call;
  }
  LexNextToken(lex);
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  return NewCharConstantASTNode(value, type,
                                syntax->lex->current_token_location);
}

// Parse the highest priority expression - a primary expression.
// This consists of one of:
// 1. A parenthesized expression.
// 2. An identifier
// 3. An integer or floating point constant
// 4. A string literal
// 5. A character constant
//
// We create an Abstract Syntax Tree node for it.  At this point we know
// the type of the AST node because it's either well defined (an integer
// constant for example) or can be read from the symbol table.
//
// The syntax for this is:

// Build the type used to match a _Generic controlling expression against the
// association type names.  This is the type of the controlling expression
// after lvalue conversion: array and function types decay to pointers and any
// top-level qualifiers are removed (C11 6.5.1.1).
static TypeRecord* GenericControllingType(TypeRecord* ctype) {
  if (TypeIsArray(ctype)) {
    TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
    TypeRecordChain(ptr, ctype->next);
    TypeRecordCalculateSize(ptr);
    return ptr;
  }
  if (TypeIsFunction(ctype)) {
    TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
    TypeRecordChain(ptr, ctype);
    TypeRecordCalculateSize(ptr);
    return ptr;
  }
  TypeRecord* copy = TypeRecordCopy(ctype);
  copy->qualifiers = kQualPlain;
  return copy;
}

// Canonicalize a primitive type's specifier bits for _Generic matching so that
// equivalent spellings compare equal (e.g. "long" == "signed long int").  The
// Character types (char/signed char/unsigned char/char8_t) stay distinct, as
// does signedness for the other integer types.
static int CanonicalPrimitive(int t) {
  if (t & kTypeChar32) {
    return kTypeChar32;
  }
  if (t & kTypeChar16) {
    return kTypeChar16;
  }
  if (t & kTypeChar8) {
    return kTypeChar8;
  }
  if (t & kTypeChar) {
    return t & (kTypeChar | kTypeSigned | kTypeUnsigned);
  }
  if (t & (kTypeFloat | kTypeDouble | kTypeLongDouble | kTypeVoid | kTypeBool)) {
    return t & (kTypeFloat | kTypeDouble | kTypeLongDouble | kTypeVoid |
                kTypeBool);
  }
  int size = t & (kTypeShort | kTypeLong | kTypeLongLong);
  int sign = (t & kTypeUnsigned) ? kTypeUnsigned : kTypeSigned;
  return size | kTypeInt | sign;
}

// Type matching for _Generic associations.  Unlike TypeEqual this distinguishes
// distinct struct/union/enum tags (which must select different associations)
// and treats equivalent integer spellings as identical.
static bool GenericTypeMatch(TypeRecord* a, TypeRecord* b) {
  if ((a->type & kTypeUnknown) != 0 || (b->type & kTypeUnknown) != 0) {
    return true;
  }
  if (a->declarator != b->declarator) {
    return false;
  }
  switch (a->declarator) {
    case kDeclArray:
      return GenericTypeMatch(a->next, b->next) &&
             a->info.array.size.fixed == b->info.array.size.fixed;
    case kDeclPointer:
      return GenericTypeMatch(a->next, b->next);
    case kDeclFunction:
      return TypeEqual(a, b);
    case kDeclPrimitive:
    default:
      if (a->qualifiers != b->qualifiers) {
        return false;
      }
      if (TypeIsStructOrUnion(a) || TypeIsStructOrUnion(b)) {
        return TypeIsStructOrUnion(a) && TypeIsStructOrUnion(b) &&
               a->info.struct_info == b->info.struct_info;
      }
      if (TypeIsEnum(a) || TypeIsEnum(b)) {
        return TypeIsEnum(a) && TypeIsEnum(b) &&
               a->info.enum_info == b->info.enum_info;
      }
      if (TypeIsBitInt(a) || TypeIsBitInt(b)) {
        return TypeIsBitInt(a) && TypeIsBitInt(b) &&
               a->bit_width == b->bit_width &&
               TypeIsUnsigned(a) == TypeIsUnsigned(b);
      }
      return CanonicalPrimitive(a->type) == CanonicalPrimitive(b->type);
  }
}

// Parse a C11 _Generic selection:
//   _Generic ( assignment-expression , generic-assoc-list )
//   generic-association:
//     type-name : assignment-expression
//     default : assignment-expression
// The controlling expression is an unevaluated operand: only its type is used
// to pick the matching association.  We return the selected expression (or the
// default), which is then analyzed normally as part of the surrounding AST.
static ASTNode* ParseGenericSelection(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  LexNextToken(lex);  // Consume "_Generic".
  SyntaxNeedBracket(syntax, TOK(lparen), followers);

  // Determine the controlling expression's type without keeping the node: it
  // is unevaluated, so we analyze it only to obtain its type.
  ASTNode* controlling =
      SyntaxParseSingleExpression(syntax, followers | TC(exprsep));
  controlling = AnalyzeExpression(controlling);
  TypeRecord* match_type =
      controlling->type != NULL ? GenericControllingType(controlling->type)
                                : NewTypeRecordWithSize(kTypeInt, kQualPlain);

  SyntaxNeedBracket(syntax, TOK(comma), followers);

  ASTNode* selected = NULL;
  ASTNode* default_expr = NULL;
  while (!LexEof(lex)) {
    bool is_default = false;
    TypeRecord* assoc_type = NULL;
    Symbol* assoc_sym = NULL;
    if (LexMatch(lex, TOK(default))) {
      is_default = true;
    } else {
      TypeParser parser;
      TypeParserInit(&parser, lex, syntax, STO(implicit), syntax->context);
      TypeRecord* type = TypeParserParseType(&parser, true);
      assoc_sym = TypeParserParseDeclarator(&parser, type);
      assoc_type = assoc_sym != NULL ? assoc_sym->type : type;
      TypeParserDestruct(&parser);
    }
    SyntaxNeedBracket(syntax, TOK(colon), followers);
    ASTNode* expr = SyntaxParseSingleExpression(syntax, followers | TC(exprsep));

    if (is_default) {
      default_expr = expr;
    } else if (selected == NULL && assoc_type != NULL &&
               GenericTypeMatch(match_type, assoc_type)) {
      selected = expr;
    } else {
      ASTNodeDelete(expr);
    }
    if (assoc_sym != NULL) {
      SymbolDelete(assoc_sym);
    }
    if (!LexMatch(lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  ASTNode* result = selected != NULL ? selected : default_expr;
  if (result == NULL) {
    SyntaxError(syntax, "No matching association in _Generic selection");
    result = (ASTNode*)NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
        syntax->lex->current_token_location);
  } else if (selected != NULL && default_expr != NULL) {
    ASTNodeDelete(default_expr);
  }
  return result;
}

// Allocate a LambdaCapture; the closure-class `field` is filled in later by
// AddLambdaCaptureFields.
static LambdaCapture* NewLambdaCapture(Symbol* captured, bool by_reference,
                                       bool is_pack_expansion,
                                       ASTNode* initializer,
                                       bool is_init_capture) {
  LambdaCapture* capture = malloc(sizeof(LambdaCapture));
  capture->captured = captured;
  capture->field = NULL;
  capture->initializer = initializer;
  capture->by_reference = by_reference;
  capture->is_pack_expansion = is_pack_expansion;
  capture->is_init_capture = is_init_capture;
  return capture;
}

// Find an existing capture of `symbol`, or NULL if it is not yet captured.
static LambdaCapture* FindLambdaCapture(Vector* captures, Symbol* symbol) {
  for (size_t i = 0; i < captures->length; i++) {
    LambdaCapture* capture = captures->value.p[i];
    if (capture->captured == symbol) {
      return capture;
    }
  }
  return NULL;
}

// Find an existing capture whose captured name matches `name` (used to reject
// duplicate captures, including init-captures that introduce a fresh symbol).
static LambdaCapture* FindLambdaCaptureName(Vector* captures, String* name) {
  for (size_t i = 0; i < captures->length; i++) {
    LambdaCapture* capture = captures->value.p[i];
    if (capture->captured != NULL &&
        StringEqualString(&capture->captured->name, name)) {
      return capture;
    }
  }
  return NULL;
}

// True when `symbol` is one of the lambda's own parameters (including the
// synthesized `this`), which must never be treated as a captured variable.
static bool LambdaFunctionOwnsSymbol(TypeRecord* func, Symbol* symbol) {
  if (func == NULL || symbol == NULL) {
    return false;
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    if (func->info.function.prototype.value.p[i] == symbol) {
      return true;
    }
  }
  return false;
}

// True when `symbol` is eligible for implicit capture: a local automatic
// variable of the enclosing scope.  Lambda parameters, invented temporaries,
// functions, statics/externs/typedefs and globals are all excluded.
static bool CanCaptureSymbol(Symbol* symbol, TypeRecord* lambda_func) {
  if (symbol == NULL || LambdaFunctionOwnsSymbol(lambda_func, symbol) ||
      symbol->flags.invented || TypeIsFunction(symbol->type) ||
      StorageIs(symbol->storage, STO(static) | STO(extern) | STO(typedef))) {
    return false;
  }
  Symbol* global = FindGlobalSymbol(&symbol->name);
  return global != symbol;
}

// Parse the `[...]` capture list, populating `captures` and reporting the
// default capture mode.  Handles the leading default ([=] / [&]), explicit
// by-value and by-reference captures, init-captures ([x = expr]) and pack
// expansions ([...xs]).
static void ParseLambdaCaptureList(Syntax* syntax, Vector* captures,
                                   LambdaCaptureDefault* capture_default,
                                   TokenClass followers) {
  *capture_default = kLambdaCaptureDefaultNone;
  if (LexMatch(syntax->lex, TOK(rsquare))) {
    return;
  }
  if (LexMatch(syntax->lex, TOK(equal))) {
    *capture_default = kLambdaCaptureDefaultValue;
    LexMatch(syntax->lex, TOK(comma));
  } else if (LexLookingAt(syntax->lex, TOK(amp))) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(rsquare)) ||
        LexLookingAt(syntax->lex, TOK(comma))) {
      *capture_default = kLambdaCaptureDefaultReference;
      LexMatch(syntax->lex, TOK(comma));
      LexCheckpointDestruct(&checkpoint);
    } else {
      LexCheckpointRestore(syntax->lex, &checkpoint);
      LexCheckpointDestruct(&checkpoint);
    }
  }

  while (!LexLookingAt(syntax->lex, TOK(rsquare)) && !LexEof(syntax->lex)) {
    bool by_reference = LexMatch(syntax->lex, TOK(amp));
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected lambda capture name");
      break;
    }
    String name;
    StringInit(&name, syntax->lex->spelling.value);
    SourceLocation name_location = syntax->lex->current_token_location;
    Symbol* symbol = SyntaxFindSymbol(syntax, &name);
    LexNextToken(syntax->lex);
    if (LexMatch(syntax->lex, TOK(equal))) {
      // Init-capture `[name = expr]`: introduce a brand-new local whose type is
      // deduced from the initializer (defaulting to int when unknown).
      ASTNode* initializer =
          ParseAssignmentExpression(syntax, followers | TC(exprsep) |
                                                TC(closebra));
      MarkCXXPackExpansionIfPresent(syntax, initializer);
      bool is_pack_expansion =
          initializer != NULL && (initializer->flags & kASTPackExpansion) != 0;
      TypeRecord* capture_type =
          initializer != NULL && initializer->type != NULL
              ? TypeRecordCopy(initializer->type)
              : NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      Symbol* init_capture =
          NewSymbol(name.value, capture_type, STO(auto));
      init_capture->flags.is_defined = true;
      init_capture->flags.is_local = true;
      init_capture->flags.is_parameter_pack = is_pack_expansion;
      bool name_independent =
          CompilerCXXAtLeast(kLanguageStandardCXX26) &&
          StringEqual(&name, "_");
      init_capture->flags.is_name_independent = name_independent;
      init_capture->location = name_location;
      if (name_independent ||
          FindLambdaCaptureName(captures, &name) == NULL) {
        VectorAppend(captures,
                     NewLambdaCapture(init_capture, by_reference,
                                      is_pack_expansion, initializer,
                                      /*is_init_capture=*/true));
      }
      StringDestruct(&name);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      continue;
    }
    bool is_pack_expansion = LexMatch(syntax->lex, TOK(ellipsis));
    if (symbol == NULL) {
      SyntaxError(syntax, "Unknown lambda capture %s", name.value);
    } else if (is_pack_expansion && !symbol->flags.is_parameter_pack) {
      SyntaxError(syntax,
                  "lambda capture pack expansion requires a function parameter pack");
    } else if (FindLambdaCapture(captures, symbol) == NULL &&
               FindLambdaCaptureName(captures, &name) == NULL) {
      VectorAppend(captures,
                   NewLambdaCapture(symbol, by_reference, is_pack_expansion,
                                    /*initializer=*/NULL,
                                    /*is_init_capture=*/false));
    }
    StringDestruct(&name);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rsquare), followers);
}

// Queue the closure's operator() for instantiation/codegen alongside template
// instantiations, so its body is emitted after the enclosing context is parsed.
static void QueueLambdaCallOperatorDefinition(Symbol* symbol) {
  Vector* declarations = NewVector();
  VectorAppend(declarations,
               NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
  CompilerQueuePendingTemplateInstantiation(
      NewDeclarationListASTNode(declarations, symbol->location));
}

// Bring the lambda's parameters into the body's local scope before parsing it.
static void AddLambdaFunctionScopeSymbols(Syntax* syntax, TypeRecord* func) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = formals->value.p[i];
    InsertLocalSymbol(syntax->local_symbol_stack, formal);
  }
}

static bool LambdaFormalNameAvailable(Vector* formals, String* name) {
  if (name->length == 0) {
    return true;
  }
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = formals->value.p[i];
    if (formal != NULL && StringEqualString(&formal->name, name)) {
      return false;
    }
  }
  return true;
}

static TemplateArgument* NewLambdaTemplateParameterTypeArgument(int index,
                                                               TypeRecord* type) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->type = type != NULL ? TypeRecordCopy(type) : NULL;
  arg->template_parameter_index = index;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static TemplateParameter* NewLambdaTemplateParameter(Syntax* syntax,
                                                     int index,
                                                     bool is_parameter_pack) {
  TemplateParameter* param = malloc(sizeof(TemplateParameter));
  memset(param, 0, sizeof(*param));
  StringInit(&param->name, SyntaxFakeName(syntax));
  param->kind = kTemplateParameterType;
  param->index = index;
  param->is_parameter_pack = is_parameter_pack;
  return param;
}

static void AddLambdaFunctionAssociatedConstraint(TypeRecord* func,
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

static bool ParseLambdaAbbreviatedParameter(Syntax* syntax, TypeRecord* func,
                                            int arg_number,
                                            TokenClass followers) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20)) {
    return false;
  }

  // Leading cv-qualifiers of a `const auto&` / `volatile auto` parameter are
  // consumed speculatively and rewound if no placeholder follows.
  LexCheckpoint cv_checkpoint;
  LexCheckpointSave(syntax->lex, &cv_checkpoint);
  int placeholder_qualifiers = kQualPlain;
  while (true) {
    if (LexLookingAt(syntax->lex, TOK(const))) {
      placeholder_qualifiers |= kQualConst;
      LexNextToken(syntax->lex);
    } else if (LexLookingAt(syntax->lex, TOK(volatile))) {
      placeholder_qualifiers |= kQualVolatile;
      LexNextToken(syntax->lex);
    } else {
      break;
    }
  }

  Symbol* concept_symbol = NULL;
  SourceLocation constraint_location = syntax->lex->current_token_location;
  Vector* concept_arguments = NULL;
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    String concept_name;
    StringInit(&concept_name, syntax->lex->spelling.value);
    Symbol* found = SyntaxFindSymbol(syntax, &concept_name);
    StringDestruct(&concept_name);
    if (found != NULL && found->flags.is_concept) {
      LexCheckpoint checkpoint;
      LexCheckpointSave(syntax->lex, &checkpoint);
      LexNextToken(syntax->lex);
      if (LexLookingAt(syntax->lex, TOK(less))) {
        concept_arguments = SyntaxParseTemplateArgumentList(syntax, followers);
      } else {
        concept_arguments = NewVector();
      }
      if (LexLookingAt(syntax->lex, TOK(auto))) {
        concept_symbol = found;
      } else {
        VectorDeleteWithContents(
            concept_arguments, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        concept_arguments = NULL;
        LexCheckpointRestore(syntax->lex, &checkpoint);
      }
      LexCheckpointDestruct(&checkpoint);
    }
  }

  if (concept_symbol == NULL && !LexLookingAt(syntax->lex, TOK(auto))) {
    LexCheckpointRestore(syntax->lex, &cv_checkpoint);
    LexCheckpointDestruct(&cv_checkpoint);
    return false;
  }
  LexCheckpointDestruct(&cv_checkpoint);
  LexNextToken(syntax->lex);  // auto

  int index = syntax->current_template_parameter_count +
              (int)func->info.function.template_parameters.length;
  TypeRecord* placeholder =
      NewTypeRecordWithSize(kTypeInt | kTypeUnknown, placeholder_qualifiers);
  placeholder->template_parameter_index = index;
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(auto), kParsingPrototype);
  Symbol* formal = TypeParserParseDeclarator(&parser, placeholder);
  TypeParserDestruct(&parser);
  assert(formal != NULL);
  if (!LambdaFormalNameAvailable(&func->info.function.prototype,
                                 &formal->name)) {
    SyntaxError(syntax, "Duplicate function argument '%s'",
                formal->name.value);
    SymbolDelete(formal);
    TypeRecordDelete(placeholder);
    if (concept_arguments != NULL) {
      VectorDeleteWithContents(concept_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return true;
  }
  formal->flags.is_defined = true;
  formal->flags.is_argument = true;
  formal->value.arg_number = arg_number;
  // `auto... xs` / `Concept auto... xs`: the declarator sets
  // formal->flags.is_parameter_pack; the invented template parameter must
  // match or deduction treats the pack as a single type parameter and the
  // instantiated operator() is never emitted for multi-arg calls.
  VectorAppend(&func->info.function.prototype, formal);
  VectorAppend(&func->info.function.template_parameters,
               NewLambdaTemplateParameter(syntax, index,
                                          formal->flags.is_parameter_pack));
  if (concept_symbol != NULL) {
    if (concept_arguments == NULL) {
      concept_arguments = NewVector();
    }
    TemplateArgument* constrained_arg =
        NewLambdaTemplateParameterTypeArgument(index, placeholder);
    if (concept_arguments->length == 0) {
      VectorAppend(concept_arguments, constrained_arg);
    } else {
      VectorInsertBefore(concept_arguments, 0, constrained_arg);
    }
    AddLambdaFunctionAssociatedConstraint(
        func, NewConceptIdConstraint(concept_symbol, concept_arguments,
                                     constraint_location));
  }
  func->info.function.template_parameter_count =
      (int)func->info.function.template_parameters.length;
  TypeRecordDelete(placeholder);
  return true;
}

// Parse the optional `(params)` of a lambda into `func`'s prototype.  An
// omitted parameter list is allowed and leaves the prototype empty.  Return
// whether the parameter list was present so C++23's expanded parameter-list
// omission can be diagnosed in older modes.
static bool ParseLambdaParameterList(Syntax* syntax, TypeRecord* func,
                                     TokenClass followers) {
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return false;
  }
  int arg_number = 0;
  while (!LexLookingAt(syntax->lex, TOK(rparen)) && !LexEof(syntax->lex)) {
    bool explicit_object_parameter =
        CompilerIsCXX() && LexMatch(syntax->lex, TOK(this));
    if (explicit_object_parameter) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
        SyntaxError(syntax, "explicit object parameters require C++23");
      }
      if (arg_number != 0) {
        SyntaxError(syntax,
                    "explicit object parameter must be the first parameter");
      }
      func->info.function.has_explicit_object_parameter = true;
    }
    if (ParseLambdaAbbreviatedParameter(syntax, func, arg_number, followers)) {
      Symbol* formal = func->info.function.prototype.value.p[
          func->info.function.prototype.length - 1];
      if (explicit_object_parameter && formal->flags.is_parameter_pack) {
        SyntaxError(syntax,
                    "explicit object parameter cannot be a parameter pack");
      }
      arg_number++;
    } else {
      TypeParser parser;
      TypeParserInit(&parser, syntax->lex, syntax, STO(auto), kParsingPrototype);
      TypeRecord* type = TypeParserParseType(&parser, true);
      Symbol* formal = TypeParserParseDeclarator(&parser, type);
      TypeParserDestruct(&parser);
      if (formal != NULL) {
        if (TypeContainsAuto(formal->type)) {
          SymbolSetType(formal, NewTypeRecordWithSize(kTypeInt, kQualPlain));
        }
        formal->flags.is_defined = true;
        formal->flags.is_argument = true;
        formal->value.arg_number = arg_number++;
        if (explicit_object_parameter && formal->flags.is_parameter_pack) {
          SyntaxError(syntax,
                      "explicit object parameter cannot be a parameter pack");
        }
        VectorAppend(&func->info.function.prototype, formal);
      }
    }
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return true;
}

static void ParseLambdaTrailingRequiresClause(Syntax* syntax,
                                              TypeRecord* func) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20) ||
      !LexLookingAt(syntax->lex, TOK(requires))) {
    return;
  }
  SyntaxOpenScope(syntax);
  AddLambdaFunctionScopeSymbols(syntax, func);
  ConstraintExpr* constraint = ConceptsParseRequiresClause(syntax);
  SyntaxCloseScope(syntax);
  AddLambdaFunctionAssociatedConstraint(func, constraint);
}

// Parse a lambda's optional noexcept-specifier.  Returns true when the lambda
// is known non-throwing (a plain `noexcept`).  A conditional `noexcept(expr)`
// operand is skipped without evaluation and conservatively reported as not
// guaranteed non-throwing, so noexcept enforcement never produces a false
// positive for a lambda whose operand we did not evaluate.
static bool SkipNoexceptSpecifier(Syntax* syntax, TokenClass followers) {
  if (!LexMatch(syntax->lex, TOK(noexcept))) {
    return false;
  }
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return true;
  }
  int depth = 1;
  while (depth > 0 && !LexEof(syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(lparen))) {
      depth++;
    } else if (LexLookingAt(syntax->lex, TOK(rparen))) {
      depth--;
    }
    LexNextToken(syntax->lex);
  }
  if (depth != 0) {
    SyntaxRecover(syntax, followers);
  }
  return false;
}

// Parse the specifiers that follow a lambda's parameter list (`mutable`,
// `constexpr`, `consteval`, `noexcept`) in any order, then optional trailing
// return type and trailing requires-clause. Returns the trailing return type if
// present, otherwise `default_type`, and reports each specifier through its
// out-parameter.
static TypeRecord* ParseLambdaSpecifiersAndReturnType(Syntax* syntax,
                                                      TypeRecord* func,
                                                      bool* is_mutable,
                                                      bool* is_static,
                                                      bool* is_constexpr,
                                                      bool* is_consteval,
                                                      bool* is_noexcept,
                                                      TypeRecord* default_type,
                                                      TokenClass followers) {
  *is_mutable = false;
  *is_static = false;
  *is_constexpr = false;
  *is_consteval = false;
  *is_noexcept = false;
  bool keep_parsing = true;
  while (keep_parsing) {
    if (LexMatch(syntax->lex, TOK(mutable))) {
      *is_mutable = true;
    } else if (LexMatch(syntax->lex, TOK(static))) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
        SyntaxError(syntax, "static lambda requires C++23");
      }
      *is_static = true;
    } else if (LexMatch(syntax->lex, TOK(constexpr))) {
      *is_constexpr = true;
    } else if (LexMatch(syntax->lex, TOK(consteval))) {
      *is_consteval = true;
      *is_constexpr = true;
    } else if (LexLookingAt(syntax->lex, TOK(noexcept))) {
      *is_noexcept = SkipNoexceptSpecifier(syntax, followers);
    } else {
      keep_parsing = false;
    }
  }
  if (*is_static && *is_mutable) {
    SyntaxError(syntax, "static lambda cannot be mutable");
  }
  if (*is_static && func->info.function.has_explicit_object_parameter) {
    SyntaxError(syntax,
                "static lambda cannot have an explicit object parameter");
  }

  TypeRecord* return_type = default_type;
  if (LexMatch(syntax->lex, TOK(arrow))) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(auto), kParsingPrototype);
    return_type = TypeParserParseType(&parser, true);
    Symbol* declarator = TypeParserParseDeclarator(&parser, return_type);
    if (declarator != NULL) {
      TypeRecord* parsed_type = TypeRecordCopy(declarator->type);
      SymbolDelete(declarator);
      TypeRecordDelete(return_type);
      return_type = parsed_type;
    }
    TypeParserDestruct(&parser);
  }
  ParseLambdaTrailingRequiresClause(syntax, func);
  SyntaxParseFunctionContracts(syntax, func, NULL, false);
  return return_type;
}

// Create the synthesized, uniquely-named closure class for a lambda and
// register it as a tag.  Capture fields are added later; for now it carries a
// single private placeholder member so it has non-zero size before captures are
// known.  Returns the tag symbol and outputs its struct type in `closure_type`.
static Symbol* NewLambdaClosureTag(Syntax* syntax, SourceLocation location,
                                   TypeRecord** closure_type) {
  String tag_name;
  StringInit(&tag_name, NULL);
  SyntaxFakeTagName(syntax, &tag_name);

  Struct* closure = NewStruct(false);
  closure->is_class = true;
  TypeRecord* type = NewTypeRecord(kTypeStruct, kQualPlain);
  TypeRecordSetStructInfo(type, closure);
  Symbol* tag = NewSymbol(tag_name.value, type, STO(implicit));
  tag->flags.invented = true;
  tag->flags.is_defined = true;
  tag->location = location;
  closure->tag_name = &tag->name;
  closure->tag_symbol = tag;
  SyntaxAddTag(syntax, tag);
  Symbol* empty_member =
      NewSymbol("__lambda_empty", NewTypeRecordWithSize(kTypeChar, kQualPlain),
                STO(implicit));
  empty_member->flags.invented = true;
  empty_member->flags.is_defined = true;
  empty_member->location = location;
  StructMember* member = NewStructMember(empty_member);
  member->access = kAccessPrivate;
  StructAddSyntheticMember(closure, member);
  TypeRecordCalculateSize(type);
  StringDestruct(&tag_name);
  *closure_type = type;
  return tag;
}

// Build the closure class's `operator()` from the lambda's parameter list,
// specifiers and return type, and add it as a public member function.  Unless
// the lambda is `mutable`, the operator is const-qualified.  The implicit
// `this` parameter is what later lets the body reach capture fields.
static Symbol* NewLambdaCallOperator(Syntax* syntax, TypeRecord* closure_type,
                                     TypeRecord* return_type, bool is_mutable,
                                     Vector* explicit_template_params,
                                     SourceLocation location,
                                     bool* out_is_static) {
  Struct* closure = closure_type->info.struct_info;
  TypeRecord* func = NewFunctionTypeRecord();
  // A C++20 explicit template-parameter-list (`[]<class T>(...)`) precedes the
  // parameters; its parameters lead the operator()'s template parameters so
  // any abbreviated `auto` parameters that follow are numbered after them.
  if (explicit_template_params != NULL) {
    for (size_t i = 0; i < explicit_template_params->length; i++) {
      VectorAppend(&func->info.function.template_parameters,
                   explicit_template_params->value.p[i]);
    }
  }
  bool has_parameter_list =
      ParseLambdaParameterList(syntax, func, TC(closebra));
  if (!has_parameter_list &&
      !CompilerCXXAtLeast(kLanguageStandardCXX23) &&
      !LexLookingAt(syntax->lex, TOK(lbrace))) {
    SyntaxError(syntax,
                "lambda specifiers without a parameter list require C++23");
  }
  bool is_static = false;
  bool is_constexpr = false;
  bool is_consteval = false;
  bool is_noexcept = false;
  return_type = ParseLambdaSpecifiersAndReturnType(
      syntax, func, &is_mutable, &is_static, &is_constexpr, &is_consteval,
      &is_noexcept, return_type, TC(closebra));
  func->info.function.is_const_member =
      !func->info.function.has_explicit_object_parameter && !is_mutable &&
      !is_static;
  // A closure's call operator is a constexpr function whenever it satisfies the
  // constexpr requirements, whether or not `constexpr` was written
  // ([expr.prim.lambda.closure]/4).  Bodies that do not satisfy them are only
  // diagnosed where a constant expression is actually required, so marking
  // every call operator constexpr costs nothing and lets closures be invoked
  // during constant evaluation.
  func->info.function.is_constexpr = true;
  func->info.function.is_consteval = is_consteval;
  func->info.function.is_noexcept = is_noexcept;
  TypeRecordChain(func, return_type);
  if (func->info.function.has_explicit_object_parameter) {
    func->info.function.cxx_member_owner = closure;
    if (is_mutable) {
      SyntaxError(syntax,
                  "lambda with an explicit object parameter cannot be mutable");
    }
  } else if (!is_static) {
    TypeRecordAddCXXThisParameter(func, closure, location);
  } else {
    func->info.function.cxx_member_owner = closure;
  }

  Symbol* op = NewSymbol("operator()", func, STO(implicit));
  op->location = location;
  op->flags.is_defined = true;
  op->flags.is_inline_defn = true;
  if (func->info.function.template_parameters.length > 0) {
    op->flags.is_template = true;
    func->info.function.template_parameter_count =
        (int)func->info.function.template_parameters.length;
    // Generic lambda inside another template: own auto params are numbered
    // after the enclosing template's; record that offset as the call
    // operator's template-parameter base for deduction/substitution.
    func->info.function.template_parameter_base =
        syntax->current_template_parameter_count;
  }
  op->value.func_defn = op;
  func->info.function.symbol = op;
  func->info.function.is_inline = true;
  func->info.function.definition = true;

  StructMember* member = NewStructMember(op);
  member->is_member_function = true;
  member->is_static = is_static;
  member->access = kAccessPublic;
  StructAddSyntheticMember(closure, member);
  *out_is_static = is_static;
  return op;
}

// State threaded through CollectDefaultLambdaCaptures while scanning the body
// for identifiers that the default capture mode must implicitly capture.
typedef struct {
  Vector* captures;
  TypeRecord* lambda_func;
  LambdaCaptureDefault capture_default;
  Vector* body_locals;  // Symbols declared in this lambda body (not capturable).
} LambdaCaptureScan;

// Collect symbols introduced by variable declarations in the lambda body so
// default-capture scanning does not treat them as enclosing-scope captures
// (e.g. `[&]{ auto inner = []{}; return inner(); }` must not capture `inner`).
static void CollectLambdaBodyLocalSymbols(ASTNode* node, void* data,
                                          int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  // Locals declared inside a nested lambda belong to that nested closure.
  // Its body is not under this compound-literal subtree; only skip walking
  // further when we are at the nested lambda expression itself if needed.
  // Nested lambda bodies are attached to their call operators, not here.
  if (node->op != AST_OP(vardecl)) {
    return;
  }
  VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
  if (decl->symbol != NULL) {
    VectorAppend((Vector*)data, decl->symbol);
  }
}

static bool LambdaBodyDeclaresSymbol(Vector* body_locals, Symbol* symbol) {
  if (body_locals == NULL || symbol == NULL) {
    return false;
  }
  for (size_t i = 0; i < body_locals->length; i++) {
    if (body_locals->value.p[i] == symbol) {
      return true;
    }
  }
  return false;
}

// Visitor: for a `[=]`/`[&]` lambda, append a capture for each enclosing-scope
// identifier referenced in the body that is not already captured.
static void CollectDefaultLambdaCaptures(ASTNode* node, void* data,
                                         int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  // Nested lambda-expressions are compound literals whose capture initializers
  // may still name enclosing locals (e.g. `[&]{ auto i = [&]{ return x; }; }`).
  // Walk those initializers so this lambda captures `x`; do not treat the
  // nested closure's own body locals as capturable (handled via body_locals).
  if (node->op != AST_OP(identifier)) {
    return;
  }
  LambdaCaptureScan* scan = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (!CanCaptureSymbol(id->symbol, scan->lambda_func) ||
      LambdaBodyDeclaresSymbol(scan->body_locals, id->symbol) ||
      FindLambdaCapture(scan->captures, id->symbol) != NULL) {
    return;
  }
  bool by_reference =
      scan->capture_default == kLambdaCaptureDefaultReference;
  // A default capture of a parameter pack is a pack capture: the closure field
  // must be marked as a pack so instantiation expands it to per-element fields
  // and so uses like `xs...` keep their pack-expansion marker after rewrite.
  bool is_pack = id->symbol->flags.is_parameter_pack;
  VectorAppend(scan->captures, NewLambdaCapture(id->symbol, by_reference,
                                                is_pack,
                                                /*initializer=*/NULL,
                                                /*is_init_capture=*/false));
}

// The closure-member type for a capture: a pointer to the captured object for
// by-reference captures, otherwise a copy of the captured (referent) type.
// Class-template specializations that are still represented as the primary
// plus concrete args (common for locals inside function templates) are
// materialized so member lookup on the capture sees instantiated members.
static TypeRecord* LambdaCaptureFieldType(LambdaCapture* capture) {
  TypeRecord* captured_type = TypeIsReference(capture->captured->type)
                                  ? capture->captured->type->next
                                  : capture->captured->type;
  captured_type = TypeMaterializeClassTemplateSpecialization(&compiler->syntax,
                                                             captured_type);
  if (capture->by_reference) {
    return NewPointerTo(kQualPlain, captured_type);
  }
  return TypeRecordCopy(captured_type);
}

// Add one private member to the closure class for each capture and record it on
// the capture, then recompute the closure's size now that all fields are known.
static void AddLambdaCaptureFields(TypeRecord* closure_type, Vector* captures,
                                   SourceLocation location) {
  Struct* closure = closure_type->info.struct_info;
  for (size_t i = 0; i < captures->length; i++) {
    LambdaCapture* capture = captures->value.p[i];
    if (capture->is_init_capture && capture->initializer != NULL) {
      capture->initializer = AnalyzeExpression(capture->initializer);
      if (capture->initializer->type != NULL && capture->captured != NULL) {
        TypeRecord* deduced = capture->initializer->type;
        if (TypeIsReference(deduced)) {
          deduced = deduced->next;
        }
        deduced = TypeMaterializeClassTemplateSpecialization(&compiler->syntax,
                                                             deduced);
        TypeRecordDelete(capture->captured->type);
        capture->captured->type = TypeRecordCopy(deduced);
      }
    }
    String field_name;
    StringInit(&field_name, capture->captured->name.value);
    if (capture->captured->flags.is_name_independent) {
      // Closure fields are an implementation detail.  Give placeholder
      // init-captures distinct internal field names so designated
      // initialization and capture rewriting cannot accidentally select a
      // different `_` field.
      StringPrintf(&field_name, "__placeholder_capture_%zu", i);
    }
    Symbol* field =
        NewSymbol(field_name.value, LambdaCaptureFieldType(capture),
                  STO(implicit));
    StringDestruct(&field_name);
    field->flags.invented = true;
    field->flags.is_defined = true;
    field->flags.is_parameter_pack = capture->is_pack_expansion;
    field->lambda_capture_source = capture->captured;
    field->lambda_capture_by_reference = capture->by_reference;
    field->location = location;
    StructMember* member = NewStructMember(field);
    member->access = kAccessPrivate;
    StructAddSyntheticMember(closure, member);
    capture->field = field;
  }
  if (captures->length > 0) {
    LambdaClosureRemoveEmptyPlaceholder(closure);
  }
  closure_type->size = closure->size;
  TypeRecordCalculateSize(closure_type);
}

// Build the designated initializer (`.field = value`) for one capture used when
// constructing the closure object: the captured value (or its address, for
// by-reference captures), or the init-capture's initializer expression.
static ASTNode* NewLambdaCaptureInitializer(LambdaCapture* capture,
                                            SourceLocation location) {
  ASTNode* value = capture->is_init_capture
                       ? capture->initializer
                       : NewIdentifierASTNode(capture->captured, location);
  if (capture->by_reference) {
    value = NewUnaryASTNode(AST_OP(address), NULL, location, value);
  } else if (capture->field != NULL &&
             TypeIsStructOrUnion(capture->field->type) &&
             (!capture->is_init_capture ||
              value->value_category != kValueCategoryPrvalue)) {
    // A by-value class capture is copy-initialized.  Preserve that semantic
    // operation in the aggregate-like closure initializer instead of letting
    // initializer flattening lower it to a byte copy.  The cast analyzer
    // selects the copy/move constructor, and designated-init code generation
    // constructs its result directly in the capture field.
    value = NewCastASTNode(capture->field->type, location, value);
  }
  if (capture->is_pack_expansion) {
    value->flags |= kASTPackExpansion;
  }
  Vector* designators = NewVector();
  VectorAppend(designators, NewStructDesignator(NewString(capture->field->name.value)));
  return NewDesignatedInitializerASTNode(
      designators, NewExpressionInitializerASTNode(value, location), location);
}

// Build the expression that reads a capture from inside the body: `this->field`
// for by-value captures, dereferenced (`*this->field`) for by-reference
// captures whose field is a pointer.  This replaces uses of the original name.
static ASTNode* NewLambdaCaptureAccess(Symbol* this_symbol,
                                       LambdaCapture* capture,
                                       SourceLocation location) {
  ASTNode* this_node = NewIdentifierASTNode(this_symbol, location);
  ASTNode* member = NewStringConstantASTNode(
      NewString(capture->field->name.value), NULL, location);
  ASTNode* access =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, this_node, member);
  if (capture->by_reference) {
    ASTNode* contents = NewUnaryASTNode(AST_OP(contents), NULL, location,
                                        access);
    if (capture->is_pack_expansion) {
      contents->flags |= kASTPackExpansion;
      access->flags |= kASTPackExpansion;
    }
    return contents;
  }
  if (capture->is_pack_expansion) {
    access->flags |= kASTPackExpansion;
  }
  return access;
}

// State threaded through RewriteLambdaCaptureUses: the captures to look up and
// the closure's `this` parameter through which their fields are reached.
typedef struct {
  Vector* captures;
  Symbol* this_symbol;
} LambdaRewrite;

// Make init-capture locals visible while parsing the body, since their names do
// not exist in the enclosing scope.
static void AddLambdaInitCaptureScopeSymbols(Syntax* syntax, Vector* captures) {
  for (size_t i = 0; captures != NULL && i < captures->length; i++) {
    LambdaCapture* capture = captures->value.p[i];
    if (capture != NULL && capture->is_init_capture &&
        capture->captured != NULL) {
      InsertLocalSymbol(syntax->local_symbol_stack, capture->captured);
    }
  }
}

// Replace child `child_id` of `parent` with `replacement` while rewriting a
// lambda body's capture uses.  Call-like VectorASTNodes use visitor child_ids
// where 0 is the callee (`left`) and argument N is visitor child_id N+1
// (vector slot N); map those explicitly.  All other node shapes go through the
// generic child-replacement helper.
static void ReplaceChildForLambdaCapture(ASTNode* parent, int child_id,
                                         ASTNode* replacement) {
  if (parent == NULL) {
    ASTNodeDelete(replacement);
    return;
  }
  // `inline_call` is call-like for visitors but is not a VectorASTNode; only
  // ordinary/builtin call nodes store the callee in `left` and args in
  // `children` with the 0 / N+1 numbering used here.
  if (ASTIsCallNode(parent) && parent->op != AST_OP(inline_call)) {
    VectorASTNode* vector = (VectorASTNode*)parent;
    if (child_id == 0) {
      ASTNode* old = vector->left;
      vector->left = replacement;
      replacement->parent = parent;
      replacement->child_id = 0;
      ASTNodeDelete(old);
      return;
    }
    ASTNode* old = vector->children->value.p[child_id - 1];
    VectorSet(vector->children, (size_t)child_id - 1, replacement);
    replacement->parent = parent;
    replacement->child_id = child_id;
    ASTNodeDelete(old);
    return;
  }
  ASTNodeReplaceChild(parent, child_id, replacement, true);
}

// Visitor: rewrite each identifier in the body that names a captured entity
// into the corresponding `this->field` access, so the body reads captures from
// the closure object rather than the (now out-of-scope) enclosing variables.
static void RewriteLambdaCaptureUses(ASTNode* node, void* data,
                                     int child_id, VisitorMode mode) {
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  if (node->parent != NULL && node->parent->op == AST_OP(reflect) &&
      ((ReflectionASTNode*)node->parent)->operand == node) {
    return;
  }
  LambdaRewrite* rewrite = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  LambdaCapture* capture = FindLambdaCapture(rewrite->captures, id->symbol);
  if (capture == NULL || capture->field == NULL) {
    return;
  }
  ASTNode* replacement =
      NewLambdaCaptureAccess(rewrite->this_symbol, capture, node->location);
  // Preserve a use-site pack-expansion marker (`xs...`) on the rewritten
  // capture access.  NewLambdaCaptureAccess already sets this for captures
  // recorded as pack expansions; OR it in for the use site as well so a
  // pack use cannot lose its expansion flag during rewrite.
  if ((node->flags & kASTPackExpansion) != 0) {
    replacement->flags |= kASTPackExpansion;
  }
  ReplaceChildForLambdaCapture(node->parent, child_id, replacement);
}

// Parse the lambda's compound-statement body as the closure operator()'s body,
// in a fresh block scope holding the parameters and any init-capture locals,
// then queue the operator for later definition.
static ASTNode* ParseLambdaBody(Syntax* syntax, Symbol* call_operator,
                                Vector* captures, TokenClass followers) {
  ParserContext old_context = syntax->context;
  syntax->context = kParsingBlockScope;
  // Nested lambdas inside a generic lambda must number their own `auto`
  // parameters after this operator()'s invented template parameters (which
  // themselves may already sit after an enclosing template).  Bump the
  // current count for the duration of the body so nested placeholders do not
  // collide with this operator's parameters.
  int old_template_parameter_count = syntax->current_template_parameter_count;
  TypeRecord* func = call_operator->type;
  if (func != NULL && TypeIsFunction(func) &&
      func->info.function.template_parameters.length > 0) {
    syntax->current_template_parameter_count =
        func->info.function.template_parameter_base +
        (int)func->info.function.template_parameters.length;
  }
  SyntaxOpenScope(syntax);
  AddLambdaFunctionScopeSymbols(syntax, call_operator->type);
  AddLambdaInitCaptureScopeSymbols(syntax, captures);
  ASTNode* body = SyntaxParseStatement(syntax, followers);
  SyntaxCloseScope(syntax);
  syntax->current_template_parameter_count = old_template_parameter_count;
  syntax->context = old_context;
  call_operator->type->info.function.body = body;
  VectorAppend(&compiler->declaration_asts, body);
  return body;
}

// Build the braced initializer that constructs the closure object, one
// designated `.field = value` initializer per capture.
static ASTNode* NewLambdaClosureInitializer(TypeRecord* closure_type,
                                            Vector* captures,
                                            SourceLocation location) {
  Vector* initializers = NewVector();
  for (size_t i = 0; i < captures->length; i++) {
    VectorAppend(initializers,
                 NewLambdaCaptureInitializer(captures->value.p[i], location));
  }
  return NewBracedInitializerASTNode(initializers, closure_type, location);
}

// Parse a C++ lambda `[captures](params) specifiers -> ret { body }` and lower
// it to an anonymous closure class.  The steps are:
//   1. parse the capture list and default capture mode;
//   2. synthesize the closure class and its operator();
//   3. parse the body (so identifier uses are resolved against the enclosing
//      scope) and, for a default capture mode, scan it for implicit captures;
//   4. add a closure field per capture and rewrite capture uses in the body to
//      `this->field` accesses;
//   5. produce a compound literal that constructs a temporary closure object,
//      brace-initialized from the captured values.
// Returns NULL when the next token does not begin a lambda.
static ASTNode* ParseCXXLambdaExpression(Syntax* syntax,
                                         TokenClass followers) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(lsquare))) {
    return NULL;
  }
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);
  Vector captures;
  VectorInit(&captures);
  LambdaCaptureDefault capture_default;
  ParseLambdaCaptureList(syntax, &captures, &capture_default, followers);

  TypeRecord* closure_type = NULL;
  NewLambdaClosureTag(syntax, location, &closure_type);

  // C++20 generic lambda with an explicit template-parameter-list:
  // `[captures]<template-params>(params)...`.  The parameters must be in scope
  // for the parameter list, trailing return type, requires-clause and body, so
  // open a scope spanning all of them and close it after the body is parsed.
  Vector* explicit_template_params = NULL;
  bool opened_template_scope = false;
  if (CompilerCXXAtLeast(kLanguageStandardCXX20) &&
      LexLookingAt(syntax->lex, TOK(less))) {
    SyntaxOpenScope(syntax);
    opened_template_scope = true;
    explicit_template_params = SyntaxParseTemplateParameterListWithBase(
        syntax, syntax->current_template_parameter_count);
  }

  // Without a trailing return type a lambda's return type is deduced from its
  // body ([expr.prim.lambda.closure]/4), exactly like an `auto`-returning
  // function, so hand operator() an `auto` placeholder and let the ordinary
  // return-statement deduction fill it in.
  bool is_static = false;
  Symbol* call_operator =
      NewLambdaCallOperator(syntax, closure_type,
                            NewTypeRecord(kTypeAuto, kQualPlain),
                            /*is_mutable=*/false, explicit_template_params,
                            location, &is_static);
  if (is_static &&
      (captures.length > 0 ||
       capture_default != kLambdaCaptureDefaultNone)) {
    SyntaxError(syntax, "static lambda cannot have captures");
  }
  TypeRecord* enclosing_function = compiler->current_function;
  compiler->current_function = call_operator->type;
  syntax->parsing_lambda_body_depth++;
  ParseLambdaBody(syntax, call_operator, &captures, followers);
  syntax->parsing_lambda_body_depth--;
  compiler->current_function = enclosing_function;
  if (opened_template_scope) {
    // The TemplateParameter objects were transferred into the operator()'s
    // template-parameter list; free only the vector container here.
    VectorDelete(explicit_template_params);
    SyntaxCloseScope(syntax);
  }
  if (!is_static && capture_default != kLambdaCaptureDefaultNone) {
    Vector body_locals;
    VectorInit(&body_locals);
    ASTNodeVisit(call_operator->type->info.function.body,
                 CollectLambdaBodyLocalSymbols, 0, &body_locals);
    LambdaCaptureScan scan = {&captures, call_operator->type, capture_default,
                              &body_locals};
    ASTNodeVisit(call_operator->type->info.function.body,
                 CollectDefaultLambdaCaptures, 0, &scan);
    VectorDestruct(&body_locals);
  }
  AddLambdaCaptureFields(closure_type, &captures, location);
  AddImplicitLambdaClosureSpecialMembers(
      syntax, closure_type->info.struct_info, closure_type->info.struct_info->tag_symbol,
      captures.length > 0, explicit_template_params != NULL);
  if (!is_static && captures.length > 0) {
    LambdaRewrite rewrite = {
        &captures, call_operator->type->info.function.prototype.value.p[0]};
    ASTNodeVisit(call_operator->type->info.function.body,
                 RewriteLambdaCaptureUses, 0, &rewrite);
  }

  // Queue operator() for analysis/codegen unless the closure captures a
  // non-pack value whose type still names an enclosing template parameter.
  // Those bodies cannot be analyzed against placeholder capture-field types;
  // SubstituteNestedStructTemplateParameters rebuilds the closure per
  // instantiation and CloneInstantiatedMemberFunctionBody re-queues the body.
  //
  // Pack-only captures are still queued eagerly: fold/pack-expansion lowering
  // (e.g. `[&]{ return (0 + ... + xs); }`) runs against the template-level
  // operator().  But if the same lambda also captures a dependent non-pack
  // (e.g. a visitor `vis` alongside a remaining pack `vars...`), defer: eager
  // analysis of the dependent capture fails, and pack expansion still runs on
  // the rebuilt per-instantiation body.
  bool defer_dependent_capture = false;
  if (syntax->current_template_parameter_count > 0) {
    for (size_t i = 0; i < captures.length; i++) {
      LambdaCapture* capture = captures.value.p[i];
      if (capture == NULL || capture->captured == NULL) {
        continue;
      }
      if (capture->is_pack_expansion) {
        continue;
      }
      if (TypeContainsTemplateParameter(capture->captured->type)) {
        defer_dependent_capture = true;
        break;
      }
    }
  }
  if (!defer_dependent_capture) {
    QueueLambdaCallOperatorDefinition(call_operator);
  }

  Symbol* temp = SyntaxNewTemporary(syntax, closure_type);
  temp->location = location;
  ASTNode* initializer =
      NewLambdaClosureInitializer(closure_type, &captures, location);
  ASTNode* result = NewCompoundLiteralASTNode(NewIdentifierASTNode(temp, location),
                                             location, initializer);
  // A lambda-expression is a prvalue that materializes a temporary closure.
  // Mark it so analysis does not treat the compound literal as a C lvalue
  // (which would break forwarding-reference deduction of `F&&` / `T&&`).
  result->flags |= kASTLambdaExpression;
  result->value_category = kValueCategoryPrvalue;
  VectorDestructWithContents(&captures, NULL, true);
  return result;
}

// primary-expression:
//   identifier
//   constant
//   string-literal
//   ( expression )
// True if `token` begins a fundamental (built-in) simple-type-specifier that
// can introduce a functional-style cast `T(...)` / `T{...}`.  Class/enum tags,
// cv-qualifiers, and `auto` are deliberately excluded: they never start a
// functional cast in an expression.
static bool TokenStartsFundamentalTypeSpecifier(Token token) {
  switch (token) {
    case TOK(char):
    case TOK(char8_t):
    case TOK(char16_t):
    case TOK(char32_t):
    case TOK(short):
    case TOK(int):
    case TOK(long):
    case TOK(float):
    case TOK(double):
    case TOK(signed):
    case TOK(unsigned):
    case TOK(bool):
    case TOK(void):
    case TOK(wchar_t):
      return true;
    default:
      return false;
  }
}

static void AppendTokenSequenceRawPiece(Lex* lex, Vector* tokens) {
  String spelling;
  StringInit(&spelling, NULL);
  LexCurrentTokenSpelling(lex, &spelling);
  TokenSequenceToken* piece = TokenSequenceTokenNew(
      lex->current_token, spelling.value, spelling.length,
      lex->current_token_location, NULL);
  piece->piece_kind = kTokenSequencePieceRaw;
  VectorAppend(tokens, piece);
  StringDestruct(&spelling);
}

static void ParseTokenSequenceInterpolator(Syntax* syntax, Vector* tokens,
                                           TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  LexNextToken(lex);

  if (LexLookingAt(lex, TOK(lparen))) {
    LexNextToken(lex);
    ASTNode* expr = SyntaxParseExpression(syntax, TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    TokenSequenceToken* piece = TokenSequenceTokenNew(
        TOK(injected_value), NULL, 0, location, expr);
    piece->piece_kind = kTokenSequencePieceTokenInterpolation;
    VectorAppend(tokens, piece);
    return;
  }

  if (LexLookingAt(lex, TOK(identifier)) &&
      StringEqual(&lex->spelling, "id")) {
    LexNextToken(lex);
    SyntaxNeedBracket(syntax, TOK(lparen), followers);
    Vector* args = NewVector();
    if (!LexLookingAt(lex, TOK(rparen))) {
      do {
        VectorAppend(args, ParseAssignmentExpression(syntax, TC(closebra)));
      } while (LexMatch(lex, TOK(comma)));
    }
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    ASTNode* arg_holder = NewVectorASTNode(AST_OP(token_sequence_id_args), NULL,
                                           location, NULL, args);
    TokenSequenceToken* piece = TokenSequenceTokenNew(
        TOK(injected_value), NULL, 0, location, arg_holder);
    piece->piece_kind = kTokenSequencePieceIdentifierInterpolation;
    VectorAppend(tokens, piece);
    return;
  }

  if (LexLookingAt(lex, TOK(identifier)) &&
      StringEqual(&lex->spelling, "tokens")) {
    LexNextToken(lex);
    SyntaxNeedBracket(syntax, TOK(lparen), followers);
    ASTNode* expr = SyntaxParseExpression(syntax, TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    TokenSequenceToken* piece = TokenSequenceTokenNew(
        TOK(injected_value), NULL, 0, location, expr);
    piece->piece_kind = kTokenSequencePieceTokensInterpolation;
    VectorAppend(tokens, piece);
    return;
  }

  SyntaxError(syntax,
              "expected '\\(', '\\id(', or '\\tokens(' after '\\' in token "
              "sequence literal");
}

static ASTNode* ParseTokenSequenceLiteral(Syntax* syntax, TokenClass followers,
                                          SourceLocation location) {
  Lex* lex = syntax->lex;
  if (!CompilerCXXAtLeast(kLanguageStandardCXX29)) {
    SyntaxError(syntax, "token sequence literals require C++29");
  }
  LexNextToken(lex);

  Vector tokens;
  VectorInit(&tokens);
  int brace_depth = 1;
  while (!LexEof(lex) && brace_depth > 0) {
    if (LexLookingAt(lex, TOK(lbrace))) {
      AppendTokenSequenceRawPiece(lex, &tokens);
      brace_depth++;
      LexNextToken(lex);
      continue;
    }
    if (LexLookingAt(lex, TOK(rbrace))) {
      brace_depth--;
      if (brace_depth == 0) {
        break;
      }
      AppendTokenSequenceRawPiece(lex, &tokens);
      LexNextToken(lex);
      continue;
    }
    if (LexLookingAt(lex, TOK(backslash))) {
      ParseTokenSequenceInterpolator(syntax, &tokens, followers);
      continue;
    }
    AppendTokenSequenceRawPiece(lex, &tokens);
    LexNextToken(lex);
  }

  if (brace_depth != 0) {
    SyntaxError(syntax, "unbalanced braces in token sequence literal");
    for (size_t i = 0; i < tokens.length; i++) {
      TokenSequenceTokenDelete(tokens.value.p[i]);
    }
    VectorDestruct(&tokens);
    SyntaxRecover(syntax, followers);
    return NewTokenSequenceLiteralASTNode(
        ReflectionCreateTokenSequence(NULL, location), location);
  }

  LexNextToken(lex);
  ReflectionValue* value = ReflectionCreateTokenSequence(&tokens, location);
  for (size_t i = 0; i < tokens.length; i++) {
    TokenSequenceTokenDelete(tokens.value.p[i]);
  }
  VectorDestruct(&tokens);
  return NewTokenSequenceLiteralASTNode(value, location);
}

static ASTNode* ParseNestedPrimaryExpression(Syntax* syntax,
                                             TokenClass followers) {
  Lex* lex = syntax->lex;

  if (LexLookingAt(lex, TOK(splice_open))) {
    SourceLocation location = lex->current_token_location;
    LexNextToken(lex);
    ASTNode* reflection =
        SyntaxParseExpression(syntax, TC(spliceclose));
    SyntaxNeedBracket(syntax, TOK(splice_close), followers);
    if (LexMatch(lex, TOK(coloncolon))) {
      if (!LexLookingAt(lex, TOK(identifier))) {
        SyntaxError(syntax, "Expected identifier after namespace splice");
        return NewSpliceQualifiedASTNode(
            reflection,
            NewIdentifierASTNode(
                NewSymbol(SyntaxFakeName(syntax),
                          NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                                kQualPlain),
                          STO(implicit)),
                location),
            location);
      }
      String* suffix_name = NewString(lex->spelling.value);
      LexNextToken(lex);
      TypeRecord* unknown =
          NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      Symbol* suffix_symbol = NewSymbol(suffix_name->value, unknown, STO(implicit));
      suffix_symbol->flags.invented = true;
      suffix_symbol->flags.is_forward_declared = true;
      ASTNode* suffix = NewIdentifierASTNode(suffix_symbol, location);
      return NewSpliceQualifiedASTNode(reflection, suffix, location);
    }
    return NewSpliceASTNode(reflection, kSpliceExpression, location);
  }

  if (LexLookingAt(lex, TOK(caret))) {
    LexCheckpoint caret_checkpoint;
    LexCheckpointSave(lex, &caret_checkpoint);
    LexNextToken(lex);
    if (LexLookingAt(lex, TOK(lbrace))) {
      LexCheckpointDestruct(&caret_checkpoint);
      SourceLocation location = caret_checkpoint.current_token_location;
      return ParseTokenSequenceLiteral(syntax, followers, location);
    }
    LexCheckpointRestore(lex, &caret_checkpoint);
    LexCheckpointDestruct(&caret_checkpoint);
  }

  // Check for parenthesized expression.
  // This either looks at the flag 'found_open_paren' in the Syntax
  // struct or looks for an open paren.  The syntax is slightly ambiguous
  // and the open paren can be the start of a cast expression or postfix
  // expression.
  if (syntax->found_open_paren || LexMatch(lex, TOK(lparen))) {
    syntax->found_open_paren = false;
    // A parenthesized sub-expression is a fresh top level: any '>' / '>>'
    // inside it is a relational/shift operator, not a template-list closer, so
    // suppress the template-argument context until the matching ')'.
    bool saved_parsing_template_argument = syntax->parsing_template_argument;
    syntax->parsing_template_argument = false;
    // GCC statement expression: ( { statements } ).  The value is that of the
    // last statement if it is an expression statement.
    if (LexLookingAt(lex, TOK(lbrace))) {
      ASTNode* compound = SyntaxParseStatement(syntax, followers | TC(closebra));
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      syntax->parsing_template_argument = saved_parsing_template_argument;
      return NewUnaryASTNode(AST_OP(stmt_expr), NULL,
                             syntax->lex->current_token_location, compound);
    }
    ASTNode* fold = TryParseCXXFoldExpression(syntax, followers);
    if (fold != NULL) {
      syntax->parsing_template_argument = saved_parsing_template_argument;
      return fold;
    }
    ASTNode* node = SyntaxParseExpression(syntax, followers | TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    syntax->parsing_template_argument = saved_parsing_template_argument;
    if (node != NULL) {
      node->flags |= kASTParenthesized;
    }
    return node;
  }

  // C11 _Generic selection (lexes as an identifier).
  if (CompilerCAtLeast(kLanguageStandardC11) &&
      LexLookingAt(lex, TOK(identifier)) &&
      StringEqual(&lex->spelling, "_Generic")) {
    return ParseGenericSelection(syntax, followers);
  }

  // C++ explicit type conversion in functional notation with a fundamental
  // simple-type-specifier: `int(x)`, `unsigned long(y)`, `bool{z}`, `double()`.
  // Named type specifiers (class/typedef names) arrive here as identifiers and
  // construct through the ordinary call path, so only the built-in keyword
  // specifiers need this branch.  A cv-qualifier or aggregate keyword
  // (const/class/struct/...) never begins a functional cast, so those keep
  // falling through to the normal type/declaration handling elsewhere.
  if (CompilerIsCXX() && TokenStartsFundamentalTypeSpecifier(lex->current_token)) {
    SourceLocation location = lex->current_token_location;
    TypeParser type_parser;
    TypeParserInit(&type_parser, lex, syntax, STO(implicit), syntax->context);
    TypeRecord* type = TypeParserParseType(&type_parser, true);
    TypeParserDestruct(&type_parser);
    bool brace_init = LexLookingAt(lex, TOK(lbrace));
    if (!brace_init && !LexLookingAt(lex, TOK(lparen))) {
      SyntaxError(syntax,
                  "expected '(' or '{' after type in functional-style cast");
      SyntaxRecover(syntax, followers);
      return NewIntConstantASTNode(0, type, location);
    }
    Token close = brace_init ? TOK(rbrace) : TOK(rparen);
    LexNextToken(lex);
    ASTNode* argument = NULL;
    if (!LexLookingAt(lex, close)) {
      argument = SyntaxParseSingleExpression(syntax, followers | TC(exprsep) |
                                                         TC(closebra));
      if (LexLookingAt(lex, TOK(comma))) {
        SyntaxError(syntax,
                    "a functional-style cast to a fundamental type takes at "
                    "most one argument");
        while (LexMatch(lex, TOK(comma))) {
          ASTNodeDelete(SyntaxParseSingleExpression(
              syntax, followers | TC(exprsep) | TC(closebra)));
        }
      }
    }
    SyntaxNeedBracket(syntax, close, followers);
    if (argument == NULL) {
      // Value-initialization of a fundamental type yields a zero-valued prvalue
      // of that type.
      return NewCastASTNode(
          type, location,
          NewIntConstantASTNode(
              0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
    }
    return NewCastASTNode(type, location, argument);
  }

  // C++23 placeholder conversion: `auto(expr)` / `auto{expr}`.  Unlike a
  // fundamental functional cast, placeholder deduction requires exactly one
  // assignment-expression and its target type is determined during semantic
  // analysis after the operand has a type.
  if (CompilerIsCXX() && LexLookingAt(lex, TOK(auto))) {
    SourceLocation location = lex->current_token_location;
    LexNextToken(lex);
    bool brace_init = LexLookingAt(lex, TOK(lbrace));
    if (!brace_init && !LexLookingAt(lex, TOK(lparen))) {
      SyntaxError(syntax,
                  "expected '(' or '{' after auto in functional-style cast");
      SyntaxRecover(syntax, followers);
      return NewIntConstantASTNode(
          0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
    }
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
      SyntaxError(syntax, "auto(x) and auto{x} require C++23");
    }
    Token close = brace_init ? TOK(rbrace) : TOK(rparen);
    LexNextToken(lex);
    ASTNode* argument = NULL;
    if (!LexLookingAt(lex, close)) {
      argument = SyntaxParseSingleExpression(syntax, followers | TC(exprsep) |
                                                         TC(closebra));
      if (LexLookingAt(lex, TOK(comma))) {
        SyntaxError(syntax,
                    "a functional-style cast to auto requires exactly one "
                    "argument");
        while (LexMatch(lex, TOK(comma))) {
          ASTNodeDelete(SyntaxParseSingleExpression(
              syntax, followers | TC(exprsep) | TC(closebra)));
        }
      }
    }
    SyntaxNeedBracket(syntax, close, followers);
    if (argument == NULL) {
      SyntaxError(syntax,
                  "a functional-style cast to auto requires exactly one "
                  "argument");
      argument = NewIntConstantASTNode(
          0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
    }
    ASTNode* result =
        NewCastASTNode(NewTypeRecord(kTypeAuto, kQualPlain), location, argument);
    ((CastASTNode*)result)->kind =
        brace_init ? kCastAutoBrace : kCastAutoParen;
    return result;
  }

  if (LexLookingAt(lex, TOK(injected_value))) {
    ASTNode* injected = LexCurrentInjectedValue(lex);
    SourceLocation location = lex->current_token_location;
    LexNextToken(lex);
    if (injected == NULL) {
      SyntaxError(syntax, "missing injected value during token replay");
      return NewIntConstantASTNode(
          0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
    }
    return ASTNodeClone(injected, IdentityCloneNode, NULL, NULL);
  }

  // Check for identifier.  In C++ an unqualified operator-function-id (e.g.
  // `operator+`) is also a valid primary expression naming a free operator
  // function, so route a leading `operator` keyword through the same identifier
  // path (SyntaxParseFullyQualifiedIdentifier already parses the operator name).
  if (LexLookingAt(lex, TOK(identifier)) ||
      LexLookingAt(lex, TOK(coloncolon)) ||
      (CompilerIsCXX() && LexLookingAt(lex, TOK(operator)))) {
    return ParseIdentifier(syntax, followers);
  }

  if (LexLookingAt(lex, TOK(this))) {
    return ParseThisExpression(syntax);
  }

  if (LexLookingAt(lex, TOK(nullptr))) {
    LexNextToken(lex);
    TypeRecord* type = NewTypeRecordWithSize(kTypeNullPointer, kQualPlain);
    return NewIntConstantASTNode(0, type, syntax->lex->current_token_location);
  }

  if (LexLookingAt(lex, TOK(true)) || LexLookingAt(lex, TOK(false))) {
    bool value = LexLookingAt(lex, TOK(true));
    SourceLocation location = lex->current_token_location;
    LexNextToken(lex);
    return NewIntConstantASTNode(value ? 1 : 0,
                                 NewTypeRecordWithSize(kTypeBool, kQualPlain),
                                 location);
  }

  if (CompilerCXXAtLeast(kLanguageStandardCXX20) &&
      LexLookingAt(lex, TOK(requires))) {
    SourceLocation location = lex->current_token_location;
    ConstraintExpr* constraint = ConceptsParseRequiresExpression(syntax);
    if (ConceptsConstraintContainsTemplateParameter(constraint)) {
      return NewRequiresExpressionASTNode(constraint, location);
    }
    int64_t value = 0;
    bool ok = constraint != NULL &&
              ConceptsEvaluateConstraint(constraint, &value);
    ConstraintExprDelete(constraint);
    return NewIntConstantASTNode(ok && value != 0 ? 1 : 0,
                                 NewTypeRecordWithSize(kTypeBool, kQualPlain),
                                 location);
  }

  ASTNode* lambda = ParseCXXLambdaExpression(syntax, followers);
  if (lambda != NULL) {
    return lambda;
  }

  // Check for integer constant.
  if (LexLookingAt(lex, TOK(number))) {
    if (lex->ud_suffix.length != 0) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX23) &&
          IsCXXSizeLiteralSuffix(&lex->ud_suffix)) {
        SyntaxError(syntax, "size_t literal suffix requires C++23");
      } else if (CompilerCXXAtLeast(kLanguageStandardCXX23)) {
        char first =
            toupper((unsigned char)lex->ud_suffix.value[0]);
        char second =
            lex->ud_suffix.length > 1
                ? toupper((unsigned char)lex->ud_suffix.value[1])
                : '\0';
        if (first == 'Z' || (first == 'U' && second == 'Z')) {
          SyntaxError(syntax, "invalid size_t literal suffix");
        }
      }
      return ParseCXXUserDefinedIntegerLiteral(syntax);
    }
    return ParseIntegerConstant(syntax, followers);
 }

  // Check for floating point constant.
  if (LexLookingAt(lex, TOK(fnumber))) {
    if (lex->ud_suffix.length != 0) {
      return ParseCXXUserDefinedFloatingLiteral(syntax);
    }
    return ParseFloatingPointConstant(syntax, followers);
  }

  // Check for string literal.
  if (LexLookingAt(lex, TOK(string))) {
    return ParseStringLiteral(syntax, followers);
  }

  // Wide string literal
  if (LexLookingAt(lex, TOK(string_wide))) {
    return ParseWideStringLiteral(syntax, followers);
   }

  // Character constant.
  if (LexLookingAt(lex, TOK(charconst))) {
    return ParseCharacterConstant(syntax, followers);
  }
  
  // Wide character constant.
  if (LexLookingAt(lex, TOK(charconst_wide))) {
    return ParseWideCharacterConstant(syntax, followers);
  }

  // Invalid primary expression, error out, recover and return 0.
  SyntaxError(syntax, "Expression syntax error; primary expression expected");
  SyntaxRecover(syntax, followers);
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  return (ASTNode*)NewIntConstantASTNode(0, type,
                                         syntax->lex->current_token_location);
}

// Every operand that is itself an expression -- the body of a parenthesized
// subexpression, an argument, a subscript -- is parsed by recursive descent, so
// each level of nesting costs stack whose amount the parser cannot observe.
// Bound the nesting instead: the limit is far above both what the language
// requires (63 levels in C, 256 in C++) and what any real program contains, and
// past it the input is rejected rather than the compiler crashing.
#define kMaxExpressionNesting 512

// Discard the expression that is nested too deeply to parse, keeping the
// brackets balanced.  Ordinary recovery would leave the rest of it in front of
// the levels already on the stack, and each would report and rescan the same
// input, so the cost of refusing one expression grew with the square of its
// length.  Stopping before a closing bracket this scan did not open, or before a
// separator that cannot appear inside an operand, hands each enclosing level the
// token it was waiting for instead.
static void SkipOverdeepExpression(Syntax* syntax) {
  Lex* lex = syntax->lex;
  int depth = 0;
  while (!LexEof(lex)) {
    switch (lex->current_token) {
      case TOK(lparen):
      case TOK(lsquare):
      case TOK(lbrace):
        depth++;
        break;
      case TOK(rparen):
      case TOK(rsquare):
      case TOK(rbrace):
        if (depth == 0) {
          return;
        }
        depth--;
        break;
      case TOK(comma):
      case TOK(semicolon):
      case TOK(colon):
        if (depth == 0) {
          return;
        }
        break;
      default:
        break;
    }
    LexNextToken(lex);
  }
}

static ASTNode* ParsePrimaryExpression(Syntax* syntax, TokenClass followers) {
  if (syntax->expression_nesting_depth >= kMaxExpressionNesting) {
    SyntaxError(syntax, "Expression nests more than %d levels deep",
                kMaxExpressionNesting);
    SkipOverdeepExpression(syntax);
    return NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain),
        syntax->lex->current_token_location);
  }
  syntax->expression_nesting_depth++;
  ASTNode* result = ParseNestedPrimaryExpression(syntax, followers);
  syntax->expression_nesting_depth--;
  return result;
}

// Check if we have a varargs intrinsic.
static ASTNode* VarargsIntrinsic(Syntax* syntax, ASTNode* left,
                               TokenClass followers) {
  if (left->op == AST_OP(identifier)) {
    IdentifierASTNode* id_node = (IdentifierASTNode*)left;
    const char* name = id_node->symbol->name.value;

    const struct Intrinsic* intrinsic = GetIntrinsic(name);
    if (intrinsic == NULL) {
      return NULL;
    }

    Vector* actuals = NewVector();
    while (!LexLookingAt(syntax->lex, TOK(rparen))) {
      ASTNode* actual;
      // Special case: __builtin_va_arg has a type as its second arg.
      if (intrinsic->opcode == AST_OP(builtin_va_arg) &&
          actuals->length == 1) {
        TypeParser parser;
        TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), kParsingBlockScope);
        TypeRecord* type = TypeParserParseType(&parser, true);
        Symbol* sym = TypeParserParseDeclarator(&parser, type);
        type = sym->type;
        actual =
            NewIntConstantASTNode(0, type, syntax->lex->current_token_location);
        SymbolDelete(sym);  // Don't need this.
        TypeParserDestruct(&parser);
      } else {
        actual = SyntaxParseSingleExpression(syntax, followers | TC(exprsep));
      }
      VectorAppend(actuals, actual);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
    }
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    bool valid_arity =
        intrinsic->opcode == AST_OP(builtin_prefetch)
            ? actuals->length >= 1 && actuals->length <= 3
            : actuals->length == (size_t)intrinsic->num_args;
    if (!valid_arity) {
      if (intrinsic->opcode == AST_OP(builtin_prefetch)) {
        SyntaxError(
            syntax,
            "Wrong number of args for builtin; expected between 1 and 3, got %zd",
            actuals->length);
      } else {
      SyntaxError(
          syntax,
          "Wrong number of args for builtin; expected %d, got %zd",
          intrinsic->num_args, actuals->length);
      }
    }
    return NewVectorASTNode(intrinsic->opcode, NULL,
                            syntax->lex->current_token_location, left, actuals);
  }
  return NULL;
}

// Parse an array subscript expression.
static ASTNode* ParseArraySubscript(ASTNode* left, Syntax* syntax,
                                    TokenClass followers) {
  if (CompilerIsCXX() &&
      CompilerCXXAtLeast(kLanguageStandardCXX23)) {
    Vector* indices = NewVector();
    while (!LexLookingAt(syntax->lex, TOK(rsquare))) {
      ASTNode* index = SyntaxParseSingleExpression(
          syntax, followers | TC(exprsep) | TC(closebra));
      MarkCXXPackExpansionIfPresent(syntax, index);
      VectorAppend(indices, index);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
    }
    SyntaxNeedBracket(syntax, TOK(rsquare), followers);
    ASTNode* only_index =
        indices->length == 1 ? (ASTNode*)VectorGet(indices, 0) : NULL;
    if (only_index != NULL &&
        (only_index->flags & kASTPackExpansion) == 0) {
      ASTNode* index = only_index;
      VectorDelete(indices);
      return NewBinaryASTNode(AST_OP(subscript), NULL,
                              syntax->lex->current_token_location, left, index);
    }
    return NewVectorASTNode(AST_OP(subscript), NULL,
                            syntax->lex->current_token_location, left, indices);
  }
  ASTNode* index = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rsquare), followers);
  return NewBinaryASTNode(AST_OP(subscript), NULL,
                          syntax->lex->current_token_location, left, index);
}

// Parse a function call or varargs builtin.
static ASTNode* ParseFunctionCall(ASTNode* left, Syntax* syntax,
                                  TokenClass followers) {
  // Check for varargs intrinsic functions.
  ASTNode* varargs = VarargsIntrinsic(syntax, left, followers);
  if (varargs != NULL) {
    return varargs;
  }

  ASTNode* type_trait = TypeTraitIntrinsic(syntax, left, followers);
  if (type_trait != NULL) {
    return type_trait;
  }
  
  // Normal function call.
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, TOK(rparen))) {
    ASTNode* actual = LexMatch(syntax->lex, TOK(lbrace))
                          ? SyntaxParseBracedInitializer(syntax)
                          : SyntaxParseSingleExpression(syntax,
                                                        followers | TC(exprsep));
    MarkCXXPackExpansionIfPresent(syntax, actual);
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return NewVectorASTNode(AST_OP(call), NULL,
                            syntax->lex->current_token_location, left,
                            actuals);
}

static bool CXXExpressionNamesTemplateTypeParameter(ASTNode* node) {
  if (!CompilerIsCXX() || node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  return id->symbol != NULL && id->symbol->flags.is_template_parameter &&
         id->symbol->flags.is_template_type_parameter &&
         id->symbol->type != NULL;
}

static ASTNode* ParseCXXDependentValueInitialization(ASTNode* type_expr,
                                                     Syntax* syntax,
                                                     TokenClass followers) {
  // `T(args)` where T names a template type parameter is a dependent explicit
  // type conversion / value-initialization.  Represent it as an ordinary
  // functional-construction `call` node with the type-parameter identifier as
  // the callee (exactly as a non-dependent `Foo(args)` is represented).  Once
  // the enclosing template is instantiated and T is concrete,
  // AnalyzeCXXFunctionalClassConstruction resolves it correctly: a scalar T
  // yields a functional cast (`T()` -> value 0), while a class T is
  // constructed or value-initialized through its constructors.  (Previously
  // this produced a hardcoded `static_cast<T>(0)`, which was only valid when T
  // turned out to be scalar and made value-initialization of a class T --
  // e.g. an allocator default argument `Allocator()` -- an illegal cast.)
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, TOK(rparen))) {
    ASTNode* actual = LexMatch(syntax->lex, TOK(lbrace))
                          ? SyntaxParseBracedInitializer(syntax)
                          : SyntaxParseSingleExpression(
                                syntax, followers | TC(exprsep));
    MarkCXXPackExpansionIfPresent(syntax, actual);
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return NewVectorASTNode(AST_OP(call), NULL,
                          syntax->lex->current_token_location, type_expr,
                          actuals);
}

// Whether the object expression of a member access is type-dependent (i.e.
// involves a template parameter), in which case a member named with a trailing
// template-argument list needs the `template` disambiguator under the standard.
static bool MemberAccessObjectIsDependent(ASTNode* object) {
  if (object == NULL) {
    return false;
  }
  if (object->type != NULL && TypeContainsTemplateParameter(object->type)) {
    return true;
  }
  switch (object->op) {
    case AST_OP(identifier): {
      IdentifierASTNode* id = (IdentifierASTNode*)object;
      return id->symbol != NULL && id->symbol->type != NULL &&
             TypeContainsTemplateParameter(id->symbol->type);
    }
    case AST_OP(dot):
    case AST_OP(arrow):
      return MemberAccessObjectIsDependent(((BinaryASTNode*)object)->left);
    default:
      return false;
  }
}

static ASTNode* ParseStructMember(ASTNode* left, ASTOpcode op, Syntax* syntax,
                                  TokenClass followers) {
  if (LexLookingAt(syntax->lex, TOK(splice_open))) {
    SourceLocation location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    ASTNode* reflection =
        SyntaxParseExpression(syntax, TC(spliceclose));
    SyntaxNeedBracket(syntax, TOK(splice_close), followers);
    ASTNode* splice =
        NewSpliceASTNode(reflection, kSpliceMember, location);
    return NewBinaryASTNode(op, NULL, location, left, splice);
  }

  String* member_name;
  // Optional 'template' disambiguator in dependent member access, e.g.
  // `g.template onMessage<R>(...)` or `p->template get<0>()`.  When present the
  // member name must be a template-id, so its `<...>` is parsed as a template
  // argument list rather than a less-than comparison.
  bool saw_template_keyword =
      CompilerIsCXX() && LexMatch(syntax->lex, TOK(template));
  if (saw_template_keyword && LexLookingAt(syntax->lex, TOK(splice_open))) {
    SourceLocation splice_location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    ASTNode* reflection =
        SyntaxParseExpression(syntax, TC(spliceclose));
    SyntaxNeedBracket(syntax, TOK(splice_close), followers);
    ASTNode* splice =
        NewSpliceASTNode(reflection, kSpliceTemplate, splice_location);
    return NewBinaryASTNode(op, NULL, splice_location, left, splice);
  }
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    member_name = NewString(syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  } else if (!saw_template_keyword && CompilerIsCXX() &&
             LexLookingAt(syntax->lex, TOK(operator))) {
    // Explicit operator / conversion call, e.g. `x.operator+(y)`,
    // `x.operator()(y)`, `p->operator int()`.  Build the same member name the
    // operator/conversion function was registered under so the access resolves
    // to it.
    String op_name;
    if (SyntaxParseMemberOperatorName(syntax, &op_name)) {
      member_name = NewString(op_name.value);
      StringDestruct(&op_name);
    } else {
      SyntaxError(syntax, "Expected operator or conversion name");
      member_name = NewString(SyntaxFakeName(syntax));
    }
  } else if (!saw_template_keyword && CompilerIsCXX() &&
             LexMatch(syntax->lex, TOK(tilde))) {
    if (LexLookingAt(syntax->lex, TOK(identifier))) {
      member_name = NewString("~");
      StringAppend(member_name, syntax->lex->spelling.value);
      LexNextToken(syntax->lex);
    } else {
      SyntaxError(syntax, "Expected destructor name");
      member_name = NewString(SyntaxFakeName(syntax));
    }
  } else {
    SyntaxError(syntax, saw_template_keyword
                            ? "Expected template member name after 'template'"
                            : "Expected struct or union member name");
    member_name = NewString(SyntaxFakeName(syntax));
  }
  bool has_template_arguments = false;
  if (saw_template_keyword) {
    // The 'template' keyword guarantees a template-id, so a following '<'
    // unambiguously opens the template argument list.
    has_template_arguments = LexLookingAt(syntax->lex, TOK(less));
  } else if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(less))) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    int depth = 0;
    int paren_depth = 0;
    int square_depth = 0;
    int brace_depth = 0;
    // True once we see a token that cannot appear at the top level of a
    // template-argument list, which means this `<` is a less-than operator
    // rather than the start of a template-id.
    bool not_a_template_id = false;
    do {
      // A `;` or a `{`/`}` brace never appears at the top level of a
      // template-argument list.  Bailing out here is not just an optimization:
      // it stops the lookahead before it can run to the end of an #include'd
      // file, which (on EOF) frees the current source out from under the
      // checkpoint we are about to restore -- a use-after-free.
      if (LexLookingAt(syntax->lex, TOK(semicolon)) ||
          LexLookingAt(syntax->lex, TOK(lbrace)) ||
          LexLookingAt(syntax->lex, TOK(rbrace))) {
        not_a_template_id = true;
        break;
      }
      Token token = syntax->lex->current_token;
      if (token == TOK(lparen)) {
        paren_depth++;
      } else if (token == TOK(rparen)) {
        paren_depth--;
      } else if (token == TOK(lsquare)) {
        square_depth++;
      } else if (token == TOK(rsquare)) {
        square_depth--;
      } else if (token == TOK(lbrace)) {
        brace_depth++;
      } else if (token == TOK(rbrace)) {
        brace_depth--;
      } else if (paren_depth == 0 && square_depth == 0 &&
                 brace_depth == 0) {
        if (token == TOK(less)) {
          depth++;
        } else {
          depth -= LexClosingAngleCount(token);
        }
      }
      LexNextToken(syntax->lex);
    } while (depth > 0 && !LexEof(syntax->lex));
    has_template_arguments = !not_a_template_id && depth == 0 &&
                             LexLookingAt(syntax->lex, TOK(lparen));
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    // We resolved the `<` as a template-argument list on a type-dependent
    // object without the `template` disambiguator.  This is well-formed here
    // only thanks to a lookahead heuristic; the standard requires the keyword,
    // and other compilers reject it, so steer the user toward portable code.
    if (has_template_arguments && MemberAccessObjectIsDependent(left)) {
      SyntaxWarning(syntax, "missing-template-keyword",
                    "use 'template' keyword to treat '%s' as a dependent "
                    "template name (e.g. '%stemplate %s<...>')",
                    member_name->value,
                    op == AST_OP(arrow) ? "ptr->" : "obj.", member_name->value);
    }
  }
  Vector* template_arguments =
      has_template_arguments
          ? SyntaxParseTemplateArgumentList(syntax, followers)
          : NULL;
  ASTNode* member_node = NewStringConstantASTNode(member_name,
                                          NULL,
                                          syntax->lex->current_token_location);
  TypeRecord* receiver_type = left != NULL ? left->type : NULL;
  if (receiver_type != NULL && TypeIsReference(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (receiver_type != NULL && op == AST_OP(arrow) &&
      TypeIsPointer(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (receiver_type != NULL && TypeIsStructOrUnion(receiver_type) &&
      receiver_type->info.struct_info != NULL) {
    StructMember* member =
        FindStructMember(receiver_type->info.struct_info, member_name);
    if (member != NULL && member->symbol != NULL &&
        member->symbol->flags.name_independent_lookup_ambiguous) {
      member_node->flags |= kASTNameIndependentLookupAmbiguous;
    }
  }
  ((ConstantASTNode*)member_node)->template_arguments = template_arguments;
  return NewBinaryASTNode(op, NULL,
                          syntax->lex->current_token_location, left,
                          member_node);
}

static ASTNode* ParseCompoundLiteral(Syntax* syntax, TypeRecord* type) {
  SourceLocation location = syntax->lex->current_token_location;
  type = TypeRecordCalculateSize(type);
  Symbol* sym = SyntaxNewTemporary(syntax, type);
  ASTNode* initializer = SyntaxParseInitializer(syntax, sym, syntax->init_storage);
  return NewCompoundLiteralASTNode(NewIdentifierASTNode(sym, location),
                                   location, initializer);
}

static bool CXXPostfixExpressionNamesType(ASTNode* node) {
  if (!CompilerIsCXX() || node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  return id->symbol != NULL && StorageIs(id->symbol->storage, STO(typedef)) &&
         id->symbol->type != NULL && TypeIsStructOrUnion(id->symbol->type);
}

static Vector* ParseCXXBracedTemporaryActuals(Syntax* syntax, TypeRecord* type,
                                              TokenClass followers) {
  StructMember* ctor = FindCXXConstructorForType(type);
  if (CXXNewConstructorSetHasInitializerList(ctor)) {
    Vector* actuals = NewVector();
    LexNextToken(syntax->lex);
    VectorAppend(actuals, SyntaxParseBracedInitializer(syntax));
    return actuals;
  }
  return ParseCXXNewInitializerArguments(syntax, TOK(lbrace), followers);
}

static ASTNode* ParseCXXBracedTemporaryExpression(ASTNode* type_expr,
                                                  Syntax* syntax,
                                                  TokenClass followers) {
  bool dependent_type =
      CXXExpressionNamesTemplateTypeParameter(type_expr);
  if (!CXXPostfixExpressionNamesType(type_expr) && !dependent_type) {
    return NULL;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)type_expr;
  TypeRecord* type = id->symbol->type;
  SourceLocation location = type_expr->location;
  if (dependent_type) {
    Vector* actuals =
        ParseCXXNewInitializerArguments(syntax, TOK(lbrace), followers);
    return NewVectorASTNode(AST_OP(call), NULL, location, type_expr, actuals);
  }
  if (id->template_arguments != NULL ||
      TypeIsClassTemplatePlaceholder(type) ||
      FindCXXConstructorForType(type) != NULL) {
    Vector* actuals =
        ParseCXXBracedTemporaryActuals(syntax, type, followers);
    return NewVectorASTNode(AST_OP(call), NULL, location, type_expr, actuals);
  }
  return ParseCompoundLiteral(syntax, TypeRecordCopy(type));
}

// Parse a postfix-expression.  This is a primary expression with a postfixed
// operator.  The syntax is:

// postfix-expression:
//   primary-expression
//   postfix-expression [ expression ]
//   postfix-expression ( argument-expression-listopt )
//   postfix-expression . identifier
//   postfix-expression -> identifier
//   postfix-expression ++
//   postfix-expression --
//   ( type-name ) { initializer-list }
//   ( type-name ) { initializer-list , }

// argument-expression-list:
//   assignment-expression
//   argument-expression-list , assignment-expression

// Parse the trailing postfix operators ([], (), {}, ++, --, ., ->) of a
// postfix-expression, given an already-parsed primary `result`.  Shared by the
// ordinary postfix-expression parser and by named-cast parsing (a named cast
// such as `static_cast<T>(x)` is itself a postfix-expression and may be
// directly followed by `.member`, `[i]`, etc.).
static bool LookingAtCXXPackIndexSuffix(Syntax* syntax) {
  if (!CompilerIsCXX() ||
      !CompilerCXXAtLeast(kLanguageStandardCXX26) ||
      !LexLookingAt(syntax->lex, TOK(ellipsis))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result = LexLookingAt(syntax->lex, TOK(lsquare));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

static ASTNode* ParseCXXPackIndexExpression(Syntax* syntax, ASTNode* pack,
                                            TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  LexMatch(syntax->lex, TOK(ellipsis));
  LexMatch(syntax->lex, TOK(lsquare));
  ASTNode* index =
      SyntaxParseSingleExpression(syntax, followers | TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rsquare), followers);

  bool valid_pack = pack != NULL && pack->op == AST_OP(identifier) &&
                    (pack->flags & kASTParenthesized) == 0;
  if (valid_pack) {
    IdentifierASTNode* id = (IdentifierASTNode*)pack;
    valid_pack = id->symbol != NULL && id->symbol->flags.is_parameter_pack;
    if (valid_pack && id->symbol->flags.is_template_template_parameter) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX29)) {
        SyntaxError(syntax,
                    "Template-name pack indexing requires C++29");
      }
      if (LexLookingAt(syntax->lex, TOK(less))) {
        id->template_arguments =
            SyntaxParseTemplateArgumentList(syntax, followers);
      }
    }
  }
  if (!valid_pack) {
    SyntaxError(syntax,
                "pack indexing requires an unexpanded parameter pack name");
  }
  return NewBinaryASTNode(AST_OP(pack_index), NULL, location, pack, index);
}

static ASTNode* ParsePostfixOperators(Syntax* syntax, ASTNode* result,
                                      TokenClass followers) {
  while (!LexEof(syntax->lex)) {
    if (LookingAtCXXPackIndexSuffix(syntax)) {
      result = ParseCXXPackIndexExpression(syntax, result, followers);
    } else if (LexMatch(syntax->lex, TOK(lsquare))) {
      result = ParseArraySubscript(result, syntax, followers);
    } else if (LexMatch(syntax->lex, TOK(lparen))) {
      result = CXXExpressionNamesTemplateTypeParameter(result)
                   ? ParseCXXDependentValueInitialization(result, syntax,
                                                         followers)
                   : ParseFunctionCall(result, syntax, followers);
    } else if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(lbrace))) {
      ASTNode* braced =
          ParseCXXBracedTemporaryExpression(result, syntax, followers);
      if (braced == NULL) {
        break;
      }
      result = braced;
     } else if (LexMatch(syntax->lex, TOK(plusplus))) {
      result = NewUnaryASTNode(AST_OP(postinc), NULL,
                               syntax->lex->current_token_location, result);
    } else if (LexMatch(syntax->lex, TOK(minusminus))) {
      result = NewUnaryASTNode(AST_OP(postdec), NULL,
                               syntax->lex->current_token_location, result);
    } else if (LexMatch(syntax->lex, TOK(dot))) {
      result = ParseStructMember(result, AST_OP(dot), syntax,
                                 followers);
    } else if (CompilerIsCXX() &&
               LexLookingAt(syntax->lex, TOK(dotstar))) {
      if (MemberPointerOperatorPrecedesFoldEllipsis(syntax)) {
        break;
      }
      LexMatch(syntax->lex, TOK(dotstar));
      ASTNode* pm = ParseCastExpression(syntax, followers);
      result = NewBinaryASTNode(AST_OP(dotstar), NULL,
                                syntax->lex->current_token_location, result,
                                pm);
    } else if (LexMatch(syntax->lex, TOK(arrow))) {
      result = ParseStructMember(result, AST_OP(arrow), syntax,
                                 followers);
    } else if (CompilerIsCXX() &&
               LexLookingAt(syntax->lex, TOK(arrowstar))) {
      if (MemberPointerOperatorPrecedesFoldEllipsis(syntax)) {
        break;
      }
      LexMatch(syntax->lex, TOK(arrowstar));
      ASTNode* pm = ParseCastExpression(syntax, followers);
      result = NewBinaryASTNode(AST_OP(arrowstar), NULL,
                                syntax->lex->current_token_location, result,
                                pm);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParsePostfixExpression(Syntax* syntax, TokenClass followers) {
  // A compound literal looks exactly like a cast except it is followed
  // by an initializer (in braces).  We've already consumed the ( type-name )
  // and determined it's not a cast, so we can parse the
  if (syntax->compound_literal_type != NULL && LexLookingAt(syntax->lex, TOK(lbrace))) {
    TypeRecord* type = syntax->compound_literal_type;
    syntax->compound_literal_type = NULL;
    return ParseCompoundLiteral(syntax, type);
  }
  ASTNode* result = ParsePrimaryExpression(syntax, followers);
  if (syntax->lex->assembler_mode) {
    // No postfix expressions in assembler mode.
    return result;
  }
  return ParsePostfixOperators(syntax, result, followers);
}


static bool PreprocessorFeatureIsSupported(const char* name) {
  if (!CompilerIsCXX()) {
    return false;
  }
  if (strcmp(name, "cxx_exceptions") == 0) {
    return CompilerExceptionsEnabled();
  }
  if (strcmp(name, "cxx_rtti") == 0) {
    return true;
  }
  if (strcmp(name, "cxx_constexpr") == 0 ||
      strcmp(name, "cxx_lambdas") == 0 ||
      strcmp(name, "cxx_rvalue_references") == 0 ||
      strcmp(name, "cxx_variadic_templates") == 0) {
    return CompilerCXXAtLeast(kLanguageStandardCXX11);
  }
  if (strcmp(name, "cxx_concepts") == 0 ||
      strcmp(name, "cxx_coroutines") == 0 ||
      strcmp(name, "cxx_modules") == 0) {
    return CompilerCXXAtLeast(kLanguageStandardCXX20);
  }
  return false;
}

static int64_t CXXAttributeProbeValue(const char* attribute_namespace,
                                      const char* name) {
  if (attribute_namespace != NULL &&
      strcmp(attribute_namespace, "gnu") != 0) {
    return 0;
  }
  const char* canonical = name;
  if (strcmp(name, "nodiscard") == 0) {
    canonical = "warn_unused_result";
  } else if (strcmp(name, "maybe_unused") == 0) {
    canonical = "unused";
  }
  if (attribute_namespace != NULL) {
    return SyntaxAttributeIsSupported(canonical) ? 1 : 0;
  }
  if (strcmp(name, "noreturn") == 0) {
    return 200809L;
  }
  if (strcmp(name, "deprecated") == 0) {
    return 201309L;
  }
  if (strcmp(name, "fallthrough") == 0 ||
      strcmp(name, "maybe_unused") == 0) {
    return 201603L;
  }
  if (strcmp(name, "likely") == 0 || strcmp(name, "unlikely") == 0) {
    return 201803L;
  }
  if (strcmp(name, "nodiscard") == 0) {
    return 201907L;
  }
  if (strcmp(name, "indeterminate") == 0 &&
      CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    return 202403L;
  }
  return 0;
}

static int64_t CAttributeProbeValue(const char* attribute_namespace,
                                    const char* name) {
  if (!CompilerCAtLeast(kLanguageStandardC23)) {
    return 0;
  }
  if (attribute_namespace != NULL) {
    return strcmp(attribute_namespace, "gnu") == 0 &&
                   SyntaxAttributeIsSupported(name)
               ? 1
               : 0;
  }
  if (strcmp(name, "deprecated") == 0 ||
      strcmp(name, "fallthrough") == 0 ||
      strcmp(name, "maybe_unused") == 0) {
    return 201904L;
  }
  if (strcmp(name, "nodiscard") == 0) {
    return 202003L;
  }
  if (strcmp(name, "noreturn") == 0) {
    return 202202L;
  }
  if (strcmp(name, "unsequenced") == 0 ||
      strcmp(name, "reproducible") == 0) {
    return 202207L;
  }
  return 0;
}

static bool ParsePreprocessorProbeName(Syntax* syntax, String* name_space,
                                       String* name) {
  StringInit(name_space, NULL);
  StringInit(name, NULL);
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  String first;
  StringInit(&first, syntax->lex->spelling.value);
  LexNextToken(syntax->lex);
  if (LexMatch(syntax->lex, TOK(coloncolon))) {
    StringSetString(name_space, &first);
    if (LexLookingAt(syntax->lex, TOK(identifier))) {
      StringSetString(name, &syntax->lex->spelling);
      LexNextToken(syntax->lex);
    }
  } else {
    StringSetString(name, &first);
  }
  StringDestruct(&first);
  return name->length != 0;
}

static ASTNode* ParsePossiblePreprocessorFunction(Syntax* syntax,
                                                  TokenClass followers) {
  
  if (StringEqual(&syntax->lex->spelling, "defined")) {
    LexNextToken(syntax->lex);
    String macro_name;
    StringInit(&macro_name, NULL);
    if (LexMatch(syntax->lex, TOK(lparen))) {
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        StringSetString(&macro_name, &syntax->lex->spelling);
        LexNextToken(syntax->lex);
      }
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
    } else if (LexLookingAt(syntax->lex, TOK(identifier))) {
      StringSetString(&macro_name, &syntax->lex->spelling);
      LexNextToken(syntax->lex);
    }
    TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
    bool defined = PreprocessorMacroNameIsDefined(
        syntax->lex->preprocessor, &macro_name);
    StringDestruct(&macro_name);
    return NewIntConstantASTNode(defined ? 1 : 0, int_type,
                                 syntax->lex->current_token_location);
  } else if (StringEqual(&syntax->lex->spelling, "__has_feature")) {
    LexNextToken(syntax->lex);
    if (LexMatch(syntax->lex, TOK(lparen))) {
      bool value = false;
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        value = PreprocessorFeatureIsSupported(
            syntax->lex->spelling.value);
        LexNextToken(syntax->lex);
      }
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewIntConstantASTNode(value,
                                   NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                   syntax->lex->current_token_location);
    }
  } else if (StringEqual(&syntax->lex->spelling, "__has_builtin")) {
    LexNextToken(syntax->lex);
    if (LexMatch(syntax->lex, TOK(lparen))) {
      bool value = false;
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        value = IntrinsicAvailableOnCurrentTarget(
            GetIntrinsic(syntax->lex->spelling.value));
        LexNextToken(syntax->lex);
      }
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewIntConstantASTNode(value,
                                   NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                   syntax->lex->current_token_location);
    }
  } else if (StringEqual(&syntax->lex->spelling, "__has_attribute") ||
             StringEqual(&syntax->lex->spelling, "__has_cpp_attribute") ||
             (CompilerCAtLeast(kLanguageStandardC23) &&
              StringEqual(&syntax->lex->spelling, "__has_c_attribute"))) {
    bool cxx_attribute =
        StringEqual(&syntax->lex->spelling, "__has_cpp_attribute");
    bool c_attribute =
        StringEqual(&syntax->lex->spelling, "__has_c_attribute");
    LexNextToken(syntax->lex);
    if (LexMatch(syntax->lex, TOK(lparen))) {
      String name_space;
      String name;
      int64_t value = 0;
      if (ParsePreprocessorProbeName(syntax, &name_space, &name)) {
        if (c_attribute) {
          value = CAttributeProbeValue(
              name_space.length == 0 ? NULL : name_space.value, name.value);
        } else if (cxx_attribute) {
          value = CXXAttributeProbeValue(
              name_space.length == 0 ? NULL : name_space.value, name.value);
        } else if (name_space.length == 0) {
          value = SyntaxAttributeIsSupported(name.value);
        }
      }
      StringDestruct(&name_space);
      StringDestruct(&name);
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewIntConstantASTNode(value,
                                   NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                   syntax->lex->current_token_location);
    }
  } else if (StringEqual(&syntax->lex->spelling, "__has_include") ||
             StringEqual(&syntax->lex->spelling, "__has_include_next")) {
    bool include_next =
    StringEqual(&syntax->lex->spelling, "__has_include_next");
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(lparen))) {
      String filename;
      bool system_include = false;
      int64_t value = 0;
      if (PreprocessorParseIncludeFilename(
                                           syntax->lex->preprocessor,
                                           &syntax->lex->line,
                                           &syntax->lex->pos,
                                           &filename, &system_include)) {
        if (include_next) {
          value = PreprocessorHasIncludeNext(syntax->lex->preprocessor,
                                             &filename, system_include);
        } else {
          value = PreprocessorHasInclude(syntax->lex->preprocessor,
                                         &filename, system_include);
        }
      }
      LexNextToken(syntax->lex);
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewIntConstantASTNode(value,
                                   NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                   syntax->lex->current_token_location);
    }
  }
  return NULL;
}

static ASTNode* CloneVLASize(ASTNode* node, void* data) {
  return node;
}

// Expression that is the size of an array for a VLAThis is the
// size expression of the VLA multiplied by the size of its type.
// sizeof(t) * GetSizeofVLA(t->next)
static ASTNode* GetSizeofVLA(TypeRecord* type, SourceLocation location) {
  ASTNode* size = ASTNodeClone(type->info.array.size.vla.size,
                               CloneVLASize, NULL, NULL);
  size->location = location;
  TypeRecord* t = type->next;
  do {
    ASTNode* next_size;
    if (TypeIsVLA(t)) {
      next_size = ASTNodeClone(t->info.array.size.vla.size,
                               CloneVLASize, NULL, NULL);
    } else {
      next_size = NewIntConstantASTNode(t->size,
                                   NewTypeRecordWithSize(
                                                 kTypeLong |
                                                 kTypeUnsigned,
                                                 kQualPlain), location);
    }
    size = NewBinaryASTNode(AST_OP(mult), t, location, size, next_size);
    t = t->next;
  } while (t != NULL && TypeIsArray(t));
  return size;
}

static ASTNode* ParseSizeof(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(ellipsis))) {
    SyntaxNeedBracket(syntax, TOK(lparen), followers);
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected parameter pack name");
      SyntaxRecover(syntax, TC(closebra));
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewSizeofPackASTNode(
          NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                location),
          location);
    }
    Symbol* symbol = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
    if (symbol == NULL || !symbol->flags.is_parameter_pack) {
      SyntaxError(syntax, "sizeof... requires a parameter pack");
      LexNextToken(syntax->lex);
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewSizeofPackASTNode(
          NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                location),
          location);
    }
    ASTNode* id = NewIdentifierASTNode(symbol, syntax->lex->current_token_location);
    if (symbol->flags.name_independent_lookup_ambiguous) {
      id->flags |= kASTNameIndependentLookupAmbiguous;
    }
    LexNextToken(syntax->lex);
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    return NewSizeofPackASTNode(id, location);
  }
  bool has_brackets = LexMatch(syntax->lex, TOK(lparen));
  ASTNode* result = NULL;
  bool sizeof_type_name = false;
  if (SyntaxLookingAtType(syntax)) {
    if (!has_brackets) {
      SyntaxError(syntax,
                  "Parentheses expected around type name "
                  " in sizeof operator");
    }
    sizeof_type_name = true;
  }
  if (sizeof_type_name) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    int size = -1;
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym != NULL) {
      if (TypeIsVLA(sym->type)) {
        // The size of a VLA is its size expression.
        result = GetSizeofVLA(sym->type,
                              syntax->lex->current_token_location);
        SymbolDelete(sym);
        TypeParserDestruct(&parser);
        goto done;
      }
      if (CompilerIsCXX() && TypeContainsTemplateParameter(sym->type)) {
        // The size depends on a template parameter, so it cannot be computed
        // until the enclosing template is instantiated.  Retain the operand
        // type so it can be substituted and re-measured then.
        result = NewSizeofASTNodeWithType(
            sym->type, syntax->lex->current_token_location);
        SymbolDelete(sym);
        TypeParserDestruct(&parser);
        goto done;
      }
      size = sym->type->size;
      SymbolDelete(sym);
    }
    TypeParserDestruct(&parser);
    assert(size != -1);
    result = NewSizeofASTNodeWithKnownSize(size,
                                           syntax->lex->current_token_location);
  } else {
    if (has_brackets) {
      syntax->found_open_paren = true;
      has_brackets = false;
    }
    ASTNode* expr = ParseUnaryExpression(syntax, followers);
    result = NewSizeofASTNodeWithExpression(expr,
                                            syntax->lex->current_token_location);
  }
done:
  if (has_brackets) {
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
  }
  return result;
}

static ASTNode* ParseAlignof(Syntax* syntax, TokenClass followers) {
  bool has_brackets = LexMatch(syntax->lex, TOK(lparen));
  ASTNode* result = NULL;
  bool alignof_type_name = false;
  if (SyntaxLookingAtType(syntax)) {
    if (!has_brackets) {
      SyntaxError(syntax,
                  "Parentheses expected around type name "
                  " in alignof operator");
    }
    alignof_type_name = true;
  }
  if (alignof_type_name) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    int alignment = -1;
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym != NULL) {
      if (CompilerIsCXX() && TypeContainsTemplateParameter(sym->type)) {
        result = NewAlignofASTNodeWithType(
            sym->type, syntax->lex->current_token_location);
        SymbolDelete(sym);
        TypeParserDestruct(&parser);
        goto done;
      }
      TypeRecordCalculateSize(sym->type);
      alignment = TypeRecordAlignment(sym->type);
      SymbolDelete(sym);
    }
    TypeParserDestruct(&parser);
    assert(alignment != -1);
    result = NewAlignofASTNodeWithKnownAlignment(
        alignment, syntax->lex->current_token_location);
  } else {
    if (has_brackets) {
      syntax->found_open_paren = true;
      has_brackets = false;
    }
    ASTNode* expr = ParseUnaryExpression(syntax, followers);
    result = NewAlignofASTNodeWithExpression(
        expr, syntax->lex->current_token_location);
  }
done:
  if (has_brackets) {
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
  }
  return result;
}

// Parses a typeid operator: `typeid ( type-id )` or `typeid ( expression )`.
// Disambiguates via SyntaxLookingAtType, mirroring the sizeof type/expression
// split.  The resulting node is rewritten into a type_info access during
// semantic analysis.
static ASTNode* ParseTypeid(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* result;
  if (SyntaxLookingAtType(syntax)) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                   syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym != NULL && sym->type != NULL) {
      // Retain the declarator's type (sym is intentionally not deleted, like
      // the cast-expression type-name path).
      type = sym->type;
    }
    TypeParserDestruct(&parser);
    result = NewTypeidASTNodeWithType(type, location);
  } else {
    ASTNode* expr = SyntaxParseExpression(syntax, followers | TC(closebra));
    result = NewTypeidASTNodeWithExpression(expr, location);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return result;
}

static ASTNode* ParseReflectionExpression(Syntax* syntax,
                                          TokenClass followers,
                                          SourceLocation location) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    SyntaxError(syntax, "Reflection expressions require C++26");
  }

  if (SyntaxLookingAtCXXAttribute(syntax)) {
    Vector attributes;
    VectorInit(&attributes);
    SyntaxParseCXXAttributes(syntax, &attributes);
    ReflectionValue* value = NULL;
    if (!CompilerCXXAtLeast(kLanguageStandardCXX29)) {
      SyntaxError(syntax, "Attribute reflection requires C++29");
    } else if (attributes.length != 1) {
      SyntaxError(syntax,
                  "Attribute reflection requires exactly one attribute");
    } else {
      Attribute* attribute = attributes.value.p[0];
      const char* identifier = AttributeIdentifier(attribute);
      if (attribute != NULL && StringEqual(&attribute->name, "annotation")) {
        SyntaxError(syntax, "Annotations are not attribute reflections");
      } else if (identifier != NULL && strcmp(identifier, "assume") == 0) {
        SyntaxError(syntax, "The assume attribute cannot be reflected");
      } else if (!SyntaxAttributeIsReflectable(attribute)) {
        SyntaxError(syntax, "Attribute '%s' is not reflectable",
                    identifier != NULL ? identifier : "<unknown>");
      } else {
        value = ReflectionCreateAttribute(attribute, location);
      }
    }
    if (value == NULL) {
      value = ReflectionCreateInvalid(location);
    }
    AttributeListDestruct(&attributes);
    return NewReflectionConstantASTNode(value, location);
  }

  if (LexLookingAt(syntax->lex, TOK(coloncolon))) {
    LexCheckpoint global_checkpoint;
    LexCheckpointSave(syntax->lex, &global_checkpoint);
    LexNextToken(syntax->lex);
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      LexCheckpointDestruct(&global_checkpoint);
      return NewReflectionASTNode(kReflectionOperandGlobalNamespace, NULL, NULL,
                                  compiler->global_namespace, location);
    }
    LexCheckpointRestore(syntax->lex, &global_checkpoint);
    LexCheckpointDestruct(&global_checkpoint);
  }

  if (LexLookingAt(syntax->lex, TOK(identifier)) ||
      LexLookingAt(syntax->lex, TOK(coloncolon))) {
    bool parse_named_entity = false;
    LexCheckpoint namespace_checkpoint;
    LexCheckpointSave(syntax->lex, &namespace_checkpoint);
    FullyQualifiedIdentifier name;
    FullyQualifiedIdentifierInit(&name);
    if (SyntaxParseFullyQualifiedIdentifier(syntax, &name)) {
      Namespace* namespace_ = SyntaxFindQualifiedNamespace(syntax, &name);
      if (namespace_ != NULL) {
        FullyQualifiedIdentifierDestruct(&name);
        LexCheckpointDestruct(&namespace_checkpoint);
        return NewReflectionASTNode(kReflectionOperandNamespace, NULL, NULL,
                                    namespace_, location);
      }
      if (name.components.length >= 2) {
        Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
            syntax, &name, name.components.length - 1);
        if (owner != NULL && owner->type != NULL &&
            TypeIsStructOrUnion(owner->type)) {
          String* member_name =
              name.components.value.p[name.components.length - 1];
          StructMember* member = FindStructMember(
              owner->type->info.struct_info, member_name);
          if (member != NULL) {
            StructMemberASTNode* operand =
                (StructMemberASTNode*)NewStructMemberASTNode(member, location);
            operand->owner_type = TypeRecordCopy(owner->type);
            FullyQualifiedIdentifierDestruct(&name);
            LexCheckpointDestruct(&namespace_checkpoint);
            return NewReflectionASTNode(kReflectionOperandExpression,
                                        (ASTNode*)operand, NULL, NULL,
                                        location);
          }
        }
      }
      bool has_type_declarator =
          LexLookingAt(syntax->lex, TOK(star)) ||
          LexLookingAt(syntax->lex, TOK(amp)) ||
          LexLookingAt(syntax->lex, TOK(ampamp)) ||
          LexLookingAt(syntax->lex, TOK(lsquare)) ||
          LexLookingAt(syntax->lex, TOK(lparen));
      if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(less))) {
        Symbol* templ = SyntaxFindQualifiedSymbol(syntax, &name);
        FullyQualifiedIdentifierDestruct(&name);
        LexCheckpointRestore(syntax->lex, &namespace_checkpoint);
        LexCheckpointDestruct(&namespace_checkpoint);
        if (templ != NULL && templ->flags.is_template &&
            templ->type != NULL && TypeIsFunction(templ->type)) {
          ASTNode* operand = ParseIdentifier(syntax, followers);
          return NewReflectionASTNode(kReflectionOperandExpression, operand,
                                      NULL, NULL, location);
        }
        TypeParser parser;
        TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                       syntax->context);
        TypeRecord* type = TypeParserParseType(&parser, true);
        Symbol* sym = TypeParserParseDeclarator(&parser, type);
        if (sym != NULL && sym->type != NULL) {
          type = sym->type;
        }
        TypeParserDestruct(&parser);
        return NewReflectionASTNode(kReflectionOperandType, NULL, type, NULL,
                                    location);
      }
      parse_named_entity =
          !has_type_declarator &&
          SyntaxFindQualifiedSymbol(syntax, &name) != NULL;
    }
    FullyQualifiedIdentifierDestruct(&name);
    LexCheckpointRestore(syntax->lex, &namespace_checkpoint);
    LexCheckpointDestruct(&namespace_checkpoint);
    if (parse_named_entity) {
      ASTNode* operand = ParseIdentifier(syntax, followers);
      return NewReflectionASTNode(kReflectionOperandExpression, operand, NULL,
                                  NULL, location);
    }
  }

  if (SyntaxLookingAtType(syntax)) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                   syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym != NULL && sym->type != NULL) {
      type = sym->type;
    }
    TypeParserDestruct(&parser);
    return NewReflectionASTNode(kReflectionOperandType, NULL, type, NULL,
                                location);
  }

  if (LexLookingAt(syntax->lex, TOK(identifier)) ||
      LexLookingAt(syntax->lex, TOK(coloncolon)) ||
      LexLookingAt(syntax->lex, TOK(operator))) {
    if (LexLookingAt(syntax->lex, TOK(identifier))) {
      Namespace* alias_target = FindDirectLocalNamespaceAlias(
          syntax->local_symbol_stack, &syntax->lex->spelling);
      if (alias_target != NULL) {
        LexNextToken(syntax->lex);
        return NewReflectionASTNode(kReflectionOperandNamespaceAlias, NULL, NULL,
                                    alias_target, location);
      }
    }
    ASTNode* operand = ParseIdentifier(syntax, followers);
    return NewReflectionASTNode(kReflectionOperandExpression, operand, NULL,
                                NULL, location);
  }

  SyntaxError(syntax, "Expected a reflectable entity after '^^'");
  return NewReflectionASTNode(kReflectionOperandExpression, NULL, NULL, NULL,
                              location);
}

static Symbol* FindCXXAllocationFunctionByArgCount(Symbol* first,
                                                   size_t arg_count);

static Symbol* GetImplicitCXXAllocationFunction(const char* name,
                                                TypeRecord* return_type,
                                                TypeRecord* arg_type,
                                                SourceLocation location) {
  String symbol_name;
  StringInit(&symbol_name, name);
  Symbol* symbol = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  Symbol* existing = FindCXXAllocationFunctionByArgCount(symbol, 1);
  if (existing != NULL) {
    TypeRecordDelete(return_type);
    TypeRecordDelete(arg_type);
    return existing;
  }

  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, return_type);
  Symbol* formal = NewSymbol(SyntaxFakeName(&compiler->syntax), arg_type,
                             STO(auto));
  formal->flags.is_argument = true;
  formal->flags.invented = true;
  VectorAppend(&func_type->info.function.prototype, formal);

  symbol = NewSymbol(name, func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->location = location;
  func_type->info.function.symbol = symbol;
  SymbolSetCXXMangledAsmName(symbol);
  if (FindGlobalSymbol(&symbol->name) == NULL) {
    bool ok = InsertGlobalSymbol(symbol);
    assert(ok);
    (void)ok;
  } else {
    Symbol* first = FindGlobalSymbol(&symbol->name);
    Symbol* tail = first;
    while (tail->overload_next != NULL) {
      tail = tail->overload_next;
    }
    symbol->namespace_ = first->namespace_;
    tail->overload_next = symbol;
    first->flags.is_overloaded = true;
    symbol->flags.is_overloaded = true;
    SymbolSetCXXMangledAsmName(first);
    SymbolSetCXXMangledAsmName(symbol);
  }
  return symbol;
}

static Symbol* FindCXXAllocationFunctionByArgCount(Symbol* first,
                                                   size_t arg_count) {
  for (Symbol* symbol = first; symbol != NULL; symbol = symbol->overload_next) {
    if (symbol->type != NULL && TypeIsFunction(symbol->type) &&
        symbol->type->info.function.prototype.length == arg_count) {
      return symbol;
    }
  }
  return NULL;
}

static Symbol* FindCXXPlacementAllocationFunction(Symbol* first) {
  for (Symbol* symbol = first; symbol != NULL; symbol = symbol->overload_next) {
    if (symbol->type == NULL || !TypeIsFunction(symbol->type) ||
        symbol->type->info.function.prototype.length != 2) {
      continue;
    }
    Symbol* pointer_formal =
        symbol->type->info.function.prototype.value.p[1];
    if (pointer_formal != NULL && TypeIsPointer(pointer_formal->type) &&
        TypeIsVoid(pointer_formal->type->next)) {
      return symbol;
    }
  }
  return NULL;
}

static Symbol* FindGlobalCXXAllocationFunction(const char* name,
                                               size_t arg_count) {
  String symbol_name;
  StringInit(&symbol_name, name);
  Symbol* first = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  return FindCXXAllocationFunctionByArgCount(first, arg_count);
}

static Symbol* GetImplicitCXXOperatorNew(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* return_type = NewPointerTo(kQualPlain, void_type);
  return GetImplicitCXXAllocationFunction("operator new", return_type,
                                          NewSizeTypeRecord(), location);
}

static Symbol* GetImplicitCXXOperatorNewArray(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* return_type = NewPointerTo(kQualPlain, void_type);
  return GetImplicitCXXAllocationFunction("operator new[]", return_type,
                                          NewSizeTypeRecord(), location);
}

static Symbol* GetImplicitCXXPlacementOperatorNew(SourceLocation location) {
  String symbol_name;
  StringInit(&symbol_name, "operator new");
  Symbol* existing =
      FindCXXPlacementAllocationFunction(FindGlobalSymbol(&symbol_name));
  StringDestruct(&symbol_name);
  if (existing != NULL) {
    return existing;
  }

  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* return_type = NewPointerTo(kQualPlain, void_type);
  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, return_type);

  Symbol* size_formal = NewSymbol(SyntaxFakeName(&compiler->syntax),
                                  NewSizeTypeRecord(), STO(auto));
  size_formal->flags.is_argument = true;
  size_formal->flags.invented = true;
  VectorAppend(&func_type->info.function.prototype, size_formal);

  TypeRecord* ptr_type =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  Symbol* ptr_formal = NewSymbol(SyntaxFakeName(&compiler->syntax), ptr_type,
                                 STO(auto));
  ptr_formal->flags.is_argument = true;
  ptr_formal->flags.invented = true;
  VectorAppend(&func_type->info.function.prototype, ptr_formal);

  Symbol* symbol = NewSymbol("operator new", func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->location = location;
  func_type->info.function.symbol = symbol;
  SymbolSetCXXMangledAsmName(symbol);

  Symbol* first = FindGlobalSymbol(&symbol->name);
  if (first == NULL) {
    bool ok = InsertGlobalSymbol(symbol);
    assert(ok);
    (void)ok;
  } else {
    Symbol* tail = first;
    while (tail->overload_next != NULL) {
      tail = tail->overload_next;
    }
    symbol->namespace_ = first->namespace_;
    tail->overload_next = symbol;
    first->flags.is_overloaded = true;
    symbol->flags.is_overloaded = true;
    SymbolSetCXXMangledAsmName(first);
    SymbolSetCXXMangledAsmName(symbol);
  }
  return symbol;
}

static Symbol* GetImplicitCXXOperatorDelete(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* arg_type =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  return GetImplicitCXXAllocationFunction("operator delete", void_type,
                                          arg_type, location);
}

static Symbol* GetImplicitCXXOperatorDeleteArray(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* arg_type =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  return GetImplicitCXXAllocationFunction("operator delete[]", void_type,
                                          arg_type, location);
}

static Symbol* GetCXXClassAllocationFunction(TypeRecord* type,
                                             const char* name,
                                             size_t arg_count) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* member = FindStructMemberByName(type->info.struct_info, name);
  if (member == NULL || !member->is_member_function ||
      member->symbol == NULL || !TypeIsFunction(member->symbol->type)) {
    return NULL;
  }
  return FindCXXAllocationFunctionByArgCount(member->symbol, arg_count);
}

static Symbol* GetCXXOperatorNewForType(TypeRecord* type,
                                        bool is_array,
                                        size_t arg_count,
                                        SourceLocation location,
                                        bool global_scope,
                                        bool standard_placement) {
  // `::new` names only the global allocation function, bypassing any
  // class-scoped operator new.
  Symbol* member = global_scope ? NULL : GetCXXClassAllocationFunction(
      type, is_array ? "operator new[]" : "operator new", arg_count);
  if (member != NULL && standard_placement) {
    member = FindCXXPlacementAllocationFunction(member);
  }
  if (member != NULL) {
    return member;
  }
  const char* name = is_array ? "operator new[]" : "operator new";
  Symbol* global = NULL;
  if (standard_placement) {
    String symbol_name;
    StringInit(&symbol_name, name);
    global =
        FindCXXPlacementAllocationFunction(FindGlobalSymbol(&symbol_name));
    StringDestruct(&symbol_name);
  } else {
    global = FindGlobalCXXAllocationFunction(name, arg_count);
  }
  if (global != NULL) {
    return global;
  }
  if (arg_count == 1) {
    return is_array ? GetImplicitCXXOperatorNewArray(location)
                    : GetImplicitCXXOperatorNew(location);
  }
  if (!is_array && standard_placement && arg_count == 2) {
    return GetImplicitCXXPlacementOperatorNew(location);
  }
  SyntaxError(&compiler->syntax, "No matching allocation function for placement new");
  return is_array ? GetImplicitCXXOperatorNewArray(location)
                  : GetImplicitCXXOperatorNew(location);
}

static Symbol* GetCXXOperatorDeleteForType(TypeRecord* type,
                                           bool is_array,
                                           SourceLocation location,
                                           bool global_scope) {
  // `::delete` names only the global deallocation function, bypassing any
  // class-scoped operator delete.
  Symbol* member = global_scope ? NULL : GetCXXClassAllocationFunction(
      type, is_array ? "operator delete[]" : "operator delete", 1);
  if (member != NULL) {
    return member;
  }
  return is_array ? GetImplicitCXXOperatorDeleteArray(location)
                  : GetImplicitCXXOperatorDelete(location);
}

static ASTNode* NewCallASTNode(Symbol* callee, SourceLocation location,
                               Vector* actuals) {
  return NewVectorASTNode(AST_OP(call), NULL, location,
                          NewIdentifierASTNode(callee, location), actuals);
}

static StructMember* FindCXXConstructorForType(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  StructMember* ctor =
      FindStructMember(type->info.struct_info, type->info.struct_info->tag_name);
  if (ctor == NULL || !ctor->is_member_function ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  return ctor;
}

static bool CXXNewConstructorSetHasInitializerList(StructMember* ctor) {
  for (StructMember* candidate = ctor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->is_member_function || candidate->symbol == NULL ||
        candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type)) {
      continue;
    }
    FunctionInfo* info = &candidate->symbol->type->info.function;
    if (!info->is_constructor) {
      continue;
    }
    for (size_t i = 0; i < info->prototype.length; i++) {
      Symbol* formal = info->prototype.value.p[i];
      TypeRecord* formal_type = formal->type;
      if (TypeIsReference(formal_type)) {
        formal_type = formal_type->next;
      }
      if (TypeIsCXXInitializerList(formal_type)) {
        return true;
      }
    }
  }
  return false;
}

static Vector* ParseCXXNewInitializerArguments(Syntax* syntax, Token open,
                                               TokenClass followers) {
  Token close = open == TOK(lparen) ? TOK(rparen) : TOK(rbrace);
  LexNextToken(syntax->lex);
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, close)) {
    ASTNode* actual = SyntaxParseSingleExpression(syntax, followers | TC(exprsep));
    MarkCXXPackExpansionIfPresent(syntax, actual);
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, close, followers | TC(exprsep));
  return actuals;
}

static bool CXXNewHasPlacementArguments(Syntax* syntax) {
  if (!LexLookingAt(syntax->lex, TOK(lparen))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result =
      !LexLookingAt(syntax->lex, TOK(rparen)) && !SyntaxLookingAtType(syntax);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

static ASTNode* NewCXXConstructorCallForReceiver(TypeRecord* allocated_type,
                                                 ASTNode* receiver,
                                                 Vector* actuals,
                                                 SourceLocation location);

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

static ASTNode* NewCXXConstructorCallForPointer(TypeRecord* allocated_type,
                                                Symbol* ptr,
                                                Vector* actuals,
                                                SourceLocation location) {
  ASTNode* receiver = NewUnaryASTNode(AST_OP(contents), allocated_type, location,
                                      NewIdentifierASTNode(ptr, location));
  return NewCXXConstructorCallForReceiver(allocated_type, receiver, actuals,
                                          location);
}

static ASTNode* NewCXXConstructorCallForReceiver(TypeRecord* allocated_type,
                                                 ASTNode* receiver,
                                                 Vector* actuals,
                                                 SourceLocation location) {
  CXXPrependCompleteObjectArgument(allocated_type, actuals, location);
  ASTNode* member =
      NewStringConstantASTNode(NewString(allocated_type->info.struct_info
                                             ->tag_name->value),
                               NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
}

static StructMember* FindCXXDestructorForType(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, "~");
  StringAppendString(&name, type->info.struct_info->tag_name);
  StructMember* destructor = FindStructMember(type->info.struct_info, &name);
  StringDestruct(&name);
  if (destructor == NULL || !destructor->is_member_function ||
      !destructor->symbol->type->info.function.is_destructor) {
    return NULL;
  }
  return destructor;
}

static ASTNode* NewCXXDestructorCallForPointer(TypeRecord* object_type,
                                               Symbol* ptr,
                                               SourceLocation location) {
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, object_type->info.struct_info->tag_name);
  ASTNode* receiver =
      NewUnaryASTNode(AST_OP(contents), object_type, location,
                      NewIdentifierASTNode(ptr, location));
  ASTNode* member =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL,
                               location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  StringDestruct(&destructor_name);
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(object_type, actuals, location);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access,
                          actuals);
}

static ASTNode* NewExpressionStatement(ASTNode* expr, SourceLocation location) {
  return NewExpressionStatementASTNode(expr, location);
}

static ASTNode* NewAssign(ASTNode* left, ASTNode* right, TypeRecord* type,
                          SourceLocation location) {
  return NewBinaryASTNode(AST_OP(assign), type, location, left, right);
}

static ASTNode* NewTypedCast(TypeRecord* type, ASTNode* expr,
                             SourceLocation location) {
  ASTNode* result = NewCastASTNode(type, location, expr);
  ((CastASTNode*)result)->kind = kCastStatic;
  return result;
}

static ASTNode* NewPtrAdd(ASTNode* ptr, ASTNode* offset,
                          SourceLocation location) {
  return NewBinaryASTNode(AST_OP(plus), NULL, location, ptr, offset);
}

static ASTNode* NewPtrSub(ASTNode* ptr, ASTNode* offset,
                          SourceLocation location) {
  return NewBinaryASTNode(AST_OP(minus), NULL, location, ptr, offset);
}

static ASTNode* NewIntLiteral(int64_t value, SourceLocation location) {
  return NewIntConstantASTNode(value,
                               NewTypeRecordWithSize(kTypeInt, kQualConst),
                               location);
}

static ASTNode* NewPostIncrement(Symbol* sym, SourceLocation location) {
  return NewUnaryASTNode(AST_OP(postinc), NULL, location,
                         NewIdentifierASTNode(sym, location));
}

static ASTNode* NewArrayConstructionLoop(Syntax* syntax, TypeRecord* element_type,
                                         Symbol* ptr, Symbol* count,
                                         SourceLocation location) {
  Symbol* index =
      SyntaxNewTemporary(syntax, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  Symbol* element_ptr =
      SyntaxNewTemporary(syntax, NewPointerTo(kQualPlain, element_type));
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(index, location),
                             NewIntLiteral(0, location), index->type,
                             location),
                   location));

  Vector* body_statements = NewVector();
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(element_ptr, location),
                             NewPtrAdd(NewIdentifierASTNode(ptr, location),
                                       NewIdentifierASTNode(index, location),
                                       location),
                             element_ptr->type, location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewCXXConstructorCallForPointer(element_type, element_ptr,
                                                   NewVector(), location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(NewPostIncrement(index, location),
                                      location));
  ASTNode* body = NewCompoundStatementASTNode(body_statements, location);
  ASTNode* cond =
      NewBinaryASTNode(AST_OP(less), NULL, location,
                       NewIdentifierASTNode(index, location),
                       NewIdentifierASTNode(count, location));
  VectorAppend(statements,
               NewCombinedStatementASTNode(AST_OP(while), cond, body,
                                           location));
  return NewCompoundStatementASTNode(statements, location);
}

static ASTNode* NewArrayDestructionLoop(Syntax* syntax, TypeRecord* element_type,
                                        Symbol* ptr, Symbol* count,
                                        SourceLocation location) {
  Symbol* index =
      SyntaxNewTemporary(syntax, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  Symbol* element_ptr =
      SyntaxNewTemporary(syntax, NewPointerTo(kQualPlain, element_type));
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(index, location),
                             NewIntLiteral(0, location), index->type,
                             location),
                   location));

  Vector* body_statements = NewVector();
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(element_ptr, location),
                             NewPtrAdd(NewIdentifierASTNode(ptr, location),
                                       NewIdentifierASTNode(index, location),
                                       location),
                             element_ptr->type, location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewCXXDestructorCallForPointer(element_type, element_ptr,
                                                  location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(NewPostIncrement(index, location),
                                      location));
  ASTNode* body = NewCompoundStatementASTNode(body_statements, location);
  ASTNode* cond =
      NewBinaryASTNode(AST_OP(less), NULL, location,
                       NewIdentifierASTNode(index, location),
                       NewIdentifierASTNode(count, location));
  VectorAppend(statements,
               NewCombinedStatementASTNode(AST_OP(while), cond, body,
                                           location));
  return NewCompoundStatementASTNode(statements, location);
}

static ASTNode* IdentityCloneNodeForCXXNew(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static Vector* CloneAndAnalyzeCXXNewDeductionActuals(Vector* actuals) {
  Vector* clones = NewVector();
  for (size_t i = 0; actuals != NULL && i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    if (actual == NULL) {
      continue;
    }
    ASTNode* clone =
        ASTNodeClone(actual, IdentityCloneNodeForCXXNew, NULL, NULL);
    VectorAppend(clones, AnalyzeExpression(clone));
  }
  return clones;
}

static TypeRecord* DeduceCXXNewAllocatedType(Syntax* syntax,
                                             TypeRecord* allocated_type,
                                             Vector* actuals) {
  if (!CompilerIsCXX() || syntax == NULL ||
      !TypeIsClassTemplatePlaceholder(allocated_type) || actuals == NULL) {
    return allocated_type;
  }
  Symbol* class_template = TypeClassTemplatePlaceholderOrigin(allocated_type);
  Vector* deduction_actuals = CloneAndAnalyzeCXXNewDeductionActuals(actuals);
  bool alias_rejected = false;
  TypeRecord* deduced =
      TypeDeduceClassTemplateFromPlaceholder(syntax, allocated_type,
                                             deduction_actuals,
                                             /*allow_explicit=*/true,
                                             &alias_rejected);
  VectorDeleteWithContents(deduction_actuals,
                           (VectorElementDestructor)ASTNodeDelete,
                           /*free_element=*/false);
  if (deduced != NULL &&
      !TypeClassTemplatePlaceholderAcceptsDeduced(allocated_type, deduced)) {
    SyntaxError(syntax,
                "Deduced template arguments do not match alias template");
    TypeRecordDelete(deduced);
    deduced = NULL;
  }
  if (deduced == NULL) {
    if (alias_rejected) {
      SyntaxError(syntax,
                  "Deduced template arguments do not match alias template");
    } else {
      SyntaxError(syntax, "Could not deduce template arguments for %s",
                  class_template != NULL ? class_template->name.value
                                         : "<class template>");
    }
    return allocated_type;
  }
  TypeRecordDelete(allocated_type);
  return deduced;
}

static Vector* ParseCXXNewDeductionInitializerArguments(Syntax* syntax,
                                                        TypeRecord* type,
                                                        Token open,
                                                        TokenClass followers) {
  if (open == TOK(lbrace) &&
      CXXNewConstructorSetHasInitializerList(FindCXXConstructorForType(type))) {
    Vector* actuals = NewVector();
    LexNextToken(syntax->lex);
    VectorAppend(actuals, SyntaxParseBracedInitializer(syntax));
    return actuals;
  }
  return ParseCXXNewInitializerArguments(syntax, open, followers);
}

// Build a `(type){ exprs... }` compound literal used to initialize a freshly
// `new`-allocated object that has no constructor (a scalar aggregate wrapper or
// a class aggregate).  With no expressions this performs value-initialization
// (zero-initialization); with expressions it performs aggregate initialization.
// `exprs`, when non-NULL, is consumed (its elements are re-wrapped and the
// vector is deleted).
static ASTNode* NewCXXNewCompoundLiteralInitializer(Syntax* syntax,
                                                    TypeRecord* type,
                                                    Vector* exprs,
                                                    SourceLocation location) {
  Symbol* storage = SyntaxNewTemporary(syntax, type);
  Vector* elements = NewVector();
  if (exprs != NULL) {
    for (size_t i = 0; i < exprs->length; i++) {
      VectorAppend(elements, NewExpressionInitializerASTNode(
                                 exprs->value.p[i], location));
    }
    VectorDelete(exprs);
  }
  ASTNode* braced = NewBracedInitializerASTNode(elements, NULL, location);
  return NewCompoundLiteralASTNode(NewIdentifierASTNode(storage, location),
                                   location, braced);
}

static ASTNode* ParseCXXNewExpression(Syntax* syntax, TokenClass followers,
                                      bool global_scope) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // new

  Vector* placement_actuals = NULL;
  if (CXXNewHasPlacementArguments(syntax)) {
    placement_actuals =
        ParseCXXNewInitializerArguments(syntax, TOK(lparen), followers);
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* allocated_type = TypeParserParseType(&parser, true);
  Token initializer_open = TOK(bad);
  ASTNode* array_size = NULL;
  if (LexLookingAt(syntax->lex, TOK(lparen)) ||
      LexLookingAt(syntax->lex, TOK(lbrace)) ||
      LexLookingAt(syntax->lex, TOK(lsquare))) {
    initializer_open = syntax->lex->current_token;
  }
  Symbol* sym = initializer_open == TOK(bad)
      ? TypeParserParseDeclarator(&parser, allocated_type)
      : NULL;
  if (sym == NULL) {
    if (allocated_type == NULL) {
      SyntaxError(syntax, "Invalid type in new expression");
      allocated_type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    }
  } else {
    allocated_type = sym->type;
  }
  if (initializer_open == TOK(bad) &&
      (LexLookingAt(syntax->lex, TOK(lparen)) ||
       LexLookingAt(syntax->lex, TOK(lbrace)) ||
       LexLookingAt(syntax->lex, TOK(lsquare)))) {
    initializer_open = syntax->lex->current_token;
  }
  TypeParserDestruct(&parser);
  Vector* ctor_actuals = NULL;
  if (TypeIsClassTemplatePlaceholder(allocated_type) &&
      (initializer_open == TOK(lparen) || initializer_open == TOK(lbrace))) {
    ctor_actuals = ParseCXXNewDeductionInitializerArguments(
        syntax, allocated_type, initializer_open, followers);
    allocated_type = DeduceCXXNewAllocatedType(syntax, allocated_type,
                                               ctor_actuals);
  }
  if (TypeIsClassTemplatePlaceholder(allocated_type)) {
    SyntaxError(syntax,
                "Class template argument deduction requires an initializer");
  }
  TypeRecordCalculateSize(allocated_type);
  bool allocated_type_dependent =
      TypeContainsTemplateParameter(allocated_type);
  if (TypeIsAbstractClass(allocated_type)) {
    SyntaxError(syntax, "Cannot allocate object of abstract class %s",
                allocated_type->info.struct_info->tag_name != NULL
                    ? allocated_type->info.struct_info->tag_name->value
                    : "<anonymous>");
  }

  ASTNode* scalar_initializer = NULL;
  bool value_init = false;
  if (initializer_open == TOK(lsquare)) {
    LexNextToken(syntax->lex);
    array_size = SyntaxParseSingleExpression(syntax, TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rsquare), followers);
  } else if (ctor_actuals == NULL &&
             (initializer_open == TOK(lparen) || initializer_open == TOK(lbrace))) {
    if (FindCXXConstructorForType(allocated_type) == NULL) {
      Vector* initializers =
          ParseCXXNewInitializerArguments(syntax, initializer_open, followers);
      bool class_direct_init =
          !allocated_type_dependent && TypeIsStructOrUnion(allocated_type) &&
          allocated_type->info.struct_info != NULL &&
          allocated_type->info.struct_info->tag_name != NULL &&
          allocated_type->info.struct_info->is_class &&
          !allocated_type->info.struct_info->is_aggregate;
      if (initializers->length == 1) {
        if (class_direct_init) {
          // `new (p) T(arg)` on a class type performs direct/copy/move
          // construction, never assignment into uninitialized storage.
          ctor_actuals = initializers;
        } else {
          // `new T(x)` / `new T{x}`: direct/copy-initialization of a scalar (or a
          // same-typed class object).  For a dependent T this assignment is kept
          // and may be rewritten to `receiver.T(x)` once T resolves to a class.
          scalar_initializer = initializers->value.p[0];
          VectorDestruct(initializers);
        }
      } else if (initializers->length == 0) {
        // `new T()` / `new T{}`: value-initialization.  A scalar (or dependent
        // T, which is resolved during instantiation) is zero-initialized with a
        // plain assignment; a concrete aggregate is zero-initialized with an
        // empty compound literal so its members and padding are cleared.
        value_init = true;
        if (allocated_type_dependent ||
            (!TypeIsStructOrUnion(allocated_type) &&
                          !TypeIsArray(allocated_type))) {
          scalar_initializer = NewIntLiteral(0, location);
        } else {
          scalar_initializer = NewCXXNewCompoundLiteralInitializer(
              syntax, allocated_type, NULL, location);
        }
        VectorDelete(initializers);
      } else if (allocated_type_dependent) {
        scalar_initializer =
            NewBracedInitializerASTNode(initializers, NULL, location);
      } else if (TypeIsStructOrUnion(allocated_type) ||
                 TypeIsArray(allocated_type)) {
        // `new T{a, b, ...}` (or the C++20 parenthesized aggregate form
        // `new T(a, b, ...)`): aggregate-initialize the object.
        scalar_initializer = NewCXXNewCompoundLiteralInitializer(
            syntax, allocated_type, initializers, location);
      } else {
        // A scalar type cannot take more than one new-initializer expression.
        SyntaxError(syntax, "new initializer for non-class type requires one expression");
        VectorDelete(initializers);
      }
    } else {
      ctor_actuals =
          ParseCXXNewInitializerArguments(syntax, initializer_open, followers);
    }
  } else if (ctor_actuals == NULL &&
             FindCXXConstructorForType(allocated_type) != NULL) {
    ctor_actuals = NewVector();
  }

  Vector* actuals = NewVector();
  ASTNode* sole_placement =
      placement_actuals != NULL && placement_actuals->length == 1
          ? placement_actuals->value.p[0] : NULL;
  TypeRecord* sole_placement_type =
      sole_placement != NULL ? sole_placement->type : NULL;
  if (sole_placement_type == NULL && sole_placement != NULL &&
      sole_placement->op == AST_OP(cast)) {
    sole_placement_type = ((CastASTNode*)sole_placement)->cast_type;
  }
  bool standard_placement =
      sole_placement_type != NULL && TypeIsPointer(sole_placement_type);
  ASTNode* allocation_size =
      allocated_type_dependent
          ? NewSizeofASTNodeWithType(allocated_type, location)
          : NewSizeofASTNodeWithKnownSize(allocated_type->size, location);
  Symbol* array_count = NULL;
  if (array_size != NULL) {
    array_count = SyntaxNewTemporary(syntax, NewSizeTypeRecord());
    allocation_size =
        NewBinaryASTNode(AST_OP(mult), NULL, location,
                         NewIdentifierASTNode(array_count, location),
                         allocation_size);
    if (!standard_placement) {
      allocation_size = NewBinaryASTNode(
          AST_OP(plus), NULL, location, allocation_size,
          NewSizeofASTNodeWithKnownSize(NewSizeTypeRecord()->size, location));
    }
  }
  VectorAppend(actuals, allocation_size);
  if (placement_actuals != NULL) {
    for (size_t i = 0; i < placement_actuals->length; i++) {
      VectorAppend(actuals, placement_actuals->value.p[i]);
    }
    VectorDelete(placement_actuals);
  }
  ASTNode* allocation =
      NewCallASTNode(GetCXXOperatorNewForType(allocated_type,
                                              array_size != NULL,
                                              actuals->length,
                                              location, global_scope,
                                              standard_placement),
                     location, actuals);
  TypeRecord* result_type = NewPointerTo(kQualPlain, allocated_type);
  ASTNode* result = NewCastASTNode(result_type, location, allocation);
  ((CastASTNode*)result)->kind = kCastStatic;
  result->flags |= kASTCXXNewExpression;
  if (standard_placement) {
    result->flags |= kASTCXXPlacementNew;
  }
  if (array_size != NULL) {
    result->flags |= kASTCXXArrayNew;
  }
  if (allocated_type_dependent) {
    result->flags |= kASTDependentNewAllocation;
  }
  if (array_size != NULL) {
    TypeRecord* size_type = NewSizeTypeRecord();
    TypeRecord* size_ptr_type = NewPointerTo(kQualPlain, size_type);
    Symbol* object_ptr = SyntaxNewTemporary(syntax, result_type);
    Symbol* header = standard_placement
        ? NULL : SyntaxNewTemporary(syntax, size_ptr_type);
    Vector* statements = NewVector();
    if (standard_placement) {
      VectorAppend(statements,
                   NewVariableDeclarationASTNode(
                       array_count,
                       NewExpressionInitializerASTNode(
                           NewIntLiteral(0, location), location),
                       location));
      VectorAppend(statements,
                   NewVariableDeclarationASTNode(object_ptr, NULL, location));
    }
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(array_count, location),
                               array_size, array_count->type, location),
                     location));
    if (standard_placement) {
      VectorAppend(statements,
                   NewExpressionStatement(
                       NewAssign(NewIdentifierASTNode(object_ptr, location),
                                 result, object_ptr->type, location),
                       location));
      TypeRecordDelete(size_ptr_type);
    } else {
      VectorAppend(statements,
                   NewExpressionStatement(
                       NewAssign(NewIdentifierASTNode(header, location),
                                 NewTypedCast(size_ptr_type, result, location),
                                 header->type, location),
                       location));
      VectorAppend(statements,
                   NewExpressionStatement(
                       NewAssign(NewUnaryASTNode(AST_OP(contents), size_type,
                                                location,
                                                NewIdentifierASTNode(header,
                                                                     location)),
                                 NewIdentifierASTNode(array_count, location),
                                 size_type, location),
                       location));
      VectorAppend(statements,
                   NewExpressionStatement(
                       NewAssign(NewIdentifierASTNode(object_ptr, location),
                                 NewTypedCast(
                                     result_type,
                                     NewPtrAdd(NewIdentifierASTNode(header,
                                                                   location),
                                               NewIntLiteral(1, location),
                                               location),
                                     location),
                                 object_ptr->type, location),
                       location));
    }
    if (TypeIsStructOrUnion(allocated_type) &&
        FindCXXConstructorForType(allocated_type) != NULL) {
      VectorAppend(statements,
                   NewArrayConstructionLoop(syntax, allocated_type, object_ptr,
                                            array_count, location));
    }
    VectorAppend(statements,
                 NewExpressionStatement(NewIdentifierASTNode(object_ptr,
                                                             location),
                                        location));
    result = NewUnaryASTNode(AST_OP(stmt_expr), result_type, location,
                             NewCompoundStatementASTNode(statements, location));
  } else if (ctor_actuals != NULL || scalar_initializer != NULL) {
    Symbol* temp = SyntaxNewTemporary(syntax, result_type);
    // The lowered comma expression assigns through this temporary and then
    // reuses it as the constructor receiver.  Keep it in addressable storage;
    // targets cannot represent that sequence with a register-only variable.
    temp->flags.address_taken = true;
    ASTNode* temp_lhs = NewIdentifierASTNode(temp, location);
    temp_lhs->flags |= kASTNeedAddress;
    ASTNode* assign =
        NewBinaryASTNode(AST_OP(assign), result_type, location,
                         temp_lhs, result);
    ASTNode* init = ctor_actuals != NULL
        ? NewCXXConstructorCallForPointer(allocated_type, temp, ctor_actuals,
                                          location)
        : NewAssign(NewUnaryASTNode(AST_OP(contents), allocated_type, location,
                                    NewIdentifierASTNode(temp, location)),
                    scalar_initializer, allocated_type, location);
    if (ctor_actuals == NULL && TypeContainsTemplateParameter(allocated_type)) {
      init->flags |= kASTDependentNewInitializer;
      if (value_init) {
        init->flags |= kASTDependentNewValueInit;
      }
    }
    Vector* statements = NewVector();
    VectorAppend(statements,
                 NewVariableDeclarationASTNode(temp, NULL, location));
    VectorAppend(statements, NewExpressionStatement(assign, location));
    VectorAppend(statements, NewExpressionStatement(init, location));
    VectorAppend(statements,
                 NewExpressionStatement(NewIdentifierASTNode(temp, location),
                                        location));
    result = NewUnaryASTNode(AST_OP(stmt_expr), result_type, location,
                            NewCompoundStatementASTNode(statements, location));
  }
  if (sym != NULL) {
    SymbolDelete(sym);
  }
  return result;
}

ASTNode* NewCXXDeleteExpressionForPointer(Syntax* syntax, ASTNode* expr,
                                          bool is_array_delete,
                                          SourceLocation location,
                                          bool global_scope) {
  TypeRecord* pointer_type = expr->type;
  if ((pointer_type != NULL && TypeContainsTemplateParameter(pointer_type)) ||
      (syntax->parsing_template_declaration &&
       (pointer_type == NULL || !TypeIsPointerOrArray(pointer_type)))) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, expr);
    ASTNode* node = NewCallASTNode(
        is_array_delete ? GetImplicitCXXOperatorDeleteArray(location)
                        : GetImplicitCXXOperatorDelete(location),
        location, actuals);
    node->flags |= kASTDependentDelete;
    if (is_array_delete) {
      node->flags |= kASTDependentArrayDelete;
    }
    return node;
  }
  TypeRecord* deleted_type =
      TypeIsPointerOrArray(pointer_type) ? pointer_type->next : NULL;
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      TypeIsStructOrUnion(deleted_type) &&
      !TypeIsCompleteClass(deleted_type)) {
    SyntaxError(syntax,
                "deleting a pointer to an incomplete class type is not "
                "allowed in C++26");
  }
  if (is_array_delete) {
    if (pointer_type == NULL || !TypeIsPointerOrArray(pointer_type)) {
      Vector* actuals = NewVector();
      VectorAppend(actuals, expr);
    return NewCallASTNode(GetImplicitCXXOperatorDeleteArray(location),
                            location, actuals);
    }
    TypeRecord* size_type = NewSizeTypeRecord();
    TypeRecord* size_ptr_type = NewPointerTo(kQualPlain, size_type);
    Symbol* object_ptr = SyntaxNewTemporary(syntax, pointer_type);
    Symbol* header = SyntaxNewTemporary(syntax, size_ptr_type);
    Symbol* count = SyntaxNewTemporary(syntax, size_type);
    Vector* statements = NewVector();
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(object_ptr, location),
                               expr, object_ptr->type, location),
                     location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(header, location),
                               NewPtrSub(NewTypedCast(size_ptr_type,
                                                      NewIdentifierASTNode(
                                                          object_ptr,
                                                          location),
                                                      location),
                                         NewIntLiteral(1, location), location),
                               header->type, location),
                     location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(count, location),
                               NewUnaryASTNode(AST_OP(contents), size_type,
                                                location,
                                                NewIdentifierASTNode(header,
                                                                     location)),
                               count->type, location),
                     location));
    if (TypeIsStructOrUnionPointer(pointer_type) &&
        FindCXXDestructorForType(pointer_type->next) != NULL) {
      VectorAppend(statements,
                   NewArrayDestructionLoop(syntax, pointer_type->next,
                                           object_ptr, count, location));
    }
    Vector* actuals = NewVector();
    VectorAppend(actuals, NewIdentifierASTNode(header, location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewCallASTNode(GetCXXOperatorDeleteForType(
                                        pointer_type->next, true, location,
                                        global_scope),
                                    location, actuals),
                     location));
    return NewUnaryASTNode(
        AST_OP(stmt_expr), NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        location, NewCompoundStatementASTNode(statements, location));
  }
  if (pointer_type == NULL || !TypeIsStructOrUnionPointer(pointer_type) ||
      FindCXXDestructorForType(pointer_type->next) == NULL) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, expr);
    TypeRecord* object_type =
        TypeIsPointerOrArray(pointer_type) ? pointer_type->next : NULL;
    return NewCallASTNode(GetCXXOperatorDeleteForType(object_type, false,
                                                     location, global_scope),
                          location, actuals);
  }

  Symbol* temp = SyntaxNewTemporary(syntax, pointer_type);
  ASTNode* assign =
      NewBinaryASTNode(AST_OP(assign), pointer_type, location,
                       NewIdentifierASTNode(temp, location), expr);
  ASTNode* destructor =
      NewCXXDestructorCallForPointer(pointer_type->next, temp, location);
  Vector* actuals = NewVector();
  VectorAppend(actuals, NewIdentifierASTNode(temp, location));
  ASTNode* deallocate =
      NewCallASTNode(GetCXXOperatorDeleteForType(pointer_type->next, false,
                                                 location, global_scope),
                     location, actuals);
  return NewBinaryASTNode(
      AST_OP(comma), NewTypeRecordWithSize(kTypeVoid, kQualPlain), location,
      assign, NewBinaryASTNode(AST_OP(comma),
                               NewTypeRecordWithSize(kTypeVoid, kQualPlain),
                               location, destructor, deallocate));
}

static ASTNode* ParseCXXDeleteExpression(Syntax* syntax, TokenClass followers,
                                         bool global_scope) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // delete
  bool is_array_delete = false;
  if (LexMatch(syntax->lex, TOK(lsquare))) {
    SyntaxNeedBracket(syntax, TOK(rsquare), followers);
    is_array_delete = true;
  }
  ASTNode* expr = ParseCastExpression(syntax, followers);
  return NewCXXDeleteExpressionForPointer(syntax, expr, is_array_delete,
                                          location, global_scope);
}

static ASTNode* ParseCXXThrowExpression(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // throw
  ASTNode* expr = NULL;
  if (!LexLookingAt(syntax->lex, TOK(semicolon)) &&
      !LexLookingAt(syntax->lex, TOK(rparen)) &&
      !LexLookingAt(syntax->lex, TOK(comma))) {
    expr = ParseAssignmentExpression(syntax, followers);
  }
  return NewThrowASTNode(expr, location);
}

static ASTNode* ParseCXXCoAwaitExpression(Syntax* syntax,
                                          TokenClass followers) {
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    compiler->current_function->info.function.has_coroutine_syntax = true;
  }
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // co_await
  ASTNode* expr = ParseCastExpression(syntax, followers);
  return NewUnaryASTNode(AST_OP(co_await), NULL, location, expr);
}

static ASTNode* ParseCXXCoYieldExpression(Syntax* syntax,
                                          TokenClass followers) {
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    compiler->current_function->info.function.has_coroutine_syntax = true;
  }
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // co_yield
  ASTNode* expr = NULL;
  if (!LexLookingAt(syntax->lex, TOK(semicolon)) &&
      !LexLookingAt(syntax->lex, TOK(rparen)) &&
      !LexLookingAt(syntax->lex, TOK(comma))) {
    expr = ParseAssignmentExpression(syntax, followers);
  } else {
    SyntaxError(syntax, "Expected expression after co_yield");
    expr = NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  return NewUnaryASTNode(AST_OP(co_yield), NULL, location, expr);
}

// Parse a unary expression with syntax:
// unary-expression:
//    postfix-expression
//    ++ unary-expression
//    -- unary-expression
//    unary-operator cast-expression
//    sizeof unary-expression
//    sizeof ( type-name )

// unary-operator: one of
//    & * + - ~ !

static ASTNode* ParsePointerToMember(Syntax* syntax, TokenClass followers);

static ASTNode* ParseUnaryExpression(Syntax* syntax, TokenClass followers) {
  if (syntax->lex->preprocessor_mode) {
    // In preprocessor mode we have a unary operator that is a the identifier
    // 'defined'.  This is followed by a possibly parenthesized macro name.
    // The result is an int constant with value 1 if the macro exists and 0
    // otherwise.
    if (LexLookingAt(syntax->lex, TOK(identifier))) {
      ASTNode* result = ParsePossiblePreprocessorFunction(syntax, followers);
      if (result != NULL) {
        return result;
      }
    }
  }

  if (LexMatch(syntax->lex, TOK(plus))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(uplus), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(minus))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(uminus), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(amp))) {
    if (CompilerIsCXX()) {
      ASTNode* member_ptr = ParsePointerToMember(syntax, followers);
      if (member_ptr != NULL) {
        return member_ptr;
      }
    }
    ASTNode* sub = ParseCastExpression(syntax, followers);
    // If we are taking the address of an identifier we need to
    // set a flag so that the later phases know that this has
    // to be in memory (can't be in a register, if that is supported).
    if (sub->op == AST_OP(identifier)) {
      IdentifierASTNode* ident = (IdentifierASTNode*)sub;
      ident->symbol->flags.address_taken = true;
    } else if (sub->op == AST_OP(splice)) {
      ((SpliceASTNode*)sub)->context = kSpliceAddressed;
    }
    return NewUnaryASTNode(AST_OP(address), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(star))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(contents), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(tilde))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(onescomp), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(bang))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(not), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(plusplus))) {
    ASTNode* sub = ParseUnaryExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(preinc), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(minusminus))) {
    ASTNode* sub = ParseUnaryExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(predec), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(sizeof))) {
    return ParseSizeof(syntax, followers);
  }

  if (LexMatch(syntax->lex, TOK(alignof))) {
    return ParseAlignof(syntax, followers);
  }

  if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(noexcept))) {
    SourceLocation location = syntax->lex->current_token_location;
    if (!LexMatch(syntax->lex, TOK(lparen))) {
      SyntaxError(syntax, "Expected ( after noexcept");
      return NewUnaryASTNode(AST_OP(noexcept_expr), NULL, location, NULL);
    }
    ASTNode* expression =
        SyntaxParseSingleExpression(syntax, TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    return NewUnaryASTNode(AST_OP(noexcept_expr), NULL, location, expression);
  }

  if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(typeid))) {
    // typeid(...) is a postfix-expression, so allow trailing postfix operators
    // such as the `.name()` member call.
    ASTNode* result = ParseTypeid(syntax, followers);
    return ParsePostfixOperators(syntax, result, followers);
  }

  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(reflect))) {
    SourceLocation location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    ASTNode* result =
        ParseReflectionExpression(syntax, followers, location);
    return ParsePostfixOperators(syntax, result, followers);
  }

  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(new))) {
    return ParseCXXNewExpression(syntax, followers, /*global_scope=*/false);
  }

  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(delete))) {
    return ParseCXXDeleteExpression(syntax, followers, /*global_scope=*/false);
  }

  // `::new` / `::delete` explicitly name the global allocation/deallocation
  // functions, bypassing any class-scoped operator new/delete.  The leading
  // `::` would otherwise be parsed as a qualified-name prefix, so peek past it.
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(coloncolon))) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    LexNextToken(syntax->lex);  // consume ::
    if (LexLookingAt(syntax->lex, TOK(new))) {
      LexCheckpointDestruct(&checkpoint);
      return ParseCXXNewExpression(syntax, followers, /*global_scope=*/true);
    }
    if (LexLookingAt(syntax->lex, TOK(delete))) {
      LexCheckpointDestruct(&checkpoint);
      return ParseCXXDeleteExpression(syntax, followers, /*global_scope=*/true);
    }
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
  }

  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(throw))) {
    return ParseCXXThrowExpression(syntax, followers);
  }

  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(co_await))) {
    return ParseCXXCoAwaitExpression(syntax, followers);
  }

  return ParsePostfixExpression(syntax, followers);
}

static CastKind CXXNamedCastKind(Token token) {
  switch (token) {
    case TOK(static_cast):
      return kCastStatic;
    case TOK(reinterpret_cast):
      return kCastReinterpret;
    case TOK(const_cast):
      return kCastConst;
    case TOK(dynamic_cast):
      return kCastDynamic;
    default:
      assert(false);
      return kCastCStyle;
  }
}

static bool TokenIsCXXNamedCast(Token token) {
  return token == TOK(static_cast) || token == TOK(reinterpret_cast) ||
         token == TOK(const_cast) || token == TOK(dynamic_cast);
}

static ASTNode* ParseCXXNamedCastExpression(Syntax* syntax,
                                            TokenClass followers) {
  Token cast_token = syntax->lex->current_token;
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);
  SyntaxNeedBracket(syntax, TOK(less), followers);

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&parser, false);
  Symbol* sym = NULL;
  if (type == NULL) {
    SyntaxError(syntax, "Invalid type name");
    type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  } else {
    sym = TypeParserParseDeclarator(&parser, type);
    type = sym->type;
  }
  TypeParserDestruct(&parser);

  SyntaxNeedTemplateClose(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  ASTNode* result = NewCastASTNode(type, location, expr);
  ((CastASTNode*)result)->kind = CXXNamedCastKind(cast_token);
  if (sym != NULL) {
    SymbolDelete(sym);
  }
  // A named cast is a postfix-expression, so it may be directly followed by
  // postfix operators, e.g. `static_cast<T>(x).member` or `static_cast<T>(p)->m`.
  return ParsePostfixOperators(syntax, result, followers);
}

static ASTNode* ParsePointerToMember(Syntax* syntax, TokenClass followers) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    return NULL;
  }
  String class_name;
  StringInit(&class_name, syntax->lex->spelling.value);
  LexNextToken(syntax->lex);
  if (!LexMatch(syntax->lex, TOK(coloncolon)) ||
      !LexLookingAt(syntax->lex, TOK(identifier))) {
    StringDestruct(&class_name);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    return NULL;
  }
  String member_name;
  StringInit(&member_name, syntax->lex->spelling.value);
  LexNextToken(syntax->lex);
  Symbol* class_sym = SyntaxFindSymbol(syntax, &class_name);
  Struct* class_info = NULL;
  if (class_sym != NULL && class_sym->type != NULL &&
      TypeIsStructOrUnion(class_sym->type) &&
      class_sym->type->info.struct_info != NULL) {
    class_info = class_sym->type->info.struct_info;
  }
  StringDestruct(&class_name);
  if (class_info == NULL) {
    StringDestruct(&member_name);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    return NULL;
  }
  StructMember* member = FindStructMember(class_info, &member_name);
  StringDestruct(&member_name);
  if (member == NULL || member->symbol == NULL || member->symbol->type == NULL ||
      member->is_static) {
    LexCheckpointRestore(syntax->lex, &checkpoint);
    return NULL;
  }
  // Forming a pointer-to-member (&Class::field) counts as a use of the data
  // member for -Wunused-private-field.
  member->symbol->flags.used = true;
  if (member->is_member_function &&
      FunctionHasExplicitObjectParameter(member->symbol->type)) {
    member->symbol->flags.address_taken = true;
    SourceLocation location = syntax->lex->current_token_location;
    ASTNode* function = NewIdentifierASTNode(member->symbol, location);
    return NewUnaryASTNode(AST_OP(address), NULL, location, function);
  }
  TypeRecord* member_type = TypeMemberPointerPointeeFromMember(member);
  TypeRecord* mptr = NewMemberPointerTypeRecord(class_info, kQualPlain);
  TypeRecordChain(mptr, member_type);
  TypeRecordCalculateSize(mptr);
  SourceLocation location = syntax->lex->current_token_location;
  StructMemberASTNode* member_node =
      (StructMemberASTNode*)NewStructMemberASTNode(member, location);
  if (member->symbol->flags.name_independent_lookup_ambiguous) {
    member_node->base.flags |= kASTNameIndependentLookupAmbiguous;
  }
  ASTNode* result =
      NewUnaryASTNode(AST_OP(member_ptr), NULL, location, (ASTNode*)member_node);
  ASTNodeSetType(result, mptr);
  (void)followers;
  return result;
}

// Parse cast expression with syntax:
// cast-expression:
//    unary-expression
//    ( type-name ) cast-expression
//
// This also handles compound literals, which are actually postfix expressions.

// True when the current token starts a class/enum/union *definition*
// (`enum {`, `struct S {`, ...), as opposed to a mere type-name (`enum E`).
static bool LookingAtDefiningTagSpecifier(Syntax* syntax) {
  Token tok = syntax->lex->current_token;
  if (tok != TOK(enum) && tok != TOK(struct) && tok != TOK(class) &&
      tok != TOK(union)) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  int paren_depth = 0;
  bool found_brace = false;
  LexNextToken(syntax->lex);
  while (!LexEof(syntax->lex)) {
    if (paren_depth == 0 && LexLookingAt(syntax->lex, TOK(lbrace))) {
      found_brace = true;
      break;
    }
    if (paren_depth == 0 &&
        (LexLookingAt(syntax->lex, TOK(rparen)) ||
         LexLookingAt(syntax->lex, TOK(semicolon)) ||
         LexLookingAt(syntax->lex, TOK(rbrace)))) {
      break;
    }
    if (LexMatch(syntax->lex, TOK(lparen))) {
      paren_depth++;
      continue;
    }
    if (paren_depth > 0 && LexLookingAt(syntax->lex, TOK(rparen))) {
      paren_depth--;
      LexNextToken(syntax->lex);
      continue;
    }
    LexNextToken(syntax->lex);
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return found_brace;
}

static ASTNode* ParseCastExpression(Syntax* syntax, TokenClass followers) {
  if (CompilerIsCXX() && TokenIsCXXNamedCast(syntax->lex->current_token)) {
    return ParseCXXNamedCastExpression(syntax, followers);
  }
  if (!syntax->lex->preprocessor_mode && !syntax->lex->assembler_mode &&
      LexMatch(syntax->lex, TOK(lparen))) {
    // A leading __attribute__ (GCC extension) only appears in type names, so
    // treat "( __attribute__((...)) type-name )" as a cast / compound literal.
    if (SyntaxLookingAtType(syntax) ||
        SyntaxLookingAtAnyAttribute(syntax)) {
      // Nested type definitions in enumerator initializers are ill-formed.
      // Diagnose before the speculative `(type-name)` parse so the error is
      // not swallowed by abort_on_error, and recover without skipping the
      // enclosing enumerator-list `}`.
      if (CompilerIsCXX() && syntax->parsing_enum_specifier_depth > 0 &&
          LookingAtDefiningTagSpecifier(syntax)) {
        SourceLocation location = syntax->lex->current_token_location;
        SyntaxError(syntax, "Type cannot be defined in an enumeration");
        SyntaxRecover(syntax, TC(closebrace) | TC(closebra) | TC(semicolon));
        LexMatch(syntax->lex, TOK(rbrace));
        LexMatch(syntax->lex, TOK(rparen));
        return NewIntConstantASTNode(
            0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
      }
      // The leading '(' is followed by something that begins a type, so this is
      // potentially a cast or compound literal `( type-name ) ...`.  It can,
      // however, also be a parenthesized functional-cast expression such as
      // `(T(args))`, where `T(args)` is not a type-id (e.g. the arguments are
      // values rather than parameter declarations).  Tentatively parse the
      // type-name; if it does not form a well-formed `( type-name )` (i.e. the
      // type-name is not immediately followed by ')', or the parse hits an
      // error), backtrack and reparse the parenthesized construct as an
      // expression.  Valid casts always succeed here, so their behavior is
      // unchanged; only inputs that previously failed as casts are rerouted.
      LexCheckpoint checkpoint;
      LexCheckpointSave(syntax->lex, &checkpoint);
      TypeParser parser;
      volatile bool parse_completed = false;
      volatile bool is_type_name = false;
      TypeRecord* volatile type = NULL;
      Symbol* volatile sym = NULL;
      // Trap any diagnostics emitted while speculatively parsing the type-name
      // so a backtrack stays silent.  `abort_on_error` makes a SyntaxError
      // longjmp out immediately (so we stop at the first one), while the trap
      // additionally catches LexErrors (e.g. "Missing close parenthesis"), which
      // do not honor `abort_on_error`.
      volatile bool saved_trap = DiagnosticErrorTrapBegin();
      bool prev_abort_on_error = abort_on_error;
      // Save the global abort target: parsing the type-name can recurse through
      // the expression parser (e.g. an array bound) and set up its own trial,
      // which would otherwise clobber our jmp_buf.
      jmp_buf saved_abort_state;
      memcpy(saved_abort_state, error_abort_state, sizeof(error_abort_state));
      // A longjmp out of the trial parse skips the bookkeeping of every level
      // it entered, so restore the nesting depths by hand.  A type-name can
      // define a class (`(struct { int x; }){0}`), so both counters are at risk.
      int saved_nesting_depth = syntax->expression_nesting_depth;
      int saved_struct_depth = syntax->struct_definition_depth;
      abort_on_error = true;
      if (setjmp(error_abort_state) == 0) {
        TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                       syntax->context);
        type = TypeParserParseType(&parser, false);
        if (type != NULL) {
          sym = TypeParserParseDeclarator(&parser, type);
          type = sym->type;
          // A well-formed cast / compound-literal type-name is an abstract
          // declarator ending exactly at the closing ')'.  Two shapes of the
          // parenthesized functional cast `(T(arg))` slip past a naive check and
          // must be rejected so we backtrack to expression parsing:
          //   * a value argument is (mis)parsed as a *named* declarator, e.g.
          //     `(Fahrenheit(c))` -> "Fahrenheit c"; a real type-name names
          //     nothing, so require the declarator to be abstract (invented),
          //   * a type argument is (mis)parsed as a *function* type, e.g.
          //     `(weak_ordering(strong_ordering::x))`; a bare function type is
          //     never a valid cast target, so reject it (function pointers,
          //     which are valid, are not function types).
          is_type_name = LexLookingAt(syntax->lex, TOK(rparen)) &&
                         sym != NULL && sym->flags.invented &&
                         !TypeIsFunction(type);
        }
        parse_completed = true;
      }
      abort_on_error = prev_abort_on_error;
      memcpy(error_abort_state, saved_abort_state, sizeof(error_abort_state));
      syntax->expression_nesting_depth = saved_nesting_depth;
      syntax->struct_definition_depth = saved_struct_depth;
      bool trapped = DiagnosticErrorTrapped();
      DiagnosticErrorTrapEnd(saved_trap);
      if (!parse_completed || trapped || !is_type_name) {
        // Not a well-formed `( type-name )`; abandon the trial parse (its
        // TypeParser is only released when it completed cleanly, since a
        // longjmp leaves it in a partial state) and reparse as an expression.
        if (parse_completed) {
          if (sym != NULL) {
            SymbolDelete(sym);
          }
          TypeParserDestruct(&parser);
        }
        LexCheckpointRestore(syntax->lex, &checkpoint);
        LexCheckpointDestruct(&checkpoint);
        syntax->found_open_paren = true;
        return ParsePostfixExpression(syntax, followers);
      }
      LexCheckpointDestruct(&checkpoint);
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      // If the (type-name) is followed by an initializer list we have
      // a compound literal.  This is actually a postfix expression so we
      // save the parsed type and move forward.  The initializer will be
      // parsed by the postfix expression parser.
      if (LexLookingAt(syntax->lex, TOK(lbrace))) {
        TypeRecord* saved_type = syntax->compound_literal_type;
        syntax->compound_literal_type = type;
        ASTNode* result = ParseCastExpression(syntax, followers);
        syntax->compound_literal_type = saved_type;
        if (sym != NULL) {
          // The compound literal will create its own symbol.
          SymbolDelete(sym);
        }
        TypeParserDestruct(&parser);
        return result;
      }
      ASTNode* expr = ParseCastExpression(syntax, followers);
      // The cast AST node will take ownership of the TypeRecord pointer.
      ASTNode* result =
          NewCastASTNode(type, syntax->lex->current_token_location, expr);
      if (sym != NULL) {
        SymbolDelete(sym);
      }
      TypeParserDestruct(&parser);
      return result;
    } else {
      // Not a type name.  We have consumed the open paren so we
      // know that it is present.  Set a flag to tell all downstream
      // parsers that we've found and consumed it.
      syntax->found_open_paren = true;
      return ParsePostfixExpression(syntax, followers);
    }
  } else {
    return ParseUnaryExpression(syntax, followers);
  }
}

// Binary operator precedences, loosest first.  Each of these was a function
// that called the next tighter one, so an expression entered eleven frames deep
// before it looked at its first operand -- and a parenthesized subexpression
// paid that again for every level of nesting.
typedef enum {
  kBinaryPrecedenceNone = 0,
  kBinaryPrecedenceLogicalOr,
  kBinaryPrecedenceLogicalAnd,
  kBinaryPrecedenceInclusiveOr,
  kBinaryPrecedenceExclusiveOr,
  kBinaryPrecedenceAnd,
  kBinaryPrecedenceEquality,
  kBinaryPrecedenceRelational,
  kBinaryPrecedenceCompare,
  kBinaryPrecedenceShift,
  kBinaryPrecedenceAdditive,
  kBinaryPrecedenceMultiplicative,
} BinaryPrecedence;

// The operator a token introduces, or none if it introduces no binary operator
// here.  Inside a template-argument list a top-level '>' closes the list and
// '>>' closes two nested ones, so neither is an operator there; one that is
// genuinely wanted has to be parenthesized, which clears the flag (see
// SyntaxNeedTemplateClose).  '>>=' is never a binary operator, so the same
// handoff happens for it without a case of its own.
static BinaryPrecedence BinaryOperatorPrecedence(Syntax* syntax, Token token,
                                                 ASTOpcode* op) {
  switch (token) {
    case TOK(star):
      *op = AST_OP(mult);
      return kBinaryPrecedenceMultiplicative;
    case TOK(slash):
      *op = AST_OP(div);
      return kBinaryPrecedenceMultiplicative;
    case TOK(percent):
      *op = AST_OP(mod);
      return kBinaryPrecedenceMultiplicative;
    case TOK(plus):
      *op = AST_OP(plus);
      return kBinaryPrecedenceAdditive;
    case TOK(minus):
      *op = AST_OP(minus);
      return kBinaryPrecedenceAdditive;
    case TOK(lessless):
      *op = AST_OP(lshift);
      return kBinaryPrecedenceShift;
    case TOK(greatergreater):
      if (syntax->parsing_template_argument) {
        return kBinaryPrecedenceNone;
      }
      *op = AST_OP(rshift);
      return kBinaryPrecedenceShift;
    // C++20 three-way comparison binds tighter than the relational operators
    // and looser than the shift operators.
    case TOK(spaceship):
      *op = AST_OP(spaceship);
      return kBinaryPrecedenceCompare;
    case TOK(less):
      *op = AST_OP(less);
      return kBinaryPrecedenceRelational;
    case TOK(lesseq):
      *op = AST_OP(lesseq);
      return kBinaryPrecedenceRelational;
    case TOK(greater):
      if (syntax->parsing_template_argument) {
        return kBinaryPrecedenceNone;
      }
      *op = AST_OP(greater);
      return kBinaryPrecedenceRelational;
    case TOK(greatereq):
      *op = AST_OP(greatereq);
      return kBinaryPrecedenceRelational;
    case TOK(equalequal):
      *op = AST_OP(equal);
      return kBinaryPrecedenceEquality;
    case TOK(bangeq):
      *op = AST_OP(noteq);
      return kBinaryPrecedenceEquality;
    case TOK(amp):
      *op = AST_OP(and);
      return kBinaryPrecedenceAnd;
    case TOK(caret):
      *op = AST_OP(exor);
      return kBinaryPrecedenceExclusiveOr;
    case TOK(bar):
      *op = AST_OP(bitor);
      return kBinaryPrecedenceInclusiveOr;
    case TOK(ampamp):
      *op = AST_OP(logand);
      return kBinaryPrecedenceLogicalAnd;
    case TOK(barbar):
      *op = AST_OP(logor);
      return kBinaryPrecedenceLogicalOr;
    default:
      return kBinaryPrecedenceNone;
  }
}

// Parse an expression made of binary operators binding at least as tightly as
// `min_precedence`.  Every one of them is left associative, so the right
// operand takes only the operators that bind more tightly, and the recursion
// that collects them is bounded by the number of precedence levels rather than
// by the length or nesting of the expression.
static ASTNode* ParseBinaryExpression(Syntax* syntax, TokenClass followers,
                                      BinaryPrecedence min_precedence) {
  ASTNode* result = ParseCastExpression(syntax, followers);
  for (;;) {
    ASTOpcode op = 0;
    BinaryPrecedence precedence =
        BinaryOperatorPrecedence(syntax, syntax->lex->current_token, &op);
    if (precedence == kBinaryPrecedenceNone || precedence < min_precedence) {
      return result;
    }
    LexNextToken(syntax->lex);
    ASTNode* right =
        ParseBinaryExpression(syntax, followers, precedence + 1);
    result = NewBinaryASTNode(op, NULL, syntax->lex->current_token_location,
                              result, right);
  }
}

static ASTNode* ParseConditionalExpression(Syntax* syntax,
                                           TokenClass followers) {
  ASTNode* result =
      ParseBinaryExpression(syntax, followers, kBinaryPrecedenceLogicalOr);
  if (LexMatch(syntax->lex, TOK(question))) {
    ASTNode* left = SyntaxParseExpression(syntax, followers);
    ASTNode* right = NULL;
    if (LexMatch(syntax->lex, TOK(colon))) {
      right = ParseConditionalExpression(syntax, followers);
    } else {
      // `cond ? a :: b` is a common typo for `cond ? a : b`.  Consume `::` as
      // `:` when it is still in the token stream; otherwise parse a leftover
      // literal-like expression as the false arm so the `?:` tree stays
      // well-formed.  Identifiers are also statements, so they are left for
      // declaration/statement recovery rather than swallowed as an arm.
      SyntaxError(syntax, "Missing : in conditional expression");
      if (LexMatch(syntax->lex, TOK(coloncolon))) {
        right = ParseConditionalExpression(syntax, followers);
      } else {
        TokenClass classes = ClassifyToken(syntax->lex->current_token);
        if ((classes & TC(expr)) != 0 &&
            (classes & (TC(stmt) | TC(decl) | TC(type))) == 0) {
          right = ParseConditionalExpression(syntax, followers);
        }
      }
    }
    right =
        NewBinaryASTNode(AST_OP(colon), NULL,
                         syntax->lex->current_token_location, left, right);
    result =
        NewBinaryASTNode(AST_OP(question), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTOpcode AssignASTOpcode(Syntax* syntax, Token tok) {
  switch (tok) {
    case TOK(equal):
      return AST_OP(assign);
    case TOK(pluseq):
      return AST_OP(pluseq);
    case TOK(minuseq):
      return AST_OP(minuseq);
    case TOK(stareq):
      return AST_OP(multeq);
    case TOK(slasheq):
      return AST_OP(diveq);
    case TOK(percenteq):
      return AST_OP(percenteq);
    case TOK(lesslesseq):
      return AST_OP(lshifteq);
    case TOK(greatergreatereq):
      return AST_OP(rshifteq);
    case TOK(ampeq):
      return AST_OP(andeq);
    case TOK(bareq):
      return AST_OP(oreq);
    case TOK(careteq):
      return AST_OP(exoreq);
    default:
      assert(false);
      return 0;
  }
}

static ASTNode* ParseAssignmentExpression(Syntax* syntax,
                                          TokenClass followers) {
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(co_yield))) {
    return ParseCXXCoYieldExpression(syntax, followers);
  }

  ASTNode* result = ParseConditionalExpression(syntax, followers);
  if (syntax->lex->preprocessor_mode || syntax->lex->assembler_mode) {
    return result;
  }
  switch (syntax->lex->current_token) {
    case TOK(equal):
    case TOK(pluseq):
    case TOK(minuseq):
    case TOK(stareq):
    case TOK(slasheq):
    case TOK(percenteq):
    case TOK(lesslesseq):
    case TOK(greatergreatereq):
    case TOK(ampeq):
    case TOK(bareq):
    case TOK(careteq): {
      Token tok = syntax->lex->current_token;
      LexNextToken(syntax->lex);
      // In C++, the right-hand side of a simple assignment may be a
      // braced-init-list (`x = {}`, `x = {1, 2}`).  A braced-init-list is not
      // an expression, so parse it explicitly here; semantic analysis lowers it
      // to a temporary of the left-hand side's type.  Only `=` accepts it as a
      // complete right-hand side, so anything trailing the braces (e.g.
      // `x = {} + 1`) remains and is correctly rejected as a syntax error.
      ASTNode* right =
          (CompilerIsCXX() && tok == TOK(equal) &&
           LexMatch(syntax->lex, TOK(lbrace)))
              ? SyntaxParseBracedInitializer(syntax)
              : ParseAssignmentExpression(syntax, followers);
      result = NewBinaryASTNode(AssignASTOpcode(syntax, tok), NULL,
                                syntax->lex->current_token_location, result,
                                right);
      break;
    }
    default:
      break;
  }
  return result;
}

ASTNode* SyntaxParseExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseAssignmentExpression(syntax, followers | TC(exprsep));
  while (LexMatch(syntax->lex, TOK(comma))) {
    ASTNode* right = ParseAssignmentExpression(syntax, followers | TC(exprsep));
    result =
        NewBinaryASTNode(AST_OP(comma), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

ASTNode* SyntaxParseSingleExpression(Syntax* syntax, TokenClass followers) {
  return ParseAssignmentExpression(syntax, followers);
}

ASTNode* SyntaxParseConditionalExpression(Syntax* syntax,
                                          TokenClass followers) {
  return ParseConditionalExpression(syntax, followers);
}

ASTNode* SyntaxParseConstraintExpression(Syntax* syntax,
                                         TokenClass followers) {
  return ParseBinaryExpression(syntax, followers, kBinaryPrecedenceLogicalOr);
}
