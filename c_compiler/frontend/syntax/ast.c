//
//  ast.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "ast.h"
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include "symbol.h"
#include "compiler.h"

static int next_ast_node_id = 1;

//
// AST arena allocator.
//
// All AST node structs are allocated from a bump allocator made up of a linked
// list of large blocks.  The AST is a graph (it has non-owning reference edges
// such as switch cases, goto targets and label back-pointers), so individual
// nodes cannot be safely free()d during teardown.  Instead the whole AST is
// "destructed" (releasing the non-arena resources each node owns: type
// references, owned strings/vectors) and then the arena blocks are freed in one
// shot.  See ASTNodeDelete (which is idempotent via kASTDestructed) and
// ASTArenaRelease.
typedef struct ASTArenaBlock {
  struct ASTArenaBlock* next;
  size_t used;
  size_t capacity;
  char data[];
} ASTArenaBlock;

#define AST_ARENA_BLOCK_SIZE (256 * 1024)

static ASTArenaBlock* ast_arena = NULL;

// Every node handed out by ASTArenaAlloc, in allocation order.  Used at
// teardown to destruct every node (releasing its type references and owned
// resources), including nodes that semantic analysis replaced or discarded and
// which are therefore no longer reachable from any declaration root.
static Vector ast_all_nodes;
static bool ast_all_nodes_initialized = false;

static ASTArenaBlock* NewASTArenaBlock(size_t capacity) {
  ASTArenaBlock* block = malloc(capacity + sizeof(ASTArenaBlock));
  block->next = NULL;
  block->used = 0;
  block->capacity = capacity;
  return block;
}

// Allocate node memory from the arena.  Memory is 16-byte aligned and zeroed.
void* ASTArenaAlloc(size_t size) {
  size_t aligned = (size + 15) & ~(size_t)15;
  if (ast_arena == NULL || ast_arena->used + aligned > ast_arena->capacity) {
    size_t capacity = aligned > AST_ARENA_BLOCK_SIZE ? aligned
                                                     : (size_t)AST_ARENA_BLOCK_SIZE;
    ASTArenaBlock* block = NewASTArenaBlock(capacity);
    block->next = ast_arena;
    ast_arena = block;
  }
  void* p = ast_arena->data + ast_arena->used;
  ast_arena->used += aligned;
  memset(p, 0, size);
  if (!ast_all_nodes_initialized) {
    VectorInit(&ast_all_nodes);
    ast_all_nodes_initialized = true;
  }
  VectorAppend(&ast_all_nodes, p);
  return p;
}

// Free all arena blocks.  First destruct every node so the non-arena resources
// they own (type references, owned strings/vectors) are released; this is
// idempotent (see ASTNodeDelete) so nodes already destructed via a declaration
// root are skipped.  Then free the node structs wholesale.
void ASTArenaRelease(void) {
  for (size_t i = 0; i < ast_all_nodes.length; i++) {
    ASTNodeDelete((ASTNode*)ast_all_nodes.value.p[i]);
  }
  if (ast_all_nodes_initialized) {
    VectorDestruct(&ast_all_nodes);
    ast_all_nodes_initialized = false;
  }
  ASTArenaBlock* block = ast_arena;
  while (block != NULL) {
    ASTArenaBlock* next = block->next;
    free(block);
    block = next;
  }
  ast_arena = NULL;
}

const char* ASTOpcodeName(ASTOpcode op) {
  switch (op) {
    case AST_OP(bad):
      return "bad";
    case AST_OP(number):
      return "number";
    case AST_OP(identifier):
      return "identifier";
    case AST_OP(string):
      return "string";
    case AST_OP(string_wide):
      return "wide string";
    case AST_OP(charconst):
      return "char const";
    case AST_OP(fnumber):
      return "fnumber";
    case AST_OP(postinc):
      return "++";
    case AST_OP(postdec):
      return "--";
    case AST_OP(uminus):
      return "-";
    case AST_OP(uplus):
      return "+";
    case AST_OP(contents):
      return "*";
    case AST_OP(address):
      return "&";
    case AST_OP(and):
      return "&";
    case AST_OP(andeq):
      return "&=";
    case AST_OP(arrow):
      return "->";
    case AST_OP(assign):
      return "=";
    case AST_OP(not):
      return "!";
    case AST_OP(bitor):
      return "|";
    case AST_OP(break):
      return "break";
    case AST_OP(exor):
      return "^";
    case AST_OP(exoreq):
      return "^=";
    case AST_OP(case):
      return "case";
    case AST_OP(colon):
      return ":";
    case AST_OP(comma):
      return ",";
    case AST_OP(complex):
      return "_Complex";
    case AST_OP(continue):
      return "continue";
    case AST_OP(do):
      return "do";
    case AST_OP(dot):
      return ".";
    case AST_OP(equal):
      return "==";
    case AST_OP(for):
      return "for";
    case AST_OP(goto):
      return "goto";
    case AST_OP(greater):
      return ">";
    case AST_OP(greatereq):
      return ">=";
    case AST_OP(if):
      return "if";
    case AST_OP(imaginary):
      return "_Imaginary";
    case AST_OP(compound):
      return "{";
    case AST_OP(less):
      return "<";
    case AST_OP(lesseq):
      return "<=";
    case AST_OP(logand):
      return "&&";
    case AST_OP(logor):
      return "||";
    case AST_OP(call):
    case AST_OP(inline_call):
      return "(";
    case AST_OP(lshift):
      return "<<";
    case AST_OP(lshifteq):
      return "<<=";
    case AST_OP(subscript):
      return "[";
    case AST_OP(minus):
      return "-";
    case AST_OP(minuseq):
      return "-=";
    case AST_OP(predec):
      return "--";
    case AST_OP(noteq):
      return "!=";
    case AST_OP(oreq):
      return "|=";
    case AST_OP(mod):
      return "%";
    case AST_OP(percenteq):
      return "%=";
    case AST_OP(plus):
      return "+";
    case AST_OP(pluseq):
      return "+=";
    case AST_OP(preinc):
      return "++";
    case AST_OP(question):
      return "?";
    case AST_OP(return ):
      return "return";
    case AST_OP(rshift):
    case AST_OP(rshiftl):
    case AST_OP(rshifta):
      return ">>";
    case AST_OP(rshifteq):
    case AST_OP(rshifteql):
    case AST_OP(rshifteqa):
      return ">>=";
    case AST_OP(sizeof):
      return "sizeof";
    case AST_OP(div):
      return "/";
    case AST_OP(diveq):
      return "/=";
    case AST_OP(mult):
      return "*";
    case AST_OP(multeq):
      return "*=";
    case AST_OP(switch):
      return "switch";
    case AST_OP(onescomp):
      return "~";
    case AST_OP(while):
      return "while";
    case AST_OP(structmember):
      return "structmember";
    case AST_OP(asm):
      return "__asm";
    case AST_OP(attribute):
      return "__attribute";

    case AST_OP(builtin_va_start):
      return "builtin_va_start";
    case AST_OP(builtin_va_arg):
      return "builtin_va_arg";
    case AST_OP(builtin_va_end):
      return "builtin_va_end";
    case AST_OP(builtin_va_copy):
      return "builtin_va_copy";

    case AST_OP(cast):
      return "cast";
    case AST_OP(label):
      return "label";
    case AST_OP(vardecl):
      return "variable";
    case AST_OP(decl_list):
      return "decl_list";
    case AST_OP(macro):
      return "macro";
    case AST_OP(expr):
      return "expr";
    case AST_OP(init):
      return "init";

    case AST_OP(expr_init):
      return "expr-init";
    case AST_OP(braced_init):
      return "braced-init";
    case AST_OP(designated_init):
      return "designated-init";

    case AST_OP(ptr_scale):
      return "ptr-scale";
      case AST_OP(compound_literal):
        return "compound_literal";
    case AST_OP(stmt_expr):
      return "stmt-expr";

    // Integer to...
    case AST_OP(i2s):
      return "i2s";
    case AST_OP(i2c):
      return "i2c";
    case AST_OP(i2l):
      return "i2l";
    case AST_OP(i2ll):
      return "i2ll";
    case AST_OP(i2f):
      return "i2f";
    case AST_OP(i2d):
      return "i2d";
    case AST_OP(i2ld):
      return "i2ld";
    case AST_OP(i2b):
      return "i2b";

    // Character to...
    case AST_OP(c2i):
      return "c2i";
    case AST_OP(c2s):
      return "c2s";
    case AST_OP(c2l):
      return "c2l";
    case AST_OP(c2ll):
      return "c2ll";
    case AST_OP(c2f):
      return "c2f";
    case AST_OP(c2d):
      return "c2d";
    case AST_OP(c2ld):
      return "c2ld";
    case AST_OP(c2b):
      return "c2b";

    // Short to...
    case AST_OP(s2i):
      return "s2i";
    case AST_OP(s2c):
      return "s2c";
    case AST_OP(s2l):
      return "s2l";
    case AST_OP(s2ll):
      return "s2ll";
    case AST_OP(s2f):
      return "s2f";
    case AST_OP(s2d):
      return "s2d";
    case AST_OP(s2ld):
      return "s2ld";
    case AST_OP(s2b):
      return "s2b";

    // Long to...
    case AST_OP(l2i):
      return "l2i";
    case AST_OP(l2c):
      return "l2c";
    case AST_OP(l2s):
      return "l2s";
    case AST_OP(l2ll):
      return "l2ll";
    case AST_OP(l2f):
      return "l2f";
    case AST_OP(l2d):
      return "l2d";
    case AST_OP(l2ld):
      return "l2ld";
    case AST_OP(l2b):
      return "l2b";

    // Long long to...
    case AST_OP(ll2i):
      return "ll2i";
    case AST_OP(ll2c):
      return "ll2c";
    case AST_OP(ll2s):
      return "ll2s";
    case AST_OP(ll2l):
      return "ll2l";
    case AST_OP(ll2f):
      return "ll2f";
    case AST_OP(ll2d):
      return "ll2d";
    case AST_OP(ll2ld):
      return "ll2ld";
    case AST_OP(ll2b):
      return "ll2b";

    // Float to...
    case AST_OP(f2i):
      return "f2i";
    case AST_OP(f2c):
      return "f2c";
    case AST_OP(f2s):
      return "f2s";
    case AST_OP(f2l):
      return "f2l";
    case AST_OP(f2ll):
      return "f2ll";
    case AST_OP(f2d):
      return "f2d";
    case AST_OP(f2ld):
      return "f2ld";
    case AST_OP(f2b):
      return "f2b";

    // Double to...
    case AST_OP(d2i):
      return "d2i";
    case AST_OP(d2c):
      return "d2c";
    case AST_OP(d2s):
      return "d2s";
    case AST_OP(d2l):
      return "d2l";
    case AST_OP(d2ll):
      return "d2ll";
    case AST_OP(d2f):
      return "d2f";
    case AST_OP(d2ld):
      return "d2ld";
    case AST_OP(d2b):
      return "d2b";

    // Long double to...
    case AST_OP(ld2i):
      return "ld2i";
    case AST_OP(ld2c):
      return "ld2c";
    case AST_OP(ld2s):
      return "ld2s";
    case AST_OP(ld2l):
      return "ld2l";
    case AST_OP(ld2ll):
      return "ld2ll";
    case AST_OP(ld2f):
      return "ld2f";
    case AST_OP(ld2d):
      return "ld2d";
    case AST_OP(ld2b):
      return "ld2b";

    // Bool to.
    case AST_OP(b2i):
      return "b2i";
    case AST_OP(b2c):
      return "b2c";
    case AST_OP(b2s):
      return "b2s";
    case AST_OP(b2l):
      return "b2l";
    case AST_OP(b2ll):
      return "b2ll";
    case AST_OP(b2f):
      return "b2f";
    case AST_OP(b2ld):
      return "b2ld";
    case AST_OP(b2d):
      return "b2d";
    default:
      return "<unknown>";
  }
}

// Fills in an AST node with deleter and printer functions.
void ASTNodeInit(ASTNode* node, ASTOpcode op, TypeRecord* type,
                 SourceLocation location, ASTNodeVirtuals* virtuals) {
  assert(virtuals != NULL);
  assert(op != AST_OP(bad));
  node->op = op;
  node->id = next_ast_node_id++;
  node->flags = 0;
  node->type = NULL;
  node->value_category = kValueCategoryPrvalue;
  node->parent = NULL;
  node->child_id = 0;
  node->location = location;
  node->virtuals = virtuals;
  ASTNodeSetType(node, type);
}

static void ASTNodeBaseDelete(ASTNode* node) {
  // Release the type reference this node holds.  The node struct itself is arena
  // allocated and is not freed here (see ASTNodeDelete / ASTArenaRelease).
  if (node->type != NULL) {
    TypeRecordDelete(node->type);
  }
}

static void SetParent(ASTNode* child, ASTNode* parent, int child_id) {
  if (child == NULL) {
    return;
  }
  child->parent = parent;
  child->child_id = child_id;
}

static void Indent(int indents, FILE* fp) {
  for (int i = 0; i < indents; i++) {
    fputc(' ', fp);
  }
}

static bool ValueNotUsed(ASTNode* node, ASTNode* value) {
  return false;
}

static bool ValueAlwaysUsed(ASTNode* node, ASTNode* value) {
  return true;
}

static void ASTNodeBasePrint(ASTNode* node, int indents, FILE* fp) {
  Indent(indents, fp);
  fprintf(fp,"(#%d) %s ", node->id, ASTOpcodeName(node->op));
  if (node->type != NULL) {
    TypeRecordPrint(node->type, fp);
  }
  fprintf(fp,"\n");
}

static void ASTNodeBaseCopy(ASTNode* dest, const ASTNode* src) {
  memcpy(dest, src, sizeof(ASTNode));
  if (dest->type != NULL) {
    TypeRecordIncRef(dest->type);
  }
}

static ASTNodeVirtuals base_vtbl = {ASTNodeBaseDelete, ASTNodeBasePrint, NULL,
                                    NULL, NULL, NULL};

ASTNode* NewASTNode(ASTOpcode op, TypeRecord* type, SourceLocation location) {
  ASTNode* node = ASTArenaAlloc(sizeof(ASTNode));
  ASTNodeInit(node, op, type, location, &base_vtbl);
  return node;
}

void ASTNodeDelete(ASTNode* node) {
  if (node == NULL) {
    return;
  }
  // The AST is a graph: nodes can be reached more than once (switch cases, goto
  // targets, label back-pointers, shared subtrees produced by inlining) which
  // would otherwise cause infinite recursion or double-release during teardown.
  // Destruct each node at most once.  The node struct itself is not freed here;
  // it lives in the AST arena and is reclaimed wholesale by ASTArenaRelease.
  if ((node->flags & kASTDestructed) != 0) {
    return;
  }
  node->flags |= kASTDestructed;
  assert(node->virtuals->deleter != NULL);
  node->virtuals->deleter(node);
}

void ASTNodeSetType(ASTNode* node, TypeRecord* type) {
  if (type == NULL) {
    return;
  }
  if (node->type == type) {
    // Already set, nothing to do.
    return;
  }
  if (node->type != NULL) {
    TypeRecordDelete(node->type);
  }
  node->type = type;
  TypeRecordIncRef(type);
}

void ASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  if (node == NULL) {
    return;
  }
  assert(node->virtuals->printer != NULL);
  node->virtuals->printer(node, indents, fp);
}

void ASTNodeReplaceChild(ASTNode* parent, int child_id, ASTNode* child,
                         bool delete_old_child) {
  if (parent == NULL) {
    return;
  }
  assert(parent->virtuals->replacer != NULL);
  parent->virtuals->replacer(parent, child_id, child, delete_old_child);
}

bool ASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  if (node == NULL) {
    return false;
  }
  if (node->virtuals->uses_value == NULL) {
    return false;
  }
  return node->virtuals->uses_value(node, value);
}

ASTNode* ASTNodeClone(const ASTNode* node,
                      ASTNode* (*func)(ASTNode* node, void*), void* data,
                      ASTNode* new_parent) {
  if (node == NULL) {
    return NULL;
  }
  ASTNode* clone;
  if (node->virtuals->cloner == NULL) {
    // Base class only.
    clone = ASTArenaAlloc(sizeof(ASTNode));
    ASTNodeBaseCopy(clone, node);
    clone = func(clone, data);
  } else {
    clone = node->virtuals->cloner(node, func, data);
  }
  if (clone != NULL) {
    clone->parent = new_parent;
  }
  return clone;
}

void ASTNodeVisit(ASTNode* node,
                  void (*func)(ASTNode* node, void*, int, VisitorMode),
                  int child_id,
                  void* data) {
  if (node == NULL) {
    return;
  }
  if (node->virtuals->visitor == NULL) {
    // Base class only.
    func(node, data, child_id, kVisitPreChildren);
    return;
  }
  node->virtuals->visitor(node, func, child_id, data);
}

ASTNode* ASTNodeMove(ASTNode* node) {
  ASTNodeReplaceChild(node->parent, node->child_id, NULL, false);
  node->parent = NULL;
  return node;
}

bool ASTNodeIsIntConstant(ASTNode* node) {
  switch (node->op) {
    case AST_OP(number):
    case AST_OP(sizeof):
      return true;
    default:
      return false;
  }
}

int64_t ASTNodeConstantValue(ASTNode* node) {
  switch (node->op) {
    case AST_OP(number):
      return ((ConstantASTNode*)node)->value.ivalue;
    case AST_OP(sizeof):
      return ((SizeofASTNode*)node)->base.value.ivalue;
    default:
      assert(false);
      return false;
  }
}

static void IdentifierASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  Indent(indents, fp);
  IdentifierASTNode* inode = (IdentifierASTNode*)node;
  fprintf(fp,"%s\n", inode->symbol->name.value);
  ASTNodeBasePrint(node, indents + 2, fp);
}

static ASTNode* IdentifierASTNodeClone(const ASTNode* node,
                                       ASTNode* (*func)(ASTNode* node, void*),
                                       void* data) {
  IdentifierASTNode* from = (IdentifierASTNode*)node;
  IdentifierASTNode* to = ASTArenaAlloc(sizeof(IdentifierASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->symbol = from->symbol;
  return func(&to->base, data);
}

static ASTNodeVirtuals identifier_vtbl = {ASTNodeBaseDelete,
                                          IdentifierASTNodePrint, NULL,
                                          IdentifierASTNodeClone, NULL,
                                          NULL,
};

ASTNode* NewIdentifierASTNode(Symbol* symbol, SourceLocation location) {
  IdentifierASTNode* node = ASTArenaAlloc(sizeof(IdentifierASTNode));
  ASTNodeInit(&node->base, AST_OP(identifier), symbol->type, location,
              &identifier_vtbl);
  node->symbol = symbol;
  return (ASTNode*)node;
}

ASTNode* NewRawIdentifierASTNode(void* symbol, SourceLocation location) {
  IdentifierASTNode* node = ASTArenaAlloc(sizeof(IdentifierASTNode));
  ASTNodeInit(&node->base, AST_OP(identifier), NULL, location,
              &identifier_vtbl);
  node->symbol = symbol;
  return (ASTNode*)node;
}

static void StructMemberASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  Indent(indents, fp);
  StructMemberASTNode* mnode = (StructMemberASTNode*)node;
  fprintf(fp,"%s@%d\n", mnode->member->symbol->name.value,
         mnode->member->byte_offset);
  ASTNodeBasePrint(node, indents + 2, fp);
}

static ASTNode* StructMemberASTNodeClone(const ASTNode* node,
                                         ASTNode* (*func)(ASTNode* node, void*),
                                         void* data) {
  StructMemberASTNode* from = (StructMemberASTNode*)node;
  StructMemberASTNode* to = ASTArenaAlloc(sizeof(StructMemberASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->member = from->member;
  to->access = from->access;
  return func(&to->base, data);
}

static ASTNodeVirtuals struct_member_vtbl = {ASTNodeBaseDelete,
                                             StructMemberASTNodePrint, NULL,
                                             StructMemberASTNodeClone, NULL,
                                             NULL,
};

ASTNode* NewStructMemberASTNode(StructMember* member, SourceLocation location) {
  StructMemberASTNode* node = ASTArenaAlloc(sizeof(StructMemberASTNode));
  ASTNodeInit(&node->base, AST_OP(structmember), member->symbol->type, location,
              &struct_member_vtbl);
  node->member = member;
  node->access = member->access;
  return (ASTNode*)node;
}

static void ConstantASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  Indent(indents, fp);
  ConstantASTNode* cnode = (ConstantASTNode*)node;
  switch (cnode->base.op) {
    case AST_OP(number):
      fprintf(fp,"%" PRId64 "\n", cnode->value.ivalue);
      break;
    case AST_OP(fnumber):
      fprintf(fp,"%g\n", cnode->value.fvalue);
      break;
    case AST_OP(string):
    case AST_OP(string_wide): {
      String escaped = {0};
      StringEscape(cnode->value.string, &escaped);
      fprintf(fp,"\"%s\"\n", escaped.value);
      StringDestruct(&escaped);
      break;
    }
    case AST_OP(charwide):
    case AST_OP(charconst):
      fprintf(fp,"'\\x%04x'\n", (int)cnode->value.ivalue);
      break;
    case AST_OP(label):
      fprintf(fp,"label %s\n", cnode->value.string->value);
      break;
    case AST_OP(sizeof): {
      fprintf(fp,"sizeof\n");
      SizeofASTNode* s = (SizeofASTNode*)node;
      ASTNodePrint(s->expr, indents + 2, fp);
      break;
    }
    default:
      fprintf(fp,"unknown constant op %d\n", cnode->base.op);
  }
}

// String constants own a heap-allocated String value; other constants store
// it inline in the union.
static bool ConstantOwnsString(const ASTNode* node) {
  return node->op == AST_OP(string) || node->op == AST_OP(string_wide);
}

static void ConstantASTNodeDelete(ASTNode* node) {
  ConstantASTNode* cnode = (ConstantASTNode*)node;
  if (ConstantOwnsString(node) && cnode->value.string != NULL) {
    StringDelete(cnode->value.string);
  }
  ASTNodeBaseDelete(node);
}

static ASTNode* ConstantASTNodeClone(const ASTNode* node,
                                     ASTNode* (*func)(ASTNode* node, void*),
                                     void* data) {
  ConstantASTNode* from = (ConstantASTNode*)node;
  ConstantASTNode* to = ASTArenaAlloc(sizeof(ConstantASTNode));
  ASTNodeBaseCopy(&to->base, node);
  memcpy(&to->value, &from->value, sizeof(to->value));
  // The clone needs its own copy of the owned string so each node can free it.
  if (ConstantOwnsString(node) && from->value.string != NULL) {
    to->value.string = NewString(from->value.string->value);
  }
  return func(&to->base, data);
}

static ASTNodeVirtuals constant_vtbl = {ConstantASTNodeDelete,
                                        ConstantASTNodePrint,
                                        NULL, ConstantASTNodeClone, NULL, NULL};

void IntConstantASTNodeInit(ConstantASTNode* node, int64_t value,
                            TypeRecord* type, SourceLocation location) {
  ASTNodeInit(&node->base, AST_OP(number), type, location, &constant_vtbl);
  node->value.ivalue = value;
}

ASTNode* NewIntConstantASTNode(int64_t value, TypeRecord* type,
                               SourceLocation location) {
  ConstantASTNode* node = ASTArenaAlloc(sizeof(ConstantASTNode));
  IntConstantASTNodeInit(node, value, type, location);
  return (ASTNode*)node;
}

ASTNode* NewRealConstantASTNode(double value, TypeRecord* type,
                                SourceLocation location) {
  ConstantASTNode* node = ASTArenaAlloc(sizeof(ConstantASTNode));
  ASTNodeInit(&node->base, AST_OP(fnumber), type, location, &constant_vtbl);
  node->value.fvalue = value;
  return (ASTNode*)node;
}

ASTNode* NewStringConstantASTNode(String* value, TypeRecord* type,
                                  SourceLocation location) {
  ConstantASTNode* node = ASTArenaAlloc(sizeof(ConstantASTNode));
  ASTNodeInit(&node->base, AST_OP(string), type, location, &constant_vtbl);
  node->value.string = value;
  return (ASTNode*)node;
}

ASTNode* NewWideStringConstantASTNode(String* value, TypeRecord* type,
                                      SourceLocation location) {
  ConstantASTNode* node = ASTArenaAlloc(sizeof(ConstantASTNode));
  ASTNodeInit(&node->base, AST_OP(string_wide), type, location, &constant_vtbl);
  node->value.string = value;
  return (ASTNode*)node;
}

ASTNode* NewCharConstantASTNode(int value, TypeRecord* type,
                                SourceLocation location) {
  ConstantASTNode* node = ASTArenaAlloc(sizeof(ConstantASTNode));
  ASTNodeInit(&node->base, AST_OP(charconst), type, location, &constant_vtbl);
  node->value.ivalue = value;
  return (ASTNode*)node;
}

static void UnaryASTNodeDelete(ASTNode* node) {
  UnaryASTNode* unode = (UnaryASTNode*)node;
  if (unode->sub != NULL) {
    ASTNodeDelete(unode->sub);
  }
  ASTNodeBaseDelete(node);
}

static void UnaryASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  ASTNodeBasePrint(node, indents, fp);
  UnaryASTNode* unode = (UnaryASTNode*)node;
  if (unode->sub != NULL) {
    ASTNodePrint(unode->sub, indents + 2, fp);
  }
}

static void UnaryASTNodeReplaceChild(ASTNode* parent, int child_id,
                                     ASTNode* child, bool delete_old_child) {
  UnaryASTNode* node = (UnaryASTNode*)parent;
  ASTNode* old = node->sub;
  node->sub = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* UnaryASTNodeClone(const ASTNode* node,
                                  ASTNode* (*func)(ASTNode* node, void*),
                                  void* data) {
  UnaryASTNode* from = (UnaryASTNode*)node;
  UnaryASTNode* to = ASTArenaAlloc(sizeof(UnaryASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->sub = ASTNodeClone(from->sub, func, data, &to->base);
  return func(&to->base, data);
}

static void UnaryASTNodeVisit(ASTNode* node,
                              void (*func)(ASTNode* node, void*, int, VisitorMode),
                              int child_id,
                              void* data) {
  UnaryASTNode* n = (UnaryASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->sub, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}


static ASTNodeVirtuals unary_vtbl = {UnaryASTNodeDelete, UnaryASTNodePrint,
                                     UnaryASTNodeReplaceChild,
                                     UnaryASTNodeClone, UnaryASTNodeVisit, ValueAlwaysUsed};

// Unary AST node with a single child.
ASTNode* NewUnaryASTNode(ASTOpcode op, TypeRecord* type,
                         SourceLocation location, ASTNode* sub) {
  UnaryASTNode* node = ASTArenaAlloc(sizeof(UnaryASTNode));
  ASTNodeInit(&node->base, op, type, location, &unary_vtbl);
  node->sub = sub;
  sub->parent = (ASTNode*)node;
  return (ASTNode*)node;
}

static void BinaryASTNodeDelete(ASTNode* node) {
  BinaryASTNode* bnode = (BinaryASTNode*)node;
  if (bnode->left != NULL) {
    ASTNodeDelete(bnode->left);
  }
  if (bnode->right != NULL) {
    ASTNodeDelete(bnode->right);
  }
  ASTNodeBaseDelete(node);
}

static void BinaryASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  BinaryASTNode* bnode = (BinaryASTNode*)node;
  if (bnode->right != NULL) {
    ASTNodePrint(bnode->right, indents + 2, fp);
  }
  ASTNodeBasePrint(node, indents, fp);
  if (bnode->left != NULL) {
    ASTNodePrint(bnode->left, indents + 1, fp);
  }
}

static void BinaryASTNodeReplaceChild(ASTNode* parent, int child_id,
                                      ASTNode* child, bool delete_old_child) {
  BinaryASTNode* node = (BinaryASTNode*)parent;
  ASTNode* old = NULL;
  switch (child_id) {
    case 0:  // Left child.
      old = node->left;
      node->left = child;
      break;
    case 1:  // Right child.
      old = node->right;
      node->right = child;
      break;
  }
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* BinaryASTNodeClone(const ASTNode* node,
                                   ASTNode* (*func)(ASTNode* node, void*),
                                   void* data) {
  BinaryASTNode* from = (BinaryASTNode*)node;
  BinaryASTNode* to = ASTArenaAlloc(sizeof(BinaryASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->left = ASTNodeClone(from->left, func, data, &to->base);
  to->right = ASTNodeClone(from->right, func, data, &to->base);
  return func(&to->base, data);
}

static void BinaryASTNodeVisit(ASTNode* node,
                               void (*func)(ASTNode* node, void*, int, VisitorMode),
                               int child_id,
                               void* data) {
  BinaryASTNode* n = (BinaryASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->left, func, 0, data);
  ASTNodeVisit(n->right, func, 1, data);
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals binary_vtbl = {BinaryASTNodeDelete, BinaryASTNodePrint,
                                      BinaryASTNodeReplaceChild,
                                      BinaryASTNodeClone, BinaryASTNodeVisit,
  ValueAlwaysUsed
};

// Binary AST node, which a left and right child.
ASTNode* NewBinaryASTNode(ASTOpcode op, TypeRecord* type,
                          SourceLocation location, ASTNode* left,
                          ASTNode* right) {
  BinaryASTNode* node = ASTArenaAlloc(sizeof(BinaryASTNode));
  ASTNodeInit(&node->base, op, type, location, &binary_vtbl);
  node->left = left;
  left->parent = (ASTNode*)node;
  left->child_id = 0;
  node->right = right;
  right->parent = (ASTNode*)node;
  right->child_id = 1;
  return (ASTNode*)node;
}

// Inlined call

static void InlineCallASTNodeDelete(ASTNode* node) {
  InlineCallASTNode* bnode = (InlineCallASTNode*)node;
  if (bnode->inlined != NULL) {
    ASTNodeDelete(bnode->inlined);
  }
  if (bnode->ret_value != NULL) {
    ASTNodeDelete(bnode->ret_value);
  }
  ASTNodeBaseDelete(node);
}

static void InlineCallASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  InlineCallASTNode* cnode = (InlineCallASTNode*)node;
  if (cnode->inlined != NULL) {
    ASTNodePrint(cnode->inlined, indents + 2, fp);
  }
  ASTNodeBasePrint(node, indents, fp);
  if (cnode->ret_value != NULL) {
    ASTNodePrint(cnode->ret_value, indents + 1, fp);
  }
}

static void InlineCallASTNodeReplaceChild(ASTNode* parent, int child_id,
                                          ASTNode* child,
                                          bool delete_old_child) {
  InlineCallASTNode* node = (InlineCallASTNode*)parent;
  ASTNode* old = NULL;
  switch (child_id) {
    case 0:  // Inlined child.
      old = node->inlined;
      node->inlined = child;
      break;
    case 1:  // ret_value child.
      old = node->ret_value;
      node->ret_value = child;
      break;
  }
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* InlineCallASTNodeClone(const ASTNode* node,
                                       ASTNode* (*func)(ASTNode* node, void*),
                                       void* data) {
  InlineCallASTNode* from = (InlineCallASTNode*)node;
  InlineCallASTNode* to = ASTArenaAlloc(sizeof(InlineCallASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->inlined = ASTNodeClone(from->inlined, func, data, &to->base);
  to->ret_value = ASTNodeClone(from->ret_value, func, data, &to->base);
  return func(&to->base, data);
}

static void InlineCallASTNodeVisit(ASTNode* node,
                                   void (*func)(ASTNode* node, void*,
                                                int, VisitorMode),
                                   int child_id,
                                   void* data) {
  InlineCallASTNode* n = (InlineCallASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->inlined, func, 0, data);
  ASTNodeVisit(n->ret_value, func, 1, data);
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals inline_call_vtbl = {
    InlineCallASTNodeDelete, InlineCallASTNodePrint,
    InlineCallASTNodeReplaceChild, InlineCallASTNodeClone,
    InlineCallASTNodeVisit, ValueAlwaysUsed};

ASTNode* NewInlineCallASTNode(TypeRecord* type, SourceLocation location,
                              ASTNode* inlined, ASTNode* ret_value) {
  InlineCallASTNode* node = ASTArenaAlloc(sizeof(InlineCallASTNode));
  ASTNodeInit(&node->base, AST_OP(inline_call), type, location,
              &inline_call_vtbl);
  node->inlined = inlined;
  inlined->parent = (ASTNode*)node;
  inlined->child_id = 0;
  node->ret_value = ret_value;
  if (ret_value != NULL) {
    ret_value->parent = (ASTNode*)node;
    ret_value->child_id = 1;
  }
  return (ASTNode*)node;
}

static void VectorASTNodeDelete(ASTNode* node) {
  VectorASTNode* vnode = (VectorASTNode*)node;
  if (vnode->left != NULL) {
    ASTNodeDelete(vnode->left);
  }
  for (size_t i = 0; i < vnode->children->length; i++) {
    ASTNode* child = (ASTNode*)vnode->children->value.p[i];
    if (child != NULL) {
      ASTNodeDelete(child);
    }
  }
  // children is heap-allocated (NewVector), so free the struct too.
  VectorDelete(vnode->children);
  ASTNodeBaseDelete(node);
}

static void VectorASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  VectorASTNode* vnode = (VectorASTNode*)node;
  if (vnode->left != NULL) {
    ASTNodePrint(vnode->left, indents + 2, fp);
  }
  ASTNodeBasePrint(node, indents, fp);
  size_t num_children = vnode->children->length;
  for (size_t i = 0; i < num_children; i++) {
    Indent(indents + 2, fp);
    fprintf(fp,"[%zd]:\n", i);
    ASTNodePrint((ASTNode*)vnode->children->value.p[i], indents + 4, fp);
  }
}

static void VectorASTNodeReplaceChild(ASTNode* parent, int child_id,
                                      ASTNode* child, bool delete_old_child) {
  VectorASTNode* node = (VectorASTNode*)parent;
  ASTNode* old = node->children->value.p[child_id];
  node->children->value.p[child_id] = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* VectorASTNodeClone(const ASTNode* node,
                                   ASTNode* (*func)(ASTNode* node, void*),
                                   void* data) {
  VectorASTNode* from = (VectorASTNode*)node;
  VectorASTNode* to = ASTArenaAlloc(sizeof(VectorASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->left = ASTNodeClone(from->left, func, data, &to->base);
  to->children = NewVector();
  for (size_t i = 0; i < from->children->length; i++) {
    ASTNode* child =
        ASTNodeClone(from->children->value.p[i], func, data, &to->base);
    VectorAppend(to->children, child);
  }
  return func(&to->base, data);
}

static void VectorASTNodeVisit(ASTNode* node,
                               void (*func)(ASTNode* node, void*,int,  VisitorMode),
                               int child_id,
                               void* data) {
  VectorASTNode* n = (VectorASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->left, func, 0, data);
  for (size_t i = 0; i < n->children->length; i++) {
    ASTNodeVisit(n->children->value.p[i], func, (int)i+1, data);
  }
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals vector_vtbl = {VectorASTNodeDelete, VectorASTNodePrint,
                                      VectorASTNodeReplaceChild,
                                      VectorASTNodeClone, VectorASTNodeVisit, ValueAlwaysUsed};

ASTNode* NewVectorASTNode(ASTOpcode op, TypeRecord* type,
                          SourceLocation location, ASTNode* left,
                          Vector* children) {
  VectorASTNode* node = ASTArenaAlloc(sizeof(VectorASTNode));
  ASTNodeInit(&node->base, op, type, location, &vector_vtbl);
  node->left = left;
  left->parent = &node->base;
  node->children = children;
  for (size_t i = 0; i < children->length; i++) {
    ASTNode* child = children->value.p[i];
    child->parent = (ASTNode*)node;
    child->child_id = (int)i;
  }
  return (ASTNode*)node;
}

// Cast AST Node.
static void CastASTNodeDelete(ASTNode* node) {
  CastASTNode* cnode = (CastASTNode*)node;
  if (cnode->cast_type != NULL) {
    TypeRecordDelete(cnode->cast_type);
  }
  if (cnode->expr != NULL) {
    ASTNodeDelete(cnode->expr);
  }
  ASTNodeBaseDelete(node);
}

static void CastASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  CastASTNode* cnode = (CastASTNode*)node;
  Indent(indents, fp);
  fprintf(fp,"cast\n");
  Indent(indents, fp);
  TypeRecordPrint(cnode->cast_type, fp);
  if (cnode->expr != NULL) {
    ASTNodePrint(cnode->expr, indents + 2, fp);
  }
}

static void CastASTNodeReplaceChild(ASTNode* parent, int child_id,
                                    ASTNode* child, bool delete_old_child) {
  CastASTNode* node = (CastASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* CastASTNodeClone(const ASTNode* node,
                                 ASTNode* (*func)(ASTNode* node, void*),
                                 void* data) {
  CastASTNode* from = (CastASTNode*)node;
  CastASTNode* to = ASTArenaAlloc(sizeof(CastASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->cast_type = from->cast_type;
  to->expr = ASTNodeClone(from->expr, func, data, &to->base);
  to->kind = from->kind;
  TypeRecordIncRef(to->cast_type);
  return func(&to->base, data);
}

static bool CastASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  CastASTNode* n = (CastASTNode*)node;
  if (TypeIsVoid(n->cast_type)) {
    return false;
  }
  return ASTNodeUsesValue(node->parent, node);
}

static ASTNodeVirtuals cast_vtbl = {CastASTNodeDelete, CastASTNodePrint,
                                    CastASTNodeReplaceChild, CastASTNodeClone, NULL,
  CastASTNodeUsesValue
};

ASTNode* NewCastASTNode(TypeRecord* type, SourceLocation location,
                        ASTNode* expr) {
  CastASTNode* node = ASTArenaAlloc(sizeof(CastASTNode));
  ASTNodeInit(&node->base, AST_OP(cast), NULL, location, &cast_vtbl);
  node->cast_type = type;
  TypeRecordIncRef(type);
  node->expr = expr;
  node->kind = kCastCStyle;
  expr->parent = (ASTNode*)node;
  return (ASTNode*)node;
}

// Pointer scale AST node
static void PtrScaleASTNodeDelete(ASTNode* node) {
  PtrScaleASTNode* cnode = (PtrScaleASTNode*)node;
  if (cnode->ref_type != NULL) {
    TypeRecordDelete(cnode->ref_type);
  }
  if (cnode->expr != NULL) {
    ASTNodeDelete(cnode->expr);
  }
  ASTNodeBaseDelete(node);
}

static void PtrScaleASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  PtrScaleASTNode* cnode = (PtrScaleASTNode*)node;
  Indent(indents, fp);
  fprintf(fp,"ptr-scale\n");
  Indent(indents, fp);
  TypeRecordPrint(cnode->ref_type, fp);
  if (cnode->expr != NULL) {
    ASTNodePrint(cnode->expr, indents + 2, fp);
  }
}

static void PtrScaleASTNodeReplaceChild(ASTNode* parent, int child_id,
                                    ASTNode* child, bool delete_old_child) {
  PtrScaleASTNode* node = (PtrScaleASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* PtrScaleASTNodeClone(const ASTNode* node,
                                 ASTNode* (*func)(ASTNode* node, void*),
                                 void* data) {
  PtrScaleASTNode* from = (PtrScaleASTNode*)node;
  PtrScaleASTNode* to = ASTArenaAlloc(sizeof(PtrScaleASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->ref_type = from->ref_type;
  to->scale_op = from->scale_op;
  to->expr = ASTNodeClone(from->expr, func, data, &to->base);
  TypeRecordIncRef(to->ref_type);
  return func(&to->base, data);
}

static bool PtrScaleASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  return ASTNodeUsesValue(node->parent, node);
}

static ASTNodeVirtuals ptr_scale_vtbl = {PtrScaleASTNodeDelete, PtrScaleASTNodePrint,
                                    PtrScaleASTNodeReplaceChild, PtrScaleASTNodeClone, NULL,
  PtrScaleASTNodeUsesValue,
};

ASTNode* NewPtrScaleASTNode(TypeRecord* type, ASTOpcode scale_op, ASTNode* expr,
                            SourceLocation location) {
  PtrScaleASTNode* node = ASTArenaAlloc(sizeof(PtrScaleASTNode));
  ASTNodeInit(&node->base, AST_OP(ptr_scale), NULL, location, &ptr_scale_vtbl);
  node->ref_type = type;
  node->scale_op = scale_op;
  TypeRecordIncRef(type);
  node->expr = expr;
  expr->parent = (ASTNode*)node;
  return (ASTNode*)node;
}


// Sizeof AST Node

void SizeofASTNodeDelete(ASTNode* node) {
  SizeofASTNode* snode = (SizeofASTNode*)node;
  if (snode->expr != NULL) {
    ASTNodeDelete(snode->expr);
  }
  ASTNodeBaseDelete(node);
}

void SizeofASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  SizeofASTNode* snode = (SizeofASTNode*)node;
  fprintf(fp,"sizeof ");
  if (snode->expr != NULL) {
    ASTNodePrint(snode->expr, indents + 2, fp);
  } else {
    ConstantASTNodePrint((ASTNode*)&snode->base, indents + 2, fp);
  }
}

void SizeofASTNodeReplaceChild(ASTNode* parent, int child_id, ASTNode* child,
                               bool delete_old_child) {
  SizeofASTNode* node = (SizeofASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* SizeofASTNodeClone(const ASTNode* node,
                                   ASTNode* (*func)(ASTNode* node, void*),
                                   void* data) {
  SizeofASTNode* from = (SizeofASTNode*)node;
  SizeofASTNode* to = ASTArenaAlloc(sizeof(SizeofASTNode));
  memcpy(&to->base, &from->base, sizeof(to->base));
  to->expr = ASTNodeClone(from->expr, func, data, &to->base.base);
  return func(&to->base.base, data);
}

static ASTNodeVirtuals sizeof_vtbl = {SizeofASTNodeDelete, SizeofASTNodePrint,
                                      SizeofASTNodeReplaceChild,
                                      SizeofASTNodeClone, NULL, ValueAlwaysUsed};

ASTNode* NewSizeofASTNodeWithKnownSize(int size, SourceLocation location) {
  SizeofASTNode* node = ASTArenaAlloc(sizeof(SizeofASTNode));
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnsigned, kQualConst);
  IntConstantASTNodeInit(&node->base, size, type, location);
  node->base.base.virtuals = &sizeof_vtbl;
  node->expr = NULL;
  node->base.base.op = AST_OP(sizeof);
  return (ASTNode*)node;
}

ASTNode* NewSizeofASTNodeWithExpression(ASTNode* expr,
                                        SourceLocation location) {
  SizeofASTNode* node = ASTArenaAlloc(sizeof(SizeofASTNode));
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnsigned, kQualConst);
  IntConstantASTNodeInit(&node->base, 0, type, location);
  node->base.base.virtuals = &sizeof_vtbl;
  node->base.base.op = AST_OP(sizeof);
  node->expr = expr;
  expr->parent = (ASTNode*)node;
  return (ASTNode*)node;
}

static void MacroNameASTNodeDelete(ASTNode* node) {
  MacroNameASTNode* mnode = (MacroNameASTNode*)node;
  StringDestruct(&mnode->macro_name);
  ASTNodeBaseDelete(node);
}

static void MacroNameASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  MacroNameASTNode* mnode = (MacroNameASTNode*)node;
  fprintf(fp,"macro: %s", mnode->macro_name.value);
}

static ASTNode* MacroNameASTNodeClone(const ASTNode* node,
                                      ASTNode* (*func)(ASTNode* node, void*),
                                      void* data) {
  MacroNameASTNode* from = (MacroNameASTNode*)node;
  MacroNameASTNode* to = ASTArenaAlloc(sizeof(MacroNameASTNode));
  ASTNodeBaseCopy(&to->base, node);
  StringInit(&to->macro_name, from->macro_name.value);
  return func(&to->base, data);
}

static ASTNodeVirtuals macro_vtbl = {
    MacroNameASTNodeDelete, MacroNameASTNodePrint, NULL, MacroNameASTNodeClone, NULL, NULL};

ASTNode* NewMacroNameASTNode(String* macro_name, SourceLocation location) {
  MacroNameASTNode* node = ASTArenaAlloc(sizeof(MacroNameASTNode));
  ASTNodeInit(&node->base, AST_OP(macro), NULL, location, &macro_vtbl);
  StringInit(&node->macro_name, macro_name->value);
  return (ASTNode*)node;
}

static void GotoStatementASTNodeDelete(ASTNode* node) {
  GotoStatementASTNode* g = (GotoStatementASTNode*)node;
  StringDelete(g->label_name);
  ASTNodeBaseDelete(node);
}

static void GotoStatementASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  GotoStatementASTNode* g = (GotoStatementASTNode*)node;
  Indent(indents, fp);
  fprintf(fp,"%s\n", g->label_name->value);
  ASTNodeBasePrint(node, indents + 2, fp);
}

static ASTNode* GotoStatementASTNodeClone(const ASTNode* node,
                                 ASTNode* (*func)(ASTNode* node, void*),
                                 void* data) {
  GotoStatementASTNode* from = (GotoStatementASTNode*)node;
  GotoStatementASTNode* to = ASTArenaAlloc(sizeof(GotoStatementASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->label_name = NewString(from->label_name->value);
  
  // Need to analyze this again.
  to->label = NULL;
  to->base.flags &= ~kASTAnalyzed;
  to->lca = NULL;

  return func(&to->base, data);
}

static ASTNodeVirtuals goto_vtbl = {GotoStatementASTNodeDelete, GotoStatementASTNodePrint, NULL,
                                    GotoStatementASTNodeClone, NULL, NULL};

ASTNode* NewGotoStatementASTNode(String* label_name, SourceLocation location) {
  GotoStatementASTNode* node = ASTArenaAlloc(sizeof(GotoStatementASTNode));
  ASTNodeInit(&node->base, AST_OP(goto), NULL, location, &goto_vtbl);
  node->label_name = label_name;
  node->label = NULL;
  node->lca = NULL;
  return (ASTNode*)node;
}

//
//  Statement AST
//

static void ExpressionStatementASTNodeDelete(ASTNode* node) {
  ExpressionStatementASTNode* enode = (ExpressionStatementASTNode*)node;
  if (enode->expr != NULL) {
    ASTNodeDelete(enode->expr);
  }
  ASTNodeBaseDelete(node);
}

static void ExpressionStatementASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  ExpressionStatementASTNode* enode = (ExpressionStatementASTNode*)node;
  if (enode->expr != NULL) {
    ASTNodePrint(enode->expr, indents + 2, fp);
  }
}

static void ExpressionStatementASTNodeReplaceChild(ASTNode* parent,
                                                   int child_id, ASTNode* child,
                                                   bool delete_old_child) {
  ExpressionStatementASTNode* node = (ExpressionStatementASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  SetParent(child, parent, child_id);

  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* ExpressionStatementASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  ExpressionStatementASTNode* from = (ExpressionStatementASTNode*)node;
  ExpressionStatementASTNode* to = ASTArenaAlloc(sizeof(ExpressionStatementASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->expr = ASTNodeClone(from->expr, func, data, &to->base);
  return func(&to->base, data);
}

static void ExpressionStatementASTNodeVisit(
    ASTNode* node, void (*func)(ASTNode*, void*, int, VisitorMode),
                                            int child_id,
                                            void* data) {
  ExpressionStatementASTNode* n = (ExpressionStatementASTNode*)node;
  func(node, data, child_id,  kVisitPreChildren);
  ASTNodeVisit(n->expr, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals expr_stmt_vtbl = {
    ExpressionStatementASTNodeDelete, ExpressionStatementASTNodePrint,
    ExpressionStatementASTNodeReplaceChild, ExpressionStatementASTNodeClone,
    ExpressionStatementASTNodeVisit, ValueNotUsed};

ASTNode* NewExpressionStatementASTNode(ASTNode* expr, SourceLocation location) {
  ExpressionStatementASTNode* node = ASTArenaAlloc(sizeof(ExpressionStatementASTNode));
  ASTNodeInit(&node->base, AST_OP(expr), NULL, location, &expr_stmt_vtbl);
  node->expr = expr;
  expr->parent = (ASTNode*)node;
  return (ASTNode*)node;
}

static void IfStatementASTNodeDelete(ASTNode* node) {
  IfStatementASTNode* enode = (IfStatementASTNode*)node;
  if (enode->cond != NULL) {
    ASTNodeDelete(enode->cond);
  }
  if (enode->if_part != NULL) {
    ASTNodeDelete(enode->if_part);
  }
  if (enode->else_part != NULL) {
    ASTNodeDelete(enode->else_part);
  }
  ASTNodeBaseDelete(node);
}

static void IfStatementASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  IfStatementASTNode* enode = (IfStatementASTNode*)node;
  if (enode->cond != NULL) {
    ASTNodePrint(enode->cond, indents + 2, fp);
  }

  ASTNodeBasePrint(&enode->base, indents, fp);

  if (enode->if_part != NULL) {
    ASTNodePrint(enode->if_part, indents + 2, fp);
  }
  Indent(indents, fp);
  fprintf(fp,"else\n");
  if (enode->else_part != NULL) {
    ASTNodePrint(enode->else_part, indents + 2, fp);
  }
}

static void IfStatementASTNodeReplaceChild(ASTNode* parent, int child_id,
                                           ASTNode* child,
                                           bool delete_old_child) {
  IfStatementASTNode* node = (IfStatementASTNode*)parent;
  ASTNode* old = NULL;
  switch (child_id) {
    case 0:  // Condition.
      old = node->cond;
      node->cond = child;
      break;
    case 1:  // If part.
      old = node->if_part;
      node->if_part = child;
      break;
    case 2:  // Else part.
      old = node->else_part;
      node->else_part = child;
      break;
  }
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* IfStatementASTNodeClone(const ASTNode* node,
                                        ASTNode* (*func)(ASTNode* node, void*),
                                        void* data) {
  IfStatementASTNode* from = (IfStatementASTNode*)node;
  IfStatementASTNode* to = ASTArenaAlloc(sizeof(IfStatementASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->cond = ASTNodeClone(from->cond, func, data, &to->base);
  to->if_part = ASTNodeClone(from->if_part, func, data, &to->base);
  to->else_part = ASTNodeClone(from->else_part, func, data, &to->base);
  return func(&to->base, data);
}

static void IfStatementASTNodeVisit(ASTNode* node,
                                    void (*func)(ASTNode* node, void*,
                                                 int, VisitorMode),
                                    int child_id,
                                    void* data) {
  IfStatementASTNode* n = (IfStatementASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->cond, func, 0, data);
  ASTNodeVisit(n->if_part, func, 1, data);
  ASTNodeVisit(n->else_part, func, 2, data);
  func(node, data, child_id, kVisitPostChildren);
}

static bool IfStatementUsesValue(ASTNode* node, ASTNode* value) {
  IfStatementASTNode* n = (IfStatementASTNode*)node;
  if (value == n->cond) {
    return true;
  }
  return false;
}

static ASTNodeVirtuals if_stmt_vtbl = {
    IfStatementASTNodeDelete, IfStatementASTNodePrint,
    IfStatementASTNodeReplaceChild, IfStatementASTNodeClone,
    IfStatementASTNodeVisit, IfStatementUsesValue};

ASTNode* NewIfStatementASTNode(ASTNode* cond, ASTNode* if_part,
                               ASTNode* else_part, SourceLocation location) {
  IfStatementASTNode* node = ASTArenaAlloc(sizeof(IfStatementASTNode));
  ASTNodeInit(&node->base, AST_OP(if), NULL, location, &if_stmt_vtbl);
  node->cond = cond;
  cond->parent = (ASTNode*)node;
  cond->child_id = 0;
  node->if_part = if_part;
  if_part->parent = (ASTNode*)node;
  if_part->child_id = 1;
  node->else_part = else_part;
  if (else_part != NULL) {
    else_part->parent = (ASTNode*)node;
    else_part->child_id = 2;
  }
  return (ASTNode*)node;
}

static void CombinedStatementASTNodeDelete(ASTNode* node) {
  CombinedStatementASTNode* enode = (CombinedStatementASTNode*)node;
  if (enode->cond != NULL) {
    ASTNodeDelete(enode->cond);
  }
  if (enode->stmt != NULL) {
    ASTNodeDelete(enode->stmt);
  }
  ASTNodeBaseDelete(node);
}

static void CombinedStatementASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  CombinedStatementASTNode* enode = (CombinedStatementASTNode*)node;
  if (enode->cond != NULL) {
    ASTNodePrint(enode->cond, indents + 2, fp);
  }

  ASTNodeBasePrint(&enode->base, indents, fp);

  if (enode->stmt != NULL) {
    ASTNodePrint(enode->stmt, indents + 2, fp);
  }
}

static void CombinedStatementASTNodeReplaceChild(ASTNode* parent, int child_id,
                                                 ASTNode* child,
                                                 bool delete_old_child) {
  CombinedStatementASTNode* node = (CombinedStatementASTNode*)parent;
  ASTNode* old = NULL;
  switch (child_id) {
    case 0:  // Condition.
      old = node->cond;
      node->cond = child;
      break;
    case 1:  // Stmt.
      old = node->stmt;
      node->stmt = child;
      break;
  }
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* CombinedStatementASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  CombinedStatementASTNode* from = (CombinedStatementASTNode*)node;
  CombinedStatementASTNode* to = ASTArenaAlloc(sizeof(CombinedStatementASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->cond = ASTNodeClone(from->cond, func, data, &to->base);
  to->stmt = ASTNodeClone(from->stmt, func, data, &to->base);
  return func(&to->base, data);
}

static void CombinedStatementASTNodeVisit(ASTNode* node,
                                          void (*func)(ASTNode* node, void*,
                                                      int,  VisitorMode),
                                          int child_id,
                                          void* data) {
  CombinedStatementASTNode* n = (CombinedStatementASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->cond, func, 0, data);
  ASTNodeVisit(n->stmt, func, 1, data);
  func(node, data, child_id, kVisitPostChildren);
}

static bool CombinedStatementASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  CombinedStatementASTNode* n = (CombinedStatementASTNode*)node;
  return value == n->cond;
}

static ASTNodeVirtuals combined_stmt_vtbl = {
    CombinedStatementASTNodeDelete, CombinedStatementASTNodePrint,
    CombinedStatementASTNodeReplaceChild, CombinedStatementASTNodeClone,
  CombinedStatementASTNodeVisit, CombinedStatementASTNodeUsesValue};

ASTNode* NewCombinedStatementASTNode(ASTOpcode tok, ASTNode* cond,
                                     ASTNode* stmt, SourceLocation location) {
  CombinedStatementASTNode* node = ASTArenaAlloc(sizeof(CombinedStatementASTNode));
  ASTNodeInit(&node->base, tok, NULL, location, &combined_stmt_vtbl);
  node->cond = cond;
  node->stmt = stmt;
  if (cond != NULL) {
    cond->parent = (ASTNode*)node;
    cond->child_id = 0;
  }
  if (stmt != NULL) {
    stmt->parent = (ASTNode*)node;
    stmt->child_id = 1;
  }
  return (ASTNode*)node;
}

static void CompoundStatementASTNodeDelete(ASTNode* node) {
  CompoundStatementASTNode* vnode = (CompoundStatementASTNode*)node;
  for (size_t i = 0; i < vnode->statements->length; i++) {
    ASTNode* stmt = (ASTNode*)vnode->statements->value.p[i];
    if (stmt != NULL) {
      ASTNodeDelete(stmt);
    }
  }
  // statements is heap-allocated (NewVector), so free the struct too.
  VectorDelete(vnode->statements);
  ASTNodeBaseDelete(node);
}

static void CompoundStatementASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  CompoundStatementASTNode* vnode = (CompoundStatementASTNode*)node;
  for (size_t i = 0; i < vnode->statements->length; i++) {
    ASTNode* stmt = (ASTNode*)vnode->statements->value.p[i];
    if (stmt != NULL) {
      ASTNodePrint(stmt, indents, fp);
    }
  }
}

static void CompoundStatementASTNodeReplaceChild(ASTNode* parent, int child_id,
                                                 ASTNode* child,
                                                 bool delete_old_child) {
  CompoundStatementASTNode* node = (CompoundStatementASTNode*)parent;
  ASTNode* old = node->statements->value.p[child_id];
  node->statements->value.p[child_id] = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* CompoundStatementASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  CompoundStatementASTNode* from = (CompoundStatementASTNode*)node;
  CompoundStatementASTNode* to = ASTArenaAlloc(sizeof(CompoundStatementASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->statements = NewVector();
  for (size_t i = 0; i < from->statements->length; i++) {
    ASTNode* child =
        ASTNodeClone(from->statements->value.p[i], func, data, &to->base);
    VectorAppend(to->statements, child);
  }
  return func(&to->base, data);
}

static void CompoundStatementASTNodeVisit(ASTNode* node,
                                          void (*func)(ASTNode* node, void*,
                                                       int, VisitorMode),
                                          int child_id,
                                          void* data) {
  CompoundStatementASTNode* n = (CompoundStatementASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  for (size_t i = 0; i < n->statements->length; i++) {
    ASTNodeVisit(n->statements->value.p[i], func, (int)i, data);
  }
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals compound_stmt_vtbl = {
    CompoundStatementASTNodeDelete, CompoundStatementASTNodePrint,
    CompoundStatementASTNodeReplaceChild, CompoundStatementASTNodeClone,
    CompoundStatementASTNodeVisit, ValueNotUsed};

// Compound statement.
ASTNode* NewCompoundStatementASTNode(Vector* statements,
                                     SourceLocation location) {
  CompoundStatementASTNode* node = ASTArenaAlloc(sizeof(CompoundStatementASTNode));
  ASTNodeInit(&node->base, AST_OP(compound), NULL, location,
              &compound_stmt_vtbl);
  node->statements = statements;
  for (size_t i = 0; i < node->statements->length; i++) {
    ASTNode* stmt = (ASTNode*)node->statements->value.p[i];
    stmt->parent = (ASTNode*)node;
    stmt->child_id = (int)i;
  }
  node->low_pc = VectorFirst(statements);
  node->high_pc = VectorLast(statements);
  return (ASTNode*)node;
}

void CompoundASTNodeInsertStatement(CompoundStatementASTNode* node,
                                    ASTNode* stmt, size_t at_index) {
  VectorInsertBefore(node->statements, at_index, stmt);
  stmt->parent = &node->base;
  stmt->child_id = (int)at_index;
  
  // The child ids for all statements have now changed.
  // Fix them.
  for (size_t i = at_index + 1; i < node->statements->length; i++) {
    ASTNode* child = node->statements->value.p[i];
    child->child_id++;
  }
}

static void ForStatementASTNodeDelete(ASTNode* node) {
  ForStatementASTNode* enode = (ForStatementASTNode*)node;
  if (enode->c1 != NULL) {
    ASTNodeDelete(enode->c1);
  }
  if (enode->c2 != NULL) {
    ASTNodeDelete(enode->c2);
  }
  if (enode->c3 != NULL) {
    ASTNodeDelete(enode->c3);
  }
  if (enode->stmt != NULL) {
    ASTNodeDelete(enode->stmt);
  }
  ASTNodeBaseDelete(node);
}

static void ForStatementASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  ForStatementASTNode* enode = (ForStatementASTNode*)node;
  Indent(indents, fp);
  fprintf(fp,"for\n");
  Indent(indents + 2, fp);
  if (enode->c1 != NULL) {
    ASTNodePrint(enode->c1, indents + 2, fp);
  }
  Indent(indents, fp);
  fprintf(fp,";\n");
  if (enode->c2 != NULL) {
    ASTNodePrint(enode->c2, indents + 2, fp);
  }
  Indent(indents, fp);
  fprintf(fp,";\n");
  if (enode->c3 != NULL) {
    ASTNodePrint(enode->c3, indents + 2, fp);
  }
  if (enode->stmt != NULL) {
    ASTNodePrint(enode->stmt, indents + 4, fp);
  }
}

static void ForStatementASTNodeReplaceChild(ASTNode* parent, int child_id,
                                            ASTNode* child,
                                            bool delete_old_child) {
  ForStatementASTNode* node = (ForStatementASTNode*)parent;
  ASTNode* old = NULL;
  switch (child_id) {
    case 0:  // e1.
      old = node->c1;
      node->c1 = child;
      break;
    case 1:  // e2.
      old = node->c2;
      node->c2 = child;
      break;
    case 2:  // e3.
      old = node->c3;
      node->c3 = child;
      break;
    case 3:  // stmt.
      old = node->stmt;
      node->stmt = child;
      break;
  }
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* ForStatementASTNodeClone(const ASTNode* node,
                                         ASTNode* (*func)(ASTNode* node, void*),
                                         void* data) {
  ForStatementASTNode* from = (ForStatementASTNode*)node;
  ForStatementASTNode* to = ASTArenaAlloc(sizeof(ForStatementASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->c1 = ASTNodeClone(from->c1, func, data, &to->base);
  to->c2 = ASTNodeClone(from->c2, func, data, &to->base);
  to->c3 = ASTNodeClone(from->c3, func, data, &to->base);
  to->stmt = ASTNodeClone(from->stmt, func, data, &to->base);
  return func(&to->base, data);
}

static void ForStatementASTNodeVisit(ASTNode* node,
                                     void (*func)(ASTNode* node, void*,
                                                  int, VisitorMode),
                                     int child_id,
                                     void* data) {
  ForStatementASTNode* n = (ForStatementASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->c1, func, 0, data);
  ASTNodeVisit(n->c2, func, 1, data);
  ASTNodeVisit(n->c3, func, 2, data);
  ASTNodeVisit(n->stmt, func, 3, data);
  func(node, data, child_id, kVisitPostChildren);
}

static bool ForStatementASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  ForStatementASTNode* n = (ForStatementASTNode*)node;
  return n->c2 == value;
}

static ASTNodeVirtuals for_stmt_vtbl = {
    ForStatementASTNodeDelete, ForStatementASTNodePrint,
    ForStatementASTNodeReplaceChild, ForStatementASTNodeClone,
    ForStatementASTNodeVisit, ForStatementASTNodeUsesValue};

ASTNode* NewForStatementASTNode(ASTNode* e1, ASTNode* e2, ASTNode* e3,
                                ASTNode* stmt, SourceLocation location) {
  ForStatementASTNode* node = ASTArenaAlloc(sizeof(ForStatementASTNode));
  ASTNodeInit(&node->base, AST_OP(for), NULL, location, &for_stmt_vtbl);
  node->c1 = e1;
  if (e1 != NULL) {
    e1->parent = (ASTNode*)node;
    e1->child_id = 0;
  }
  node->c2 = e2;
  if (e2 != NULL) {
    e2->parent = (ASTNode*)node;
    e2->child_id = 1;
  }
  node->c3 = e3;
  if (e3 != NULL) {
    e3->parent = (ASTNode*)node;
    e3->child_id = 2;
  }
  node->stmt = stmt;
  if (stmt != NULL) {
    stmt->parent = (ASTNode*)node;
    stmt->child_id = 3;
  }
  return (ASTNode*)node;
}

static void VariableDeclarationASTNodeDelete(ASTNode* node) {
  VariableDeclarationASTNode* vnode = (VariableDeclarationASTNode*)node;
  if (vnode->initializer != NULL) {
    ASTNodeDelete(vnode->initializer);
  }
  ASTNodeBaseDelete(node);
}

static void VariableDeclarationASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  VariableDeclarationASTNode* vnode = (VariableDeclarationASTNode*)node;
  Indent(indents, fp);
  SymbolPrint(vnode->symbol, fp);
  if (vnode->initializer != NULL) {
    fprintf(fp,"\n");
    ASTNodePrint(vnode->initializer, indents + 2, fp);
  }
}

static void VariableDeclarationASTNodeReplaceChild(ASTNode* parent,
                                                   int child_id, ASTNode* child,
                                                   bool delete_old_child) {
  VariableDeclarationASTNode* node = (VariableDeclarationASTNode*)parent;
  ASTNode* old = node->initializer;
  node->initializer = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* VariableDeclarationASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  VariableDeclarationASTNode* from = (VariableDeclarationASTNode*)node;
  VariableDeclarationASTNode* to = ASTArenaAlloc(sizeof(VariableDeclarationASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->symbol = from->symbol;
  to->initializer = ASTNodeClone(from->initializer, func, data, &to->base);
  return func(&to->base, data);
}

static void VariableDeclarationASTNodeVisit(ASTNode* node,
                                            void (*func)(ASTNode* node, void*,
                                                         int, VisitorMode),
                                            int child_id,
                                            void* data) {
  VariableDeclarationASTNode* n = (VariableDeclarationASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->initializer, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals var_decl_vtbl = {
    VariableDeclarationASTNodeDelete, VariableDeclarationASTNodePrint,
    VariableDeclarationASTNodeReplaceChild, VariableDeclarationASTNodeClone,
    VariableDeclarationASTNodeVisit, ValueAlwaysUsed};

ASTNode* NewVariableDeclarationASTNode(Symbol* symbol, ASTNode* initializer,
                                       SourceLocation location) {
  VariableDeclarationASTNode* node = ASTArenaAlloc(sizeof(VariableDeclarationASTNode));
  ASTNodeInit(&node->base, AST_OP(vardecl), symbol->type, location,
              &var_decl_vtbl);
  node->symbol = symbol;
  node->initializer = initializer;
  if (initializer != NULL) {
    initializer->parent = (ASTNode*)node;
  }
  node->saved_sp = NULL;
  return (ASTNode*)node;
}

static void DeclarationListASTNodeDelete(ASTNode* node) {
  DeclarationListASTNode* vnode = (DeclarationListASTNode*)node;
  for (size_t i = 0; i < vnode->declarations->length; i++) {
    ASTNode* stmt = (ASTNode*)vnode->declarations->value.p[i];
    if (stmt != NULL) {
      ASTNodeDelete(stmt);
    }
  }
  // declarations is heap-allocated (NewVector), so free the struct too.
  VectorDelete(vnode->declarations);
  ASTNodeBaseDelete(node);
}

static void DeclarationListASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  DeclarationListASTNode* vnode = (DeclarationListASTNode*)node;
  for (size_t i = 0; i < vnode->declarations->length; i++) {
    ASTNode* stmt = (ASTNode*)vnode->declarations->value.p[i];
    if (stmt != NULL) {
      ASTNodePrint(stmt, indents, fp);
    }
  }
}

static ASTNode* DeclarationListASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  DeclarationListASTNode* from = (DeclarationListASTNode*)node;
  DeclarationListASTNode* to = ASTArenaAlloc(sizeof(DeclarationListASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->declarations = NewVector();
  for (size_t i = 0; i < from->declarations->length; i++) {
    ASTNode* child =
        ASTNodeClone(from->declarations->value.p[i], func, data, &to->base);
    VectorAppend(to->declarations, child);
  }
  return func(&to->base, data);
}

static void DeclarationListASTNodeVisit(ASTNode* node,
                                        void (*func)(ASTNode* node, void*,
                                                     int, VisitorMode),
                                        int child_id,
                                        void* data) {
  DeclarationListASTNode* n = (DeclarationListASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  for (size_t i = 0; i < n->declarations->length; i++) {
    ASTNodeVisit(n->declarations->value.p[i], func, (int)i, data);
  }
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals decl_list_vtbl = {
    DeclarationListASTNodeDelete, DeclarationListASTNodePrint, NULL,
    DeclarationListASTNodeClone, DeclarationListASTNodeVisit, ValueNotUsed};

// Declaration list.
ASTNode* NewDeclarationListASTNode(Vector* declarations,
                                   SourceLocation location) {
  DeclarationListASTNode* node = ASTArenaAlloc(sizeof(DeclarationListASTNode));
  ASTNodeInit(&node->base, AST_OP(decl_list), NULL, location, &decl_list_vtbl);
  node->declarations = declarations;
  return (ASTNode*)node;
}

static void CaseLabelASTNodeDelete(ASTNode* node) {
  CaseLabelASTNode* cnode = (CaseLabelASTNode*)node;
  if (cnode->expr != NULL) {
    ASTNodeDelete(cnode->expr);
  }
  if (cnode->stmt != NULL) {
    ASTNodeDelete(cnode->stmt);
  }
  ASTNodeBaseDelete(node);
}

static void CaseLabelASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  CaseLabelASTNode* cnode = (CaseLabelASTNode*)node;
  if (cnode->expr != NULL) {
    ASTNodePrint(cnode->expr, indents + 2, fp);
    ASTNodeBasePrint(&cnode->base, indents + 2, fp);
  } else {
    Indent(indents, fp);
    fprintf(fp,"default\n");
  }
  ASTNodePrint(cnode->stmt, indents + 2, fp);
}

static ASTNode* CaseLabelASTNodeClone(const ASTNode* node,
                                      ASTNode* (*func)(ASTNode* node, void*),
                                      void* data) {
  CaseLabelASTNode* from = (CaseLabelASTNode*)node;
  CaseLabelASTNode* to = ASTArenaAlloc(sizeof(CaseLabelASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->value = from->value;
  to->expr = ASTNodeClone(from->expr, func, data, &to->base);
  to->stmt = ASTNodeClone(from->stmt, func, data, &to->base);
  to->label = from->label;
  return func(&to->base, data);
}

static void CaseLabelASTNodeVisit(ASTNode* node,
                                  void (*func)(ASTNode* node, void*,
                                               int, VisitorMode),
                                  int child_id,
                                  void* data) {
  CaseLabelASTNode* n = (CaseLabelASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->expr, func, 0, data);
  ASTNodeVisit(n->stmt, func, 1, data);
  func(node, data, child_id, kVisitPostChildren);
}

static void CaseLabelASTNodeReplaceChild(ASTNode* parent, int child_id,
                                             ASTNode* child,
                                             bool delete_old_child) {
  CaseLabelASTNode* node = (CaseLabelASTNode*)parent;
  ASTNode* old;
  switch (child_id) {
    case 0:
      old = node->expr;
      node->expr = child;
      break;
    case 1:
      old = node->stmt;
      node->stmt = child;
      break;
    default:
      assert(false);
      break;
  }
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals case_label_vtbl = {
    CaseLabelASTNodeDelete, CaseLabelASTNodePrint, CaseLabelASTNodeReplaceChild, CaseLabelASTNodeClone,
    CaseLabelASTNodeVisit, ValueAlwaysUsed};

ASTNode* NewCaseLabelASTNode(ASTNode* expr, ASTNode* stmt, SourceLocation location) {
  CaseLabelASTNode* node = ASTArenaAlloc(sizeof(CaseLabelASTNode));
  ASTNodeInit(&node->base, AST_OP(case), NULL, location, &case_label_vtbl);
  node->expr = expr;
  if (expr != NULL) {
    expr->parent = &node->base;
    expr->child_id = 0;
  }
  node->stmt = stmt;
  if (node->stmt != NULL) {
    stmt->parent = &node->base;
    stmt->child_id = 1;
  }
  node->value = 0;
  node->label = NULL;
  return (ASTNode*)node;
}

static void SwitchStatementASTNodeDelete(ASTNode* node) {
  SwitchStatementASTNode* snode = (SwitchStatementASTNode*)node;
  if (snode->expr != NULL) {
    ASTNodeDelete(snode->expr);
  }
  if (snode->stmt != NULL) {
    ASTNodeDelete(snode->stmt);
  }
  // The cases vector holds non-owning pointers to case nodes (owned by the
  // statement subtree); free only the vector's backing array.
  VectorDestruct(&snode->cases);
  ASTNodeBaseDelete(node);
}

static void SwitchStatementASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  SwitchStatementASTNode* snode = (SwitchStatementASTNode*)node;
  if (snode->expr != NULL) {
    ASTNodePrint(snode->expr, indents + 2, fp);
  }
  ASTNodeBasePrint(&snode->base, indents, fp);
  if (snode->stmt != NULL) {
    ASTNodePrint(snode->stmt, indents + 2, fp);
  }
}

static void SwitchStatementASTNodeReplaceChild(ASTNode* parent, int child_id,
                                               ASTNode* child,
                                               bool delete_old_child) {
  SwitchStatementASTNode* node = (SwitchStatementASTNode*)parent;
  ASTNode* old = NULL;
  switch (child_id) {
    case 0:  // expr.
      old = node->expr;
      node->expr = child;
      break;
    case 1:  // stmt.
      old = node->stmt;
      node->stmt = child;
      break;
  }
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* SwitchStatementASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  SwitchStatementASTNode* from = (SwitchStatementASTNode*)node;
  SwitchStatementASTNode* to = ASTArenaAlloc(sizeof(SwitchStatementASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->expr = ASTNodeClone(from->expr, func, data, &to->base);
  to->stmt = ASTNodeClone(from->stmt, func, data, &to->base);

  VectorInit(&to->cases);
  to->default_node = NULL;
  to->density = 0;
  to->min_case_value = LLONG_MAX;
  to->max_case_value = LLONG_MIN;
  to->all_cases_covered = false;
  to->max_case_width = from->max_case_width;
  to->all_cases_positive = from->all_cases_positive;
  
  // We need to perform semantic analysis again
  to->base.flags &= ~kASTAnalyzed;
  return func(&to->base, data);
}

static void SwitchStatementASTNodeVisit(ASTNode* node,
                                        void (*func)(ASTNode* node, void*,
                                                    int,  VisitorMode),
                                        int child_id,
                                        void* data) {
  SwitchStatementASTNode* n = (SwitchStatementASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->expr, func, 0, data);
  ASTNodeVisit(n->stmt, func, 1, data);
  func(node, data, child_id, kVisitPostChildren);
}

static bool SwitchStatementASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  SwitchStatementASTNode* n = (SwitchStatementASTNode*)node;
  return n->expr == value;
}

static ASTNodeVirtuals switch_stmt_vtbl = {
    SwitchStatementASTNodeDelete, SwitchStatementASTNodePrint,
    SwitchStatementASTNodeReplaceChild, SwitchStatementASTNodeClone,
    SwitchStatementASTNodeVisit, SwitchStatementASTNodeUsesValue};

ASTNode* NewSwitchStatementASTNode(ASTNode* expr, ASTNode* stmt,
                                   SourceLocation location) {
  SwitchStatementASTNode* node = ASTArenaAlloc(sizeof(SwitchStatementASTNode));
  ASTNodeInit(&node->base, AST_OP(switch), NULL, location, &switch_stmt_vtbl);
  node->expr = expr;
  expr->parent = (ASTNode*)node;
  expr->child_id = 0;

  node->stmt = stmt;
  stmt->parent = (ASTNode*)node;
  stmt->child_id = 1;

  VectorInit(&node->cases);
  node->default_node = NULL;
  node->density = 0;
  node->min_case_value = LLONG_MAX;
  node->max_case_value = LLONG_MIN;
  node->all_cases_covered = false;
  node->max_case_width = 0;
  node->all_cases_positive = false;
  return (ASTNode*)node;
}

static void LabelASTNodeDelete(ASTNode* node) {
  LabelASTNode* lnode = (LabelASTNode*)node;
  if (lnode->stmt != NULL) {
    ASTNodeDelete(lnode->stmt);
  }
  StringDestruct(&lnode->name);
  ASTNodeBaseDelete(node);
}

static void LabelASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  LabelASTNode* lnode = (LabelASTNode*)node;
  ASTNodeBasePrint(&lnode->base, indents + 2, fp);
  fprintf(fp," %s\n", lnode->name.value);
  ASTNodePrint(lnode->stmt, indents + 2, fp);
}

static ASTNode* LabelASTNodeClone(const ASTNode* node,
                                  ASTNode* (*func)(ASTNode* node, void*),
                                  void* data) {
  LabelASTNode* from = (LabelASTNode*)node;
  LabelASTNode* to = ASTArenaAlloc(sizeof(LabelASTNode));
  ASTNodeBaseCopy(&to->base, node);
  StringInit(&to->name, from->name.value);
  to->stmt = ASTNodeClone(from->stmt, func, data, &to->base);
  to->label = from->label;
  return func(&to->base, data);
}

static void LabelASTNodeReplaceChild(ASTNode* parent, int child_id,
                                               ASTNode* child,
                                               bool delete_old_child) {
  LabelASTNode* node = (LabelASTNode*)parent;
  ASTNode* old = node->stmt;
  assert(child_id == 0);
  node->stmt = child;
  SetParent(child, parent, child_id);

  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static void LabelASTNodeVisit(ASTNode* node,
                                  void (*func)(ASTNode* node, void*,
                                               int, VisitorMode),
                                  int child_id,
                                  void* data) {
  LabelASTNode* n = (LabelASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->stmt, func, 1, data);
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals label_vtbl = {LabelASTNodeDelete, LabelASTNodePrint,
                                     LabelASTNodeReplaceChild, LabelASTNodeClone, LabelASTNodeVisit, NULL};

ASTNode* NewLabelASTNode(const char* name, ASTNode* stmt, bool named, SourceLocation location) {
  LabelASTNode* node = ASTArenaAlloc(sizeof(LabelASTNode));
  ASTNodeInit(&node->base, AST_OP(label), NULL, location, &label_vtbl);
  StringInit(&node->name, name);
  node->stmt = stmt;
  if (stmt != NULL) {
    stmt->parent = &node->base;
  }
  node->label = NULL;
  node->named = named;
  return (ASTNode*)node;
}

static void AsmASTNodeDelete(ASTNode* node) {
  AsmASTNode* anode = (AsmASTNode*)node;
  StringDelete(anode->text);
  VectorDestructWithContents(&anode->outputs, (VectorElementDestructor)AsmOperandDelete,
                             false);
  VectorDestructWithContents(&anode->inputs, (VectorElementDestructor)AsmOperandDelete,
                             false);
  VectorDestructWithContents(&anode->clobbers, (VectorElementDestructor)StringDelete,
                             false);
  VectorDestructWithContents(&anode->labels, (VectorElementDestructor)StringDelete,
                             false);
  VectorDestruct(&anode->label_nodes);
  ASTNodeBaseDelete(node);
}

static void AsmASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  AsmASTNode* lnode = (AsmASTNode*)node;
  ASTNodeBasePrint(&lnode->base, indents + 2, fp);
  fprintf(fp," %s\n", lnode->text->value);
}

static ASTNode* AsmASTNodeClone(const ASTNode* node,
                                ASTNode* (*func)(ASTNode* node, void*),
                                void* data) {
  AsmASTNode* from = (AsmASTNode*)node;
  AsmASTNode* to = ASTArenaAlloc(sizeof(AsmASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->text = NewString(from->text->value);
  to->is_volatile = from->is_volatile;
  to->is_goto = from->is_goto;
  VectorInit(&to->outputs);
  VectorInit(&to->inputs);
  VectorInit(&to->clobbers);
  VectorInit(&to->labels);
  VectorInit(&to->label_nodes);
  for (size_t i = 0; i < from->outputs.length; i++) {
    VectorAppend(&to->outputs,
                 AsmOperandClone(from->outputs.value.p[i], func, data, &to->base));
  }
  for (size_t i = 0; i < from->inputs.length; i++) {
    VectorAppend(&to->inputs,
                 AsmOperandClone(from->inputs.value.p[i], func, data, &to->base));
  }
  for (size_t i = 0; i < from->clobbers.length; i++) {
    String* from_clobber = from->clobbers.value.p[i];
    VectorAppend(&to->clobbers, NewString(from_clobber->value));
  }
  for (size_t i = 0; i < from->labels.length; i++) {
    String* from_label = from->labels.value.p[i];
    VectorAppend(&to->labels, NewString(from_label->value));
    VectorAppend(&to->label_nodes,
                 i < from->label_nodes.length ? from->label_nodes.value.p[i] : NULL);
  }
  return func(&to->base, data);
}

static ASTNodeVirtuals asm_vtbl = {AsmASTNodeDelete, AsmASTNodePrint, NULL,
                                   AsmASTNodeClone, NULL, NULL};

ASTNode* NewAsmASTNode(String* text, bool is_volatile,
                       SourceLocation location) {
  AsmASTNode* node = ASTArenaAlloc(sizeof(AsmASTNode));
  TypeRecord* type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  ASTNodeInit(&node->base, AST_OP(asm), type, location, &asm_vtbl);
  node->text = text;
  node->is_volatile = is_volatile;
  node->is_goto = false;
  VectorInit(&node->outputs);
  VectorInit(&node->inputs);
  VectorInit(&node->clobbers);
  VectorInit(&node->labels);
  VectorInit(&node->label_nodes);
  return (ASTNode*)node;
}

AsmOperand* NewAsmOperand(const char* constraint, const char* name,
                          ASTNode* expr, bool is_output) {
  AsmOperand* operand = malloc(sizeof(AsmOperand));
  StringInit(&operand->constraint, constraint);
  StringInit(&operand->name, name == NULL ? "" : name);
  operand->expr = expr;
  operand->is_output = is_output;
  operand->is_readwrite = constraint != NULL && constraint[0] == '+';
  operand->is_early_clobber = constraint != NULL && strchr(constraint, '&') != NULL;
  return operand;
}

void AsmOperandDelete(AsmOperand* operand) {
  if (operand == NULL) {
    return;
  }
  StringDestruct(&operand->constraint);
  StringDestruct(&operand->name);
  ASTNodeDelete(operand->expr);
  free(operand);
}

AsmOperand* AsmOperandClone(AsmOperand* operand,
                            ASTNode* (*func)(ASTNode* node, void*),
                            void* data, ASTNode* new_parent) {
  ASTNode* expr = operand->expr == NULL ? NULL : ASTNodeClone(operand->expr, func, data,
                                                              new_parent);
  AsmOperand* clone = NewAsmOperand(operand->constraint.value,
                                    operand->name.length == 0 ? NULL : operand->name.value,
                                    expr, operand->is_output);
  clone->is_readwrite = operand->is_readwrite;
  clone->is_early_clobber = operand->is_early_clobber;
  return clone;
}

//
// Initializer AST nodes.
//

static void ExpressionInitializerASTNodeDelete(ASTNode* node) {
  ExpressionInitializerASTNode* enode = (ExpressionInitializerASTNode*)node;
  ASTNodeDelete(enode->expr);
  ASTNodeBaseDelete(node);
}

static void ExpressionInitializerASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  ExpressionInitializerASTNode* enode = (ExpressionInitializerASTNode*)node;
  Indent(indents, fp);
  fprintf(fp,"expr-init\n");
  ASTNodePrint(enode->expr, indents + 2, fp);
}

static void ExpressionInitializerASTNodeReplaceChild(ASTNode* parent,
                                                     int child_id,
                                                     ASTNode* child,
                                                     bool delete_old_child) {
  ExpressionInitializerASTNode* node = (ExpressionInitializerASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* ExpressionInitializerASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  ExpressionInitializerASTNode* from = (ExpressionInitializerASTNode*)node;
  ExpressionInitializerASTNode* to =
      ASTArenaAlloc(sizeof(ExpressionInitializerASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->expr = ASTNodeClone(from->expr, func, data, &to->base);
  return func(&to->base, data);
}

static void ExpressionInitializerASTNodeVisit(ASTNode* node,
                                              void (*func)(ASTNode* node, void*,
                                                           int, VisitorMode),
                                              int child_id,
                                              void* data) {
  ExpressionInitializerASTNode* n = (ExpressionInitializerASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->expr, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals expr_init_vtbl = {
    ExpressionInitializerASTNodeDelete, ExpressionInitializerASTNodePrint,
    ExpressionInitializerASTNodeReplaceChild, ExpressionInitializerASTNodeClone,
    ExpressionInitializerASTNodeVisit, ValueAlwaysUsed};

ASTNode* NewExpressionInitializerASTNode(ASTNode* expr,
                                         SourceLocation location) {
  ExpressionInitializerASTNode* node =
      ASTArenaAlloc(sizeof(ExpressionInitializerASTNode));
  ASTNodeInit(&node->base, AST_OP(expr_init), NULL, location, &expr_init_vtbl);
  node->expr = expr;
  expr->parent = (ASTNode*)node;
  return (ASTNode*)node;
}

static void BracedInitializerASTNodeDelete(ASTNode* node) {
  BracedInitializerASTNode* lnode = (BracedInitializerASTNode*)node;
  for (size_t i = 0; i < lnode->initializers->length; i++) {
    ASTNode* init = (ASTNode*)lnode->initializers->value.p[i];
    if (init != NULL) {
      ASTNodeDelete(init);
    }
  }
  VectorDelete(lnode->initializers);
  ASTNodeBaseDelete(node);
}

static void BracedInitializerASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  BracedInitializerASTNode* lnode = (BracedInitializerASTNode*)node;
  for (size_t i = 0; i < lnode->initializers->length; i++) {
    ASTNode* init = (ASTNode*)lnode->initializers->value.p[i];
    if (init != NULL) {
      ASTNodePrint(init, indents + 2, fp);
    }
  }
  Indent(indents, fp);
  fprintf(fp,"braced-init\n");
}

static void BracedInitializerASTNodeReplaceChild(ASTNode* parent, int child_id,
                                                 ASTNode* child,
                                                 bool delete_old_child) {
  BracedInitializerASTNode* lnode = (BracedInitializerASTNode*)parent;
  ASTNode* old = lnode->initializers->value.p[child_id];
  lnode->initializers->value.p[child_id] = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* BracedInitializerASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  BracedInitializerASTNode* from = (BracedInitializerASTNode*)node;
  BracedInitializerASTNode* to = ASTArenaAlloc(sizeof(BracedInitializerASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->initializers = NewVector();
  for (size_t i = 0; i < from->initializers->length; i++) {
    ASTNode* init =
        ASTNodeClone(from->initializers->value.p[i], func, data, &to->base);
    VectorAppend(to->initializers, init);
  }
  return func(&to->base, data);
}

static void BracedInitializerASTNodeVisit(ASTNode* node,
                                          void (*func)(ASTNode* node, void*,
                                                       int, VisitorMode),
                                          int child_id,
                                          void* data) {
  BracedInitializerASTNode* n = (BracedInitializerASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  for (size_t i = 0; i < n->initializers->length; i++) {
    ASTNodeVisit(n->initializers->value.p[i], func, (int)i, data);
  }
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals braced_init_vtbl = {
    BracedInitializerASTNodeDelete, BracedInitializerASTNodePrint,
    BracedInitializerASTNodeReplaceChild, BracedInitializerASTNodeClone,
    BracedInitializerASTNodeVisit, NULL};

ASTNode* NewBracedInitializerASTNode(Vector* initializers,
                                     TypeRecord* type,
                                     SourceLocation location) {
  BracedInitializerASTNode* node = ASTArenaAlloc(sizeof(BracedInitializerASTNode));
  ASTNodeInit(&node->base, AST_OP(braced_init), type, location,
              &braced_init_vtbl);
  node->initializers = initializers;
  for (size_t i = 0; i < node->initializers->length; i++) {
    ASTNode* init = (ASTNode*)node->initializers->value.p[i];
    init->parent = (ASTNode*)node;
    init->child_id = (int)i;
  }
  return (ASTNode*)node;
}

Designator* NewArrayDesignator(TypeRecord* type, int index) {
  Designator* d = malloc(sizeof(Designator));
  d->designator_type = kDesignatorArray;
  d->value.array_index = index;
  d->array_index_end = index;
  d->type = type;
  TypeRecordIncRef(type);
  return d;
}

Designator* NewStructDesignator(String* member) {
  Designator* d = malloc(sizeof(Designator));
  d->designator_type = kDesignatorStruct;
  d->value.struct_member_name = member;
  d->type = NULL;
  return d;
}

Designator* NewStructMemberDesignator(StructMember* member) {
  Designator* d = malloc(sizeof(Designator));
  d->designator_type = kDesignatorStruct;
  d->value.struct_member = member;
  d->type = NULL;
  return d;
}

// Frees a Designator's owned resources.  Only array designators take a
// reference on their type (NewArrayDesignator/clone); struct designators leave
// it NULL so the decref is a no-op.
static void DesignatorDestruct(void* p) {
  Designator* d = (Designator*)p;
  TypeRecordDelete(d->type);
}

static void DesignatedInitializerASTNodeDelete(ASTNode* node) {
  DesignatedInitializerASTNode* dnode = (DesignatedInitializerASTNode*)node;
  VectorDeleteWithContents(dnode->designators, DesignatorDestruct,
                           /*free_element=*/true);
  ASTNodeDelete(dnode->init);
  ASTNodeBaseDelete(node);
}

static void DesignatedInitializerASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  DesignatedInitializerASTNode* dnode = (DesignatedInitializerASTNode*)node;
  Indent(indents + 2, fp);
  if (dnode->designators != NULL) {
    for (size_t i = 0; i < dnode->designators->length; i++) {
      Designator* d = (Designator*)dnode->designators->value.p[i];
      if (d->designator_type == kDesignatorArray) {
        fprintf(fp,"[%d]", d->value.array_index);
      } else {
        if (d->value.struct_member != NULL) {
          fprintf(fp,".%s", d->value.struct_member->symbol->name.value);
        } else if (d->value.struct_member != NULL) {
          fprintf(fp,".%s", d->value.struct_member_name->value);
        }
      }
    }
    fprintf(fp,"\n");
  }
  Indent(indents, fp);
  fprintf(fp,"designated-initializer\n");
  ASTNodePrint(dnode->init, indents + 2, fp);
  ASTNodeBasePrint(node, indents, fp);
}

static void DesignatedInitializerASTNodeReplaceChild(ASTNode* parent,
                                                     int child_id,
                                                     ASTNode* child,
                                                     bool delete_old_child) {
  DesignatedInitializerASTNode* node = (DesignatedInitializerASTNode*)parent;
  ASTNode* old = node->init;
  node->init = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* DesignatedInitializerASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  DesignatedInitializerASTNode* from = (DesignatedInitializerASTNode*)node;
  DesignatedInitializerASTNode* to =
      ASTArenaAlloc(sizeof(DesignatedInitializerASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->designators = NewVector();
  for (size_t i = 0; i < from->designators->length; i++) {
    Designator* from_d = (Designator*)from->designators->value.p[i];
    Designator* to_d = malloc(sizeof(Designator));
    to_d->designator_type = from_d->designator_type;
    to_d->type = from_d->type;
    TypeRecordIncRef(to_d->type);
    memcpy(&to_d->value, &from_d->value, sizeof(to_d->value));
    VectorAppend(to->designators, to_d);
  }
  to->init = ASTNodeClone(from->init, func, data, &to->base);
  return func(&to->base, data);
}

static void DesignatedInitializerASTNodeVisit(ASTNode* node,
                                              void (*func)(ASTNode* node, void*,
                                                           int, VisitorMode),
                                              int child_id,
                                              void* data) {
  DesignatedInitializerASTNode* n = (DesignatedInitializerASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->init, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static ASTNodeVirtuals designated_init_vtbl = {
    DesignatedInitializerASTNodeDelete, DesignatedInitializerASTNodePrint,
    DesignatedInitializerASTNodeReplaceChild, DesignatedInitializerASTNodeClone,
    DesignatedInitializerASTNodeVisit, ValueAlwaysUsed};

ASTNode* NewDesignatedInitializerASTNode(Vector* designators, ASTNode* init,
                                         SourceLocation location) {
  DesignatedInitializerASTNode* node =
      ASTArenaAlloc(sizeof(DesignatedInitializerASTNode));
  ASTNodeInit(&node->base, AST_OP(designated_init), NULL, location,
              &designated_init_vtbl);
  node->designators = designators;
  node->init = init;
  init->parent = (ASTNode*)node;
  init->child_id = 0;
  return (ASTNode*)node;
}

bool IsBitfieldReference(ASTNode* node) {
  if (node->op != AST_OP(dot) && node->op != AST_OP(arrow)) {
    return false;
  }
  BinaryASTNode* dot_or_arrow = (BinaryASTNode*)node;
  StructMemberASTNode* member_node = (StructMemberASTNode*)dot_or_arrow->right;
  return StructMemberIsBitField(member_node->member);
}


// Compound literal.
static void CompoundLiteralASTNodeDelete(ASTNode* node) {
  CompoundLiteralASTNode* cnode = (CompoundLiteralASTNode*)node;
  if (cnode->sym != NULL) {
    ASTNodeDelete(cnode->sym);
  }
  if (cnode->initializer != NULL) {
    ASTNodeDelete(cnode->initializer);
  }
  ASTNodeBaseDelete(node);
}

static void CompoundLiteralASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  CompoundLiteralASTNode* cnode = (CompoundLiteralASTNode*)node;
  ASTNodeBasePrint(node, indents, fp);
  if (cnode->sym != NULL) {
    ASTNodePrint(cnode->sym, indents + 2, fp);
  }
  if (cnode->initializer != NULL) {
    ASTNodePrint(cnode->initializer, indents + 2, fp);
  }
}

static void CompoundLiteralASTNodeReplaceChild(ASTNode* parent, int child_id,
                                    ASTNode* child, bool delete_old_child) {
  CompoundLiteralASTNode* node = (CompoundLiteralASTNode*)parent;
  ASTNode* old = node->initializer;
  if (child == 0) {
    node->sym = child;
  } else {
    node->initializer = child;
  }
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* CompoundLiteralASTNodeClone(const ASTNode* node,
                                 ASTNode* (*func)(ASTNode* node, void*),
                                 void* data) {
  CompoundLiteralASTNode* from = (CompoundLiteralASTNode*)node;
  CompoundLiteralASTNode* to = ASTArenaAlloc(sizeof(CompoundLiteralASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->sym = ASTNodeClone(from->sym, func, data, &to->base);
  to->initializer = ASTNodeClone(from->initializer, func, data, &to->base);
  return func(&to->base, data);
}

static ASTNodeVirtuals compound_literal_vtbl = {CompoundLiteralASTNodeDelete, CompoundLiteralASTNodePrint,
                                    CompoundLiteralASTNodeReplaceChild, CompoundLiteralASTNodeClone, NULL,
  NULL
};

ASTNode* NewCompoundLiteralASTNode(ASTNode* sym, SourceLocation location,
                        ASTNode* initializer) {
  CompoundLiteralASTNode* node = ASTArenaAlloc(sizeof(CompoundLiteralASTNode));
  ASTNodeInit(&node->base, AST_OP(compound_literal), sym->type, location, &compound_literal_vtbl);
  node->sym = sym;
  sym->parent = (ASTNode*)node;
  node->initializer = initializer;
  initializer->parent = (ASTNode*)node;
  return (ASTNode*)node;
}

