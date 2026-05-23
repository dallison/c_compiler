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
  int num_initializers;  // Number of initializers for this node.
} INode;

INode* BuildINode(TypeRecord* type, INode* parent);

static INode* NewINode(IKind kind, TypeRecord* type, INode* parent) {
  INode* inode = malloc(sizeof(INode));
  inode->kind = kind;
  inode->type = type;
  inode->expr = NULL;
  inode->parent = parent;
  inode->next = NULL;
  inode->current = NULL;
  inode->index = 0;
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
  size_t num_children = type->info.struct_info->members.length;
  for (size_t i = 0; i < num_children; i++) {
    StructMember* member = type->info.struct_info->members.value.p[i];
    // Flexible array members are effectively invisible in initializers.
    if (TypeIsArray(member->symbol->type) &&
        member->symbol->type->info.array.is_flexible) {
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

// Initialize the current node and advance to the next.  Returns true
// if the initialization is valid.
static bool InitCurrentAndAdvance(INode* inode, ASTNode* expr, bool constants_only) {
  if (inode == NULL) {
    return false;
  }
  if (inode->expr != NULL) {
    return false;
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
      if (TypeEqual(inode->type, expr->type)) {
        if (constants_only) {
          SemanticError(expr, "Expression is not a compile-time constant");
          return true;
        }
        // A struct can be initialized by an expression with the same type.
        inode->expr = ASTNodeMove(expr);
        return AdvanceCurrent(inode->parent);
      }
      // Lazy append of all struct members.
      AppendStructMembers(inode);
      return InitCurrentAndAdvance(inode->current, expr, constants_only);
  }
}

// Get an inode child given its index, or NULL if index is invalid.
static INode* GetChildAtIndex(INode* inode, size_t index) {
  if (index >= inode->children.length) {
    return NULL;
  }
  return inode->children.value.p[index];
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
                                       designator->value.array_index);
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
      StructMember* member = FindStructMember(inode->type->info.struct_info,
                                              designator->value.struct_member_name);
      if (member == NULL) {
        SemanticError(ast_node, "Unknown struct member %s used in designator",
                      designator->value.struct_member_name->value);
        return NULL;
      }
      if (TypeIsArray(member->symbol->type) &&
          member->symbol->type->info.array.is_flexible) {
        SemanticError(ast_node, "Use of flexible array member in designator");
        return NULL;
      }
      return GetChildAtIndex(inode, member->index);
    }
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
      INode* designated_node = inode->parent;
      LazyInitINode(inode);
      for (size_t i = 0; i < designated_init->designators->length; i++) {
        designated_node = FindDesignator(designated_node, init_expr, designated_init->designators->value.p[i]);
        if (designated_node == NULL) {
          return false;
        }
      }
      // Set designated node as parent's current.
      designated_node->parent->current = designated_node;
      return InitializeINode(designated_node, designated_init->init, constants_only);
    }
    default:
      return false;
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
      VectorAppend(designators,
                   NewStructMemberDesignator(
                                       type->info.struct_info->
                                       members.value.p[inode->index]));
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
    // Top level.
    BuildSingleDesignator(inode, inode->kind, inode->type, designators);
  } else {
    BuildDesignator(inode, designators);
  }
  inode->expr = AnalyzeExpression(inode->expr);
  ASTNode* designated_init = NewDesignatedInitializerASTNode(designators,
                                                inode->expr,
                                                inode->expr->location);
  NormalConversion(inode->expr, inode->type);
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
  // PrintINode(inode, 0);
  ASTNode* braced_init = NewBracedInitializerASTNode(NewVector(), type, ast_node->location);
  FlattenINode(inode, (BracedInitializerASTNode*)braced_init);
  // ASTNodePrint(braced_init, 0);
  DeleteINode(inode);
  return braced_init;
}

