//
//  syntax_ctor_preamble.c
//  c_compiler
//
//  The constructor preamble: base and member initialization statements
//  inserted before a constructor body, and their cleanups.
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

// True when some user-provided default constructor in the overload set for
// `record_type` has only defaulted parameters after the complete-object slot.
static bool CXXConstructorSetHasUserProvidedDefault(TypeRecord* record_type,
                                                    StructMember* constructor) {
  if (record_type == NULL || record_type->info.struct_info == NULL) {
    return false;
  }
  size_t first_user_formal =
      StructHasVirtualBases(record_type->info.struct_info) ? 2 : 1;
  for (StructMember* candidate = constructor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type)) {
      continue;
    }
    FunctionInfo* info = &candidate->symbol->type->info.function;
    if (!info->is_constructor || info->is_deleted ||
        !info->is_user_provided ||
        info->prototype.length < first_user_formal) {
      continue;
    }
    bool all_defaulted = true;
    for (size_t i = first_user_formal; i < info->prototype.length; i++) {
      Symbol* formal = info->prototype.value.p[i];
      if (formal == NULL || SymbolExtraOf(formal)->default_argument == NULL) {
        all_defaulted = false;
        break;
      }
    }
    if (all_defaulted) {
      return true;
    }
  }
  return false;
}

// Returns a value-initialization (`init` with `{}`) of member `member` via
// `this`, marked for memzero before a trivial default constructor runs.
static ASTNode* NewCXXAggregateMemberZeroInitializer(
    Syntax* syntax, TypeRecord* func, StructMember* member,
    SourceLocation location) {
  (void)syntax;
  ASTNode* target =
      NewCXXThisMemberAccess(func, member->symbol->name.value, location);
  if (target == NULL) {
    return NULL;
  }
  // Value-initialization zero-fills the object, then runs a non-trivial
  // default constructor.  Assigning an empty temporary would call operator=,
  // which value-initialization does not do and which may be deleted.
  target->flags |= kASTNeedAddress;
  ASTNode* init = NewBinaryASTNode(
      AST_OP(init), member->symbol->type, location, target,
      NewBracedInitializerASTNode(NewVector(), NULL, location));
  init->flags |= kASTCXXMemberInitializer | kASTValueInitMemzero;
  return NewExpressionStatementASTNode(init, location);
}

// Returns a call of special member `name` on subobject expression `element`,
// with the complete-object argument prepended when required.
ASTNode* NewCXXElementSpecialMemberCall(ASTNode* element,
                                        TypeRecord* element_type,
                                        const char* name,
                                        SourceLocation location) {
  ASTNode* member = NewStringConstantASTNode(NewString(name), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, element, member);
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(element_type, actuals,
                                   /*complete_object=*/true, location);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access,
                          actuals);
}

// Calls special member `name` on `count` consecutive elements starting at the
// element `first` designates, counting up, or down from the last when
// `reverse` is set:
//   index = 0; while (index < count) { (&first)[index].name(); ++index; }
ASTNode* NewCXXArrayElementLoop(Syntax* syntax, ASTNode* first,
                                TypeRecord* element_type,
                                unsigned long count, const char* name,
                                bool reverse, SourceLocation location) {
  TypeRecord* index_type =
      NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain);
  Symbol* index = SyntaxNewTemporary(syntax, index_type);
  ASTNode* start = NewIntConstantASTNode(reverse ? count : 0,
                                         TypeRecordCopy(index_type), location);
  ASTNode* end = NewIntConstantASTNode(reverse ? 0 : count,
                                       TypeRecordCopy(index_type), location);
  ASTNode* step = NewBinaryASTNode(
      AST_OP(assign), NULL, location, NewIdentifierASTNode(index, location),
      NewBinaryASTNode(reverse ? AST_OP(minus) : AST_OP(plus), NULL, location,
                       NewIdentifierASTNode(index, location),
                       NewIntConstantASTNode(1, TypeRecordCopy(index_type),
                                             location)));
  ASTNode* base = NewUnaryASTNode(
      AST_OP(address), NewPointerTo(kQualPlain, TypeRecordCopy(element_type)),
      location, first);
  ASTNode* element = NewBinaryASTNode(AST_OP(subscript),
                                      TypeRecordCopy(element_type), location,
                                      base, NewIdentifierASTNode(index, location));
  ASTNode* call =
      NewCXXElementSpecialMemberCall(element, element_type, name, location);

  Vector* body = NewVector();
  if (reverse) {
    VectorAppend(body, NewExpressionStatementASTNode(step, location));
    VectorAppend(body, NewExpressionStatementASTNode(call, location));
  } else {
    VectorAppend(body, NewExpressionStatementASTNode(call, location));
    VectorAppend(body, NewExpressionStatementASTNode(step, location));
  }
  ASTNode* condition = NewBinaryASTNode(
      reverse ? AST_OP(greater) : AST_OP(less), NULL, location,
      NewIdentifierASTNode(index, location), end);
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewExpressionStatementASTNode(
                   NewBinaryASTNode(AST_OP(assign), NULL, location,
                                    NewIdentifierASTNode(index, location),
                                    start),
                   location));
  VectorAppend(statements,
               NewCombinedStatementASTNode(
                   AST_OP(while), condition,
                   NewCompoundStatementASTNode(body, location), location));
  return NewCompoundStatementASTNode(statements, location);
}

// Returns `this->name[index]` for array member `name` of the constructor
// `func`'s class.
static ASTNode* NewCXXThisMemberElement(TypeRecord* func, const char* name,
                                        size_t index,
                                        SourceLocation location) {
  ASTNode* receiver = NewCXXThisMemberAccess(func, name, location);
  return NewBinaryASTNode(
      AST_OP(subscript), NULL, location, receiver,
      NewIntConstantASTNode((int64_t)index,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location));
}

// Returns the statement initializing element `index` of array member
// `member`: a constructor call with the element's initializer (taken out of
// `actuals`) when the element type has constructors, else an assignment of
// the initializer or of zero.
static ASTNode* NewCXXArrayMemberElementInitializer(
    TypeRecord* func, StructMember* member, TypeRecord* element_type,
    StructMember* element_constructor, Vector* actuals, size_t index,
    SourceLocation location) {
  ASTNode* receiver = NewCXXThisMemberElement(func, member->symbol->name.value,
                                              index, location);
  if (element_constructor != NULL) {
    Vector* element_actuals = NewVector();
    if (index < actuals->length) {
      VectorAppend(element_actuals, actuals->value.p[index]);
      actuals->value.p[index] = NULL;
    }
    CXXPrependCompleteObjectArgument(element_type, element_actuals,
                                     /*complete_object=*/true, location);
    ASTNode* constructor_name = NewStringConstantASTNode(
        NewString(CXXConstructorNameForType(element_type)), NULL, location);
    ASTNode* access = NewBinaryASTNode(AST_OP(dot), NULL, location,
                                       receiver, constructor_name);
    ASTNode* call =
        NewVectorASTNode(AST_OP(call), NULL, location, access,
                         element_actuals);
    call->flags |= kASTCXXMemberInitializer;
    return NewExpressionStatementASTNode(call, location);
  }
  ASTNode* value =
      index < actuals->length
          ? actuals->value.p[index]
          : NewIntConstantASTNode(
                0, TypeRecordCopy(element_type), location);
  if (index < actuals->length) {
    actuals->value.p[index] = NULL;
  }
  ASTNode* assign = NewBinaryASTNode(AST_OP(assign), element_type,
                                     location, receiver, value);
  assign->flags |= kASTCXXMemberInitializer;
  return NewExpressionStatementASTNode(assign, location);
}

// Returns the initialization of fixed-array member `member` from `actuals`
// (consumed): one initializer per element, with an exception-only destructor
// after each element that needs one, or a loop for a long array that is
// default-constructed.  Returns NULL when the bound is still dependent or
// there are too many initializers.
static ASTNode* NewCXXArrayMemberInitializer(Syntax* syntax, TypeRecord* func,
                                             StructMember* member,
                                             Vector* actuals,
                                             SourceLocation location) {
  TypeRecord* member_type = member->symbol->type;
  // `uint32_t words_[max_words]` still has a placeholder bound while the
  // class template is parsed.  The mem-initializer is kept in the raw list
  // and lowered once the specialization's bound is known.
  if (member_type->info.array.template_parameter_index >= 0 ||
      member_type->info.array.is_dependent_bound) {
    VectorDeleteWithContents(actuals, (VectorElementDestructor)ASTNodeDelete,
                             /*free_element=*/false);
    return NULL;
  }
  size_t element_count = member_type->info.array.size.fixed;
  if (actuals->length > element_count) {
    SyntaxError(syntax, "too many initializers for array member %s",
                member->symbol->name.value);
    VectorDeleteWithContents(actuals,
                             (VectorElementDestructor)ASTNodeDelete,
                             /*free_element=*/false);
    return NULL;
  }
  TypeRecord* element_type = member_type->next;
  StructMember* element_constructor =
      TypeIsStructOrUnion(element_type) ? FindCXXConstructor(element_type)
                                       : NULL;
  if (element_constructor != NULL && actuals->length == 0 &&
      element_count > kCXXUnrolledArrayElementLimit) {
    VectorDelete(actuals);
    ASTNode* receiver =
        NewCXXThisMemberAccess(func, member->symbol->name.value, location);
    if (receiver == NULL) {
      return NULL;
    }
    ASTNode* first = NewBinaryASTNode(
        AST_OP(subscript), NULL, location, receiver,
        NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location));
    return NewCXXArrayElementLoop(syntax, first, element_type, element_count,
                                  CXXConstructorNameForType(element_type),
                                  /*reverse=*/false, location);
  }
  Vector* statements = NewVector();
  for (size_t i = 0; i < element_count; i++) {
    VectorAppend(statements, NewCXXArrayMemberElementInitializer(
                                 func, member, element_type,
                                 element_constructor, actuals, i, location));
    if (CompilerExceptionsEnabled() &&
        TypeHasNonTrivialDestructor(element_type)) {
      ASTNode* cleanup_receiver = NewCXXThisMemberElement(
          func, member->symbol->name.value, i, location);
      ASTNode* cleanup = NewCXXMemberDestructorCall(
          syntax, func, member, element_type, cleanup_receiver, location);
      if (cleanup != NULL) {
        cleanup->flags |= kASTEHCleanupOnly;
        VectorAppend(statements, cleanup);
      }
    }
  }
  VectorDeleteWithContents(actuals,
                           (VectorElementDestructor)ASTNodeDelete,
                           /*free_element=*/false);
  return NewCompoundStatementASTNode(statements, location);
}

// Returns `this->member = { ... }` initializing aggregate member `member`
// directly from the braced initializer that is the only element of
// `actuals` (consumed).
static ASTNode* NewCXXBracedAggregateMemberInitializer(
    TypeRecord* func, StructMember* member, Vector* actuals,
    SourceLocation location) {
  ASTNode* initializer = actuals->value.p[0];
  actuals->value.p[0] = NULL;
  VectorDelete(actuals);
  ASTNode* target =
      NewCXXThisMemberAccess(func, member->symbol->name.value, location);
  if (target == NULL) {
    ASTNodeDelete(initializer);
    return NULL;
  }
  target->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* init =
      NewBinaryASTNode(AST_OP(init), member->symbol->type, location, target,
                       initializer);
  init->flags |= kASTCXXMemberInitializer;
  return NewExpressionStatementASTNode(init, location);
}

// Returns the statement `this->member.constructor_name(actuals)` that
// constructs `member` (of type `member_type`) as a complete object.  Consumes
// `actuals`; returns NULL when the member or constructor name is unavailable.
static ASTNode* NewCXXMemberConstructorCallStatement(
    TypeRecord* func, StructMember* member, TypeRecord* member_type,
    const char* constructor_name, Vector* actuals, SourceLocation location) {
  ASTNode* receiver =
      NewCXXThisMemberAccess(func, member->symbol->name.value, location);
  if (receiver == NULL || constructor_name == NULL) {
    VectorDelete(actuals);
    return NULL;
  }
  CXXPrependCompleteObjectArgument(member_type, actuals,
                                   /*complete_object=*/true, location);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(constructor_name), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member_name);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  call->flags |= kASTCXXMemberInitializer;
  return NewExpressionStatementASTNode(call, location);
}

// Returns the constructor call initializing class member `member` from
// `actuals` (consumed).  Value-initialization (`m()`) of a class without a
// user-provided default constructor zeroes the member first.
static ASTNode* NewCXXMemberConstructorInitializer(
    Syntax* syntax, TypeRecord* func, StructMember* member,
    StructMember* constructor, Vector* actuals, bool value_initialize_empty,
    SourceLocation location) {
  TypeRecord* member_type = member->symbol->type;
  bool zero_before_default =
      value_initialize_empty && actuals->length == 0 &&
      !CXXConstructorSetHasUserProvidedDefault(member_type, constructor);
  ASTNode* constructor_call = NewCXXMemberConstructorCallStatement(
      func, member, member_type, CXXConstructorNameForType(member_type),
      actuals, location);
  if (constructor_call == NULL || !zero_before_default) {
    return constructor_call;
  }
  ASTNode* zero = NewCXXAggregateMemberZeroInitializer(
      syntax, func, member, location);
  if (zero == NULL) {
    return constructor_call;
  }
  Vector* statements = NewVector();
  VectorAppend(statements, zero);
  VectorAppend(statements, constructor_call);
  return NewCompoundStatementASTNode(statements, location);
}

// Returns the value-initialization of `member` from an empty
// mem-initializer (`m()`): zeroing for an aggregate or array, else
// `this->member = 0`.
static ASTNode* NewCXXValueInitializedMemberInitializer(
    Syntax* syntax, TypeRecord* func, StructMember* member,
    SourceLocation location) {
  TypeRecord* member_type = member->symbol->type;
  if (TypeIsStructOrUnion(member_type) || TypeIsArray(member_type)) {
    return NewCXXAggregateMemberZeroInitializer(syntax, func, member,
                                                location);
  }
  ASTNode* target =
      NewCXXThisMemberAccess(func, member->symbol->name.value, location);
  if (target == NULL) {
    return NULL;
  }
  ASTNode* value =
      NewIntConstantASTNode(0, TypeRecordCopy(member_type), location);
  ASTNode* assign =
      NewBinaryASTNode(AST_OP(assign), member_type, location, target, value);
  assign->flags |= kASTCXXMemberInitializer;
  return NewExpressionStatementASTNode(assign, location);
}

// Returns the statement initializing data member `member` of the constructor
// `func`'s class from mem-initializer arguments `actuals` (consumed): an
// element-wise array initialization, a braced aggregate initialization, a
// constructor call, value-initialization, or an assignment of the single
// argument.  `value_initialize_empty` zeroes a class member initialized with
// `()` before its implicit default constructor runs.  Returns NULL when
// nothing is to be done now or on error.
ASTNode* NewCXXMemberInitializerStatement(Syntax* syntax,
                                                TypeRecord* func,
                                                StructMember* member,
                                                Vector* actuals,
                                                bool value_initialize_empty,
                                                SourceLocation location) {
  if (member == NULL || member->symbol == NULL || actuals == NULL) {
    if (actuals != NULL) {
      VectorDelete(actuals);
    }
    return NULL;
  }

  TypeRecord* member_type = member->symbol->type;
  if (TypeIsFixedArray(member_type) && member_type->next != NULL) {
    return NewCXXArrayMemberInitializer(syntax, func, member, actuals,
                                        location);
  }
  StructMember* constructor = TypeIsStructOrUnion(member_type)
                                  ? FindCXXConstructor(member_type)
                                  : NULL;
  bool braced_aggregate =
      TypeIsStructOrUnion(member_type) &&
      member_type->info.struct_info != NULL &&
      member_type->info.struct_info->is_aggregate &&
      actuals->length == 1 &&
      actuals->value.p[0] != NULL &&
      ((ASTNode*)actuals->value.p[0])->op == AST_OP(braced_init);
  if (braced_aggregate) {
    return NewCXXBracedAggregateMemberInitializer(func, member, actuals,
                                                  location);
  }
  if (constructor != NULL) {
    return NewCXXMemberConstructorInitializer(syntax, func, member,
                                              constructor, actuals,
                                              value_initialize_empty,
                                              location);
  }

  if (actuals->length == 0) {
    VectorDelete(actuals);
    return NewCXXValueInitializedMemberInitializer(syntax, func, member,
                                                   location);
  }

  if (actuals->length != 1 && member_type != NULL &&
      TypeContainsTemplateParameter(member_type)) {
    // `tree_(key_compare(), allocator_type())` names a constructor of a
    // template parameter.  The concrete constructor is chosen when the class
    // is instantiated; keep the arguments as a call until then.
    const char* constructor_name = CXXConstructorNameForType(member_type);
    if (constructor_name == NULL && member->symbol != NULL) {
      constructor_name = member->symbol->name.value;
    }
    return NewCXXMemberConstructorCallStatement(
        func, member, member_type, constructor_name, actuals, location);
  }

  if (actuals->length != 1) {
    SyntaxError(syntax,
                "member initializer for %s requires one expression",
                member->symbol->name.value);
    VectorDelete(actuals);
    return NULL;
  }
  ASTNode* value = actuals->value.p[0];
  VectorDelete(actuals);
  ASTNode* target =
      NewCXXThisMemberAccess(func, member->symbol->name.value, location);
  if (target == NULL) {
    return NULL;
  }
  ASTNode* assign =
      NewBinaryASTNode(AST_OP(assign), member_type, location, target, value);
  assign->flags |= kASTCXXMemberInitializer;
  return NewExpressionStatementASTNode(assign, location);
}

// Like NewCXXMemberInitializerStatement with value-initialization of empty `()`
// mem-initializers for class members (zero-before-default when applicable).
ASTNode* SyntaxNewCXXMemberInitializerStatement(
    Syntax* syntax, TypeRecord* func, StructMember* member, Vector* actuals,
    SourceLocation location) {
  return NewCXXMemberInitializerStatement(syntax, func, member, actuals, true,
                                          location);
}

// Partial-construction cleanup: once a constructor has fully constructed a base
// or member subobject, an exception thrown by a later initializer or by the
// constructor body must destroy that subobject (but not the incomplete `*this`).
// This inserts, right after the subobject's construction statement, its
// destructor call marked kASTEHCleanupOnly, so the backend emits it only inside
// an exception cleanup pad -- never on the normal path -- with an EH range
// covering the remainder of the constructor.  Subobjects nest by construction
// order, so the runtime destroys them in reverse (see libc/eh_throw.c and
// GenerateCompoundStatement).  Returns the number of statements inserted.
static size_t InsertCXXPartialCleanupDestructor(Vector* body, size_t insert_at,
                                                ASTNode* dtor_stmt) {
  if (dtor_stmt == NULL) {
    return 0;
  }
  dtor_stmt->flags |= kASTEHCleanupOnly;
  VectorInsertOrAppend(body, insert_at, dtor_stmt);
  return 1;
}

// Returns an exception-only statement that destroys `*this` after a delegating
// constructor's target finishes, or NULL when the destructor cannot be formed.
static ASTNode* NewCXXDelegatingConstructorCleanup(TypeRecord* func,
                                                   Struct* owner,
                                                   SourceLocation location) {
  if (func == NULL || owner == NULL || owner->tag_name == NULL ||
      owner->tag_symbol == NULL || owner->tag_symbol->type == NULL ||
      func->info.function.prototype.length == 0) {
    return NULL;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  if (this_symbol == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, owner->tag_name);
  ASTNode* destructor =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL, location);
  StringDestruct(&destructor_name);
  ASTNode* member_access = NewBinaryASTNode(
      AST_OP(arrow), NULL, location,
      NewIdentifierASTNode(this_symbol, location), destructor);
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(owner->tag_symbol->type, actuals,
                                   /*complete_object=*/true, location);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  return NewExpressionStatementASTNode(call, location);
}

// Inserts a delegating constructor's target-constructor call at the start of
// `body`, followed (with exceptions enabled) by an exception-only call of the
// class destructor: once the target constructor finishes, the object is
// complete and a throw from the delegating body must destroy it.
static void InsertCXXDelegatingConstructorCall(TypeRecord* func, Struct* owner,
                                               Vector* body,
                                               CXXConstructorInitList* init_list,
                                               SourceLocation location) {
  VectorInsertOrAppend(body, 0, init_list->delegating_statement);
  init_list->delegating_statement = NULL;
  if (CompilerExceptionsEnabled()) {
    ASTNode* cleanup =
        NewCXXDelegatingConstructorCleanup(func, owner, location);
    InsertCXXPartialCleanupDestructor(body, 1, cleanup);
  }
}

// Returns the source parameter of an implicitly defined copy or move
// constructor `func`, or NULL for any other constructor.
//
// A defaulted copy/move constructor must copy/move-construct each base
// subobject (virtual or not) from the corresponding subobject of the source,
// not default-construct it.  The source's base subobject is extracted directly
// by offset (NewCXXSourceBaseSubobject) and passed as the base-constructor
// argument, so the base's copy/move constructor binds to it without an
// access-checked derived-to-base conversion (which would reject a
// private/protected base).
static Symbol* CXXDefaultedCopyConstructorSource(TypeRecord* func) {
  CXXSpecialMemberKind ctor_kind = func->info.function.cxx_special_member_kind;
  return (ctor_kind == kCXXSpecialMemberCopyConstructor ||
          ctor_kind == kCXXSpecialMemberMoveConstructor) &&
                 !func->info.function.is_user_provided &&
                 func->info.function.prototype.length > 0
             ? func->info.function.prototype.value
                   .p[func->info.function.prototype.length - 1]
             : NULL;
}

// Inserts at `*insert_at` the virtual-base pointer setup and the
// construction of each virtual base of `owner` (explicitly initialized, copied
// from `base_copy_source`, or default-constructed), guarded so that only the
// complete-object constructor variant runs them.
static void InsertCXXVirtualBaseInitializers(Syntax* syntax, TypeRecord* func,
                                             Struct* owner,
                                             CXXConstructorInitList* init_list,
                                             Symbol* base_copy_source,
                                             Vector* body, size_t* insert_at,
                                             SourceLocation location) {
  Vector* complete_initializers = NewVector();
  AppendCXXVBPtrInitializers(func, complete_initializers, location);
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i];
    ASTNode* call =
        FindCXXExplicitVirtualBaseInitializer(init_list, base);
    if (call == NULL &&
        !VectorContainsPointer(&init_list->virtual_base_specs, base)) {
      Vector* vbase_actuals = NULL;
      if (base_copy_source != NULL) {
        CXXBaseSpecifier vbase_spec;
        vbase_spec.type = base->type;
        vbase_spec.access = base->access;
        vbase_spec.byte_offset = base->byte_offset;
        vbase_spec.is_virtual = true;
        ASTNode* source_base =
            NewCXXSourceBaseSubobject(base_copy_source, &vbase_spec, location);
        if (source_base != NULL) {
          vbase_actuals = NewVector();
          VectorAppend(vbase_actuals, source_base);
        }
      }
      call = NewCXXVirtualBaseSpecialMemberCall(syntax, func, base, false,
                                                vbase_actuals, location);
    }
    if (call == NULL) {
      continue;
    }
    VectorAppend(complete_initializers, call);
  }
  InsertCXXCompleteObjectGuardedStatements(func, body, insert_at,
                                           complete_initializers, location);
}

// Partial-construction cleanup for the virtual bases just constructed: if a
// later initializer or the constructor body throws, the complete object must
// destroy its virtual bases (in reverse construction order) yet not run its
// own destructor.  The complete-object guard restricts this to the complete-
// object constructor variant -- the base-subobject variant never builds the
// virtual bases -- and kASTEHCleanupOnly makes the backend emit it only inside
// an exception cleanup pad.  Inserted before the direct-base initializers so
// its EH range starts earliest and is therefore torn down last (matching the
// reverse-of-construction order; see GenerateCompoundStatement).
static void InsertCXXVirtualBasePartialCleanup(Syntax* syntax, TypeRecord* func,
                                               Struct* owner, Vector* body,
                                               size_t* insert_at,
                                               SourceLocation location) {
  Vector* vbase_dtors = NewVector();
  for (size_t i = owner->virtual_bases.length; i > 0; i--) {
    CXXVirtualBaseInfo* vbase = owner->virtual_bases.value.p[i - 1];
    if (!TypeHasNonTrivialDestructor(vbase->type)) {
      continue;
    }
    ASTNode* dtor = NewCXXVirtualBaseSpecialMemberCall(
        syntax, func, vbase, /*destructor=*/true, NULL, location);
    if (dtor != NULL) {
      VectorAppend(vbase_dtors, dtor);
    }
  }
  ASTNode* guarded =
      NewCXXCompleteObjectGuardedStatement(func, vbase_dtors, location);
  if (guarded != NULL) {
    guarded->flags |= kASTEHCleanupOnly;
    VectorInsertOrAppend(body, *insert_at, guarded);
    (*insert_at)++;
  }
}

// Inserts at `*insert_at` the construction of each non-virtual direct base of
// `owner` (explicitly initialized, copied from `base_copy_source`, or
// default-constructed), each followed by its exception-only destructor call.
static void InsertCXXDirectBaseInitializers(Syntax* syntax, TypeRecord* func,
                                            Struct* owner,
                                            CXXConstructorInitList* init_list,
                                            Symbol* base_copy_source,
                                            Vector* body, size_t* insert_at,
                                            SourceLocation location) {
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base->is_virtual) {
      continue;
    }
    ASTNode* call = NULL;
    for (size_t j = 0; j < init_list->base_specs.length; j++) {
      if (init_list->base_specs.value.p[j] == base) {
        call = init_list->base_statements.value.p[j];
        break;
      }
    }
    if (call == NULL && !VectorContainsPointer(&init_list->base_specs, base)) {
      Vector* base_actuals = NULL;
      if (base_copy_source != NULL) {
        ASTNode* source_base =
            NewCXXSourceBaseSubobject(base_copy_source, base, location);
        if (source_base != NULL) {
          base_actuals = NewVector();
          VectorAppend(base_actuals, source_base);
        }
      }
      call = NewCXXBaseSpecialMemberCall(syntax, func, base, false,
                                         /*complete_object=*/false, base_actuals,
                                         location);
    }
    if (call == NULL) {
      continue;
    }
    VectorInsertOrAppend(body, *insert_at, call);
    (*insert_at)++;
    if (CompilerExceptionsEnabled() && TypeHasNonTrivialDestructor(base->type)) {
      ASTNode* dtor = NewCXXBaseSpecialMemberCall(syntax, func, base,
                                                  /*destructor=*/true,
                                                  /*complete_object=*/false,
                                                  NULL, location);
      *insert_at += InsertCXXPartialCleanupDestructor(body, *insert_at, dtor);
    }
  }
}

// Inserts at `*insert_at` the __vptr initializers of `owner`, or, while the
// class is still being parsed and its vtables do not exist yet, records the
// position so SyntaxFlushPendingVPtrInitializers inserts them later.
static void InsertCXXVPtrInitializers(TypeRecord* func, Struct* owner,
                                      Vector* body, size_t* insert_at,
                                      SourceLocation location) {
  if (owner->vtables_registered) {
    Vector* vptr_initializers = NewVector();
    AppendCXXVPtrInitializers(func, vptr_initializers, location);
    for (size_t i = 0; i < vptr_initializers->length; i++) {
      VectorInsertOrAppend(body, *insert_at, vptr_initializers->value.p[i]);
      (*insert_at)++;
    }
    VectorDelete(vptr_initializers);
    return;
  }
  // The class is still being parsed, so its vtables (and the symbols the
  // __vptr initializers must reference) do not exist yet.  Defer the
  // insertion; SyntaxFlushPendingVPtrInitializers fills it in at `insert_at`
  // -- right after the base-class initializers -- once the vtables are
  // registered.  Member initializers and the user body, inserted after this
  // point, sit at higher indices and remain correctly ordered.
  if (!pending_vptr_inits_initialized) {
    VectorInit(&pending_vptr_inits);
    pending_vptr_inits_initialized = true;
  }
  PendingVPtrInit* pending = malloc(sizeof(PendingVPtrInit));
  pending->owner = owner;
  pending->func = func;
  pending->index = *insert_at;
  pending->location = location;
  VectorAppend(&pending_vptr_inits, pending);
}

// Returns true for a non-static data member (not a member
// function, static member or member typedef).
static bool CXXConstructorInitializesMember(StructMember* member) {
  return member != NULL && member->symbol != NULL && !member->is_static &&
         !member->is_member_function &&
         !StorageIs(member->symbol->storage, STO(typedef));
}

// Returns the variant member a union constructor activates.
//
// A union holds at most one active variant member, so a constructor may not
// initialize every member the way a struct constructor does.  The
// mem-initializer list names the member to activate; failing that the first
// variant member carrying a default member initializer is activated.  Every
// other variant member stays uninitialized -- running its default
// constructor would both clobber the active member's storage and leave a
// constructed object the destructor never tears down.
static StructMember* FindCXXActiveUnionVariantMember(
    Struct* owner, CXXConstructorInitList* init_list) {
  StructMember* union_variant_member = NULL;
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (!CXXConstructorInitializesMember(member)) {
      continue;
    }
    if (VectorContainsPointer(&init_list->member_specs, member)) {
      return member;
    }
    if (union_variant_member == NULL && member->default_initializer != NULL) {
      union_variant_member = member;
    }
  }
  return union_variant_member;
}

// Returns the initialization of `member` from the same member of
// `member_copy_source` in an implicitly defined copy constructor: a
// copy-construction for a class member, else an assignment.
static ASTNode* NewCXXCopiedMemberInitializer(Syntax* syntax, TypeRecord* func,
                                              StructMember* member,
                                              Symbol* member_copy_source,
                                              SourceLocation location) {
  ASTNode* source_member =
      NewCXXSourceMemberAccess(member_copy_source,
                               member->symbol->name.value, location);
  if (TypeIsStructOrUnion(member->symbol->type)) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, source_member);
    return SyntaxNewCXXMemberInitializerStatement(
        syntax, func, member, actuals, location);
  }
  ASTNode* target_member =
      NewCXXThisMemberAccess(func, member->symbol->name.value, location);
  ASTNode* assign =
      NewBinaryASTNode(AST_OP(assign), member->symbol->type, location,
                       target_member, source_member);
  assign->flags |= kASTCXXMemberInitializer;
  return NewExpressionStatementASTNode(assign, location);
}

// Partial-construction cleanup for `member`, inserted at `*insert_at` right
// after its construction.  A fixed-array member yields one destructor per
// element (reverse index order); the codegen pools the consecutive
// kASTEHCleanupOnly statements into a single cleanup pad.  This covers a throw
// after the member is fully constructed; a throw *during* element
// construction of an array is not yet covered (it needs a runtime element
// counter) and leaks the elements built so far.  Neither is an array member
// long enough to be constructed by a loop, whose per-element cleanup would be
// as long as the array.
static void InsertCXXMemberPartialCleanup(TypeRecord* func,
                                          StructMember* member, Vector* body,
                                          size_t* insert_at,
                                          SourceLocation location) {
  TypeRecord* member_type = member->symbol->type;
  unsigned long member_elements = 1;
  for (TypeRecord* t = member_type; TypeIsFixedArray(t); t = t->next) {
    member_elements *= (unsigned long)t->info.array.size.fixed;
  }
  if (CompilerExceptionsEnabled() &&
      member_elements <= kCXXUnrolledArrayElementLimit &&
      TypeHasNonTrivialDestructor(member_type)) {
    Vector member_dtors;
    VectorInit(&member_dtors);
    AppendCXXSingleMemberDestructorCalls(func, member, &member_dtors,
                                         location);
    for (size_t k = 0; k < member_dtors.length; k++) {
      *insert_at += InsertCXXPartialCleanupDestructor(
          body, *insert_at, member_dtors.value.p[k]);
    }
    VectorDestruct(&member_dtors);
  }
}

// Inserts at `*insert_at` the initialization of each data member of `owner`
// in declaration order (explicit mem-initializer, copy from
// `member_copy_source`, or default member initializer / default
// construction), each followed by its partial-construction cleanup.  A union
// initializes only its active variant member.
static void InsertCXXMemberInitializers(Syntax* syntax, TypeRecord* func,
                                        Struct* owner,
                                        CXXConstructorInitList* init_list,
                                        Symbol* member_copy_source,
                                        Vector* body, size_t* insert_at,
                                        SourceLocation location) {
  StructMember* union_variant_member = NULL;
  if (owner->is_union && member_copy_source == NULL) {
    union_variant_member = FindCXXActiveUnionVariantMember(owner, init_list);
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (!CXXConstructorInitializesMember(member)) {
      continue;
    }
    if (owner->is_union && member_copy_source == NULL &&
        member != union_variant_member) {
      continue;
    }
    ASTNode* stmt = FindCXXExplicitMemberInitializer(init_list, member);
    if (stmt == NULL &&
        InsertCXXAnonymousAggregateInitializers(syntax, func, init_list,
                                                member, body, insert_at)) {
      continue;
    }
    if (stmt == NULL && member_copy_source != NULL &&
        !TypeIsFixedArray(member->symbol->type)) {
      stmt = NewCXXCopiedMemberInitializer(syntax, func, member,
                                           member_copy_source, location);
    }
    if (stmt == NULL) {
      stmt = NewCXXDefaultMemberInitializerStatement(syntax, func, member);
    }
    if (stmt == NULL) {
      continue;
    }
    VectorInsertOrAppend(body, *insert_at, stmt);
    (*insert_at)++;
    InsertCXXMemberPartialCleanup(func, member, body, insert_at, location);
  }
}

// Inserts the implicit part of constructor `func`'s body at the start of
// `body`: the resolved mem-initializers, in construction order -- virtual
// bases (complete-object variant only), direct bases, __vptr setup, then data
// members -- with exception-only cleanup of each constructed subobject.  A
// delegating constructor gets only the delegated call.  Does nothing for a
// non-constructor or outside C++.
void SyntaxInsertCXXConstructorPreamble(Syntax* syntax, TypeRecord* func,
                                        Vector* body,
                                        CXXConstructorInitList* init_list,
                                        SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_constructor ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  SyntaxResolveCXXConstructorInitializerList(syntax, func, init_list);
  if (init_list->delegating_statement != NULL) {
    InsertCXXDelegatingConstructorCall(func, owner, body, init_list, location);
    return;
  }
  size_t insert_at = 0;
  Symbol* base_copy_source = CXXDefaultedCopyConstructorSource(func);
  Symbol* member_copy_source =
      func->info.function.cxx_special_member_kind ==
              kCXXSpecialMemberCopyConstructor
          ? base_copy_source
          : NULL;
  InsertCXXVirtualBaseInitializers(syntax, func, owner, init_list,
                                   base_copy_source, body, &insert_at,
                                   location);
  if (CompilerExceptionsEnabled() && owner->virtual_bases.length > 0) {
    InsertCXXVirtualBasePartialCleanup(syntax, func, owner, body, &insert_at,
                                       location);
  }
  InsertCXXDirectBaseInitializers(syntax, func, owner, init_list,
                                  base_copy_source, body, &insert_at,
                                  location);
  InsertCXXVPtrInitializers(func, owner, body, &insert_at, location);
  Vector* complete_restores = NewVector();
  AppendCXXVBPtrInitializers(func, complete_restores, location);
  InsertCXXCompleteObjectGuardedStatements(func, body, &insert_at,
                                           complete_restores, location);
  InsertCXXMemberInitializers(syntax, func, owner, init_list,
                              member_copy_source, body, &insert_at, location);
}
