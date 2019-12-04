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
#include "errors.h"
#include "symbol.h"

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
  node->op = op;
  node->flags = 0;
  node->type = NULL;
  node->parent = NULL;
  node->child_id = 0;
  node->location = location;
  node->virtuals = virtuals;
  ASTNodeSetType(node, type);
}

static void ASTNodeBaseDelete(ASTNode* node) {
  if (node->type != NULL) {
    TypeRecordDelete(node->type);
  }
  free(node);
}

static void Indent(int indents) {
  for (int i = 0; i < indents; i++) {
    putchar(' ');
  }
}

static void ASTNodeBasePrint(ASTNode* node, int indents) {
  Indent(indents);
  printf("%s ", ASTOpcodeName(node->op));
  if (node->type != NULL) {
    TypeRecordPrint(node->type);
  }
  printf("\n");
}

static ASTNodeVirtuals base_vtbl = {ASTNodeBaseDelete, ASTNodeBasePrint, NULL};

ASTNode* NewASTNode(ASTOpcode op, TypeRecord* type, SourceLocation location) {
  ASTNode* node = malloc(sizeof(ASTNode));
  ASTNodeInit(node, op, type, location, &base_vtbl);
  return node;
}

void ASTNodeDelete(ASTNode* node) {
  if (node == NULL) {
    return;
  }
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

void ASTNodePrint(ASTNode* node, int indents) {
  if (node == NULL) {
    return;
  }
  assert(node->virtuals->printer != NULL);
  node->virtuals->printer(node, indents);
}

void ASTNodeReplaceChild(ASTNode* parent, int child_id, ASTNode* child,
                         bool delete_old_child) {
  if (parent == NULL) {
    return;
  }
  assert(parent->virtuals->replacer != NULL);
  parent->virtuals->replacer(parent, child_id, child, delete_old_child);
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
  }
}

static void IdentifierASTNodePrint(ASTNode* node, int indents) {
  Indent(indents);
  IdentifierASTNode* inode = (IdentifierASTNode*)node;
  printf("%s\n", inode->symbol->name.value);
  ASTNodeBasePrint(node, indents + 2);
}

static ASTNodeVirtuals identifier_vtbl = {ASTNodeBaseDelete,
                                          IdentifierASTNodePrint, NULL};

ASTNode* NewIdentifierASTNode(Symbol* symbol, SourceLocation location) {
  IdentifierASTNode* node = malloc(sizeof(IdentifierASTNode));
  ASTNodeInit(&node->base, AST_OP(identifier), symbol->type, location,
              &identifier_vtbl);
  node->symbol = symbol;
  return (ASTNode*)node;
}

static void StructMemberASTNodePrint(ASTNode* node, int indents) {
  Indent(indents);
  StructMemberASTNode* mnode = (StructMemberASTNode*)node;
  printf("%s@%d\n", mnode->member->symbol->name.value,
         mnode->member->byte_offset);
  ASTNodeBasePrint(node, indents + 2);
}

static ASTNodeVirtuals struct_member_vtbl = {ASTNodeBaseDelete,
                                             StructMemberASTNodePrint, NULL};

ASTNode* NewStructMemberASTNode(StructMember* member, SourceLocation location) {
  StructMemberASTNode* node = malloc(sizeof(StructMemberASTNode));
  ASTNodeInit(&node->base, AST_OP(identifier), member->symbol->type, location,
              &struct_member_vtbl);
  node->member = member;
  return (ASTNode*)node;
}

static void ConstantASTNodePrint(ASTNode* node, int indents) {
  Indent(indents);
  ConstantASTNode* cnode = (ConstantASTNode*)node;
  switch (cnode->base.op) {
    case AST_OP(number):
      printf("%" PRId64 "\n", cnode->value.ivalue);
      break;
    case AST_OP(fnumber):
      printf("%g\n", cnode->value.fvalue);
      break;
    case AST_OP(string): {
      String escaped;
      StringInit(&escaped, NULL);
      StringEscape(cnode->value.string, &escaped);
      printf("\"%s\"\n", escaped.value);
      break;
    }
    case AST_OP(charconst):
      printf("'\\x%04x'\n", (int)cnode->value.ivalue);
      break;
    case AST_OP(label):
      printf("label %s\n", cnode->value.string->value);
      break;
    case AST_OP(sizeof): {
      printf("sizeof\n");
      SizeofASTNode* s = (SizeofASTNode*)node;
      ASTNodePrint(s->expr, indents + 2);
      break;
    }
    default:
      printf("unknown constant op %d\n", cnode->base.op);
  }
}

static ASTNodeVirtuals constant_vtbl = {ASTNodeBaseDelete, ConstantASTNodePrint,
                                        NULL};

void IntConstantASTNodeInit(ConstantASTNode* node, int64_t value,
                            TypeRecord* type, SourceLocation location) {
  ASTNodeInit(&node->base, AST_OP(number), type, location, &constant_vtbl);
  node->value.ivalue = value;
}

ASTNode* NewIntConstantASTNode(int64_t value, TypeRecord* type,
                               SourceLocation location) {
  ConstantASTNode* node = malloc(sizeof(ConstantASTNode));
  IntConstantASTNodeInit(node, value, type, location);
  return (ASTNode*)node;
}

ASTNode* NewRealConstantASTNode(double value, TypeRecord* type,
                                SourceLocation location) {
  ConstantASTNode* node = malloc(sizeof(ConstantASTNode));
  ASTNodeInit(&node->base, AST_OP(fnumber), type, location, &constant_vtbl);
  node->value.fvalue = value;
  return (ASTNode*)node;
}

ASTNode* NewStringConstantASTNode(String* value, TypeRecord* type,
                                  SourceLocation location) {
  ConstantASTNode* node = malloc(sizeof(ConstantASTNode));
  ASTNodeInit(&node->base, AST_OP(string), type, location, &constant_vtbl);
  node->value.string = value;
  return (ASTNode*)node;
}

ASTNode* NewCharConstantASTNode(int value, TypeRecord* type,
                                SourceLocation location) {
  ConstantASTNode* node = malloc(sizeof(ConstantASTNode));
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

static void UnaryASTNodePrint(ASTNode* node, int indents) {
  ASTNodeBasePrint(node, indents);
  UnaryASTNode* unode = (UnaryASTNode*)node;
  if (unode->sub != NULL) {
    ASTNodePrint(unode->sub, indents + 2);
  }
}

static void UnaryASTNodeReplaceChild(ASTNode* parent, int child_id,
                                     ASTNode* child, bool delete_old_child) {
  UnaryASTNode* node = (UnaryASTNode*)parent;
  ASTNode* old = node->sub;
  node->sub = child;
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals unary_vtbl = {UnaryASTNodeDelete, UnaryASTNodePrint,
                                     UnaryASTNodeReplaceChild};

// Unary AST node with a single child.
ASTNode* NewUnaryASTNode(ASTOpcode op, TypeRecord* type,
                         SourceLocation location, ASTNode* sub) {
  UnaryASTNode* node = malloc(sizeof(UnaryASTNode));
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

static void BinaryASTNodePrint(ASTNode* node, int indents) {
  BinaryASTNode* bnode = (BinaryASTNode*)node;
  if (bnode->right != NULL) {
    ASTNodePrint(bnode->right, indents + 2);
  }
  ASTNodeBasePrint(node, indents);
  if (bnode->left != NULL) {
    ASTNodePrint(bnode->left, indents + 1);
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
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals binary_vtbl = {BinaryASTNodeDelete, BinaryASTNodePrint,
                                      BinaryASTNodeReplaceChild};

// Binary AST node, which a left and right child.
ASTNode* NewBinaryASTNode(ASTOpcode op, TypeRecord* type,
                          SourceLocation location, ASTNode* left,
                          ASTNode* right) {
  BinaryASTNode* node = malloc(sizeof(BinaryASTNode));
  ASTNodeInit(&node->base, op, type, location, &binary_vtbl);
  node->left = left;
  left->parent = (ASTNode*)node;
  left->child_id = 0;
  node->right = right;
  right->parent = (ASTNode*)node;
  right->child_id = 1;
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
  VectorDestruct(vnode->children);
  ASTNodeBaseDelete(node);
}

static void VectorASTNodePrint(ASTNode* node, int indents) {
  VectorASTNode* vnode = (VectorASTNode*)node;
  if (vnode->left != NULL) {
    ASTNodePrint(vnode->left, indents + 2);
  }
  ASTNodeBasePrint(node, indents);
  size_t num_children = vnode->children->length;
  for (size_t i = 0; i < num_children; i++) {
    Indent(indents + 2);
    printf("[%zd]:\n", i);
    ASTNodePrint((ASTNode*)vnode->children->value.p[i], indents + 4);
  }
}

static void VectorASTNodeReplaceChild(ASTNode* parent, int child_id,
                                      ASTNode* child, bool delete_old_child) {
  VectorASTNode* node = (VectorASTNode*)parent;
  ASTNode* old = node->children->value.p[child_id];
  node->children->value.p[child_id] = child;
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals vector_vtbl = {VectorASTNodeDelete, VectorASTNodePrint,
                                      VectorASTNodeReplaceChild};

ASTNode* NewVectorASTNode(ASTOpcode op, TypeRecord* type,
                          SourceLocation location, ASTNode* left,
                          Vector* children) {
  VectorASTNode* node = malloc(sizeof(VectorASTNode));
  ASTNodeInit(&node->base, op, type, location, &vector_vtbl);
  node->left = left;
  node->children = children;
  for (size_t i = 0; i < children->length; i++) {
    ASTNode* child = children->value.p[i];
    child->parent = (ASTNode*)node;
    child->child_id = (int)i;
  }
  return (ASTNode*)node;
}

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

static void CastASTNodePrint(ASTNode* node, int indents) {
  CastASTNode* cnode = (CastASTNode*)node;
  Indent(indents);
  printf("cast\n");
  Indent(indents);
  TypeRecordPrint(cnode->cast_type);
  if (cnode->expr != NULL) {
    ASTNodePrint(cnode->expr, indents + 2);
  }
}

static void CastASTNodeReplaceChild(ASTNode* parent, int child_id,
                                    ASTNode* child, bool delete_old_child) {
  CastASTNode* node = (CastASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals cast_vtbl = {CastASTNodeDelete, CastASTNodePrint,
                                    CastASTNodeReplaceChild};

ASTNode* NewCastASTNode(TypeRecord* type, SourceLocation location,
                        ASTNode* expr) {
  CastASTNode* node = malloc(sizeof(CastASTNode));
  ASTNodeInit(&node->base, AST_OP(cast), NULL, location, &cast_vtbl);
  node->cast_type = type;
  TypeRecordIncRef(type);
  node->expr = expr;
  expr->parent = (ASTNode*)node;
  return (ASTNode*)node;
}

ASTNode* NewSizeofASTNodeWithKnownSize(int size, SourceLocation location) {
  SizeofASTNode* node = malloc(sizeof(SizeofASTNode));
  TypeRecord* type = NewTypeRecord(kTypeInt | kTypeUnsigned, kQualConst);
  IntConstantASTNodeInit(&node->base, size, type, location);
  node->expr = NULL;
  node->base.base.op = AST_OP(sizeof);
  return (ASTNode*)node;
}

ASTNode* NewSizeofASTNodeWithExpression(ASTNode* expr,
                                        SourceLocation location) {
  SizeofASTNode* node = malloc(sizeof(SizeofASTNode));
  TypeRecord* type = NewTypeRecord(kTypeInt | kTypeUnsigned, kQualConst);
  IntConstantASTNodeInit(&node->base, 0, type, location);
  node->base.base.op = AST_OP(sizeof);
  node->expr = expr;
  expr->parent = (ASTNode*)node;
  return (ASTNode*)node;
}

void SizeofASTNodeDelete(ASTNode* node) {
  SizeofASTNode* snode = (SizeofASTNode*)node;
  if (snode->expr != NULL) {
    ASTNodeDelete(snode->expr);
  }
  ASTNodeBaseDelete(node);
}

void SizeofASTNodePrint(ASTNode* node, int indents) {
  SizeofASTNode* snode = (SizeofASTNode*)node;
  printf("sizeof ");
  if (snode->expr != NULL) {
    ASTNodePrint(snode->expr, indents + 2);
  } else {
    ConstantASTNodePrint((ASTNode*)&snode->base, indents + 2);
  }
}

void SizeofASTNodeReplaceChild(ASTNode* parent, int child_id, ASTNode* child,
                               bool delete_old_child) {
  SizeofASTNode* node = (SizeofASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static void MacroNameASTNodeDelete(ASTNode* node) {
  MacroNameASTNode* mnode = (MacroNameASTNode*)node;
  StringDestruct(&mnode->macro_name);
  ASTNodeBaseDelete(node);
}

static void MacroNameASTNodePrint(ASTNode* node, int indents) {
  MacroNameASTNode* mnode = (MacroNameASTNode*)node;
  printf("macro: %s", mnode->macro_name.value);
}

static ASTNodeVirtuals macro_vtbl = {MacroNameASTNodeDelete,
                                     MacroNameASTNodePrint, NULL};

ASTNode* NewMacroNameASTNode(String* macro_name, SourceLocation location) {
  MacroNameASTNode* node = malloc(sizeof(MacroNameASTNode));
  ASTNodeInit(&node->base, AST_OP(macro), NULL, location, &macro_vtbl);
  StringInit(&node->macro_name, macro_name->value);
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

static void ExpressionStatementASTNodePrint(ASTNode* node, int indents) {
  ExpressionStatementASTNode* enode = (ExpressionStatementASTNode*)node;
  if (enode->expr != NULL) {
    ASTNodePrint(enode->expr, indents + 2);
  }
}

static void ExpressionStatementASTNodeReplaceChild(ASTNode* parent,
                                                   int child_id, ASTNode* child,
                                                   bool delete_old_child) {
  ExpressionStatementASTNode* node = (ExpressionStatementASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals expr_stmt_vtbl = {
    ExpressionStatementASTNodeDelete, ExpressionStatementASTNodePrint,
    ExpressionStatementASTNodeReplaceChild};

ASTNode* NewExpressionStatementASTNode(ASTNode* expr, SourceLocation location) {
  ExpressionStatementASTNode* node = malloc(sizeof(ExpressionStatementASTNode));
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

static void IfStatementASTNodePrint(ASTNode* node, int indents) {
  IfStatementASTNode* enode = (IfStatementASTNode*)node;
  if (enode->cond != NULL) {
    ASTNodePrint(enode->cond, indents + 2);
  }

  ASTNodeBasePrint(&enode->base, indents);

  if (enode->if_part != NULL) {
    ASTNodePrint(enode->if_part, indents + 2);
  }
  Indent(indents);
  printf("else\n");
  if (enode->else_part != NULL) {
    ASTNodePrint(enode->else_part, indents + 2);
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
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals if_stmt_vtbl = {IfStatementASTNodeDelete,
                                       IfStatementASTNodePrint,
                                       IfStatementASTNodeReplaceChild};

ASTNode* NewIfStatementASTNode(ASTNode* cond, ASTNode* if_part,
                               ASTNode* else_part, SourceLocation location) {
  IfStatementASTNode* node = malloc(sizeof(IfStatementASTNode));
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

static void CombinedStatementASTNodePrint(ASTNode* node, int indents) {
  CombinedStatementASTNode* enode = (CombinedStatementASTNode*)node;
  if (enode->cond != NULL) {
    ASTNodePrint(enode->cond, indents + 2);
  }

  ASTNodeBasePrint(&enode->base, indents);

  if (enode->stmt != NULL) {
    ASTNodePrint(enode->stmt, indents + 2);
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
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals combined_stmt_vtbl = {
    CombinedStatementASTNodeDelete, CombinedStatementASTNodePrint,
    CombinedStatementASTNodeReplaceChild};

ASTNode* NewCombinedStatementASTNode(ASTOpcode tok, ASTNode* cond,
                                     ASTNode* stmt, SourceLocation location) {
  CombinedStatementASTNode* node = malloc(sizeof(CombinedStatementASTNode));
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
  VectorDestruct(vnode->statements);
  ASTNodeBaseDelete(node);
}

static void CompoundStatementASTNodePrint(ASTNode* node, int indents) {
  CompoundStatementASTNode* vnode = (CompoundStatementASTNode*)node;
  for (size_t i = 0; i < vnode->statements->length; i++) {
    ASTNode* stmt = (ASTNode*)vnode->statements->value.p[i];
    if (stmt != NULL) {
      ASTNodePrint(stmt, indents);
    }
  }
}

static void CompoundStatementASTNodeReplaceChild(ASTNode* parent, int child_id,
                                                 ASTNode* child,
                                                 bool delete_old_child) {
  CompoundStatementASTNode* node = (CompoundStatementASTNode*)parent;
  ASTNode* old = node->statements->value.p[child_id];
  node->statements->value.p[child_id] = child;
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals compound_stmt_vtbl = {
    CompoundStatementASTNodeDelete, CompoundStatementASTNodePrint,
    CompoundStatementASTNodeReplaceChild};

// Compound statement.
ASTNode* NewCompoundStatementASTNode(Vector* statements,
                                     SourceLocation location) {
  CompoundStatementASTNode* node = malloc(sizeof(CompoundStatementASTNode));
  ASTNodeInit(&node->base, AST_OP(compound), NULL, location,
              &compound_stmt_vtbl);
  node->statements = statements;
  for (size_t i = 0; i < node->statements->length; i++) {
    ASTNode* stmt = (ASTNode*)node->statements->value.p[i];
    stmt->parent = (ASTNode*)node;
    stmt->child_id = (int)i;
  }
  return (ASTNode*)node;
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

static void ForStatementASTNodePrint(ASTNode* node, int indents) {
  ForStatementASTNode* enode = (ForStatementASTNode*)node;
  Indent(indents);
  printf("for\n");
  Indent(indents + 2);
  if (enode->c1 != NULL) {
    ASTNodePrint(enode->c1, indents + 2);
  }
  Indent(indents);
  printf(";\n");
  if (enode->c2 != NULL) {
    ASTNodePrint(enode->c2, indents + 2);
  }
  Indent(indents);
  printf(";\n");
  if (enode->c3 != NULL) {
    ASTNodePrint(enode->c3, indents + 2);
  }
  if (enode->stmt != NULL) {
    ASTNodePrint(enode->stmt, indents + 4);
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
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals for_stmt_vtbl = {ForStatementASTNodeDelete,
                                        ForStatementASTNodePrint,
                                        ForStatementASTNodeReplaceChild};

ASTNode* NewForStatementASTNode(ASTNode* e1, ASTNode* e2, ASTNode* e3,
                                ASTNode* stmt, SourceLocation location) {
  ForStatementASTNode* node = malloc(sizeof(ForStatementASTNode));
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
  stmt->parent = (ASTNode*)node;
  stmt->child_id = 3;
  return (ASTNode*)node;
}

static void VariableDeclarationASTNodeDelete(ASTNode* node) {
  VariableDeclarationASTNode* vnode = (VariableDeclarationASTNode*)node;
  if (vnode->initializer != NULL) {
    ASTNodeDelete(vnode->initializer);
  }
  ASTNodeDelete(node);
}

static void VariableDeclarationASTNodePrint(ASTNode* node, int indents) {
  VariableDeclarationASTNode* vnode = (VariableDeclarationASTNode*)node;
  Indent(indents);
  SymbolPrint(vnode->symbol);
  if (vnode->initializer != NULL) {
    printf("\n");
    ASTNodePrint(vnode->initializer, indents + 2);
  }
}

static void VariableDeclarationASTNodeReplaceChild(ASTNode* parent,
                                                   int child_id, ASTNode* child,
                                                   bool delete_old_child) {
  VariableDeclarationASTNode* node = (VariableDeclarationASTNode*)parent;
  ASTNode* old = node->initializer;
  node->initializer = child;
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals var_decl_vtbl = {VariableDeclarationASTNodeDelete,
                                        VariableDeclarationASTNodePrint,
                                        VariableDeclarationASTNodeReplaceChild};

ASTNode* NewVariableDeclarationASTNode(Symbol* symbol, ASTNode* initializer,
                                       SourceLocation location) {
  VariableDeclarationASTNode* node = malloc(sizeof(VariableDeclarationASTNode));
  ASTNodeInit(&node->base, AST_OP(vardecl), symbol->type, location,
              &var_decl_vtbl);
  node->symbol = symbol;
  node->initializer = initializer;
  if (initializer != NULL) {
    initializer->parent = (ASTNode*)node;
  }
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
  VectorDestruct(vnode->declarations);
  ASTNodeBaseDelete(node);
}

static void DeclarationListASTNodePrint(ASTNode* node, int indents) {
  DeclarationListASTNode* vnode = (DeclarationListASTNode*)node;
  for (size_t i = 0; i < vnode->declarations->length; i++) {
    ASTNode* stmt = (ASTNode*)vnode->declarations->value.p[i];
    if (stmt != NULL) {
      ASTNodePrint(stmt, indents);
    }
  }
}

static ASTNodeVirtuals decl_list_vtbl = {DeclarationListASTNodeDelete,
                                         DeclarationListASTNodePrint, NULL};

// Declaration list.
ASTNode* NewDeclarationListASTNode(Vector* declarations,
                                   SourceLocation location) {
  DeclarationListASTNode* node = malloc(sizeof(DeclarationListASTNode));
  ASTNodeInit(&node->base, AST_OP(decl_list), NULL, location, &decl_list_vtbl);
  node->declarations = declarations;
  return (ASTNode*)node;
}

static void CaseLabelASTNodeDelete(ASTNode* node) {
  CaseLabelASTNode* cnode = (CaseLabelASTNode*)node;
  if (cnode->expr != NULL) {
    ASTNodeDelete(cnode->expr);
  }
  ASTNodeBaseDelete(node);
}

static void CaseLabelASTNodePrint(ASTNode* node, int indents) {
  CaseLabelASTNode* cnode = (CaseLabelASTNode*)node;
  if (cnode->expr != NULL) {
    ASTNodePrint(cnode->expr, indents + 2);
    ASTNodeBasePrint(&cnode->base, indents + 2);
  } else {
    Indent(indents);
    printf("default\n");
  }
}

static ASTNodeVirtuals case_label_vtbl = {CaseLabelASTNodeDelete,
                                          CaseLabelASTNodePrint, NULL};

ASTNode* NewCaseLabelASTNode(ASTNode* expr, SourceLocation location) {
  CaseLabelASTNode* node = malloc(sizeof(CaseLabelASTNode));
  ASTNodeInit(&node->base, AST_OP(case), NULL, location, &case_label_vtbl);
  node->expr = expr;
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
  ASTNodeBaseDelete(node);
}

static void SwitchStatementASTNodePrint(ASTNode* node, int indents) {
  SwitchStatementASTNode* snode = (SwitchStatementASTNode*)node;
  if (snode->expr != NULL) {
    ASTNodePrint(snode->expr, indents + 2);
  }
  ASTNodeBasePrint(&snode->base, indents);
  if (snode->stmt != NULL) {
    ASTNodePrint(snode->stmt, indents + 2);
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
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals switch_stmt_vtbl = {SwitchStatementASTNodeDelete,
                                           SwitchStatementASTNodePrint,
                                           SwitchStatementASTNodeReplaceChild};

ASTNode* NewSwitchStatementASTNode(ASTNode* expr, ASTNode* stmt,
                                   SourceLocation location) {
  SwitchStatementASTNode* node = malloc(sizeof(SwitchStatementASTNode));
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
  return (ASTNode*)node;
}

static void LabelASTNodeDelete(ASTNode* node) {
  LabelASTNode* lnode = (LabelASTNode*)node;
  StringDestruct(&lnode->name);
  ASTNodeBaseDelete(node);
}

static void LabelASTNodePrint(ASTNode* node, int indents) {
  LabelASTNode* lnode = (LabelASTNode*)node;
  ASTNodeBasePrint(&lnode->base, indents + 2);
  printf(" %s\n", lnode->name.value);
}

static ASTNodeVirtuals label_vtbl = {LabelASTNodeDelete, LabelASTNodePrint,
                                     NULL};

ASTNode* NewLabelASTNode(const char* name, SourceLocation location) {
  LabelASTNode* node = malloc(sizeof(LabelASTNode));
  ASTNodeInit(&node->base, AST_OP(label), NULL, location, &label_vtbl);
  StringInit(&node->name, name);
  node->label = NULL;
  return (ASTNode*)node;
}

static void AsmASTNodeDelete(ASTNode* node) {
  AsmASTNode* anode = (AsmASTNode*)node;
  StringDelete(anode->text);
  ASTNodeBaseDelete(node);
}

static void AsmASTNodePrint(ASTNode* node, int indents) {
  AsmASTNode* lnode = (AsmASTNode*)node;
  ASTNodeBasePrint(&lnode->base, indents + 2);
  printf(" %s\n", lnode->text->value);
}

static ASTNodeVirtuals asm_vtbl = {AsmASTNodeDelete, AsmASTNodePrint, NULL};

ASTNode* NewAsmASTNode(String* text, bool is_volatile,
                       SourceLocation location) {
  AsmASTNode* node = malloc(sizeof(AsmASTNode));
  TypeRecord* type = NewTypeRecord(kTypeVoid, kQualPlain);
  ASTNodeInit(&node->base, AST_OP(asm), type, location, &asm_vtbl);
  node->text = text;
  node->is_volatile = is_volatile;
  return (ASTNode*)node;
}

//
// Initializer AST nodes.
//

static void ExpressionInitializerASTNodeDelete(ASTNode* node) {
  ExpressionInitializerASTNode* enode = (ExpressionInitializerASTNode*)node;
  ASTNodeDelete(enode->expr);
  ASTNodeBaseDelete(node);
}

static void ExpressionInitializerASTNodePrint(ASTNode* node, int indents) {
  ExpressionInitializerASTNode* enode = (ExpressionInitializerASTNode*)node;
  Indent(indents);
  printf("expr-init\n");
  ASTNodePrint(enode->expr, indents + 2);
}

static void ExpressionInitializerASTNodeReplaceChild(ASTNode* parent,
                                                     int child_id,
                                                     ASTNode* child,
                                                     bool delete_old_child) {
  ExpressionInitializerASTNode* node = (ExpressionInitializerASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals expr_init_vtbl = {
    ExpressionInitializerASTNodeDelete, ExpressionInitializerASTNodePrint,
    ExpressionInitializerASTNodeReplaceChild};

ASTNode* NewExpressionInitializerASTNode(ASTNode* expr,
                                         SourceLocation location) {
  ExpressionInitializerASTNode* node =
      malloc(sizeof(ExpressionInitializerASTNode));
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

static void BracedInitializerASTNodePrint(ASTNode* node, int indents) {
  BracedInitializerASTNode* lnode = (BracedInitializerASTNode*)node;
  for (size_t i = 0; i < lnode->initializers->length; i++) {
    ASTNode* init = (ASTNode*)lnode->initializers->value.p[i];
    if (init != NULL) {
      ASTNodePrint(init, indents + 2);
    }
  }
  Indent(indents);
  printf("braced-init\n");
}

static void BracedInitializerASTNodeReplaceChild(ASTNode* parent, int child_id,
                                                 ASTNode* child,
                                                 bool delete_old_child) {
  BracedInitializerASTNode* lnode = (BracedInitializerASTNode*)parent;
  ASTNode* old = lnode->initializers->value.p[child_id];
  lnode->initializers->value.p[child_id] = child;
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals braced_init_vtbl = {
    BracedInitializerASTNodeDelete, BracedInitializerASTNodePrint,
    BracedInitializerASTNodeReplaceChild};

ASTNode* NewBracedInitializerASTNode(Vector* initializers,
                                     SourceLocation location) {
  BracedInitializerASTNode* node = malloc(sizeof(BracedInitializerASTNode));
  ASTNodeInit(&node->base, AST_OP(braced_init), NULL, location,
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
  d->type = type;
  TypeRecordIncRef(type);
  return d;
}

Designator* NewStructDesignator(String* member) {
  Designator* d = malloc(sizeof(Designator));
  d->designator_type = kDesignatorStruct;
  d->value.struct_member_name = member;
  return d;
}

Designator* NewStructMemberDesignator(StructMember* member) {
  Designator* d = malloc(sizeof(Designator));
  d->designator_type = kDesignatorStruct;
  d->value.struct_member = member;
  return d;
}

static void DesignatedInitializerASTNodeDelete(ASTNode* node) {
  DesignatedInitializerASTNode* dnode = (DesignatedInitializerASTNode*)node;
  VectorDelete(dnode->designators);
  ASTNodeDelete(dnode->init);
  ASTNodeBaseDelete(node);
}

static void DesignatedInitializerASTNodePrint(ASTNode* node, int indents) {
  DesignatedInitializerASTNode* dnode = (DesignatedInitializerASTNode*)node;
  Indent(indents + 2);
  if (dnode->designators != NULL) {
    for (size_t i = 0; i < dnode->designators->length; i++) {
      Designator* d = (Designator*)dnode->designators->value.p[i];
      if (d->designator_type == kDesignatorArray) {
        printf("[%d]", d->value.array_index);
      } else {
        if (d->value.struct_member != NULL) {
          printf(".%s", d->value.struct_member->symbol->name.value);
        } else if (d->value.struct_member != NULL) {
          printf(".%s", d->value.struct_member_name->value);
        }
      }
    }
    printf("\n");
  }
  Indent(indents);
  printf("designated-initializer\n");
  ASTNodePrint(dnode->init, indents + 2);
  ASTNodeBasePrint(node, indents);
}

static void DesignatedInitializerASTNodeReplaceChild(ASTNode* parent,
                                                     int child_id,
                                                     ASTNode* child,
                                                     bool delete_old_child) {
  DesignatedInitializerASTNode* node = (DesignatedInitializerASTNode*)parent;
  ASTNode* old = node->init;
  node->init = child;
  child->parent = parent;
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNodeVirtuals designated_init_vtbl = {
    DesignatedInitializerASTNodeDelete, DesignatedInitializerASTNodePrint,
    DesignatedInitializerASTNodeReplaceChild};

ASTNode* NewDesignatedInitializerASTNode(Vector* designators, ASTNode* init,
                                         SourceLocation location) {
  DesignatedInitializerASTNode* node =
      malloc(sizeof(DesignatedInitializerASTNode));
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
