//
//  type_enum.c
//  c_compiler
//

#include "type_enum.h"
#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

#include <assert.h>
#include "ast.h"
#include "compiler.h"
#include "concepts.h"
#include "constexpr.h"
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"

static bool CXXExpressionNamesNonTypeTemplateParameter(ASTNode* node,
                                                       int* index) {
  if (!CompilerIsCXX() || node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL || !id->symbol->flags.is_template_parameter ||
      id->symbol->flags.is_template_type_parameter ||
      id->symbol->template_parameter_index < 0) {
    return false;
  }
  if (index != NULL) {
    *index = id->symbol->template_parameter_index;
  }
  return true;
}

void CheckTagType(TypeParser* parser, Symbol* old,
                         bool is_union, bool is_enum) {
  if (old == NULL) {
    return;
  }
  // Using-directive aliases occupy the tag table but are not themselves a
  // class/enum definition.  The caller reclaims them; do not dereference a
  // NULL type here.
  TypeRecord* type = old->type;
  if (type == NULL) {
    return;
  }
  bool error = false;
  if (TypeIsEnum(type)) {
    error = !is_enum;
  } else if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    error = type->info.struct_info->is_union != is_union;
  } else {
    error = true;
  }
  if (error) {
    SyntaxError(parser->syntax,
              "Tag %s declared with different tag type",
              old->name.value);
  }
}

void AddInjectedEnumName(TypeParser* parser, Symbol* tag) {
  if (!CompilerIsCXX() || tag == NULL || tag->flags.invented ||
      tag->type == NULL || tag->type->info.enum_info == NULL) {
    return;
  }
  if (SyntaxFindSymbol(parser->syntax, &tag->name) != NULL) {
    return;
  }

  Symbol* alias = NewSymbol(tag->name.value, tag->type, STO(typedef));
  alias->namespace_ = tag->namespace_;
  if (!SyntaxAddSymbol(parser->syntax, alias)) {
    SymbolDelete(alias);
  }
}

static bool EnumUnderlyingTypesMatch(Enum* e, TypeRecord* type) {
  return e != NULL && type != NULL && e->has_fixed_underlying &&
         e->fixed_underlying_type == type->type &&
         e->fixed_underlying_size == type->size &&
         e->fixed_underlying_bit_width == type->bit_width;
}

static void SetEnumFixedUnderlying(Enum* e, TypeRecord* type) {
  if (e == NULL || type == NULL) {
    return;
  }
  e->has_fixed_underlying = true;
  e->fixed_underlying_type = type->type;
  e->fixed_underlying_size = type->size;
  e->fixed_underlying_bit_width = type->bit_width;
}

static TypeRecord* ParseEnumUnderlyingType(TypeParser* parser) {
  if (!(CompilerIsCXX() || CompilerCAtLeast(kLanguageStandardC23)) ||
      !LexLookingAt(parser->lex, TOK(colon))) {
    return NULL;
  }

  // In C23, `enum E` can itself be a type-name followed by a grammar colon
  // (for example in a generic association).  Treat ':' as an underlying-type
  // introducer only when a type actually follows it.
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  LexNextToken(parser->lex);
  if (!SyntaxLookingAtType(parser->syntax)) {
    LexCheckpointRestore(parser->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);

  TypeParser underlying_parser;
  TypeParserInit(&underlying_parser, parser->lex, parser->syntax, STO(implicit),
                 parser->context);
  TypeRecord* type = TypeParserParseType(&underlying_parser, true);
  TypeParserDestruct(&underlying_parser);
  if (type == NULL) {
    type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }
  if (!TypeIsIntegral(type) || TypeIsEnum(type) ||
      (!CompilerIsCXX() && (TypeIsBool(type) || TypeIsAtomic(type)))) {
    SyntaxError(parser->syntax, "Enum underlying type must be integral");
    TypeRecordDelete(type);
    type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  } else if (!CompilerIsCXX()) {
    // C23 ignores qualifiers on the specified integer type.
    type->qualifiers = kQualPlain;
  }
  return type;
}

bool EnumFixedValueIsRepresentable(const Enum* e, int64_t value) {
  if (e == NULL || !e->has_fixed_underlying ||
      e->fixed_underlying_size <= 0) {
    return true;
  }
  int bits = (e->fixed_underlying_type & kTypeBitInt) != 0
                 ? e->fixed_underlying_bit_width
                 : e->fixed_underlying_size * 8;
  if ((e->fixed_underlying_type & kTypeUnsigned) != 0) {
    if (value < 0) {
      return false;
    }
    return bits >= 63 ||
           (uint64_t)value <= (UINT64_C(1) << bits) - UINT64_C(1);
  }
  if (bits >= 64) {
    return true;
  }
  int64_t minimum = -(INT64_C(1) << (bits - 1));
  int64_t maximum = (INT64_C(1) << (bits - 1)) - INT64_C(1);
  return value >= minimum && value <= maximum;
}

EnumValueAssignStatus EnumAssignEnumeratorValue(Enum* e, bool has_explicit_value,
                                                int64_t explicit_value,
                                                bool* inout_implicit_overflow,
                                                int64_t* out_assigned_value) {
  if (e == NULL || inout_implicit_overflow == NULL ||
      out_assigned_value == NULL) {
    return kEnumValueAssignOverflow;
  }
  if (has_explicit_value) {
    *out_assigned_value = explicit_value;
    *inout_implicit_overflow = false;
  } else {
    if (*inout_implicit_overflow) {
      return kEnumValueAssignOverflow;
    }
    *out_assigned_value = e->next_value;
  }
  if (!EnumFixedValueIsRepresentable(e, *out_assigned_value)) {
    return kEnumValueAssignNotRepresentable;
  }
  if (*out_assigned_value == INT64_MAX) {
    *inout_implicit_overflow = true;
  } else {
    e->next_value = *out_assigned_value + 1;
    *inout_implicit_overflow = false;
  }
  return kEnumValueAssignOk;
}

Symbol* EnumAddScopedConstant(Enum* e, TypeRecord* enum_type, const char* name,
                              int64_t value) {
  if (e == NULL || enum_type == NULL || name == NULL) {
    return NULL;
  }
  Symbol* constant = NewScopedEnumConstant(name, value, enum_type);
  VectorAppend(&e->constants, constant);
  return constant;
}

void EnumRemoveConstantsFrom(Enum* e, size_t old_count) {
  if (e == NULL) {
    return;
  }
  while (e->constants.length > old_count) {
    Symbol* constant = e->constants.value.p[e->constants.length - 1];
    VectorPop(&e->constants);
    SymbolDelete(constant);
  }
}

static Type EnumInferUnderlyingTypeFromConstants(const Enum* e) {
  enum TypeSelection {
    kUnsignedChar,
    kSignedChar,
    kUnsignedInt,
    kSignedInt,
  } type_selection = kUnsignedInt;
  for (size_t i = 0; i < e->constants.length; i++) {
    Symbol* constant = e->constants.value.p[i];
    if (constant == NULL || !constant->flags.value_set) {
      continue;
    }
    int64_t value = constant->value.ivalue;
    switch (type_selection) {
      case kUnsignedInt:
        if (value < 0) {
          type_selection = kSignedInt;
        }
        break;
      case kUnsignedChar:
        if (value > 255) {
          type_selection = kUnsignedInt;
        }
        if (value < 0) {
          if (value < 256) {
            type_selection = kSignedInt;
          } else {
            type_selection = kUnsignedInt;
          }
        }
        break;
      case kSignedInt:
        break;
      case kSignedChar:
        if (value > 255) {
          type_selection = kSignedInt;
        }
        break;
    }
  }
  switch (type_selection) {
    case kUnsignedInt:
      return kTypeInt | kTypeUnsigned;
    case kUnsignedChar:
      return kTypeChar | kTypeUnsigned;
    case kSignedInt:
      return kTypeInt;
    case kSignedChar:
      return kTypeChar;
  }
}

void EnumCompleteDefinition(Enum* e, TypeRecord* enum_type, Symbol* tag) {
  if (e == NULL || enum_type == NULL || tag == NULL) {
    return;
  }
  Type underlying = e->has_fixed_underlying
                        ? e->fixed_underlying_type
                        : EnumInferUnderlyingTypeFromConstants(e);
  enum_type->type = kTypeEnum | underlying;
  enum_type->size =
      e->has_fixed_underlying ? e->fixed_underlying_size : SizeofType(underlying);
  if (e->has_fixed_underlying) {
    enum_type->bit_width = e->fixed_underlying_bit_width;
  }
  TypeRecordCalculateSize(enum_type);
  if (e->is_scoped) {
    for (size_t i = 0; i < e->constants.length; i++) {
      Symbol* constant = e->constants.value.p[i];
      if (constant == NULL || constant->type == NULL) {
        continue;
      }
      constant->type->type = enum_type->type;
      constant->type->size = enum_type->size;
      constant->type->bit_width = enum_type->bit_width;
    }
  }
  tag->flags.is_forward_declared = false;
  tag->flags.is_defined = true;
}

static void ApplyEnumUnderlyingType(Syntax* syntax, Enum* e,
                                    TypeRecord* enum_type,
                                    TypeRecord* explicit_underlying,
                                    bool is_scoped,
                                    bool was_previously_declared) {
  TypeRecord* fixed_underlying = explicit_underlying;
  if (fixed_underlying == NULL && is_scoped && !e->has_fixed_underlying) {
    fixed_underlying = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }
  if (fixed_underlying == NULL) {
    return;
  }
  if (explicit_underlying != NULL && was_previously_declared &&
      !e->has_fixed_underlying) {
    SyntaxError(syntax,
                "Enum %s cannot be redeclared with a fixed underlying type",
                e->tag_name != NULL ? e->tag_name->value : "<anonymous>");
    return;
  }
  if (e->has_fixed_underlying &&
      !EnumUnderlyingTypesMatch(e, fixed_underlying)) {
    SyntaxError(syntax, "Enum %s redeclared with different underlying type",
                e->tag_name != NULL ? e->tag_name->value : "<anonymous>");
    return;
  }
  SetEnumFixedUnderlying(e, fixed_underlying);
  enum_type->type = kTypeEnum | e->fixed_underlying_type;
  enum_type->size = e->fixed_underlying_size;
  enum_type->bit_width = e->fixed_underlying_bit_width;
  if (explicit_underlying == NULL) {
    TypeRecordDelete(fixed_underlying);
  }
}

static Type ParseEnumConstants(TypeParser* parser, Enum* e,
                               TypeRecord* enum_type) {
  enum TypeSelection {
    kUnsignedChar,      // Not used.
    kSignedChar,        // Not used.
    kUnsignedInt,
    kSignedInt,
  } type_selection = kUnsignedInt;
  bool next_value_overflow = false;
  
  while (!LexLookingAt(parser->lex, TOK(rbrace)) && !LexEof(parser->lex)) {
    if (LexLookingAt(parser->lex, TOK(identifier))) {
      String const_name;
      StringInit(&const_name, parser->lex->spelling.value);
      LexNextToken(parser->lex);
      int dependent_value_template_parameter_index = -1;
      int64_t assigned_value = e->next_value;
      EnumValueAssignStatus assign_status = kEnumValueAssignOk;
      if (LexMatch(parser->lex, TOK(equal))) {
        // Followers must include ',' / '}' / ')' so recovery from a nested
        // type definition in the initializer does not skip the enumerator
        // list's closing brace (that skip left ParseStructMembers spinning).
        ASTNode* value = SyntaxParseSingleExpression(
            parser->syntax, TC(semicolon) | TC(exprsep) | TC(closebrace) |
                                TC(closebra));
        value = AnalyzeExpression(value);
        int64_t next_value = e->next_value;
        if (!EvaluateIntegerExpression(value, &next_value)) {
          if (!CXXExpressionNamesNonTypeTemplateParameter(
                  value, &dependent_value_template_parameter_index)) {
            SyntaxError(parser->syntax,
                        "Constant integer expression required for value of "
                        "enum constant %s",
                        const_name.value);
          }
          assign_status = EnumAssignEnumeratorValue(
              e, true, e->next_value, &next_value_overflow, &assigned_value);
        } else {
          assign_status = EnumAssignEnumeratorValue(
              e, true, next_value, &next_value_overflow, &assigned_value);
        }
        ASTNodeDelete(value);
      } else {
        assign_status = EnumAssignEnumeratorValue(
            e, false, 0, &next_value_overflow, &assigned_value);
      }
      if (assign_status == kEnumValueAssignOverflow) {
        SyntaxError(parser->syntax,
                    "Value of enum constant %s exceeds the supported integer "
                    "constant range",
                    const_name.value);
      } else if (assign_status == kEnumValueAssignNotRepresentable) {
        SyntaxError(parser->syntax,
                    "Value %" PRId64
                    " of enum constant %s is not representable in the "
                    "fixed underlying type",
                    assigned_value, const_name.value);
      }
      switch (type_selection) {
        case kUnsignedInt:
          if (assigned_value < 0) {
            type_selection = kSignedInt;
          }
          break;
        case kUnsignedChar:
          if (assigned_value > 255) {
            type_selection = kUnsignedInt;
          }
          if (assigned_value < 0) {
            if (assigned_value < 256) {
              type_selection = kSignedInt;
            } else {
              type_selection = kUnsignedInt;
            }
          }
          break;
        case kSignedInt:
          break;
        case kSignedChar:
          if (assigned_value > 255) {
            type_selection = kSignedInt;
          }
          break;
      }

      Symbol* ec = e->is_scoped
          ? NewScopedEnumConstant(const_name.value, assigned_value, enum_type)
          : NewEnumConstant(const_name.value, assigned_value);
      if (!CompilerIsCXX() && e->has_fixed_underlying) {
        TypeRecordDelete(ec->type);
        ec->type = TypeRecordCopy(enum_type);
        ec->type->qualifiers |= kQualConst;
      }
      ec->dependent_value_template_parameter_index =
          dependent_value_template_parameter_index;
      if (dependent_value_template_parameter_index >= 0) {
        ec->flags.value_set = false;
      }
      StringDestruct(&const_name);

      if (e->is_scoped) {
        if (!InsertLocalSymbol(parser->syntax->local_symbol_stack, ec)) {
          SyntaxError(parser->syntax,
                      "Enum constant %s is already defined in this enum",
                      ec->name.value);
        }
        VectorAppend(&e->constants, ec);
      } else {
        // Insert the constant as a symbol in the current scope.
        bool ok = SyntaxAddSymbol(parser->syntax, ec);
        if (!ok) {
          SyntaxError(parser->syntax,
                      "Enum constant %s is already defined in this scope",
                      ec->name.value);
          SymbolDelete(ec);
        } else {
          VectorAppend(&e->constants, ec);
        }
      }
    }
    if (!LexMatch(parser->lex, TOK(comma))) {
      break;
    }
  }
  if (e->has_fixed_underlying) {
    return e->fixed_underlying_type;
  }
  switch (type_selection) {
    case kUnsignedInt:
      return kTypeInt | kTypeUnsigned;
    case kUnsignedChar:
      return kTypeChar | kTypeUnsigned;
    case kSignedInt:
      return kTypeInt;
    case kSignedChar:
      return kTypeChar;
  }
}

static Symbol* ParseEnumBody(TypeParser* parser, String* tag_name,
                             bool is_scoped,
                             TypeRecord* explicit_underlying) {
  // We have an enum body.
  // First check that this is not a duplicate definition.
  Enum* e = NULL;
  bool empty_tag_name = tag_name->length == 0;
  if (empty_tag_name) {
    SyntaxFakeTagName(parser->syntax, tag_name);
  }
  Symbol* tag = SyntaxFindTopScopeTag(parser->syntax, tag_name);
  if (tag != NULL && (tag->flags.is_using_alias || tag->type == NULL)) {
    // A using-directive made this name visible; an enum definition in this
    // scope still introduces a distinct enumeration.
    tag->flags.is_using_alias = false;
    tag->alias_target = NULL;
    tag->type = NULL;
    tag->flags.is_forward_declared = true;
    tag->flags.is_defined = false;
  }
  bool was_previously_declared =
      tag != NULL && tag->type != NULL && TypeIsEnum(tag->type);
  if (tag != NULL && tag->type != NULL && TypeIsEnum(tag->type) &&
      tag->type->info.enum_info != NULL) {
    if (!tag->flags.is_forward_declared) {
      SyntaxError(parser->syntax, "Duplicate definition of enum %s",
                  tag_name->value);
    } else {
      CheckTagType(parser, tag, false, true);
      if (tag->type->info.enum_info->is_scoped != is_scoped) {
        SyntaxError(parser->syntax, "Enum %s redeclared with different scopedness",
                    tag->name.value);
      }
    }
    e = tag->type->info.enum_info;
  } else if (tag != NULL && tag->type != NULL) {
    CheckTagType(parser, tag, false, true);
    // Keep parsing the body on a fresh enumeration so a mismatched tag
    // cannot crash recovery.
    e = NewEnum();
    TypeRecord* type = NewTypeRecordWithSize(kTypeEnum, kQualPlain);
    type->info.enum_info = e;
    tag = NewSymbol(tag_name->value, type, STO(implicit));
    e->tag_name = &tag->name;
    e->tag_symbol = tag;
    e->is_scoped = is_scoped;
    tag->flags.invented = true;
  } else {
    // Tag doesn't exist, or a using-directive alias was reclaimed above.
    e = NewEnum();
    TypeRecord* type = NewTypeRecordWithSize(kTypeEnum, kQualPlain);
    type->info.enum_info = e;
    if (tag == NULL) {
      tag = NewSymbol(tag_name->value, type, STO(implicit));
      if (empty_tag_name) {
        tag->flags.invented = true;
      }
      SyntaxAddTag(parser->syntax, tag);
    } else {
      tag->type = type;
    }
    e->tag_name = &tag->name;
    e->tag_symbol = tag;
    e->is_scoped = is_scoped;
    AddInjectedEnumName(parser, tag);
  }
  if (e == NULL || tag == NULL || tag->type == NULL) {
    SyntaxRecover(parser->syntax, TC(closebra) | TC(semicolon));
    return tag;
  }
  ApplyEnumUnderlyingType(parser->syntax, e, tag->type, explicit_underlying,
                          is_scoped, was_previously_declared);

  // Note in the symbol that this tag is now defined and not
  // forward declared.
  tag->flags.is_forward_declared = false;
  tag->flags.is_defined = true;

  // Now 'tag' will be the struct tag pointer
  // and 'e' will be a pointer to the Enum information.
  e->tag_symbol = tag;
  e->is_scoped = is_scoped;
  if (is_scoped) {
    // Enumerators declared earlier in an enum-specifier are visible to later
    // enumerator initializers, but scoped-enum names must not leak into the
    // enclosing scope.
    SyntaxOpenScope(parser->syntax);
  }
  parser->syntax->parsing_enum_specifier_depth++;
  Type t = ParseEnumConstants(parser, e, tag->type);
  parser->syntax->parsing_enum_specifier_depth--;
  if (is_scoped) {
    SyntaxCloseScope(parser->syntax);
  }
  tag->type->type |= t;
  tag->type->size = SizeofType(t);
  if (e->is_scoped) {
    for (size_t i = 0; i < e->constants.length; i++) {
      Symbol* constant = e->constants.value.p[i];
      constant->type->type |= t;
      constant->type->size = tag->type->size;
    }
  }
  
  SyntaxNeedBracket(parser->syntax, TOK(rbrace),
                    TC(expr) | TC(closebrace) | TC(semicolon));
  return tag;
}

// Parse an enum definition or reference.
Symbol* TypeParserParseEnum(TypeParser* parser) {
  // Parse common __attribute__ syntax.
  Vector attributes = {0};
  while (SyntaxParseAnyAttribute(parser->syntax, &attributes)) {
  }

  String tag_name = {0};
  FullyQualifiedIdentifier qualified_tag = {0};
  bool has_qualified_tag = false;
  Symbol* tag = NULL;
  FullyQualifiedIdentifierInit(&qualified_tag);
  bool is_scoped = false;
  TypeRecord* explicit_underlying = NULL;

  if (CompilerIsCXX() &&
      (LexLookingAt(parser->lex, TOK(class)) ||
       LexLookingAt(parser->lex, TOK(struct)))) {
    is_scoped = true;
    LexNextToken(parser->lex);
    SyntaxParseCXXAttributes(parser->syntax, &attributes);
  }

  if (LexLookingAt(parser->lex, TOK(semicolon))) {
    // Don't consume the semicolon.
    goto done;
  }

  // Read the tag name if there is one.
  if (SyntaxCurrentTokenStartsQualifiedName(parser->syntax)) {
    SyntaxParseFullyQualifiedIdentifier(parser->syntax, &qualified_tag);
    has_qualified_tag = qualified_tag.is_qualified;
    if (!has_qualified_tag) {
      StringSet(&tag_name, FullyQualifiedIdentifierLast(&qualified_tag));
    }
  } else if (LexLookingAt(parser->lex, TOK(identifier))) {
    // Struct tag is present.
    StringSetString(&tag_name, &parser->lex->spelling);
    LexNextToken(parser->lex);
  }
  SyntaxParseCXXAttributes(parser->syntax, &attributes);
  explicit_underlying = ParseEnumUnderlyingType(parser);
  if (LexMatch(parser->lex, TOK(lbrace))) {
    if (parser->syntax->parsing_enum_specifier_depth > 0 &&
        CompilerIsCXX()) {
      SyntaxError(parser->syntax, "Type cannot be defined in an enumeration");
      int depth = 1;
      while (!LexEof(parser->lex) && depth > 0) {
        if (LexLookingAt(parser->lex, TOK(lbrace))) {
          depth++;
        } else if (LexLookingAt(parser->lex, TOK(rbrace))) {
          depth--;
        }
        LexNextToken(parser->lex);
      }
      Enum* dummy_enum = NewEnum();
      TypeRecord* dummy_type = NewTypeRecordWithSize(kTypeEnum, kQualPlain);
      dummy_type->info.enum_info = dummy_enum;
      tag = NewSymbol(SyntaxFakeName(parser->syntax), dummy_type, STO(implicit));
      tag->flags.invented = true;
      dummy_enum->tag_name = &tag->name;
      dummy_enum->tag_symbol = tag;
      SyntaxAddTag(parser->syntax, tag);
      goto done;
    }
    if (has_qualified_tag) {
      SyntaxError(parser->syntax, "Cannot define qualified enum tag %s",
                  qualified_tag.spelling.value);
    }
    tag = ParseEnumBody(parser, &tag_name, is_scoped, explicit_underlying);
  } else {
    // No open brace, this is a reference to an existing enum or the
    // creation of a new one.
    if (tag_name.length == 0 && !has_qualified_tag) {
      // No tag name, nothing to do.
      goto done;
    }
    tag = has_qualified_tag ? SyntaxFindQualifiedTag(parser->syntax, &qualified_tag)
                            : SyntaxFindTag(parser->syntax, &tag_name);
    if (tag == NULL) {
      if (has_qualified_tag) {
        SyntaxError(parser->syntax, "Unknown enum tag %s",
                    qualified_tag.spelling.value);
        goto done;
      }
      // New tag.
      Enum* e = NewEnum();
      TypeRecord* type = NewTypeRecordWithSize(kTypeEnum, kQualPlain);
      type->info.enum_info = e;
      tag = NewSymbol(tag_name.value, type, STO(implicit));
      tag->flags.is_forward_declared = true;
      e->tag_name = &tag->name;
      e->tag_symbol = tag;
      e->is_scoped = is_scoped;
      ApplyEnumUnderlyingType(parser->syntax, e, tag->type, explicit_underlying,
                              is_scoped, false);
      SyntaxAddTag(parser->syntax, tag);
      AddInjectedEnumName(parser, tag);
    } else {
      // Tag already exists, make sure it's the same tag type.
      CheckTagType(parser, tag, false, true);
      if (tag->type != NULL && TypeIsEnum(tag->type) &&
          tag->type->info.enum_info != NULL) {
        tag->type->info.enum_info->tag_symbol = tag;
        if (tag->type->info.enum_info->is_scoped != is_scoped) {
          SyntaxError(parser->syntax,
                      "Enum %s redeclared with different scopedness",
                      tag->name.value);
        }
        ApplyEnumUnderlyingType(parser->syntax, tag->type->info.enum_info,
                                tag->type, explicit_underlying, is_scoped, true);
      }
    }
  }

done:
  if (explicit_underlying != NULL) {
    TypeRecordDelete(explicit_underlying);
  }
  FullyQualifiedIdentifierDestruct(&qualified_tag);
  StringDestruct(&tag_name);
  AttributeListDestruct(&attributes);
  return tag;
}

//
// Type inference functions.
//

