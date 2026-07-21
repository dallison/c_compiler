//
//  type_member.c
//  c_compiler
//
#include "type_class_internal.h"
#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <assert.h>
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
#include "compiler.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"


static StructMember* FindDirectStructMemberByName(Struct* str,
                                                  const char* name);

typedef struct {
  Symbol* symbol;
  CXXConstructorInitList* initializers;
} PendingInlineConstructorPreamble;

typedef struct {
  Symbol* symbol;
  CXXConstructorInitList* initializers;
} TemplateConstructorInitializers;

static void AddOwnedAssociatedConstraint(ConstraintExpr** target,
                                         ConstraintExpr* constraint) {
  if (target == NULL || constraint == NULL) {
    return;
  }
  ConstraintExpr* current = *target;
  if (current == NULL) {
    *target = constraint;
    return;
  }
  *target = NewConjunctionConstraint(current, constraint, constraint->location);
}

static void MoveTemplateParameterConstraints(Vector* parameters,
                                             ConstraintExpr** target) {
  if (parameters == NULL || target == NULL) {
    return;
  }
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* param = parameters->value.p[i];
    if (param == NULL || param->associated_constraint == NULL) {
      continue;
    }
    ConstraintExpr* constraint = param->associated_constraint;
    param->associated_constraint = NULL;
    AddOwnedAssociatedConstraint(target, constraint);
  }
}

static void FinalizeMemberFunctionTemplateConstraints(
    TypeRecord* func, Vector* member_template_parameters,
    ConstraintExpr* member_requires_clause) {
  if (func == NULL || !TypeIsFunction(func)) {
    ConstraintExprDelete(member_requires_clause);
    return;
  }
  MoveTemplateParameterConstraints(member_template_parameters,
                                   &func->info.function.associated_constraint);
  // The callers transfer the parameter entries into the function type and clear
  // the passed-in list before calling us, so a type-constraint written as a
  // constrained template parameter (`template <Concept T>`) now lives on a
  // parameter in `func->template_parameters`.  Collect those too; otherwise the
  // constraint is silently dropped and never participates in overload
  // resolution (unlike a trailing `requires` clause, which is added below).
  MoveTemplateParameterConstraints(&func->info.function.template_parameters,
                                   &func->info.function.associated_constraint);
  if (member_requires_clause != NULL) {
    AddOwnedAssociatedConstraint(&func->info.function.associated_constraint,
                                 member_requires_clause);
  }
}

static Vector pending_inline_constructor_preambles;
static bool pending_inline_constructor_preambles_initialized = false;

static Vector template_constructor_initializers;
static bool template_constructor_initializers_initialized = false;

static CXXMemberUsingDeclaration* NewCXXMemberUsingDeclaration(
    TypeRecord* base_type, const char* member_name, CXXAccess access,
    SourceLocation location, bool is_pack_expansion);
static void ImportCXXMemberUsingDeclaration(TypeParser* parser, Struct* owner,
                                            TypeRecord* base_type,
                                            const char* member_name,
                                            CXXAccess access,
                                            SourceLocation location);
static bool CanOverloadStructMember(StructMember* existing,
                                    StructMember* member);

bool StructMemberIsNestedType(StructMember* member) {
  return member != NULL && member->symbol != NULL &&
         StorageIs(member->symbol->storage, STO(typedef));
}

bool StructHasMemberFunction(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && member->is_member_function) {
      return true;
    }
  }
  return false;
}

static bool TypeIsNamedCXXNestedType(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL) {
    return false;
  }
  if (TypeIsStructOrUnion(type)) {
    return type->info.struct_info != NULL &&
           type->info.struct_info->tag_symbol != NULL &&
           !type->info.struct_info->tag_symbol->flags.invented;
  }
  if (TypeIsEnum(type)) {
    return type->info.enum_info != NULL &&
           type->info.enum_info->tag_symbol != NULL &&
           !type->info.enum_info->tag_symbol->flags.invented;
  }
  return false;
}

static void MarkCXXNestedClassTemplate(TypeRecord* type, Vector* parameters) {
  if (!CompilerIsCXX() || type == NULL || parameters == NULL ||
      !TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return;
  }
  Struct* nested = type->info.struct_info;
  nested->is_template = true;
  nested->template_parameter_count = (int)parameters->length;
  if (nested->tag_symbol != NULL) {
    nested->tag_symbol->flags.is_template = true;
  }
  VectorDestructWithContents(&nested->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&nested->template_parameters);
  for (size_t i = 0; i < parameters->length; i++) {
    VectorAppend(&nested->template_parameters, parameters->value.p[i]);
  }
  parameters->length = 0;
}

// True when `alias_type` is a class/union/enum type that designates exactly the
// same tag as `tag` -- i.e. the existing nested-type member and the type being
// added are the same tag (a forward declaration and its completion share one
// Struct/Enum object).  Used to distinguish redeclaration from a real clash.
static bool NestedTypeMemberDesignatesTag(TypeRecord* alias_type, Symbol* tag) {
  if (alias_type == NULL || tag == NULL) {
    return false;
  }
  if (TypeIsStructOrUnion(alias_type) && alias_type->info.struct_info != NULL) {
    return alias_type->info.struct_info->tag_symbol == tag;
  }
  if (TypeIsEnum(alias_type) && alias_type->info.enum_info != NULL) {
    return alias_type->info.enum_info->tag_symbol == tag;
  }
  return false;
}

static void AddCXXNestedTypeMember(TypeParser* parser, Struct* owner,
                                   TypeRecord* type, CXXAccess access) {
  Symbol* tag = NULL;
  if (TypeIsStructOrUnion(type)) {
    tag = type->info.struct_info != NULL
        ? type->info.struct_info->tag_symbol
        : NULL;
  } else if (TypeIsEnum(type)) {
    tag = type->info.enum_info != NULL
        ? type->info.enum_info->tag_symbol
        : NULL;
  }
  if (tag == NULL) {
    return;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    type->info.struct_info->lexical_parent = owner;
  }
  // Only a *direct* member of this class collides.  A base class's nested type
  // of the same name is lawfully hidden by this one, so do not walk bases here.
  StructMember* existing = MapFindPointerKey(&owner->symbol_table, &tag->name);
  if (existing != NULL) {
    // Completing a previously forward-declared nested type -- `class X;`
    // followed by `class X { ... };` -- reaches here twice for the same tag,
    // which is completed in place.  Recognize that the existing alias already
    // designates this very tag and refresh it to the (now complete) type
    // instead of reporting a spurious duplicate.  A genuine redefinition of
    // the type is diagnosed earlier when the second body is parsed.
    if (existing->symbol != NULL && existing->symbol->type != NULL &&
        NestedTypeMemberDesignatesTag(existing->symbol->type, tag)) {
      existing->symbol->type = TypeRecordCopy(type);
      return;
    }
    SyntaxError(parser->syntax, "Duplicate nested type %s", tag->name.value);
    return;
  }

  Symbol* alias = NewSymbol(tag->name.value, TypeRecordCopy(type), STO(typedef));
  alias->location = tag->location;
  alias->flags.is_template = tag->flags.is_template;
  StructMember* member = NewStructMember(alias);
  member->access = access;
  AddStructMember(parser, owner, member);
}

static void AddCXXUnscopedEnumConstantMembers(TypeParser* parser,
                                              Struct* owner,
                                              TypeRecord* type,
                                              CXXAccess access) {
  if (!CompilerIsCXX() || owner == NULL || type == NULL ||
      !TypeIsEnum(type) || type->info.enum_info == NULL ||
      type->info.enum_info->is_scoped) {
    return;
  }
  Enum* e = type->info.enum_info;
  for (size_t i = 0; i < e->constants.length; i++) {
    Symbol* constant = e->constants.value.p[i];
    if (constant == NULL) {
      continue;
    }
    if (FindStructMember(owner, &constant->name) != NULL) {
      SyntaxError(parser->syntax, "Duplicate enum constant %s",
                  constant->name.value);
      continue;
    }
    StructMember* member = NewStructMember(SymbolClone(constant));
    member->access = access;
    member->is_static = true;
    AddStructMember(parser, owner, member);

    // Make the constant visible by unqualified name within the class body so
    // later member declarations (e.g. array bounds) and inline member bodies
    // can use it, matching C++ class scope rules.
    Symbol* scope_constant = SymbolClone(constant);
    if (!SyntaxAddSymbol(parser->syntax, scope_constant)) {
      SymbolDelete(scope_constant);
    }
  }
}

static void AddCXXNestedAliasMember(TypeParser* parser, Struct* owner,
                                    const char* name, TypeRecord* type,
                                    CXXAccess access,
                                    SourceLocation location,
                                    bool is_template_alias) {
  if (!CompilerIsCXX() || owner == NULL || name == NULL || type == NULL) {
    return;
  }
  // Only a *direct* member of this class collides: a derived class may lawfully
  // redefine (hide) a nested type or typedef inherited from a base class, so the
  // duplicate check must not walk base classes.
  if (FindDirectStructMemberByName(owner, name) != NULL) {
    SyntaxError(parser->syntax, "Duplicate nested type %s", name);
    return;
  }

  Symbol* alias = NewSymbol(name, type, STO(typedef));
  alias->location = location;
  alias->flags.is_template = is_template_alias;
  StructMember* member = NewStructMember(alias);
  member->access = access;
  AddStructMember(parser, owner, member);

  // Also make the alias visible by unqualified name within the class body so
  // later member declarations and inline member bodies can use it as a type.
  // The class-body symbol scope opened in ParseStructBody owns the lookup; the
  // injected symbol is tracked in all_local_symbols and freed at end of
  // compilation, so closing that scope does not free it.
  Symbol* scope_alias = NewSymbol(name, type, STO(typedef));
  scope_alias->location = location;
  scope_alias->flags.is_template = is_template_alias;
  LocalSymbolTable* saved_scope = parser->syntax->local_symbol_stack;
  if (is_template_alias && saved_scope != NULL && saved_scope->prev != NULL) {
    parser->syntax->local_symbol_stack = saved_scope->prev;
  }
  if (!SyntaxAddSymbol(parser->syntax, scope_alias)) {
    SymbolDelete(scope_alias);
  }
  parser->syntax->local_symbol_stack = saved_scope;
}

static void ParseCXXMemberUsingAlias(TypeParser* parser, Struct* owner,
                                     CXXAccess access,
                                     SourceLocation location,
                                     bool is_template_alias) {
  if (!LexLookingAt(parser->lex, TOK(identifier))) {
    SyntaxError(parser->syntax, "Expected alias name after using");
    SyntaxRecover(parser->syntax, TC(semicolon));
    return;
  }
  String alias_name;
  StringInit(&alias_name, parser->lex->spelling.value);
  LexNextToken(parser->lex);
  if (!LexMatch(parser->lex, TOK(equal))) {
    SyntaxError(parser->syntax, "Expected = in using alias declaration");
    StringDestruct(&alias_name);
    SyntaxRecover(parser->syntax, TC(semicolon));
    return;
  }
  TypeRecord* type = TypeParserParseType(parser, true);
  Symbol* parsed = NULL;
  if (type != NULL && !LexLookingAt(parser->lex, TOK(semicolon))) {
    parsed = TypeParserParseDeclarator(parser, type);
  }
  TypeRecord* alias_type = parsed != NULL ? parsed->type : type;
  AddCXXNestedAliasMember(parser, owner, alias_name.value, alias_type, access,
                          location, is_template_alias);
  if (parsed != NULL) {
    SymbolDelete(parsed);
  }
  StringDestruct(&alias_name);
}

static bool CXXMemberUsingLooksLikeAlias(TypeParser* parser) {
  if (!LexLookingAt(parser->lex, TOK(identifier))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  LexNextToken(parser->lex);
  bool is_alias = LexLookingAt(parser->lex, TOK(equal));
  LexCheckpointRestore(parser->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return is_alias;
}

static void ParseCXXMemberUsingDeclaration(TypeParser* parser, Struct* owner,
                                           CXXAccess access,
                                           SourceLocation location) {
  if (CXXMemberUsingLooksLikeAlias(parser)) {
    ParseCXXMemberUsingAlias(parser, owner, access, location,
                             /*is_template_alias=*/false);
    return;
  }

  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(parser->syntax, &name,
                                                         TC(decl))) {
    SyntaxError(parser->syntax, "Expected qualified name after using");
    FullyQualifiedIdentifierDestruct(&name);
    SyntaxRecover(parser->syntax, TC(semicolon));
    return;
  }
  bool is_pack_expansion = LexMatch(parser->lex, TOK(ellipsis));
  if (!name.is_qualified || name.components.length != 2) {
    SyntaxError(parser->syntax,
                "Member using declaration requires Base::member");
    FullyQualifiedIdentifierDestruct(&name);
    return;
  }

  String* base_name = name.components.value.p[0];
  String* member_name = name.components.value.p[1];
  Symbol* base_symbol = SyntaxFindSymbol(parser->syntax, base_name);
  if (base_symbol == NULL) {
    base_symbol = SyntaxFindTag(parser->syntax, base_name);
  }
  if (base_symbol == NULL || base_symbol->type == NULL) {
    SyntaxError(parser->syntax, "No such base class %s", base_name->value);
    FullyQualifiedIdentifierDestruct(&name);
    return;
  }

  TypeRecord* base_type = NULL;
  Vector* base_args = NULL;
  if (base_symbol->flags.is_template && name.template_arguments.length > 0) {
    base_args = TemplateArgumentVectorCopy(name.template_arguments.value.p[0]);
  }
  if (base_symbol->flags.is_template && base_args != NULL &&
      !parser->syntax->parsing_template_declaration &&
      TypeIsStructOrUnion(base_symbol->type)) {
    base_type = InstantiateSimpleClassTemplate(parser, base_symbol, base_args);
    VectorDestructWithContents(base_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    base_args = NULL;
  } else {
    base_type = TypeRecordCopy(base_symbol->type);
    if (base_symbol->flags.is_template && base_args != NULL) {
      base_type->template_origin = base_symbol;
      if (base_type->template_arguments != NULL) {
        VectorDeleteWithContents(
            base_type->template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      base_type->template_arguments = base_args;
      base_args = NULL;
    }
  }
  int placeholder_index = -1;
  bool is_template_parameter_base =
      TypeIsTemplateParameterPlaceholder(base_type, &placeholder_index);
  if (is_pack_expansion &&
      !CurrentTemplateParameterIsPack(parser->syntax, placeholder_index)) {
    SyntaxError(parser->syntax,
                "member using pack expansion requires a template parameter pack");
    TypeRecordDelete(base_type);
    FullyQualifiedIdentifierDestruct(&name);
    return;
  }

  bool is_dependent = is_template_parameter_base ||
                      TypeContainsTemplateParameter(base_type);
  if (is_dependent || parser->syntax->parsing_template_declaration) {
    VectorAppend(&owner->member_using_declarations,
                 NewCXXMemberUsingDeclaration(base_type, member_name->value,
                                              access, location,
                                              is_pack_expansion));
  } else {
    ImportCXXMemberUsingDeclaration(parser, owner, base_type,
                                    member_name->value, access, location);
  }
  TypeRecordDelete(base_type);
  FullyQualifiedIdentifierDestruct(&name);
}

static void ParseCXXMemberTypedef(TypeParser* parser, Struct* owner,
                                  CXXAccess access,
                                  SourceLocation location) {
  Storage old_storage = parser->storage;
  parser->storage = STO(typedef);
  TypeRecord* type = TypeParserParseType(parser, true);
  while (!LexEof(parser->lex)) {
    Symbol* alias = TypeParserParseDeclarator(parser, type);
    if (alias == NULL) {
      SyntaxError(parser->syntax, "Invalid typedef member");
      break;
    }
    AddCXXNestedAliasMember(parser, owner, alias->name.value, alias->type,
                            access, alias->location != 0 ? alias->location
                                                         : location,
                            /*is_template_alias=*/false);
    SymbolDelete(alias);
    if (!LexMatch(parser->lex, TOK(comma))) {
      break;
    }
  }
  parser->storage = old_storage;
}

static CXXMemberUsingDeclaration* NewCXXMemberUsingDeclaration(
    TypeRecord* base_type, const char* member_name, CXXAccess access,
    SourceLocation location, bool is_pack_expansion) {
  CXXMemberUsingDeclaration* decl = malloc(sizeof(CXXMemberUsingDeclaration));
  decl->base_type = TypeRecordCopy(base_type);
  StringInit(&decl->member_name, member_name);
  decl->access = access;
  decl->location = location;
  decl->is_pack_expansion = is_pack_expansion;
  return decl;
}

static StructMember* CloneCXXMemberUsingMember(StructMember* member,
                                               CXXAccess access,
                                               int byte_offset) {
  if (member == NULL || member->symbol == NULL) {
    return NULL;
  }
  StructMember* clone = NewStructMember(SymbolClone(member->symbol));
  clone->byte_offset = byte_offset;
  clone->bit_offset = member->bit_offset;
  clone->bit_size = member->bit_size;
  clone->index = member->index;
  clone->cxx_vcall_offset = member->cxx_vcall_offset;
  clone->is_anon = member->is_anon;
  clone->is_static = member->is_static;
  clone->is_mutable = member->is_mutable;
  clone->is_member_function = member->is_member_function;
  clone->is_using_declaration = true;
  clone->access = access;
  return clone;
}

static void AddCXXMemberUsingFunction(TypeParser* parser, Struct* owner,
                                      StructMember* member) {
  if (member == NULL) {
    return;
  }
  StructMember* existing =
      MapFindPointerKey(&owner->symbol_table, &member->symbol->name);
  if (existing != NULL) {
    if (!CanOverloadStructMember(existing, member)) {
      String member_name;
      StringInit(&member_name, NULL);
      SymbolFunctionDiagnosticName(member->symbol, &member_name);
      SyntaxError(parser->syntax, "Duplicate class member %s",
                  member_name.value);
      StringDestruct(&member_name);
      StructMemberDelete(member);
      return;
    }
    if (FindStructMemberOverload(existing, member->symbol->type) != NULL) {
      String member_name;
      StringInit(&member_name, NULL);
      SymbolFunctionDiagnosticName(member->symbol, &member_name);
      SyntaxError(parser->syntax, "Duplicate class member %s",
                  member_name.value);
      StringDestruct(&member_name);
      StructMemberDelete(member);
      return;
    }
    AppendStructMemberOverload(parser, owner, existing, member);
  } else {
    AddStructMember(parser, owner, member);
  }
}

static void ImportCXXMemberUsingDeclaration(TypeParser* parser, Struct* owner,
                                            TypeRecord* base_type,
                                            const char* member_name,
                                            CXXAccess access,
                                            SourceLocation location) {
  if (owner == NULL || base_type == NULL || member_name == NULL) {
    return;
  }
  TypeRecord* lookup_base =
      TypeMaterializeClassTemplateSpecialization(parser->syntax, base_type);
  if (!TypeIsStructOrUnion(lookup_base) ||
      lookup_base->info.struct_info == NULL) {
    SyntaxError(parser->syntax,
                "member using declaration requires a class base");
    if (lookup_base != base_type) {
      TypeRecordDelete(lookup_base);
    }
    return;
  }
  Struct* base_struct = lookup_base->info.struct_info;
  int base_offset = 0;
  if (!StructHasBaseStruct(owner, base_struct, &base_offset) &&
      !StructHasBaseType(parser->syntax, owner, lookup_base, &base_offset)) {
    SyntaxError(parser->syntax,
                "using declaration base is not a base class");
    if (lookup_base != base_type) {
      TypeRecordDelete(lookup_base);
    }
    return;
  }
  CXXAccess ignored_access = kAccessPublic;
  Struct* ignored_owner = NULL;
  int member_offset = 0;
  StructMember* first = FindStructMemberWithAccessAndOffsetByName(
      base_struct, member_name, &ignored_access, &ignored_owner,
      &member_offset);
  if (first == NULL) {
    SyntaxError(parser->syntax, "No such base class member %s", member_name);
    if (lookup_base != base_type) {
      TypeRecordDelete(lookup_base);
    }
    return;
  }
  if (ignored_access == kAccessPrivate) {
    SyntaxError(parser->syntax, "%s is a private member of %s", member_name,
                ignored_owner != NULL && ignored_owner->tag_name != NULL
                    ? ignored_owner->tag_name->value
                    : "<anonymous>");
    if (lookup_base != base_type) {
      TypeRecordDelete(lookup_base);
    }
    return;
  }
  bool imported = false;
  for (StructMember* member = first; member != NULL;
       member = member->overload_next) {
    int byte_offset = base_offset + member_offset;
    if (member->is_member_function) {
      byte_offset = base_offset + CXXBaseOffsetForMember(base_struct, member);
    }
    StructMember* clone =
        CloneCXXMemberUsingMember(member, access, byte_offset);
    if (clone != NULL) {
      clone->symbol->location = location;
      AddCXXMemberUsingFunction(parser, owner, clone);
      imported = true;
    }
  }
  if (!imported) {
    SyntaxError(parser->syntax, "No such base class member %s", member_name);
  }
  if (lookup_base != base_type) {
    TypeRecordDelete(lookup_base);
  }
}

static void ApplyCXXMemberUsingDeclaration(TypeParser* parser, Struct* owner,
                                           CXXMemberUsingDeclaration* decl,
                                           Vector* args) {
  if (decl == NULL || decl->base_type == NULL) {
    return;
  }
  int pack_index = -1;
  if (decl->is_pack_expansion &&
      TypeIsTemplateParameterPlaceholder(decl->base_type, &pack_index) &&
      pack_index >= 0 && args != NULL && (size_t)pack_index < args->length) {
    TemplateArgument* pack = args->value.p[pack_index];
    if (pack != NULL && pack->pack_arguments != NULL) {
      for (size_t i = 0; i < pack->pack_arguments->length; i++) {
        TemplateArgument* element = pack->pack_arguments->value.p[i];
        if (element == NULL || element->kind != kTemplateParameterType ||
            element->type == NULL) {
          SyntaxError(parser->syntax,
                      "member using pack expansion requires class types");
          continue;
        }
        ImportCXXMemberUsingDeclaration(parser, owner, element->type,
                                        decl->member_name.value, decl->access,
                                        decl->location);
      }
      return;
    }
  }
  TypeRecord* base_type =
      args != NULL ? SubstituteTemplateParameters(parser, decl->base_type, args)
                   : TypeRecordCopy(decl->base_type);
  ImportCXXMemberUsingDeclaration(parser, owner, base_type,
                                  decl->member_name.value, decl->access,
                                  decl->location);
  TypeRecordDelete(base_type);
}

void ApplyCXXMemberUsingDeclarations(TypeParser* parser, Struct* owner,
                                            Struct* template_struct,
                                            Vector* args) {
  if (!CompilerIsCXX() || owner == NULL || template_struct == NULL) {
    return;
  }
  for (size_t i = 0; i < template_struct->member_using_declarations.length;
       i++) {
    ApplyCXXMemberUsingDeclaration(
        parser, owner, template_struct->member_using_declarations.value.p[i],
        args);
  }
}

StructMember* FindStructMember(Struct* str, String* name) {
  StructMember* member = MapFindPointerKey(&str->symbol_table, name);
  if (member != NULL) {
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMember(base->type->info.struct_info, name);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

// A conversion operator is a member function whose synthesized name matches the
// name derived from its result type (e.g. a member named "operator long" whose
// result type is `long`).  Regular operator overloads such as "operator+" never
// satisfy this because their name is unrelated to the return type.
static bool MemberIsConversionOperator(StructMember* member) {
  if (member == NULL || !member->is_member_function || member->symbol == NULL ||
      !TypeIsFunction(member->symbol->type) ||
      member->symbol->type->next == NULL) {
    return false;
  }
  String expected;
  ConversionOperatorName(member->symbol->type->next, &expected);
  bool matches = strcmp(expected.value, member->symbol->name.value) == 0;
  StringDestruct(&expected);
  return matches;
}

// Append every conversion-operator member reachable from `str`, including those
// inherited from base classes, to `out` (a Vector of StructMember*).  Only the
// head of each overload chain is recorded; overloads that share a name (e.g.
// const/non-const) also share a result type, so callers ranking by result type
// do not need the whole chain.
void CollectConversionOperators(Struct* str, Vector* out) {
  if (str == NULL || out == NULL) {
    return;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (MemberIsConversionOperator(member)) {
      VectorAppend(out, member);
    }
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      CollectConversionOperators(base->type->info.struct_info, out);
    }
  }
}

static StructMember* FindDirectStructMemberByName(Struct* str,
                                                  const char* name) {
  if (str == NULL || name == NULL) {
    return NULL;
  }
  return MapFindPointerKey(&str->symbol_name_table, (void*)name);
}

StructMember* FindStructMemberByName(Struct* str, const char* name) {
  if (str == NULL || name == NULL) {
    return NULL;
  }
  StructMember* member = FindDirectStructMemberByName(str, name);
  if (member != NULL) {
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberByName(base->type->info.struct_info, name);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

static CXXAccess CombineInheritedAccess(CXXAccess base_access,
                                        CXXAccess member_access) {
  if (member_access == kAccessPrivate || base_access == kAccessPrivate) {
    return kAccessPrivate;
  }
  if (member_access == kAccessProtected || base_access == kAccessProtected) {
    return kAccessProtected;
  }
  return kAccessPublic;
}

static StructMember* FindStructMemberWithAccessByNameFromBase(
    Struct* str, const char* name, CXXAccess inherited, int inherited_offset,
    CXXAccess* access, Struct** owner, int* byte_offset) {
  if (str == NULL || name == NULL) {
    return NULL;
  }
  StructMember* member = FindDirectStructMemberByName(str, name);
  if (member != NULL) {
    if (access != NULL) {
      *access = CombineInheritedAccess(inherited, member->access);
    }
    if (owner != NULL) {
      *owner = str;
    }
    if (byte_offset != NULL) {
      *byte_offset = inherited_offset + member->byte_offset;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberWithAccessByNameFromBase(
          base->type->info.struct_info, name,
          CombineInheritedAccess(inherited, base->access),
          inherited_offset + base->byte_offset, access, owner, byte_offset);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

static StructMember* FindStructMemberWithAccessFromBase(Struct* str,
                                                        String* name,
                                                        CXXAccess inherited,
                                                        int inherited_offset,
                                                        CXXAccess* access,
                                                        Struct** owner,
                                                        int* byte_offset) {
  StructMember* member = MapFindPointerKey(&str->symbol_table, name);
  if (member != NULL) {
    if (access != NULL) {
      *access = CombineInheritedAccess(inherited, member->access);
    }
    if (owner != NULL) {
      *owner = str;
    }
    if (byte_offset != NULL) {
      *byte_offset = inherited_offset + member->byte_offset;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberWithAccessFromBase(
          base->type->info.struct_info, name,
          CombineInheritedAccess(inherited, base->access),
          inherited_offset + base->byte_offset, access, owner, byte_offset);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

StructMember* FindStructMemberWithAccess(Struct* str, String* name,
                                         CXXAccess* access,
                                         Struct** owner) {
  return FindStructMemberWithAccessAndOffset(str, name, access, owner, NULL);
}

StructMember* FindStructMemberWithAccessByName(Struct* str, const char* name,
                                               CXXAccess* access,
                                               Struct** owner) {
  return FindStructMemberWithAccessAndOffsetByName(str, name, access, owner,
                                                   NULL);
}

StructMember* FindStructMemberWithAccessAndOffset(Struct* str, String* name,
                                                  CXXAccess* access,
                                                  Struct** owner,
                                                  int* byte_offset) {
  StructMember* member = MapFindPointerKey(&str->symbol_table, name);
  if (member != NULL) {
    if (access != NULL) {
      *access = member->access;
    }
    if (owner != NULL) {
      *owner = str;
    }
    if (byte_offset != NULL) {
      *byte_offset = member->byte_offset;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberWithAccessFromBase(
          base->type->info.struct_info, name, base->access, base->byte_offset,
          access, owner, byte_offset);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

StructMember* FindStructMemberWithAccessAndOffsetByName(
    Struct* str, const char* name, CXXAccess* access, Struct** owner,
    int* byte_offset) {
  if (str == NULL || name == NULL) {
    return NULL;
  }
  StructMember* member = FindDirectStructMemberByName(str, name);
  if (member != NULL) {
    if (access != NULL) {
      *access = member->access;
    }
    if (owner != NULL) {
      *owner = str;
    }
    if (byte_offset != NULL) {
      *byte_offset = member->byte_offset;
    }
    return member;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL) {
      member = FindStructMemberWithAccessByNameFromBase(
          base->type->info.struct_info, name, base->access, base->byte_offset,
          access, owner, byte_offset);
      if (member != NULL) {
        return member;
      }
    }
  }
  return NULL;
}

StructMember* FindStructMemberOverload(StructMember* first, TypeRecord* type) {
  for (StructMember* overload = first; overload != NULL;
       overload = overload->overload_next) {
    bool overload_is_template =
        overload->symbol != NULL && overload->symbol->flags.is_template;
    bool type_is_template =
        TypeIsFunction(type) && type->info.function.template_parameter_count > 0;
    if (overload_is_template != type_is_template) {
      continue;
    }
    if (TypeEqual(overload->symbol->type, type)) {
      return overload;
    }
  }
  return NULL;
}

// Like FindStructMemberOverload, but for two function templates that share the
// same signature it additionally requires their associated constraints
// (requires-clauses / constrained template parameters) to be equivalent before
// treating them as the *same* declaration.  This lets a class declare several
// overloads of e.g. `operator()` distinguished solely by their `requires`
// clause, as the range-access CPOs in <ranges> do.
static StructMember* FindConstrainedMemberOverload(StructMember* first,
                                                   Symbol* candidate) {
  if (candidate == NULL) {
    return NULL;
  }
  bool candidate_is_template =
      TypeIsFunction(candidate->type) &&
      candidate->type->info.function.template_parameter_count > 0;
  for (StructMember* overload = first; overload != NULL;
       overload = overload->overload_next) {
    if (overload->symbol == NULL) {
      continue;
    }
    bool overload_is_template = overload->symbol->flags.is_template;
    if (overload_is_template != candidate_is_template) {
      continue;
    }
    if (!TypeEqual(overload->symbol->type, candidate->type)) {
      continue;
    }
    if (overload_is_template &&
        !ConceptsFunctionTemplateConstraintsEquivalent(overload->symbol,
                                                       candidate)) {
      continue;
    }
    return overload;
  }
  return NULL;
}

static bool CheckStructMember(Struct* str, String* name) {
  return MapFindPointerKey(&str->symbol_table, name) == NULL;
}

void StructInsertMemberIntoTables(Struct* str, StructMember* member) {
  if (str == NULL || member == NULL || member->symbol == NULL) {
    return;
  }
  MapKeyValue kv;
  kv.key.p = &member->symbol->name;
  kv.value.p = member;
  MapInsert(&str->symbol_table, kv);
  kv.key.p = member->symbol->name.value;
  MapInsert(&str->symbol_name_table, kv);
}

void StructRebuildMemberLookupTables(Struct* str) {
  if (str == NULL) {
    return;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = (StructMember*)VectorGet(&str->members, i);
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    StructInsertMemberIntoTables(str, member);
  }
}

void AddStructMember(TypeParser* parser, Struct* str, StructMember* member) {
  if (member->is_member_function) {
    RegisterCXXVirtualMember(parser, str, member);
    SymbolSetCXXMangledAsmName(member->symbol);
  } else if (member->is_static && member->symbol != NULL) {
    SymbolSetCXXDataAsmName(member->symbol, str);
  }
  VectorAppend(&str->members, member);
  StructInsertMemberIntoTables(str, member);
}

static bool LexMatchContextualIdentifier(Lex* lex, const char* name) {
  if (!LexLookingAt(lex, TOK(identifier)) ||
      !StringEqual(&lex->spelling, name)) {
    return false;
  }
  LexNextToken(lex);
  return true;
}

static void ParseCXXVirtSpecifiers(TypeParser* parser, TypeRecord* func) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func)) {
    return;
  }
  bool progress = true;
  while (progress) {
    progress = false;
    if (LexMatchContextualIdentifier(parser->lex, "override")) {
      if (func->info.function.is_override) {
        SyntaxError(parser->syntax, "duplicate override specifier");
      }
      func->info.function.is_override = true;
      progress = true;
    } else if (LexMatchContextualIdentifier(parser->lex, "final")) {
      if (func->info.function.is_final) {
        SyntaxError(parser->syntax, "duplicate final specifier");
      }
      func->info.function.is_final = true;
      progress = true;
    }
  }
}

void ParseCXXPureSpecifier(TypeParser* parser, TypeRecord* func) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func) ||
      !LexMatch(parser->lex, TOK(equal))) {
    return;
  }
  if (LexMatch(parser->lex, TOK(default))) {
    func->info.function.is_defaulted = true;
    func->info.function.is_explicitly_defaulted = true;
    func->info.function.is_constexpr_eligible = true;
    func->info.function.is_inline = true;
    return;
  }
  if (LexMatch(parser->lex, TOK(delete))) {
    func->info.function.is_deleted = true;
    func->info.function.is_explicitly_deleted = true;
    return;
  }
  if (!LexLookingAt(parser->lex, TOK(number)) || parser->lex->number != 0) {
    SyntaxError(parser->syntax,
                "function specifier must be '= 0', '= default', or '= delete'");
    if (!LexLookingAt(parser->lex, TOK(semicolon)) &&
        !LexLookingAt(parser->lex, TOK(rbrace))) {
      LexNextToken(parser->lex);
    }
    return;
  }
  LexNextToken(parser->lex);
  if (!func->info.function.is_virtual) {
    SyntaxError(parser->syntax, "pure specifier requires a virtual function");
  }
  func->info.function.is_pure_virtual = true;
}

static bool CanOverloadStructMember(StructMember* existing,
                                    StructMember* member) {
  return CompilerIsCXX() && existing != NULL && member != NULL &&
         existing->is_member_function && member->is_member_function;
}

static bool ParseCXXExplicitSpecifier(TypeParser* parser, bool* saw_explicit) {
  *saw_explicit = false;
  if (!CompilerIsCXX() || !LexMatch(parser->lex, TOK(explicit))) {
    return false;
  }
  *saw_explicit = true;
  if (!LexMatch(parser->lex, TOK(lparen))) {
    return true;
  }
  ASTNode* expr =
      SyntaxParseExpression(parser->syntax, TC(closebra) | TC(exprsep));
  // A value-dependent condition (e.g. `explicit(sizeof(T) > 4)`) is detected on
  // the parsed (pre-analysis) tree, before constant-folding could collapse it
  // to the placeholder's size.  The unanalyzed condition is stashed and
  // re-folded per instantiation (see ParseCXXExplicitDeclarationSpecifier and
  // InstantiateMemberFunctionType); mirrors static_assert deferral.
  if (parser->syntax->parsing_template_declaration &&
      ExpressionIsTemplateDependent(expr)) {
    SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(decl));
    parser->syntax->pending_explicit_condition = expr;
    return true;
  }
  expr = AnalyzeExpression(expr);
  int64_t value = 0;
  bool ok = EvaluateIntegerExpression(expr, &value);
  if (!ok) {
    SyntaxError(parser->syntax,
                "explicit specifier must be a constant expression");
  }
  ASTNodeDelete(expr);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(decl));
  return ok && value != 0;
}

static void SetStructMemberOverloadAsmName(Struct* str,
                                           StructMember* first,
                                           StructMember* overload) {
  (void)str;
  (void)first;
  SymbolSetCXXMangledAsmName(overload->symbol);
}

void AppendStructMemberOverload(TypeParser* parser, Struct* str,
                                       StructMember* first,
                                       StructMember* member) {
  VectorAppend(&str->members, member);
  StructMember* tail = first;
  while (tail->overload_next != NULL) {
    tail = tail->overload_next;
  }
  tail->overload_next = member;
  first->symbol->flags.is_overloaded = true;
  member->symbol->flags.is_overloaded = true;
  RegisterCXXVirtualMember(parser, str, member);
  SetStructMemberOverloadAsmName(str, first, first);
  SetStructMemberOverloadAsmName(str, first, member);
  (void)parser;
}

static bool SkipInlineMemberFunctionBody(TypeParser* parser) {
  if (!LexMatch(parser->lex, TOK(lbrace))) {
    return false;
  }
  int brace_count = 1;
  while (brace_count > 0 && !LexEof(parser->lex)) {
    if (LexLookingAt(parser->lex, TOK(lbrace))) {
      brace_count++;
    } else if (LexLookingAt(parser->lex, TOK(rbrace))) {
      brace_count--;
    }
    LexNextToken(parser->lex);
  }
  return true;
}

static void AddInlineFunctionScopeSymbols(Syntax* syntax, TypeRecord* func) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = formals->value.p[i];
    // A function parameter is a block-scope name and hides a class member of
    // the same name in the body ([basic.scope.block], [basic.lookup.unqual]).
    // Marking it block-scope makes the member name-hiding redirect (see
    // BuildValueName in expr_parser.c) leave references to the parameter alone
    // instead of rewriting them to `this->member`.  This matters once inline
    // bodies are parsed in complete-class context, where a later-declared
    // member (e.g. `data()`/`size()`) is visible while the body is analyzed.
    formal->flags.is_block_scope = true;
    InsertLocalSymbol(syntax->local_symbol_stack, formal);
  }
}

// Parse a constructor/destructor's trailing requires-clause (C++20), e.g.
// `Class() requires C<T> = default;`.  The regular function-declarator path
// handles this in ParseCXXTrailingRequiresClause, but constructors are parsed
// through a dedicated path that must consume the clause before the pure/default
// specifier is examined, otherwise the leftover `requires` token is mistaken
// for the start of a new member declaration.
static void ParseCXXSpecialMemberTrailingRequires(TypeParser* parser,
                                                   TypeRecord* func) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20) || func == NULL ||
      !TypeIsFunction(func) ||
      !LexLookingAt(parser->lex, TOK(requires))) {
    return;
  }
  SyntaxOpenScope(parser->syntax);
  if (parser->cxx_member_owner != NULL) {
    SyntaxInsertClassMembersForConstraint(parser->syntax,
                                          parser->cxx_member_owner);
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL && formal->name.length > 0) {
      InsertLocalSymbol(parser->syntax->local_symbol_stack, formal);
    }
  }
  ConstraintExpr* constraint = ConceptsParseRequiresClause(parser->syntax);
  SyntaxCloseScope(parser->syntax);
  AddOwnedAssociatedConstraint(&func->info.function.associated_constraint,
                               constraint);
}

void QueueInlineMemberFunctionDefinition(Symbol* symbol) {
  Vector* declarations = NewVector();
  VectorAppend(declarations,
               NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
  VectorAppend(&compiler->pending_template_instantiations,
               NewDeclarationListASTNode(declarations, symbol->location));
}

static void QueueInlineConstructorPreamble(Symbol* symbol,
                                           CXXConstructorInitList* initializers) {
  if (symbol == NULL || initializers == NULL) {
    return;
  }
  if (!pending_inline_constructor_preambles_initialized) {
    VectorInit(&pending_inline_constructor_preambles);
    pending_inline_constructor_preambles_initialized = true;
  }
  PendingInlineConstructorPreamble* pending =
      malloc(sizeof(PendingInlineConstructorPreamble));
  pending->symbol = symbol;
  pending->initializers = initializers;
  VectorAppend(&pending_inline_constructor_preambles, pending);
}

static void QueueTemplateConstructorInitializers(
    Symbol* symbol, CXXConstructorInitList* initializers) {
  if (symbol == NULL || initializers == NULL) {
    return;
  }
  if (!template_constructor_initializers_initialized) {
    VectorInit(&template_constructor_initializers);
    template_constructor_initializers_initialized = true;
  }
  TemplateConstructorInitializers* stored =
      malloc(sizeof(TemplateConstructorInitializers));
  stored->symbol = symbol;
  stored->initializers = initializers;
  VectorAppend(&template_constructor_initializers, stored);
}

// Associate the deferred constructor member-initializer list already recorded
// for `from` with `to`.  Used when a class-template instantiation creates a
// class-level member function *template* constructor (`to`) from the primary
// template's constructor (`from`): the preamble is not inserted at
// class-instantiation time (its own template parameters are still unbound), so
// the per-call instantiation must be able to rediscover the init-list keyed on
// the class-level symbol it clones from.
//
// The init-list is cloned (rather than the pointer shared) and its parameter
// references are re-pointed from `from`'s prototype to `to`'s.  The class-level
// constructor `to` has its own freshly cloned prototype, and the per-call
// preamble insertion builds its clone maps off that prototype.  If the shared
// list kept naming `from`'s parameters, those maps would miss them and a pack
// initializer such as `value(std::forward<Args>(args)...)` would fail to expand
// (its `args` pack would never be found), reintroducing the parameter's
// dependent type into `std::forward`'s explicit argument.
void CopyTemplateConstructorInitializersKey(Symbol* from, Symbol* to) {
  if (from == NULL || to == NULL || from == to) {
    return;
  }
  CXXConstructorInitList* inits = FindTemplateConstructorInitializers(from);
  if (inits != NULL && FindTemplateConstructorInitializers(to) == NULL) {
    CXXConstructorInitList* cloned =
        SyntaxCXXConstructorInitListCloneDeferred(inits);
    // `from` (the primary constructor) numbers its own template parameters after
    // the enclosing class's; `to` (the class-level clone) resets its base to 0.
    // Rebase the init-list expressions by the primary's base so the member's own
    // parameters become zero-based, matching `to` and the per-call arguments.
    int rebase_base = TypeIsFunction(from->type)
                          ? from->type->info.function.template_parameter_base
                          : 0;
    SyntaxCXXConstructorInitListRemapFormals(cloned, from->type, to->type,
                                             rebase_base);
    QueueTemplateConstructorInitializers(to, cloned);
  }
}

CXXConstructorInitList* FindTemplateConstructorInitializers(
    Symbol* symbol) {
  if (!template_constructor_initializers_initialized || symbol == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < template_constructor_initializers.length; i++) {
    TemplateConstructorInitializers* stored =
        template_constructor_initializers.value.p[i];
    if (stored != NULL && stored->symbol == symbol) {
      return stored->initializers;
    }
  }
  return NULL;
}

// An inline member function body whose parse was deferred until the enclosing
// class is complete.  `body_text` holds the body's macro-expanded source
// (starting at the opening '{'), captured when the body was first scanned; it is
// replayed from a string source with the preprocessor suppressed so that
// unqualified names bind in complete-class context without re-running the
// preprocessor against a possibly drifted macro state.  `body_lineno` /
// `body_file_index` / `body_path_index` / `body_is_system_header` tag the replay
// source so tokens keep their original diagnostic locations.  `initializers`
// holds the already-parsed constructor member-initializer list (owned; consumed
// by FinishInlineMemberFunctionBody).
typedef struct {
  Symbol* member_symbol;
  String body_text;
  int body_lineno;
  uint32_t body_file_index;
  size_t body_path_index;
  bool body_is_system_header;
  CXXConstructorInitList initializers;
  ParserContext old_context;
} DeferredInlineBody;

// Parse the statements of an inline member function body and perform the
// post-body bookkeeping (constructor preamble, member/base destructor calls,
// definition queueing).  Assumes a fresh function scope is open, the scope
// symbols (parameters and `this`) have been inserted, and the lexer is
// positioned at the body's opening '{'.  Consumes the body and its optional
// trailing ';', closes the scope and restores `syntax->context` to
// `old_context`.  Takes ownership of `*cxx_initializers`.
static void FinishInlineMemberFunctionBody(
    TypeParser* parser, Symbol* member_symbol,
    CXXConstructorInitList* cxx_initializers, ParserContext old_context) {
  Syntax* syntax = parser->syntax;
  LexMatch(parser->lex, TOK(lbrace));

  TypeRecord* old_current_function = compiler->current_function;
  compiler->current_function = member_symbol->type;
  Vector* body = NewVector();
  bool seen_statement = false;
  while (!LexLookingAt(parser->lex, TOK(rbrace)) && !LexEof(parser->lex)) {
    ASTNode* stmt;
    if (SyntaxLookingAtDeclaration(syntax)) {
      if (seen_statement) {
        SyntaxWarning(syntax, "declaration-after-statement",
                      "declaration after statement");
      }
      stmt = SyntaxParseLocalDeclaration(syntax);
    } else {
      seen_statement = true;
      stmt = SyntaxParseStatement(syntax, TC(semicolon));
    }
    if (stmt != NULL) {
      VectorAppend(body, stmt);
    }
  }

  CXXConstructorInitList* deferred_initializers = NULL;
  if (member_symbol->type->info.function.is_constructor) {
    if (syntax->parsing_template_declaration) {
      deferred_initializers =
          SyntaxCXXConstructorInitListCloneRaw(cxx_initializers);
    } else {
      deferred_initializers = malloc(sizeof(CXXConstructorInitList));
      *deferred_initializers = *cxx_initializers;
    }
  } else {
    SyntaxInsertCXXConstructorPreamble(syntax, member_symbol->type, body,
                                       cxx_initializers,
                                       member_symbol->location);
  }
  AppendCXXMemberDestructorCalls(member_symbol->type, body,
                                 member_symbol->location);
  AppendCXXBaseDestructorCalls(syntax, member_symbol->type, body,
                               member_symbol->location);
  SyntaxCloseScope(syntax);
  syntax->context = old_context;
  compiler->current_function = old_current_function;
  member_symbol->type->info.function.body =
      NewCompoundStatementASTNode(body, parser->lex->current_token_location);
  VectorAppend(&compiler->declaration_asts,
               member_symbol->type->info.function.body);
  SyntaxNeedBracket(syntax, TOK(rbrace), TC(decl));

  if (!syntax->parsing_template_declaration) {
    QueueInlineMemberFunctionDefinition(member_symbol);
  }
  LexMatch(parser->lex, TOK(semicolon));
  if (deferred_initializers != NULL) {
    if (syntax->parsing_template_declaration) {
      QueueTemplateConstructorInitializers(member_symbol, deferred_initializers);
      SyntaxCXXConstructorInitListDestruct(cxx_initializers);
    } else {
      QueueInlineConstructorPreamble(member_symbol, deferred_initializers);
    }
  } else {
    SyntaxCXXConstructorInitListDestruct(cxx_initializers);
  }
}

// Decide whether an inline member function body should be parsed later, once
// the whole class is defined, rather than at the point it textually appears.
// Deferring gives the body a complete-class context so unqualified names bind
// to members declared later in the class (e.g. `front()` calling `begin()`),
// matching [class.mem]/7.  Member templates carry their own template-parameter
// scope that is opened and closed around this call, so their bodies must be
// parsed eagerly while that scope is live.
static bool ShouldDeferInlineMemberBody(TypeParser* parser,
                                        Symbol* member_symbol) {
  if (!CompilerIsCXX()) {
    return false;
  }
  if (parser->deferred_inline_bodies == NULL) {
    return false;
  }
  if (member_symbol == NULL || member_symbol->type == NULL ||
      !TypeIsFunction(member_symbol->type)) {
    return false;
  }
  if (member_symbol->flags.is_template ||
      member_symbol->type->info.function.template_parameter_count > 0) {
    return false;
  }
  return true;
}

static bool ParseInlineMemberFunctionBody(TypeParser* parser,
                                          Symbol* member_symbol) {
  Syntax* syntax = parser->syntax;
  ParserContext old_context = syntax->context;
  syntax->context = kParsingBlockScope;
  SyntaxOpenScope(syntax);
  AddInlineFunctionScopeSymbols(syntax, member_symbol->type);

  CXXConstructorInitList cxx_initializers;
  SyntaxCXXConstructorInitListInit(&cxx_initializers);
  SyntaxParseCXXConstructorInitializerList(syntax, member_symbol->type,
                                           &cxx_initializers);
  if (CompilerCXXAtLeast(kLanguageStandardCXX20) &&
      LexLookingAt(parser->lex, TOK(requires))) {
    ConstraintExpr* constraint = ConceptsParseRequiresClause(syntax);
    if (constraint != NULL && TypeIsFunction(member_symbol->type)) {
      AddOwnedAssociatedConstraint(
          &member_symbol->type->info.function.associated_constraint,
          constraint);
    } else {
      ConstraintExprDelete(constraint);
    }
  }
  if (!LexLookingAt(parser->lex, TOK(lbrace))) {
    SyntaxCloseScope(syntax);
    syntax->context = old_context;
    SyntaxCXXConstructorInitListDestruct(&cxx_initializers);
    return false;
  }

  member_symbol->flags.is_defined = true;
  member_symbol->flags.is_inline_defn = true;
  if (!StorageIs(member_symbol->storage, STO(static))) {
    member_symbol->flags.is_weak = true;
  }
  member_symbol->type->info.function.is_inline = true;
  member_symbol->type->info.function.definition = true;
  member_symbol->type->info.function.is_user_provided = true;
  member_symbol->value.func_defn = member_symbol;

  if (ShouldDeferInlineMemberBody(parser, member_symbol)) {
    // Capture the body's already-expanded text (from the '{') and skip it; it is
    // re-parsed by FlushDeferredInlineMemberBodies once every member is known.
    // The constructor member-initializer list, already parsed above, is carried
    // along so the preamble can be built against the same AST.
    Lex* lex = parser->lex;
    DeferredInlineBody* deferred = malloc(sizeof(DeferredInlineBody));
    deferred->member_symbol = member_symbol;
    deferred->body_lineno = lex->source->lineno;
    deferred->body_file_index = lex->source->file_index;
    deferred->body_path_index = lex->source->path_index;
    deferred->body_is_system_header = lex->source->is_system_header;
    StringInit(&deferred->body_text, NULL);
    LexBeginCapture(lex, &deferred->body_text);
    deferred->initializers = cxx_initializers;  // ownership moved
    deferred->old_context = old_context;
    SkipInlineMemberFunctionBody(parser);
    LexEndCapture(lex);
    LexMatch(lex, TOK(semicolon));
    SyntaxCloseScope(syntax);
    syntax->context = old_context;
    VectorAppend(parser->deferred_inline_bodies, deferred);
    return true;
  }

  FinishInlineMemberFunctionBody(parser, member_symbol, &cxx_initializers,
                                 old_context);
  return true;
}

// Re-parse the inline member function bodies whose parsing was deferred while
// the class body was scanned.  Called once the class's member declarations are
// all registered but before the class scope is torn down, so every member is
// visible to unqualified name lookup inside each body (complete-class context).
// Each body is re-lexed from its captured text; the main lexer is checkpointed
// beforehand and restored to the end-of-members position afterwards.
static void FlushDeferredInlineMemberBodies(TypeParser* parser,
                                            Vector* deferred) {
  if (deferred == NULL || deferred->length == 0) {
    return;
  }
  Syntax* syntax = parser->syntax;
  Lex* lex = parser->lex;
  // Save the lexer position at the end of the class body so parsing can resume
  // there once every deferred body has been replayed.
  LexCheckpoint end_checkpoint;
  LexCheckpointSave(lex, &end_checkpoint);
  for (size_t i = 0; i < deferred->length; i++) {
    DeferredInlineBody* entry = deferred->value.p[i];

    // Re-lex the recorded body from a throwaway string source with the
    // preprocessor suppressed, so the already-expanded text is tokenized exactly
    // as first seen.  The source is tagged with the body's original file index
    // and starting line so tokens carry correct diagnostic locations.
    String* text = NewString(NULL);
    StringSetString(text, &entry->body_text);
    Source* replay = NewSourceFromString("<deferred-inline-body>", text);
    replay->file_index = entry->body_file_index;
    replay->lineno = entry->body_lineno - 1;
    replay->path_index = entry->body_path_index;
    replay->is_system_header = entry->body_is_system_header;

    lex->source = replay;
    lex->suppress_preprocessing = true;
    StringClear(&lex->line);
    lex->pos = 0;
    LexNextToken(lex);  // Prime the first token (the opening '{').

    syntax->context = kParsingBlockScope;
    SyntaxOpenScope(syntax);
    AddInlineFunctionScopeSymbols(syntax, entry->member_symbol->type);
    FinishInlineMemberFunctionBody(parser, entry->member_symbol,
                                   &entry->initializers, entry->old_context);

    lex->suppress_preprocessing = false;
    lex->source = NULL;  // Real source is reinstated by end_checkpoint below.
    SourceDelete(replay);
    StringDestruct(&entry->body_text);
    free(entry);
  }
  LexCheckpointRestore(lex, &end_checkpoint);
  LexCheckpointDestruct(&end_checkpoint);
  VectorClear(deferred);
}

void FinalizePendingInlineConstructorPreambles(TypeParser* parser,
                                                     Struct* owner) {
  if (!pending_inline_constructor_preambles_initialized || owner == NULL) {
    return;
  }
  for (size_t i = 0; i < pending_inline_constructor_preambles.length; i++) {
    PendingInlineConstructorPreamble* pending =
        pending_inline_constructor_preambles.value.p[i];
    if (pending == NULL || pending->symbol == NULL ||
        pending->symbol->type == NULL ||
        pending->symbol->type->info.function.cxx_member_owner != owner) {
      continue;
    }
    TypeRecord* func = pending->symbol->type;
    if (func->info.function.body == NULL ||
        func->info.function.body->op != AST_OP(compound)) {
      continue;
    }
    CompoundStatementASTNode* body =
        (CompoundStatementASTNode*)func->info.function.body;
    SyntaxInsertCXXConstructorPreamble(parser->syntax, func, body->statements,
                                       pending->initializers,
                                       pending->symbol->location);
    SyntaxCXXConstructorInitListDestruct(pending->initializers);
    free(pending->initializers);
    pending->initializers = NULL;
    pending->symbol = NULL;
    free(pending);
    pending_inline_constructor_preambles.value.p[i] = NULL;
  }
}

static bool ParseClassSpecialMember(TypeParser* parser, Struct* str,
                                    String* class_name, CXXAccess access,
                                    bool is_virtual, bool is_constexpr,
                                    bool is_consteval, bool is_explicit,
                                    bool is_member_template,
                                    Vector* member_template_parameters,
                                    ConstraintExpr* member_template_requires_clause,
                                    int member_template_parameter_base) {
  if (!CompilerIsCXX() || class_name->length == 0) {
    return false;
  }

  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  bool is_destructor = LexMatch(parser->lex, TOK(tilde));
  if (!LexLookingAt(parser->lex, TOK(identifier)) ||
      !CXXClassNameMatchesUnqualifiedTemplateName(class_name,
                                                  &parser->lex->spelling)) {
    if (is_destructor) {
      SyntaxError(parser->syntax, "Expected class name after '~'");
      LexCheckpointDestruct(&checkpoint);
      return true;
    }
    LexCheckpointRestore(parser->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return false;
  }

  SourceLocation location = parser->lex->current_token_location;
  LexNextToken(parser->lex);
  if (!LexMatch(parser->lex, TOK(lparen))) {
    if (is_destructor) {
      SyntaxError(parser->syntax, "Expected '(' in destructor declaration");
      LexCheckpointDestruct(&checkpoint);
      return true;
    }
    LexCheckpointRestore(parser->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return false;
  }
  LexCheckpointDestruct(&checkpoint);

  TypeParser proto_parser;
  TypeParserInit(&proto_parser, parser->lex, parser->syntax, STO(auto),
                 kParsingPrototype);
  proto_parser.cxx_member_owner = str;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constexpr = is_constexpr;
  func->info.function.is_consteval = is_consteval;
  // A constructor/destructor declaration inside the class is not itself an
  // inline definition.  FinishInlineMemberFunctionBody marks it inline when a
  // body is actually present; constexpr/consteval declarations are implicitly
  // inline even when their definition appears elsewhere.
  func->info.function.is_inline = is_constexpr || is_consteval;
  func->info.function.is_constructor = !is_destructor;
  func->info.function.is_destructor = is_destructor;
  // Only a constructor may be declared explicit; a destructor never converts.
  func->info.function.is_explicit = is_explicit && !is_destructor;
  if (!is_destructor && parser->syntax->pending_explicit_condition != NULL) {
    func->info.function.explicit_condition =
        parser->syntax->pending_explicit_condition;
    parser->syntax->pending_explicit_condition = NULL;
  }
  if (is_virtual && !is_destructor) {
    SyntaxError(parser->syntax, "Constructors cannot be virtual");
  }
  func->info.function.is_virtual = is_virtual && is_destructor;
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  ParseFunctionPrototype(&proto_parser, func);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
  if (LexMatch(parser->lex, TOK(const))) {
    SyntaxError(parser->syntax, "Constructors and destructors cannot be const");
  }
  ParseCXXExceptionSpecifier(parser, func);
  ParseCXXVirtSpecifiers(parser, func);
  ParseCXXSpecialMemberTrailingRequires(parser, func);
  ParseCXXPureSpecifier(parser, func);
  TypeParserDestruct(&proto_parser);
  TypeRecordAddCXXThisParameter(func, str, location);

  String member_name;
  StringInit(&member_name, is_destructor ? "~" : "");
  StringAppendString(&member_name, class_name);
  Symbol* member_symbol = NewSymbol(member_name.value, func, STO(implicit));
  member_symbol->location = location;
  func->info.function.symbol = member_symbol;
  if (is_member_template) {
    if (is_destructor) {
      // A destructor is never a template ([class.dtor]); a template following
      // `~Class` is ill-formed.
      SyntaxError(parser->syntax, "A destructor cannot be a template");
    } else {
      // Record the constructor as a member function template so overload
      // resolution deduces its parameters at the call site, exactly like an
      // ordinary member function template (e.g. `emplace`).
      member_symbol->flags.is_template = true;
      func->info.function.template_parameter_count =
          (int)member_template_parameters->length;
      func->info.function.template_parameter_base =
          member_template_parameter_base;
      VectorDestructWithContents(
          &func->info.function.template_parameters,
          (VectorElementDestructor)TemplateParameterDelete,
          /*free_element=*/false);
      VectorInit(&func->info.function.template_parameters);
      for (size_t i = 0; i < member_template_parameters->length; i++) {
        VectorAppend(&func->info.function.template_parameters,
                     member_template_parameters->value.p[i]);
      }
      // Ownership of the parameter entries has moved into the function type;
      // clear the source so the caller's cleanup frees only the vector.
      member_template_parameters->length = 0;
    }
  }
  FinalizeMemberFunctionTemplateConstraints(
      func, member_template_parameters, member_template_requires_clause);
  CXXFinalizeSpecialMemberMetadata(member_symbol, str, true);
  StructMember* member = NewStructMember(member_symbol);
  member->is_member_function = true;
  member->access = access;
  StructMember* existing = MapFindPointerKey(&str->symbol_table, &member_name);
  if (existing != NULL) {
    if (!CanOverloadStructMember(existing, member)) {
      String diagnostic_name;
      StringInit(&diagnostic_name, NULL);
      SymbolFunctionDiagnosticName(member_symbol, &diagnostic_name);
      SyntaxError(parser->syntax, "Duplicate class member %s",
                  diagnostic_name.value);
      StringDestruct(&diagnostic_name);
      StructMemberDelete(member);
      StringDestruct(&member_name);
      SkipInlineMemberFunctionBody(parser);
      return true;
    }
    if (FindStructMemberOverload(existing, member_symbol->type) != NULL) {
      String diagnostic_name;
      StringInit(&diagnostic_name, NULL);
      SymbolFunctionDiagnosticName(member_symbol, &diagnostic_name);
      SyntaxError(parser->syntax, "Duplicate class member %s",
                  diagnostic_name.value);
      StringDestruct(&diagnostic_name);
      StructMemberDelete(member);
      StringDestruct(&member_name);
      SkipInlineMemberFunctionBody(parser);
      return true;
    }
    AppendStructMemberOverload(parser, str, existing, member);
  } else {
    AddStructMember(parser, str, member);
  }
  StringDestruct(&member_name);
  if (func->info.function.is_defaulted) {
    SynthesizeDefaultedMemberFunctionBody(parser, member_symbol);
  } else if (!func->info.function.is_deleted) {
    ParseInlineMemberFunctionBody(parser, member_symbol);
  }
  return true;
}

static bool ParseCXXConversionOperatorMember(
    TypeParser* parser, Struct* str, CXXAccess access, bool is_virtual,
    bool is_explicit, bool is_member_template,
    Vector* member_template_parameters,
    ConstraintExpr* member_template_requires_clause,
    int member_template_parameter_base) {
  if (!CompilerIsCXX() || !LexLookingAt(parser->lex, TOK(operator))) {
    return false;
  }

  SourceLocation location = parser->lex->current_token_location;
  LexNextToken(parser->lex);

  TypeRecord* return_type = ParseCXXConversionType(parser);
  if (!LexLookingAt(parser->lex, TOK(lparen))) {
    SyntaxError(parser->syntax,
                "Unsupported conversion operator target type");
    SyntaxRecover(parser->syntax, TC(semicolon) | TC(openbra) | TC(closebra));
    // This early exit precedes the template-parameter storage below, so the
    // member's requires-clause would otherwise never be consumed.  The caller
    // frees the parameter vector; free the requires-clause here to match.
    if (is_member_template) {
      ConstraintExprDelete(member_template_requires_clause);
    }
    return true;
  }
  SyntaxNeedBracket(parser->syntax, TOK(lparen), TC(decl));
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(decl));

  String member_name;
  ConversionOperatorName(return_type, &member_name);
  Symbol* member_symbol =
      NewCXXConversionOperatorSymbol(parser, str, return_type, location,
                                     is_virtual);
  TypeRecord* func = member_symbol->type;
  func->info.function.is_explicit = is_explicit;
  func->info.function.is_explicit_conversion = is_explicit;
  if (parser->syntax->pending_explicit_condition != NULL) {
    func->info.function.explicit_condition =
        parser->syntax->pending_explicit_condition;
    parser->syntax->pending_explicit_condition = NULL;
  }
  if (is_member_template) {
    // Store the template parameters on the function type, exactly like an
    // ordinary member function template.  Deduction of the parameters happens
    // at the conversion site by matching the (dependent) target type against
    // the requested type ([temp.deduct.conv]).
    member_symbol->flags.is_template = true;
    func->info.function.template_parameter_count =
        (int)member_template_parameters->length;
    func->info.function.template_parameter_base =
        member_template_parameter_base;
    VectorDestructWithContents(
        &func->info.function.template_parameters,
        (VectorElementDestructor)TemplateParameterDelete,
        /*free_element=*/false);
    VectorInit(&func->info.function.template_parameters);
    for (size_t i = 0; i < member_template_parameters->length; i++) {
      VectorAppend(&func->info.function.template_parameters,
                   member_template_parameters->value.p[i]);
    }
    // Ownership of the parameter entries has moved into the function type.
    member_template_parameters->length = 0;
    FinalizeMemberFunctionTemplateConstraints(
        func, member_template_parameters, member_template_requires_clause);
  }
  StructMember* member = NewStructMember(member_symbol);
  member->is_member_function = true;
  member->access = access;

  StructMember* existing = MapFindPointerKey(&str->symbol_table, &member_name);
  if (existing != NULL) {
    if (!CanOverloadStructMember(existing, member)) {
      SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                  member_name.value);
      StructMemberDelete(member);
      SkipInlineMemberFunctionBody(parser);
      StringDestruct(&member_name);
      return true;
    }
    if (FindStructMemberOverload(existing, member_symbol->type) != NULL) {
      SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                  member_name.value);
      StructMemberDelete(member);
      SkipInlineMemberFunctionBody(parser);
      StringDestruct(&member_name);
      return true;
    }
    AppendStructMemberOverload(parser, str, existing, member);
  } else {
    AddStructMember(parser, str, member);
  }
  ParseCXXVirtSpecifiers(parser, func);
  ParseCXXPureSpecifier(parser, func);
  ParseInlineMemberFunctionBody(parser, member_symbol);
  StringDestruct(&member_name);
  return true;
}

static void CopyAnonymousMembers(TypeParser* parser, Struct* dest, Struct* src ) {
  for (size_t i = 0; i < src->members.length; i++) {
    StructMember* member = src->members.value.p[i];
    Symbol* symbol = member->symbol;
    if (member->is_anon) {
      // Anonymous member, deal with recursively.
      CopyAnonymousMembers(parser, dest, symbol->type->info.struct_info);
      continue;
    }
    if (!CheckStructMember(dest, &symbol->name)) {
      SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                symbol->name.value);
    } else {
      // Insert a *copy* of the member into the destination's symbol table with
      // the byte offset adjusted to be relative to the destination struct.  We
      // must not mutate the original member's byte_offset: it is shared with the
      // anonymous aggregate's own members vector, where the offset must stay
      // relative to that aggregate (otherwise designated/positional static
      // initializers and member access through the aggregate compute the wrong
      // offset).
      if (!src->is_union) {
        AlignNextOffset(dest, symbol->type);
      }
      StructMember* dest_member = NewStructMember(symbol);
      dest_member->bit_offset = member->bit_offset;
      dest_member->bit_size = member->bit_size;
      dest_member->index = member->index;
      dest_member->is_anon = member->is_anon;
      dest_member->byte_offset = dest->next_offset;

      StructInsertMemberIntoTables(dest, dest_member);

      if (!src->is_union) {
        // Members of an anonymous struct are laid out sequentially regardless
        // of whether the enclosing aggregate is a union, so always advance.
        UpdateStructSize(dest, symbol->type, false);
      }
    }
  }
}

static ASTNode* ParseCXXStaticDataMemberInitializer(TypeParser* parser,
                                                    Symbol* symbol) {
  if (symbol == NULL) {
    return NULL;
  }
  if (LexMatch(parser->lex, TOK(equal))) {
    parser->syntax->init_storage = symbol->storage;
    return SyntaxParseInitializer(parser->syntax, symbol, symbol->storage);
  }
  if (LexMatch(parser->lex, TOK(lbrace))) {
    return SyntaxParseBracedInitializer(parser->syntax);
  }
  return NULL;
}

static bool CXXStaticDataMemberAllowsInClassInitializer(Symbol* symbol) {
  return symbol != NULL && TypeIsConst(symbol->type) &&
         (TypeIsIntegral(symbol->type) || TypeIsEnum(symbol->type));
}

static void AnalyzeCXXStaticDataMemberConstantInitializer(TypeParser* parser,
                                                          Symbol* symbol,
                                                          ASTNode* initializer) {
  ASTNode* decl = NewVariableDeclarationASTNode(
      symbol, initializer, symbol->location);
  SemanticAnalyzeVariableDefinition(parser->syntax,
                                    (VariableDeclarationASTNode*)decl);
  if (!symbol->flags.value_set) {
    SyntaxError(parser->syntax,
                "Static data member initializer must be a constant expression");
  }
  ASTNodeDelete(decl);
}

static void QueueCXXInlineStaticDataMemberDefinition(TypeParser* parser,
                                                     Struct* owner,
                                                     StructMember* member,
                                                     ASTNode* initializer,
                                                     bool is_inline_member) {
  if (!CompilerIsCXX() || parser == NULL || parser->syntax == NULL ||
      owner == NULL || member == NULL || member->symbol == NULL ||
      !member->is_static || member->is_member_function ||
      parser->syntax->parsing_template_declaration) {
    return;
  }

  Symbol* symbol = member->symbol;
  bool implicit_inline = symbol->flags.is_constexpr;
  if (!is_inline_member && !implicit_inline) {
    if (initializer != NULL) {
      if (CXXStaticDataMemberAllowsInClassInitializer(symbol)) {
        AnalyzeCXXStaticDataMemberConstantInitializer(parser, symbol,
                                                     initializer);
        return;
      }
      SyntaxError(parser->syntax,
                  "Static data member initializer requires inline");
    }
    return;
  }

  TypeRecordCalculateSize(symbol->type);
  symbol->flags.is_defined = true;
  if (!StorageIs(symbol->storage, STO(static))) {
    symbol->flags.is_weak = true;
    SymbolSetCXXDataAsmName(symbol, owner);
  }
  if (symbol->flags.is_constexpr && initializer == NULL) {
    SyntaxError(parser->syntax,
                "constexpr variable requires an initializer");
  }

  ASTNode* decl = NewVariableDeclarationASTNode(
      symbol, initializer, symbol->location);
  bool const_eval_candidate =
      symbol->flags.is_constexpr || symbol->flags.is_constinit ||
      (TypeIsConst(symbol->type) && !TypeIsStructOrUnion(symbol->type));
  // A static data member whose own type is the enclosing class cannot be
  // evaluated here: the class is still incomplete (we are mid-definition) and
  // its constructors' inline bodies have not yet been semantically analyzed, so
  // constant-evaluating the initializer would fail.  Defer such a member to a
  // separate queue that is evaluated and code generated only once the whole
  // class -- including its inline member-function bodies -- is complete (see
  // CompileDeferredCXXStaticMembers).  Its value is a class object, so it is
  // never needed as a constant within the class body itself.  Keeping it out of
  // inline_static_member_definitions also prevents it from being code generated
  // (as a still-unevaluated dynamic initializer) before that point.
  if (const_eval_candidate && TypeIsStructOrUnion(symbol->type) &&
      symbol->type->info.struct_info == owner) {
    VectorAppend(&compiler->cxx_deferred_static_member_definitions, decl);
    return;
  }
  VectorAppend(&parser->syntax->inline_static_member_definitions, decl);
  if (!const_eval_candidate) {
    return;
  }
  // The initializer is in the scope of `owner` and may use its private
  // members (e.g. a private constructor of a comparison category).
  Struct* saved_access = compiler->current_class_access_context;
  compiler->current_class_access_context = owner;
  SemanticAnalyzeVariableDefinition(parser->syntax,
                                    (VariableDeclarationASTNode*)decl);
  compiler->current_class_access_context = saved_access;
}

void ParseStructMembers(TypeParser* parser, Struct* str, bool is_union,
                               String* tag_name) {
  CXXAccess current_access = str->is_class ? kAccessPrivate : kAccessPublic;
  // Inline member function bodies are parsed in complete-class context: their
  // parse is deferred until every member of this class is declared.  Publish a
  // collection frame for this class body; nested classes install their own.
  Vector deferred_inline_bodies;
  VectorInit(&deferred_inline_bodies);
  Vector* saved_deferred_inline_bodies = parser->deferred_inline_bodies;
  parser->deferred_inline_bodies = &deferred_inline_bodies;
  while (!LexLookingAt(parser->lex, TOK(rbrace))) {
    if (CompilerIsCXX() && LexLookingAt(parser->lex, TOK(static_assert))) {
      ASTNode* node = SyntaxParseStaticAssert(parser->syntax);
      ASTNodeDelete(node);
      continue;
    } else if (LexLookingAt(parser->lex, TOK(public))) {
      current_access = kAccessPublic;
      LexNextToken(parser->lex);
      SyntaxNeedBracket(parser->syntax, TOK(colon), TC(decl));
      continue;
    } else if (LexLookingAt(parser->lex, TOK(private))) {
      current_access = kAccessPrivate;
      LexNextToken(parser->lex);
      SyntaxNeedBracket(parser->syntax, TOK(colon), TC(decl));
      continue;
    } else if (LexLookingAt(parser->lex, TOK(protected))) {
      current_access = kAccessProtected;
      LexNextToken(parser->lex);
      SyntaxNeedBracket(parser->syntax, TOK(colon), TC(decl));
      continue;
    } else if (CompilerIsCXX() && LexLookingAt(parser->lex, TOK(friend))) {
      // Friend declarations grant access but do not introduce a member; they
      // are parsed in the enclosing namespace scope by the syntax layer.
      SyntaxParseFriendDeclaration(parser->syntax, str);
      continue;
    }

    Vector member_attributes = {0};
    VectorInit(&member_attributes);
    while (SyntaxParseCXXAlignas(parser->syntax, &member_attributes) ||
           SyntaxParseCXXAttributes(parser->syntax, &member_attributes)) {
    }
    parser->cxx_member_owner = str;

    if (CompilerIsCXX() && LexMatch(parser->lex, TOK(using))) {
      ParseCXXMemberUsingDeclaration(parser, str, current_access,
                                     parser->lex->current_token_location);
      AttributeListDestruct(&member_attributes);
      if (!LexLookingAt(parser->lex, TOK(rbrace))) {
        SyntaxNeedSemicolon(parser->syntax, TC(type));
      }
      continue;
    }

    if (CompilerIsCXX() && LexMatch(parser->lex, TOK(typedef))) {
      ParseCXXMemberTypedef(parser, str, current_access,
                            parser->lex->current_token_location);
      AttributeListDestruct(&member_attributes);
      if (!LexLookingAt(parser->lex, TOK(rbrace))) {
        SyntaxNeedSemicolon(parser->syntax, TC(type));
      }
      continue;
    }

    bool is_member_template = false;
    bool old_parsing_template = parser->syntax->parsing_template_declaration;
    int old_template_parameter_count =
        parser->syntax->current_template_parameter_count;
    Vector* old_template_parameters = parser->syntax->current_template_parameters;
    ConstraintExpr* old_requires_clause =
        parser->syntax->current_template_requires_clause;
    Vector* member_template_parameters = NULL;
    ConstraintExpr* member_template_requires_clause = NULL;
    int member_template_parameter_base = old_template_parameter_count;
    if (str->template_parameter_count > member_template_parameter_base) {
      member_template_parameter_base = str->template_parameter_count;
    }
    if (CompilerIsCXX() && LexMatch(parser->lex, TOK(template))) {
      SyntaxOpenScope(parser->syntax);
      member_template_parameters =
          SyntaxParseTemplateParameterListWithBase(
              parser->syntax, member_template_parameter_base);
      parser->syntax->parsing_template_declaration = true;
      parser->syntax->current_template_parameters = member_template_parameters;
      parser->syntax->current_template_parameter_count =
          member_template_parameter_base +
          (int)member_template_parameters->length;
      member_template_requires_clause = ConceptsParseRequiresClause(parser->syntax);
      is_member_template = true;
    }

    if (is_member_template && CompilerIsCXX() &&
        LexLookingAt(parser->lex, TOK(friend))) {
      SyntaxParseFriendDeclaration(parser->syntax, str);
      AttributeListDestruct(&member_attributes);
      SyntaxCloseScope(parser->syntax);
      VectorDestructWithContents(member_template_parameters,
                                 (VectorElementDestructor)TemplateParameterDelete,
                                 /*free_element=*/false);
      parser->syntax->parsing_template_declaration = old_parsing_template;
      parser->syntax->current_template_parameter_count =
          old_template_parameter_count;
      parser->syntax->current_template_parameters = old_template_parameters;
      ConstraintExprDelete(member_template_requires_clause);
      parser->syntax->current_template_requires_clause = old_requires_clause;
      member_template_requires_clause = NULL;
      continue;
    }

    if (is_member_template && CompilerIsCXX() &&
        LexMatch(parser->lex, TOK(using))) {
      if (CXXMemberUsingLooksLikeAlias(parser)) {
        ParseCXXMemberUsingAlias(parser, str, current_access,
                                 parser->lex->current_token_location,
                                 /*is_template_alias=*/true);
      } else {
        SyntaxError(parser->syntax,
                    "Member using declaration cannot be a template");
        SyntaxRecover(parser->syntax, TC(semicolon));
      }
      AttributeListDestruct(&member_attributes);
      if (!LexLookingAt(parser->lex, TOK(rbrace))) {
        SyntaxNeedSemicolon(parser->syntax, TC(type));
      }
      SyntaxCloseScope(parser->syntax);
      VectorDestructWithContents(member_template_parameters,
                                 (VectorElementDestructor)TemplateParameterDelete,
                                 /*free_element=*/false);
      parser->syntax->parsing_template_declaration = old_parsing_template;
      parser->syntax->current_template_parameter_count =
          old_template_parameter_count;
      parser->syntax->current_template_parameters = old_template_parameters;
      ConstraintExprDelete(member_template_requires_clause);
      parser->syntax->current_template_requires_clause = old_requires_clause;
      member_template_requires_clause = NULL;
      continue;
    }

    bool is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
    bool is_constexpr_member = false;
    bool is_consteval_member = false;
    if (CompilerIsCXX() && LexMatch(parser->lex, TOK(consteval))) {
      is_consteval_member = true;
      is_constexpr_member = true;
    } else {
      is_constexpr_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(constexpr));
    }
    parser->is_inline = is_inline_member;
    parser->is_constexpr = is_constexpr_member;
    parser->is_consteval = is_consteval_member;
    bool saw_explicit_member = false;
    bool is_explicit_member =
        ParseCXXExplicitSpecifier(parser, &saw_explicit_member);
    if (!is_inline_member) {
      is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
      parser->is_inline = is_inline_member;
    }
    if (!is_constexpr_member && !is_consteval_member) {
      if (CompilerIsCXX() && LexMatch(parser->lex, TOK(consteval))) {
        is_consteval_member = true;
        is_constexpr_member = true;
      } else {
        is_constexpr_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(constexpr));
      }
      parser->is_constexpr = is_constexpr_member;
      parser->is_consteval = is_consteval_member;
    }
    bool is_virtual_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(virtual));
    if (!is_inline_member) {
      is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
      parser->is_inline = is_inline_member;
    }
    if (ParseClassSpecialMember(parser, str, tag_name, current_access,
                                is_virtual_member, is_constexpr_member,
                                is_consteval_member, is_explicit_member,
                                is_member_template, member_template_parameters,
                                member_template_requires_clause,
                                member_template_parameter_base)) {
      AttributeListDestruct(&member_attributes);
      if (is_member_template) {
        // A constructor template consumes the parameter entries (moving them
        // into the function type) and leaves the vector empty; a destructor
        // template is diagnosed inside ParseClassSpecialMember with its
        // entries still present.  Either way the scope, parsing flags, and the
        // (now possibly empty) parameter vector are torn down here.
        SyntaxCloseScope(parser->syntax);
        VectorDestructWithContents(member_template_parameters,
                                   (VectorElementDestructor)TemplateParameterDelete,
                                   /*free_element=*/false);
        parser->syntax->parsing_template_declaration = old_parsing_template;
        parser->syntax->current_template_parameter_count =
            old_template_parameter_count;
        parser->syntax->current_template_parameters = old_template_parameters;
        parser->syntax->current_template_requires_clause = old_requires_clause;
        member_template_requires_clause = NULL;
      }
      if (!LexLookingAt(parser->lex, TOK(rbrace))) {
        LexMatch(parser->lex, TOK(semicolon));
      }
      continue;
    }

    bool is_thread_member = false;
    bool is_static_member = false;
    while (true) {
      if (!is_thread_member &&
          (LexLookingAt(parser->lex, TOK(thread)) ||
           LexLookingAt(parser->lex, TOK(thread_local)))) {
        is_thread_member = true;
        LexNextToken(parser->lex);
      } else if (!is_static_member && LexLookingAt(parser->lex, TOK(static))) {
        is_static_member = LexMatch(parser->lex, TOK(static));
      } else {
        break;
      }
    }
    // 'mutable' is a storage-class specifier on a data member.  Accept it in
    // either order with respect to 'static' so the (ill-formed) combination is
    // still reported by the conflict check rather than as a parse error.
    bool is_mutable_member =
        CompilerIsCXX() && LexMatch(parser->lex, TOK(mutable));
    if (is_mutable_member && !is_static_member) {
      is_static_member = LexMatch(parser->lex, TOK(static));
    }
    if (!is_inline_member) {
      is_inline_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(inline));
      parser->is_inline = is_inline_member;
    }
    if (!is_constexpr_member && !is_consteval_member) {
      if (CompilerIsCXX() && LexMatch(parser->lex, TOK(consteval))) {
        is_consteval_member = true;
        is_constexpr_member = true;
      } else {
        is_constexpr_member = CompilerIsCXX() && LexMatch(parser->lex, TOK(constexpr));
      }
      parser->is_constexpr = is_constexpr_member;
      parser->is_consteval = is_consteval_member;
    }
    if (is_virtual_member && is_static_member) {
      SyntaxError(parser->syntax, "static member functions cannot be virtual");
    }
    if (!is_static_member &&
        ParseCXXConversionOperatorMember(parser, str, current_access,
                                         is_virtual_member, is_explicit_member,
                                         is_member_template,
                                         member_template_parameters,
                                         member_template_requires_clause,
                                         member_template_parameter_base)) {
      AttributeListDestruct(&member_attributes);
      if (is_member_template) {
        // The conversion operator template consumed the parameter entries
        // (moving them into the function type) and the requires-clause; tear
        // down the template scope and parsing flags, freeing only the (now
        // empty) parameter vector container.
        SyntaxCloseScope(parser->syntax);
        VectorDestructWithContents(
            member_template_parameters,
            (VectorElementDestructor)TemplateParameterDelete,
            /*free_element=*/false);
        parser->syntax->parsing_template_declaration = old_parsing_template;
        parser->syntax->current_template_parameter_count =
            old_template_parameter_count;
        parser->syntax->current_template_parameters = old_template_parameters;
        parser->syntax->current_template_requires_clause = old_requires_clause;
        member_template_requires_clause = NULL;
      }
      if (!LexLookingAt(parser->lex, TOK(rbrace))) {
        LexMatch(parser->lex, TOK(semicolon));
      }
      continue;
    }
    if (saw_explicit_member) {
      SyntaxError(parser->syntax,
                  "explicit is only supported on conversion operators");
    }
    bool possible_anon = LexLookingAt(parser->lex, TOK(union)) ||
            LexLookingAt(parser->lex, TOK(struct)) ||
            LexLookingAt(parser->lex, TOK(class)) ||
            LexLookingAt(parser->lex, TOK(enum));
    bool saved_direct_class_template = parser->parsing_direct_class_template;
    parser->parsing_direct_class_template =
        is_member_template && possible_anon &&
        !LexLookingAt(parser->lex, TOK(enum));
    TypeRecord* member_type = TypeParserParseType(parser, true);
    parser->parsing_direct_class_template = saved_direct_class_template;
    bool member_decl_had_inline_body = false;
    while (!LexEof(parser->lex)) {
      bool has_inline_body = false;
      if (possible_anon && LexLookingAt(parser->lex, TOK(semicolon))) {
        if (TypeIsNamedCXXNestedType(member_type)) {
          if (is_member_template) {
            MarkCXXNestedClassTemplate(member_type, member_template_parameters);
          }
          AddCXXNestedTypeMember(parser, str, member_type, current_access);
          if (!TypeIsEnum(member_type)) {
            break;
          }
        }
        if (TypeIsEnum(member_type)) {
          AddCXXUnscopedEnumConstantMembers(parser, str, member_type,
                                            current_access);
          break;
        }
        TypeRecordCalculateSize(member_type);
        AlignNextOffset(str, member_type);
        int anon_base = str->next_offset;
        CopyAnonymousMembers(parser, str, member_type->info.struct_info);

        // Make a fake member symbol to represent the anonymous member.  This
        // is inserted into the members vector but not the symbol table.
        Symbol* member_symbol = NewSymbol(SyntaxFakeName(parser->syntax),
                                          member_type, STO(implicit));
        StructMember* member = NewStructMember(member_symbol);
        member->byte_offset = anon_base;
        member->access = current_access;
        VectorAppend(&str->members, member);

        // Account for the space occupied by the anonymous aggregate.  In a
        // struct we advance past it; in a union it overlays the other members
        // at the same base offset, so we only grow the union's size and leave
        // next_offset where it was.
        if (is_union) {
          if (anon_base + member_type->size > str->size) {
            str->size = anon_base + member_type->size;
          }
          str->next_offset = anon_base;
        } else {
          str->next_offset = anon_base + member_type->size;
          str->size = str->next_offset;
        }
        member->is_anon = true;
        // Syntax doesn't allow anonymous members to be in a comma-separated
        // list.
        break;
      }
      Symbol* member_symbol = TypeParserParseDeclarator(parser, member_type);
      if (member_symbol == NULL) {
        SyntaxError(parser->syntax, "Invalid type for struct member");
      } else {
        while (SyntaxParseCXXAlignas(parser->syntax, &member_attributes) ||
               SyntaxParseCXXAttributes(parser->syntax, &member_attributes)) {
        }
        VectorCopy(&member_symbol->attributes, &member_attributes);
        VectorClear(&member_attributes);
        SyntaxApplyDeclarationAttributes(member_symbol);
        StructMember* member = NewStructMember(member_symbol);
        member_symbol->flags.is_constexpr = is_constexpr_member &&
                                            !TypeIsFunction(member_symbol->type);
        if (member_symbol->flags.is_constexpr) {
          member_symbol->type->qualifiers |= kQualConst;
        }
        member->is_static = is_static_member;
        member->is_member_function = TypeIsFunction(member_symbol->type);
        if (is_thread_member) {
          member_symbol->storage |= STO(thread);
        }
        SyntaxCheckThreadLocal(parser->syntax, member_symbol, kParsingFileScope,
                               member->is_static, !member->is_static &&
                                   !member->is_member_function);
        if (member->is_member_function &&
            SymbolIsCXXAllocationFunction(member_symbol)) {
          member->is_static = true;
        }
        member->is_mutable = is_mutable_member;
        if (is_mutable_member) {
          if (member->is_static) {
            SyntaxError(parser->syntax,
                        "member cannot be declared both 'mutable' and 'static'");
            member->is_mutable = false;
          } else if (member->is_member_function) {
            SyntaxError(parser->syntax,
                        "'mutable' can only be applied to data members");
            member->is_mutable = false;
          } else if (TypeIsConst(member_symbol->type)) {
            SyntaxError(parser->syntax,
                        "'mutable' cannot be applied to a const member");
            member->is_mutable = false;
          } else if (TypeIsReference(member_symbol->type)) {
            SyntaxError(parser->syntax,
                        "'mutable' cannot be applied to a reference member");
            member->is_mutable = false;
          }
        }
        member->access = current_access;
        if (member->is_member_function) {
          member_symbol->type->info.function.is_constexpr =
              is_constexpr_member;
          member_symbol->type->info.function.is_consteval =
              is_consteval_member;
          if (is_constexpr_member || is_consteval_member) {
            member_symbol->type->info.function.is_inline = true;
          }
        }
        if (member->is_member_function && !member->is_static) {
          member_symbol->type->info.function.is_virtual = is_virtual_member;
          TypeRecordAddCXXThisParameter(member_symbol->type, str,
                                        member_symbol->location);
        } else if (member->is_member_function) {
          member_symbol->type->info.function.cxx_member_owner = str;
        }
        if (is_member_template) {
          if (member->is_member_function) {
            member_symbol->flags.is_template = true;
            member_symbol->type->info.function.template_parameter_count =
                (int)member_template_parameters->length;
            member_symbol->type->info.function.template_parameter_base =
                member_template_parameter_base;
            VectorDestructWithContents(
                &member_symbol->type->info.function.template_parameters,
                (VectorElementDestructor)TemplateParameterDelete,
                /*free_element=*/false);
            VectorInit(&member_symbol->type->info.function.template_parameters);
            for (size_t i = 0; i < member_template_parameters->length; i++) {
              VectorAppend(
                  &member_symbol->type->info.function.template_parameters,
                  member_template_parameters->value.p[i]);
            }
            member_template_parameters->length = 0;
            FinalizeMemberFunctionTemplateConstraints(
                member_symbol->type, member_template_parameters,
                member_template_requires_clause);
            member_template_requires_clause = NULL;
          } else if (TypeIsStructOrUnion(member_symbol->type) &&
                     member_symbol->type->info.struct_info != NULL) {
            member_symbol->flags.is_template = true;
            Struct* nested = member_symbol->type->info.struct_info;
            nested->is_template = true;
            nested->template_parameter_count =
                (int)member_template_parameters->length;
            VectorDestructWithContents(
                &nested->template_parameters,
                (VectorElementDestructor)TemplateParameterDelete,
                /*free_element=*/false);
            VectorInit(&nested->template_parameters);
            for (size_t i = 0; i < member_template_parameters->length; i++) {
              VectorAppend(&nested->template_parameters,
                           member_template_parameters->value.p[i]);
            }
            member_template_parameters->length = 0;
          } else {
            SyntaxError(parser->syntax,
                        "Member templates must be functions, classes, or aliases");
          }
        }
        if (member->is_member_function) {
          ParseCXXVirtSpecifiers(parser, member_symbol->type);
          ParseCXXPureSpecifier(parser, member_symbol->type);
          CXXFinalizeSpecialMemberMetadata(member_symbol, str, true);
          if (member->is_static &&
              (member_symbol->type->info.function.is_override ||
               member_symbol->type->info.function.is_final ||
               member_symbol->type->info.function.is_pure_virtual)) {
            SyntaxError(parser->syntax,
                        "static member functions cannot use virtual specifiers");
          }
        }
        StructMember* existing =
            MapFindPointerKey(&str->symbol_table, &member_symbol->name);
        if (existing != NULL) {
          if (!CanOverloadStructMember(existing, member)) {
            SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                        member_symbol->name.value);
            StructMemberDelete(member);
            member = NULL;
          } else if (FindConstrainedMemberOverload(existing, member_symbol) !=
                     NULL) {
            SyntaxError(parser->syntax, "Duplicate struct/union member %s",
                        member_symbol->name.value);
            StructMemberDelete(member);
            member = NULL;
          } else {
            AppendStructMemberOverload(parser, str, existing, member);
          }
        } else {
          // Add to symbol table.
          AddStructMember(parser, str, member);
        }

        // Make a static member function visible by unqualified name within the
        // class body so a sibling member (e.g. a static method calling another
        // static method) can reference it without the Class:: qualifier, as
        // C++ class-scope lookup requires.  Only the first overload needs to be
        // injected: overload resolution recovers the full set from the owning
        // struct via the function's cxx_member_owner.  Non-static members are
        // intentionally excluded so unqualified uses still route through the
        // implicit `this->` member access.
        if (member != NULL && member->is_static && member->is_member_function &&
            member->symbol != NULL) {
          Symbol* scope_fn = SymbolClone(member->symbol);
          scope_fn->overload_next = NULL;
          if (!SyntaxAddSymbol(parser->syntax, scope_fn)) {
            SymbolDelete(scope_fn);
          }
        }

        // Check for bitfield.
        if (member == NULL) {
          // Already diagnosed as a duplicate.
        } else if (LexMatch(parser->lex, TOK(colon))) {
          if (member->is_static || member->is_member_function) {
            SyntaxError(parser->syntax,
                        "Only non-static data members can be bitfields");
          }
          ParseBitField(parser, is_union, str, member_symbol, member);
          if (!member->is_static && !member->is_member_function) {
            member->default_initializer =
                SyntaxParseCXXDefaultMemberInitializer(parser->syntax);
          }
        } else if (member->is_static || member->is_member_function) {
          if (member->is_member_function &&
              member_symbol->type->info.function.is_defaulted) {
            SynthesizeDefaultedMemberFunctionBody(parser, member_symbol);
          } else if (member->is_member_function &&
                     member_symbol->type->info.function.is_deleted) {
            has_inline_body = false;
          } else if (member->is_member_function) {
            has_inline_body = member->is_member_function &&
                              ParseInlineMemberFunctionBody(parser,
                                                            member_symbol);
          } else {
            ASTNode* initializer =
                ParseCXXStaticDataMemberInitializer(parser, member_symbol);
            if (initializer != NULL) {
              member->default_initializer =
                  CloneCXXDefaultMemberInitializer(initializer);
            }
            // A const integral/enum static data member with an in-class
            // initializer yields a constant usable by unqualified name in the
            // rest of the class body (array bounds, default arguments, etc.).
            // Inside a template the regular queue defers this, but such
            // initializers are typically non-dependent, so evaluate them now to
            // make the constant available.  Dependent initializers must wait
            // until template substitution; evaluating them against the primary
            // template would reject valid constexpr expressions such as
            // `static constexpr intmax_t n = f(R::num)`.
            if (parser->syntax->parsing_template_declaration &&
                !is_inline_member && initializer != NULL &&
                CXXStaticDataMemberAllowsInClassInitializer(member_symbol) &&
                !DependentExpressionContainsTemplateParameter(initializer)) {
              AnalyzeCXXStaticDataMemberConstantInitializer(parser, member_symbol,
                                                            initializer);
            } else {
              QueueCXXInlineStaticDataMemberDefinition(
                  parser, str, member, initializer, is_inline_member);
            }
            // Inject a clone into the class scope so static data members
            // resolve by unqualified name in later member initializers.  For
            // non-dependent constants this clone carries the folded value; for
            // dependent class templates it preserves the symbol identity until
            // instantiation can fold the initializer.
            bool inject_scope_member =
                member_symbol->flags.value_set ||
                (parser->syntax->parsing_template_declaration &&
                 member->is_static && !member->is_member_function);
            if (inject_scope_member) {
              Symbol* scope_constant = SymbolClone(member_symbol);
              if (!SyntaxAddSymbol(parser->syntax, scope_constant)) {
                SymbolDelete(scope_constant);
              }
            }
          }
          member_decl_had_inline_body |= has_inline_body;
        } else {
          // Regular member, align the member to the appropriate boundary.
          AlignNextOffsetForSymbol(str, member_symbol);
          member->byte_offset = str->next_offset;
          member->index = str->members.length - 1;

          UpdateStructSize(str, member_symbol->type, is_union);
          member->default_initializer =
              SyntaxParseCXXDefaultMemberInitializer(parser->syntax);
        }
      }
      if (!LexMatch(parser->lex, TOK(comma))) {
        if (has_inline_body) {
          break;
        }
        break;
      }
    }

    if (!member_decl_had_inline_body && !LexLookingAt(parser->lex, TOK(rbrace))) {
      SyntaxNeedSemicolon(parser->syntax, TC(type));
    }
    AttributeListDestruct(&member_attributes);
    if (is_member_template) {
      SyntaxCloseScope(parser->syntax);
      VectorDestructWithContents(member_template_parameters,
                                 (VectorElementDestructor)TemplateParameterDelete,
                                 /*free_element=*/false);
      parser->syntax->parsing_template_declaration = old_parsing_template;
      parser->syntax->current_template_parameter_count =
          old_template_parameter_count;
      parser->syntax->current_template_parameters = old_template_parameters;
      ConstraintExprDelete(member_template_requires_clause);
      parser->syntax->current_template_requires_clause = old_requires_clause;
      member_template_requires_clause = NULL;
    }
  }

  // Every member is now declared; re-parse the deferred inline bodies in
  // complete-class context, then restore this frame's collection pointer.
  FlushDeferredInlineMemberBodies(parser, &deferred_inline_bodies);
  parser->deferred_inline_bodies = saved_deferred_inline_bodies;
  VectorDestruct(&deferred_inline_bodies);
}
