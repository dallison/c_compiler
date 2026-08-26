//
//  init_semantics.c
//  c_compiler
//
//  Created by David Allison on 12/2/17.
//  Copyright © 2017 David Allison. All rights reserved.
//
#include <assert.h>
#include <limits.h>

#include "init_semantics.h"
#include <stdlib.h>
#include "compiler.h"
#include "constexpr.h"
#include "expr_evaluator.h"
#include "expr_semantics.h"
#include "list.h"
#include "syntax.h"

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
  if ((min == 0 && inode->current != prev) ||
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
  if (!inode->type->info.array.is_flexible &&
      last > inode->type->info.array.size.fixed) {
    last = inode->type->info.array.size.fixed;
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
  // The whole (sub)object is initialized by a single expression, e.g. copy-
  // initialization from another object of the same class, or a call whose
  // return value is constructed directly into this storage.  That expression
  // supplies every member, so applying the class's default member initializers
  // here would emit them *after* the initializing expression and overwrite it.
  if (inode->expr != NULL) {
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
  if (expr->op == AST_OP(string) || expr->op == AST_OP(string_wide)) {
    TypeRecord* literal_element =
        TypeIsArray(expr->type) ? expr->type->next : NULL;
    bool character_array = TypeIsCharFamily(inode->type->next) ||
                           TypeIsInt(inode->type->next);
    if (literal_element == NULL || !character_array ||
        inode->type->next->type != literal_element->type) {
      SemanticError(expr,
                    "String literal encoding does not match character array "
                    "element type");
    } else {
      ConstantASTNode* literal = (ConstantASTNode*)expr;
      TypeRecordCalculateSize(literal_element);
      size_t element_size = (size_t)literal_element->size;
      size_t string_length = literal->value.string->length / element_size + 1;
      if (!inode->type->info.array.is_flexible) {
        if (string_length > inode->type->info.array.size.fixed + 1) {
          SemanticError(expr, "Too many initializers for character array");
        }
      } else {
        inode->type->info.array.is_flexible = false;
        inode->type->info.array.size.fixed = (int)string_length;
        TypeRecordCalculateSize(inode->type);
      }
      inode->expr = ASTNodeMove(expr);
      expr->type->size = inode->type->size;
      return AdvanceCurrent(inode->parent);
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
  if (expr->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)expr)->symbol;
    if (symbol != NULL && CompilerSymbolIsMetaPromotedStatic(symbol)) {
      return true;
    }
    if (symbol != NULL &&
        CompilerMetaPromotedPointerTarget(symbol) != NULL) {
      return true;
    }
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
static ASTNode* FoldRequiredScalarConstant(ASTNode* expr) {
  ASTNode* folded = NULL;
  if (TypeIsIntegral(expr->type)) {
    int64_t value;
    if (EvaluateIntegerExpression(expr, &value)) {
      folded =
          NewIntConstantASTNode(value, expr->type, expr->location);
    }
  } else if (TypeIsFloatingPoint(expr->type)) {
    double value;
    if (EvaluateFloatingPointExpression(expr, &value)) {
      folded =
          NewRealConstantASTNode(value, expr->type, expr->location);
    }
  }
  if (folded == NULL) {
    return expr;
  }
  ASTNode* parent = expr->parent;
  int child_id = expr->child_id;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, folded, true);
  } else {
    ASTNodeDelete(expr);
  }
  folded->flags |= kASTAnalyzed;
  return folded;
}

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
  bool dependent_initializer =
      constants_only && CompilerIsCXX() &&
      ExpressionIsTemplateDependent(expr);
  bool required_depth_incremented =
      constants_only && !dependent_initializer;
  if (required_depth_incremented) {
    compiler->constant_evaluation_required_depth++;
  }
  expr = AnalyzeExpression(expr);
  if (required_depth_incremented) {
    compiler->constant_evaluation_required_depth--;
  }
  dependent_initializer =
      constants_only && CompilerIsCXX() &&
      ExpressionIsTemplateDependent(expr);
  if (constants_only && !dependent_initializer) {
    expr = FoldRequiredScalarConstant(expr);
  }
  switch (inode->kind) {
    case kIScalar:
      if (constants_only && !dependent_initializer) {
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
        if (constants_only && !dependent_initializer) {
          ASTNode* constant = ConstexprObjectInitializerForExpression(
              inode->type, expr);
          if (constant == NULL) {
            SemanticError(expr, "Expression is not a compile-time constant");
            return true;
          }
          return InitializeINode(inode, constant, true);
        }
        inode->expr = ASTNodeMove(expr);
        return AdvanceCurrent(inode->parent);
      }
      // Lazy append of all struct members.
      AppendStructMembers(inode);
      return InitCurrentAndAdvance(inode->current, expr, constants_only);
  }
}

// An array whose bound comes from its initializer gets one element for every
// index up to the largest designated one, so a designator sets the size of the
// object as well as the amount of work to build it.  The size is held in an
// int, so an index whose array would not fit is rejected before its elements
// are materialized.
static bool ArrayDesignatorBoundFits(INode* inode, int index) {
  if (!inode->type->info.array.is_flexible) {
    // The bound is already known; an index outside it is reported as out of
    // bounds by the caller, and no elements beyond it are built.
    return true;
  }
  TypeRecord* element = inode->type->next;
  TypeRecordCalculateSize(element);
  int element_size = element != NULL && element->size > 0 ? element->size : 1;
  return (int64_t)index + 1 <= (int64_t)INT_MAX / element_size;
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
      if (designator->value.array_index < 0) {
        // The index is used unsigned below, where a negative one would ask for
        // an array of nearly the whole address space.
        SemanticError(ast_node,
                      "Array designator [%d] is outside bounds of the array",
                      designator->value.array_index);
        return NULL;
      }
      if (!ArrayDesignatorBoundFits(inode, designator->value.array_index)) {
        SemanticError(ast_node,
                      "Array designator [%d] needs an array larger than this "
                      "compiler can lay out",
                      designator->value.array_index);
        return NULL;
      }
      if (designator->value.array_index >= inode->children.length) {
        AppendArrayINodeChildren(inode, designator->value.array_index);
      }
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
          member->symbol->type->info.array.is_flexible &&
          !designator->is_resolved_member) {
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
        if (base == designator->value.base ||
            (designator->value.base == NULL && base != NULL &&
             designator->type != NULL &&
             TypeEqual(base->type, designator->type))) {
          designator->value.base = base;
          designator->base_byte_offset = base->byte_offset;
          return GetChildAtIndex(inode, i, true);
        }
      }
      SemanticError(ast_node, "Unknown base class used in designator");
      return NULL;
    }
  }
  return NULL;
}

static StructMember* CXXDirectAssociatedMember(Struct* owner, String* name,
                                               size_t* declaration_order) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  StructMember* member = FindStructMember(owner, name);
  if (!StructMemberIsObjectMember(member)) {
    return NULL;
  }

  // A member injected by an anonymous aggregate has the declaration position
  // of that aggregate. FindStructMember also searches base classes, so failure
  // to associate the result with a direct element means it is inherited.
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* direct = owner->members.value.p[i];
    if (direct == member) {
      if (declaration_order != NULL) {
        *declaration_order = i;
      }
      return member;
    }
    if (direct != NULL && direct->is_anon && direct->symbol != NULL &&
        TypeIsStructOrUnion(direct->symbol->type) &&
        FindStructMember(direct->symbol->type->info.struct_info,
                         &member->symbol->name) != NULL) {
      if (declaration_order != NULL) {
        *declaration_order = i;
      }
      return member;
    }
  }
  return NULL;
}

typedef enum {
  kCXXDesignatedMemberNotFound,
  kCXXDesignatedMemberUnique,
  kCXXDesignatedMemberAmbiguous,
  kCXXDesignatedMemberNonAggregateBase,
} CXXDesignatedMemberStatus;

// Find the path of direct base classes from `owner` to the class that directly
// contains `name`. P2287 only permits that path when every traversed base is an
// aggregate. Multiple matching base subobjects make member lookup ambiguous.
static CXXDesignatedMemberStatus CXXFindDesignatedMemberBasePath(
    Struct* owner, String* name, Vector* path, StructMember** member) {
  StructMember* direct = CXXDirectAssociatedMember(owner, name, NULL);
  if (direct != NULL) {
    if (member != NULL) {
      *member = direct;
    }
    return kCXXDesignatedMemberUnique;
  }

  size_t matches = 0;
  CXXDesignatedMemberStatus selected = kCXXDesignatedMemberNotFound;
  StructMember* selected_member = NULL;
  for (size_t i = 0; owner != NULL && i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Vector nested;
    VectorInit(&nested);
    StructMember* nested_member = NULL;
    CXXDesignatedMemberStatus status = CXXFindDesignatedMemberBasePath(
        base->type->info.struct_info, name, &nested, &nested_member);
    if (status == kCXXDesignatedMemberNotFound) {
      VectorDestruct(&nested);
      continue;
    }
    if (status == kCXXDesignatedMemberAmbiguous) {
      VectorDestruct(&nested);
      return kCXXDesignatedMemberAmbiguous;
    }
    matches++;
    if (matches > 1) {
      VectorDestruct(&nested);
      return kCXXDesignatedMemberAmbiguous;
    }
    VectorAppend(path, base);
    for (size_t j = 0; j < nested.length; j++) {
      VectorAppend(path, nested.value.p[j]);
    }
    VectorDestruct(&nested);
    selected =
        status == kCXXDesignatedMemberNonAggregateBase ||
                !base->type->info.struct_info->is_aggregate
            ? kCXXDesignatedMemberNonAggregateBase
            : kCXXDesignatedMemberUnique;
    selected_member = nested_member;
  }
  if (matches == 1 && member != NULL) {
    *member = selected_member;
  }
  return selected;
}

// Resolve a source `.member` designator. For an inherited member in C++29,
// prepend internal base-class designators so the existing initializer tree,
// constexpr evaluator, and code generators initialize the correct subobject.
static CXXDesignatedMemberStatus CXXResolveDesignatedMemberPath(
    INode* inode, ASTNode* initializer, StructMember** member) {
  if (inode == NULL || inode->kind != kIStruct || initializer == NULL ||
      initializer->op != AST_OP(designated_init)) {
    return kCXXDesignatedMemberNotFound;
  }
  DesignatedInitializerASTNode* designated =
      (DesignatedInitializerASTNode*)initializer;
  if (designated->designators == NULL ||
      designated->designators->length == 0) {
    return kCXXDesignatedMemberNotFound;
  }
  Designator* first = designated->designators->value.p[0];
  if (first == NULL || first->designator_type != kDesignatorStruct) {
    return kCXXDesignatedMemberNotFound;
  }
  String* name = first->is_resolved_member
                     ? &first->value.struct_member->symbol->name
                     : first->value.struct_member_name;
  Struct* owner = inode->type->info.struct_info;
  StructMember* direct = CXXDirectAssociatedMember(owner, name, NULL);
  if (direct != NULL) {
    if (member != NULL) {
      *member = direct;
    }
    return kCXXDesignatedMemberUnique;
  }

  Vector bases;
  VectorInit(&bases);
  StructMember* inherited = NULL;
  CXXDesignatedMemberStatus status =
      CXXFindDesignatedMemberBasePath(owner, name, &bases, &inherited);
  if ((status == kCXXDesignatedMemberUnique ||
       status == kCXXDesignatedMemberNonAggregateBase) &&
      bases.length > 0) {
    Vector* resolved = NewVector();
    for (size_t i = 0; i < bases.length; i++) {
      VectorAppend(resolved, NewCXXBaseDesignator(bases.value.p[i]));
    }
    for (size_t i = 0; i < designated->designators->length; i++) {
      VectorAppend(resolved, designated->designators->value.p[i]);
    }
    VectorDelete(designated->designators);
    designated->designators = resolved;
  }
  VectorDestruct(&bases);
  if (member != NULL) {
    *member = inherited;
  }
  return status;
}

static Designator* CXXSingleStructDesignator(ASTNode* initializer) {
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
  return designator;
}

static String* CXXSingleDesignatorName(ASTNode* initializer) {
  Designator* designator = CXXSingleStructDesignator(initializer);
  if (designator == NULL) {
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

typedef struct {
  Vector* designators;
  size_t index;
  TypeRecord* type;
  bool done;
} CXXDesignatorOrderCursor;

static bool CXXNextDesignatorOrder(CXXDesignatorOrderCursor* cursor,
                                   size_t* order) {
  if (cursor == NULL || cursor->done || cursor->designators == NULL ||
      cursor->index >= cursor->designators->length ||
      cursor->type == NULL || !TypeIsStructOrUnion(cursor->type) ||
      cursor->type->info.struct_info == NULL) {
    return false;
  }
  Designator* designator = cursor->designators->value.p[cursor->index++];
  Struct* owner = cursor->type->info.struct_info;
  if (designator->designator_type == kDesignatorBase) {
    for (size_t i = 0; i < owner->bases.length; i++) {
      CXXBaseSpecifier* base = owner->bases.value.p[i];
      if (base == designator->value.base ||
          (designator->value.base == NULL && base != NULL &&
           designator->type != NULL &&
           TypeEqual(base->type, designator->type))) {
        designator->value.base = base;
        designator->base_byte_offset = base->byte_offset;
        *order = i;
        cursor->type = base->type;
        return true;
      }
    }
    return false;
  }
  if (designator->designator_type != kDesignatorStruct) {
    return false;
  }
  String* name =
      designator->is_resolved_member
          ? &designator->value.struct_member->symbol->name
          : designator->value.struct_member_name;
  size_t member_order = 0;
  StructMember* member =
      CXXDirectAssociatedMember(owner, name, &member_order);
  if (member == NULL) {
    return false;
  }
  *order = owner->bases.length + member_order;
  cursor->type = member->symbol->type;
  // DaveCC accepts nested C-style designators as an extension. Their ordering
  // remains based on the first named field, while P2287-inserted base
  // designators participate recursively.
  cursor->done = true;
  return true;
}

static int CXXCompareDesignatorOrder(TypeRecord* type, ASTNode* left,
                                     ASTNode* right) {
  if (left == NULL || right == NULL ||
      left->op != AST_OP(designated_init) ||
      right->op != AST_OP(designated_init)) {
    return 0;
  }
  CXXDesignatorOrderCursor l = {
      .designators = ((DesignatedInitializerASTNode*)left)->designators,
      .type = type,
  };
  CXXDesignatorOrderCursor r = {
      .designators = ((DesignatedInitializerASTNode*)right)->designators,
      .type = type,
  };
  for (;;) {
    size_t left_order = 0;
    size_t right_order = 0;
    bool has_left = CXXNextDesignatorOrder(&l, &left_order);
    bool has_right = CXXNextDesignatorOrder(&r, &right_order);
    if (!has_left || !has_right) {
      return 0;
    }
    if (left_order < right_order) {
      return -1;
    }
    if (left_order > right_order) {
      return 1;
    }
    if (l.done && r.done) {
      return 0;
    }
  }
}

static size_t CXXDirectBaseIndex(Struct* owner, CXXBaseSpecifier* target) {
  for (size_t i = 0; owner != NULL && i < owner->bases.length; i++) {
    if (owner->bases.value.p[i] == target) {
      return i;
    }
  }
  return SIZE_MAX;
}

static void CheckCXXDesignatedInitializers(
    INode* inode, BracedInitializerASTNode* braced_init) {
  if (!CompilerIsCXX() || inode == NULL || inode->kind != kIStruct ||
      braced_init == NULL || braced_init->initializers == NULL) {
    return;
  }

  Struct* owner = inode->type->info.struct_info;
  bool saw_designated = false;
  bool saw_source_designated = false;
  bool saw_nondesignated = false;
  bool nondesignated_after_designated = false;
  ASTNode* first_nondesignated_after_designated = NULL;
  size_t positional_prefix = 0;
  for (size_t i = 0; i < braced_init->initializers->length; i++) {
    ASTNode* initializer = braced_init->initializers->value.p[i];
    if (initializer != NULL &&
        initializer->op == AST_OP(designated_init)) {
      saw_designated = true;
      if ((initializer->flags & kASTSourceDesignatedInitializer) != 0) {
        saw_source_designated = true;
      }
    } else {
      saw_nondesignated = true;
      if (saw_designated) {
        nondesignated_after_designated = true;
        if (first_nondesignated_after_designated == NULL) {
          first_nondesignated_after_designated = initializer;
        }
      } else {
        positional_prefix++;
      }
    }
  }
  if (saw_source_designated && nondesignated_after_designated) {
    SemanticError(
        first_nondesignated_after_designated,
        "non-designated initializer cannot follow a designated initializer");
  }
  if (saw_source_designated && (owner == NULL || !owner->is_aggregate)) {
    SemanticError((ASTNode*)braced_init,
                  "designated initialization requires an aggregate class");
  }
  if (saw_source_designated && saw_nondesignated &&
      !CompilerCXXAtLeast(kLanguageStandardCXX29)) {
    SemanticError(
        (ASTNode*)braced_init,
        "mixing designated and non-designated initializers requires C++29");
  } else if (saw_source_designated && saw_nondesignated &&
             !nondesignated_after_designated && owner != NULL &&
             positional_prefix > owner->bases.length) {
    SemanticError(
        braced_init->initializers->value.p[owner->bases.length],
        "non-designated initializer in a mixed list must initialize a direct "
        "base class");
  }

  ASTNode* previous = NULL;
  for (size_t i = 0; i < braced_init->initializers->length; i++) {
    ASTNode* initializer = braced_init->initializers->value.p[i];
    if (initializer == NULL ||
        initializer->op != AST_OP(designated_init)) {
      continue;
    }
    if ((initializer->flags & kASTSourceDesignatedInitializer) == 0) {
      continue;
    }
    String* designator_name = CXXSingleDesignatorName(initializer);
    Designator* designator = CXXSingleStructDesignator(initializer);
    // Only source-level `.member =` designators are constrained to use a name
    // once.  Positional aggregate initialization is internally flattened to
    // resolved member designators and may legitimately initialize multiple
    // C++26 name-independent members that are all spelled `_`.
    if (designator_name != NULL && designator != NULL &&
        !designator->is_resolved_member) {
      for (size_t j = 0; j < i; j++) {
        Designator* prior_designator = CXXSingleStructDesignator(
            braced_init->initializers->value.p[j]);
        String* prior_name = CXXSingleDesignatorName(
            braced_init->initializers->value.p[j]);
        if (prior_name != NULL && prior_designator != NULL &&
            !prior_designator->is_resolved_member &&
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

    StructMember* member = NULL;
    CXXDesignatedMemberStatus status =
        CXXResolveDesignatedMemberPath(inode, initializer, &member);
    DesignatedInitializerASTNode* designated =
        (DesignatedInitializerASTNode*)initializer;
    bool inherited =
        designated->designators != NULL &&
        designated->designators->length > 0 &&
        ((Designator*)designated->designators->value.p[0])->designator_type ==
            kDesignatorBase;
    if (status == kCXXDesignatedMemberAmbiguous) {
      SemanticError(initializer, "designated member lookup is ambiguous");
      continue;
    }
    if (status == kCXXDesignatedMemberNonAggregateBase) {
      SemanticError(
          initializer,
          "designated member is inherited through a non-aggregate base class");
    }
    if (inherited && !CompilerCXXAtLeast(kLanguageStandardCXX29)) {
      SemanticError(initializer,
                    "designating an inherited member requires C++29");
    }
    if (member == NULL) {
      continue;
    }
    if (designator != NULL && !designator->is_resolved_member &&
        member->symbol != NULL &&
        member->symbol->flags.name_independent_lookup_ambiguous) {
      SemanticError(
          initializer,
          "reference to name-independent declaration '%s' is ambiguous",
          member->symbol->name.value);
      continue;
    }
    if (positional_prefix > 0 && inherited) {
      CXXBaseSpecifier* base =
          ((Designator*)designated->designators->value.p[0])->value.base;
      if (CXXDirectBaseIndex(owner, base) < positional_prefix) {
        SemanticError(initializer,
                      "base class is initialized by both positional and "
                      "designated initializers");
      }
    }
    if (previous != NULL &&
        CXXCompareDesignatorOrder(inode->type, previous, initializer) > 0) {
      SemanticError(
          initializer,
          "designator order for field '%s' does not match declaration order "
          "in '%s'",
          member->symbol->name.value,
          owner->tag_name != NULL ? owner->tag_name->value : "<anonymous>");
    }
    previous = initializer;
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


// True if this INode (or, for an aggregate element, any of its descendants)
// received at least one initializer.  A scalar element records initializers on
// itself, but a struct/array element records them on its members/elements, so
// its own `num_initializers` stays zero even when fully initialized.  The
// flexible-array length deduction must recurse to count such elements or a
// `T x[] = {{...},{...}}` would deduce a length (and thus size) of zero.
static bool INodeWasInitialized(INode* inode) {
  if (inode->num_initializers > 0 || inode->expr != NULL) {
    return true;
  }
  for (size_t i = 0; i < inode->children.length; i++) {
    if (INodeWasInitialized((INode*)inode->children.value.p[i])) {
      return true;
    }
  }
  return false;
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
      if (braced_init->initializers->length == 0 && !CompilerIsCXX() &&
          !CompilerCAtLeast(kLanguageStandardC23)) {
        SemanticError(init_expr,
                      "empty initializer requires C23");
      }
      LazyInitINode(inode);
      CheckCXXDesignatedInitializers(inode, braced_init);
      size_t cxx_positional_prefix = 0;
      bool cxx_has_designated = false;
      if (CompilerIsCXX() &&
          CompilerCXXAtLeast(kLanguageStandardCXX29) &&
          inode->kind == kIStruct) {
        for (size_t i = 0; i < braced_init->initializers->length; i++) {
          ASTNode* initializer = braced_init->initializers->value.p[i];
          if (initializer != NULL &&
              initializer->op == AST_OP(designated_init)) {
            cxx_has_designated = true;
            break;
          }
          cxx_positional_prefix++;
        }
      }
      INode* parent = inode->parent;
      inode->parent = NULL;
      if (braced_init->initializers->length == 0 &&
          inode->kind == kIScalar &&
          (CompilerIsCXX() || CompilerCAtLeast(kLanguageStandardC23))) {
        inode->num_initializers++;
        inode->expr =
            TypeIsFloatingPoint(inode->type)
                ? NewRealConstantASTNode(0.0, TypeRecordCopy(inode->type),
                                         init_expr->location)
                : NewIntConstantASTNode(0, TypeRecordCopy(inode->type),
                                        init_expr->location);
      }
      for (size_t i = 0; i < braced_init->initializers->length; i++) {
        INode* current = inode->current == NULL ? inode : inode->current;
        ASTNode* initializer = braced_init->initializers->value.p[i];
        bool mixed_positional_base =
            cxx_has_designated && i < cxx_positional_prefix &&
            current != NULL && current->is_base_subobject;
        bool directly_targets_base =
            mixed_positional_base &&
            initializer->op == AST_OP(braced_init);
        if (mixed_positional_base &&
            initializer->op == AST_OP(expr_init)) {
          ASTNode* expression =
              ((ExpressionInitializerASTNode*)initializer)->expr;
          directly_targets_base =
              expression != NULL &&
              (expression->op == AST_OP(compound_literal) ||
               (expression->type != NULL &&
                StructInitializationTypesMatch(expression->type,
                                                current->type)));
        }
        if (!InitializeINode(current, initializer, constants_only)) {
          SemanticError(initializer,
                        "Too many initializers");
          break;
        }
        if (mixed_positional_base &&
            initializer->op == AST_OP(expr_init) &&
            current->expr == NULL && !directly_targets_base) {
          ASTNode* analyzed_expression =
              ((ExpressionInitializerASTNode*)initializer)->expr;
          if (analyzed_expression == NULL ||
              analyzed_expression->type == NULL ||
              !StructInitializationTypesMatch(analyzed_expression->type,
                                              current->type)) {
            SemanticError(
                initializer,
                "non-designated initializer in a mixed list must initialize a "
                "direct base class");
          }
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
          if (INodeWasInitialized(child)) {
            inode->num_initializers = (int)i + 1;
            break;
          }
        }
        if (parent == NULL) {
          inode->type->info.array.size.fixed = inode->num_initializers;
          inode->type->info.array.is_flexible = false;
          TypeRecordCalculateSize(inode->type);
        }
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

