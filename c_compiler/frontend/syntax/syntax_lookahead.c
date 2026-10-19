//
//  syntax_lookahead.c
//  c_compiler
//
//  Token lookahead that decides whether the input starts a type, a
//  declaration, an expression or a special member definition.
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

// Returns true when the current identifier on the physical line is followed by
// `::`, skipping an optional same-line template-argument list after the name.
bool SyntaxCurrentIdentifierFollowedByScopeOperator(Syntax* syntax) {
  Lex* lex = syntax->lex;
  if (!LexLookingAt(lex, TOK(identifier))) {
    return false;
  }

  size_t pos = lex->pos;
  while (pos < lex->line.length &&
         isspace((unsigned char)lex->line.value[pos])) {
    pos++;
  }
  if (CompilerIsCXX() && pos < lex->line.length && lex->line.value[pos] == '<') {
    int depth = 1;
    pos++;
    while (pos < lex->line.length && depth > 0) {
      if (lex->line.value[pos] == '<') {
        depth++;
      } else if (lex->line.value[pos] == '>') {
        depth--;
      }
      pos++;
    }
    while (pos < lex->line.length &&
           isspace((unsigned char)lex->line.value[pos])) {
      pos++;
    }
  }
  return pos + 1 < lex->line.length &&
         lex->line.value[pos] == ':' &&
         lex->line.value[pos + 1] == ':';
}

// Returns true when a qualified name starting at the current identifier ends
// with `::*` on the current line; restores the lexer checkpoint before return.
bool SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(Syntax* syntax) {
  Lex* lex = syntax->lex;
  if (!CompilerIsCXX() || !LexLookingAt(lex, TOK(identifier))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(lex, &checkpoint);
  bool result = false;
  // `type_identity<T>::type::*` and `ns::Class::*` are member-pointer
  // declarators.  Walk nested-name components, including template-ids, and
  // stop at `::*`.  Stay on the current line: LexNextToken past EOL runs
  // preprocessor directives, and restoring the checkpoint cannot undo an
  // include that was pushed and popped.
  while (lex->pos < lex->line.length && LexLookingAt(lex, TOK(identifier))) {
    LexNextToken(lex);
    if (lex->pos >= lex->line.length) {
      break;
    }
    if (LexLookingAt(lex, TOK(less))) {
      LexNextToken(lex);
      int depth = 1;
      while (lex->pos < lex->line.length && depth > 0) {
        if (LexLookingAt(lex, TOK(less))) {
          depth++;
        } else if (LexLookingAt(lex, TOK(greater))) {
          depth--;
        }
        if (lex->pos >= lex->line.length) {
          depth = 0;
          break;
        }
        LexNextToken(lex);
      }
      if (lex->pos >= lex->line.length) {
        break;
      }
    }
    if (!LexLookingAt(lex, TOK(coloncolon))) {
      break;
    }
    if (lex->pos >= lex->line.length) {
      break;
    }
    LexNextToken(lex);
    if (LexLookingAt(lex, TOK(star))) {
      result = true;
      break;
    }
  }
  LexCheckpointRestore(lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

// Reads an identifier from `lex->line` starting at `*pos`, appends it to
// `out`, and advances `*pos`; returns false when no identifier is present.
static bool ReadLookaheadIdentifier(Lex* lex, size_t* pos, String* out) {
  while (*pos < lex->line.length &&
         isspace((unsigned char)lex->line.value[*pos])) {
    (*pos)++;
  }
  size_t i = 0;
  while (*pos < lex->line.length) {
    char ch = lex->line.value[*pos];
    if (!(isalnum((unsigned char)ch) || ch == '_')) {
      break;
    }
    StringAppendChar(out, ch);
    i++;
    (*pos)++;
  }
  return i != 0;
}

// Advances `*pos` past a `<...>` template-id on the current physical line when
// one immediately follows the cursor position.
static void SkipLookaheadTemplateId(Lex* lex, size_t* pos) {
  while (*pos < lex->line.length &&
         isspace((unsigned char)lex->line.value[*pos])) {
    (*pos)++;
  }
  if (*pos >= lex->line.length || lex->line.value[*pos] != '<') {
    return;
  }
  int depth = 1;
  (*pos)++;
  while (*pos < lex->line.length && depth > 0) {
    if (lex->line.value[*pos] == '<') {
      depth++;
    } else if (lex->line.value[*pos] == '>') {
      depth--;
    }
    (*pos)++;
  }
  while (*pos < lex->line.length &&
         isspace((unsigned char)lex->line.value[*pos])) {
    (*pos)++;
  }
}

// Returns true when the current token is a C++ special-member decl-specifier
// (`constexpr`, `consteval`, `inline`, or `explicit`).
static bool LookingAtCXXSpecialMemberDeclSpecifier(Lex* lex) {
  return LexLookingAt(lex, TOK(constexpr)) ||
         LexLookingAt(lex, TOK(consteval)) ||
         LexLookingAt(lex, TOK(inline)) ||
         LexLookingAt(lex, TOK(explicit));
}

// Consumes leading special-member decl-specifiers and records the matching
// flags on `parser`; `explicit` is consumed without setting a flag.
void ApplyCXXSpecialMemberDeclSpecifiers(TypeParser* parser) {
  while (true) {
    if (LexMatch(parser->lex, TOK(constexpr))) {
      parser->is_constexpr = true;
      continue;
    }
    if (LexMatch(parser->lex, TOK(consteval))) {
      parser->is_consteval = true;
      continue;
    }
    if (LexMatch(parser->lex, TOK(inline))) {
      parser->is_inline = true;
      continue;
    }
    if (LexMatch(parser->lex, TOK(explicit))) {
      continue;
    }
    break;
  }
}

// Returns true when the input on the current line looks like an out-of-line
// constructor, destructor, or conversion operator definition; does not consume
// tokens permanently (uses lexer checkpoints).
bool CurrentLineLooksLikeSpecialMemberDefinition(Syntax* syntax) {
  Lex* lex = syntax->lex;
  LexCheckpoint start;
  LexCheckpointSave(lex, &start);
  Vector attrs = {0};
  while (SyntaxParseAnyAttribute(syntax, &attrs)) {
  }
  VectorClear(&attrs);

  if (CompilerIsCXX() && LookingAtCXXSpecialMemberDeclSpecifier(lex)) {
    while (LookingAtCXXSpecialMemberDeclSpecifier(lex)) {
      LexNextToken(lex);
    }
  }
  String previous;
  StringInit(&previous, NULL);
  String current;
  StringInit(&current, NULL);
  size_t pos = lex->pos;
  bool result = false;
  bool saw_qualifier = false;

  if (LexLookingAt(lex, TOK(identifier))) {
    StringSetString(&previous, &lex->spelling);
    SkipLookaheadTemplateId(lex, &pos);
  } else if (LexLookingAt(lex, TOK(coloncolon))) {
    if (!ReadLookaheadIdentifier(lex, &pos, &previous)) {
      goto done;
    }
    SkipLookaheadTemplateId(lex, &pos);
  } else {
    goto done;
  }

  while (true) {
    StringClear(&current);
    while (pos < lex->line.length &&
           isspace((unsigned char)lex->line.value[pos])) {
      pos++;
    }
    if (pos + 1 >= lex->line.length ||
        lex->line.value[pos] != ':' ||
        lex->line.value[pos + 1] != ':') {
      goto done;
    }
    pos += 2;
    saw_qualifier = true;

    while (pos < lex->line.length &&
           isspace((unsigned char)lex->line.value[pos])) {
      pos++;
    }
    if (strncmp(lex->line.value + pos, "operator", 8) == 0 &&
        (pos + 8 == lex->line.length ||
         !isalnum((unsigned char)lex->line.value[pos + 8]))) {
      result = true;
      break;
    }
    bool is_destructor = pos < lex->line.length && lex->line.value[pos] == '~';
    if (is_destructor) {
      pos++;
      StringAppendChar(&current, '~');
      if (!ReadLookaheadIdentifier(lex, &pos, &current)) {
        goto done;
      }
    } else if (!ReadLookaheadIdentifier(lex, &pos, &current)) {
      goto done;
    }
    SkipLookaheadTemplateId(lex, &pos);

    while (pos < lex->line.length &&
           isspace((unsigned char)lex->line.value[pos])) {
      pos++;
    }
    if (pos + 1 < lex->line.length &&
        lex->line.value[pos] == ':' &&
        lex->line.value[pos + 1] == ':') {
      StringSetString(&previous, &current);
      continue;
    }

    if (pos >= lex->line.length || lex->line.value[pos] != '(') {
      goto done;
    }
    if (current.length > 0 && current.value[0] == '~') {
      result = strcmp(current.value + 1, previous.value) == 0;
    } else {
      result = StringEqualString(&current, &previous);
    }
    break;
  }

done:
  StringDestruct(&current);
  StringDestruct(&previous);
  LexCheckpointRestore(lex, &start);
  LexCheckpointDestruct(&start);
  if (!result && saw_qualifier) {
    // The character lookahead above is intentionally cheap, but `lex->line`
    // ends at a physical newline.  Retry with tokens so an out-of-line
    // constructor whose final `::Name` is wrapped onto the next line is still
    // recognized before ordinary declaration parsing treats the qualifier as
    // a return type.
    LexCheckpoint multiline;
    LexCheckpointSave(lex, &multiline);
    Vector multiline_attrs = {0};
    while (SyntaxParseAnyAttribute(syntax, &multiline_attrs)) {
    }
    VectorClear(&multiline_attrs);
    while (LookingAtCXXSpecialMemberDeclSpecifier(lex)) {
      LexNextToken(lex);
    }
    FullyQualifiedIdentifier name;
    FullyQualifiedIdentifierInit(&name);
    if (SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                           TC(decl)) &&
        name.is_qualified && name.components.length >= 2 &&
        LexLookingAt(lex, TOK(lparen))) {
      String* owner_name =
          name.components.value.p[name.components.length - 2];
      String* member_name =
          name.components.value.p[name.components.length - 1];
      const char* member = member_name->value;
      if (member[0] == '~') {
        member++;
      }
      result = StringEqual(owner_name, member);
    }
    FullyQualifiedIdentifierDestruct(&name);
    LexCheckpointRestore(lex, &multiline);
    LexCheckpointDestruct(&multiline);
  }
  return result;
}

// Determines whether the qualified-name at the current position actually names
// a type.  The shallow `starts-a-qualified-name` test treats every `a::b` as a
// potential type, which misclassifies a qualified *value* such as
// `std::nothrow` as a type and, e.g., breaks `new (std::nothrow) T`.  Resolve
// the name: if it denotes a class/enum tag or a typedef it is a type; if it
// denotes a variable/function it is not.  When the name cannot be resolved
// (a dependent name, an incomplete template-id, etc.) fall back to the
// conservative "could be a type" answer so dependent code keeps parsing.
// The class being defined, or the class of the member function being parsed,
// or one enclosing it, whose injected-class-name is `name`.
static Struct* FindEnclosingClassByInjectedName(Syntax* syntax, String* name) {
  Struct* owner = syntax->cxx_class_head;
  if (owner == NULL && syntax->context == kParsingBlockScope &&
      compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    owner = compiler->current_function->info.function.cxx_member_owner;
  }
  for (; owner != NULL; owner = owner->lexical_parent) {
    if (CurrentClassNameMatchesTypeName(owner, name) &&
        owner->tag_symbol != NULL && owner->tag_symbol->type != NULL &&
        TypeIsStructOrUnion(owner->tag_symbol->type)) {
      return owner;
    }
  }
  return NULL;
}

// Resolves the qualified name at the current token and returns whether it
// denotes a type; restores the lexer and falls back conservatively when lookup
// is inconclusive.
static bool SyntaxQualifiedNameLooksLikeType(Syntax* syntax) {
  if (SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax)) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  if (SyntaxCurrentTokenStartsQualifiedName(syntax)) {
    SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
        syntax, &name, TC(openbra) | TC(stmt));
  } else {
    SyntaxParseFullyQualifiedIdentifier(syntax, &name);
  }
  bool followed_by_assignment = LexLookingAt(syntax->lex, TOK(equal));
  bool decided = false;
  bool is_type = false;
  if (name.is_qualified) {
    if (SyntaxFindQualifiedTag(syntax, &name) != NULL) {
      decided = true;
      is_type = true;
    } else {
      Symbol* symbol = SyntaxFindQualifiedSymbol(syntax, &name);
      if (symbol != NULL) {
        decided = true;
        is_type = StorageIs(symbol->storage, STO(typedef));
      } else if (name.components.length == 2) {
        String* owner_name = name.components.value.p[0];
        String* member_name = name.components.value.p[1];
        Symbol* owner = SyntaxFindTag(syntax, owner_name);
        Struct* owner_struct =
            owner != NULL && owner->type != NULL &&
                    TypeIsStructOrUnion(owner->type)
                ? owner->type->info.struct_info
                : NULL;
        // A member class template's name inside its own definition.
        if (owner_struct == NULL) {
          owner_struct = FindEnclosingClassByInjectedName(syntax, owner_name);
        }
        if (owner_struct != NULL) {
          StructMember* member = FindStructMember(owner_struct, member_name);
          if (member != NULL && member->symbol != NULL) {
            decided = true;
            is_type = StorageIs(member->symbol->storage, STO(typedef));
          }
        }
      }
    }
  }
  FullyQualifiedIdentifierDestruct(&name);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  // When the name positively resolves to a typedef, trust that: a type-name
  // followed by `=` is a defaulted parameter (`void f(T::x = v)`), not an
  // assignment.  The `followed_by_assignment` heuristic below is only meant to
  // steer an *unresolved* qualified name (e.g. a dependent `T::value = 5;`
  // statement) away from being parsed as a declaration, so apply it only when
  // lookup was inconclusive.
  if (decided) {
    return is_type;
  }
  if (followed_by_assignment) {
    return false;
  }
  return SyntaxCurrentTokenStartsQualifiedName(syntax);
}

// Returns true when the current identifier is an injected-class-name of an
// enclosing class being defined or of the current member function's class.
bool SyntaxCurrentClassNameStartsType(Syntax* syntax) {
  return CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(identifier)) &&
         FindEnclosingClassByInjectedName(syntax, &syntax->lex->spelling) !=
             NULL;
}

// For a class name followed by `::`: returns whether the qualified name it
// starts names a type (`C::m` and `C<T>::m` name a type only if `m` does).  A
// pointer-to-member declarator (`C::*`) does not start a type.
static bool ScopedClassNameStartsType(Syntax* syntax) {
  if (SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax)) {
    return false;
  }
  return SyntaxQualifiedNameLooksLikeType(syntax);
}

// Returns true when the current identifier is a DaveCC type-trait builtin
// followed by `(`, which yields a type.
static bool IdentifierStartsTypeTraitBuiltin(Syntax* syntax) {
  if (!CompilerIsCXX() ||
      !SyntaxIdentifierStartsDaveCCTypeTraitBuiltin(syntax)) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool is_type_trait = LexLookingAt(syntax->lex, TOK(lparen));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return is_type_trait;
}

// Returns whether the current identifier, which names no ordinary symbol,
// starts a type: an inherited injected-class-name, a class tag, the name of
// an enclosing class, or a qualified name that resolves to a type.
static bool UndeclaredIdentifierStartsType(Syntax* syntax) {
  if (FindCurrentInheritedInjectedClassName(
          syntax, &syntax->lex->spelling) != NULL) {
    return true;
  }
  if (CompilerIsCXX() &&
      SyntaxFindTag(syntax, &syntax->lex->spelling) != NULL) {
    if (SyntaxCurrentIdentifierFollowedByScopeOperator(syntax)) {
      return ScopedClassNameStartsType(syntax);
    }
    return true;
  }
  if (SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax)) {
    return false;
  }
  if (SyntaxCurrentClassNameStartsType(syntax) &&
      !SyntaxCurrentIdentifierFollowedByScopeOperator(syntax)) {
    return true;
  }
  return SyntaxQualifiedNameLooksLikeType(syntax);
}

// Returns true when the current identifier, a concept name, is followed by
// an optional template argument list and `auto` (`Concept<T> auto x`).
static bool ConceptNameStartsConstrainedAuto(Syntax* syntax) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  if (LexLookingAt(syntax->lex, TOK(less))) {
    int depth = 0;
    int paren_depth = 0;
    int square_depth = 0;
    int brace_depth = 0;
    while (!LexEof(syntax->lex)) {
      Token t = syntax->lex->current_token;
      if (t == TOK(lparen)) {
        paren_depth++;
      } else if (t == TOK(rparen)) {
        paren_depth--;
      } else if (t == TOK(lsquare)) {
        square_depth++;
      } else if (t == TOK(rsquare)) {
        square_depth--;
      } else if (t == TOK(lbrace)) {
        brace_depth++;
      } else if (t == TOK(rbrace)) {
        brace_depth--;
      } else if (paren_depth == 0 && square_depth == 0 &&
                 brace_depth == 0) {
        if (t == TOK(less)) {
          depth++;
        } else {
          depth -= LexClosingAngleCount(t);
        }
      }
      LexNextToken(syntax->lex);
      if (depth <= 0) {
        break;
      }
    }
  }
  bool constrained_auto = LexLookingAt(syntax->lex, TOK(auto));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return constrained_auto;
}

// Returns whether the current identifier starts a type: a typedef or class
// name (checking the full qualified name for `C::m`), a type-trait builtin,
// a constrained `auto`, or a qualified name resolving to a type.
static bool IdentifierStartsType(Syntax* syntax) {
  if (CompilerIsCXX() && syntax->parsing_friend_type_specifier &&
      SyntaxCurrentIdentifierFollowedByScopeOperator(syntax) &&
      !SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax)) {
    return true;
  }
  if (IdentifierStartsTypeTraitBuiltin(syntax)) {
    return true;
  }
  Symbol* sym = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
  if (sym == NULL) {
    return UndeclaredIdentifierStartsType(syntax);
  }
  if (StorageIs(sym->storage , STO(typedef))) {
    // `C::m` and `C<T>::m` name a type only if `m` does.
    if (CompilerIsCXX() && sym->type != NULL &&
        TypeIsStructOrUnion(sym->type) &&
        SyntaxCurrentIdentifierFollowedByScopeOperator(syntax)) {
      return ScopedClassNameStartsType(syntax);
    }
    return true;
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX20) && sym->flags.is_concept &&
      ConceptNameStartsConstrainedAuto(syntax)) {
    return true;
  }
  // A member function can hide a namespace-scope class with the same
  // spelling.  For a qualified start, inspect the complete name before
  // deciding this is an expression; `ExtensionSet::Prefetch value` is a
  // declaration even inside a class that has `ExtensionSet(...)`.
  if (CompilerIsCXX() &&
      SyntaxCurrentIdentifierFollowedByScopeOperator(syntax) &&
      !SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax)) {
    return SyntaxQualifiedNameLooksLikeType(syntax);
  }
  return false;
}

// Returns whether the `::` at the current token starts a globally qualified
// type name.  `::new` and `::delete` are expressions naming the global
// allocation functions, never the start of a declaration's type.
static bool GlobalScopeStartsType(Syntax* syntax) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool new_or_delete = LexLookingAt(syntax->lex, TOK(new)) ||
                       LexLookingAt(syntax->lex, TOK(delete));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  if (new_or_delete) {
    return false;
  }
  return SyntaxQualifiedNameLooksLikeType(syntax);
}

// Returns true when the current token starts a type (a type specifier,
// cv-qualifier, constexpr-family specifier, or a name that denotes a type),
// without consuming anything.
bool SyntaxLookingAtType(Syntax* syntax) {
  switch (syntax->lex->current_token) {
    case TOK(atomic):
    case TOK(bitint):
    // Complex and imaginary types are not implemented, but the specifier still
    // starts a type: the type parser diagnoses it and consumes it, which no
    // other parser does.
    case TOK(complex):
    case TOK(imaginary):
    case TOK(char):
    case TOK(char8_t):
    case TOK(char16_t):
    case TOK(char32_t):
    case TOK(int):
    case TOK(int128):
    case TOK(short):
    case TOK(long):
    case TOK(float):
    case TOK(double):
    case TOK(class):
    case TOK(struct):
    case TOK(union):
    case TOK(enum):
    case TOK(bool):
    case TOK(signed):
    case TOK(unsigned):
    case TOK(const):
    case TOK(volatile):
    case TOK(restrict):
    case TOK(void):
    case TOK(wchar_t):
    case TOK(consteval):
    case TOK(constexpr):
    case TOK(constinit):
      return true;
    case TOK(auto):
      return CompilerIsCXX();
    case TOK(decltype):
      return CompilerIsCXX();
    case TOK(typeof):
    case TOK(typeof_unqual):
      return true;
    case TOK(typename):
      return CompilerIsCXX();
    case TOK(identifier):
      return IdentifierStartsType(syntax);
    case TOK(coloncolon):
      return GlobalScopeStartsType(syntax);
    default:
      return false;
  }
}

// A functional-style temporary followed by member access is necessarily an
// expression statement, even when its leading name is also a type.  Without
// this lookahead, `T(value).member()` is sent to the declaration parser as the
// parenthesized declarator `T(value)`.  The same holds for a temporary whose
// parenthesized or braced operand cannot be a declarator (`T{1};`, `T(2);`).
static bool CXXTypeStartsTemporaryExpression(Syntax* syntax) {
  if (!CompilerIsCXX() ||
      (!LexLookingAt(syntax->lex, TOK(identifier)) &&
       !LexLookingAt(syntax->lex, TOK(coloncolon)))) {
    return false;
  }
  Symbol* initial_symbol =
      LexLookingAt(syntax->lex, TOK(identifier))
          ? SyntaxFindSymbol(syntax, &syntax->lex->spelling)
          : NULL;
  bool template_name =
      initial_symbol != NULL && initial_symbol->flags.is_template;
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    LexCheckpoint qualification;
    LexCheckpointSave(syntax->lex, &qualification);
    LexNextToken(syntax->lex);
    bool qualified = LexLookingAt(syntax->lex, TOK(coloncolon));
    LexCheckpointRestore(syntax->lex, &qualification);
    LexCheckpointDestruct(&qualification);
    bool qualified_type_name =
        qualified && SyntaxQualifiedNameLooksLikeType(syntax);
    bool type_name =
        initial_symbol != NULL &&
        (initial_symbol->flags.is_template ||
         StorageIs(initial_symbol->storage, STO(typedef)) ||
         (initial_symbol->type != NULL &&
          TypeIsStructOrUnion(initial_symbol->type)));
    if (!type_name &&
        SyntaxFindTag(syntax, &syntax->lex->spelling) == NULL &&
        !qualified_type_name) {
      return false;
    }
    template_name |= qualified_type_name;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool is_member_access = false;

  // Keep this lookahead lexical. Running TypeParserParseType here is not
  // speculative: declarations and template instantiations it encounters can
  // mutate symbol tables even after the lexer checkpoint is restored.
  if (LexLookingAt(syntax->lex, TOK(coloncolon))) {
    LexNextToken(syntax->lex);
  }
  bool have_type_name = LexLookingAt(syntax->lex, TOK(identifier));
  while (have_type_name) {
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(less))) {
      if (!template_name) {
        have_type_name = false;
        break;
      }
      int angle_depth = 0;
      do {
        Token token = syntax->lex->current_token;
        if (token == TOK(semicolon)) {
          break;
        }
        if (token == TOK(less)) {
          angle_depth++;
        } else {
          angle_depth -= LexClosingAngleCount(token);
        }
        LexNextToken(syntax->lex);
      } while (!LexEof(syntax->lex) && angle_depth > 0);
      if (angle_depth != 0) {
        have_type_name = false;
        break;
      }
    }
    if (!LexLookingAt(syntax->lex, TOK(coloncolon))) {
      break;
    }
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(template))) {
      LexNextToken(syntax->lex);
    }
    have_type_name = LexLookingAt(syntax->lex, TOK(identifier));
  }

  if (have_type_name && (LexLookingAt(syntax->lex, TOK(lparen)) ||
                         LexLookingAt(syntax->lex, TOK(lbrace)))) {
    // `T{pos}.member()` is the same kind of temporary as `T(pos).member()`.
    // A declaration would put a name between the type and the braces
    // (`T var{pos}`), so a brace immediately after the type name is an
    // expression.
    bool brace = LexLookingAt(syntax->lex, TOK(lbrace));
    Token open = brace ? TOK(lbrace) : TOK(lparen);
    Token close = brace ? TOK(rbrace) : TOK(rparen);
    // At block scope a declarator cannot start with `{`, a literal or
    // `this`, so `T{1};` and `T(2);` are discarded temporaries rather than
    // declarations of an unnamed variable.
    bool block_scope = syntax->context == kParsingBlockScope;
    bool not_declarator = block_scope && brace;
    if (block_scope && !brace) {
      LexCheckpoint open_paren;
      LexCheckpointSave(syntax->lex, &open_paren);
      LexNextToken(syntax->lex);
      switch (syntax->lex->current_token) {
        case TOK(number):
        case TOK(string):
        case TOK(string_wide):
        case TOK(charconst):
        case TOK(charconst_wide):
        case TOK(true):
        case TOK(false):
        case TOK(nullptr):
        case TOK(this):
          not_declarator = true;
          break;
        default:
          break;
      }
      LexCheckpointRestore(syntax->lex, &open_paren);
      LexCheckpointDestruct(&open_paren);
    }
    int depth = 0;
    // `Type(obj.member())(arg)` is a temporary, then a call.  A declarator
    // cannot contain `.` or `->`, so those tokens inside the argument list
    // mean this is an expression even when another `(...)` follows.
    bool interior_member = false;
    do {
      if (depth > 0 &&
          (LexLookingAt(syntax->lex, TOK(dot)) ||
           LexLookingAt(syntax->lex, TOK(arrow)))) {
        interior_member = true;
      }
      if (LexLookingAt(syntax->lex, open)) {
        depth++;
      } else if (LexLookingAt(syntax->lex, close)) {
        depth--;
      }
      LexNextToken(syntax->lex);
    } while (!LexEof(syntax->lex) && depth > 0);
    is_member_access =
        depth == 0 &&
        (not_declarator || interior_member ||
         LexLookingAt(syntax->lex, TOK(dot)) ||
         LexLookingAt(syntax->lex, TOK(arrow)));
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return is_member_access && SyntaxLookingAtType(syntax);
}

// `JoinTupleLoop<0, sizeof...(T)>()` constructs a temporary.  There is no
// declarator name, so it is an expression even though the name is a class
// template.  The following `(...)` is the call of that temporary.
static bool CXXClassTemplateTemporaryLooksLikeExpression(Syntax* syntax) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  Symbol* symbol = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
  if (symbol == NULL || !symbol->flags.is_template || symbol->type == NULL ||
      !TypeIsStructOrUnion(symbol->type)) {
    symbol = SyntaxFindTag(syntax, &syntax->lex->spelling);
  }
  if (symbol == NULL || !symbol->flags.is_template || symbol->type == NULL ||
      !TypeIsStructOrUnion(symbol->type)) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result = false;
  if (LexLookingAt(syntax->lex, TOK(less))) {
    int angle_depth = 0;
    int paren_depth = 0;
    int square_depth = 0;
    do {
      Token token = syntax->lex->current_token;
      if (token == TOK(semicolon) || token == TOK(lbrace) ||
          token == TOK(rbrace)) {
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
      } else if (paren_depth == 0 && square_depth == 0) {
        if (token == TOK(less)) {
          angle_depth++;
        } else {
          angle_depth -= LexClosingAngleCount(token);
        }
      }
      LexNextToken(syntax->lex);
    } while (!LexEof(syntax->lex) && angle_depth > 0);
    if (angle_depth == 0 && LexLookingAt(syntax->lex, TOK(lparen))) {
      int depth = 0;
      do {
        if (LexLookingAt(syntax->lex, TOK(lparen))) {
          depth++;
        } else if (LexLookingAt(syntax->lex, TOK(rparen))) {
          depth--;
        }
        LexNextToken(syntax->lex);
      } while (!LexEof(syntax->lex) && depth > 0);
      result = depth == 0 &&
               (LexLookingAt(syntax->lex, TOK(lparen)) ||
                LexLookingAt(syntax->lex, TOK(semicolon)) ||
                LexLookingAt(syntax->lex, TOK(dot)) ||
                LexLookingAt(syntax->lex, TOK(arrow)));
    }
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

// `foo<Args>(...)` is a function-template call, not a declaration.  Without
// this, a block-scope statement such as `InitializeStorage<T>(args...)` is
// sent to the declaration parser because the name is unknown or only visible
// as a later class member, and the pack expansion then fails to parse.
static bool CXXTemplateIdLooksLikeCallExpression(Syntax* syntax) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  Symbol* symbol = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
  bool is_function = false;
  if (symbol != NULL && symbol->type != NULL &&
      (TypeIsFunction(symbol->type) || symbol->flags.is_template)) {
    is_function = TypeIsFunction(symbol->type) || symbol->flags.is_template;
    if (StorageIs(symbol->storage, STO(typedef)) &&
        !TypeIsFunction(symbol->type)) {
      is_function = false;
    }
  }
  if (!is_function) {
    Struct* owner = syntax->cxx_class_head;
    if (owner == NULL && syntax->context == kParsingBlockScope &&
        compiler->current_function != NULL &&
        TypeIsFunction(compiler->current_function)) {
      owner = compiler->current_function->info.function.cxx_member_owner;
    }
    if (owner != NULL) {
      StructMember* member = FindStructMember(owner, &syntax->lex->spelling);
      is_function = member != NULL && member->is_member_function;
    }
  }
  if (!is_function) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result = false;
  if (LexLookingAt(syntax->lex, TOK(less))) {
    int angle_depth = 0;
    int paren_depth = 0;
    int square_depth = 0;
    int brace_depth = 0;
    do {
      Token token = syntax->lex->current_token;
      if (token == TOK(semicolon) || token == TOK(lbrace) ||
          token == TOK(rbrace)) {
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
    } while (!LexEof(syntax->lex) && angle_depth > 0);
    result = angle_depth == 0 && LexLookingAt(syntax->lex, TOK(lparen));
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

// `AssertHelper(type, file, line, "") = message` is a constructor call, not a
// declaration.  A type name followed by a parenthesized list that contains a
// comma cannot be a declarator (`T(a)` can declare `a`; `T(a, b)` cannot).
static bool CXXFunctionalCastLooksLikeExpression(Syntax* syntax) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(identifier)) ||
      !SyntaxLookingAtType(syntax)) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result = false;
  if (LexLookingAt(syntax->lex, TOK(lparen))) {
    int depth = 0;
    bool top_level_comma = false;
    do {
      if (LexLookingAt(syntax->lex, TOK(lparen))) {
        depth++;
      } else if (LexLookingAt(syntax->lex, TOK(rparen))) {
        depth--;
      } else if (depth == 1 && LexLookingAt(syntax->lex, TOK(comma))) {
        top_level_comma = true;
      } else if (LexLookingAt(syntax->lex, TOK(semicolon)) ||
                 LexLookingAt(syntax->lex, TOK(lbrace)) ||
                 LexLookingAt(syntax->lex, TOK(rbrace))) {
        break;
      }
      LexNextToken(syntax->lex);
    } while (!LexEof(syntax->lex) && depth > 0);
    result = top_level_comma && depth == 0;
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

// Returns true when a qualified name starting at the current token is followed
// by `(` as in a call; restores the lexer checkpoint before returning.
static bool CXXQualifiedNameLooksLikeCallExpression(Syntax* syntax) {
  // Recognize any qualified-name start: a leading `::`, a name that a
  // namespace/template prefix makes look qualified, or a plain `ident::`
  // sequence (e.g. a dependent `T::member` where `T` is a template
  // parameter or a concrete `Class::member`).  Without the plain `ident::`
  // case, a dependent qualified call used as an expression statement would
  // be misclassified as a declaration and send the declaration parser into
  // an infinite loop.
  if (!CompilerIsCXX() ||
      !(SyntaxCurrentTokenStartsQualifiedName(syntax) ||
        SyntaxCurrentIdentifierFollowedByScopeOperator(syntax))) {
    return false;
  }

  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                     TC(openbra) | TC(stmt));
  bool result = name.is_qualified && LexLookingAt(syntax->lex, TOK(lparen));
  FullyQualifiedIdentifierDestruct(&name);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

// Returns whether the current token sequence starts a declaration rather than
// an expression, using type and call-expression lookahead without consuming
// input permanently.
bool SyntaxLookingAtDeclaration(Syntax* syntax) {
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      LexLookingAt(syntax->lex, TOK(consteval))) {
    LexCheckpoint block_checkpoint;
    LexCheckpointSave(syntax->lex, &block_checkpoint);
    LexNextToken(syntax->lex);
    bool is_consteval_block = LexLookingAt(syntax->lex, TOK(lbrace));
    LexCheckpointRestore(syntax->lex, &block_checkpoint);
    LexCheckpointDestruct(&block_checkpoint);
    if (is_consteval_block) {
      return false;
    }
  }
  if (SyntaxLookingAtAnyAttribute(syntax)) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    Vector attrs = {0};
    VectorInit(&attrs);
    SyntaxParseAnyAttribute(syntax, &attrs);
    AttributeListDestruct(&attrs);
    bool result = SyntaxLookingAtDeclaration(syntax);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return result;
  }
  if (CXXQualifiedNameLooksLikeCallExpression(syntax)) {
    return false;
  }
  if (CXXFunctionalCastLooksLikeExpression(syntax)) {
    return false;
  }
  if (CXXTemplateIdLooksLikeCallExpression(syntax)) {
    return false;
  }
  if (CXXClassTemplateTemporaryLooksLikeExpression(syntax)) {
    return false;
  }
  if (CXXTypeStartsTemporaryExpression(syntax)) {
    return false;
  }
  switch (syntax->lex->current_token) {
    // No expression can begin with an alignment specifier, so it always
    // introduces a declaration.
    case TOK(alignas):
      return true;
    case TOK(extern):
    case TOK(static):
    case TOK(auto):
    case TOK(register):
    case TOK(thread):
    case TOK(thread_local):
    case TOK(noreturn):
    case TOK(typedef):
    case TOK(using):
    case TOK(namespace):
    case TOK(static_assert):
    case TOK(explicit):
    case TOK(consteval):
    case TOK(constexpr):
    case TOK(constinit):
      return syntax->lex->current_token != TOK(namespace) ||
             (CompilerIsCXX() && syntax->context == kParsingBlockScope);
    default:
      return SyntaxLookingAtType(syntax);
  }
}
