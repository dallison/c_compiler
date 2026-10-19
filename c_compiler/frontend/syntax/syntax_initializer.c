//
//  syntax_initializer.c
//  c_compiler
//
//  Initializers: braced and designated initializer lists, parameter-pack
//  expansions in them, and C++ direct and default object construction.
//

#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "concepts.h"
#include "constexpr.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "macro_expansion.h"
#include "typo_correction.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_inheritance.h"
#include "type_internal.h"
#include "type_member.h"
#include "type_special_member.h"
#include "errors.h"
#include "compiler.h"
#include "module_identity.h"
#include "module_syntax.h"
#include "reflection_semantics.h"
#include "syntax_internal.h"

// A designator index is kept in an int, and the size of the array it
// designates is an int as well.  A value that does not fit would be truncated
// into an unrelated index, so it is rejected here and the initializer it
// carries is attached to element zero to keep parsing.
static int DesignatorArrayIndex(Syntax* syntax, int64_t value) {
  if (value < INT_MIN || value > INT_MAX) {
    SyntaxError(syntax, "Array designator index %lld is out of range",
                (long long)value);
    return 0;
  }
  return (int)value;
}

// Clone a designator list, substituting a single array index at position
// range_pos (used to expand a [start ... end] range designator).
static Vector* CloneDesignators(Vector* src, int range_pos, int index_value) {
  Vector* dst = NewVector();
  for (size_t i = 0; i < src->length; i++) {
    Designator* d = src->value.p[i];
    if (d->designator_type == kDesignatorArray) {
      int v = ((int)i == range_pos) ? index_value : d->value.array_index;
      VectorAppend(dst, NewArrayDesignator(d->type, v));
    } else {
      VectorAppend(dst, NewStructDesignator(
                            NewString(d->value.struct_member_name->value)));
    }
  }
  return dst;
}

// Parses one designated initializer (designators, then `=` or a braced
// clause), appends the resulting AST to `initializers`, and expands GCC
// `[start ... end]` ranges into one entry per index.  Consumes tokens and
// may emit syntax errors.
static void ParseDesignatedInitializer(Syntax* syntax,
                                           Vector* initializers) {
  Vector* designators = NewVector();
  SourceLocation location = syntax->lex->current_token_location;
  
  // Collect all designators.  Each is either an array or struct designator.
  // The array designator is a constant expression enclosed in square
  // brackets. The struct designator is a dot followed by a struct member
  // name.
  while (LexLookingAt(syntax->lex, TOK(lsquare)) ||
         LexLookingAt(syntax->lex, TOK(dot))) {
    if (LexMatch(syntax->lex, TOK(lsquare))) {
      // Array designator.
      ASTNode* index_expr = SyntaxParseExpression(syntax, TC(semicolon));
      index_expr = AnalyzeExpression(index_expr);
      int64_t value;
      bool ok = EvaluateIntegerExpression(index_expr, &value);
      if (!ok) {
        SyntaxError(syntax,
              "Need constant expression inside [] in designated initializer");
        value = 0;
      }
      // GCC range designator: [ start ... end ].
      int64_t end_value = value;
      if (LexMatch(syntax->lex, TOK(ellipsis))) {
        ASTNode* end_expr = SyntaxParseExpression(syntax, TC(semicolon));
        end_expr = AnalyzeExpression(end_expr);
        if (!EvaluateIntegerExpression(end_expr, &end_value)) {
          SyntaxError(syntax,
                "Need constant expression inside [] in designated initializer");
          end_value = value;
        }
      }
      SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
      int index = DesignatorArrayIndex(syntax, value);
      Designator* d = NewArrayDesignator(NULL, index);
      d->array_index_end =
          end_value == value ? index : DesignatorArrayIndex(syntax, end_value);
      VectorAppend(designators, d);
    } else if (LexMatch(syntax->lex, TOK(dot))) {
      // Struct designator.  The dot is followed by a struct member name.
      String* member_name;
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        member_name = NewString(syntax->lex->spelling.value);
        LexNextToken(syntax->lex);
      } else {
        SyntaxError(syntax,
                    "Need member name after . in designated initializer");
        member_name = NewString(SyntaxFakeName(syntax));
      }
      VectorAppend(designators, NewStructDesignator(member_name));
    }
  }
  // This is followed by either `= initializer-clause` or a direct braced
  // initializer.  C++ designated initializers permit both `.member = value`
  // and `.member { value }`.
  bool has_equal = LexMatch(syntax->lex, TOK(equal));
  bool has_direct_braces =
      !has_equal && LexMatch(syntax->lex, TOK(lbrace));
  if (!has_equal && !has_direct_braces) {
    SyntaxError(syntax, "Expected = or { in designated initializer");
    SyntaxRecover(syntax, TC(exprsep));
  } else {
    ASTNode* init;
    // The initialization expression is either a brace-enclosed
    // initializer or a single expression.
    if (has_direct_braces || LexMatch(syntax->lex, TOK(lbrace))) {
      init = ParseBracedInitializer(syntax);
    } else {
      init = NewExpressionInitializerASTNode(
                                             SyntaxParseSingleExpression(syntax, TC(exprsep)), location);
    }
    // If one of the designators is a [start ... end] range, expand it into one
    // designated initializer per index, cloning the value for each.
    int range_pos = -1;
    for (size_t i = 0; i < designators->length; i++) {
      Designator* d = designators->value.p[i];
      if (d->designator_type == kDesignatorArray &&
          d->array_index_end > d->value.array_index) {
        range_pos = (int)i;
        break;
      }
    }
    if (range_pos >= 0) {
      Designator* range = designators->value.p[range_pos];
      int start = range->value.array_index;
      int end = range->array_index_end;
      // Expanding a [start ... end] range clones the initializer once per
      // index.  A range that would materialize millions of AST nodes is
      // rejected the same way a single huge designator used to be: diagnose
      // and keep a single slot so parsing can continue.
      if (end > start && (int64_t)end - (int64_t)start >= 65536) {
        SyntaxError(syntax,
                    "Array designator range [%d ... %d] is larger than this "
                    "compiler can lay out",
                    start, end);
        end = start;
      }
      for (int v = start; v <= end; v++) {
        Vector* desigs = CloneDesignators(designators, range_pos, v);
        ASTNode* init_for_index = (v == end) ? init : CloneInitializer(init);
        ASTNode* designated = NewDesignatedInitializerASTNode(
            desigs, init_for_index, location);
        designated->flags |= kASTSourceDesignatedInitializer;
        VectorAppend(initializers, designated);
      }
      VectorDelete(designators);
    } else {
      ASTNode* designated =
          NewDesignatedInitializerASTNode(designators, init, location);
      designated->flags |= kASTSourceDesignatedInitializer;
      VectorAppend(initializers, designated);
    }
  }

}

typedef struct {
  bool found;
} CXXPackExpressionSearch;

// Returns true when `argument` or any nested pack argument or type operand
// refers to a template parameter pack.
static bool CXXTemplateArgumentReferencesParameterPack(
    TemplateArgument* argument) {
  if (argument == NULL) {
    return false;
  }
  if (argument->references_parameter_pack) {
    return true;
  }
  if (argument->template_parameter_index >= 0 &&
      CurrentTemplateParameterIsPack(&compiler->syntax,
                                    argument->template_parameter_index)) {
    return true;
  }
  for (size_t i = 0;
       argument->pack_arguments != NULL &&
       i < argument->pack_arguments->length; i++) {
    if (CXXTemplateArgumentReferencesParameterPack(
            argument->pack_arguments->value.p[i])) {
      return true;
    }
  }
  if (argument->type != NULL &&
      CXXTypeContainsParameterPack(&compiler->syntax, argument->type)) {
    return true;
  }
  return false;
}

// AST visitor pre-order hook: sets `((CXXPackExpressionSearch*)data)->found`
// when `node` references a function parameter pack (directly or via sizeof/
// alignof or template arguments).
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
  if ((node->op == AST_OP(sizeof) || node->op == AST_OP(alignof)) &&
      CXXTypeContainsParameterPack(
          &compiler->syntax, ((SizeofASTNode*)node)->type_operand)) {
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
    if (id->symbol != NULL &&
        CXXTypeContainsParameterPack(&compiler->syntax, id->symbol->type)) {
      ((CXXPackExpressionSearch*)data)->found = true;
      return;
    }
    template_arguments = id->template_arguments;
  } else if (shape == kASTShapeStructMember) {
    template_arguments =
        ((StructMemberASTNode*)node)->template_arguments;
  } else if (shape == kASTShapeConstant) {
    template_arguments = ((ConstantASTNode*)node)->template_arguments;
  }
  for (size_t i = 0;
       template_arguments != NULL && i < template_arguments->length; i++) {
    if (CXXTemplateArgumentReferencesParameterPack(
            template_arguments->value.p[i])) {
      ((CXXPackExpressionSearch*)data)->found = true;
      return;
    }
  }
}

// Returns true if `node`'s expression tree references a C++ parameter pack.
bool SyntaxExpressionContainsParameterPack(ASTNode* node) {
  CXXPackExpressionSearch search = {0};
  ASTNodeVisit(node, FindCXXParameterPackExpression, 0, &search);
  return search.found;
}

// Returns true if any link in `type` or its template arguments depends on a
// pack template parameter or on an expression that contains a pack.
bool CXXTypeContainsParameterPack(Syntax* syntax, TypeRecord* type) {
  for (TypeRecord* current = type; current != NULL;
       current = current->next) {
    int parameter_index = -1;
    TypeIsTemplateParameterPlaceholder(current, &parameter_index);
    if (parameter_index < 0 &&
        TypeDependentOf(current)->dependent_member_name != NULL) {
      parameter_index = current->template_parameter_index;
    }
    if (parameter_index >= 0 &&
        CurrentTemplateParameterIsPack(syntax, parameter_index)) {
      return true;
    }
    if (current->template_parameter_index >= 0 &&
        current->template_parameter_index != parameter_index &&
        CurrentTemplateParameterIsPack(syntax,
                                       current->template_parameter_index)) {
      return true;
    }
    if (TypeDependentOf(current)->dependent_decltype_expr != NULL &&
        SyntaxExpressionContainsParameterPack(
            TypeDependentOf(current)->dependent_decltype_expr)) {
      return true;
    }
    for (size_t i = 0;
         current->template_arguments != NULL &&
         i < current->template_arguments->length; i++) {
      if (CXXTemplateArgumentReferencesParameterPack(
              current->template_arguments->value.p[i])) {
        return true;
      }
    }
  }
  return false;
}

// When the next token is `...`, requires `expr` to reference a parameter
// pack, sets `kASTPackExpansion` on it, and otherwise diagnoses; consumes
// the ellipsis when present.
static void MarkCXXPackExpansionIfPresent(Syntax* syntax, ASTNode* expr) {
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(ellipsis))) {
    return;
  }
  if (!SyntaxExpressionContainsParameterPack(expr)) {
    SyntaxError(syntax, "pack expansion requires a function parameter pack");
  }
  expr->flags |= kASTPackExpansion;
}

// At a `[` that begins an initializer clause.  C++ has no array designators,
// so there it is a lambda unless the bracket is followed by what can only
// continue GNU's `[index] = value` designation (`=`, `[`, or `.`).
static bool LookingAtArrayDesignator(Syntax* syntax) {
  if (!CompilerIsCXX()) {
    return true;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  int depth = 0;
  do {
    if (LexLookingAt(syntax->lex, TOK(lsquare)) ||
        LexLookingAt(syntax->lex, TOK(lparen)) ||
        LexLookingAt(syntax->lex, TOK(lbrace))) {
      depth++;
    } else if (LexLookingAt(syntax->lex, TOK(rsquare)) ||
               LexLookingAt(syntax->lex, TOK(rparen)) ||
               LexLookingAt(syntax->lex, TOK(rbrace))) {
      depth--;
    } else if (LexLookingAt(syntax->lex, TOK(eof))) {
      break;
    }
    LexNextToken(syntax->lex);
  } while (depth > 0);
  bool designator = LexLookingAt(syntax->lex, TOK(equal)) ||
                    LexLookingAt(syntax->lex, TOK(lsquare)) ||
                    LexLookingAt(syntax->lex, TOK(dot));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return designator;
}

// Parse a brace-enclosed initializer.
ASTNode* ParseBracedInitializer(Syntax* syntax) {
  Vector* initializers = NewVector();
  SourceLocation location = syntax->lex->current_token_location;
  while (!LexLookingAt(syntax->lex, TOK(rbrace))) {
    if (LexMatch(syntax->lex, TOK(lbrace))) {
      // Braced initializer.
      VectorAppend(initializers, ParseBracedInitializer(syntax));
    } else if ((LexLookingAt(syntax->lex, TOK(lsquare)) &&
                LookingAtArrayDesignator(syntax)) ||
               LexLookingAt(syntax->lex, TOK(dot))) {
      // Designated initializer.
      ParseDesignatedInitializer(syntax, initializers);
    } else {
      // Not a designated initializer, expression.
      ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(exprsep));
      MarkCXXPackExpansionIfPresent(syntax, expr);
      VectorAppend(initializers,
                   NewExpressionInitializerASTNode(expr, location));
    }
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebra));
  return NewBracedInitializerASTNode(initializers, NULL, location);
}

// Records that a braced initializer directly followed its declarator.  C++
// placeholder deduction distinguishes `auto x{value}` from
// `auto x = {value}`, so this source-level distinction must survive template
// cloning and deferred semantic analysis.
ASTNode* MarkDirectListInitializer(ASTNode* initializer) {
  if (initializer != NULL && initializer->op == AST_OP(braced_init)) {
    ((BracedInitializerASTNode*)initializer)->is_direct_list_initializer = true;
  }
  return initializer;
}

// Parses a brace-enclosed initializer and returns its AST (same as
// ParseBracedInitializer).
ASTNode* SyntaxParseBracedInitializer(Syntax* syntax) {
  return ParseBracedInitializer(syntax);
}

// Parses a symbol initializer.
ASTNode* SyntaxParseInitializer(Syntax* syntax, Symbol* sym, Storage storage) {
  /* `extern T x = v;` is a definition ([basic.def]/2, C11 6.9.2p1); C++ uses
   * it to give a namespace-scope const object external linkage. */
  if (StorageIs(storage, STO(extern)) && !CompilerIsCXX()) {
    SyntaxWarning(syntax, "extern-initializer", "extern with initializer");
  }
  if (StorageIs(storage, STO(typedef))) {
    SyntaxError(syntax, "typdefs can't have initializers");
  }
  sym->flags.is_defined = true;
  if (LexMatch(syntax->lex, TOK(lbrace))) {
    // Braced initializer.
    return ParseBracedInitializer(syntax);
  }

  SourceLocation location = syntax->lex->current_token_location;
  return NewExpressionInitializerASTNode(
      SyntaxParseSingleExpression(syntax, TC(semicolon) | TC(stmt)), location);
}

// Parses an in-class default member initializer after `=` or `{`; returns
// NULL when neither token is present or when not compiling C++.  Consumes
// the initializer tokens when parsing succeeds.
ASTNode* SyntaxParseCXXDefaultMemberInitializer(Syntax* syntax) {
  if (!CompilerIsCXX()) {
    return NULL;
  }
  bool braced = LexMatch(syntax->lex, TOK(lbrace));
  if (!braced && !LexMatch(syntax->lex, TOK(equal))) {
    return NULL;
  }
  // Copy-list-initialization (`T x = {}`) is a braced-init-list after `=`,
  // not an expression.  SyntaxParseSingleExpression would reject `{`.
  if (!braced) {
    braced = LexMatch(syntax->lex, TOK(lbrace));
  }
  SourceLocation location = syntax->lex->current_token_location;
  bool saved = syntax->parsing_default_member_initializer;
  syntax->parsing_default_member_initializer = true;
  ASTNode* result;
  if (braced) {
    result = ParseBracedInitializer(syntax);
  } else {
    ASTNode* expr =
        SyntaxParseSingleExpression(syntax, TC(semicolon) | TC(exprsep));
    result = NewExpressionInitializerASTNode(expr, location);
  }
  syntax->parsing_default_member_initializer = saved;
  return result;
}

// Returns the class tag name used as the constructor's member name, or NULL
// when `type` is not a named class type.
const char* CXXConstructorNameForType(TypeRecord* type) {
  if (!CompilerIsCXX() || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  return type->info.struct_info->tag_name->value;
}

// Builds a call to `sym`'s class constructor with `actuals` as arguments;
// prepends the implicit object parameter (and complete-object flag when
// needed).  Takes ownership of `actuals`; on failure deletes `actuals` and
// returns NULL.
ASTNode* SyntaxNewCXXConstructorCall(Syntax* syntax, Symbol* sym,
                                     Vector* actuals,
                                     SourceLocation location) {
  (void)syntax;
  const char* constructor_name = CXXConstructorNameForType(sym->type);
  if (constructor_name == NULL) {
    VectorDelete(actuals);
    return NULL;
  }

  CXXPrependCompleteObjectArgument(sym->type, actuals,
                                   /*complete_object=*/true, location);
  ASTNode* receiver = NewIdentifierASTNode(sym, location);
  ASTNode* member = NewStringConstantASTNode(NewString(constructor_name), NULL,
                                            location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
}

// Returns the declared constructor overload set for class `type`, or NULL
// when the type has no usable constructor name or member.
StructMember* FindCXXConstructor(TypeRecord* type) {
  const char* constructor_name = CXXConstructorNameForType(type);
  if (constructor_name == NULL) {
    return NULL;
  }
  StructMember* ctor =
      FindStructMemberByName(type->info.struct_info, constructor_name);
  if (ctor == NULL || !ctor->is_member_function ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  return ctor;
}

// True if `type` is a class type that has a (non-deleted) default constructor:
// one callable with no explicit arguments.  A constructor whose only parameters
// are the implicit object parameter (and, for a class with virtual bases, the
// most-derived flag) qualifies; a class whose every constructor takes explicit
// arguments (e.g. `Point(int, int)`) does not.  Used so a data member without
// an explicit initializer is default-constructed only when that is actually
// well-formed -- never for a non-default-constructible member of an aggregate,
// which is aggregate-initialized instead.
bool CXXTypeHasDefaultConstructor(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  StructMember* ctor = FindCXXConstructor(type);
  if (ctor == NULL) {
    return false;
  }
  size_t expected = 1;
  if (StructHasVirtualBases(type->info.struct_info)) {
    expected++;
  }
  for (StructMember* candidate = ctor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type)) {
      continue;
    }
    FunctionInfo* info = &candidate->symbol->type->info.function;
    if (!info->is_constructor || info->is_deleted) {
      continue;
    }
    bool callable_without_arguments = info->prototype.length >= expected;
    for (size_t j = expected;
         callable_without_arguments && j < info->prototype.length; j++) {
      Symbol* parameter = info->prototype.value.p[j];
      if (parameter == NULL ||
          SymbolExtraOf(parameter)->default_argument == NULL) {
        callable_without_arguments = false;
      }
    }
    if (callable_without_arguments) {
      return true;
    }
  }
  return false;
}

// Strips fixed-array dimensions from `type`, multiplies their sizes into
// `*count` when `count` is non-NULL, and returns the underlying element type.
TypeRecord* CXXArrayBaseElementType(TypeRecord* type,
                                    unsigned long* count) {
  unsigned long elements = 1;
  while (TypeIsFixedArray(type)) {
    elements *= (unsigned long)type->info.array.size.fixed;
    type = type->next;
  }
  if (count != NULL) {
    *count = elements;
  }
  return type;
}

// Builds an lvalue expression for element `flat_index` of array variable
// `array`, using nested subscripts through each array dimension.
ASTNode* NewCXXArrayElementExpression(Symbol* array,
                                      unsigned long flat_index,
                                      SourceLocation location) {
  ASTNode* expression = NewIdentifierASTNode(array, location);
  TypeRecord* array_type = array->type;
  while (TypeIsFixedArray(array_type)) {
    unsigned long lower_elements = 1;
    for (TypeRecord* lower = array_type->next; TypeIsFixedArray(lower);
         lower = lower->next) {
      lower_elements *= (unsigned long)lower->info.array.size.fixed;
    }
    unsigned long index = flat_index / lower_elements;
    flat_index %= lower_elements;
    expression = NewBinaryASTNode(
        AST_OP(subscript), TypeRecordCopy(array_type->next), location,
        expression,
        NewIntConstantASTNode(
            index, NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain),
            location));
    array_type = array_type->next;
  }
  return expression;
}

// Returns true if any overload in constructor set `ctor` is an
// initializer-list constructor (takes std::initializer_list or equivalent).
bool CXXConstructorSetHasInitializerList(StructMember* ctor) {
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
    if (CXXConstructorIsInitializerListConstructor(info)) {
      return true;
    }
  }
  return false;
}

// Returns true when class `type` has an initializer-list constructor.
bool TypeHasCXXInitializerListConstructor(TypeRecord* type) {
  return CXXConstructorSetHasInitializerList(FindCXXConstructor(type));
}

// For copy-list initialization of non-aggregate class `sym`, replaces a
// braced initializer with a constructor call when appropriate; otherwise
// returns `initializer` unchanged.  May consume braced-init elements into
// constructor `actuals`.
ASTNode* NewCXXCopyListConstructorInitializer(Syntax* syntax,
                                                    Symbol* sym,
                                                    ASTNode* initializer) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      initializer == NULL || initializer->op != AST_OP(braced_init) ||
      !TypeIsStructOrUnion(sym->type) || sym->type->info.struct_info == NULL ||
      sym->type->info.struct_info->is_aggregate) {
    return initializer;
  }
  StructMember* constructor = FindCXXConstructor(sym->type);
  if (constructor == NULL) {
    return initializer;
  }
  Vector* actuals = NewVector();
  if (CXXConstructorSetHasInitializerList(constructor)) {
    VectorAppend(actuals, initializer);
    return SyntaxNewCXXConstructorCall(syntax, sym, actuals,
                                       initializer->location);
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init != NULL && init->op == AST_OP(expr_init)) {
      VectorAppend(actuals, ((ExpressionInitializerASTNode*)init)->expr);
    } else if (init != NULL) {
      VectorAppend(actuals, init);
    }
  }
  braced->initializers->length = 0;
  return SyntaxNewCXXConstructorCall(syntax, sym, actuals,
                                     initializer->location);
}

// Parses a comma-separated initializer argument list terminated by `close`
// (expression or braced initializer per element) and returns the AST nodes;
// caller owns the vector.  Consumes tokens through the closing delimiter.
Vector* ParseCXXInitializerArgumentList(Syntax* syntax, Token close) {
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, close)) {
    ASTNode* actual = LexMatch(syntax->lex, TOK(lbrace))
                          ? SyntaxParseBracedInitializer(syntax)
                          : SyntaxParseSingleExpression(syntax,
                                                        TC(closebra) |
                                                            TC(exprsep));
    MarkCXXPackExpansionIfPresent(syntax, actual);
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, close, TC(exprsep) | TC(decl));
  return actuals;
}

// Runs semantic analysis on each expression in `actuals`, replacing slots
// in place with analyzed nodes.
static void AnalyzeCXXInitializerArgumentTypes(Vector* actuals) {
  for (size_t i = 0; actuals != NULL && i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    if (actual != NULL) {
      VectorSet(actuals, i, AnalyzeExpression(actual));
    }
  }
}

// When `sym` has a class-template placeholder type, deduces template
// arguments from `actuals`, updates `sym`'s type on success, and diagnoses
// on failure.  Returns false when deduction fails (non-placeholder types
// succeed trivially).
static bool ResolveCXXClassTemplateArgumentDeduction(Syntax* syntax,
                                                     Symbol* sym,
                                                     Vector* actuals,
                                                     bool allow_explicit) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      !TypeIsClassTemplatePlaceholder(sym->type)) {
    return true;
  }
  Symbol* class_template = CXXClassTemplateOrigin(sym->type);
  AnalyzeCXXInitializerArgumentTypes(actuals);
  bool alias_rejected = false;
  TypeRecord* deduced = TypeDeduceClassTemplateFromPlaceholder(
      syntax, sym->type, actuals, allow_explicit, &alias_rejected);
  if (deduced == NULL) {
    if (alias_rejected) {
      SyntaxError(syntax,
                  "Deduced template arguments do not match alias template");
    } else {
      SyntaxError(syntax, "Could not deduce template arguments for %s",
                  class_template != NULL ? class_template->name.value
                                         : "<class template>");
    }
    return false;
  }
  if (!TypeClassTemplatePlaceholderAcceptsDeduced(sym->type, deduced)) {
    SyntaxError(syntax,
                "Deduced template arguments do not match alias template");
    TypeRecordDelete(deduced);
    return false;
  }
  SymbolSetType(sym, deduced);
  return true;
}

// Builds the argument vector used for class-template deduction from
// `initializer`, optionally keeping a braced init as one argument; may
// analyze expression inits in place.  Caller owns the returned vector.
static Vector* CXXDeductionActualsFromInitializer(ASTNode* initializer,
                                                  bool preserve_braced) {
  if (initializer == NULL) {
    return NULL;
  }
  Vector* actuals = NewVector();
  if (initializer->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)initializer;
    expr_init->expr = AnalyzeExpression(expr_init->expr);
    VectorAppend(actuals, expr_init->expr);
    return actuals;
  }
  if (initializer->op == AST_OP(braced_init)) {
    if (preserve_braced) {
      VectorAppend(actuals, initializer);
      return actuals;
    }
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ASTNode* init = braced->initializers->value.p[i];
      if (init != NULL && init->op == AST_OP(expr_init)) {
        ExpressionInitializerASTNode* expr_init =
            (ExpressionInitializerASTNode*)init;
        expr_init->expr = AnalyzeExpression(expr_init->expr);
        VectorAppend(actuals, expr_init->expr);
      } else if (init != NULL) {
        VectorAppend(actuals, init);
      }
    }
    return actuals;
  }
  VectorAppend(actuals, AnalyzeExpression(initializer));
  return actuals;
}

// Deduces class template arguments for placeholder `sym` from `initializer`
// and updates `sym`'s type; returns false and emits an error when deduction
// fails.
bool ResolveCXXClassTemplateArgumentDeductionFromInitializer(
    Syntax* syntax, Symbol* sym, ASTNode* initializer) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      !TypeIsClassTemplatePlaceholder(sym->type)) {
    return true;
  }
  bool preserve_braced = initializer != NULL &&
                         initializer->op == AST_OP(braced_init) &&
                         TypeHasCXXInitializerListConstructor(sym->type);
  Vector* actuals =
      CXXDeductionActualsFromInitializer(initializer, preserve_braced);
  bool ok = ResolveCXXClassTemplateArgumentDeduction(
      syntax, sym, actuals, /*allow_explicit=*/false);
  VectorDelete(actuals);
  return ok;
}

// Wraps each entry of `actuals` in a braced-initializer AST, clears and
// deletes `actuals`, and returns the new braced initializer (caller owns it).
static ASTNode* NewCXXBracedInitializerFromActuals(Vector* actuals,
                                                   SourceLocation location) {
  Vector* initializers = NewVector();
  for (size_t i = 0; actuals != NULL && i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    if (actual != NULL && actual->op == AST_OP(braced_init)) {
      VectorAppend(initializers, actual);
    } else {
      VectorAppend(initializers,
                   NewExpressionInitializerASTNode(actual, location));
    }
  }
  if (actuals != NULL) {
    actuals->length = 0;
    VectorDelete(actuals);
  }
  return NewBracedInitializerASTNode(initializers, NULL, location);
}

// Parses C++ direct initialization `(...)` or `{...}` for `sym`, performs
// CTAD when needed, and returns an initializer or constructor-call AST, or
// NULL when no direct-init syntax is present or deduction fails.  When
// `wrap_aggregate_braces` is true, may wrap the result for variable init.
ASTNode* ParseCXXDirectInitializer(Syntax* syntax, Symbol* sym,
                                   bool wrap_aggregate_braces) {
  if (!CompilerIsCXX() || sym == NULL) {
    return NULL;
  }
  if (!TypeIsStructOrUnion(sym->type) ||
      (!TypeIsClassTemplatePlaceholder(sym->type) &&
       (sym->type == NULL || sym->type->info.struct_info == NULL))) {
    if (!LexMatch(syntax->lex, TOK(lparen))) {
      return NULL;
    }
    SourceLocation location = syntax->lex->current_token_location;
    Vector* actuals = ParseCXXInitializerArgumentList(syntax, TOK(rparen));
    if (actuals->length == 1 &&
        (!TypeContainsTemplateParameter(sym->type) ||
         ((((ASTNode*)actuals->value.p[0])->flags & kASTPackExpansion) != 0))) {
      ASTNode* initializer = actuals->value.p[0];
      actuals->length = 0;
      VectorDelete(actuals);
      if ((initializer->flags & kASTPackExpansion) != 0) {
        return initializer;
      }
      // An expression initializer is what applies the conversion to the
      // declared type (`long q(obj)` calls obj's conversion function).
      ASTNode* direct = NewExpressionInitializerASTNode(initializer, location);
      ((ExpressionInitializerASTNode*)direct)->is_direct_initializer = true;
      // Code generation stores a local's initializer only through an init
      // node, so build the same tree as `T x = e`.
      if (wrap_aggregate_braces) {
        return NewVariableInitExpression(syntax, sym, direct);
      }
      return direct;
    }
    return NewCXXBracedInitializerFromActuals(actuals, location);
  }
  if (!TypeIsClassTemplatePlaceholder(sym->type) &&
      sym->type->info.struct_info->is_aggregate &&
      LexLookingAt(syntax->lex, TOK(lbrace))) {
    return NULL;
  }
  SourceLocation location = syntax->lex->current_token_location;
  Vector* actuals = NULL;
  bool braced = false;
  if (LexMatch(syntax->lex, TOK(lparen))) {
    actuals = ParseCXXInitializerArgumentList(syntax, TOK(rparen));
  } else if (LexMatch(syntax->lex, TOK(lbrace))) {
    braced = true;
    StructMember* constructor = FindCXXConstructor(sym->type);
    if (CXXConstructorSetHasInitializerList(constructor)) {
      actuals = NewVector();
      VectorAppend(actuals, SyntaxParseBracedInitializer(syntax));
    } else {
      actuals = ParseCXXInitializerArgumentList(syntax, TOK(rbrace));
    }
  } else {
    return NULL;
  }
  if (!ResolveCXXClassTemplateArgumentDeduction(
          syntax, sym, actuals, /*allow_explicit=*/true)) {
    VectorDeleteWithContents(actuals, (VectorElementDestructor)ASTNodeDelete,
                             /*free_element=*/false);
    return NULL;
  }
  StructMember* constructor = FindCXXConstructor(sym->type);
  // C++20 parenthesized aggregate initialization ([dcl.init]/16.6.2.5): a
  // parenthesized expression-list initializing an aggregate is treated as
  // aggregate initialization.  An aggregate has only implicit special-member
  // constructors, and aggregate init's single-element-of-same-type rule already
  // reproduces the copy/move case, so routing the whole parenthesized list
  // through aggregate init matches "consider constructors first, fall back to
  // aggregate init".  (The type is concrete here: any CTAD placeholder has been
  // resolved by ResolveCXXClassTemplateArgumentDeduction above.)
  //
  // A class with any user-declared (non-invented) constructor is never an
  // aggregate.  The `is_aggregate` flag on a class template's *primary* struct
  // is unreliable (it can be stale/true even though the class has constructors),
  // and the injected-class-name inside a member body can still refer to that
  // primary struct, so verify structurally rather than trusting the flag alone.
  bool has_user_declared_constructor = false;
  for (StructMember* c = constructor; c != NULL; c = c->overload_next) {
    if (c->is_member_function && c->symbol != NULL &&
        !c->symbol->flags.invented) {
      has_user_declared_constructor = true;
      break;
    }
  }
  bool paren_aggregate_init =
      !braced && !TypeIsClassTemplatePlaceholder(sym->type) &&
      sym->type->info.struct_info != NULL &&
      sym->type->info.struct_info->is_aggregate &&
      !sym->type->info.struct_info->is_template &&
      !has_user_declared_constructor;
  // A dependent specialization whose primary template is only declared
  // (`template <class F> class Action; Action<R(int)> a(f);`) shows no
  // constructors yet.  Its arguments are kept in a braced container that
  // instantiation rebuilds into the constructor call.
  bool dependent_arguments = !braced && constructor == NULL &&
                             TypeContainsTemplateParameter(sym->type);
  if (constructor == NULL ||
      (braced && sym->type->info.struct_info->is_aggregate) ||
      paren_aggregate_init) {
    ASTNode* braced_initializer =
        (braced || paren_aggregate_init || dependent_arguments)
            ? NewCXXBracedInitializerFromActuals(actuals, location)
            : NULL;
    if (braced_initializer == NULL) {
      VectorDeleteWithContents(actuals, (VectorElementDestructor)ASTNodeDelete,
                               /*free_element=*/false);
    }
    if (braced_initializer != NULL && wrap_aggregate_braces &&
        !dependent_arguments) {
      return NewVariableInitExpression(syntax, sym, braced_initializer);
    }
    return braced_initializer;
  }
  return SyntaxNewCXXConstructorCall(syntax, sym, actuals, location);
}

// Returns a statement or expression that default-constructs `sym` when its
// type requires a non-trivial default constructor (including each element of
// an array), or NULL when no call is needed or the type is not constructible.
ASTNode* SyntaxNewCXXDefaultConstructorCallIfNeeded(Syntax* syntax,
                                                    Symbol* sym) {
  if (sym == NULL || StorageIs(sym->storage, STO(typedef))) {
    return NULL;
  }
  if (TypeIsFixedArray(sym->type)) {
    unsigned long element_count = 0;
    TypeRecord* element_type =
        CXXArrayBaseElementType(sym->type, &element_count);
    const char* constructor_name =
        CXXConstructorNameForType(element_type);
    if (constructor_name == NULL || element_type->info.struct_info == NULL) {
      return NULL;
    }
    StructMember* ctor = FindStructMemberByName(
        element_type->info.struct_info, constructor_name);
    if (ctor == NULL || !ctor->is_member_function ||
        !ctor->symbol->type->info.function.is_constructor) {
      return NULL;
    }
    if (element_type->info.struct_info->is_aggregate &&
        ctor->symbol->flags.invented &&
        (ctor->symbol->type->info.function.is_deleted ||
         ctor->symbol->type->info.function.is_trivial_special_member)) {
      return NULL;
    }
    if (element_count > kCXXUnrolledArrayElementLimit) {
      return NewUnaryASTNode(
          AST_OP(stmt_expr), NewTypeRecordWithSize(kTypeVoid, kQualPlain),
          sym->location,
          NewCXXArrayElementLoop(
              syntax, NewCXXArrayElementExpression(sym, 0, sym->location),
              element_type, element_count, constructor_name,
              /*reverse=*/false, sym->location));
    }
    ASTNode* sequence = NULL;
    for (unsigned long i = 0; i < element_count; i++) {
      SourceLocation location = sym->location;
      ASTNode* element =
          NewCXXArrayElementExpression(sym, i, location);
      ASTNode* call = NewCXXElementSpecialMemberCall(
          element, element_type, constructor_name, location);
      if (sequence == NULL) {
        sequence = call;
      } else {
        sequence = NewBinaryASTNode(
            AST_OP(comma), NewTypeRecordWithSize(kTypeVoid, kQualPlain),
            location, sequence, call);
      }
    }
    if (sequence == NULL) {
      return NULL;
    }
    Vector* statements = NewVector();
    VectorAppend(statements,
                 NewExpressionStatementASTNode(sequence, sym->location));
    return NewUnaryASTNode(
        AST_OP(stmt_expr), NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        sym->location,
        NewCompoundStatementASTNode(statements, sym->location));
  }
  const char* constructor_name = CXXConstructorNameForType(sym->type);
  if (constructor_name == NULL) {
    return NULL;
  }
  StructMember* ctor =
      FindStructMemberByName(sym->type->info.struct_info, constructor_name);
  if (ctor == NULL || !ctor->is_member_function ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  // Calling a trivial implicit default constructor has no observable effect,
  // so aggregate default-initialization can omit it.  A non-trivial implicit
  // constructor must still run: in particular, aggregates may have default
  // member initializers or members with non-trivial default constructors.
  if (TypeIsStructOrUnion(sym->type) && sym->type->info.struct_info != NULL &&
      sym->type->info.struct_info->is_aggregate &&
      ctor->symbol->flags.invented &&
      (ctor->symbol->type->info.function.is_deleted ||
       ctor->symbol->type->info.function.is_trivial_special_member)) {
    return NULL;
  }
  return SyntaxNewCXXConstructorCall(syntax, sym, NewVector(), sym->location);
}

// Default-initializes `sym` for constexpr contexts: uses an empty braced
// initializer for arrays of class type (value initialization), otherwise
// the same default-constructor call as
// SyntaxNewCXXDefaultConstructorCallIfNeeded.  When `local` is true, wraps
// the result in a variable-init expression.
ASTNode* SyntaxNewCXXConstexprDefaultInitializer(Syntax* syntax, Symbol* sym,
                                                 bool local) {
  ASTNode* initializer = SyntaxNewCXXDefaultConstructorCallIfNeeded(syntax, sym);
  if (initializer == NULL || !TypeIsFixedArray(sym->type)) {
    return initializer;
  }
  // Default-initializing an array of classes default-constructs each element,
  // which is what value-initializing from `{}` does; unlike the per-element
  // constructor sequence, that is an initializer constant evaluation accepts.
  ASTNode* braced =
      NewBracedInitializerASTNode(NewVector(), NULL, sym->location);
  return local ? NewVariableInitExpression(syntax, sym, braced) : braced;
}
