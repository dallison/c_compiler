//
//  expr_parser_reflect.c
//  c_compiler
//
//  Reflection, token-sequence literals, and preprocessor probes.
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


static void AppendTokenSequenceRawPiece(Lex* lex, Vector* tokens);
static void ParseTokenSequenceInterpolator(Syntax* syntax, Vector* tokens, TokenClass followers);
static bool PreprocessorFeatureIsSupported(const char* name);
static int64_t CXXAttributeProbeValue(const char* attribute_namespace, const char* name);
static int64_t CAttributeProbeValue(const char* attribute_namespace, const char* name);
static bool ParsePreprocessorProbeName(Syntax* syntax, String* name_space, String* name);

// Copies the current token spelling and location into a raw token-sequence
// piece.
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


// Parses a token-sequence interpolation and appends the reflected splice
// piece.
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


// Parses a balanced token-sequence literal, preserving raw tokens and
// interpolations.
ASTNode* ParseTokenSequenceLiteral(Syntax* syntax, TokenClass followers,
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


// Reports whether a named C++ language feature is supported for __has_feature.
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


// Returns the standardized version value for a C++ attribute probe, or zero
// when unsupported.
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


// Returns the standardized version value for a C attribute probe, or zero when
// unsupported.
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


// Parses an optional namespace and name used by __has_attribute-style probes.
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


// Parses defined and supported __has_* preprocessor operators, returning NULL
// for an ordinary identifier.
ASTNode* ParsePossiblePreprocessorFunction(Syntax* syntax,
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


// The operand of `^^`. An attribute, a qualified name, a type, or an
// expression each become a different reflection operand kind.
ASTNode* ParseReflectionExpression(Syntax* syntax,
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
