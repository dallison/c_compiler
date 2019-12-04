//
//  init_semantics.c
//  c_compiler
//
//  Created by David Allison on 12/2/17.
//  Copyright © 2017 David Allison. All rights reserved.
//
#include "init_semantics.h"
#include <stdlib.h>
#include "expr_semantics.h"
#include "list.h"

// The semantic analysis of an initializer converts the tree
// of initializers into a single braced initializer containing
// designated initializers only.  This is to simplify both
// semantic analysis and code generation.

typedef enum {
  kIScalar,      // Scalar with an initialization expression.
  kIArray,       // Array with inode for every element.
  kIStruct,      // Struct/union with inode for every member.
} IKind;

// Initialization Node.
typedef struct INode {
  IKind kind;
  TypeRecord* type;     // Not owned.
  ASTNode* expr;        // Nod owned.
  struct INode* parent;
  struct INode* next;     // Not owned.
  struct INode* current;  // Not owned.
  Vector children;
  size_t index;
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
  VectorInit(&inode->children);
  return inode;
}

static INode* BuildStructINode(TypeRecord* type, INode* parent) {
  INode* inode = NewINode(kIStruct, type, parent);
  INode* prev = NULL;
  for (size_t i = 0; i < type->info.struct_info->members.length; i++) {
    StructMember* member = type->info.struct_info->members.value.p[i];
    // Flexible array members are effectively invisible in initializers.
    if (TypeIsArray(member->symbol->type) &&
        member->symbol->type->info.array.is_flexible) {
      continue;
    }
    INode* child = BuildINode(member->symbol->type, inode);
    child->index = i;
    VectorAppend(&inode->children, child);
    if (prev != NULL) {
      prev->next = child;
    }
    prev = child;
    if (type->info.struct_info->is_union) {
      // Only first element of a union is initialized.
      break;
    }
  }
  
  // Current node is first child.
  inode->current = (INode*)inode->children.value.p[0];
  return inode;
}


static INode* BuildArrayINode(TypeRecord* type,  INode* parent) {
  INode* inode = NewINode(kIArray, type, parent);
  INode* prev = NULL;
  // Add an inode for every member of the array.
  for (size_t i = 0; i < type->info.array.size; i++) {
    INode* child = BuildINode(type->next, inode);
    child->index = i;
    VectorAppend(&inode->children, child);
    if (prev != NULL) {
      prev->next = child;
    }
    prev = child;
  }
  // Current node is first child.
  inode->current = (INode*)inode->children.value.p[0];
  return inode;
}

INode* BuildINode(TypeRecord* type, INode* parent) {
  if (TypeIsStructOrUnion(type)) {
    return BuildStructINode(type, parent);
  }
  if (TypeIsArray(type)) {
    return BuildArrayINode(type, parent);
  }
  INode* inode = NewINode(kIScalar, type, parent);
  inode->current = inode;
  return inode;
}

static void DeleteINodeChild(INode* child) {
  VectorDestructWithContents(&child->children,
                            (VectorElementDestructor)DeleteINodeChild);

}

static void DeleteINode(INode* inode) {
  VectorDestructWithContents(&inode->children,
                           (VectorElementDestructor)DeleteINodeChild);
  free(inode);
}

static void DoIndent(int indent) {
  for (int i = 0; i < indent; i++) {
    putchar(' ');
  }
}


static void PrintINode(INode* inode, int indent) {
  DoIndent(indent);
  printf("%p: parent: %p current: %p expr: %p ", inode,
         inode->parent, inode->current, inode->expr);
  switch (inode->kind) {
    case kIScalar:
      printf("scalar\n");
      break;
    case kIStruct:
    case kIArray:
      printf("%s {\n", inode->kind == kIArray ? "array" : "struct");
      for (size_t i = 0; i < inode->children.length; i++) {
         PrintINode(inode->children.value.p[i], indent + 2);
      }
  
      DoIndent(indent);
      printf("}\n");
      break;
  }
  DoIndent(indent);
  printf("}\n");
}

static bool AdvanceCurrent(INode* inode) {
  if (inode == NULL) {
    return true;
  }
  if (inode->kind == kIScalar || inode->current->next == NULL) {
    AdvanceCurrent(inode->parent);
    return true;
  }
  inode->current = inode->current->next;
  return true;
}

// Initialize the current node and advance to the next.  Returns true
// if the initialization is valid.
static bool InitCurrentAndAdvance(INode* inode, ASTNode* expr) {
  if (inode->expr != NULL) {
    return false;
  }
  switch (inode->kind) {
    case kIScalar:
      inode->expr = expr;
      return AdvanceCurrent(inode);
      
    case kIArray:
      if (expr->op == AST_OP(string)) {
        // Array initialized by string?
        if (TypeIsChar(inode->type->next)) {
          if (!inode->type->info.array.is_flexible) {
            ConstantASTNode* c = (ConstantASTNode*)expr;
            if (c->value.string->length > inode->type->info.array.size) {
              SemanticError(expr,
                            "Too many initializers for character array");
            }
          }
          inode->expr = expr;
          return AdvanceCurrent(inode);
        }
      }
      if (expr->op == AST_OP(string_wide)) {
         // Array initialized by wide string?
         if (TypeIsInt(inode->type->next)) {
           inode->expr = expr;
           if (!inode->type->info.array.is_flexible) {
             ConstantASTNode* c = (ConstantASTNode*)expr;
             if ((c->value.string->length / sizeof(int)) >
                 inode->type->info.array.size) {
               SemanticError(expr,
                             "Too many initializers for wide character array");
             }
           }
          return AdvanceCurrent(inode);
         }
       }
      return InitCurrentAndAdvance(inode->current, expr);
      
    case kIStruct:
      if (TypeEqual(inode->type, expr->type)) {
        // A struct can be initialized by an expression with the same type.
        inode->expr = expr;
        return AdvanceCurrent(inode);
      }
      return InitCurrentAndAdvance(inode->current, expr);
  }
}

static INode* GetChildAtIndex(INode* inode, size_t index) {
  if (index >= inode->children.length) {
    return NULL;
  }
  return inode->children.value.p[index];
}

static INode* FindDesignator(INode* inode,
                             ASTNode* ast_node,
                             Designator* designator) {
  switch (designator->designator_type) {
    case kDesignatorArray: {
      if (inode->kind != kIArray) {
        SemanticError(ast_node, "Use of array designator on a non-array");
        return NULL;
      }
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
      StructMember* member = FindStructMember(inode->type->info.struct_info, designator->value.struct_member_name);
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

static INode* FindBracedParent(INode* inode) {
  while (inode->parent != NULL) {
    inode = inode->parent;
  }
  return inode;
}

static bool InitializeINode(INode* inode, ASTNode* init_expr) {
  switch (init_expr->op) {
    case AST_OP(expr_init): {
      ExpressionInitializerASTNode* expr_init = (ExpressionInitializerASTNode*)init_expr;
      return InitCurrentAndAdvance(inode, expr_init->expr);
    }
    case AST_OP(braced_init): {
      BracedInitializerASTNode* braced_init = (BracedInitializerASTNode*)init_expr;
      INode* parent = inode->parent;
      inode->parent = NULL;
      for (size_t i = 0; i < braced_init->initializers->length; i++) {
        if (!InitializeINode(inode->current, braced_init->initializers->value.p[i])) {
          SemanticError((ASTNode*)braced_init->initializers->value.p[i],
                        "Too many initializers");
          break;
        }
      }
      inode->parent = parent;
      return AdvanceCurrent(parent);
    }
    case AST_OP(designated_init): {
      DesignatedInitializerASTNode* designated_init = (DesignatedInitializerASTNode*)init_expr;
      INode* designated_node = FindBracedParent(inode);
      for (size_t i = 0; i < designated_init->designators->length; i++) {
        designated_node = FindDesignator(designated_node, init_expr, designated_init->designators->value.p[i]);
        if (designated_node == NULL) {
          return false;
        }
      }
      // Set designated node as parent's current.
      designated_node->parent->current = designated_node;
      return InitializeINode(designated_node, designated_init->init);
    }
    default:
      return false;
  }
}

static void BuildDesignator(INode* inode, Vector* designators) {
  if (inode->parent == NULL) {
    return;
  }
  BuildDesignator(inode->parent, designators);
  switch (inode->parent->kind) {
    case kIArray:
      VectorAppend(designators, NewArrayDesignator(inode->type, (int)inode->index));
      break;
    case kIStruct:
      VectorAppend(designators,
                   NewStructDesignator(
                                       inode->parent->type->
                                       info.struct_info->
                                       members.value.p[inode->index]));
      break;

    case kIScalar:
      abort();
      break;
  }
  
}

static ASTNode* BuildDesignatedInitializer(INode* inode) {
  Vector* designators = NewVector();
  BuildDesignator(inode, designators);
  SemanticConvertType(inode->expr, inode->type);
  return NewDesignatedInitializerASTNode(designators,
                                                inode->expr,
                                                inode->expr->location);
}

static void ExpandINode(INode* inode,
                            BracedInitializerASTNode* braced_init) {
  if (inode->expr != NULL) {
     VectorAppend(braced_init->initializers,
                  BuildDesignatedInitializer(inode));
  }
  for (size_t i = 0; i < inode->children.length; i++) {
    ExpandINode(inode->children.value.p[i], braced_init);
  }
}

ASTNode* AnalyzeInitializer(TypeRecord* type, ASTNode* ast_node) {
  INode* inode = BuildINode(type, NULL);
  InitializeINode(inode, ast_node);
  PrintINode(inode, 0);
  ASTNode* braced_init = NewBracedInitializerASTNode(NewVector(), ast_node->location);
  ExpandINode(inode, (BracedInitializerASTNode*)braced_init);
  ASTNodePrint(braced_init, 0);
  DeleteINode(inode);
  return braced_init;
}

#if 0
// For processing the user-supplied initializer into the form we want
// we keep a state struct.  This is used for array and struct/union
// initialization.
typedef struct {
  int num_elements;  // Number of elements in the state arrays.
  int* limits;       // # elements in array dimension or # members in struct.
  int* indexes;      // Current element in array or current struct member index.
  int current;       // "Current Object": the thing we are initializing.
  TypeRecord** types;              // Flattened type chain.
  BracedInitializerASTNode* init;  // Result.
  bool is_struct;
  Struct* struct_info;   // Struct information (if is_struct is true)
  StructMember* member;  // Member being initialized.
} InitializerState;

// Initialize an instance of the InitializerState object.
static void StateInit(InitializerState* state) {
  state->current = -1;
  state->num_elements = 1;
  state->indexes = NULL;
  state->limits = NULL;
  state->types = NULL;
  state->struct_info = NULL;
  state->is_struct = false;
  state->member = NULL;
}

// We have a value to assign to another element of the Array or struct
// initializer.
static void NextIndex(InitializerState* state) {
  // The num_elements state field is one greater than the number of
  // dimensions in the array (the last is the base type).
  int index;
  if (state->is_struct) {
    index = state->num_elements - 1;
  } else {
    index = state->num_elements - 2;
  }

  // Looks backwards from the last array dimension for a space for the
  // index.  If a dimension is full we move to the next highest one
  // and use that one.  In this case we zero out the index for all
  // lower dimensions.
  while (index >= state->current) {
    if (state->limits[index] == 0 ||
        state->indexes[index] < state->limits[index]) {
      state->indexes[index]++;  // Increment next available index.
      // Zero out all the lower dimensions.
      index++;
      while (index < state->num_elements - 1) {
        state->indexes[index++] = 0;
      }
      return;
    }
    index--;
  }
}

// Check if there is space for another initializer.
static bool CheckInitializerSpace(InitializerState* state) {
  // The num_elements state field is one greater than the number of
  // dimensions in the array (the last is the base type).
  int index;
  if (state->is_struct) {
    // We are initializeing a struct so there are members in the struct.
    index = state->num_elements - 1;
  } else {
    index = state->num_elements - 2;
  }

  // Looks backwards from the last array dimension for a space for the
  // index.  If a dimension is full we move to the next highest one
  // and use that one.
  while (index >= state->current) {
    if (state->limits[index] == 0 ||
        state->indexes[index] < state->limits[index]) {
      return true;
    }
    index--;
  }
  return false;
}

// We have reached the end of a braced initializer for an array dimension.
// Close the dimension and move to the next in the parent.
static void CloseCurrentArrayState(InitializerState* state) {
  if (state->current >= 0) {
    int index = state->current;
    state->indexes[index]++;
    // Zero out all the lower dimensions.
    index++;
    while (index < state->num_elements) {
      state->indexes[index++] = 0;
    }
  }
}

// Make a designator out of the current state indexes.
static Vector* MakeDesignator(InitializerState* state) {
  Vector* vec = NewVector();

  // First the array dimensions.
  for (int i = 0; i < state->num_elements - 1; i++) {
    Designator* designator = NewArrayDesignator(state->indexes[i]);
    designator->type = state->types[i];
    VectorAppend(vec, designator);
  }

  // Now the struct member designator if it exists.
  if (state->member != NULL) {
    Designator* designator = NewStructMemberDesignator(state->member);
    designator->type = state->types[state->num_elements - 1];
    VectorAppend(vec, designator);
  }

  // No point in returning an empty vector.
  if (vec->length == 0) {
    VectorDelete(vec);
    return NULL;
  }
  return vec;
}

// Our output consists of a fully designated braced initializer.  This makes
// a designated initializer out of the state.
static ASTNode* MakeDesignatedInitializer(InitializerState* state,
                                          ASTNode* init) {
  return NewDesignatedInitializerASTNode(MakeDesignator(state), init,
                                         init->location);
}

// We have a designated initializer specifed by the user.  We need to insert
// this into the state by setting the indexes array as specifed by the
// initializer.
static void InsertDesignator(DesignatedInitializerASTNode* init,
                             InitializerState* state) {
  int index = state->current;  // Start at current object.
  for (size_t i = 0; i < init->designators->length; i++) {
    Designator* d = (Designator*)init->designators->value.p[i];
    if (d->designator_type == kDesignatorArray) {
      if (TypeIsArray(state->types[index])) {
        int array_index = d->value.array_index;
        if (index >= state->num_elements - 2) {
          SemanticError((ASTNode*)init,
                        "Designated array index [%d] exceeds array dimensions",
                        array_index);
          break;
        }
        if (state->limits[index] != 0 &&
            (array_index < 0 || array_index >= state->limits[index])) {
          SemanticError(
              (ASTNode*)init,
              "Designated array index [%d] exceeds array size of [%d]",
              array_index, state->limits[index]);
        } else {
          state->indexes[index] = d->value.array_index;
        }
      } else {
        SemanticError((ASTNode*)init,
                      "Cannot use [] designator for an non-array");
      }
    } else if (d->designator_type == kDesignatorStruct) {
      if (!state->is_struct) {
        SemanticError(
            (ASTNode*)init,
            "Cannot use struct member designator on a non struct/union");
        break;
      }
      state->member = FindStructMember(state->struct_info,
                                       d->value.struct_member_name);
      if (state->member == NULL) {
        SemanticError((ASTNode*)init,
                      "Designated initializer specifies undefined member %s of "
                      "struct/union %s",
                      state->struct_info->tag_name,
                      d->value.struct_member_name->value);
        break;
      }
      state->indexes[state->current] = (int)state->member->index;
    } else {
      SemanticError((ASTNode*)init, "Internal error: unknown designator type");
    }
    index++;
  }
}

// We are initializing a struct/union.  The limits array element in the state
// says how many members there are to initialize.
static void AnalyzeStructInitializer(ASTNode* init,
                                     InitializerState* state) {
  if (init->op == AST_OP(braced_init)) {
    state->current++;
    BracedInitializerASTNode* braced_init = (BracedInitializerASTNode*)init;
    for (size_t i = 0; i < braced_init->initializers->length; i++) {
      ASTNode* subinit = (ASTNode*)braced_init->initializers->value.p[i];
      StructMember* old_member = state->member;
      if (state->indexes[state->current] == state->limits[state->current]) {
        SemanticError(init, "Too many initializers for struct");
        return;
      }
      state->member =
          (StructMember*)
              state->struct_info->members.value.p[state->indexes[state->current]];
      if (subinit->op == AST_OP(designated_init)) {
        DesignatedInitializerASTNode* designated_init =
            (DesignatedInitializerASTNode*)subinit;
        InsertDesignator(designated_init, state);
        subinit = AnalyzeInitializer(state->member->symbol->type,
                                     designated_init->init);
      } else {
        subinit =
            AnalyzeInitializer(state->member->symbol->type, subinit);
      }

      VectorAppend(state->init->initializers,
                   MakeDesignatedInitializer(state, subinit));
      state->member = old_member;
      state->indexes[state->current]++;
    }
    state->current--;
  } else {
    // Non-braced initializer for struct.
    // TODO
  }
}

// An expression initializer has been found.  Convert this to a fully designated
// initializer and insert it into the result.
static void AnalyzeExpressionInitializer(ExpressionInitializerASTNode* init,
                                         InitializerState* state) {
  AnalyzeExpression(init->expr);
  TypeRecord* type = state->types[state->num_elements - 1];

  // If this expression is part of a struct we initialize the current struct
  // member.
  if (state->is_struct) {
    int index = state->indexes[state->num_elements - 1];
    if (index < type->info.struct_info->members.length) {
      state->member =
          (StructMember*)type->info.struct_info->members.value.p[index];
      type = state->member->symbol->type;
      SemanticConvertType(init->expr, type);
    }
  } else {
    // A scalar, convert the initalizer to the type.
    SemanticConvertType(init->expr, type);
  }

  // Make a new designated initializer for the result.
  ASTNode* new_init = MakeDesignatedInitializer(
      state, NewExpressionInitializerASTNode(init->expr, init->base.location));
  VectorAppend(state->init->initializers, new_init);
  ASTNodeSetType(new_init, type);

  // We have used the init->expr in the result so we need to prevent it being
  // deleted from the original tree.
  init->expr = NULL;
}

// Analyze an array initialization.
static void AnalyzeArrayInitializer(ASTNode* init,
                                    InitializerState* state) {
  switch (init->op) {
    case AST_OP(braced_init): {
      BracedInitializerASTNode* braced_init = (BracedInitializerASTNode*)init;
      if (state->current >= 0) {
        if (state->limits[state->current] > 0 &&
            state->indexes[state->current] >= state->limits[state->current]) {
          SemanticError(init, "Too many braced initializers; max is %d",
                        state->indexes[state->current]);
          return;
        }
      }
      if (state->current == state->num_elements - 2) {
        // Last dimension, this is either a scalar or struct.
        if (state->is_struct) {
          AnalyzeStructInitializer(init, state);
          return;
        } else {
          // Brace enclosed scalar.
          if (braced_init->initializers->length > 1) {
            SemanticWarning(init, "braced-scalar-init",
                            "Too many initializers for scalar");
          }
          ASTNode* subinit = (ASTNode*)braced_init->initializers->value.p[0];
          AnalyzeArrayInitializer(subinit, state);
          return;
        }
        return;
      }
      state->current++;
      for (size_t i = 0; i < braced_init->initializers->length; i++) {
        ASTNode* subinit = (ASTNode*)braced_init->initializers->value.p[i];
        AnalyzeArrayInitializer(subinit, state);
        CloseCurrentArrayState(state);
      }
      state->current--;
      break;
    }

    case AST_OP(expr_init): {
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)init;
      if (expr_init->expr->op == AST_OP(string) ||
          expr_init->expr->op == AST_OP(string_wide)) {
        // We are allowed to initialize an array of chars with a string literal.
        // TODO
        abort();
        break;
      }
      bool ok = CheckInitializerSpace(state);
      if (!ok) {
        SemanticWarning(init, "too-many-initialzers", "Too many intializers");
      }
      AnalyzeExpressionInitializer(expr_init, state);
      NextIndex(state);
      break;
    }
    case AST_OP(designated_init): {
      DesignatedInitializerASTNode* designated_init =
          (DesignatedInitializerASTNode*)init;
      InsertDesignator(designated_init, state);
      AnalyzeArrayInitializer(designated_init->init, state);
      break;
    }
    default:;
  }
}

// Analyze an initializer for the type passed.  We do not know
// the type of an initializer until we know what it is initializing.
// The result is a fully designated initializer (as if the user had
// designated each and every expression in the initializer with its
// array indices or struct member.  This simplifies code generation.
ASTNode* AnalyzeInitializer(TypeRecord* type, ASTNode* init) {
  InitializerState state;
  StateInit(&state);
  state.init = (BracedInitializerASTNode*)NewBracedInitializerASTNode(
      NewVector(), init->location);
  ASTNodeSetType((ASTNode*)state.init, type);

  if (TypeIsArray(type)) {
    // How many dimensions are there in the array?
    int num_dims = 0;
    TypeRecord* dim = type;
    while (TypeIsArray(dim)) {
      dim = dim->next;
      num_dims++;
    }

    // Fill in state struct for the array initialization.
    int num_elements = num_dims + 1;
    state.num_elements = num_elements;
    state.indexes = calloc(num_elements, sizeof(int));
    state.limits = calloc(num_elements, sizeof(int));
    state.types = calloc(num_elements, sizeof(TypeRecord*));
    state.struct_info = NULL;

    // Set the dimension sizes in the limits array and the types.
    dim = type;
    int index = 0;
    while (TypeIsArray(dim)) {
      state.limits[index] = dim->info.array.size;
      state.types[index] = dim;
      dim = dim->next;
      index++;
    }

    // Insert the base type as the last in the types array.
    state.types[index] = dim;

    // If this is an array of structs we need to insert the number of
    // struct members as the final limits.
    if (TypeIsStructOrUnion(state.types[num_elements - 1])) {
      Struct* struct_info = state.types[num_elements - 1]->info.struct_info;
      state.struct_info = struct_info;
      state.is_struct = true;
      state.limits[index] =
          struct_info->is_union ? 1 : (int)struct_info->members.length;
    }

    // Perform the heavy lifting.
    AnalyzeArrayInitializer(init, &state);
    if (type->info.array.size == 0) {
      type->info.array.size = state.indexes[0] - 1;
      TypeRecordCalculateSize(type);
    }
  } else if (TypeIsStructOrUnion(type)) {
    // We are initializing a struct or union.
    switch (init->op) {
      case AST_OP(braced_init):
        // Initialization of a struct using a braced initializer.
        state.limits = calloc(1, sizeof(int));
        state.indexes = calloc(1, sizeof(int));
        state.types = malloc(1 * sizeof(TypeRecord*));
        state.types[0] = type;
        state.is_struct = true;
        state.struct_info = type->info.struct_info;
        state.limits[0] = state.struct_info->is_union
                              ? 1
                              : (int)state.struct_info->members.length;
        AnalyzeStructInitializer(init, &state);
        break;

      case AST_OP(expr_init): {
        // Initialization of a struct with another struct.
        ExpressionInitializerASTNode* expr_init =
            (ExpressionInitializerASTNode*)init;
        ASTNode* new_init = MakeDesignatedInitializer(
            &state, NewExpressionInitializerASTNode(expr_init->expr,
                                                    expr_init->base.location));
        VectorAppend(state.init->initializers, new_init);
        ASTNodeSetType(new_init, type);

        // We have used the expr_init->expr in the result so we need to prevent
        // it being deleted from the original tree.
        expr_init->expr = NULL;
        break;
      }
      default:
        SemanticError(
            init,
            "Unexpected initializer expected for struct/union initialization");
    }
  } else {
    // Scalar initialization.
    bool error_emitted = false;
    while (init->op == AST_OP(braced_init)) {
      // Brace enclosed initializer for scalar.
      BracedInitializerASTNode* braced_init = (BracedInitializerASTNode*)init;
      if (braced_init->initializers->length > 1) {
        if (!error_emitted) {
          SemanticWarning(init, "braced-scalar-init",
                          "Too many initializers for scalar");
          error_emitted = true;
        }
      }
      init = (ASTNode*)braced_init->initializers->value.p[0];
    }

    if (init->op == AST_OP(designated_init)) {
      SemanticError(init, "Unexpected designated initializer for scalar");
    } else {
      state.types = malloc(1 * sizeof(TypeRecord*));
      state.types[0] = type;
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)init;
      AnalyzeExpressionInitializer(expr_init, &state);
    }
  }

  // Free up state.
  free(state.indexes);
  free(state.limits);
  free(state.types);
  return (ASTNode*)state.init;
}
#endif

