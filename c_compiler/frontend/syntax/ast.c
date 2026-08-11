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
#include "concepts.h"
#include "errors.h"
#include "symbol.h"
#include "compiler.h"
#include "type_compare.h"

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
    case AST_OP(dotstar):
      return ".*";
    case AST_OP(arrowstar):
      return "->*";
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
    case AST_OP(try):
      return "try";
    case AST_OP(catch):
      return "catch";
    case AST_OP(less):
      return "<";
    case AST_OP(lesseq):
      return "<=";
    case AST_OP(spaceship):
      return "<=>";
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
    case AST_OP(co_return):
      return "co_return";
    case AST_OP(throw):
      return "throw";
    case AST_OP(co_await):
      return "co_await";
    case AST_OP(co_yield):
      return "co_yield";
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
    case AST_OP(alignof):
      return "alignof";
    case AST_OP(typeid):
      return "typeid";
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
    case AST_OP(member_ptr):
      return "member_ptr";
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
    case AST_OP(builtin_atomic_load):
      return "builtin_atomic_load";
    case AST_OP(builtin_atomic_store):
      return "builtin_atomic_store";
    case AST_OP(builtin_atomic_fetch_add):
      return "builtin_atomic_fetch_add";
    case AST_OP(builtin_atomic_fetch_sub):
      return "builtin_atomic_fetch_sub";
    case AST_OP(builtin_atomic_add_fetch):
      return "builtin_atomic_add_fetch";
    case AST_OP(builtin_atomic_sub_fetch):
      return "builtin_atomic_sub_fetch";
    case AST_OP(builtin_atomic_compare_exchange_bool):
      return "builtin_atomic_compare_exchange_bool";
    case AST_OP(builtin_atomic_compare_exchange_val):
      return "builtin_atomic_compare_exchange_val";
    case AST_OP(builtin_atomic_compare_exchange_n):
      return "builtin_atomic_compare_exchange_n";
    case AST_OP(builtin_atomic_fence):
      return "builtin_atomic_fence";
    case AST_OP(builtin_source_file):
      return "builtin_source_file";
    case AST_OP(builtin_source_line):
      return "builtin_source_line";
    case AST_OP(builtin_source_column):
      return "builtin_source_column";
    case AST_OP(builtin_source_function):
      return "builtin_source_function";
    case AST_OP(builtin_source_pretty_function):
      return "builtin_source_pretty_function";
    case AST_OP(noexcept_expr):
      return "noexcept_expr";
    case AST_OP(builtin_expect):
      return "builtin_expect";
    case AST_OP(builtin_prefetch):
      return "builtin_prefetch";
    case AST_OP(builtin_clz):
      return "builtin_clz";
    case AST_OP(builtin_ctz):
      return "builtin_ctz";
    case AST_OP(builtin_popcount):
      return "builtin_popcount";
    case AST_OP(builtin_rotl):
      return "builtin_rotl";
    case AST_OP(builtin_rotr):
      return "builtin_rotr";
    case AST_OP(builtin_trap):
      return "builtin_trap";
    case AST_OP(builtin_unreachable):
      return "builtin_unreachable";

    case AST_OP(cast):
      return "cast";
    case AST_OP(label):
      return "label";
    case AST_OP(vardecl):
      return "variable";
    case AST_OP(structured_binding):
      return "structured_binding";
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
    case AST_OP(range_begin):
      return "range-begin";
    case AST_OP(range_end):
      return "range-end";

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
  // Once the arena has been released every node struct has already been
  // destructed (via the ast_all_nodes sweep in ASTArenaRelease) and its backing
  // memory freed, so ast_arena is NULL.  Later teardown steps still hold
  // pointers to arena nodes -- e.g. Symbol default arguments and variable-
  // template / partial-specialization initializers reached from SymbolDestruct
  // -- and would call ASTNodeDelete on them.  Reading such a freed node here is
  // an order-dependent, layout-sensitive use-after-free, so bail out: the node
  // is already destructed and there is nothing left to release.
  if (ast_arena == NULL) {
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
  if (node == NULL || type == NULL) {
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

void ASTNodeClearType(ASTNode* node) {
  if (node == NULL || node->type == NULL) {
    return;
  }
  TypeRecordDelete(node->type);
  node->type = NULL;
}

static bool ASTTypeIsFunctionTemplatePrimary(TypeRecord* type) {
  if (type == NULL || !TypeIsFunction(type)) {
    return false;
  }
  Symbol* sym = type->info.function.symbol;
  return sym != NULL && sym->flags.is_template && sym->type == type;
}

void ASTNodeSetInstantiatedCalleeType(ASTNode* node, TypeRecord* type) {
  if (type == NULL) {
    return;
  }
  if (node->type == type) {
    return;
  }
  TypeRecord* old = node->type;
  TypeRecordIncRef(type);
  node->type = type;
  if (old != NULL && !ASTTypeIsFunctionTemplatePrimary(old)) {
    TypeRecordDelete(old);
  }
}

void ASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  if (node == NULL) {
    return;
  }
  assert(node->virtuals->printer != NULL);
  node->virtuals->printer(node, indents, fp);
}

typedef struct {
  int indents;
  int depth;
  FILE* fp;
} ASTTreePrintContext;

static void ASTTreePrintNodeDescription(ASTNode* node, FILE* fp) {
  fprintf(fp, "(#%d) %s", node->id, ASTOpcodeName(node->op));
  switch (node->op) {
    case AST_OP(identifier): {
      IdentifierASTNode* identifier = (IdentifierASTNode*)node;
      if (identifier->symbol != NULL) {
        fprintf(fp, " %s", identifier->symbol->name.value);
      }
      break;
    }
    case AST_OP(structmember): {
      StructMemberASTNode* member = (StructMemberASTNode*)node;
      if (member->member != NULL && member->member->symbol != NULL) {
        fprintf(fp, " %s@%d", member->member->symbol->name.value,
                member->byte_offset);
      }
      break;
    }
    case AST_OP(number):
    case AST_OP(charconst):
      fprintf(fp, " %" PRId64, ((ConstantASTNode*)node)->value.ivalue);
      break;
    case AST_OP(fnumber):
      fprintf(fp, " %g", ((ConstantASTNode*)node)->value.fvalue);
      break;
    case AST_OP(string):
    case AST_OP(string_wide): {
      String escaped = {0};
      StringEscape(((ConstantASTNode*)node)->value.string, &escaped);
      fprintf(fp, " \"%s\"", escaped.value);
      StringDestruct(&escaped);
      break;
    }
    case AST_OP(vardecl): {
      VariableDeclarationASTNode* declaration =
          (VariableDeclarationASTNode*)node;
      if (declaration->symbol != NULL) {
        fprintf(fp, " %s", declaration->symbol->name.value);
      }
      break;
    }
    default:
      break;
  }
  if (node->type != NULL) {
    String type_name;
    StringInit(&type_name, "");
    TypeRecordToString(node->type, &type_name);
    fprintf(fp, " -> %s", type_name.value);
    StringDestruct(&type_name);
  }
  fputc('\n', fp);
}

static void ASTTreePrintVisitor(ASTNode* node, void* data, int child_id,
                                VisitorMode mode) {
  (void)child_id;
  if (node == NULL) {
    return;
  }
  ASTTreePrintContext* context = data;
  if (mode == kVisitPostChildren) {
    context->depth--;
    return;
  }
  Indent(context->indents, context->fp);
  for (int i = 1; i < context->depth; i++) {
    fputs("|   ", context->fp);
  }
  if (context->depth > 0) {
    fputs("+-- ", context->fp);
  }
  ASTTreePrintNodeDescription(node, context->fp);
  if (node->virtuals->visitor != NULL) {
    context->depth++;
  }
}

void ASTNodePrintTree(ASTNode* node, int indents, FILE* fp) {
  if (node == NULL) {
    return;
  }
  ASTTreePrintContext context = {
      .indents = indents,
      .depth = 0,
      .fp = fp,
  };
  ASTNodeVisit(node, ASTTreePrintVisitor, 0, &context);
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

static bool* ast_visit_early_stop;

void ASTNodeVisit(ASTNode* node,
                  void (*func)(ASTNode* node, void*, int, VisitorMode),
                  int child_id,
                  void* data) {
  if (node == NULL ||
      (ast_visit_early_stop != NULL && *ast_visit_early_stop)) {
    return;
  }
  if (node->virtuals->visitor == NULL) {
    // Base class only.
    func(node, data, child_id, kVisitPreChildren);
    return;
  }
  node->virtuals->visitor(node, func, child_id, data);
}

typedef struct {
  ASTNodeUpwardVisitor predicate;
  void* data;
  bool* found;
} ASTNodeAnyContext;

static void ASTNodeAnyVisitor(ASTNode* node, void* data, int child_id,
                              VisitorMode mode) {
  (void)child_id;
  ASTNodeAnyContext* context = data;
  if (mode == kVisitPreChildren && !*context->found &&
      context->predicate(node, context->data)) {
    *context->found = true;
  }
}

bool ASTNodeAny(ASTNode* node, ASTNodeUpwardVisitor predicate, void* data) {
  if (node == NULL || predicate == NULL) {
    return false;
  }
  bool found = false;
  bool* saved_stop = ast_visit_early_stop;
  ast_visit_early_stop = &found;
  ASTNodeAnyContext context = {predicate, data, &found};
  ASTNodeVisit(node, ASTNodeAnyVisitor, 0, &context);
  ast_visit_early_stop = saved_stop;
  return found;
}

void ASTNodeVisitUpwards(ASTNode* node, ASTNodeUpwardVisitor func,
                         void* data) {
  if (func == NULL) {
    return;
  }
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    if (!func(current, data)) {
      return;
    }
  }
}

static ASTNode* ASTNodeTransformCurrent(ASTNode* node, ASTNodeTransformer func,
                                        void* data) {
  ASTNodeTransformAction action = kASTTransformContinue;
  ASTNode* transformed = func(node, data, &action);
  if (transformed == NULL || transformed != node ||
      action == kASTTransformSkipChildren) {
    return transformed;
  }
  if (node->virtuals->transformer != NULL) {
    node->virtuals->transformer(node, func, data);
  }
  return node;
}

ASTNode* ASTNodeVisitAndTransform(ASTNode* node, ASTNodeTransformer func,
                                  void* data) {
  if (node == NULL || func == NULL) {
    return node;
  }
  return ASTNodeTransformCurrent(node, func, data);
}

ASTNode* ASTNodeVisitAndTransformUpwards(ASTNode* node,
                                         ASTNodeTransformer func,
                                         void* data) {
  if (node == NULL || func == NULL) {
    return node;
  }
  ASTNode* result = node;
  for (ASTNode* current = node; current != NULL;) {
    ASTNode* parent = current->parent;
    int child_id = current->child_id;
    ASTNodeTransformAction action = kASTTransformContinue;
    ASTNode* transformed = func(current, data, &action);
    if (transformed != current) {
      if (current == result) {
        result = transformed;
      }
      if (parent != NULL) {
        ASTNodeReplaceChild(parent, child_id, transformed, true);
      }
    }
    if (action == kASTTransformSkipChildren) {
      return result;
    }
    current = transformed != NULL ? transformed->parent : parent;
  }
  return result;
}

bool ASTNodeIsStatement(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  switch (node->op) {
    case AST_OP(asm):
    case AST_OP(break):
    case AST_OP(case):
    case AST_OP(co_return):
    case AST_OP(compound):
    case AST_OP(continue):
    case AST_OP(decl_list):
    case AST_OP(do):
    case AST_OP(expr):
    case AST_OP(for):
    case AST_OP(goto):
    case AST_OP(if):
    case AST_OP(label):
    case AST_OP(return):
    case AST_OP(switch):
    case AST_OP(throw):
    case AST_OP(try):
    case AST_OP(while):
      return true;
    default:
      return false;
  }
}

bool ASTNodeChildIsStatement(ASTNode* parent, int child_id) {
  if (parent == NULL) {
    return false;
  }
  switch (parent->op) {
    case AST_OP(compound):
      return true;
    case AST_OP(if):
      return child_id == 1 || child_id == 2;
    case AST_OP(while):
    case AST_OP(do):
      return child_id == 1;
    case AST_OP(for):
      return child_id == 3;
    case AST_OP(switch):
      return child_id == 1;
    case AST_OP(try):
      return child_id == 0;
    case AST_OP(catch):
      return child_id == 0;
    case AST_OP(case):
      return child_id == 1;
    case AST_OP(label):
      return child_id == 0;
    default:
      return false;
  }
}

static void ASTNodeTransformChild(ASTNode* parent, int child_id,
                                  ASTNode* child, ASTNodeTransformer func,
                                  void* data) {
  ASTNode* transformed = ASTNodeVisitAndTransform(child, func, data);
  if (transformed != child) {
    ASTNodeReplaceChild(parent, child_id, transformed, true);
  } else {
    SetParent(child, parent, child_id);
  }
}

static void ASTNodeTransformVectorElement(ASTNode* parent, Vector* children,
                                          size_t index,
                                          ASTNodeTransformer func,
                                          void* data) {
  ASTNode* child = children->value.p[index];
  ASTNode* transformed = ASTNodeVisitAndTransform(child, func, data);
  if (transformed == child) {
    SetParent(child, parent, (int)index);
    return;
  }
  if (transformed == NULL) {
    ASTNodeDelete(child);
    VectorDeleteElement(children, index);
    for (size_t i = index; i < children->length; i++) {
      ASTNode* remaining = children->value.p[i];
      if (remaining != NULL) {
        remaining->child_id = (int)i;
      }
    }
    return;
  }
  children->value.p[index] = transformed;
  SetParent(transformed, parent, (int)index);
  ASTNodeDelete(child);
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
    case AST_OP(alignof):
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
    case AST_OP(alignof):
      return ((SizeofASTNode*)node)->base.value.ivalue;
    default:
      assert(false);
      return false;
  }
}

// True for "call-like" nodes: an ordinary or inline call and the variadic /
// atomic builtins.  These are all VectorASTNodes whose callee/target occupies
// child slot 0 and whose argument list is stored in the child vector starting
// at child_id 1, so callers indexing the argument vector must subtract one.
bool ASTIsCallNode(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  switch (node->op) {
    case AST_OP(call):
    case AST_OP(inline_call):
    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_arg):
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_copy):
    case AST_OP(builtin_atomic_load):
    case AST_OP(builtin_atomic_store):
    case AST_OP(builtin_atomic_fetch_add):
    case AST_OP(builtin_atomic_fetch_sub):
    case AST_OP(builtin_atomic_add_fetch):
    case AST_OP(builtin_atomic_sub_fetch):
    case AST_OP(builtin_atomic_compare_exchange_bool):
    case AST_OP(builtin_atomic_compare_exchange_val):
    case AST_OP(builtin_atomic_compare_exchange_n):
    case AST_OP(builtin_atomic_fence):
      return true;
    default:
      return false;
  }
}

static void IdentifierASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  Indent(indents, fp);
  IdentifierASTNode* inode = (IdentifierASTNode*)node;
  fprintf(fp,"%s\n", inode->symbol->name.value);
  ASTNodeBasePrint(node, indents + 2, fp);
}

static void IdentifierASTNodeDelete(ASTNode* node) {
  IdentifierASTNode* inode = (IdentifierASTNode*)node;
  if (inode->template_arguments != NULL) {
    VectorDeleteWithContents(inode->template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  ASTNodeBaseDelete(node);
}

static ASTNode* IdentifierASTNodeClone(const ASTNode* node,
                                       ASTNode* (*func)(ASTNode* node, void*),
                                       void* data) {
  IdentifierASTNode* from = (IdentifierASTNode*)node;
  IdentifierASTNode* to = ASTArenaAlloc(sizeof(IdentifierASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->symbol = from->symbol;
  to->template_arguments =
      TemplateArgumentVectorCopy(from->template_arguments);
  return func(&to->base, data);
}

static ASTNodeVirtuals identifier_vtbl = {IdentifierASTNodeDelete,
                                          IdentifierASTNodePrint, NULL,
                                          IdentifierASTNodeClone, NULL,
                                          NULL,
};

ASTNode* NewIdentifierASTNode(Symbol* symbol, SourceLocation location) {
  IdentifierASTNode* node = ASTArenaAlloc(sizeof(IdentifierASTNode));
  ASTNodeInit(&node->base, AST_OP(identifier), symbol->type, location,
              &identifier_vtbl);
  node->symbol = symbol;
  node->template_arguments = NULL;
  return (ASTNode*)node;
}

ASTNode* NewRawIdentifierASTNode(void* symbol, SourceLocation location) {
  IdentifierASTNode* node = ASTArenaAlloc(sizeof(IdentifierASTNode));
  ASTNodeInit(&node->base, AST_OP(identifier), NULL, location,
              &identifier_vtbl);
  node->symbol = symbol;
  node->template_arguments = NULL;
  return (ASTNode*)node;
}

static void StructMemberASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  Indent(indents, fp);
  StructMemberASTNode* mnode = (StructMemberASTNode*)node;
  fprintf(fp,"%s@%d\n", mnode->member->symbol->name.value,
         mnode->byte_offset);
  ASTNodeBasePrint(node, indents + 2, fp);
}

static void StructMemberASTNodeDelete(ASTNode* node) {
  StructMemberASTNode* mnode = (StructMemberASTNode*)node;
  if (mnode->template_arguments != NULL) {
    VectorDeleteWithContents(mnode->template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  TypeRecordDelete(mnode->owner_type);
  ASTNodeBaseDelete(node);
}

static ASTNode* StructMemberASTNodeClone(const ASTNode* node,
                                         ASTNode* (*func)(ASTNode* node, void*),
                                         void* data) {
  StructMemberASTNode* from = (StructMemberASTNode*)node;
  StructMemberASTNode* to = ASTArenaAlloc(sizeof(StructMemberASTNode));
  ASTNodeBaseCopy(&to->base, node);
  StructMemberASTNodeSetMember(to, from->member);
  to->template_arguments =
      TemplateArgumentVectorCopy(from->template_arguments);
  to->owner_type = from->owner_type;
  TypeRecordIncRef(to->owner_type);
  return func(&to->base, data);
}

static ASTNodeVirtuals struct_member_vtbl = {StructMemberASTNodeDelete,
                                             StructMemberASTNodePrint, NULL,
                                             StructMemberASTNodeClone, NULL,
                                             NULL,
};

void StructMemberASTNodeSetMember(StructMemberASTNode* node,
                                  StructMember* member) {
  if (node == NULL || member == NULL) {
    return;
  }
  node->member = member;
  node->access = member->access;
  node->byte_offset = member->byte_offset;
}

ASTNode* NewStructMemberASTNode(StructMember* member, SourceLocation location) {
  StructMemberASTNode* node = ASTArenaAlloc(sizeof(StructMemberASTNode));
  ASTNodeInit(&node->base, AST_OP(structmember), member->symbol->type, location,
              &struct_member_vtbl);
  StructMemberASTNodeSetMember(node, member);
  node->template_arguments = NULL;
  node->owner_type = NULL;
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
    case AST_OP(sizeof):
    case AST_OP(alignof): {
      fprintf(fp,"%s\n", node->op == AST_OP(sizeof) ? "sizeof" : "alignof");
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
  if (cnode->template_arguments != NULL) {
    VectorDeleteWithContents(cnode->template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
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
  to->template_arguments =
      TemplateArgumentVectorCopy(from->template_arguments);
  return func(&to->base, data);
}

static ASTNodeVirtuals constant_vtbl = {ConstantASTNodeDelete,
                                        ConstantASTNodePrint,
                                        NULL, ConstantASTNodeClone, NULL, NULL};

void IntConstantASTNodeInit(ConstantASTNode* node, int64_t value,
                            TypeRecord* type, SourceLocation location) {
  ASTNodeInit(&node->base, AST_OP(number), type, location, &constant_vtbl);
  node->value.ivalue = value;
  node->template_arguments = NULL;
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
  node->template_arguments = NULL;
  return (ASTNode*)node;
}

ASTNode* NewStringConstantASTNode(String* value, TypeRecord* type,
                                  SourceLocation location) {
  ConstantASTNode* node = ASTArenaAlloc(sizeof(ConstantASTNode));
  ASTNodeInit(&node->base, AST_OP(string), type, location, &constant_vtbl);
  node->base.value_category = kValueCategoryLvalue;
  node->value.string = value;
  node->template_arguments = NULL;
  return (ASTNode*)node;
}

ASTNode* NewWideStringConstantASTNode(String* value, TypeRecord* type,
                                      SourceLocation location) {
  ConstantASTNode* node = ASTArenaAlloc(sizeof(ConstantASTNode));
  ASTNodeInit(&node->base, AST_OP(string_wide), type, location, &constant_vtbl);
  node->base.value_category = kValueCategoryLvalue;
  node->value.string = value;
  node->template_arguments = NULL;
  return (ASTNode*)node;
}

ASTNode* NewCharConstantASTNode(int value, TypeRecord* type,
                                SourceLocation location) {
  ConstantASTNode* node = ASTArenaAlloc(sizeof(ConstantASTNode));
  ASTNodeInit(&node->base, AST_OP(charconst), type, location, &constant_vtbl);
  node->value.ivalue = value;
  node->template_arguments = NULL;
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

static void UnaryASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                  void* data) {
  UnaryASTNode* n = (UnaryASTNode*)node;
  ASTNodeTransformChild(node, 0, n->sub, func, data);
}

static ASTNodeVirtuals unary_vtbl = {UnaryASTNodeDelete, UnaryASTNodePrint,
                                     UnaryASTNodeReplaceChild,
                                     UnaryASTNodeClone, UnaryASTNodeVisit,
                                     ValueAlwaysUsed, UnaryASTNodeTransform};

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

static void BinaryASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                   void* data) {
  BinaryASTNode* n = (BinaryASTNode*)node;
  ASTNodeTransformChild(node, 0, n->left, func, data);
  ASTNodeTransformChild(node, 1, n->right, func, data);
}

static ASTNodeVirtuals binary_vtbl = {BinaryASTNodeDelete, BinaryASTNodePrint,
                                      BinaryASTNodeReplaceChild,
                                      BinaryASTNodeClone, BinaryASTNodeVisit,
  ValueAlwaysUsed, BinaryASTNodeTransform
};

// Binary AST node, which a left and right child.
ASTNode* NewBinaryASTNode(ASTOpcode op, TypeRecord* type,
                          SourceLocation location, ASTNode* left,
                          ASTNode* right) {
  BinaryASTNode* node = ASTArenaAlloc(sizeof(BinaryASTNode));
  ASTNodeInit(&node->base, op, type, location, &binary_vtbl);
  node->left = left;
  if (left != NULL) {
    left->parent = (ASTNode*)node;
    left->child_id = 0;
  }
  node->right = right;
  if (right != NULL) {
    right->parent = (ASTNode*)node;
    right->child_id = 1;
  }
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

typedef struct {
  InlineCallASTNode* owner;
  GotoStatementASTNode* jump;
} InlineCallGotoReconnect;

static bool InlineCallNodeBelongsTo(ASTNode* node,
                                    InlineCallASTNode* owner) {
  for (ASTNode* parent = node != NULL ? node->parent : NULL;
       parent != NULL; parent = parent->parent) {
    if (parent->op == AST_OP(inline_call)) {
      return parent == (ASTNode*)owner;
    }
  }
  return false;
}

static void FindInlineCallGotoLabel(ASTNode* node, void* data, int child_id,
                                    VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(label)) {
    return;
  }
  InlineCallGotoReconnect* reconnect = data;
  LabelASTNode* label = (LabelASTNode*)node;
  if (InlineCallNodeBelongsTo(node, reconnect->owner) &&
      StringEqualString(reconnect->jump->label_name, &label->name)) {
    reconnect->jump->label = node;
  }
}

static void ReconnectInlineCallGoto(ASTNode* node, void* data, int child_id,
                                    VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(goto)) {
    return;
  }
  InlineCallASTNode* owner = data;
  if (!InlineCallNodeBelongsTo(node, owner)) {
    return;
  }
  GotoStatementASTNode* jump = (GotoStatementASTNode*)node;
  jump->label = NULL;
  jump->lca = owner->inlined;
  InlineCallGotoReconnect reconnect = {
      .owner = owner,
      .jump = jump,
  };
  ASTNodeVisit(owner->inlined, FindInlineCallGotoLabel, 0, &reconnect);
}

static ASTNode* InlineCallASTNodeClone(const ASTNode* node,
                                       ASTNode* (*func)(ASTNode* node, void*),
                                       void* data) {
  InlineCallASTNode* from = (InlineCallASTNode*)node;
  InlineCallASTNode* to = ASTArenaAlloc(sizeof(InlineCallASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->inlined = ASTNodeClone(from->inlined, func, data, &to->base);
  to->ret_value = ASTNodeClone(from->ret_value, func, data, &to->base);
  ASTNodeVisit(to->inlined, ReconnectInlineCallGoto, 0, to);
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

static void InlineCallASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                       void* data) {
  InlineCallASTNode* n = (InlineCallASTNode*)node;
  ASTNodeTransformChild(node, 0, n->inlined, func, data);
  ASTNodeTransformChild(node, 1, n->ret_value, func, data);
}

static ASTNodeVirtuals inline_call_vtbl = {
    InlineCallASTNodeDelete, InlineCallASTNodePrint,
    InlineCallASTNodeReplaceChild, InlineCallASTNodeClone,
    InlineCallASTNodeVisit, ValueAlwaysUsed, InlineCallASTNodeTransform};

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

static void VectorASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                   void* data) {
  VectorASTNode* n = (VectorASTNode*)node;
  ASTNode* transformed = ASTNodeVisitAndTransform(n->left, func, data);
  if (transformed != n->left) {
    ASTNode* old = n->left;
    n->left = transformed;
    SetParent(transformed, node, 0);
    ASTNodeDelete(old);
  }
  for (size_t i = n->children->length; i > 0; i--) {
    ASTNodeTransformVectorElement(node, n->children, i - 1, func, data);
  }
}

static ASTNodeVirtuals vector_vtbl = {VectorASTNodeDelete, VectorASTNodePrint,
                                      VectorASTNodeReplaceChild,
                                      VectorASTNodeClone, VectorASTNodeVisit,
                                      ValueAlwaysUsed, VectorASTNodeTransform};

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

static void RequiresExpressionASTNodePrint(ASTNode* node, int indents,
                                           FILE* fp) {
  Indent(indents, fp);
  fprintf(fp, "requires-expression\n");
  ASTNodeBasePrint(node, indents + 2, fp);
}

static void RequiresExpressionASTNodeDelete(ASTNode* node) {
  RequiresExpressionASTNode* requires_node =
      (RequiresExpressionASTNode*)node;
  ConstraintExprDelete(requires_node->constraint);
  requires_node->constraint = NULL;
  ASTNodeBaseDelete(node);
}

static ASTNode* RequiresExpressionASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*),
    void* data) {
  const RequiresExpressionASTNode* from =
      (const RequiresExpressionASTNode*)node;
  RequiresExpressionASTNode* to =
      ASTArenaAlloc(sizeof(RequiresExpressionASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->constraint = ConceptsCloneConstraint(from->constraint);
  return func(&to->base, data);
}

static ASTNodeVirtuals requires_expr_vtbl = {
    RequiresExpressionASTNodeDelete, RequiresExpressionASTNodePrint,
    NULL, RequiresExpressionASTNodeClone, NULL, NULL};

ASTNode* NewRequiresExpressionASTNode(ConstraintExpr* constraint,
                                      SourceLocation location) {
  RequiresExpressionASTNode* node =
      ASTArenaAlloc(sizeof(RequiresExpressionASTNode));
  ASTNodeInit(&node->base, AST_OP(requires_expr),
              NewTypeRecordWithSize(kTypeBool, kQualPlain), location,
              &requires_expr_vtbl);
  node->constraint = constraint;
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
  to->dynamic_runtime = from->dynamic_runtime;
  TypeRecordIncRef(to->cast_type);
  return func(&to->base, data);
}

static void CastASTNodeVisit(ASTNode* node,
                             void (*func)(ASTNode* node, void*, int,
                                          VisitorMode),
                             int child_id, void* data) {
  CastASTNode* n = (CastASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->expr, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static bool CastASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  CastASTNode* n = (CastASTNode*)node;
  if (TypeIsVoid(n->cast_type)) {
    return false;
  }
  return ASTNodeUsesValue(node->parent, node);
}

static void CastASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                 void* data) {
  CastASTNode* n = (CastASTNode*)node;
  ASTNodeTransformChild(node, 0, n->expr, func, data);
}

static ASTNodeVirtuals cast_vtbl = {CastASTNodeDelete, CastASTNodePrint,
                                    CastASTNodeReplaceChild, CastASTNodeClone,
                                    CastASTNodeVisit,
  CastASTNodeUsesValue, CastASTNodeTransform
};

ASTNode* NewCastASTNode(TypeRecord* type, SourceLocation location,
                        ASTNode* expr) {
  CastASTNode* node = ASTArenaAlloc(sizeof(CastASTNode));
  ASTNodeInit(&node->base, AST_OP(cast), NULL, location, &cast_vtbl);
  node->cast_type = type;
  TypeRecordIncRef(type);
  node->expr = expr;
  node->kind = kCastCStyle;
  node->dynamic_runtime = false;
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

static void PtrScaleASTNodeVisit(ASTNode* node,
                                 void (*func)(ASTNode*, void*, int,
                                              VisitorMode),
                                 int child_id, void* data) {
  PtrScaleASTNode* n = (PtrScaleASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->expr, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static bool PtrScaleASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  return ASTNodeUsesValue(node->parent, node);
}

static void PtrScaleASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                     void* data) {
  PtrScaleASTNode* n = (PtrScaleASTNode*)node;
  ASTNodeTransformChild(node, 0, n->expr, func, data);
}

static ASTNodeVirtuals ptr_scale_vtbl = {PtrScaleASTNodeDelete, PtrScaleASTNodePrint,
                                    PtrScaleASTNodeReplaceChild, PtrScaleASTNodeClone,
  PtrScaleASTNodeVisit,
  PtrScaleASTNodeUsesValue, PtrScaleASTNodeTransform,
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
  if (snode->type_operand != NULL) {
    TypeRecordDelete(snode->type_operand);
  }
  ASTNodeBaseDelete(node);
}

void SizeofASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  SizeofASTNode* snode = (SizeofASTNode*)node;
  fprintf(fp, "%s ", node->op == AST_OP(sizeof) ? "sizeof" : "alignof");
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
  to->type_operand = from->type_operand;
  if (to->type_operand != NULL) {
    TypeRecordIncRef(to->type_operand);
  }
  to->is_pack_size = from->is_pack_size;
  return func(&to->base.base, data);
}

static void SizeofASTNodeVisit(ASTNode* node,
                               void (*func)(ASTNode* node, void*, int,
                                            VisitorMode),
                               int child_id, void* data) {
  SizeofASTNode* n = (SizeofASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->expr, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static void SizeofASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                   void* data) {
  SizeofASTNode* n = (SizeofASTNode*)node;
  ASTNodeTransformChild(node, 0, n->expr, func, data);
}

static ASTNodeVirtuals sizeof_vtbl = {SizeofASTNodeDelete, SizeofASTNodePrint,
                                      SizeofASTNodeReplaceChild,
                                      SizeofASTNodeClone, SizeofASTNodeVisit,
                                      ValueAlwaysUsed, SizeofASTNodeTransform};

ASTNode* NewSizeofASTNodeWithKnownSize(int size, SourceLocation location) {
  SizeofASTNode* node = ASTArenaAlloc(sizeof(SizeofASTNode));
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnsigned, kQualConst);
  IntConstantASTNodeInit(&node->base, size, type, location);
  node->base.base.virtuals = &sizeof_vtbl;
  node->expr = NULL;
  node->type_operand = NULL;
  node->is_pack_size = false;
  node->base.base.op = AST_OP(sizeof);
  return (ASTNode*)node;
}

ASTNode* NewSizeofASTNodeWithType(TypeRecord* type, SourceLocation location) {
  SizeofASTNode* node = ASTArenaAlloc(sizeof(SizeofASTNode));
  TypeRecord* result_type =
      NewTypeRecordWithSize(kTypeInt | kTypeUnsigned, kQualConst);
  IntConstantASTNodeInit(&node->base, type != NULL ? type->size : 0,
                         result_type, location);
  node->base.base.virtuals = &sizeof_vtbl;
  node->expr = NULL;
  node->type_operand = type;
  if (type != NULL) {
    TypeRecordIncRef(type);
  }
  node->is_pack_size = false;
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
  node->type_operand = NULL;
  node->is_pack_size = false;
  expr->parent = (ASTNode*)node;
  return (ASTNode*)node;
}

ASTNode* NewSizeofPackASTNode(ASTNode* expr, SourceLocation location) {
  ASTNode* node = NewSizeofASTNodeWithExpression(expr, location);
  ((SizeofASTNode*)node)->is_pack_size = true;
  return node;
}

ASTNode* NewAlignofASTNodeWithKnownAlignment(int alignment,
                                             SourceLocation location) {
  ASTNode* node = NewSizeofASTNodeWithKnownSize(alignment, location);
  node->op = AST_OP(alignof);
  return node;
}

ASTNode* NewAlignofASTNodeWithType(TypeRecord* type, SourceLocation location) {
  ASTNode* node = NewSizeofASTNodeWithType(type, location);
  node->op = AST_OP(alignof);
  return node;
}

ASTNode* NewAlignofASTNodeWithExpression(ASTNode* expr,
                                         SourceLocation location) {
  ASTNode* node = NewSizeofASTNodeWithExpression(expr, location);
  node->op = AST_OP(alignof);
  return node;
}

// typeid AST Node

static void TypeidASTNodeDelete(ASTNode* node) {
  TypeidASTNode* tnode = (TypeidASTNode*)node;
  if (tnode->expr != NULL) {
    ASTNodeDelete(tnode->expr);
  }
  ASTNodeBaseDelete(node);
}

static void TypeidASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  TypeidASTNode* tnode = (TypeidASTNode*)node;
  fprintf(fp, "typeid ");
  if (tnode->expr != NULL) {
    ASTNodePrint(tnode->expr, indents + 2, fp);
  }
}

static void TypeidASTNodeReplaceChild(ASTNode* parent, int child_id,
                                      ASTNode* child, bool delete_old_child) {
  TypeidASTNode* node = (TypeidASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* TypeidASTNodeClone(const ASTNode* node,
                                   ASTNode* (*func)(ASTNode* node, void*),
                                   void* data) {
  TypeidASTNode* from = (TypeidASTNode*)node;
  TypeidASTNode* to = ASTArenaAlloc(sizeof(TypeidASTNode));
  memcpy(&to->base, &from->base, sizeof(to->base));
  to->expr = from->expr != NULL ? ASTNodeClone(from->expr, func, data, &to->base)
                                : NULL;
  to->operand_type = from->operand_type;
  return func(&to->base, data);
}

static void TypeidASTNodeVisit(ASTNode* node,
                               void (*func)(ASTNode* node, void*, int,
                                            VisitorMode),
                               int child_id, void* data) {
  TypeidASTNode* n = (TypeidASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  if (n->expr != NULL) {
    ASTNodeVisit(n->expr, func, 0, data);
  }
  func(node, data, child_id, kVisitPostChildren);
}

static void TypeidASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                   void* data) {
  TypeidASTNode* n = (TypeidASTNode*)node;
  if (n->expr != NULL) {
    ASTNodeTransformChild(node, 0, n->expr, func, data);
  }
}

static ASTNodeVirtuals typeid_vtbl = {TypeidASTNodeDelete, TypeidASTNodePrint,
                                      TypeidASTNodeReplaceChild,
                                      TypeidASTNodeClone, TypeidASTNodeVisit,
                                      ValueAlwaysUsed, TypeidASTNodeTransform};

ASTNode* NewTypeidASTNodeWithType(TypeRecord* type, SourceLocation location) {
  TypeidASTNode* node = ASTArenaAlloc(sizeof(TypeidASTNode));
  ASTNodeInit(&node->base, AST_OP(typeid), NULL, location, &typeid_vtbl);
  node->expr = NULL;
  node->operand_type = type;
  return (ASTNode*)node;
}

ASTNode* NewTypeidASTNodeWithExpression(ASTNode* expr, SourceLocation location) {
  TypeidASTNode* node = ASTArenaAlloc(sizeof(TypeidASTNode));
  ASTNodeInit(&node->base, AST_OP(typeid), NULL, location, &typeid_vtbl);
  node->expr = expr;
  node->operand_type = NULL;
  if (expr != NULL) {
    expr->parent = (ASTNode*)node;
  }
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

static void ExpressionStatementASTNodeTransform(ASTNode* node,
                                                ASTNodeTransformer func,
                                                void* data) {
  ExpressionStatementASTNode* n = (ExpressionStatementASTNode*)node;
  ASTNodeTransformChild(node, 0, n->expr, func, data);
}

static ASTNodeVirtuals expr_stmt_vtbl = {
    ExpressionStatementASTNodeDelete, ExpressionStatementASTNodePrint,
    ExpressionStatementASTNodeReplaceChild, ExpressionStatementASTNodeClone,
    ExpressionStatementASTNodeVisit, ValueNotUsed,
    ExpressionStatementASTNodeTransform};

ASTNode* NewExpressionStatementASTNode(ASTNode* expr, SourceLocation location) {
  ExpressionStatementASTNode* node = ASTArenaAlloc(sizeof(ExpressionStatementASTNode));
  ASTNodeInit(&node->base, AST_OP(expr), NULL, location, &expr_stmt_vtbl);
  node->expr = expr;
  expr->parent = (ASTNode*)node;
  return (ASTNode*)node;
}

static void StaticAssertASTNodeDelete(ASTNode* node) {
  StaticAssertASTNode* assert_node = (StaticAssertASTNode*)node;
  ASTNodeDelete(assert_node->expr);
  StringDestruct(&assert_node->message);
  ASTNodeBaseDelete(node);
}

static void StaticAssertASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  StaticAssertASTNode* assert_node = (StaticAssertASTNode*)node;
  Indent(indents, fp);
  fprintf(fp, "static_assert: %s\n", assert_node->message.value);
  ASTNodePrint(assert_node->expr, indents + 2, fp);
  ASTNodeBasePrint(node, indents + 2, fp);
}

static void StaticAssertASTNodeReplaceChild(ASTNode* parent, int child_id,
                                            ASTNode* child,
                                            bool delete_old_child) {
  assert(child_id == 0);
  StaticAssertASTNode* assert_node = (StaticAssertASTNode*)parent;
  ASTNode* old = assert_node->expr;
  assert_node->expr = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* StaticAssertASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  const StaticAssertASTNode* from = (const StaticAssertASTNode*)node;
  StaticAssertASTNode* to = ASTArenaAlloc(sizeof(StaticAssertASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->expr = ASTNodeClone(from->expr, func, data, &to->base);
  StringInit(&to->message, from->message.value);
  to->base.flags &= ~kASTAnalyzed;
  return func(&to->base, data);
}

static void StaticAssertASTNodeVisit(
    ASTNode* node, void (*func)(ASTNode*, void*, int, VisitorMode),
    int child_id, void* data) {
  StaticAssertASTNode* assert_node = (StaticAssertASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(assert_node->expr, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static void StaticAssertASTNodeTransform(ASTNode* node,
                                         ASTNodeTransformer func,
                                         void* data) {
  StaticAssertASTNode* assert_node = (StaticAssertASTNode*)node;
  ASTNodeTransformChild(node, 0, assert_node->expr, func, data);
}

static ASTNodeVirtuals static_assert_vtbl = {
    StaticAssertASTNodeDelete, StaticAssertASTNodePrint,
    StaticAssertASTNodeReplaceChild, StaticAssertASTNodeClone,
    StaticAssertASTNodeVisit, ValueNotUsed, StaticAssertASTNodeTransform};

ASTNode* NewStaticAssertASTNode(ASTNode* expr, String* message,
                                SourceLocation location) {
  StaticAssertASTNode* node = ASTArenaAlloc(sizeof(StaticAssertASTNode));
  ASTNodeInit(&node->base, AST_OP(static_assert), NULL, location,
              &static_assert_vtbl);
  node->expr = expr;
  SetParent(expr, (ASTNode*)node, 0);
  StringInit(&node->message,
             message != NULL ? message->value : "static assertion failed");
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
  to->is_constexpr = from->is_constexpr;
  to->is_consteval = from->is_consteval;
  to->consteval_negated = from->consteval_negated;
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

static void IfStatementASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                        void* data) {
  IfStatementASTNode* n = (IfStatementASTNode*)node;
  ASTNodeTransformChild(node, 0, n->cond, func, data);
  ASTNodeTransformChild(node, 1, n->if_part, func, data);
  ASTNodeTransformChild(node, 2, n->else_part, func, data);
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
    IfStatementASTNodeVisit, IfStatementUsesValue,
    IfStatementASTNodeTransform};

ASTNode* NewIfStatementASTNode(ASTNode* cond, ASTNode* if_part,
                               ASTNode* else_part, bool is_constexpr,
                               SourceLocation location) {
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
  node->is_constexpr = is_constexpr;
  node->is_consteval = false;
  node->consteval_negated = false;
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

static void CombinedStatementASTNodeTransform(ASTNode* node,
                                              ASTNodeTransformer func,
                                              void* data) {
  CombinedStatementASTNode* n = (CombinedStatementASTNode*)node;
  ASTNodeTransformChild(node, 0, n->cond, func, data);
  ASTNodeTransformChild(node, 1, n->stmt, func, data);
}

static bool CombinedStatementASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  CombinedStatementASTNode* n = (CombinedStatementASTNode*)node;
  return value == n->cond;
}

static ASTNodeVirtuals combined_stmt_vtbl = {
    CombinedStatementASTNodeDelete, CombinedStatementASTNodePrint,
    CombinedStatementASTNodeReplaceChild, CombinedStatementASTNodeClone,
  CombinedStatementASTNodeVisit, CombinedStatementASTNodeUsesValue,
  CombinedStatementASTNodeTransform};

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

static void ThrowASTNodeDelete(ASTNode* node) {
  ThrowASTNode* tnode = (ThrowASTNode*)node;
  if (tnode->expr != NULL) {
    ASTNodeDelete(tnode->expr);
  }
  ASTNodeBaseDelete(node);
}

static void ThrowASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  ThrowASTNode* tnode = (ThrowASTNode*)node;
  ASTNodeBasePrint(node, indents, fp);
  if (tnode->expr != NULL) {
    ASTNodePrint(tnode->expr, indents + 2, fp);
  }
}

static void ThrowASTNodeReplaceChild(ASTNode* parent, int child_id,
                                     ASTNode* child, bool delete_old_child) {
  (void)child_id;
  ThrowASTNode* node = (ThrowASTNode*)parent;
  ASTNode* old = node->expr;
  node->expr = child;
  SetParent(child, parent, 0);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* ThrowASTNodeClone(const ASTNode* node,
                                  ASTNode* (*func)(ASTNode* node, void*),
                                  void* data) {
  ThrowASTNode* from = (ThrowASTNode*)node;
  ThrowASTNode* to = ASTArenaAlloc(sizeof(ThrowASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->expr = ASTNodeClone(from->expr, func, data, &to->base);
  return func(&to->base, data);
}

static void ThrowASTNodeVisit(ASTNode* node,
                              void (*func)(ASTNode* node, void*,
                                           int, VisitorMode),
                              int child_id, void* data) {
  ThrowASTNode* n = (ThrowASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->expr, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static void ThrowASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                  void* data) {
  ThrowASTNode* n = (ThrowASTNode*)node;
  ASTNodeTransformChild(node, 0, n->expr, func, data);
}

static ASTNodeVirtuals throw_vtbl = {
    ThrowASTNodeDelete, ThrowASTNodePrint, ThrowASTNodeReplaceChild,
    ThrowASTNodeClone, ThrowASTNodeVisit, ValueNotUsed,
    ThrowASTNodeTransform};

ASTNode* NewThrowASTNode(ASTNode* expr, SourceLocation location) {
  ThrowASTNode* node = ASTArenaAlloc(sizeof(ThrowASTNode));
  ASTNodeInit(&node->base, AST_OP(throw), NULL, location, &throw_vtbl);
  node->expr = expr;
  SetParent(expr, (ASTNode*)node, 0);
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

static void CompoundStatementASTNodeTransform(ASTNode* node,
                                              ASTNodeTransformer func,
                                              void* data) {
  CompoundStatementASTNode* n = (CompoundStatementASTNode*)node;
  for (size_t i = n->statements->length; i > 0; i--) {
    ASTNodeTransformVectorElement(node, n->statements, i - 1, func, data);
  }
  n->low_pc = VectorFirst(n->statements);
  n->high_pc = VectorLast(n->statements);
}

static ASTNodeVirtuals compound_stmt_vtbl = {
    CompoundStatementASTNodeDelete, CompoundStatementASTNodePrint,
    CompoundStatementASTNodeReplaceChild, CompoundStatementASTNodeClone,
    CompoundStatementASTNodeVisit, ValueNotUsed,
    CompoundStatementASTNodeTransform};

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

static void CatchASTNodeDelete(ASTNode* node) {
  CatchASTNode* cnode = (CatchASTNode*)node;
  if (cnode->stmt != NULL) {
    ASTNodeDelete(cnode->stmt);
  }
  ASTNodeBaseDelete(node);
}

static void CatchASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  CatchASTNode* cnode = (CatchASTNode*)node;
  Indent(indents, fp);
  fprintf(fp, "%s\n", cnode->is_catch_all ? "catch (...)" : "catch");
  if (cnode->symbol != NULL) {
    Indent(indents + 2, fp);
    fprintf(fp, "%s\n", cnode->symbol->name.value);
  }
  if (cnode->stmt != NULL) {
    ASTNodePrint(cnode->stmt, indents + 2, fp);
  }
}

static void CatchASTNodeReplaceChild(ASTNode* parent, int child_id,
                                     ASTNode* child, bool delete_old_child) {
  (void)child_id;
  CatchASTNode* node = (CatchASTNode*)parent;
  ASTNode* old = node->stmt;
  node->stmt = child;
  SetParent(child, parent, 0);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* CatchASTNodeClone(const ASTNode* node,
                                  ASTNode* (*func)(ASTNode* node, void*),
                                  void* data) {
  CatchASTNode* from = (CatchASTNode*)node;
  CatchASTNode* to = ASTArenaAlloc(sizeof(CatchASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->symbol = from->symbol;
  to->stmt = ASTNodeClone(from->stmt, func, data, &to->base);
  to->is_catch_all = from->is_catch_all;
  return func(&to->base, data);
}

static void CatchASTNodeVisit(ASTNode* node,
                              void (*func)(ASTNode* node, void*,
                                           int, VisitorMode),
                              int child_id, void* data) {
  CatchASTNode* n = (CatchASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->stmt, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static void CatchASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                  void* data) {
  CatchASTNode* n = (CatchASTNode*)node;
  ASTNodeTransformChild(node, 0, n->stmt, func, data);
}

static ASTNodeVirtuals catch_vtbl = {
    CatchASTNodeDelete, CatchASTNodePrint, CatchASTNodeReplaceChild,
    CatchASTNodeClone, CatchASTNodeVisit, ValueNotUsed,
    CatchASTNodeTransform};

ASTNode* NewCatchASTNode(Symbol* symbol, bool is_catch_all, ASTNode* stmt,
                         SourceLocation location) {
  CatchASTNode* node = ASTArenaAlloc(sizeof(CatchASTNode));
  ASTNodeInit(&node->base, AST_OP(catch), NULL, location, &catch_vtbl);
  node->symbol = symbol;
  node->stmt = stmt;
  node->is_catch_all = is_catch_all;
  SetParent(stmt, (ASTNode*)node, 0);
  return (ASTNode*)node;
}

static void TryASTNodeDelete(ASTNode* node) {
  TryASTNode* tnode = (TryASTNode*)node;
  if (tnode->try_stmt != NULL) {
    ASTNodeDelete(tnode->try_stmt);
  }
  for (size_t i = 0; i < tnode->catches->length; i++) {
    ASTNodeDelete(tnode->catches->value.p[i]);
  }
  VectorDelete(tnode->catches);
  ASTNodeBaseDelete(node);
}

static void TryASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  TryASTNode* tnode = (TryASTNode*)node;
  ASTNodeBasePrint(node, indents, fp);
  if (tnode->try_stmt != NULL) {
    ASTNodePrint(tnode->try_stmt, indents + 2, fp);
  }
  for (size_t i = 0; i < tnode->catches->length; i++) {
    ASTNodePrint(tnode->catches->value.p[i], indents + 2, fp);
  }
}

static void TryASTNodeReplaceChild(ASTNode* parent, int child_id,
                                   ASTNode* child, bool delete_old_child) {
  TryASTNode* node = (TryASTNode*)parent;
  ASTNode* old = NULL;
  if (child_id == 0) {
    old = node->try_stmt;
    node->try_stmt = child;
  } else {
    size_t index = (size_t)(child_id - 1);
    old = node->catches->value.p[index];
    node->catches->value.p[index] = child;
  }
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* TryASTNodeClone(const ASTNode* node,
                                ASTNode* (*func)(ASTNode* node, void*),
                                void* data) {
  TryASTNode* from = (TryASTNode*)node;
  TryASTNode* to = ASTArenaAlloc(sizeof(TryASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->try_stmt = ASTNodeClone(from->try_stmt, func, data, &to->base);
  to->catches = NewVector();
  for (size_t i = 0; i < from->catches->length; i++) {
    ASTNode* child =
        ASTNodeClone(from->catches->value.p[i], func, data, &to->base);
    VectorAppend(to->catches, child);
  }
  return func(&to->base, data);
}

static void TryASTNodeVisit(ASTNode* node,
                            void (*func)(ASTNode* node, void*,
                                         int, VisitorMode),
                            int child_id, void* data) {
  TryASTNode* n = (TryASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->try_stmt, func, 0, data);
  for (size_t i = 0; i < n->catches->length; i++) {
    ASTNodeVisit(n->catches->value.p[i], func, (int)i + 1, data);
  }
  func(node, data, child_id, kVisitPostChildren);
}

static void TryASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                void* data) {
  TryASTNode* n = (TryASTNode*)node;
  ASTNodeTransformChild(node, 0, n->try_stmt, func, data);
  for (size_t i = n->catches->length; i > 0; i--) {
    ASTNodeTransformVectorElement(node, n->catches, i - 1, func, data);
  }
  for (size_t i = 0; i < n->catches->length; i++) {
    SetParent(n->catches->value.p[i], node, (int)i + 1);
  }
}

static ASTNodeVirtuals try_vtbl = {
    TryASTNodeDelete, TryASTNodePrint, TryASTNodeReplaceChild,
    TryASTNodeClone, TryASTNodeVisit, ValueNotUsed, TryASTNodeTransform};

ASTNode* NewTryASTNode(ASTNode* try_stmt, Vector* catches,
                       SourceLocation location) {
  TryASTNode* node = ASTArenaAlloc(sizeof(TryASTNode));
  ASTNodeInit(&node->base, AST_OP(try), NULL, location, &try_vtbl);
  node->try_stmt = try_stmt;
  node->catches = catches;
  SetParent(try_stmt, (ASTNode*)node, 0);
  for (size_t i = 0; i < catches->length; i++) {
    SetParent(catches->value.p[i], (ASTNode*)node, (int)i + 1);
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

static void ForStatementASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                         void* data) {
  ForStatementASTNode* n = (ForStatementASTNode*)node;
  ASTNodeTransformChild(node, 0, n->c1, func, data);
  ASTNodeTransformChild(node, 1, n->c2, func, data);
  ASTNodeTransformChild(node, 2, n->c3, func, data);
  ASTNodeTransformChild(node, 3, n->stmt, func, data);
}

static bool ForStatementASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  ForStatementASTNode* n = (ForStatementASTNode*)node;
  return n->c2 == value;
}

static ASTNodeVirtuals for_stmt_vtbl = {
    ForStatementASTNodeDelete, ForStatementASTNodePrint,
    ForStatementASTNodeReplaceChild, ForStatementASTNodeClone,
    ForStatementASTNodeVisit, ForStatementASTNodeUsesValue,
    ForStatementASTNodeTransform};

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
  to->local_static_guard = from->local_static_guard;
  to->local_static_init_kind = from->local_static_init_kind;
  to->saved_sp = NULL;
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

static void VariableDeclarationASTNodeTransform(ASTNode* node,
                                                ASTNodeTransformer func,
                                                void* data) {
  VariableDeclarationASTNode* n = (VariableDeclarationASTNode*)node;
  ASTNodeTransformChild(node, 0, n->initializer, func, data);
}

static ASTNodeVirtuals var_decl_vtbl = {
    VariableDeclarationASTNodeDelete, VariableDeclarationASTNodePrint,
    VariableDeclarationASTNodeReplaceChild, VariableDeclarationASTNodeClone,
    VariableDeclarationASTNodeVisit, ValueAlwaysUsed,
    VariableDeclarationASTNodeTransform};

ASTNode* NewVariableDeclarationASTNode(Symbol* symbol, ASTNode* initializer,
                                       SourceLocation location) {
  VariableDeclarationASTNode* node = ASTArenaAlloc(sizeof(VariableDeclarationASTNode));
  ASTNodeInit(&node->base, AST_OP(vardecl), symbol->type, location,
              &var_decl_vtbl);
  node->symbol = symbol;
  node->initializer = initializer;
  node->local_static_guard = NULL;
  node->local_static_init_kind = kLocalStaticInitNone;
  if (initializer != NULL) {
    initializer->parent = (ASTNode*)node;
  }
  node->saved_sp = NULL;
  return (ASTNode*)node;
}

static void StructuredBindingASTNodeDelete(ASTNode* node) {
  StructuredBindingASTNode* binding = (StructuredBindingASTNode*)node;
  TypeRecordDelete(binding->declared_type);
  VectorDestructWithContents(binding->names,
                             (VectorElementDestructor)StringDelete,
                             /*free_element=*/false);
  VectorDelete(binding->names);
  VectorDelete(binding->symbols);
  if (binding->initializer != NULL) {
    ASTNodeDelete(binding->initializer);
  }
  ASTNodeBaseDelete(node);
}

static void StructuredBindingASTNodePrint(ASTNode* node, int indents, FILE* fp) {
  StructuredBindingASTNode* binding = (StructuredBindingASTNode*)node;
  Indent(indents, fp);
  fprintf(fp, "structured_binding");
  for (size_t i = 0; i < binding->names->length; i++) {
    String* name = binding->names->value.p[i];
    fprintf(fp, "%s%s", i == 0 ? " [" : ", ",
            name != NULL ? name->value : "<null>");
  }
  fprintf(fp, "]\n");
  if (binding->initializer != NULL) {
    ASTNodePrint(binding->initializer, indents + 2, fp);
  }
}

static void StructuredBindingASTNodeReplaceChild(ASTNode* parent, int child_id,
                                                 ASTNode* child,
                                                 bool delete_old_child) {
  StructuredBindingASTNode* node = (StructuredBindingASTNode*)parent;
  ASTNode* old = node->initializer;
  assert(child_id == 0);
  node->initializer = child;
  SetParent(child, parent, child_id);
  if (delete_old_child) {
    ASTNodeDelete(old);
  }
}

static ASTNode* StructuredBindingASTNodeClone(
    const ASTNode* node, ASTNode* (*func)(ASTNode* node, void*), void* data) {
  StructuredBindingASTNode* from = (StructuredBindingASTNode*)node;
  StructuredBindingASTNode* to =
      ASTArenaAlloc(sizeof(StructuredBindingASTNode));
  ASTNodeBaseCopy(&to->base, node);
  to->declared_type = TypeRecordCopy(from->declared_type);
  to->names = NewVector();
  to->symbols = NewVector();
  for (size_t i = 0; i < from->names->length; i++) {
    String* name = from->names->value.p[i];
    VectorAppend(to->names, NewString(name != NULL ? name->value : ""));
  }
  for (size_t i = 0; i < from->symbols->length; i++) {
    VectorAppend(to->symbols, from->symbols->value.p[i]);
  }
  to->initializer = ASTNodeClone(from->initializer, func, data, &to->base);
  return func(&to->base, data);
}

static void StructuredBindingASTNodeVisit(ASTNode* node,
                                          void (*func)(ASTNode* node, void*,
                                                       int, VisitorMode),
                                          int child_id,
                                          void* data) {
  StructuredBindingASTNode* binding = (StructuredBindingASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(binding->initializer, func, 0, data);
  func(node, data, child_id, kVisitPostChildren);
}

static void StructuredBindingASTNodeTransform(ASTNode* node,
                                              ASTNodeTransformer func,
                                              void* data) {
  StructuredBindingASTNode* binding = (StructuredBindingASTNode*)node;
  ASTNodeTransformChild(node, 0, binding->initializer, func, data);
}

static ASTNodeVirtuals structured_binding_vtbl = {
    StructuredBindingASTNodeDelete, StructuredBindingASTNodePrint,
    StructuredBindingASTNodeReplaceChild, StructuredBindingASTNodeClone,
    StructuredBindingASTNodeVisit, ValueAlwaysUsed,
    StructuredBindingASTNodeTransform};

ASTNode* NewStructuredBindingASTNode(TypeRecord* declared_type, Vector* names,
                                     Vector* symbols,
                                     ASTNode* initializer,
                                     SourceLocation location) {
  StructuredBindingASTNode* node =
      ASTArenaAlloc(sizeof(StructuredBindingASTNode));
  ASTNodeInit(&node->base, AST_OP(structured_binding), declared_type, location,
              &structured_binding_vtbl);
  node->declared_type = declared_type;
  node->names = names;
  node->symbols = symbols;
  node->initializer = initializer;
  if (initializer != NULL) {
    initializer->parent = (ASTNode*)node;
    initializer->child_id = 0;
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

static void DeclarationListASTNodeTransform(ASTNode* node,
                                            ASTNodeTransformer func,
                                            void* data) {
  DeclarationListASTNode* n = (DeclarationListASTNode*)node;
  for (size_t i = n->declarations->length; i > 0; i--) {
    ASTNodeTransformVectorElement(node, n->declarations, i - 1, func, data);
  }
}

static ASTNodeVirtuals decl_list_vtbl = {
    DeclarationListASTNodeDelete, DeclarationListASTNodePrint, NULL,
    DeclarationListASTNodeClone, DeclarationListASTNodeVisit, ValueNotUsed,
    DeclarationListASTNodeTransform};

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

static void CaseLabelASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                      void* data) {
  CaseLabelASTNode* n = (CaseLabelASTNode*)node;
  ASTNodeTransformChild(node, 0, n->expr, func, data);
  ASTNodeTransformChild(node, 1, n->stmt, func, data);
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
    CaseLabelASTNodeVisit, ValueAlwaysUsed, CaseLabelASTNodeTransform};

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

static void SwitchStatementASTNodeTransform(ASTNode* node,
                                            ASTNodeTransformer func,
                                            void* data) {
  SwitchStatementASTNode* n = (SwitchStatementASTNode*)node;
  ASTNodeTransformChild(node, 0, n->expr, func, data);
  ASTNodeTransformChild(node, 1, n->stmt, func, data);
}

static bool SwitchStatementASTNodeUsesValue(ASTNode* node, ASTNode* value) {
  SwitchStatementASTNode* n = (SwitchStatementASTNode*)node;
  return n->expr == value;
}

static ASTNodeVirtuals switch_stmt_vtbl = {
    SwitchStatementASTNodeDelete, SwitchStatementASTNodePrint,
    SwitchStatementASTNodeReplaceChild, SwitchStatementASTNodeClone,
    SwitchStatementASTNodeVisit, SwitchStatementASTNodeUsesValue,
    SwitchStatementASTNodeTransform};

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
  // IR labels belong to one generated function instance. A cloned AST must
  // allocate a fresh IR label when it is emitted; retaining the source label
  // leaves jumps pointing at freed IR after an already-generated inline body
  // is cloned again.
  to->label = NULL;
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

static void LabelASTNodeTransform(ASTNode* node, ASTNodeTransformer func,
                                  void* data) {
  LabelASTNode* n = (LabelASTNode*)node;
  ASTNodeTransformChild(node, 0, n->stmt, func, data);
}

static ASTNodeVirtuals label_vtbl = {LabelASTNodeDelete, LabelASTNodePrint,
                                     LabelASTNodeReplaceChild,
                                     LabelASTNodeClone, LabelASTNodeVisit,
                                     NULL, LabelASTNodeTransform};

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

static void ExpressionInitializerASTNodeTransform(ASTNode* node,
                                                  ASTNodeTransformer func,
                                                  void* data) {
  ExpressionInitializerASTNode* n = (ExpressionInitializerASTNode*)node;
  ASTNodeTransformChild(node, 0, n->expr, func, data);
}

static ASTNodeVirtuals expr_init_vtbl = {
    ExpressionInitializerASTNodeDelete, ExpressionInitializerASTNodePrint,
    ExpressionInitializerASTNodeReplaceChild, ExpressionInitializerASTNodeClone,
    ExpressionInitializerASTNodeVisit, ValueAlwaysUsed,
    ExpressionInitializerASTNodeTransform};

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

static void BracedInitializerASTNodeTransform(ASTNode* node,
                                              ASTNodeTransformer func,
                                              void* data) {
  BracedInitializerASTNode* n = (BracedInitializerASTNode*)node;
  for (size_t i = n->initializers->length; i > 0; i--) {
    ASTNodeTransformVectorElement(node, n->initializers, i - 1, func, data);
  }
}

static ASTNodeVirtuals braced_init_vtbl = {
    BracedInitializerASTNodeDelete, BracedInitializerASTNodePrint,
    BracedInitializerASTNodeReplaceChild, BracedInitializerASTNodeClone,
    BracedInitializerASTNodeVisit, NULL, BracedInitializerASTNodeTransform};

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
  d->is_resolved_member = false;
  TypeRecordIncRef(type);
  return d;
}

Designator* NewStructDesignator(String* member) {
  Designator* d = malloc(sizeof(Designator));
  d->designator_type = kDesignatorStruct;
  d->value.struct_member_name = member;
  d->type = NULL;
  d->is_resolved_member = false;
  return d;
}

Designator* NewStructMemberDesignator(StructMember* member) {
  Designator* d = malloc(sizeof(Designator));
  d->designator_type = kDesignatorStruct;
  d->value.struct_member = member;
  d->type = NULL;
  d->is_resolved_member = true;
  return d;
}

Designator* NewCXXBaseDesignator(CXXBaseSpecifier* base) {
  Designator* d = malloc(sizeof(Designator));
  d->designator_type = kDesignatorBase;
  d->value.base = base;
  d->type = NULL;
  d->is_resolved_member = true;
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
      } else if (d->designator_type == kDesignatorStruct) {
        if (d->is_resolved_member && d->value.struct_member != NULL) {
          fprintf(fp,".%s", d->value.struct_member->symbol->name.value);
        } else if (!d->is_resolved_member &&
                   d->value.struct_member_name != NULL) {
          fprintf(fp,".%s", d->value.struct_member_name->value);
        }
      } else {
        fprintf(fp, ".<base>");
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
    to_d->array_index_end = from_d->array_index_end;
    to_d->is_resolved_member = from_d->is_resolved_member;
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

static void DesignatedInitializerASTNodeTransform(ASTNode* node,
                                                  ASTNodeTransformer func,
                                                  void* data) {
  DesignatedInitializerASTNode* n = (DesignatedInitializerASTNode*)node;
  ASTNodeTransformChild(node, 0, n->init, func, data);
}

static ASTNodeVirtuals designated_init_vtbl = {
    DesignatedInitializerASTNodeDelete, DesignatedInitializerASTNodePrint,
    DesignatedInitializerASTNodeReplaceChild, DesignatedInitializerASTNodeClone,
    DesignatedInitializerASTNodeVisit, ValueAlwaysUsed,
    DesignatedInitializerASTNodeTransform};

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
  if (dot_or_arrow->right == NULL ||
      dot_or_arrow->right->op != AST_OP(structmember)) {
    return false;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)dot_or_arrow->right;
  return member_node->member != NULL &&
         StructMemberIsBitField(member_node->member);
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
  ASTNode* old = NULL;
  if (child_id == 0) {
    old = node->sym;
    node->sym = child;
  } else {
    old = node->initializer;
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

static void CompoundLiteralASTNodeVisit(ASTNode* node,
                                        void (*func)(ASTNode* node, void*, int,
                                                     VisitorMode),
                                        int child_id, void* data) {
  CompoundLiteralASTNode* n = (CompoundLiteralASTNode*)node;
  func(node, data, child_id, kVisitPreChildren);
  ASTNodeVisit(n->sym, func, 0, data);
  ASTNodeVisit(n->initializer, func, 1, data);
  func(node, data, child_id, kVisitPostChildren);
}

static void CompoundLiteralASTNodeTransform(ASTNode* node,
                                            ASTNodeTransformer func,
                                            void* data) {
  CompoundLiteralASTNode* n = (CompoundLiteralASTNode*)node;
  ASTNodeTransformChild(node, 0, n->sym, func, data);
  ASTNodeTransformChild(node, 1, n->initializer, func, data);
}

static ASTNodeVirtuals compound_literal_vtbl = {
    CompoundLiteralASTNodeDelete, CompoundLiteralASTNodePrint,
    CompoundLiteralASTNodeReplaceChild, CompoundLiteralASTNodeClone,
    CompoundLiteralASTNodeVisit, NULL, CompoundLiteralASTNodeTransform};

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

ASTNodeShape ASTNodeGetShape(const ASTNode* node) {
  ASTNodeVirtuals* v = node->virtuals;
  if (v == &unary_vtbl) return kASTShapeUnary;
  if (v == &binary_vtbl) return kASTShapeBinary;
  if (v == &inline_call_vtbl) return kASTShapeInlineCall;
  if (v == &vector_vtbl) return kASTShapeVector;
  if (v == &requires_expr_vtbl) return kASTShapeRequiresExpr;
  if (v == &identifier_vtbl) return kASTShapeIdentifier;
  if (v == &struct_member_vtbl) return kASTShapeStructMember;
  if (v == &constant_vtbl) return kASTShapeConstant;
  if (v == &cast_vtbl) return kASTShapeCast;
  if (v == &sizeof_vtbl) return kASTShapeSizeof;
  if (v == &typeid_vtbl) return kASTShapeTypeid;
  if (v == &macro_vtbl) return kASTShapeMacro;
  if (v == &expr_stmt_vtbl) return kASTShapeExprStmt;
  if (v == &static_assert_vtbl) return kASTShapeStaticAssert;
  if (v == &if_stmt_vtbl) return kASTShapeIf;
  if (v == &combined_stmt_vtbl) return kASTShapeCombined;
  if (v == &throw_vtbl) return kASTShapeThrow;
  if (v == &compound_stmt_vtbl) return kASTShapeCompound;
  if (v == &catch_vtbl) return kASTShapeCatch;
  if (v == &try_vtbl) return kASTShapeTry;
  if (v == &for_stmt_vtbl) return kASTShapeFor;
  if (v == &var_decl_vtbl) return kASTShapeVarDecl;
  if (v == &decl_list_vtbl) return kASTShapeDeclList;
  if (v == &case_label_vtbl) return kASTShapeCaseLabel;
  if (v == &switch_stmt_vtbl) return kASTShapeSwitch;
  if (v == &label_vtbl) return kASTShapeLabel;
  if (v == &asm_vtbl) return kASTShapeAsm;
  if (v == &goto_vtbl) return kASTShapeGoto;
  if (v == &ptr_scale_vtbl) return kASTShapePtrScale;
  if (v == &expr_init_vtbl) return kASTShapeExprInit;
  if (v == &braced_init_vtbl) return kASTShapeBracedInit;
  if (v == &designated_init_vtbl) return kASTShapeDesignatedInit;
  if (v == &compound_literal_vtbl) return kASTShapeCompoundLiteral;
  return kASTShapeBase;
}

ASTNode* ASTNodeAllocForShape(ASTNodeShape shape, ASTOpcode op) {
  switch (shape) {
    case kASTShapeUnary: {
      UnaryASTNode* n = ASTArenaAlloc(sizeof(UnaryASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &unary_vtbl);
      return &n->base;
    }
    case kASTShapeBinary: {
      BinaryASTNode* n = ASTArenaAlloc(sizeof(BinaryASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &binary_vtbl);
      return &n->base;
    }
    case kASTShapeInlineCall: {
      InlineCallASTNode* n = ASTArenaAlloc(sizeof(InlineCallASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &inline_call_vtbl);
      return &n->base;
    }
    case kASTShapeVector: {
      VectorASTNode* n = ASTArenaAlloc(sizeof(VectorASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &vector_vtbl);
      return &n->base;
    }
    case kASTShapeRequiresExpr: {
      RequiresExpressionASTNode* n =
          ASTArenaAlloc(sizeof(RequiresExpressionASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &requires_expr_vtbl);
      return &n->base;
    }
    case kASTShapeIdentifier: {
      IdentifierASTNode* n = ASTArenaAlloc(sizeof(IdentifierASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &identifier_vtbl);
      return &n->base;
    }
    case kASTShapeStructMember: {
      StructMemberASTNode* n = ASTArenaAlloc(sizeof(StructMemberASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &struct_member_vtbl);
      return &n->base;
    }
    case kASTShapeConstant: {
      ConstantASTNode* n = ASTArenaAlloc(sizeof(ConstantASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &constant_vtbl);
      return &n->base;
    }
    case kASTShapeCast: {
      CastASTNode* n = ASTArenaAlloc(sizeof(CastASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &cast_vtbl);
      return &n->base;
    }
    case kASTShapeSizeof: {
      SizeofASTNode* n = ASTArenaAlloc(sizeof(SizeofASTNode));
      ASTNodeInit(&n->base.base, op, NULL, 0, &sizeof_vtbl);
      return &n->base.base;
    }
    case kASTShapeTypeid: {
      TypeidASTNode* n = ASTArenaAlloc(sizeof(TypeidASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &typeid_vtbl);
      return &n->base;
    }
    case kASTShapeMacro: {
      MacroNameASTNode* n = ASTArenaAlloc(sizeof(MacroNameASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &macro_vtbl);
      StringInit(&n->macro_name, NULL);
      return &n->base;
    }
    case kASTShapeExprStmt: {
      ExpressionStatementASTNode* n =
          ASTArenaAlloc(sizeof(ExpressionStatementASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &expr_stmt_vtbl);
      return &n->base;
    }
    case kASTShapeStaticAssert: {
      StaticAssertASTNode* n = ASTArenaAlloc(sizeof(StaticAssertASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &static_assert_vtbl);
      StringInit(&n->message, NULL);
      return &n->base;
    }
    case kASTShapeIf: {
      IfStatementASTNode* n = ASTArenaAlloc(sizeof(IfStatementASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &if_stmt_vtbl);
      return &n->base;
    }
    case kASTShapeCombined: {
      CombinedStatementASTNode* n =
          ASTArenaAlloc(sizeof(CombinedStatementASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &combined_stmt_vtbl);
      return &n->base;
    }
    case kASTShapeThrow: {
      ThrowASTNode* n = ASTArenaAlloc(sizeof(ThrowASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &throw_vtbl);
      return &n->base;
    }
    case kASTShapeCompound: {
      CompoundStatementASTNode* n =
          ASTArenaAlloc(sizeof(CompoundStatementASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &compound_stmt_vtbl);
      return &n->base;
    }
    case kASTShapeCatch: {
      CatchASTNode* n = ASTArenaAlloc(sizeof(CatchASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &catch_vtbl);
      return &n->base;
    }
    case kASTShapeTry: {
      TryASTNode* n = ASTArenaAlloc(sizeof(TryASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &try_vtbl);
      return &n->base;
    }
    case kASTShapeFor: {
      ForStatementASTNode* n = ASTArenaAlloc(sizeof(ForStatementASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &for_stmt_vtbl);
      return &n->base;
    }
    case kASTShapeVarDecl: {
      VariableDeclarationASTNode* n =
          ASTArenaAlloc(sizeof(VariableDeclarationASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &var_decl_vtbl);
      n->symbol = NULL;
      n->initializer = NULL;
      n->local_static_guard = NULL;
      n->local_static_init_kind = kLocalStaticInitNone;
      n->saved_sp = NULL;
      return &n->base;
    }
    case kASTShapeDeclList: {
      DeclarationListASTNode* n =
          ASTArenaAlloc(sizeof(DeclarationListASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &decl_list_vtbl);
      return &n->base;
    }
    case kASTShapeCaseLabel: {
      CaseLabelASTNode* n = ASTArenaAlloc(sizeof(CaseLabelASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &case_label_vtbl);
      return &n->base;
    }
    case kASTShapeSwitch: {
      SwitchStatementASTNode* n =
          ASTArenaAlloc(sizeof(SwitchStatementASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &switch_stmt_vtbl);
      VectorInit(&n->cases);
      return &n->base;
    }
    case kASTShapeLabel: {
      LabelASTNode* n = ASTArenaAlloc(sizeof(LabelASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &label_vtbl);
      StringInit(&n->name, NULL);
      return &n->base;
    }
    case kASTShapeAsm: {
      AsmASTNode* n = ASTArenaAlloc(sizeof(AsmASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &asm_vtbl);
      VectorInit(&n->outputs);
      VectorInit(&n->inputs);
      VectorInit(&n->clobbers);
      VectorInit(&n->labels);
      VectorInit(&n->label_nodes);
      return &n->base;
    }
    case kASTShapeGoto: {
      GotoStatementASTNode* n = ASTArenaAlloc(sizeof(GotoStatementASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &goto_vtbl);
      return &n->base;
    }
    case kASTShapePtrScale: {
      PtrScaleASTNode* n = ASTArenaAlloc(sizeof(PtrScaleASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &ptr_scale_vtbl);
      return &n->base;
    }
    case kASTShapeExprInit: {
      ExpressionInitializerASTNode* n =
          ASTArenaAlloc(sizeof(ExpressionInitializerASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &expr_init_vtbl);
      return &n->base;
    }
    case kASTShapeBracedInit: {
      BracedInitializerASTNode* n =
          ASTArenaAlloc(sizeof(BracedInitializerASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &braced_init_vtbl);
      return &n->base;
    }
    case kASTShapeDesignatedInit: {
      DesignatedInitializerASTNode* n =
          ASTArenaAlloc(sizeof(DesignatedInitializerASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &designated_init_vtbl);
      return &n->base;
    }
    case kASTShapeCompoundLiteral: {
      CompoundLiteralASTNode* n =
          ASTArenaAlloc(sizeof(CompoundLiteralASTNode));
      ASTNodeInit(&n->base, op, NULL, 0, &compound_literal_vtbl);
      return &n->base;
    }
    case kASTShapeBase:
    default: {
      ASTNode* n = ASTArenaAlloc(sizeof(ASTNode));
      ASTNodeInit(n, op, NULL, 0, &base_vtbl);
      return n;
    }
  }
}

