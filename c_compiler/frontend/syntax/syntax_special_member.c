//
//  syntax_special_member.c
//  c_compiler
//
//  Calls to base and member special member functions (constructors,
//  destructors, assignments) and defaulted memberwise bodies.
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

// For an old-style C function we have the types for the arguments specified
// in the definition, before the body.  The previous formal arguments need
// to be resolved by the new formals with types.
void ResolveOldStyleFormalArgument(Syntax* syntax, TypeRecord* func, Symbol* formal) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* prev_formal = (Symbol*)formals->value.p[i];
    if (StringEqualString(&prev_formal->name, &formal->name)) {
      formals->value.p[i] = formal;
      formal->flags.is_argument = true;
      formal->value.arg_number = prev_formal->value.arg_number;
      SymbolDelete(prev_formal);
      return;
    }
  }
  SyntaxError(syntax, "No such function parameter %s", formal->name.value);
}

// Inserts each formal parameter of `func` into `syntax`'s local symbol stack
// as a block-scope name for the function body.
void AddFunctionScopeSymbols(Syntax* syntax, TypeRecord* func) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = formals->value.p[i];
    formal->flags.is_block_scope = true;
    InsertLocalSymbol(syntax->local_symbol_stack, formal);
  }
}

// Returns the constructor or destructor member of base class `base` (named
// after the base tag, or `~Tag` when `destructor` is true), or NULL when
// the base is not a class or has no matching special member.
static StructMember* FindCXXBaseSpecialMember(CXXBaseSpecifier* base,
                                              bool destructor) {
  if (base == NULL || base->type == NULL || !TypeIsStructOrUnion(base->type) ||
      base->type->info.struct_info == NULL ||
      base->type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String member_name;
  if (destructor) {
    StringInit(&member_name, "~");
    StringAppendString(&member_name, base->type->info.struct_info->tag_name);
  } else {
    StringInit(&member_name, base->type->info.struct_info->tag_name->value);
  }
  StructMember* member =
      FindStructMember(base->type->info.struct_info, &member_name);
  StringDestruct(&member_name);
  if (member == NULL || !member->is_member_function) {
    return NULL;
  }
  TypeRecord* func = member->symbol->type;
  if (!TypeIsFunction(func)) {
    return NULL;
  }
  if (destructor && !func->info.function.is_destructor) {
    return NULL;
  }
  if (!destructor && !func->info.function.is_constructor) {
    return NULL;
  }
  return member;
}

// Returns true when calls into `type` need the hidden complete-object
// `int` argument (class types with virtual bases).
bool TypeNeedsCXXCompleteObjectArgument(TypeRecord* type) {
  return TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
         StructHasVirtualBases(type->info.struct_info);
}

// Builds the hidden `int` actual (1 or 0) passed for complete-object
// construction/destruction of types with virtual bases.
static ASTNode* NewCXXCompleteObjectArgument(bool complete_object,
                                            SourceLocation location) {
  return NewIntConstantASTNode(complete_object ? 1 : 0,
                               NewTypeRecordWithSize(kTypeInt, kQualPlain),
                               location);
}

// Prepends the complete-object flag to `actuals` when `type` has virtual
// bases; no-op when `actuals` is NULL or the flag is not required.
void CXXPrependCompleteObjectArgument(TypeRecord* type, Vector* actuals,
                                      bool complete_object,
                                      SourceLocation location) {
  if (actuals == NULL || !TypeNeedsCXXCompleteObjectArgument(type)) {
    return;
  }
  ASTNode* arg = NewCXXCompleteObjectArgument(complete_object, location);
  if (actuals->length == 0) {
    VectorAppend(actuals, arg);
  } else {
    VectorInsertBefore(actuals, 0, arg);
  }
}

// Builds an lvalue expression naming the base subobject of `source` (the
// `other` reference parameter of a defaulted copy/move special member) as
// `*(B*)((char*)&source + byte_offset)`.  The nodes are emitted pre-analyzed
// with forced types so the later analysis pass does not re-run a derived-to-base
// conversion on them.  This is important because that conversion is access
// checked and would reject a *private/protected* base (e.g.
// `struct D : private B {}`), even though a class may always access its own base
// subobjects from within its special members.  Using the raw offset mirrors how
// the `this` receiver already reaches the base subobject and sidesteps the check
// entirely.
ASTNode* NewCXXSourceBaseSubobject(Symbol* source,
                                   CXXBaseSpecifier* base,
                                   SourceLocation location) {
  if (source == NULL || source->type == NULL || base == NULL ||
      base->type == NULL) {
    return NULL;
  }
  if (!TypeIsReference(source->type)) {
    return NULL;
  }
  TypeRecord* referent = source->type->next;
  TypeRecord* source_pointer =
      NewPointerTo(kQualPlain, TypeRecordCopy(referent));
  // Reading the reference parameter yields the underlying pointer to the
  // referent (references are lowered to pointers), so treat the identifier as a
  // `owner*` directly -- exactly as the `this` pointer is used for the receiver.
  // kASTNeedAddress tells codegen to load the pointer held by the reference
  // parameter without then loading the referenced object.  The latter would
  // pass the source object's first word as the base address.
  ASTNode* source_ptr = NewIdentifierASTNode(source, location);
  source_ptr->flags |= kASTAnalyzed | kASTNeedAddress;
  ASTNodeSetType(source_ptr, source_pointer);
  TypeRecord* base_pointer =
      NewPointerTo(kQualPlain, TypeRecordCopy(base->type));
  ASTNode* offset = NewIntConstantASTNode(
      base->byte_offset, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  ASTNode* base_ptr =
      NewBinaryASTNode(AST_OP(plus), base_pointer, location, source_ptr, offset);
  base_ptr->flags |= kASTAnalyzed;
  ASTNodeSetType(base_ptr, base_pointer);
  ASTNode* base_ref = NewUnaryASTNode(AST_OP(contents),
                                      TypeRecordCopy(base->type), location,
                                      base_ptr);
  base_ref->flags |= kASTAnalyzed;
  base_ref->value_category =
      source->type->declarator == kDeclRValueReference
          ? kValueCategoryXvalue
          : kValueCategoryLvalue;
  ASTNodeSetType(base_ref, TypeRecordCopy(base->type));
  return base_ref;
}

// Builds an expression statement that calls a base constructor or destructor
// on a subobject of `this`, using `actuals` as arguments (ownership
// transferred to the call, or an empty vector created).  Returns NULL and
// frees `actuals` when the base member or `this` is unavailable.
ASTNode* NewCXXBaseSpecialMemberCall(Syntax* syntax,
                                     TypeRecord* receiver_func,
                                     CXXBaseSpecifier* base,
                                     bool destructor,
                                     bool complete_object,
                                     Vector* actuals,
                                     SourceLocation location) {
  StructMember* member = FindCXXBaseSpecialMember(base, destructor);
  Symbol* this_symbol = FindThisSymbol(syntax);
  if (this_symbol == NULL && receiver_func != NULL &&
      TypeIsFunction(receiver_func) &&
      receiver_func->info.function.prototype.length > 0) {
    this_symbol = receiver_func->info.function.prototype.value.p[0];
  }
  if (member == NULL || this_symbol == NULL) {
    if (actuals != NULL) {
      VectorDelete(actuals);
    }
    return NULL;
  }
  if (actuals == NULL) {
    actuals = NewVector();
  }
  CXXPrependCompleteObjectArgument(base->type, actuals, complete_object,
                                   location);

  String member_name;
  if (destructor) {
    StringInit(&member_name, "~");
    StringAppendString(&member_name, base->type->info.struct_info->tag_name);
  } else {
    StringInit(&member_name, base->type->info.struct_info->tag_name->value);
  }
  ASTNode* receiver = NewIdentifierASTNode(this_symbol, location);
  TypeRecord* base_pointer =
      NewPointerTo(kQualPlain, TypeRecordCopy(base->type));
  if (base->byte_offset == 0 && !base->is_virtual) {
    // Member lookup must start from the base subobject even when no address
    // adjustment is required. Virtual-base special-member calls retain the
    // complete-object receiver used by their hidden construction/destruction
    // protocol.  Keep the forced type through template-body clone: clearing
    // kASTAnalyzed would re-type `this` as the derived class, and a
    // `class D : private B` mem-init would then fail to convert `D*` to `B*`
    // (public-only derived-to-base) and select only `B()` ("need 1, got N").
    receiver->flags |= kASTAnalyzed | kASTForcedTypeAdjustment;
    ASTNodeSetType(receiver, base_pointer);
  } else if (base->byte_offset != 0) {
    ASTNode* offset =
        NewIntConstantASTNode(base->byte_offset,
                              NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location);
    receiver = NewBinaryASTNode(AST_OP(plus), base_pointer, location,
                                receiver, offset);
    // base->byte_offset is already measured in bytes.  Preserve this forced
    // adjustment when a template special member body is cloned; re-analyzing
    // it as ordinary pointer arithmetic would scale the offset by sizeof(*this).
    receiver->flags |= kASTAnalyzed | kASTForcedTypeAdjustment;
    ASTNodeSetType(receiver, base_pointer);
  }
  ASTNode* member_node =
      NewStringConstantASTNode(NewString(member_name.value), NULL, location);
  StringDestruct(&member_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver, member_node);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access,
                       actuals);
  return NewExpressionStatementASTNode(call, location);
}

// Like NewCXXBaseSpecialMemberCall for virtual base `vbase`, with
// complete-object false for the hidden construction protocol.
ASTNode* NewCXXVirtualBaseSpecialMemberCall(Syntax* syntax,
                                            TypeRecord* receiver_func,
                                            CXXVirtualBaseInfo* vbase,
                                            bool destructor,
                                            Vector* actuals,
                                            SourceLocation location) {
  if (vbase == NULL) {
    if (actuals != NULL) {
      VectorDelete(actuals);
    }
    return NULL;
  }
  CXXBaseSpecifier base;
  base.type = vbase->type;
  base.access = vbase->access;
  base.byte_offset = vbase->byte_offset;
  base.is_virtual = true;
  return NewCXXBaseSpecialMemberCall(syntax, receiver_func, &base, destructor,
                                     /*complete_object=*/false, actuals,
                                     location);
}

// Appends destructor calls for each direct and virtual base of the class
// whose destructor is `func`, in reverse declaration order, to `body`.
void AppendCXXBaseDestructorCalls(Syntax* syntax, TypeRecord* func,
                                  Vector* body, SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_destructor ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = owner->bases.length; i > 0; i--) {
    CXXBaseSpecifier* base = owner->bases.value.p[i - 1];
    if (base->is_virtual) {
      continue;
    }
    ASTNode* call = NewCXXBaseSpecialMemberCall(syntax, func, base, true,
                                                /*complete_object=*/false,
                                                NULL, location);
    if (call == NULL) {
      continue;
    }
    VectorAppend(body, call);
  }
  Vector* virtual_base_destructors = NewVector();
  for (size_t i = owner->virtual_bases.length; i > 0; i--) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i - 1];
    ASTNode* call = NewCXXVirtualBaseSpecialMemberCall(syntax, func, base,
                                                       true, NULL, location);
    if (call == NULL) {
      continue;
    }
    VectorAppend(virtual_base_destructors, call);
  }
  ASTNode* guarded = NewCXXCompleteObjectGuardedStatement(
      func, virtual_base_destructors, location);
  if (guarded != NULL) {
    VectorAppend(body, guarded);
  }
}

// Emits the base-subobject copy/move assignments for a defaulted copy/move
// assignment operator.  The memberwise-copy helper only assigns the class's own
// members; a derived class must additionally forward to each direct base's
// assignment operator so the base subobject is assigned rather than left
// untouched.  The base assignment is expressed as
// `(B*)(this + offset)->operator=(*(B*)(&other + offset))`: reinterpreting
// `this` as `B*` (via a type-forced, already-analyzed pointer adjustment) makes
// name lookup resolve the *base's* operator= instead of the enclosing class's,
// and the argument is the source's base subobject extracted by the same offset
// trick (see NewCXXSourceBaseSubobject) so no access-checked derived-to-base
// conversion is needed -- which matters for private/protected bases.
void AppendCXXBaseAssignments(Syntax* syntax, TypeRecord* func, Vector* body,
                              SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL ||
      func->info.function.prototype.length < 2) {
    return;
  }
  CXXSpecialMemberKind kind = func->info.function.cxx_special_member_kind;
  if (kind != kCXXSpecialMemberCopyAssignment &&
      kind != kCXXSpecialMemberMoveAssignment) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  Symbol* source =
      func->info.function.prototype.value
          .p[func->info.function.prototype.length - 1];
  if (this_symbol == NULL || source == NULL) {
    return;
  }
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base == NULL || base->is_virtual || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    TypeRecord* base_pointer =
        NewPointerTo(kQualPlain, TypeRecordCopy(base->type));
    ASTNode* offset = NewIntConstantASTNode(
        base->byte_offset, NewTypeRecordWithSize(kTypeInt, kQualPlain),
        location);
    ASTNode* receiver =
        NewBinaryASTNode(AST_OP(plus), base_pointer, location,
                         NewIdentifierASTNode(this_symbol, location), offset);
    receiver->flags |= kASTAnalyzed;
    ASTNodeSetType(receiver, base_pointer);
    ASTNode* source_base = NewCXXSourceBaseSubobject(source, base, location);
    if (source_base == NULL) {
      continue;
    }
    ASTNode* member_node =
        NewStringConstantASTNode(NewString("operator="), NULL, location);
    ASTNode* member_access =
        NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver, member_node);
    Vector* actuals = NewVector();
    VectorAppend(actuals, source_base);
    ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, location,
                                     member_access, actuals);
    VectorAppend(body, NewExpressionStatementASTNode(call, location));
  }
}

// Returns the user-declared destructor member for class `type`, or NULL
// when `type` is not a named class or has no destructor.
static StructMember* FindCXXDestructorForObjectType(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL || type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(type->info.struct_info, &destructor_name);
  StringDestruct(&destructor_name);
  if (destructor == NULL || !destructor->is_member_function ||
      destructor->symbol == NULL || destructor->symbol->type == NULL ||
      !destructor->symbol->type->info.function.is_destructor) {
    return NULL;
  }
  return destructor;
}

// Returns the class type whose destructor should run for storage `type`
// (the array element type when `type` is an array of destructible classes),
// or NULL when nothing needs destruction.
static TypeRecord* CXXDestructibleElementType(TypeRecord* type) {
  if (TypeIsFixedArray(type) && type->next != NULL &&
      FindCXXDestructorForObjectType(type->next) != NULL) {
    return type->next;
  }
  if (FindCXXDestructorForObjectType(type) != NULL) {
    return type;
  }
  return NULL;
}

// Builds a statement that destroys the subobject named by `receiver` using
// `object_type`'s destructor; takes ownership of `receiver` and deletes it
// on failure.  Returns NULL when prerequisites are missing.
ASTNode* NewCXXMemberDestructorCall(Syntax* syntax, TypeRecord* func,
                                    StructMember* member,
                                    TypeRecord* object_type,
                                    ASTNode* receiver,
                                    SourceLocation location) {
  (void)syntax;
  if (func == NULL || func->info.function.prototype.length == 0 ||
      member == NULL || object_type == NULL ||
      object_type->info.struct_info == NULL ||
      object_type->info.struct_info->tag_name == NULL) {
    ASTNodeDelete(receiver);
    return NULL;
  }
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(object_type, actuals,
                                   /*complete_object=*/true, location);
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, object_type->info.struct_info->tag_name);
  ASTNode* destructor =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL, location);
  StringDestruct(&destructor_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, destructor);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  return NewExpressionStatementASTNode(call, location);
}

// Builds `this->member_name` as an lvalue for data member `member` of the
// class whose member function type is `func`, or NULL when `this` is missing.
static ASTNode* NewCXXMemberReceiver(TypeRecord* func, StructMember* member,
                                     SourceLocation location) {
  if (func == NULL || func->info.function.prototype.length == 0 ||
      member == NULL || member->symbol == NULL) {
    return NULL;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  ASTNode* this_node = NewIdentifierASTNode(this_symbol, location);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(member->symbol->name.value), NULL,
                               location);
  return NewBinaryASTNode(AST_OP(arrow), NULL, location, this_node,
                          member_name);
}

// Appends destructor calls for each non-static data member of the class
// whose destructor is `func`, in reverse member order, to `body`.
void SyntaxAppendCXXMemberDestructorCalls(Syntax* syntax, TypeRecord* func,
                                           Vector* body,
                                           SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_destructor ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = owner->members.length; i > 0; i--) {
    StructMember* member = owner->members.value.p[i - 1];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || StructMemberIsNestedType(member)) {
      // Nested classes and typedefs participate in name lookup but occupy no
      // storage, so they have nothing to destroy.
      continue;
    }
    TypeRecord* member_type = member->symbol->type;
    TypeRecord* object_type = CXXDestructibleElementType(member_type);
    if (object_type == NULL) {
      continue;
    }
    if (TypeIsFixedArray(member_type)) {
      for (size_t index = member_type->info.array.size.fixed; index > 0;
           index--) {
        ASTNode* receiver = NewCXXMemberReceiver(func, member, location);
        ASTNode* index_node = NewIntConstantASTNode(
            (int64_t)index - 1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
            location);
        receiver = NewBinaryASTNode(AST_OP(subscript), NULL, location,
                                    receiver, index_node);
        ASTNode* call = NewCXXMemberDestructorCall(
            syntax, func, member, object_type, receiver, location);
        if (call != NULL) {
          VectorAppend(body, call);
        }
      }
    } else {
      ASTNode* receiver = NewCXXMemberReceiver(func, member, location);
      ASTNode* call = NewCXXMemberDestructorCall(
          syntax, func, member, object_type, receiver, location);
      if (call != NULL) {
        VectorAppend(body, call);
      }
    }
  }
}

// Allocates a deferred mem-initializer entry naming base or member `name`
// with argument ASTs `actuals` (caller transfers ownership of `actuals`).
CXXDeferredConstructorInitializer* NewCXXDeferredConstructorInitializer(
    const char* name, Vector* actuals, SourceLocation location,
    bool is_pack_expansion) {
  CXXDeferredConstructorInitializer* init =
      malloc(sizeof(CXXDeferredConstructorInitializer));
  StringInit(&init->name, name);
  init->actuals = actuals;
  init->location = location;
  init->is_pack_expansion = is_pack_expansion;
  return init;
}

// Returns a new vector of clones of each AST in `actuals` (empty when
// `actuals` is NULL); caller owns the vector and its nodes.
Vector* CloneCXXConstructorInitializerActuals(Vector* actuals) {
  Vector* clone = NewVector();
  if (actuals == NULL) {
    return clone;
  }
  for (size_t i = 0; i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    VectorAppend(clone, ASTNodeClone(actual, IdentityCloneNode, NULL, NULL));
  }
  return clone;
}

// Deep-copies deferred mem-initializer `init`, or returns NULL when `init`
// is NULL.
CXXDeferredConstructorInitializer* CloneCXXDeferredConstructorInitializer(
    CXXDeferredConstructorInitializer* init) {
  if (init == NULL) {
    return NULL;
  }
  return NewCXXDeferredConstructorInitializer(
      init->name.value, CloneCXXConstructorInitializerActuals(init->actuals),
      init->location, init->is_pack_expansion);
}

// Frees `init`, its name string, and its `actuals` vector.
void CXXDeferredConstructorInitializerDelete(
    CXXDeferredConstructorInitializer* init) {
  if (init == NULL) {
    return;
  }
  StringDestruct(&init->name);
  if (init->actuals != NULL) {
    VectorDelete(init->actuals);
  }
  free(init);
}

// Returns true when `func` is a defaulted copy/move constructor or
// assignment operator that gets a compiler-generated memberwise body.
static bool CXXFunctionNeedsMemberwiseDefaultedBody(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return false;
  }
  switch (func->info.function.cxx_special_member_kind) {
    case kCXXSpecialMemberCopyConstructor:
    case kCXXSpecialMemberMoveConstructor:
    case kCXXSpecialMemberCopyAssignment:
    case kCXXSpecialMemberMoveAssignment:
      return true;
    default:
      return false;
  }
}

// Appends memberwise copy/move assignments or constructor member-init
// statements for each eligible data member of the class owning `func` to
// `body`.
void AppendCXXDefaultedMemberwiseAssignments(TypeRecord* func,
                                             Vector* body,
                                             SourceLocation location) {
  if (!CXXFunctionNeedsMemberwiseDefaultedBody(func) ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL) {
      formal->value.arg_number = (int32_t)i;
    }
  }
  Symbol* source = CXXSourceObjectParameter(func);
  if (source == NULL) {
    return;
  }
  bool is_constructor_initializer =
      func->info.function.cxx_special_member_kind ==
      kCXXSpecialMemberCopyConstructor;
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    TypeRecord* member_type = member->symbol->type;
    if (is_constructor_initializer && !TypeIsFixedArray(member_type)) {
      continue;
    }
    if (TypeIsFixedArray(member_type)) {
      for (size_t index = 0; index < member_type->info.array.size.fixed;
           index++) {
        ASTNode* target =
            NewCXXThisMemberAccess(func, member->symbol->name.value, location);
        ASTNode* value =
            NewCXXSourceMemberAccess(source, member->symbol->name.value,
                                     location);
        target = NewBinaryASTNode(
            AST_OP(subscript), NULL, location, target,
            NewIntConstantASTNode((int64_t)index,
                                  NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                  location));
        value = NewBinaryASTNode(
            AST_OP(subscript), NULL, location, value,
            NewIntConstantASTNode((int64_t)index,
                                  NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                  location));
        ASTNode* assign = NewBinaryASTNode(AST_OP(assign), member_type->next,
                                           location, target, value);
        if (is_constructor_initializer) {
          assign->flags |= kASTCXXMemberInitializer;
        }
        VectorAppend(body, NewExpressionStatementASTNode(assign, location));
      }
    } else {
      ASTNode* target =
          NewCXXThisMemberAccess(func, member->symbol->name.value, location);
      ASTNode* value =
          NewCXXSourceMemberAccess(source, member->symbol->name.value,
                                   location);
      if (is_constructor_initializer && TypeIsStructOrUnion(member_type)) {
        Vector* actuals = NewVector();
        VectorAppend(actuals, value);
        ASTNode* init = SyntaxNewCXXMemberInitializerStatement(
            &compiler->syntax, func, member, actuals, location);
        if (init != NULL) {
          VectorAppend(body, init);
        }
        continue;
      }
      ASTNode* assign =
          NewBinaryASTNode(AST_OP(assign), member_type, location, target, value);
      if (is_constructor_initializer) {
        assign->flags |= kASTCXXMemberInitializer;
      }
      VectorAppend(body, NewExpressionStatementASTNode(assign, location));
    }
  }
}

// Appends `return *this;` to `body` for a defaulted copy or move assignment
// operator `func`.
void AppendCXXDefaultedAssignmentReturnThis(TypeRecord* func,
                                            Vector* body,
                                            SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) || func->next == NULL ||
      TypeIsVoid(func->next) || func->info.function.prototype.length == 0 ||
      (func->info.function.cxx_special_member_kind !=
           kCXXSpecialMemberCopyAssignment &&
       func->info.function.cxx_special_member_kind !=
           kCXXSpecialMemberMoveAssignment)) {
    return;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  TypeRecord* object_type =
      TypeIsPointer(this_symbol->type) ? this_symbol->type->next : NULL;
  ASTNode* object =
      NewUnaryASTNode(AST_OP(contents), object_type, location,
                      NewIdentifierASTNode(this_symbol, location));
  VectorAppend(body,
               NewCombinedStatementASTNode(AST_OP(return), object, NULL,
                                           location));
}
