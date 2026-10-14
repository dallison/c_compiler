//
//  expr_parser_member.c
//  c_compiler
//
//  Implicit this, enclosing-class members, and member access (., ->).
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


// While a lambda body is parsed, an explicit `this` names the enclosing
// member function's object rather than the closure's synthesized `this`.
// Nested lambda parsing saves and restores this override. Member lookup and
// `this` parsing read it; lambda parsing writes it.
Symbol* lambda_enclosing_this;


static StructMember* FindMemberOnThis(Symbol* this_symbol, const char* name);
static bool MemberAccessObjectIsDependent(ASTNode* object);
static ASTNode* ParseQualifiedNestedStructMember( ASTNode* left, ASTOpcode op, Syntax* syntax, String* scope_name, Vector* scope_args, String* member_name);

// Finds the current member function’s implicit this parameter.
Symbol* FindThisSymbol(Syntax* syntax) {
  String this_name;
  StringInit(&this_name, "this");
  Symbol* symbol = SyntaxFindSymbol(syntax, &this_name);
  StringDestruct(&this_name);
  return symbol;
}


// Returns the class represented by a valid implicit this symbol.
Struct* StructFromThisSymbol(Symbol* this_symbol) {
  if (this_symbol == NULL || this_symbol->type == NULL ||
      !TypeIsStructOrUnionPointer(this_symbol->type) ||
      this_symbol->type->next == NULL) {
    return NULL;
  }
  return this_symbol->type->next->info.struct_info;
}


// Looks up a named member on the class addressed by an implicit this symbol.
static StructMember* FindMemberOnThis(Symbol* this_symbol, const char* name) {
  Struct* owner = StructFromThisSymbol(this_symbol);
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  String member_name;
  StringInit(&member_name, name);
  StructMember* member = FindStructMember(owner, &member_name);
  StringDestruct(&member_name);
  return member;
}


// Inside a lambda, `this` is the closure.  Unqualified names that name
// members of the enclosing class must go through that class's `this`
// ([expr.prim.lambda.capture]): `[&]{ pos_ = buf_; }` is `this->pos_`.
Symbol* ThisSymbolForUnqualifiedMember(
    Syntax* syntax, FullyQualifiedIdentifier* name,
    bool allow_unresolved_member) {
  Symbol* closure_this = FindThisSymbol(syntax);
  if (name == NULL || name->is_qualified) {
    return closure_this;
  }
  const char* member_name = FullyQualifiedIdentifierLast(name);
  StructMember* closure_member = FindMemberOnThis(closure_this, member_name);
  if (lambda_enclosing_this == NULL) {
    return closure_this;
  }
  StructMember* enclosing_member =
      FindMemberOnThis(lambda_enclosing_this, member_name);
  if (closure_member != NULL && !closure_member->is_static) {
    return closure_this;
  }
  if (enclosing_member != NULL && !enclosing_member->is_static) {
    return lambda_enclosing_this;
  }
  if (allow_unresolved_member &&
      StructFromThisSymbol(lambda_enclosing_this) != NULL) {
    return lambda_enclosing_this;
  }
  return closure_this;
}


// Builds this->member for an unqualified member name, optionally retaining an
// unresolved dependent member.
ASTNode* NewMemberAccessFromThis(Syntax* syntax,
                                        FullyQualifiedIdentifier* name,
                                        bool allow_unresolved_member) {
  if (name->is_qualified) {
    return NULL;
  }
  Symbol* this_symbol = ThisSymbolForUnqualifiedMember(
      syntax, name, allow_unresolved_member);
  if (StructFromThisSymbol(this_symbol) == NULL) {
    return NULL;
  }

  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(name));
  StructMember* member =
      FindStructMember(this_symbol->type->next->info.struct_info, &member_name);
  if (member != NULL && member->is_static) {
    // The overload head can be static while a later overload is not
    // (`static Capacity(size_t)` then `Capacity() const`).  Keep the
    // `this->` form so the call resolves against the whole set.
    bool has_nonstatic = false;
    for (StructMember* overload = member->overload_next; overload != NULL;
         overload = overload->overload_next) {
      if (!overload->is_static) {
        has_nonstatic = true;
        break;
      }
    }
    if (!has_nonstatic) {
      StringDestruct(&member_name);
      return NULL;
    }
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


// Default member initializers are parsed while the class is still incomplete,
// so a later data member (`char* pos_ = buf_; char buf_[N];`) is not yet in
// the symbol table.  Build `this->name` against the class being defined; member
// analysis runs after the class is complete and resolves the name then.
ASTNode* NewMemberAccessFromEnclosingClass(
    Syntax* syntax, FullyQualifiedIdentifier* name) {
  if (!CompilerIsCXX() || name == NULL || name->is_qualified ||
      !syntax->parsing_default_member_initializer ||
      syntax->cxx_class_head == NULL ||
      syntax->cxx_class_head->tag_symbol == NULL ||
      syntax->cxx_class_head->tag_symbol->type == NULL) {
    return NULL;
  }
  if (FindThisSymbol(syntax) != NULL) {
    return NULL;
  }
  TypeRecord* class_type =
      TypeRecordCopy(syntax->cxx_class_head->tag_symbol->type);
  Symbol* this_symbol =
      NewSymbol("this", NewPointerTo(kQualPlain, class_type), STO(implicit));
  this_symbol->flags.invented = true;
  ASTNode* left =
      NewIdentifierASTNode(this_symbol, syntax->lex->current_token_location);
  ASTNode* right = NewStringConstantASTNode(
      NewString(FullyQualifiedIdentifierLast(name)), NULL,
      syntax->lex->current_token_location);
  return NewBinaryASTNode(AST_OP(arrow), NULL,
                          syntax->lex->current_token_location, left, right);
}


// Builds this->Base::member when a qualified name selects a base-class member.
ASTNode* NewQualifiedBaseMemberAccessFromThis(
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
  Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
      syntax, name, name->components.length - 1);
  if ((owner == NULL || owner->type == NULL) &&
      name->components.length == 2 && !name->absolute) {
    Struct* derived = this_symbol->type->next->info.struct_info;
    String* base_name = name->components.value.p[0];
    for (size_t i = 0; derived != NULL && i < derived->bases.length; i++) {
      CXXBaseSpecifier* base = derived->bases.value.p[i];
      if (base == NULL || base->type == NULL ||
          !TypeIsStructOrUnion(base->type) ||
          base->type->info.struct_info == NULL ||
          base->type->info.struct_info->tag_name == NULL) {
        continue;
      }
      if (StringEqual(base_name, base->type->info.struct_info->tag_name->value)) {
        owner = base->type->info.struct_info->tag_symbol;
        break;
      }
    }
  }
  if (owner == NULL || owner->type == NULL) {
    return NULL;
  }
  // `using ostream = basic_ostream<char>` is formed while the class template
  // is only forward-declared.  The alias still names that incomplete primary;
  // the derived class's base is the completed specialization.  Materialize
  // before the derived-to-base check and the member lookup.
  TypeRecord* owner_type = owner->type;
  TypeRecord* materialized =
      TypeMaterializeClassTemplateSpecialization(syntax, owner_type);
  bool own_materialized = materialized != NULL && materialized != owner_type;
  if (own_materialized && TypeIsStructOrUnion(materialized) &&
      materialized->info.struct_info != NULL) {
    owner_type = materialized;
  } else if (own_materialized) {
    TypeRecordDelete(materialized);
    own_materialized = false;
  }
  if (!TypeIsStructOrUnion(owner_type) ||
      owner_type->info.struct_info == NULL) {
    if (own_materialized) {
      TypeRecordDelete(materialized);
    }
    return NULL;
  }
  if (TypeContainsTemplateParameter(owner_type)) {
    if (own_materialized) {
      TypeRecordDelete(materialized);
    }
    return NULL;
  }
  // A qualified-id is dependent only when its nested-name-specifier depends on
  // a template parameter.  Class-template bases stay unresolved inside a
  // template definition so a later instantiation can still see the derived
  // class's hidden overloads (basic_ostream).  A non-template base is not
  // dependent: `UnorderedElementsAreMatcherImplBase::DescribeToImpl` must bind
  // while the member body is parsed.
  Struct* owner_struct = owner_type->info.struct_info;
  if (syntax->current_template_parameter_count > 0 &&
      (owner_struct->is_template || owner_type->template_origin != NULL ||
       owner_struct->template_parameter_count > 0)) {
    if (own_materialized) {
      TypeRecordDelete(materialized);
    }
    return NULL;
  }
  TypeRecord* receiver_type = this_symbol->type->next;
  if (receiver_type->info.struct_info != owner_struct &&
      !StructIsDerivedFrom(receiver_type->info.struct_info, owner_struct,
                           /*public_only=*/false)) {
    if (own_materialized) {
      TypeRecordDelete(materialized);
    }
    return NULL;
  }
  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(name));
  StructMember* member = FindStructMember(owner_struct, &member_name);
  if (member == NULL || member->symbol == NULL) {
    StringDestruct(&member_name);
    if (own_materialized) {
      TypeRecordDelete(materialized);
    }
    return NULL;
  }
  if (member->is_static) {
    ASTNode* ref = NewIdentifierASTNode(
        member->symbol, syntax->lex->current_token_location);
    ref->flags |= kASTQualifiedName;
    StringDestruct(&member_name);
    if (own_materialized) {
      TypeRecordDelete(materialized);
    }
    return ref;
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
  member_node->owner_type = owner_type;
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
ASTNode* NewStaticMemberReference(Syntax* syntax,
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
  StructMember* member = NULL;
  bool in_enclosing_class = false;
  for (int depth = 0; owner != NULL && depth < 16; depth++) {
    member = FindStructMember(owner, &member_name);
    if (member != NULL) {
      break;
    }
    // A local class sees the names of the class whose member function it is
    // defined in; only its static members are usable there ([class.local]).
    Symbol* enclosing = owner->access_enclosing_function;
    owner = enclosing != NULL && enclosing->type != NULL &&
                    TypeIsFunction(enclosing->type)
                ? enclosing->type->info.function.cxx_member_owner
                : NULL;
    in_enclosing_class = true;
  }
  StringDestruct(&member_name);
  if (member == NULL || !member->is_static || member->symbol == NULL ||
      (member->is_member_function && !in_enclosing_class)) {
    return NULL;
  }
  ASTNode* ref = NewIdentifierASTNode(member->symbol,
                                      syntax->lex->current_token_location);
  if (member->is_member_function) {
    ref->flags |= kASTQualifiedName;
  }
  return ref;
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
Symbol* CurrentClassSelfTagSymbol(Syntax* syntax,
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


// Inside a member body of an explicit or partial specialization
// (`template <> struct hash<E>`), the injected-class-name followed by `<`
// names the class template itself (`hash<unsigned long>()`), not the
// specialization.  Returns the primary class template when `name` is the
// injected-class-name of the current class `str` and `str` is such a
// specialization; otherwise NULL.
Symbol* CurrentClassSpecializationPrimary(FullyQualifiedIdentifier* name,
                                                 Struct* str) {
  if (!CompilerIsCXX() || name->is_qualified || str == NULL ||
      str->tag_name == NULL || str->tag_symbol == NULL ||
      str->lexical_parent != NULL) {
    return NULL;
  }
  Struct* owner = NULL;
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    owner = compiler->current_function->info.function.cxx_member_owner;
  }
  if (owner != str) {
    return NULL;
  }
  const char* last = FullyQualifiedIdentifierLast(name);
  size_t length = strlen(last);
  if (strncmp(str->tag_name->value, last, length) != 0 ||
      str->tag_name->value[length] != '<') {
    return NULL;
  }
  String bare;
  StringInit(&bare, last);
  Symbol* primary =
      NamespaceLookupTagInEnclosingScopes(str->tag_symbol->namespace_, &bare);
  StringDestruct(&bare);
  if (primary == NULL || !primary->flags.is_template ||
      primary->type == NULL || !TypeIsStructOrUnion(primary->type) ||
      primary->type->info.struct_info == NULL ||
      !primary->type->info.struct_info->is_template) {
    return NULL;
  }
  return primary;
}


// Parses this and resolves it to the enclosing member object, including
// lambda-body overrides.
ASTNode* ParseThisExpression(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);
  Symbol* this_symbol =
      lambda_enclosing_this != NULL ? lambda_enclosing_this
                                    : FindThisSymbol(syntax);
  if (this_symbol == NULL && syntax->parsing_default_member_initializer &&
      syntax->cxx_class_head != NULL &&
      syntax->cxx_class_head->tag_symbol != NULL &&
      syntax->cxx_class_head->tag_symbol->type != NULL) {
    // An NSDMI is parsed before constructors (and their implicit `this`
    // parameters) exist.  Keep an invented placeholder that constructor
    // synthesis later rebinds just like an implicit member access.
    TypeRecord* class_type =
        TypeRecordCopy(syntax->cxx_class_head->tag_symbol->type);
    this_symbol =
        NewSymbol("this", NewPointerTo(kQualPlain, class_type), STO(implicit));
    this_symbol->flags.invented = true;
  }
  if (this_symbol == NULL) {
    SyntaxError(syntax, "'this' is only valid inside a C++ member function");
    TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                             kQualPlain);
    return (ASTNode*)NewIntConstantASTNode(0, type, location);
  }
  return NewIdentifierASTNode(this_symbol, location);
}


// Reports whether the object side of a member access still has a dependent
// type.
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


// `expr.StorageT<I>::get()`: the first identifier is a nested-name-specifier
// naming a member alias (or base), not the accessed member.  Build a qualified
// member access whose owner is the alias template-id so instantiation resolves
// `get` on `Storage<T, I>` rather than looking up `::get` in the derived class.
static ASTNode* ParseQualifiedNestedStructMember(
    ASTNode* left, ASTOpcode op, Syntax* syntax, String* scope_name,
    Vector* scope_args, String* member_name) {
  SourceLocation location = syntax->lex->current_token_location;
  if (member_name == NULL) {
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected member name after '::'");
      StringDelete(scope_name);
      if (scope_args != NULL) {
        VectorDeleteWithContents(scope_args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
      }
      return NewBinaryASTNode(
          op, NULL, location, left,
          NewStringConstantASTNode(NewString(SyntaxFakeName(syntax)), NULL,
                                   location));
    }
    member_name = NewString(syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  }

  Symbol* scope_symbol = SyntaxFindSymbol(syntax, scope_name);
  if (scope_symbol == NULL && left != NULL && left->type != NULL) {
    TypeRecord* receiver = left->type;
    if (TypeIsReference(receiver)) {
      receiver = receiver->next;
    }
    if (op == AST_OP(arrow) && receiver != NULL && TypeIsPointer(receiver)) {
      receiver = receiver->next;
    }
    if (receiver != NULL && TypeIsStructOrUnion(receiver) &&
        receiver->info.struct_info != NULL) {
      StructMember* scope_member =
          FindStructMember(receiver->info.struct_info, scope_name);
      if (scope_member != NULL) {
        scope_symbol = scope_member->symbol;
      }
    }
  }

  TypeRecord* owner_type = NULL;
  StructMember* member = NULL;
  if (scope_symbol != NULL && scope_symbol->flags.is_template) {
    owner_type = NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
    owner_type->template_origin = scope_symbol;
    owner_type->template_arguments = scope_args;
    scope_args = NULL;
    if (scope_symbol->type != NULL && TypeIsStructOrUnion(scope_symbol->type) &&
        scope_symbol->type->info.struct_info != NULL) {
      member = FindStructMember(scope_symbol->type->info.struct_info,
                                member_name);
    }
  } else if (scope_symbol != NULL && scope_symbol->type != NULL &&
             TypeIsStructOrUnion(scope_symbol->type) &&
             scope_symbol->type->info.struct_info != NULL) {
    owner_type = TypeRecordCopy(scope_symbol->type);
    member = FindStructMember(scope_symbol->type->info.struct_info, member_name);
  }

  ASTNode* right;
  if (member != NULL && member->symbol != NULL) {
    right = NewStructMemberASTNode(member, location);
    right->flags |= kASTQualifiedName;
    ((StructMemberASTNode*)right)->owner_type = owner_type;
    owner_type = NULL;
  } else {
    right = NewStringConstantASTNode(member_name, NULL, location);
    member_name = NULL;
    TypeRecordDelete(owner_type);
  }
  StringDelete(scope_name);
  if (member_name != NULL) {
    StringDelete(member_name);
  }
  if (scope_args != NULL) {
    VectorDeleteWithContents(scope_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  return NewBinaryASTNode(op, NULL, location, left, right);
}


// The name after `.` or `->`. A splice, an operator-function-id, a
// template-id, or a qualified `Base::member` each take their own branch;
// anything else is an ordinary member access.
ASTNode* ParseStructMember(ASTNode* left, ASTOpcode op, Syntax* syntax,
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
  String* qualified_base_scope = NULL;
  TypeRecord* conversion_type = NULL;
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
  } else if (CompilerIsCXX() &&
             LexLookingAt(syntax->lex, TOK(operator))) {
    // Explicit operator / conversion call, e.g. `x.operator+(y)`,
    // `x.operator()(y)`, `p->operator int()`,
    // `this->template operator[]<K>(k)`.  Build the same member name the
    // operator/conversion function was registered under so the access resolves
    // to it.
    String op_name;
    if (SyntaxParseMemberOperatorName(syntax, &op_name, &conversion_type)) {
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
  // `node->Node::~Node()` and `rep_->Rep::~Rep()`.  The nested-name-specifier
  // names the type being destroyed; explicit-destructor analysis matches the
  // `~Name` spelling.
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(coloncolon))) {
    while (LexMatch(syntax->lex, TOK(coloncolon))) {
      if (LexMatch(syntax->lex, TOK(tilde)) &&
          LexLookingAt(syntax->lex, TOK(identifier))) {
        String* dtor = NewString("~");
        StringAppend(dtor, syntax->lex->spelling.value);
        LexNextToken(syntax->lex);
        StringDelete(member_name);
        member_name = dtor;
        break;
      }
      // `p->Base::template AddAllocated<H>(...)`: the base-class qualifier is
      // followed by a dependent template member name, not a destructor.
      if (LexMatch(syntax->lex, TOK(template))) {
        saw_template_keyword = true;
        qualified_base_scope = member_name;
        member_name = NULL;
        if (LexLookingAt(syntax->lex, TOK(identifier))) {
          member_name = NewString(syntax->lex->spelling.value);
          LexNextToken(syntax->lex);
        } else if (LexLookingAt(syntax->lex, TOK(operator))) {
          String op_name;
          if (SyntaxParseMemberOperatorName(syntax, &op_name, NULL)) {
            member_name = NewString(op_name.value);
            StringDestruct(&op_name);
          } else {
            SyntaxError(syntax, "Expected operator or conversion name");
            member_name = NewString(SyntaxFakeName(syntax));
          }
        } else {
          SyntaxError(syntax,
                      "Expected template member name after 'template'");
          member_name = NewString(SyntaxFakeName(syntax));
        }
        break;
      }
      if (!LexLookingAt(syntax->lex, TOK(identifier))) {
        SyntaxError(syntax, "Expected destructor name");
        break;
      }
      LexNextToken(syntax->lex);
    }
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
    has_template_arguments =
        !not_a_template_id && depth == 0 &&
        (LexLookingAt(syntax->lex, TOK(lparen)) ||
         LexLookingAt(syntax->lex, TOK(coloncolon)));
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    // We resolved the `<` as a template-argument list on a type-dependent
    // object without the `template` disambiguator.  This is well-formed here
    // only thanks to a lookahead heuristic; the standard requires the keyword,
    // and other compilers reject it, so steer the user toward portable code.
    if (has_template_arguments && member_name != NULL &&
        MemberAccessObjectIsDependent(left)) {
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
  if (conversion_type != NULL &&
      (qualified_base_scope != NULL ||
       (CompilerIsCXX() && template_arguments != NULL &&
        LexLookingAt(syntax->lex, TOK(coloncolon))))) {
    TypeRecordDelete(conversion_type);
    conversion_type = NULL;
  }
  if (qualified_base_scope != NULL) {
    String* scope_name = NewString(qualified_base_scope->value);
    String* inner_member = NewString(member_name->value);
    StringDelete(qualified_base_scope);
    StringDelete(member_name);
    member_name = NULL;
    qualified_base_scope = NULL;
    ASTNode* result = ParseQualifiedNestedStructMember(
        left, op, syntax, scope_name, NULL, inner_member);
    if (template_arguments != NULL && result != NULL) {
      BinaryASTNode* access = (BinaryASTNode*)result;
      ASTNode* member_node = access->right;
      if (member_node != NULL &&
          member_node->op == AST_OP(structmember)) {
        ((StructMemberASTNode*)member_node)->template_arguments =
            template_arguments;
        template_arguments = NULL;
      } else if (member_node != NULL &&
                 member_node->op == AST_OP(identifier)) {
        ((IdentifierASTNode*)member_node)->template_arguments =
            template_arguments;
        template_arguments = NULL;
      } else {
        VectorDeleteWithContents(
            template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
    } else if (template_arguments != NULL) {
      VectorDeleteWithContents(
          template_arguments,
          (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    return result;
  }
  if (CompilerIsCXX() && template_arguments != NULL &&
      LexMatch(syntax->lex, TOK(coloncolon))) {
    return ParseQualifiedNestedStructMember(left, op, syntax, member_name,
                                           template_arguments, NULL);
  }
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
  ((ConstantASTNode*)member_node)->conversion_type = conversion_type;
  return NewBinaryASTNode(op, NULL,
                          syntax->lex->current_token_location, left,
                          member_node);
}
