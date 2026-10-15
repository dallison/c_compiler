//
//  expr_parser_lambda.c
//  c_compiler
//
//  Lambda expressions: captures, closure types, and call operators.
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
#include "expr_parser_internal.h"
#include "expr_semantics.h"
#include "preprocessor.h"
#include "reflection.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "std_header_suggestion.h"
#include "typo_correction.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_internal.h"
#include "type_member.h"
#include "compiler.h"
#include "type_traits_semantics.h"

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


// State threaded through CollectDefaultLambdaCaptures while scanning the body
// for identifiers that the default capture mode must implicitly capture.
typedef struct {
  Vector* captures;
  TypeRecord* lambda_func;
  LambdaCaptureDefault capture_default;
  Vector* body_locals;  // Symbols declared in this lambda body (not capturable).
} LambdaCaptureScan;


// State threaded through RewriteLambdaCaptureUses: the captures to look up and
// the closure's `this` parameter through which their fields are reached.
typedef struct {
  Vector* captures;
  Symbol* this_symbol;
} LambdaRewrite;


static LambdaCapture* NewLambdaCapture(Symbol* captured, bool by_reference, bool is_pack_expansion, ASTNode* initializer, bool is_init_capture);
static LambdaCapture* FindLambdaCapture(Vector* captures, Symbol* symbol);
static LambdaCapture* FindLambdaCaptureName(Vector* captures, String* name);
static bool LambdaFunctionOwnsSymbol(TypeRecord* func, Symbol* symbol);
static bool SymbolIsCXXThis(Symbol* symbol);
static bool CanCaptureSymbol(Symbol* symbol, TypeRecord* lambda_func);
static void ParseLambdaCaptureList(Syntax* syntax, Vector* captures, LambdaCaptureDefault* capture_default, TokenClass followers);
static void QueueLambdaCallOperatorDefinition(Symbol* symbol);
static void AddLambdaFunctionScopeSymbols(Syntax* syntax, TypeRecord* func);
static bool LambdaFormalNameAvailable(Vector* formals, String* name);
static TemplateArgument* NewLambdaTemplateParameterTypeArgument(int index, TypeRecord* type);
static TemplateParameter* NewLambdaTemplateParameter(Syntax* syntax, int index, bool is_parameter_pack);
static void AddLambdaFunctionAssociatedConstraint(TypeRecord* func, ConstraintExpr* constraint);
static bool ParseLambdaAbbreviatedParameter(Syntax* syntax, TypeRecord* func, int arg_number, TokenClass followers);
static bool ParseLambdaParameterList(Syntax* syntax, TypeRecord* func, TokenClass followers);
static void ParseLambdaTrailingRequiresClause(Syntax* syntax, TypeRecord* func);
static bool SkipNoexceptSpecifier(Syntax* syntax, TokenClass followers);
static TypeRecord* ParseLambdaSpecifiersAndReturnType(Syntax* syntax, TypeRecord* func, bool* is_mutable, bool* is_static, bool* is_constexpr, bool* is_consteval, bool* is_noexcept, TypeRecord* default_type, Vector* attributes, TokenClass followers);
static Struct* EnclosingClassForLambda(Syntax* syntax);
static Symbol* NewLambdaClosureTag(Syntax* syntax, SourceLocation location, TypeRecord** closure_type);
static Symbol* NewLambdaCallOperator(Syntax* syntax, TypeRecord* closure_type, TypeRecord* return_type, bool is_mutable, Vector* explicit_template_params, SourceLocation location, bool* out_is_static);
static void CollectLambdaBodyLocalSymbols(ASTNode* node, void* data, int child_id, VisitorMode mode);
static bool LambdaBodyDeclaresSymbol(Vector* body_locals, Symbol* symbol);
static void CollectDefaultLambdaCaptures(ASTNode* node, void* data, int child_id, VisitorMode mode);
static TypeRecord* LambdaCaptureFieldType(LambdaCapture* capture);
static void AddLambdaCaptureFields(TypeRecord* closure_type, Vector* captures, SourceLocation location);
static ASTNode* NewLambdaCaptureInitializer(LambdaCapture* capture, SourceLocation location);
static ASTNode* NewLambdaCaptureAccess(Symbol* this_symbol, LambdaCapture* capture, SourceLocation location);
static void AddLambdaInitCaptureScopeSymbols(Syntax* syntax, Vector* captures);
static void ReplaceChildForLambdaCapture(ASTNode* parent, int child_id, ASTNode* replacement);
static void RewriteLambdaCaptureUses(ASTNode* node, void* data, int child_id, VisitorMode mode);
static ASTNode* ParseLambdaBody(Syntax* syntax, Symbol* call_operator, Vector* captures, TokenClass followers);
static void RefreshGenericLambdaTemplatePatternBodyAfterCaptureSetup( TypeRecord* func);
static ASTNode* NewLambdaClosureInitializer(TypeRecord* closure_type, Vector* captures, SourceLocation location);


// Allocates and initializes the parser metadata for one explicit or implicit
// lambda capture.
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
static bool SymbolIsCXXThis(Symbol* symbol) {
  return symbol != NULL && symbol->flags.is_argument &&
         symbol->name.value != NULL && strcmp(symbol->name.value, "this") == 0;
}


// Reports whether a symbol belongs to an enclosing automatic scope and can
// therefore be captured.
static bool CanCaptureSymbol(Symbol* symbol, TypeRecord* lambda_func) {
  if (symbol == NULL || LambdaFunctionOwnsSymbol(lambda_func, symbol) ||
      TypeIsFunction(symbol->type) ||
      StorageIs(symbol->storage, STO(static) | STO(extern) | STO(typedef))) {
    return false;
  }
  // `this` is an invented parameter, but `[&]` / `[=]` must still capture it
  // when a lambda names an enclosing-class member.
  if (symbol->flags.invented && !SymbolIsCXXThis(symbol)) {
    return false;
  }
  // Enumerators are constants, not objects.  Default capture must not treat
  // `E::enumerator` as a by-reference local (that emits an undefined symbol).
  // STO(implicit) is 0, so it cannot be tested with StorageIs().  A constexpr
  // local array or class object also has a value, but its uses read storage.
  if (symbol->flags.value_set && symbol->storage == STO(implicit) &&
      !(symbol->flags.is_constexpr && symbol->type != NULL &&
        (TypeIsArray(symbol->type) || TypeIsStructOrUnion(symbol->type)))) {
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
    // `[...xs = init]` / `[&...xs = init]` ([expr.prim.lambda.capture]).
    bool leading_pack = LexMatch(syntax->lex, TOK(ellipsis));
    if (LexLookingAt(syntax->lex, TOK(this))) {
      Symbol* symbol =
          lambda_enclosing_this != NULL ? lambda_enclosing_this
                                        : FindThisSymbol(syntax);
      if (by_reference) {
        SyntaxError(syntax, "'this' cannot be captured by reference");
      }
      if (symbol == NULL) {
        SyntaxError(syntax,
                    "'this' capture is only valid inside a member function");
      } else if (FindLambdaCapture(captures, symbol) == NULL) {
        VectorAppend(captures,
                     NewLambdaCapture(symbol, /*by_reference=*/false,
                                      /*is_pack_expansion=*/false,
                                      /*initializer=*/NULL,
                                      /*is_init_capture=*/false));
      }
      LexNextToken(syntax->lex);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      continue;
    }
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
      if (leading_pack && initializer != NULL) {
        if (!CXXExpressionContainsParameterPack(syntax, initializer)) {
          SyntaxError(syntax,
                      "pack expansion requires a function parameter pack");
        }
        initializer->flags |= kASTPackExpansion;
      } else {
        MarkCXXPackExpansionIfPresent(syntax, initializer);
      }
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
    if (leading_pack) {
      SyntaxError(syntax, "Expected = after lambda init-capture pack %s",
                  name.value);
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
  CompilerQueuePendingFunctionDefinition(symbol);
}


// Bring the lambda's parameters into the body's local scope before parsing it.
static void AddLambdaFunctionScopeSymbols(Syntax* syntax, TypeRecord* func) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = formals->value.p[i];
    InsertLocalSymbol(syntax->local_symbol_stack, formal);
  }
}


// Checks that a new lambda parameter name does not duplicate an earlier
// parameter.
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


// Builds the dependent type argument associated with an invented
// generic-lambda template parameter.
static TemplateArgument* NewLambdaTemplateParameterTypeArgument(int index,
                                                               TypeRecord* type) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterType;
  arg->type = type != NULL ? TypeRecordCopy(type) : NULL;
  arg->template_parameter_index = index;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}


// Creates the invented template type parameter used for an auto lambda
// parameter.
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


// Combines an abbreviated lambda parameter constraint with the call operator’s
// existing constraints.
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


// Parses an auto or constrained-auto lambda parameter and adds its invented
// template parameter and formal.
static bool ParseLambdaAbbreviatedParameter(Syntax* syntax, TypeRecord* func,
                                            int arg_number,
                                            TokenClass followers) {
  // Unconstrained `auto` parameters make a generic lambda (C++14).
  // `Concept auto` parameters are an abbreviated function template (C++20).
  if (!CompilerCXXAtLeast(kLanguageStandardCXX14)) {
    return false;
  }
  bool allow_constrained_auto = CompilerCXXAtLeast(kLanguageStandardCXX20);

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
  if (allow_constrained_auto && LexLookingAt(syntax->lex, TOK(identifier))) {
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


// Parses and attaches a trailing requires-clause to a lambda call operator.
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
                                                      Vector* attributes,
                                                      TokenClass followers) {
  *is_mutable = false;
  *is_static = false;
  *is_constexpr = false;
  *is_consteval = false;
  *is_noexcept = false;
  bool saw_constexpr = false;
  bool saw_consteval = false;
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
      if (saw_consteval) {
        SyntaxError(syntax,
                    "'constexpr' and 'consteval' cannot both be specified");
      } else if (saw_constexpr) {
        SyntaxError(syntax, "Duplicate 'constexpr' specifier");
      }
      saw_constexpr = true;
      *is_constexpr = true;
    } else if (LexMatch(syntax->lex, TOK(consteval))) {
      if (saw_consteval) {
        SyntaxError(syntax, "Duplicate 'consteval' specifier");
      } else if (saw_constexpr) {
        SyntaxError(syntax,
                    "'constexpr' and 'consteval' cannot both be specified");
      }
      saw_consteval = true;
      *is_consteval = true;
      *is_constexpr = true;
    } else if (LexLookingAt(syntax->lex, TOK(noexcept))) {
      *is_noexcept = SkipNoexceptSpecifier(syntax, followers);
    } else if (attributes != NULL &&
               SyntaxParseAnyAttribute(syntax, attributes)) {
      // `[&]() __attribute__((always_inline)) { ... }` (and `[[...]]`).
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
    // The trailing return type is in the scope of the lambda parameters
    // ([expr.prim.lambda.closure]).  `decltype(x << y)` must see `x` and `y`,
    // not an implicit member access through the enclosing class.
    SyntaxOpenScope(syntax);
    AddLambdaFunctionScopeSymbols(syntax, func);
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
    SyntaxCloseScope(syntax);
  }
  ParseLambdaTrailingRequiresClause(syntax, func);
  SyntaxParseFunctionContracts(syntax, func, NULL, false);
  // Attributes may also follow the trailing return type: `[]() -> T attr {}`.
  if (attributes != NULL) {
    while (SyntaxParseAnyAttribute(syntax, attributes)) {
    }
  }
  return return_type;
}


// Create the synthesized, uniquely-named closure class for a lambda and
// register it as a tag.  Capture fields are added later; for now it carries a
// single private placeholder member so it has non-zero size before captures are
// known.  Returns the tag symbol and outputs its struct type in `closure_type`.
static Struct* EnclosingClassForLambda(Syntax* syntax) {
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function) &&
      compiler->current_function->info.function.cxx_member_owner != NULL) {
    return compiler->current_function->info.function.cxx_member_owner;
  }
  if (syntax != NULL) {
    return syntax->cxx_class_head;
  }
  return NULL;
}


// Creates the anonymous closure class and returns its tag and class type.
static Symbol* NewLambdaClosureTag(Syntax* syntax, SourceLocation location,
                                   TypeRecord** closure_type) {
  String tag_name;
  StringInit(&tag_name, NULL);
  SyntaxFakeTagName(syntax, &tag_name);

  Struct* closure = NewStruct(false);
  closure->is_class = true;
  // A lambda in a member function is a nested class of the enclosing class
  // for access purposes ([expr.prim.lambda], [class.access.nest]): its body
  // may name private members and invoke private constructors.
  closure->lexical_parent = EnclosingClassForLambda(syntax);
  // A lambda is a local class of the function that contains it, so its body
  // has that function's access ([class.local]): a lambda in a friend function
  // may call the befriended class's private members.
  if (syntax != NULL && syntax->context == kParsingBlockScope &&
      compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    closure->access_enclosing_function =
        compiler->current_function->info.function.symbol;
  }
  closure->in_namespace_template =
      syntax != NULL && syntax->parsing_template_declaration &&
      closure->lexical_parent == NULL &&
      closure->access_enclosing_function == NULL;
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
  Vector attributes;
  VectorInit(&attributes);
  return_type = ParseLambdaSpecifiersAndReturnType(
      syntax, func, &is_mutable, &is_static, &is_constexpr, &is_consteval,
      &is_noexcept, return_type, &attributes, TC(closebra));
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

  // The closure is named by a per-translation-unit counter (`__invented__N`),
  // so the same name can denote different lambdas in different objects.  Keep
  // the call operator local to its object.
  Symbol* op = NewSymbol("operator()", func, STO(static));
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
  VectorAppendVector(&op->attributes, &attributes);
  VectorClear(&attributes);
  VectorDestruct(&attributes);
  SyntaxApplyDeclarationAttributes(syntax, op);

  StructMember* member = NewStructMember(op);
  member->is_member_function = true;
  member->is_static = is_static;
  member->access = kAccessPublic;
  StructAddSyntheticMember(closure, member);
  *out_is_static = is_static;
  return op;
}


// Collect symbols introduced in the lambda body so default-capture scanning
// does not treat them as enclosing-scope captures (e.g. `[&]{ auto inner =
// []{}; return inner(); }` must not capture `inner`, and a catch parameter
// must not be treated as a by-reference capture of a non-existent local).
static void CollectLambdaBodyLocalSymbols(ASTNode* node, void* data,
                                          int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  Vector* locals = (Vector*)data;
  if (node->op == AST_OP(vardecl)) {
    VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
    if (decl->symbol != NULL) {
      VectorAppend(locals, decl->symbol);
    }
    return;
  }
  if (node->op == AST_OP(catch)) {
    CatchASTNode* clause = (CatchASTNode*)node;
    if (clause->symbol != NULL) {
      VectorAppend(locals, clause->symbol);
    }
    return;
  }
  if (node->op == AST_OP(structured_binding)) {
    StructuredBindingASTNode* binding = (StructuredBindingASTNode*)node;
    if (binding->symbols != NULL) {
      for (size_t i = 0; i < binding->symbols->length; i++) {
        Symbol* symbol = binding->symbols->value.p[i];
        if (symbol != NULL) {
          VectorAppend(locals, symbol);
        }
      }
    }
    if (binding->condition_symbol != NULL) {
      VectorAppend(locals, binding->condition_symbol);
    }
  }
}


// Reports whether a symbol was declared inside the lambda body and is
// therefore not capturable.
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
  if (id->symbol != NULL &&
      (id->symbol->variable_template != NULL || id->symbol->flags.is_template ||
       id->symbol->flags.is_concept)) {
    return;
  }
  if (!CanCaptureSymbol(id->symbol, scan->lambda_func) ||
      LambdaBodyDeclaresSymbol(scan->body_locals, id->symbol) ||
      FindLambdaCapture(scan->captures, id->symbol) != NULL) {
    return;
  }
  bool by_reference =
      scan->capture_default == kLambdaCaptureDefaultReference;
  // `this` is always captured as a pointer copy, even under `[&]`.
  if (SymbolIsCXXThis(id->symbol)) {
    by_reference = false;
  }
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
      closure->lambda_has_init_capture = true;
      closure->lambda_body_needs_reanalysis = true;
      capture->initializer = AnalyzeExpression(capture->initializer);
      if (capture->initializer->type != NULL && capture->captured != NULL) {
        TypeRecord* deduced = capture->initializer->type;
        if (TypeIsReference(deduced)) {
          deduced = deduced->next;
        }
        deduced = TypeMaterializeClassTemplateSpecialization(&compiler->syntax,
                                                             deduced);
        TypeRecordDelete(capture->captured->type);
        // A dependent initializer (`[f = std::move(op)]` inside a function
        // template) is only a placeholder here, often plain `int`.  Record
        // `auto` so each instantiation rebuilds the closure and deduces the
        // field from the concrete initializer.
        // A pack init-capture (`[...xs = args]`) keeps its pack type instead:
        // instantiation expands the field into one element per pack argument
        // only when the field's type names the pack.
        bool keeps_pack_type = deduced != NULL && capture->is_pack_expansion &&
                               !TypeContainsAuto(deduced) &&
                               TypeContainsTemplateParameter(deduced);
        bool placeholder = !keeps_pack_type &&
                           (deduced == NULL || TypeIsUnknown(deduced) ||
                            TypeContainsAuto(deduced) ||
                            TypeContainsTemplateParameter(deduced));
        capture->captured->type =
            placeholder ? NewTypeRecord(kTypeAuto, kQualPlain)
                        : TypeRecordCopy(deduced);
        if (!placeholder) {
          capture->captured->type->qualifiers &= ~(kQualConst | kQualVolatile);
        }
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
  // Leave the arrow untyped so member lookup still resolves the field.
  // The dereference's type is the captured object: a clone can skip
  // reanalysis when the expression still looks dependent (`this` of a
  // closure nested in a class template), and code generation then loads a
  // node with no type (`(*hasher)(...)`).
  ASTNode* access =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, this_node, member);
  if (capture->by_reference) {
    TypeRecord* field_type =
        capture->field != NULL ? capture->field->type : NULL;
    TypeRecord* captured = field_type != NULL ? field_type->next : NULL;
    ASTNode* contents =
        NewUnaryASTNode(AST_OP(contents), captured, location, access);
    contents->value_category = kValueCategoryLvalue;
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
    // Stored child ids index `children` directly.  The visitor id reserves 0
    // for the callee, so argument N arrives here as N+1.
    replacement->child_id = child_id - 1;
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
  int errors_before = NumErrors();
  ASTNode* body = SyntaxParseStatement(syntax, followers);
  SyntaxCloseScope(syntax);
  syntax->current_template_parameter_count = old_template_parameter_count;
  syntax->context = old_context;
  call_operator->type->info.function.body = body;
  call_operator->type->info.function.body_had_parse_errors =
      NumErrors() != errors_before;
  VectorAppend(&compiler->declaration_asts, body);
  return body;
}


// Immutable operator() body copy used by CloneTemplateFunctionBody.  Taken after
// capture setup and capture-use rewriting so the pattern matches what the call
// operator will instantiate, while staying free of call-site analysis.
static void RefreshGenericLambdaTemplatePatternBodyAfterCaptureSetup(
    TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) || func->info.function.body == NULL ||
      func->info.function.template_parameters.length == 0) {
    return;
  }
  if (func->info.function.template_pattern_body != NULL &&
      func->info.function.template_pattern_body != func->info.function.body) {
    ASTNodeDelete(func->info.function.template_pattern_body);
  }
  func->info.function.template_pattern_body =
      ASTNodeClone(func->info.function.body, IdentityCloneNode, NULL, NULL);
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
ASTNode* ParseCXXLambdaExpression(Syntax* syntax,
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
  Symbol* enclosing_this =
      lambda_enclosing_this != NULL ? lambda_enclosing_this
                                    : FindThisSymbol(syntax);
  Symbol* saved_lambda_enclosing_this = lambda_enclosing_this;
  compiler->current_function = call_operator->type;
  lambda_enclosing_this = enclosing_this;
  syntax->parsing_lambda_body_depth++;
  ParseLambdaBody(syntax, call_operator, &captures, followers);
  syntax->parsing_lambda_body_depth--;
  lambda_enclosing_this = saved_lambda_enclosing_this;
  compiler->current_function = enclosing_function;
  // Snapshot before `[=]`/`[&]` capture scanning or other post-parse work can
  // semantically analyze the shared operator() body (e.g. lowering
  // `std::is_same_v` for `if constexpr`).
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
  if (call_operator->type->info.function.template_parameters.length > 0) {
    RefreshGenericLambdaTemplatePatternBodyAfterCaptureSetup(
        call_operator->type);
  }

  // Queue operator() for analysis/codegen unless the closure captures a
  // non-pack value whose type still names an enclosing template parameter, or
  // the body itself names one (`static_cast<Encoder*>(...)` inside a
  // captureless lambda).  The pattern body would otherwise be emitted with
  // `auto` still undeduced.  SubstituteNestedStructTemplateParameters rebuilds
  // the closure per instantiation and CloneInstantiatedMemberFunctionBody
  // re-queues the substituted body.
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
      if (TypeContainsTemplateParameter(capture->captured->type) ||
          TypeContainsAuto(capture->captured->type)) {
        defer_dependent_capture = true;
        break;
      }
    }
  }
  // A generic lambda with a default capture (`[=](auto n)` / `[&](auto n)`)
  // shares its operator() body with every deduced specialization.  Eager
  // analysis of that pattern can lower `std::is_same_v` to a non-constant
  // lvalue load before the call site clones the body, breaking
  // `if constexpr (std::is_same_v<...>)`.  A non-generic `[=]` / `[&]` has no
  // pattern to protect, and leaving it unqueued drops the call operator
  // (`[=]{ return base + x; }()`).
  bool generic_default_capture =
      capture_default != kLambdaCaptureDefaultNone &&
      call_operator->type->info.function.template_parameters.length > 0;
  if (!defer_dependent_capture &&
      !LambdaCallOperatorBodyDependsOnEnclosingTemplate(call_operator) &&
      !generic_default_capture) {
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
