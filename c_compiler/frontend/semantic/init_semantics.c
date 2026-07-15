//
//  init_semantics.c
//  c_compiler
//
//  Created by David Allison on 12/2/17.
//  Copyright © 2017 David Allison. All rights reserved.
//
#include <assert.h>

#include "init_semantics.h"
#include <stdlib.h>
#include "compiler.h"
#include "expr_semantics.h"
#include "list.h"

// The semantic analysis of an initializer converts the tree
// of initializers into a single braced initializer containing
// designated initializers only.  This is to simplify both
// semantic analysis and code generation.

// INodes are lazy initialized because arrays might be huge and we don't
// want to allocate a ton of memory for the indexes if they are never
// initialized.
// For example, this is common:
// int array[1000000] = {0};
//
// We don't want to allocate 1000000 INodes for the elements when only one
// is initialized.  We do this by doubling the number of children added
// to the array INode every time we run out of them.

typedef enum {
  kIScalar,      // Scalar with an initialization expression.
  kIArray,       // Array with inode for every element.
  kIStruct,      // Struct/union with inode for every member.
} IKind;

// Instance Node.
// These form an instance tree for all values that can be initialized.
typedef struct INode {
  IKind kind;
  TypeRecord* type;     // Not owned.
  ASTNode* expr;        // Not owned.
  struct INode* parent;
  struct INode* next;     // Not owned.
  struct INode* current;  // Not owned.
  Vector children;
  size_t index;             // Index into parent.
  bool is_base_subobject;
  int num_initializers;  // Number of initializers for this node.
} INode;

INode* BuildINode(TypeRecord* type, INode* parent);

static bool StructMemberIsObjectMember(StructMember* member) {
  return member != NULL && member->symbol != NULL && !member->is_static &&
         !member->is_member_function &&
         !StorageIs(member->symbol->storage, STO(typedef));
}

static bool StructInitializationTypesMatch(TypeRecord* expr_type,
                                           TypeRecord* target_type) {
  if (!TypeIsStructOrUnion(expr_type) || !TypeIsStructOrUnion(target_type) ||
      expr_type->info.struct_info == NULL ||
      target_type->info.struct_info == NULL) {
    return false;
  }
  if (TypeEqual(expr_type, target_type)) {
    return true;
  }
  TypeRecord* unqualified_expr = TypeRecordCopy(expr_type);
  TypeRecord* unqualified_target = TypeRecordCopy(target_type);
  unqualified_expr->qualifiers = kQualPlain;
  unqualified_target->qualifiers = kQualPlain;
  bool same_unqualified_type =
      TypeEqual(unqualified_expr, unqualified_target);
  TypeRecordDelete(unqualified_expr);
  TypeRecordDelete(unqualified_target);
  if (same_unqualified_type) {
    return true;
  }
  Struct* expr_struct = expr_type->info.struct_info;
  Struct* target_struct = target_type->info.struct_info;
  if (expr_struct == target_struct) {
    return true;
  }
  if (expr_struct->is_union != target_struct->is_union) {
    return false;
  }
  size_t expr_index = 0;
  size_t target_index = 0;
  bool matched_member = false;
  for (;;) {
    while (expr_index < expr_struct->members.length &&
           !StructMemberIsObjectMember(
               expr_struct->members.value.p[expr_index])) {
      expr_index++;
    }
    while (target_index < target_struct->members.length &&
           !StructMemberIsObjectMember(
               target_struct->members.value.p[target_index])) {
      target_index++;
    }
    if (expr_index == expr_struct->members.length ||
        target_index == target_struct->members.length) {
      return matched_member &&
             expr_index == expr_struct->members.length &&
             target_index == target_struct->members.length;
    }
    StructMember* expr_member = expr_struct->members.value.p[expr_index++];
    StructMember* target_member =
        target_struct->members.value.p[target_index++];
    if (!StringEqualString(&expr_member->symbol->name,
                           &target_member->symbol->name) ||
        (!TypeEqual(expr_member->symbol->type, target_member->symbol->type) &&
         !StructInitializationTypesMatch(expr_member->symbol->type,
                                         target_member->symbol->type))) {
      return false;
    }
    matched_member = true;
  }
}

static INode* NewINode(IKind kind, TypeRecord* type, INode* parent) {
  INode* inode = malloc(sizeof(INode));
  inode->kind = kind;
  inode->type = type;
  inode->expr = NULL;
  inode->parent = parent;
  inode->next = NULL;
  inode->current = NULL;
  inode->index = 0;
  inode->is_base_subobject = false;
  inode->num_initializers = 0;
  VectorInit(&inode->children);
  return inode;
}

// Append all struct members to INode.
static void AppendStructMembers(INode* inode) {
  if (inode->children.length > 0) {
    // Already done.
    return;
  }
  TypeRecord* type = inode->type;
  INode* prev = NULL;
  for (size_t i = 0; i < type->info.struct_info->bases.length; i++) {
    CXXBaseSpecifier* base = type->info.struct_info->bases.value.p[i];
    if (base == NULL || base->is_virtual || base->access != kAccessPublic ||
        base->type == NULL) {
      continue;
    }
    INode* child = BuildINode(base->type, inode);
    child->index = i;
    child->is_base_subobject = true;
    VectorAppend(&inode->children, child);
    if (!type->info.struct_info->is_union) {
      if (prev != NULL) {
        prev->next = child;
      }
      prev = child;
    }
  }
  size_t num_children = type->info.struct_info->members.length;
  for (size_t i = 0; i < num_children; i++) {
    StructMember* member = type->info.struct_info->members.value.p[i];
    if (!StructMemberIsObjectMember(member)) {
      continue;
    }
    INode* child = BuildINode(member->symbol->type, inode);
    child->index = i;
    VectorAppend(&inode->children, child);
    if (type->info.struct_info->is_union) {
      // All members of union don't have a next pointer.
    } else {
      // Struct members are chained together.
      if (prev != NULL) {
        prev->next = child;
      }
      prev = child;
    }
  }
  if (inode->children.length > 0) {
    // Current node is first child.
    inode->current = (INode*)inode->children.value.p[0];
  }
}

// Append more elements into the array INode.  Exponentially
// increase the size.
// for an array of 1000:
// 1: first = 0; last = 1
// 2: first = 1; last = 2
// 3: first = 2; last = 4
// 4: first = 4; last = 8
// 5: first = 8; last = 16
// 6: first = 16; last = 32
// 7: first = 32; last = 64
// 8: first = 64; last = 128
// 9: first = 128; last = 256
// 10: first = 256; last = 512
// 11: first = 512; last = 1024 (done)
static void AppendArrayINodeChildren(INode* inode, size_t min) {
  size_t first = inode->children.length;
  INode* prev = VectorLast(&inode->children);
  // Only append if we're out of nodes.
  if (inode->current != prev ||
      (!inode->type->info.array.is_flexible && first == inode->type->info.array.size.fixed)) {
    return;
  }
  // Exponentially increase the size by doubling it until it goes
  // beyond the length.
  size_t last = first == 0 ? 1 : first * 2;
  if (min > 0 && last <= min) {
    // Make sure we add the minimum amount.
    last = min + 1;
  }
  for (size_t i = first; i < last; i++) {
    INode* child = BuildINode(inode->type->next, inode);
    child->index = i;
    VectorAppend(&inode->children, child);
    if (prev != NULL) {
      prev->next = child;
    }
    prev = child;
  }
  if (first == 0) {
    // Current node is first child, but only if this is the first
    // time we add children.
    inode->current = (INode*)inode->children.value.p[0];
  }
}


INode* BuildINode(TypeRecord* type, INode* parent) {
  IKind kind = kIScalar;
  if (TypeIsStructOrUnion(type)) {
    kind = kIStruct;
  } else if (TypeIsArray(type)) {
    kind = kIArray;
  }
  return NewINode(kind, type, parent);
}

void LazyInitINode(INode* inode) {
  switch (inode->kind) {
    case kIArray:
      AppendArrayINodeChildren(inode, 0);
      break;
    case kIStruct:
      AppendStructMembers(inode);
      break;
    case kIScalar:
      break;
  }
}

static void DeleteINodeChild(INode* child) {
  VectorDestructWithContents(&child->children,
                            (VectorElementDestructor)DeleteINodeChild, /*free_element=*/true);

}

static void DeleteINode(INode* inode) {
  VectorDestructWithContents(&inode->children,
                           (VectorElementDestructor)DeleteINodeChild, /*free_element=*/true);
  free(inode);
}

static bool AdvanceCurrent(INode* inode) {
  if (inode == NULL) {
    return true;
  }
  if (inode->kind == kIArray) {
    // Append some of remaining children.
    AppendArrayINodeChildren(inode, 0);
  }
  if (inode->kind == kIScalar || inode->current->next == NULL) {
    AdvanceCurrent(inode->parent);
    return true;
  }
  inode->current = inode->current->next;
  return true;
}

static bool InitCurrentAndAdvance(INode* inode, ASTNode* expr, bool constants_only);
static bool InitializeINode(INode* inode, ASTNode* init_expr, bool constants_only);

static ASTNode* IdentityCloneNodeForInitializer(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static ASTNode* CloneInitializer(ASTNode* init) {
  if (init == NULL) {
    return NULL;
  }
  if (init->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)init;
    return NewExpressionInitializerASTNode(
        ASTNodeClone(expr_init->expr, IdentityCloneNodeForInitializer, NULL,
                     NULL),
        init->location);
  }
  return ASTNodeClone(init, IdentityCloneNodeForInitializer, NULL, NULL);
}

static void ApplyCXXDefaultMemberInitializers(INode* inode,
                                              bool constants_only) {
  if (!CompilerIsCXX() || inode == NULL) {
    return;
  }
  if (inode->kind != kIStruct) {
    for (size_t i = 0; i < inode->children.length; i++) {
      ApplyCXXDefaultMemberInitializers(inode->children.value.p[i],
                                        constants_only);
    }
    return;
  }
  LazyInitINode(inode);
  for (size_t i = 0; i < inode->children.length; i++) {
    INode* child = inode->children.value.p[i];
    if (child == NULL) {
      continue;
    }
    StructMember* member =
        child->is_base_subobject
            ? NULL
            : inode->type->info.struct_info->members.value.p[child->index];
    if (member != NULL && member->default_initializer != NULL &&
        child->expr == NULL && child->children.length == 0) {
      ASTNode* default_init = CloneInitializer(member->default_initializer);
      if (default_init != NULL) {
        InitializeINode(child, default_init, constants_only);
      }
    }
    ApplyCXXDefaultMemberInitializers(child, constants_only);
  }
}

static bool InitArrayAndAdvance(INode* inode, ASTNode* expr, bool constants_only) {
  if (expr->op == AST_OP(string)) {
    // Array initialized by string?
    if (TypeIsChar(inode->type->next)) {
      ConstantASTNode* c = (ConstantASTNode*)expr;
      if (!inode->type->info.array.is_flexible) {
        // Length with terminating zero.
        size_t string_length = c->value.string->length + 1;
        if (string_length > inode->type->info.array.size.fixed + 1) {
          SemanticError(expr,
                        "Too many initializers for character array");
        }
      } else {
        inode->type->info.array.is_flexible = false;
        inode->type->info.array.size.fixed = (int)c->value.string->length + 1;
        TypeRecordCalculateSize(inode->type);
      }
      inode->expr = ASTNodeMove(expr);
      expr->type->size = inode->type->size;
      return AdvanceCurrent(inode->parent);
    } else if (TypeIsInt(inode->type->next)) {
      SemanticError(expr,
                     "Initializing a wide-char array with a non-wide string literal");
    }
  }
  if (expr->op == AST_OP(string_wide)) {
    // Array initialized by wide string?
    if (TypeIsInt(inode->type->next)) {
      ConstantASTNode* c = (ConstantASTNode*)expr;
      if (!inode->type->info.array.is_flexible) {
        // Length with terminating zero.
        size_t string_length = c->value.string->length / sizeof(int);
        if (string_length > inode->type->info.array.size.fixed + 1) {
           SemanticError(expr,
                        "Too many initializers for wide character array");
         }
       } else {
         inode->type->info.array.is_flexible = false;
         inode->type->info.array.size.fixed = (int)(c->value.string->length / sizeof(int)) + 1;
         TypeRecordCalculateSize(inode->type);
       }
      inode->expr = ASTNodeMove(expr);
      expr->type->size = inode->type->size;
      return AdvanceCurrent(inode->parent);
    } else if (TypeIsChar(inode->type->next)) {
      SemanticError(expr,
                      "Initializing a char array with a wide string literal");
    }
  }
  // Lazy init of half of remaining array members.
  AppendArrayINodeChildren(inode, 0);
  return InitCurrentAndAdvance(inode->current, expr, constants_only);
}

static bool HasStaticAddress(ASTNode* expr) {
  if (TypeIsArray(expr->type) || TypeIsFunction(expr->type)) {
    return true;
  }
  if (expr->op == AST_OP(address)) {
    return true;
  }
  return false;
}

bool InitializerIsLinkTimeConstant(ASTNode* init) {
  if (init == NULL) {
    return true;
  }
  switch (init->op) {
    case AST_OP(braced_init): {
      BracedInitializerASTNode* braced = (BracedInitializerASTNode*)init;
      for (size_t i = 0; i < braced->initializers->length; i++) {
        if (!InitializerIsLinkTimeConstant(
                (ASTNode*)VectorGet(braced->initializers, i))) {
          return false;
        }
      }
      return true;
    }
    case AST_OP(designated_init):
      return InitializerIsLinkTimeConstant(
          ((DesignatedInitializerASTNode*)init)->init);
    case AST_OP(expr_init):
      return InitializerIsLinkTimeConstant(
          ((ExpressionInitializerASTNode*)init)->expr);
    default:
      return IsConstantExpression(init) || HasStaticAddress(init);
  }
}

// Initialize the current node and advance to the next.  Returns true
// if the initialization is valid.
static bool InitCurrentAndAdvance(INode* inode, ASTNode* expr, bool constants_only) {
  if (inode == NULL) {
    return false;
  }
  if (inode->expr != NULL) {
    return false;
  }
  // A compound literal initializing an aggregate is treated as if its
  // brace-enclosed initializer appeared directly here.  Its anonymous object is
  // not separately materialized, which also lets a compound literal serve as a
  // constant initializer for a static aggregate.
  if (expr->op == AST_OP(compound_literal) &&
      (inode->kind == kIStruct || inode->kind == kIArray) &&
      (TypeIsStructOrUnion(expr->type) || TypeIsArray(expr->type))) {
    CompoundLiteralASTNode* cl = (CompoundLiteralASTNode*)expr;
    return InitializeINode(inode, cl->initializer, constants_only);
  }
  expr = AnalyzeExpression(expr);
  switch (inode->kind) {
    case kIScalar:
      if (constants_only) {
        // Complile-time constants are constant expressions of anything
        // that can be done using a single relocation (something that
        // has a static address).
        if (!IsConstantExpression(expr) && !HasStaticAddress(expr)) {
          SemanticError(expr, "Expression is not a compile-time constant");
        }
      }
      inode->expr = ASTNodeMove(expr);
      assert(inode->expr->op != AST_OP(braced_init));
      return AdvanceCurrent(inode);
      
    case kIArray:
      return InitArrayAndAdvance(inode, expr, constants_only);
 
    case kIStruct:
      if (StructInitializationTypesMatch(expr->type, inode->type)) {
        // A struct/union can be initialized by an expression of the same
        // struct/union type (ignoring top-level qualifiers on the source).
        if (constants_only) {
          SemanticError(expr, "Expression is not a compile-time constant");
          return true;
        }
        inode->expr = ASTNodeMove(expr);
        return AdvanceCurrent(inode->parent);
      }
      // Lazy append of all struct members.
      AppendStructMembers(inode);
      return InitCurrentAndAdvance(inode->current, expr, constants_only);
  }
}

// Get an inode child given its index, or NULL if index is invalid.
static INode* GetChildAtIndex(INode* inode, size_t index, bool is_base) {
  for (size_t i = 0; i < inode->children.length; i++) {
    INode* child = inode->children.value.p[i];
    if (child->index == index && child->is_base_subobject == is_base) {
      return child;
    }
  }
  return NULL;
}

// Given a designator and an inode, find the child inode corresponding
// to the designator.
static INode* FindDesignator(INode* inode,
                             ASTNode* ast_node,
                             Designator* designator) {
  switch (designator->designator_type) {
    case kDesignatorArray: {
      if (inode->kind != kIArray) {
        SemanticError(ast_node, "Use of array designator on a non-array");
        return NULL;
      }
      AppendArrayINodeChildren(inode, designator->value.array_index);
      INode* element = GetChildAtIndex(inode,
                                       designator->value.array_index, false);
      if (element == NULL) {
        // Too few elements in array.
        SemanticError(ast_node,
                      "Array designator [%d] is outside bounds of the array",
                      designator->value.array_index);
        return NULL;
      }
      return element;
    }
      
    case kDesignatorStruct: {
      if (inode->kind != kIStruct) {
        SemanticError(ast_node, "Use of struct designator on a non-struct");
        return NULL;
      }
      AppendStructMembers(inode);
      StructMember* member =
          designator->is_resolved_member
              ? designator->value.struct_member
              : FindStructMember(inode->type->info.struct_info,
                                 designator->value.struct_member_name);
      if (member == NULL) {
        const char* name = designator->is_resolved_member
                               ? "<resolved>"
                               : designator->value.struct_member_name->value;
        SemanticError(ast_node, "Unknown struct member %s used in designator",
                      name);
        return NULL;
      }
      if (TypeIsArray(member->symbol->type) &&
          member->symbol->type->info.array.is_flexible) {
        SemanticError(ast_node, "Use of flexible array member in designator");
        return NULL;
      }
      designator->value.struct_member = member;
      designator->is_resolved_member = true;
      return GetChildAtIndex(inode, member->index, false);
    }

    case kDesignatorBase: {
      if (inode->kind != kIStruct) {
        SemanticError(ast_node, "Use of base designator on a non-struct");
        return NULL;
      }
      AppendStructMembers(inode);
      for (size_t i = 0; i < inode->type->info.struct_info->bases.length; i++) {
        CXXBaseSpecifier* base = inode->type->info.struct_info->bases.value.p[i];
        if (base == designator->value.base) {
          return GetChildAtIndex(inode, i, true);
        }
      }
      SemanticError(ast_node, "Unknown base class used in designator");
      return NULL;
    }
  }
  return NULL;
}

// Return the direct object member named by the first designator in a C++
// designated-initializer clause.  C++ declaration-order checking applies to
// this top-level field only, which is the only ordering case reachable in an
// otherwise-valid C++ designated-initializer list.
static StructMember* CXXDirectDesignatedMember(INode* inode,
                                               ASTNode* initializer,
                                               size_t* declaration_order) {
  if (!CompilerIsCXX() || inode == NULL || inode->kind != kIStruct ||
      initializer == NULL || initializer->op != AST_OP(designated_init) ||
      declaration_order == NULL) {
    return NULL;
  }
  DesignatedInitializerASTNode* designated =
      (DesignatedInitializerASTNode*)initializer;
  if (designated->designators == NULL ||
      designated->designators->length == 0) {
    return NULL;
  }
  Designator* first = designated->designators->value.p[0];
  if (first == NULL || first->designator_type != kDesignatorStruct) {
    return NULL;
  }

  Struct* owner = inode->type->info.struct_info;
  if (owner == NULL || !owner->is_aggregate) {
    return NULL;
  }
  StructMember* member =
      first->is_resolved_member
          ? first->value.struct_member
          : FindStructMember(owner, first->value.struct_member_name);
  if (!StructMemberIsObjectMember(member)) {
    return NULL;
  }

  // FindStructMember also searches base classes.  C++ designated initializers
  // name direct non-static data members, so inherited members do not
  // participate in this ordering check.  A member injected by an anonymous
  // aggregate has the declaration position of that aggregate.
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* direct = owner->members.value.p[i];
    if (direct == member) {
      *declaration_order = i;
      return member;
    }
    if (direct != NULL && direct->is_anon && direct->symbol != NULL &&
        TypeIsStructOrUnion(direct->symbol->type) &&
        FindStructMember(direct->symbol->type->info.struct_info,
                         &member->symbol->name) != NULL) {
      *declaration_order = i;
      return member;
    }
  }
  return NULL;
}

static String* CXXSingleDesignatorName(ASTNode* initializer) {
  if (initializer == NULL || initializer->op != AST_OP(designated_init)) {
    return NULL;
  }
  DesignatedInitializerASTNode* designated =
      (DesignatedInitializerASTNode*)initializer;
  if (designated->designators == NULL ||
      designated->designators->length != 1) {
    return NULL;
  }
  Designator* designator = designated->designators->value.p[0];
  if (designator == NULL ||
      designator->designator_type != kDesignatorStruct) {
    return NULL;
  }
  if (designator->is_resolved_member &&
      (designator->value.struct_member == NULL ||
       designator->value.struct_member->symbol == NULL)) {
    return NULL;
  }
  return designator->is_resolved_member
             ? &designator->value.struct_member->symbol->name
             : designator->value.struct_member_name;
}

static void CheckCXXDesignatedInitializers(
    INode* inode, BracedInitializerASTNode* braced_init) {
  if (!CompilerIsCXX() || inode == NULL || inode->kind != kIStruct ||
      braced_init == NULL || braced_init->initializers == NULL) {
    return;
  }

  StructMember* previous = NULL;
  size_t previous_order = 0;
  for (size_t i = 0; i < braced_init->initializers->length; i++) {
    ASTNode* initializer = braced_init->initializers->value.p[i];
    String* designator_name = CXXSingleDesignatorName(initializer);
    if (designator_name != NULL) {
      for (size_t j = 0; j < i; j++) {
        String* prior_name = CXXSingleDesignatorName(
            braced_init->initializers->value.p[j]);
        if (prior_name != NULL &&
            StringEqualString(designator_name, prior_name)) {
          SemanticError(
              initializer,
              "'.%s' designator used multiple times in the same initializer "
              "list",
              designator_name->value);
          break;
        }
      }
    }

    size_t declaration_order = 0;
    StructMember* member =
        CXXDirectDesignatedMember(inode, initializer, &declaration_order);
    if (member == NULL) {
      continue;
    }
    if (previous != NULL && declaration_order < previous_order) {
      Struct* owner = inode->type->info.struct_info;
      SemanticError(
          initializer,
          "designator order for field '%s' does not match declaration order "
          "in '%s'",
          member->symbol->name.value,
          owner->tag_name != NULL ? owner->tag_name->value : "<anonymous>");
    }
    previous = member;
    previous_order = declaration_order;
  }
}

// If a flexible array (without a size) is initialized with a braced
// initalizer we calculate its size from the number of initializers
// inside the braces.  If there are designated initializers in there
// we need to use the maximum value of the array index to set the
// current size and keep going.
static COMPILER_UNUSED int GetArraySizeFromInitializer(BracedInitializerASTNode* braced_init) {
  int size = 0;
  for (size_t i = 0; i < braced_init->initializers->length; i++) {
    ASTNode* init = braced_init->initializers->value.p[i];
    if (init->op == AST_OP(designated_init)) {
      DesignatedInitializerASTNode* d = (DesignatedInitializerASTNode*)init;
      // First designator.
      Designator* designator = d->designators->value.p[0];
      if (designator->designator_type == kDesignatorArray) {
        int size_from_designator = designator->value.array_index + 1;
        if (size < size_from_designator) {
          size = size_from_designator;
        }
      }
    } else {
      size++;
    }
  }
  return size;
}


static bool InitializeINode(INode* inode, ASTNode* init_expr, bool constants_only) {
  switch (init_expr->op) {
    case AST_OP(expr_init): {
      inode->num_initializers++;
      ExpressionInitializerASTNode* expr_init = (ExpressionInitializerASTNode*)init_expr;
      return InitCurrentAndAdvance(inode, expr_init->expr, constants_only);
    }
    case AST_OP(braced_init): {
      BracedInitializerASTNode* braced_init = (BracedInitializerASTNode*)init_expr;
      LazyInitINode(inode);
      CheckCXXDesignatedInitializers(inode, braced_init);
      INode* parent = inode->parent;
      inode->parent = NULL;
      for (size_t i = 0; i < braced_init->initializers->length; i++) {
        INode* current = inode->current == NULL ? inode : inode->current;
        if (!InitializeINode(current, braced_init->initializers->value.p[i], constants_only)) {
          SemanticError((ASTNode*)braced_init->initializers->value.p[i],
                        "Too many initializers");
          break;
        }
      }
      if (inode->kind == kIArray && inode->type->info.array.is_flexible) {
        // "Flexible" array (without size).  We can set it now and add all
        // nodes for its children now that we know the size.  This only
        // occurs at the top level and never inside a struct.
        // We look for the last child with an initializer.  There can be gaps
        // if designated initializers are used.
        for (ssize_t i = inode->children.length - 1; i >= 0; i--) {
          INode* child = inode->children.value.p[i];
          if (child->num_initializers > 0) {
            inode->num_initializers = (int)i + 1;
            break;
          }
        }
        inode->type->info.array.size.fixed = inode->num_initializers;
        inode->type->info.array.is_flexible = false;
        TypeRecordCalculateSize(inode->type);
      }
      inode->parent = parent;
      return AdvanceCurrent(parent);
    }
      
    case AST_OP(designated_init): {
      DesignatedInitializerASTNode* designated_init = (DesignatedInitializerASTNode*)init_expr;
      INode* designated_node = inode->parent != NULL ? inode->parent : inode;
      LazyInitINode(inode);
      for (size_t i = 0; i < designated_init->designators->length; i++) {
        designated_node = FindDesignator(designated_node, init_expr, designated_init->designators->value.p[i]);
        if (designated_node == NULL) {
          return false;
        }
      }
      // Set designated node as parent's current.
      if (designated_node->parent != NULL) {
        designated_node->parent->current = designated_node;
      }
      // A designated initializer may overwrite a value set by an earlier
      // initializer (the last assignment wins), e.g. with overlapping
      // [start ... end] range designators.  Clear any existing value so the
      // re-initialization is not rejected as "too many initializers".
      designated_node->expr = NULL;
      switch (designated_init->init->op) {
        case AST_OP(expr_init):
        case AST_OP(braced_init):
        case AST_OP(designated_init):
          return InitializeINode(designated_node, designated_init->init,
                                 constants_only);
        default:
          return InitCurrentAndAdvance(designated_node, designated_init->init,
                                       constants_only);
      }
    }
    default:
      // Some scalar expressions, including pointer-to-member constants, are
      // stored directly in a braced initializer rather than wrapped in an
      // expr_init node.  They still initialize the current scalar subobject.
      return InitCurrentAndAdvance(inode, init_expr, constants_only);
  }
}

static void BuildSingleDesignator(INode* inode, IKind kind,
                                  TypeRecord* type,
                                  Vector* designators) {
  switch (kind) {
    case kIArray:
      VectorAppend(designators,
                   NewArrayDesignator(inode->type, (int)inode->index));
      break;
    case kIStruct:
      {
        if (inode->is_base_subobject) {
          CXXBaseSpecifier* base =
              type->info.struct_info->bases.value.p[inode->index];
          VectorAppend(designators, NewCXXBaseDesignator(base));
        } else if (type->info.struct_info != NULL &&
                   inode->index < type->info.struct_info->members.length) {
          StructMember* member =
              type->info.struct_info->members.value.p[inode->index];
          VectorAppend(designators,
                       NewStructMemberDesignator(member));
        }
        // Otherwise there is no member to designate (e.g. a whole-object copy
        // into an empty struct); leave the designator list empty so the object
        // is initialized directly rather than dereferencing a missing member.
      }
      break;

    case kIScalar:
      break;
  }
}

static void BuildDesignator(INode* inode, Vector* designators) {
  if (inode->parent == NULL) {
    return;
  }
  BuildDesignator(inode->parent, designators);
  BuildSingleDesignator(inode,
                        inode->parent->kind,
                        inode->parent->type,
                        designators);
}

static ASTNode* BuildDesignatedInitializer(INode* inode) {
  Vector* designators = NewVector();
  if (inode->parent == NULL) {
    // Top level: this INode *is* the object being initialized, so the
    // designator path from the object root to it is empty.  (A whole-object
    // struct copy lands here; emitting `members[0]`'s designator would offset
    // the destination by that member's byte offset -- harmless when it is 0 but
    // wrong for a class with a base, whose first declared member sits after the
    // base subobject.)  BuildSingleDesignator is only meaningful for describing
    // a subobject's position *within its parent*, which the recursive
    // BuildDesignator handles for non-top-level nodes below.
  } else {
    BuildDesignator(inode, designators);
  }
  inode->expr = AnalyzeExpression(inode->expr);
  ASTNode* designated_init = NewDesignatedInitializerASTNode(designators,
                                                inode->expr,
                                                inode->expr->location);
  if (TypeIsReference(inode->type)) {
    NormalConversion(inode->expr, inode->type->next);
    inode->expr->flags |= kASTNeedAddress;
  } else {
    NormalConversion(inode->expr, inode->type);
  }
  ASTNodeSetType(designated_init, inode->type);
  return designated_init;
}

static void FlattenINode(INode* inode,
                            BracedInitializerASTNode* braced_init) {
  if (inode->expr != NULL) {
     VectorAppend(braced_init->initializers,
                  BuildDesignatedInitializer(inode));
  }
  for (size_t i = 0; i < inode->children.length; i++) {
    FlattenINode(inode->children.value.p[i], braced_init);
  }
}

ASTNode* AnalyzeInitializer(TypeRecord* type, ASTNode* ast_node, bool constants_only) {
  INode* inode = BuildINode(type, NULL);
  InitializeINode(inode, ast_node, constants_only);
  ApplyCXXDefaultMemberInitializers(inode, constants_only);
  // PrintINode(inode, 0);
  ASTNode* braced_init = NewBracedInitializerASTNode(NewVector(), type, ast_node->location);
  FlattenINode(inode, (BracedInitializerASTNode*)braced_init);
  // ASTNodePrint(braced_init, 0);
  DeleteINode(inode);
  return braced_init;
}

