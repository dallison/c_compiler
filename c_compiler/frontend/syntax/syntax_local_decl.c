//
//  syntax_local_decl.c
//  c_compiler
//
//  Block-scope declarations: local variables, structured bindings, copy
//  initialization and anonymous aggregates.
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

// Returns an `init` AST node binding `sym` to `initializer` for a local
// declaration.  Does not consume tokens.
ASTNode* NewVariableInitExpression(Syntax* syntax, Symbol* sym,
                                   ASTNode* initializer) {
  ASTNode* decl_id =
      NewIdentifierASTNode(sym, syntax->lex->current_token_location);
  decl_id->flags |= kASTNeedAddress;
  ASTNode* init = NewBinaryASTNode(AST_OP(init), sym->type,
                                  syntax->lex->current_token_location, decl_id,
                                  initializer);
  decl_id->flags |= kASTIsDeclaration;
  return init;
}

// Consumes `[ name, ... ]`, adds each binding symbol to `symbols` and its name
// to the returned vector, and sets `*pack_index` to the pack position or -1.
// Returns NULL when `[` is not present.
static Vector* ParseStructuredBindingNames(Syntax* syntax, Vector* symbols,
                                           int* pack_index) {
  if (!LexMatch(syntax->lex, TOK(lsquare))) {
    return NULL;
  }
  Vector* names = NewVector();
  *pack_index = -1;
  while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rsquare))) {
    bool is_pack = LexMatch(syntax->lex, TOK(ellipsis));
    if (is_pack) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
        SyntaxError(syntax,
                    "Structured binding packs require C++26");
      }
      if (*pack_index >= 0) {
        SyntaxError(syntax,
                    "Structured binding declaration cannot contain multiple packs");
      } else {
        *pack_index = (int)names->length;
      }
    }
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected structured binding name");
      break;
    }
    String* name = NewString(syntax->lex->spelling.value);
    Symbol* sym = NewSymbol(name->value, NewTypeRecord(kTypeAuto, kQualPlain),
                            STO(implicit));
    sym->flags.is_local = true;
    sym->flags.is_defined = true;
    sym->flags.is_parameter_pack = is_pack;
    if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
        StringEqual(name, "_")) {
      // A structured binding outside namespace scope is name-independent
      // regardless of the backing object's storage duration.
      sym->flags.is_name_independent = true;
    }
    sym->structured_binding_pack_size = -1;
    sym->location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    if (SyntaxLookingAtCXXAttribute(syntax)) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
        SyntaxError(syntax,
                    "Attributes on structured bindings require C++26");
      }
      SyntaxParseCXXAttributes(syntax, &sym->attributes);
      SyntaxApplyDeclarationAttributes(syntax, sym);
    }
    if (!SyntaxAddSymbol(syntax, sym)) {
      SyntaxError(syntax, "Duplicate structured binding name: %s",
                  name->value);
      StringDelete(name);
      SymbolDelete(sym);
    } else {
      VectorAppend(names, name);
      VectorAppend(symbols, sym);
    }
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
  return names;
}

// Returns `base_type` with any trailing `&` / `&&` reference specifiers applied
// for a structured binding declarator.
static TypeRecord* ParseStructuredBindingDeclaredType(Syntax* syntax,
                                                      TypeRecord* base_type) {
  TypeRecord* declared = TypeRecordCopy(base_type);
  if (CompilerIsCXX() &&
      (LexLookingAt(syntax->lex, TOK(amp)) ||
       LexLookingAt(syntax->lex, TOK(ampamp)))) {
    bool rvalue = LexMatch(syntax->lex, TOK(ampamp));
    if (!rvalue) {
      LexMatch(syntax->lex, TOK(amp));
    }
    TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
    TypeRecordChain(ref, declared);
    TypeRecordCalculateSize(ref);
    declared = ref;
  }
  return declared;
}

// When the next tokens start a structured binding, parses it into
// `declarations` and returns true.  Restores the lexer when `[` is absent.
static bool TryParseStructuredBindingDeclaration(Syntax* syntax,
                                                 TypeRecord* base_type,
                                                 Storage storage,
                                                 bool is_constexpr,
                                                 bool is_constinit,
                                                 Vector* declarations) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX17)) {
    return false;
  }
  // A NULL base type means the decl-specifier was invalid (e.g. an unknown type
  // name, with the error already reported).  A structured binding always has a
  // valid decl-specifier (`auto`/`const auto&`/...), so this can never be one;
  // bail out and let the normal declarator path recover -- copying a NULL type
  // below would dereference it.
  if (base_type == NULL) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  TypeRecord* declared_type =
      ParseStructuredBindingDeclaredType(syntax, base_type);
  if (!LexLookingAt(syntax->lex, TOK(lsquare))) {
    TypeRecordDelete(declared_type);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return false;
  }
  Vector* symbols = NewVector();
  int pack_index = -1;
  Vector* names =
      ParseStructuredBindingNames(syntax, symbols, &pack_index);
  LexCheckpointDestruct(&checkpoint);
  if ((is_constexpr || is_constinit) &&
      !CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    SyntaxError(syntax,
                "constexpr and constinit structured bindings require C++26");
  }
  for (size_t i = 0; i < symbols->length; i++) {
    Symbol* symbol = symbols->value.p[i];
    if (symbol != NULL) {
      symbol->flags.is_constexpr = is_constexpr;
      symbol->flags.is_constinit = is_constinit;
    }
  }
  if (names == NULL || names->length == 0) {
    SyntaxError(syntax, "Structured binding declaration requires at least one name");
  }
  if (pack_index >= 0 && syntax->current_template_parameter_count == 0) {
    SyntaxError(
        syntax,
        "Structured binding pack can only appear in a templated context");
  }
  if (StorageIs(storage, STO(extern))) {
    SyntaxError(syntax, "Structured binding declaration cannot be extern");
  }
  ASTNode* initializer = NULL;
  if (LexMatch(syntax->lex, TOK(equal))) {
    if (LexMatch(syntax->lex, TOK(lbrace))) {
      initializer = ParseBracedInitializer(syntax);
    } else {
      initializer = NewExpressionInitializerASTNode(
          SyntaxParseSingleExpression(syntax, TC(semicolon) | TC(stmt)),
          syntax->lex->current_token_location);
    }
  } else if (LexMatch(syntax->lex, TOK(lbrace))) {
    initializer = MarkDirectListInitializer(ParseBracedInitializer(syntax));
  } else {
    SyntaxError(syntax, "Structured binding declaration requires an initializer");
  }
  VectorAppend(declarations,
               NewStructuredBindingASTNode(declared_type, storage, names, symbols,
                                           pack_index,
                                           initializer,
                                           syntax->lex->current_token_location));
  return true;
}

// True if `type` has a nontrivial copy or move constructor.  This includes
// user-declared constructors and implicitly generated constructors that must
// copy or move a nontrivial subobject.
static bool CXXTypeHasNontrivialCopyOrMoveConstructor(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  for (StructMember* c = FindCXXConstructor(type); c != NULL;
       c = c->overload_next) {
    if (c->is_member_function && c->symbol != NULL && c->symbol->type != NULL &&
        TypeIsFunction(c->symbol->type) &&
        (c->symbol->type->info.function.cxx_special_member_kind ==
             kCXXSpecialMemberCopyConstructor ||
         c->symbol->type->info.function.cxx_special_member_kind ==
             kCXXSpecialMemberMoveConstructor) &&
        (c->symbol->type->info.function.is_user_declared ||
         !c->symbol->type->info.function.is_trivial_special_member)) {
      return true;
    }
  }
  return false;
}

// True if copy-/move-initializing an object of `type` must run a copy or move
// constructor rather than degrading to a byte-wise copy: the class itself, a
// base, or a (possibly array) non-static data member -- recursively -- has a
// nontrivial copy or move constructor.  This is the recursive generalization
// of CXXTypeHasNontrivialCopyOrMoveConstructor: a class whose own copy/move
// constructor is implicit is still non-trivially copyable if any subobject is,
// and a byte copy would then alias whatever resource that subobject's
// user-defined copy constructor is responsible for duplicating (e.g. the bucket
// array owned by std::unordered_map's __hash_table member), causing shared state
// and later double-frees.  A class with no such subobject is left to the
// member-wise `init` path.
static bool CXXTypeRequiresCopyConstructorCallImpl(TypeRecord* type,
                                                   Vector* visited) {
  if (type == NULL) {
    return false;
  }
  if (TypeIsFixedArray(type)) {
    return CXXTypeRequiresCopyConstructorCallImpl(type->next, visited);
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  if (CXXTypeHasNontrivialCopyOrMoveConstructor(type)) {
    return true;
  }
  Struct* owner = type->info.struct_info;
  // Classes can contain themselves through compiler-internal or incomplete
  // representations.  Do not recurse forever when following such a cycle.
  if (VectorContainsPointer(visited, owner)) {
    return false;
  }
  VectorAppend(visited, owner);
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base != NULL &&
        CXXTypeRequiresCopyConstructorCallImpl(base->type, visited)) {
      return true;
    }
  }
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i];
    if (base != NULL &&
        CXXTypeRequiresCopyConstructorCallImpl(base->type, visited)) {
      return true;
    }
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    if (CXXTypeRequiresCopyConstructorCallImpl(member->symbol->type, visited)) {
      return true;
    }
  }
  return false;
}

// True when copy- or move-initializing `type` must invoke a copy or move
// constructor rather than a member-wise `init`, including nontrivial subobjects.
static bool CXXTypeRequiresCopyConstructorCall(TypeRecord* type) {
  Vector visited;
  VectorInit(&visited);
  bool requires_call =
      CXXTypeRequiresCopyConstructorCallImpl(type, &visited);
  VectorDestruct(&visited);
  return requires_call;
}

// Copy-initialization of a class object from a single expression, `T b = expr;`.
// Like direct-initialization `T b(expr);`, this must select and invoke a
// constructor (copy, move, or converting) rather than degrading to a shallow
// member-wise `init`.  A member-wise init merely copies the bytes of the source
// object, which for a class managing a resource (e.g. a string's heap pointer)
// aliases that resource instead of running the user-defined copy constructor and
// leads to double-frees / dangling pointers.
//
// Same-class initialization always uses constructor selection.  Initialization
// from a different class is rewritten only when the target has nontrivial copy
// semantics; otherwise the member-wise path is retained because it also handles
// conversion operators (for example the comparison-category types).
//
// `initializer` is the node produced by SyntaxParseInitializer after the braced
// list-constructor rewrite; only a plain expression initializer (`expr_init`)
// for a non-aggregate class type with at least one constructor is rewritten.
static ASTNode* NewCXXCopyInitConstructorInitializer(Syntax* syntax, Symbol* sym,
                                                     ASTNode* initializer) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      initializer == NULL || initializer->op != AST_OP(expr_init) ||
      !TypeIsStructOrUnion(sym->type) || sym->type->info.struct_info == NULL) {
    return initializer;
  }
  if (FindCXXConstructor(sym->type) == NULL) {
    return initializer;
  }
  ExpressionInitializerASTNode* expr_init =
      (ExpressionInitializerASTNode*)initializer;
  ASTNode* expr = expr_init->expr;
  if (expr == NULL) {
    return initializer;
  }
  bool same_class =
      TypeIsStructOrUnion(expr->type) &&
      expr->type->info.struct_info == sym->type->info.struct_info;
  if (!same_class && !CXXTypeRequiresCopyConstructorCall(sym->type)) {
    return initializer;
  }
  // [over.match.copy]: from another class, the candidates are the converting
  // constructors, whose argument may not itself use a user-defined conversion
  // ([over.best.ics]/4), and the source's conversion functions to `T`.  A
  // constructor call would wrongly allow `M(initializer_list<int>)` through a
  // conversion function template, so let the conversion function initialize
  // the object directly when no constructor takes the source as it is.
  if (!same_class && expr->type != NULL && TypeIsStructOrUnion(expr->type) &&
      CXXFindConvertingConstructorCandidate(sym->type, expr,
                                            /*allow_explicit=*/false) == NULL &&
      CXXClassHasConversionOperatorTo(expr, sym->type)) {
    return initializer;
  }
  bool known_prvalue =
      (expr->flags & kASTAnalyzed) != 0 || expr->op == AST_OP(call) ||
      expr->op == AST_OP(compound_literal);
  if (known_prvalue && expr->value_category == kValueCategoryPrvalue &&
      same_class) {
    return initializer;
  }
  // Hand the operand to the constructor call and detach it from the wrapper so
  // the discarded `expr_init` node does not co-own it.
  expr_init->expr = NULL;
  Vector* actuals = NewVector();
  VectorAppend(actuals, expr);
  return SyntaxNewCXXConstructorCall(syntax, sym, actuals,
                                     initializer->location);
}

// Rewrites copy-initialization of a class local into a constructor call when
// required; otherwise returns `initializer` unchanged.  Deletes the old node
// when a rewrite occurs.
ASTNode* SyntaxRewriteCXXCopyInitConstructorIfNeeded(Syntax* syntax,
                                                     Symbol* sym,
                                                     ASTNode* initializer) {
  if (initializer == NULL) {
    return initializer;
  }
  if (initializer->op == AST_OP(expr_init)) {
    ASTNode* rewritten =
        NewCXXCopyInitConstructorInitializer(syntax, sym, initializer);
    if (rewritten != initializer) {
      ASTNodeDelete(initializer);
    }
    return rewritten;
  }
  if (initializer->op != AST_OP(init)) {
    return initializer;
  }
  BinaryASTNode* init = (BinaryASTNode*)initializer;
  ASTNode* expression_initializer = init->right;
  ASTNode* rewritten = NewCXXCopyInitConstructorInitializer(
      syntax, sym, expression_initializer);
  if (rewritten == expression_initializer) {
    return initializer;
  }

  init->right = NULL;
  ASTNodeDelete(initializer);
  ASTNodeDelete(expression_initializer);
  return rewritten;
}

// Adds block-scope symbols for each named member of anonymous aggregate `aggregate`
// at `base_offset` from host object `object`, recursing into nested anon unions.
static void InjectBlockScopeAnonymousAggregateMembers(Syntax* syntax,
                                                      Symbol* object,
                                                      Struct* aggregate,
                                                      int base_offset) {
  if (aggregate == NULL) {
    return;
  }
  for (size_t i = 0; i < aggregate->members.length; i++) {
    StructMember* member = aggregate->members.value.p[i];
    if (member == NULL) {
      continue;
    }
    if (member->is_anon) {
      Struct* nested = NULL;
      if (member->symbol != NULL && member->symbol->type != NULL &&
          TypeIsStructOrUnion(member->symbol->type)) {
        nested = member->symbol->type->info.struct_info;
      }
      InjectBlockScopeAnonymousAggregateMembers(
          syntax, object, nested, base_offset + member->byte_offset);
      continue;
    }
    if (member->symbol == NULL || member->symbol->name.length == 0 ||
        member->is_member_function || member->is_static ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    Symbol* injected = SymbolClone(member->symbol);
    injected->flags.is_local = true;
    injected->flags.is_defined = true;
    injected->flags.invented = false;
    injected->storage = STO(auto);
    injected->location = object->location;
    SymbolMutableExtra(injected)->anonymous_union_host = object;
    SymbolMutableExtra(injected)->anonymous_union_member_offset =
        base_offset + member->byte_offset;
    if (!SyntaxAddSymbol(syntax, injected)) {
      SyntaxError(syntax, "Duplicate symbol %s", injected->name.value);
      SymbolDelete(injected);
    }
  }
}

// True when `type` names an unnamed class or union in block scope (after
// stripping pointers and references).
static bool LocalDeclarationTypeIsAnonymousAggregate(TypeParser* parser,
                                                     TypeRecord* type) {
  if (!CompilerIsCXX() || parser->syntax->context != kParsingBlockScope ||
      type == NULL) {
    return false;
  }
  TypeRecord* bare = type;
  while (bare != NULL && (TypeIsPointer(bare) || TypeIsReference(bare))) {
    bare = bare->next;
  }
  if (bare == NULL || !TypeIsStructOrUnion(bare) ||
      bare->info.struct_info == NULL) {
    return false;
  }
  Struct* aggregate = bare->info.struct_info;
  Symbol* tag = aggregate->tag_symbol;
  return tag == NULL || tag->flags.invented || aggregate->tag_name == NULL ||
         aggregate->tag_name->length == 0;
}

// For an invented unnamed aggregate object `sym`, injects its members into the
// current block scope so they can be named unqualified.
static void MaybeInjectBlockScopeAnonymousAggregateMembers(Syntax* syntax,
                                                           Symbol* sym) {
  if (!CompilerIsCXX() || syntax->context != kParsingBlockScope ||
      sym == NULL || !sym->flags.invented || TypeIsFunction(sym->type)) {
    return;
  }
  // Only an unnamed aggregate object promotes its members into the block
  // (`struct { int x; };`).  A pointer or reference is not that object:
  // a local class member function's `this` has the class type behind a
  // pointer, and injecting those members collides with the enclosing
  // function's parameters of the same name.
  TypeRecord* type = sym->type;
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return;
  }
  Struct* aggregate = type->info.struct_info;
  Symbol* tag = aggregate->tag_symbol;
  if (tag != NULL && !tag->flags.invented && aggregate->tag_name != NULL &&
      aggregate->tag_name->length != 0) {
    return;
  }
  InjectBlockScopeAnonymousAggregateMembers(syntax, sym, aggregate, 0);
}

// Emits -Wshadow when a newly declared block-scope variable `sym` hides a
// variable or parameter from an enclosing scope (or a file-scope object),
// mirroring clang/gcc -Wshadow.  Called for genuinely new local declarations,
// so the current innermost scope is skipped and only enclosing scopes are
// searched.
static void CheckLocalVariableShadow(Syntax* syntax, Symbol* sym) {
  if (!WarningIsEnabled("shadow")) {
    return;
  }
  if (sym == NULL || sym->name.length == 0) {
    return;
  }
  // Only ordinary objects shadow.  Functions, typedefs, and compiler-generated
  // symbols are not variables and are left alone.
  if (TypeIsFunction(sym->type) || StorageIs(sym->storage, STO(typedef)) ||
      sym->flags.is_temp || sym->flags.invented) {
    return;
  }
  if (syntax->local_symbol_stack == NULL) {
    return;
  }

  Symbol* shadowed = NULL;
  const char* what = NULL;
  // Enclosing local scopes first (the innermost scope is skipped via ->prev).
  Symbol* enclosing =
      FindLocalSymbol(syntax->local_symbol_stack->prev, &sym->name);
  if (enclosing != NULL) {
    if (TypeIsFunction(enclosing->type) ||
        StorageIs(enclosing->storage, STO(typedef))) {
      return;
    }
    shadowed = enclosing;
    what = enclosing->flags.is_argument ? "a parameter" : "a previous local";
  } else {
    Symbol* global = FindFileScopeSymbol(syntax, &sym->name);
    if (global == NULL || global == sym || TypeIsFunction(global->type) ||
        StorageIs(global->storage, STO(typedef))) {
      return;
    }
    shadowed = global;
    what = "a global declaration";
  }

  ReportWarningAtLocation(sym->location, "shadow",
                          "declaration of '%s' shadows %s",
                          sym->name.value, what);
  ReportNoteAtLocation(shadowed->location,
                       "shadowed declaration is here");
}

/* A class object declared constexpr may be default-initialized; that
 * constructor call is the initializer.  Inside a template the concrete
 * constructor is filled in when the declaration is cloned.  A dependent type
 * is deferred too: it may be a class, and a scalar is diagnosed at
 * instantiation once the type is known. */
bool CXXDeferConstexprDefaultInitialization(Syntax* syntax,
                                            TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  if (TypeIsUnknown(type) || TypeContainsTemplateParameter(type)) {
    return true;
  }
  bool in_template = syntax->parsing_template_declaration ||
                     syntax->current_template_parameter_count > 0;
  return in_template && (TypeIsStructOrUnion(type) || TypeIsArray(type));
}

// AST walk predicate: true when an identifier in the walk names a symbol other
// than `data` whose type still contains `auto` (excluding function templates).
static bool InitializerNamesUndeducedAuto(ASTNode* node, void* data) {
  Symbol* self = data;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* named = ((IdentifierASTNode*)node)->symbol;
  if (named == NULL || named == self || named->type == NULL) {
    return false;
  }
  // Analyzing a call instantiates a function template and deduces its
  // placeholder return type (`auto at = transposed(a);`).
  if (named->flags.is_template && TypeIsFunction(named->type)) {
    return false;
  }
  return TypeContainsAuto(named->type);
}

// Applies the declaration specifiers to a block-scope declarator's symbol:
// constexpr, constinit and consteval, the local flag of automatic objects, and
// the namespace-qualified mangled name of a block-scope function declaration.
static void ApplyLocalDeclaratorSpecifiers(TypeParser* parser, Symbol* sym,
                                           Storage storage) {
  Syntax* syntax = parser->syntax;
  if (TypeIsFunction(sym->type)) {
    if (parser->is_constinit) {
      SyntaxError(syntax, "'constinit' cannot be applied to a function");
    }
  } else {
    sym->flags.is_constexpr = parser->is_constexpr;
    sym->flags.is_constinit = parser->is_constinit;
    DiagnoseConstevalOnNonFunction(syntax, sym->type, parser->is_consteval);
    if (parser->is_constinit &&
        !StorageIs(storage, STO(static) | STO(extern) | STO(thread))) {
      SyntaxError(
          syntax,
          "'constinit' variable must have static or thread storage duration");
    }
    if (sym->flags.is_constexpr && !TypeIsReference(sym->type)) {
      SymbolSetType(sym, TypeRecordAddQualifiers(sym->type, kQualConst));
    }
  }
  if (!StorageIs(sym->storage, STO(extern)) &&
      !TypeIsFunction(sym->type)) {
    // Local declarators need this before duplicate-name handling so C++26
    // automatic `_` declarations can be recognized as name-independent.
    // A block-scope function declaration has extern linkage (C 6.2.2/5)
    // and names a function, not an automatic object (00078.c).
    sym->flags.is_local = true;
  }
  // It names the function of the enclosing namespace, mangled as such.
  if (CompilerIsCXX() && TypeIsFunction(sym->type) &&
      sym->asm_name.length == 0) {
    if (sym->namespace_ == NULL &&
        syntax->current_namespace != compiler->global_namespace) {
      sym->namespace_ = syntax->current_namespace;
    }
    SymbolSetCXXMangledAsmName(sym);
  }
}

// Checks a block-scope redeclaration of `old_sym` by `sym`: redefinition,
// type and linkage must agree.  Marks the previous symbol defined or
// tentative and merges default arguments and contracts.  Returns true when
// the declarator can be merged into `old_sym`.
static bool CheckLocalRedeclaration(TypeParser* parser, Symbol* sym,
                                    Symbol* old_sym, Storage storage) {
  Syntax* syntax = parser->syntax;
  bool ok = true;
  // We have this symbol already.  If it's a declaration then it's
  // OK to declare (and define) it now.  If it's a definition then
  // this must be a declaration.
  if (old_sym->flags.is_defined) {
    if (StorageIs(storage, STO(extern))) {
      // This might be a declaration, only if there is no initializer
      if (LexLookingAt(parser->lex, TOK(equal))) {
        SyntaxError(syntax,
                    "Extern variable can't have an initializer: %s",
                    sym->name.value);
        ok = false;
      }
    } else {
      if (IsDefinition(parser, old_sym, storage)) {
        // This is a declaration of a previously known definition.
        String symbol_name;
        StringInit(&symbol_name, NULL);
        SymbolFunctionDiagnosticName(sym, &symbol_name);
        SyntaxError(syntax, "Duplicate definition of local symbol %s",
                    symbol_name.value);
        StringDestruct(&symbol_name);
        ok = false;
      }
    }
  } else {
    // Old sym is a declaration.
    if (IsDefinition(parser, sym, storage)) {
      // This is a definition so the original symbol is now defined.
      old_sym->flags.is_defined = true;
    } else if (!StorageIs(storage, STO(extern))) {
      old_sym->flags.is_tentative_decl = true;
    }
  }

  if (!RedeclarationTypesEqual(sym->type, old_sym->type)) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(sym, &suffix);
    SyntaxError(syntax, "Symbol %s redeclared with different type%s",
                sym->name.value, suffix.value);
    StringDestruct(&suffix);
    TypeErrorDetails(syntax->lex->current_token_location,
                     sym->type, old_sym->type);
    ReportNoteAtLocation(old_sym->location,
                         "Previously declared here");
    // Keep the declarator we just parsed.  The previous symbol has an
    // incompatible type, and for a name introduced by a using-declaration
    // or using-directive it has no type at all, which the rest of the
    // declaration processing cannot work with.
    ok = false;
  } else {
    MergeCXXDefaultArguments(syntax, old_sym, sym);
    MergeCXXContractAssertions(syntax, old_sym, sym);
    CheckMatchingConstexprConsteval(syntax, old_sym->type, sym->type);
    // Symbol declaration is the same type as the definition, make sure
    // the linkage matches.
    Storage old_storage = old_sym->storage & ~STO(extern);
    Storage new_storage = sym->storage & ~STO(extern);

    if (old_storage != new_storage) {
      SyntaxError(syntax, "Symbol %s redeclared with different linkage",
                  sym->name.value);
      ok = false;
    }
  }
  return ok;
}

// Enters the first block-scope declaration of a name.  An `extern`
// declaration refers to the file-scope symbol of that name when there is
// one.  A placeholder-typed (`auto`) variable is not visible in its own
// initializer, so its insertion is left to the caller (`*defer_auto_insert`).
// Returns the symbol the declarator now refers to.
static Symbol* DeclareNewLocalSymbol(Syntax* syntax, Symbol* sym,
                                     Storage storage,
                                     bool* defer_auto_insert) {
  if (StorageIs(storage, STO(extern))) {
    Symbol* link = FindFileScopeSymbol(syntax, &sym->name);
    if (link != NULL) {
      SymbolDelete(sym);
      sym = link;
    } else {
      bool added = SyntaxAddSymbol(syntax, sym);
      if (!added) {
        SyntaxError(syntax, "Duplicate symbol %s",
                    sym->name.value);
      }
    }
  } else {
    // A placeholder-typed variable is not visible in its own initializer
    // (`auto co = std::make_unique<co::Coroutine>(...)` still names the
    // namespace).  Insert it after the initializer is parsed.
    *defer_auto_insert = CompilerIsCXX() && !TypeIsFunction(sym->type) &&
                         TypeContainsAuto(sym->type);
    if (!*defer_auto_insert) {
      CheckLocalVariableShadow(syntax, sym);
      bool added = SyntaxAddSymbol(syntax, sym);
      if (!added) {
        SyntaxError(syntax, "Duplicate symbol %s",
                    sym->name.value);
      } else {
        MaybeInjectBlockScopeAnonymousAggregateMembers(syntax, sym);
      }
    }
  }
  return sym;
}

// Matches a block-scope declarator against an earlier declaration of its
// name in the same scope, merging into it when allowed, or enters it as a new
// local symbol.  Returns the symbol the declarator refers to from now on.
static Symbol* ResolveLocalDeclarator(TypeParser* parser, Symbol* sym,
                                      Storage storage,
                                      bool* defer_auto_insert) {
  Syntax* syntax = parser->syntax;
  MarkCXX26AutomaticNameIndependent(sym);
  Symbol* old_sym =
      FindTopLocalSymbol(syntax->local_symbol_stack, &sym->name);
  if (CompilerIsCXX() && old_sym != NULL &&
      !StorageIs(sym->storage, STO(typedef)) &&
      SymbolIsHideableClassOrEnumName(old_sym)) {
    HideClassOrEnumName(syntax, old_sym);
    old_sym = NULL;
  }
  if (IsC23InferredAutoType(sym->type) && old_sym != NULL) {
    SyntaxError(syntax,
                "C23 inferred auto declaration cannot redeclare '%s'",
                sym->name.value);
  }
  if (old_sym == NULL || sym->flags.is_name_independent) {
    return DeclareNewLocalSymbol(syntax, sym, storage, defer_auto_insert);
  }
  if (CheckLocalRedeclaration(parser, sym, old_sym, storage)) {
    // We now refer to the previously defined symbol rather than this new
    // one.
    SymbolDelete(sym);
    sym = old_sym;
    if (!TypeIsFunction(sym->type) && parser->is_constexpr) {
      sym->flags.is_constexpr = true;
      if (!TypeIsReference(sym->type)) {
        SymbolSetType(sym, TypeRecordAddQualifiers(sym->type, kQualConst));
      }
    }
    if (!TypeIsFunction(sym->type) && parser->is_constinit) {
      sym->flags.is_constinit = true;
    }
  }
  return sym;
}

// A block-scope function declaration.  A body is not allowed (C has no
// nested functions); one is skipped to recover.
static void DeclareLocalFunction(TypeParser* parser, Symbol* sym) {
  Syntax* syntax = parser->syntax;
  if (!CompilerIsCXX() && parser->is_constexpr) {
    SyntaxError(syntax, "C constexpr functions are not supported");
  }
  if (TypeContainsAuto(sym->type)) {
    SyntaxError(syntax, "auto function return type is not supported yet");
  }
  if (LexMatch(syntax->lex, TOK(lbrace))) {
    // C does not supported nested functions.
    SyntaxError(syntax, "Function definition not allowed here");
    // Skip function body so that we can attempt to recover.
    SkipFunctionBody(syntax);
    // Continue on to check for comma, which probably won't exist
    // then we will exit the loop.
  }
}

// Parses a block-scope object's initializer and turns it into the
// initialization to perform: copy-initialization through a constructor, a
// guarded thread_local initialization, or, without an initializer, the
// default construction (or constexpr default initializer) the type needs.
// Returns the initializer, or NULL for none.
static ASTNode* ParseLocalObjectInitializer(TypeParser* parser, Symbol* sym,
                                            Storage storage) {
  Syntax* syntax = parser->syntax;
  ASTNode* initializer = NULL;
  if (LexMatch(syntax->lex, TOK(equal))) {
    syntax->init_storage = storage;
    initializer = SyntaxParseInitializer(syntax, sym, storage);
    ResolveCXXClassTemplateArgumentDeductionFromInitializer(
        syntax, sym, initializer);
    initializer =
        NewCXXCopyListConstructorInitializer(syntax, sym, initializer);
    initializer =
        NewCXXCopyInitConstructorInitializer(syntax, sym, initializer);
    if (initializer == NULL || initializer->op != AST_OP(call)) {
      initializer = NewVariableInitExpression(syntax, sym, initializer);
    }
    if (!StorageIs(sym->storage, STO(extern)) &&
        StorageIs(sym->storage, STO(thread)) &&
        CXXThreadLocalInitializerIsDynamic(initializer)) {
      initializer = NewCXXThreadLocalGuardedInit(syntax, sym, initializer);
    }
    return initializer;
  }
  initializer = ParseCXXDirectInitializer(syntax, sym, true);
  if (initializer == NULL && CompilerIsCXX() &&
      LexMatch(syntax->lex, TOK(lbrace))) {
    initializer = NewVariableInitExpression(
        syntax, sym,
        MarkDirectListInitializer(ParseBracedInitializer(syntax)));
  }
  if (initializer == NULL) {
    if (TypeContainsAuto(sym->type)) {
      SyntaxError(syntax, "auto variable requires an initializer");
    } else if (sym->flags.is_constexpr || sym->flags.is_constinit) {
      if (!CXXDeferConstexprDefaultInitialization(syntax, sym->type)) {
        initializer = SyntaxNewCXXConstexprDefaultInitializer(
            syntax, sym, /*local=*/true);
        if (initializer == NULL) {
          SyntaxError(syntax, sym->flags.is_constinit
                                  ? "constinit variable requires an initializer"
                                  : "constexpr variable requires an initializer");
        }
      }
    } else if (!StorageIs(storage, STO(extern)) &&
               StorageIs(storage, STO(thread))) {
      initializer = NewCXXThreadLocalGuardedConstructor(syntax, sym);
    } else {
      initializer =
          SyntaxNewCXXDefaultConstructorCallIfNeeded(syntax, sym);
    }
  } else if (!StorageIs(storage, STO(extern)) &&
             StorageIs(sym->storage, STO(thread)) &&
             initializer->op == AST_OP(call) &&
             CXXThreadLocalInitializerIsDynamic(initializer)) {
    initializer = NewCXXThreadLocalGuardedInit(syntax, sym, initializer);
  }
  if (!syntax->parsing_template_declaration &&
      TypeIsClassTemplatePlaceholder(sym->type)) {
    SyntaxError(syntax,
                "Class template argument deduction requires an initializer");
  }
  return initializer;
}

// Appends the declaration of block-scope object `sym` to `declarations`.
// Constant and `auto` variables are analyzed immediately so that later code
// in the block sees their values and deduced types; local statics and
// thread_locals are also recorded on the syntax for later preparation.
static void AppendLocalVariableDeclaration(TypeParser* parser,
                                           Vector* declarations, Symbol* sym,
                                           Storage storage,
                                           ASTNode* initializer) {
  Syntax* syntax = parser->syntax;
  ASTNode* decl = NewVariableDeclarationASTNode(
      sym, initializer, syntax->lex->current_token_location);
  if (StorageIs(storage, STO(static)) &&
      !StorageIs(storage, STO(thread))) {
    ((VariableDeclarationASTNode*)decl)->local_static_init_kind =
        CompilerIsCXX() ? kLocalStaticInitUnclassified
                        : kLocalStaticInitConstant;
  }
  VectorAppend(declarations, decl);
  // Fold a const scalar (and constexpr / constinit) at the declaration so
  // a later constant use in this block can see the value.  Do not do that
  // when the initializer is still template-dependent: member access such
  // as `rep.rep->length` is typed against the uninstantiated class, and
  // that type is kept when the function template is instantiated.
  bool eager_const_value =
      (sym->flags.is_constexpr || sym->flags.is_constinit ||
       (TypeIsConst(sym->type) && !TypeIsStructOrUnion(sym->type))) &&
      !ExpressionIsTemplateDependent(initializer);
  // Deduce `auto` eagerly at parse time. Block-scope declarations are
  // otherwise analyzed only after the whole function body is parsed, so
  // a later `decltype(var)` in the same block (resolved during parsing)
  // would still see the undeduced `auto` type. Analyzing here fixes the
  // symbol's type up front; the definition is idempotent under the
  // later body-analysis pass. Skip this inside a template, where the
  // initializer may be dependent and unanalyzable until instantiation
  // (decltype defers via dependent_decltype_expr there anyway).
  bool eager_auto =
      (TypeContainsAuto(sym->type) ||
       (CompilerIsCXX() && sym->type->declarator == kDeclArray &&
        sym->type->info.array.is_flexible && initializer != NULL)) &&
      syntax->current_template_parameters == NULL &&
      (initializer == NULL ||
       !ASTNodeAny(initializer, InitializerNamesUndeducedAuto, sym));
  if (eager_const_value || eager_auto) {
    SemanticAnalyzeVariableDefinition(syntax,
                                    (VariableDeclarationASTNode*)decl);
  }
  
  if (!StorageIs(storage, STO(extern)) &&
      StorageIs(storage, STO(static) | STO(thread))) {
    VectorAppend(&syntax->local_statics, decl);
  }
}

// Declares the block-scope object `sym`: checks its type, parses its
// initializer, enters a deferred `auto` symbol once the initializer is
// parsed, and appends its declaration to `declarations`.
static void DeclareLocalObject(TypeParser* parser, Symbol* sym,
                               Storage storage, bool defer_auto_insert,
                               Vector* declarations) {
  Syntax* syntax = parser->syntax;
  if (!StorageIs(sym->storage, STO(extern))) {
    sym->flags.is_defined = true;
  }
  if (parser->is_inline) {
     SyntaxError(syntax, "inline can only be applied to functions");
  }
  CheckDeclaredObjectType(syntax, sym);
  // Any initializer?
  ASTNode* initializer = ParseLocalObjectInitializer(parser, sym, storage);
  if (defer_auto_insert) {
    CheckLocalVariableShadow(syntax, sym);
    if (!SyntaxAddSymbol(syntax, sym)) {
      SyntaxError(syntax, "Duplicate symbol %s", sym->name.value);
    } else {
      MaybeInjectBlockScopeAnonymousAggregateMembers(syntax, sym);
    }
  }
  ValidateC23AutoInitializer(syntax, sym, initializer);
  ValidateC23ConstexprObject(syntax, sym, initializer);
  AppendLocalVariableDeclaration(parser, declarations, sym, storage,
                                 initializer);
}

// Parses the init-declarator list of a block-scope declaration whose
// specifiers (`type`, `storage`, leading `attributes`) have been parsed,
// appending a declaration for each object to `declarations`.  A lone
// anonymous struct or union declares its members in the enclosing block.
static void ParseLocalDeclarationList(TypeParser* parser,
                                      TypeRecord* type, Storage storage,
                                      Vector* attributes, Vector* declarations) {
  Syntax* syntax = parser->syntax;
  while (!LexEof(syntax->lex)) {
    bool implicit_anonymous_aggregate =
        LexLookingAt(syntax->lex, TOK(semicolon)) &&
        declarations->length == 0 &&
        LocalDeclarationTypeIsAnonymousAggregate(parser, type);
    if (LexLookingAt(syntax->lex, TOK(semicolon)) &&
        !implicit_anonymous_aggregate) {
      // We don't need to have a variable declaration.
      break;
    }
    Symbol* sym = TypeParserParseDeclarator(parser, type);
    while (SyntaxParseAnyAttribute(syntax, attributes) ||
           SyntaxParseCXXAlignas(syntax, attributes)) {
    }
    bool defer_auto_insert = false;
    ValidateC23AutoDeclarator(syntax, sym);
    if (sym != NULL) {
      ApplyLocalDeclaratorSpecifiers(parser, sym, storage);
      sym = ResolveLocalDeclarator(parser, sym, storage, &defer_auto_insert);
    }

    // If we don't have a symbol in the type declarator we look for a comma and
    // continue parsring.
    if (sym == NULL) {
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      TypeParserReset(parser);
      continue;
    }

    // Parse trailing __attribute__ / __declspec / C++ attribute syntax.
    while (SyntaxParseAnyAttribute(syntax, attributes)) {
    }

    // Symbol takes ownerhip of attribute strings.
    VectorAppendVector(&sym->attributes, attributes);
    VectorClear(attributes);
    SyntaxApplyDeclarationAttributes(syntax, sym);
    if (StorageIs(storage, STO(static)) && !TypeIsFunction(sym->type)) {
      SetCXXInlineLocalStaticAsmName(sym, "");
    }
    
    // Check for thread-local violations.
    SyntaxCheckThreadLocal(syntax, sym, kParsingBlockScope, false, false);

    // Declaring or defining a function?
    if (TypeIsFunction(sym->type)) {
      DeclareLocalFunction(parser, sym);
    } else {
      DeclareLocalObject(parser, sym, storage, defer_auto_insert,
                         declarations);
    }

    if (IsC23InferredAutoType(type) &&
        LexLookingAt(syntax->lex, TOK(comma))) {
      SyntaxError(syntax,
                  "C23 inferred auto declaration must contain one declarator");
    }
    if (implicit_anonymous_aggregate) {
      break;
    }
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
    TypeParserReset(parser);
  }
}

// Parses a local symbol declaration or definition.  This occurs inside a
// function.  The symbol is added to the local scope (top symbol table in the
// local symbol stack).
static ASTNode* ParseLocalDeclarationImpl(Syntax* syntax,
                                          bool require_semicolon) {
  if (LexLookingAt(syntax->lex, TOK(static_assert))) {
    return SyntaxParseStaticAssert(syntax);
  }
  if (LexLookingAt(syntax->lex, TOK(using))) {
    return ParseUsingDeclaration(syntax);
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(namespace))) {
    SourceLocation location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    if (!LookingAtNamespaceAliasDefinition(syntax)) {
      SyntaxError(syntax, "namespace definition is not allowed at block scope");
    }
    return ParseNamespaceAliasDefinition(syntax, location);
  }

  Vector* declarations = NewVector();

  Storage storage = STO(implicit);
  bool is_inline = false;
  bool is_constexpr = false;
  bool is_consteval = false;
  bool is_constinit = false;
  bool is_explicit = false;
  TypeRecord* type = NULL;
  Vector attributes = {0};
  syntax->context = kParsingBlockScope;
  
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &is_constexpr,
                            &is_consteval, &is_constinit,
                            &is_explicit, &type, &attributes,
                            kParsingBlockScope);

  if (is_inline) {
    SyntaxError(syntax, "inline is not allowed here");
  }
  if (is_explicit) {
    SyntaxError(syntax, "explicit is not allowed here");
  }
  
  // Parse the type specifier (char, unsigned int, etc.)
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage, kParsingBlockScope);
  parser.is_constexpr = is_constexpr;
  parser.is_consteval = is_consteval;
  parser.is_constinit = is_constinit;

  // Claim a reference on the freshly built base type for the duration of
  // declarator parsing.  Each declarator that adopts it takes its own
  // reference, so releasing ours afterwards frees the base type when no
  // declarator used it (e.g. a bare `struct S { ... };`).
  TypeRecordIncRef(type);

  // Now we get a sequence of declarations, separated by commas.
  if (!TryParseStructuredBindingDeclaration(syntax, type, storage,
                                            parser.is_constexpr,
                                            parser.is_constinit,
                                            declarations)) {
    ParseLocalDeclarationList(&parser, type, storage, &attributes,
                              declarations);
  }
  TypeParserDestruct(&parser);
  TypeRecordDelete(type);

  if (require_semicolon) {
    // Stop at a closing brace as well: a missing ';' after a local declaration
    // must not skip the '}' that ends the function and swallow what follows.
    SyntaxNeedSemicolon(syntax, TC(type) | TC(closebrace));
  }

  AttributeListDestruct(&attributes);
  
  return NewDeclarationListASTNode(declarations,
                                   syntax->lex->current_token_location);
}

// Parses a block-scope declaration through its terminating semicolon and
// returns a declaration-list AST node.
ASTNode* SyntaxParseLocalDeclaration(Syntax* syntax) {
  return ParseLocalDeclarationImpl(syntax, /*require_semicolon=*/true);
}

// Parses a C++ condition-declaration ("if (T x = init)", "while (T* p = q)",
// "switch (T c = get())").  Identical to a local declaration except that the
// terminating ')' takes the place of the usual ';', so no semicolon is
// consumed.  The declared symbol is added to the current (caller-opened) scope.
ASTNode* SyntaxParseConditionDeclaration(Syntax* syntax) {
  return ParseLocalDeclarationImpl(syntax, /*require_semicolon=*/false);
}
